#include "coroutine.h"
#include <iostream>

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