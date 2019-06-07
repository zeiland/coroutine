#ifndef COROUTINE_H_
#define COROUTINE_H_

#include <functional>
#include <queue>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <iterator>


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#if __APPLE__ && __MACH__
	#include <sys/ucontext.h>
#else 
	#include <ucontext.h>
#endif 


namespace coroutine{

constexpr int STACK_SIZE = 128 * 1024;

using CoroutineFunc = std::function<void()>;

static void proxyFunc();

class Coroutine{
public:
	friend class Schedule;

	Coroutine(CoroutineFunc func);
	~Coroutine();

	void func();

private:
	CoroutineFunc func_;     //执行函数
	ucontext_t ctx_;         //中断时保存的上下文
	char *stack_;            //栈
};


class Schedule{
public:
	friend class Coroutine;

	Schedule();

    static Schedule& getInstance();

    int createCoroutine(CoroutineFunc func);
    void resumeCoroutine(int id);
	void yieldCoroutine();
	int getIdCoroutine();

	Coroutine* getCoroutine();
	void deleteCoroutine();
	void suspendCoroutine();

private:
	int getNextId();


	ucontext_t main_;        // ctx where id = 0
	int max_id_;             
	int cur_id_;             

	std::unordered_map<int, Coroutine*> cos_;

    //if coroutine not in dead or ready, it is suspend 
	std::queue<int> dead_ids_;                
    std::unordered_set<int> ready_ids_;      

};

template<typename T>
class Channel{
public:
    void push(T data);
    T pop();
private:
    std::queue<T> data_;
    std::queue<int> wait_ids_;
};

static void proxyFunc(){
	Schedule& schedule = Schedule::getInstance();
	Coroutine* co = schedule.getCoroutine();
	co->func(); 
	schedule.deleteCoroutine();
}


Coroutine::Coroutine(CoroutineFunc func)
	:func_(func),
	 stack_(nullptr)
{
    stack_ = (char*)malloc(STACK_SIZE);
	Schedule& schedule = Schedule::getInstance();
	getcontext(&ctx_); 
	ctx_.uc_stack.ss_sp = stack_; 
	ctx_.uc_stack.ss_size = STACK_SIZE; 
	ctx_.uc_link = &schedule.main_;
	makecontext(&ctx_, (void (*)(void)) proxyFunc, 0);
}

Coroutine::~Coroutine(){
	free(stack_);
}

void Coroutine::func(){
	func_();
}


Schedule::Schedule()
	:max_id_(0),
	 cur_id_(0)
{}

Schedule& Schedule::getInstance(){
    static Schedule instance;
    return instance;
}

int Schedule::createCoroutine(CoroutineFunc func){
	int id = 0;
	if(dead_ids_.size() != 0){
		id = dead_ids_.front();
		dead_ids_.pop();
        delete cos_[id];
        cos_.erase(id);
	} else{
		id = ++max_id_;
	}

	cos_[id] = new Coroutine(func);
	ready_ids_.insert(id);

    return id;
}

void Schedule::resumeCoroutine(int id){

	if(cur_id_ == id){
		return;
	}
	assert(id == 0 || cos_.find(id) != cos_.end());

	int last_id = cur_id_;
    cur_id_ = id;

	if(last_id == 0){
		Coroutine* co = cos_[id];
		if(ready_ids_.find(id) == ready_ids_.end()){
			ready_ids_.insert(id);
		}
		swapcontext(&main_, &co->ctx_);
	} else{	
		if(id == 0){
			swapcontext(&cos_[last_id]->ctx_, &main_);
		} else{
			Coroutine* co = cos_[id];
			if(ready_ids_.find(id) == ready_ids_.end()){
				ready_ids_.insert(id);
			}
			swapcontext(&cos_[last_id]->ctx_, &co->ctx_);
		}
	}
}

void Schedule::yieldCoroutine(){
	int next_id = getNextId();
	resumeCoroutine(next_id);
}

int Schedule::getIdCoroutine(){
	return cur_id_;
}

Coroutine* Schedule::getCoroutine(){
	return cos_[cur_id_];
}

void Schedule::deleteCoroutine(){
	ready_ids_.erase(cur_id_);
	dead_ids_.push(cur_id_);

	cur_id_ = 0;
}

void Schedule::suspendCoroutine(){
    int id = getNextId();
	ready_ids_.erase(cur_id_);
    resumeCoroutine(id);
}


int Schedule::getNextId(){
	if(cur_id_ == 0){
		if(!ready_ids_.empty()){
			return *ready_ids_.begin();
		} else{
			return 0;
		}
	} else{
		auto ptr = ready_ids_.find(cur_id_);
		if(std::next(ptr) != ready_ids_.end()){
			return *std::next(ptr);
		} else{
			return 0; 
		}
	}
}

template <typename T>
void Channel<T>::push(T data){
    data_.push(data);
	if(!wait_ids_.empty()){
		int id = wait_ids_.front();
		wait_ids_.pop();
		Schedule& schedule = Schedule::getInstance();
		schedule.resumeCoroutine(id);
	}
}

template <typename T>
T Channel<T>::pop(){

	while(data_.empty()){
		Schedule& schedule = Schedule::getInstance();
		wait_ids_.push(schedule.getIdCoroutine());
		schedule.suspendCoroutine();
	}
	T data = data_.front();
	data_.pop();
	return data;
}





int createCoroutine(CoroutineFunc func){ 
    Schedule& schedule = Schedule::getInstance();
	return schedule.createCoroutine(func);
}       


void resumeCoroutine(int id){
    Schedule& schedule = Schedule::getInstance();
	schedule.resumeCoroutine(id);
}


void yieldCoroutine(){
	Schedule& schedule = Schedule::getInstance();
	schedule.yieldCoroutine();
}

int getIdCoroutine(){
	Schedule& schedule = Schedule::getInstance();
	schedule.getIdCoroutine();
}          

template<typename T>
Channel<T>* createChannel(){
    return new Channel<T>();
}

template<typename T>
void deleteChannel(Channel<T>* c){
    delete c;
}

}
#endif