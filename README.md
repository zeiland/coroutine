# coroutine
A symmetric coroutine library for c++.


目前大部分协程的开源代码是非对称的，如云风的https://github.com/cloudwu/coroutine 。
我个人认为协程应该像线程一样，具有一定程度的自动调度功能。出于学习的目的，我实现了一个简单的对称协程。

At present, most of the open source codes of the coroutine are asymmetric, such as https://github.com/cloudwu/coroutine.
Personally, I think that the coroutines should have a certain degree of automatic scheduling function, just like threads.For the purpose of learning, I implement a simple symmetric coroutine.

这是一个使用ucontext的对称协程简单实现。

This is a simple implementation of symmetric coroutine based on ucontext.

每个协程拥有自己的运行栈，大小可通过STACK_SIZE调整。

Each protocol has its own running stack, and its size can be adjusted by STACK_SIZE.

每个协程有Ready,Suspend,Dead三种状态。

Each process has three states: Ready, Suspend and Dead.

你可以通过调用yield让协程自动调度，也可以使用resume来进行主动调度。

You can call yield to schedule coroutine automatically, or you can use resume to do active scheduling.

Channel类可以在协程间传递信息。

Channel class can pass information between coroutines.

api:
```
int createCoroutine(CoroutineFunc func)
void resumeCoroutine(int id)
void yieldCoroutine()
int getIdCoroutine()
template<typename T> Channel<T>* createChannel()
template<typename T> void deleteChannel(Channel<T>* c)
```

example：

```
int main(){
    auto channel1 = coroutine::createChannel<int>();
    auto channel2 = coroutine::createChannel<int>();
    auto func1 = [channel1](){
        for(int i=0;i<5;++i){
            int id = coroutine::getIdCoroutine();
            std::cout <<"id = "<< id << " "<<"n = " << i <<std::endl;
            channel1->push(i);
        }
    };

    auto func2 = [channel1,channel2](){
        while(1){
            int id = coroutine::getIdCoroutine();
            int i = channel1->pop();
            std::cout <<"id = "<< id << " "<<"2n = " << 2 * i <<std::endl;
            channel2->push(2 * i);
        }
    };

    auto func3 = [channel2](){
        while(1){
            int id = coroutine::getIdCoroutine();
            int i = channel2->pop();
            std::cout <<"id = "<< id << " "<<"4n = " << 2 * i <<std::endl;
        }
    };

    int id1 = coroutine::createCoroutine(func1);
    int id2 = coroutine::createCoroutine(func2);
    int id3 = coroutine::createCoroutine(func3);

    for(int i=0; i<100; ++i){
        coroutine::yieldCoroutine();
    }

    coroutine::deleteChannel(channel1);
    coroutine::deleteChannel(channel2);
    return 0;
}
```
