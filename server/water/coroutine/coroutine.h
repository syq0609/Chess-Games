/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-05-27 14:38 +0800
 *
 * Description: 轻量化的c++协程库, 使用调度器对协程进行统一调度
 *              采用共享栈技术，支持开启海量协程
 *
 *              特性简介:
 *                  1, 代码中链接本协程库之后, 所有的代码都在协程中执行
 *                  2, 可以在协程内调用this_corot::getId()来获取运行当前代码的corotId
 *                  3, 线程启动后, 线程入口函数的代码在main corot中执行, 其corotid == corot::MAIN_COROT_ID
 *                  4, corot的调度需要通过在main corot中调用corot::schedule()或corot::resume()来完成, 有两种方式:
 *                      a) 循环调用schedule(), 即可实现对本线程中所有协程的自动调度, 
 *                         此时同一thread内的corotos间的关系, 与同一process内的同一优先级threads的行为类似, 都会得到均等的执行机会.
 *                      b) 调用resume() 即可启动或恢复指定线程的执行
 *                  5, 任意线程通过调用corot::this_corot::yield()将执行权限交还给main corot
 *                  6, 可以在任意corot中调用corot::create()来创建新的corot
 */

#include <cstdint>
#include <functional>

namespace corot
{
using CorotId = uint32_t;

extern const CorotId MAIN_COROT_ID; //mian corot 的 id

//创建协程, 返回被创建的协程的id
CorotId create(const std::function<void(void)>& func);

//强制销毁一个corot, 不建议使用
//与线程类似，corot会在启动函数正常return后自动销毁
void destroy(CorotId coid);

//对当前正在运行的所有 corots 进行一次自动调度, 
//返回当前存在的sub corot 数量
//仅在main corot内调用才有效
uint32_t schedule();

//运行指定的corot
//仅在main corot内调用才有效 
bool resume(CorotId coid);

//销毁一个corot
//尽在main corot中调用有效
//void destroy(CorotId coid);

namespace this_corot
{

//获取当前corot的id
CorotId getId();
//将执行权让出到main corot
void yield();
}

}

