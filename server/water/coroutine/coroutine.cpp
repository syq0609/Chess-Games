#include "coroutine.h"

#include <stddef.h>
#include <string.h>
#include <functional>
#include <vector>
#include <list>
#include <ucontext.h>
#include <assert.h>

#include <iostream>

using std::cout;
using std::endl;

namespace corot
{
const uint32_t STACK_SIZE = 1024 * 1024 * 20; //20mb
const CorotId MAIN_COROT_ID = -1; //默认corotid

enum class CorotStatus
{
    null,
    ready,
    running,
    sleeping,
};
struct Corot
{
    friend class Scheduler;
    CorotId id = MAIN_COROT_ID;
    //        bool isDetached = false;
    std::function<void ()> exec;
    ucontext_t ctx;
    std::vector<uint8_t> stackData;
    CorotStatus status = CorotStatus::null;
};

class Scheduler
{
public:
    Scheduler() = default;
    ~Scheduler();

    CorotId create(const std::function<void(void)>& func);

    void destroy(CorotId coid);

    void destroy(Corot* co);

    bool resumeCorot(CorotId coid);

    bool resumeCorot(Corot* co);

    void yieldRunningCorot();

    uint32_t doSchedule();

    CorotId getRunningCorotId() const;

    Corot* getRunningCorot(); //done

    Corot* getCorot(CorotId coid);

private:
    static void invoke();

private:
    std::array<uint8_t, STACK_SIZE> stack;
    std::vector<Corot*> tasks;
    CorotId runningCoid = MAIN_COROT_ID;
    std::list<CorotId> avaliableIds; //当前已经回收的taskId
    ucontext_t ctx;

public:
    static Scheduler& me();
};

Scheduler& Scheduler::me()
{
    static thread_local Scheduler* s_me = new Scheduler();
    return *s_me;

    //下面的写法, 在栈上直接构建thread_local对象, 会在进程启动时宕机, 原因尚未查明
    //static thread_local Scheduler s_me;
    //return s_me;
}

Scheduler::~Scheduler()
{
    for(auto& co : tasks)
    {
        if(co == nullptr)
            continue;
        delete co;
        co = nullptr;
    }
}

inline CorotId Scheduler::create(const std::function<void(void)>& func)
{
    Corot* co = new Corot();
    co->exec = func;

    if (this->avaliableIds.empty())
    {
        this->tasks.push_back(co);
        co->id = this->tasks.size() - 1;
        //DEBUG
        //cout << "scheduler create corot=" << runningCoid << ", sub=" << co->id << endl;
    }
    else
    {
        co->id = this->avaliableIds.front();
        this->avaliableIds.pop_front();
        this->tasks[co->id] = co;
        //DEBUG
        //cout << "create reuse new corot=" << runningCoid << ", sub=" << co->id << endl;
    }
    co->status = CorotStatus::ready;
    return co->id;
}

inline void Scheduler::destroy(CorotId coid)
{
    delete this->tasks[coid];
    this->tasks[coid] = nullptr;
    this->avaliableIds.push_back(coid);
}

inline void Scheduler::destroy(Corot* co)
{
    CorotId coid = co->id;
    delete this->tasks[coid];
    this->tasks[coid] = nullptr;
    this->avaliableIds.push_back(coid);
}

inline bool Scheduler::resumeCorot(CorotId coid)
{
    Corot* co = getCorot(coid);
    return resumeCorot(co);
}

inline bool Scheduler::resumeCorot(Corot* co)
{
    if (co == nullptr)
        return true;

    CorotId coid = co->id;
    //DEBUG
    //cout << "scheduler resume, corot=" << runningCoid << ", sub=" << coid << endl; 

    auto& taskCtx = co->ctx;
    auto& schedulerCtx = this->ctx;

    Corot* runningCorot = getRunningCorot();
    if (runningCorot != nullptr)
        runningCorot->status = CorotStatus::sleeping;

    switch (co->status)
    {
    case CorotStatus::ready:
        {
            if (-1 == getcontext(&taskCtx)) //构造一个ctx, ucontext函数族要求这样来初始化一个供makecontext函数使用的ctx
                return false;
            taskCtx.uc_stack.ss_sp = this->stack.data();   //这里使用scheduler持有的buf作为task的执行堆栈, 每个task都用这个栈来执行
            taskCtx.uc_stack.ss_size = this->stack.size();
            taskCtx.uc_link = &schedulerCtx; //taskCtx的继承者ctx的存储地址, 这个ctx中现在还是无效数据，将在下面调用swapcontext时将其填充为当前上下文
            this->runningCoid = coid;
            co->status = CorotStatus::running;
            makecontext(&taskCtx, &Scheduler::invoke, 0); //此函数要求本行必须在以上三行之后调用
            if (-1 == swapcontext(&schedulerCtx, &taskCtx)) //切换上下文到taskCtx，并把当前的ctx保存在schedulerCtx中, 实现longjump
                return false;
        }
        break; //当task 调用yield()时，会走到这里
    case CorotStatus::sleeping:
        {
            uint8_t* stackBottom = this->stack.data() + this->stack.size();
            uint8_t* stackTop = stackBottom - co->stackData.size();
            memcpy(stackTop, co->stackData.data(), co->stackData.size()); /*恢复堆栈数据到运行堆栈, 这里恢复后不调用vector::clear(), 
                                                                            因为clear()后只能节省最大一个STACK_SIZE的空间，与每次切换都重新内存的开销对比性价比太低
                                                                            */
            this->runningCoid = coid;
            co->status = CorotStatus::running;
            if (-1 == swapcontext(&schedulerCtx, &taskCtx)) //切换上下文到taskCtx，并把当前的ctx保存在schedulerCtx中, 实现longjump
                return false;
        }
        break; //当task 调用yield()时，会走到这里
    case CorotStatus::null:
        break;
    default:
        break;
    }
    return true;
}

inline void Scheduler::yieldRunningCorot()
{
    Corot* co = getRunningCorot();
    if (co == nullptr) //是main corot, 直接返回即可, 即main corot继续执行
        return;
    //DEBUG
    //cout << "scheduler yield, corotid=" << runningCoid << endl;  

    assert(static_cast<void*>(&co) > static_cast<void*>(this->stack.data())); //栈溢出检查
    /*
       下面计算当前的堆栈使用情况，并保存当前的栈。注意：
       这里的stackTop不是真正的栈顶，不包含比stackBottom变量所在的内存更高地址上的栈空间。
       同时由于现代编译器默认启用栈溢出保护(对于gcc，叫stack-protector)的特性，会调整局部变量的压栈顺序，
       所以具体哪些局部变量在比&stackBottom更高的栈上，也不一定。
       所以堆栈恢复后，不能再访问此函数内部的局部变量。简单的讲，就是这里保存的栈内存，对于当前函数而言不完整。
       但是此函数最后一行是swapcontext(), 这就保证了下次被其它地方的swapcontext切换回来后，本函数直接就要return退出了，
       本函数使用的栈内存也就不再有用了，所以本函自身的堆栈保存残缺不影响正常执行
       */
    uint8_t* stackBottom = this->stack.data() + this->stack.size(); //整个栈空间的栈底指针
    uint8_t* stackTop = reinterpret_cast<uint8_t*>(&stackBottom);    
    ptrdiff_t stackSize = stackBottom - stackTop;
    co->stackData.resize(stackSize);
    memcpy(co->stackData.data(), stackTop, stackSize);
    co->status = CorotStatus::sleeping;
    this->runningCoid = MAIN_COROT_ID;
    if (-1 == swapcontext(&(co->ctx), &(this->ctx))) //此行执行完必须return
        return;
}

inline uint32_t Scheduler::doSchedule()
{
    if (this->runningCoid != MAIN_COROT_ID)
    {
        //DEBUG
        //cout << "corot used error, scheduler is not running in main task, this_corotid=" << runningCoid << endl;
        return 0;
    }
    uint32_t ret = 0;
    for (CorotId coid = 0; coid < this->tasks.size(); ++coid)
    {
        Corot* co = this->tasks[coid];
        if (co == nullptr)
            continue;

        //DEBUG
        //cout << "scheduler auto resume, coroid=" << runningCoid <<  ", sub=" << coid << ", tasks.size=" << tasks.size() << endl;
        if(!resumeCorot(coid))
        {
            cout << "resume failed, coid=" << coid << endl;
            destroy(coid);
            continue;
        }
        ret++;
    }
    //DEBUG
    //cout << "scheduler return, coroid=" << runningCoid << ", tasks.size=" << tasks.size() << endl;
    return ret;
}

inline CorotId Scheduler::getRunningCorotId() const
{
    return runningCoid;
}

inline Corot* Scheduler::getRunningCorot() //done
{
    CorotId coid = this->runningCoid;
    return getCorot(coid);
}

Corot* Scheduler::getCorot(CorotId coid)
{
    if (coid == MAIN_COROT_ID)
        return nullptr;

    if (coid >= this->tasks.size())
        return nullptr;
    return this->tasks[coid];
}

void Scheduler::invoke()
{
    CorotId coid = Scheduler::me().runningCoid;
    Corot* co = Scheduler::me().getCorot(coid);
    co->exec();
    //co->exe()正常执行完毕退出会走到这里, 期间:
    //里面执行yield时会返回到resume函数swapcontent的下一行继续执行, 即跳转到main_corot
    //yield之后, 在主进程再次调用resume函数时会到yield函数内swapcontent的下一行继续执行, 即回到自身执行绪
    
    //DEBUG
    //cout << "schedule invoke, corot exit, this_coroid=" << coid << endl;
    Scheduler::me().destroy(coid); //协程正常退出, 销毁
    Scheduler::me().runningCoid = MAIN_COROT_ID; //控制权即将返回调度器
}


/////////////////////////////////////////////////////////////////////////////////////

uint32_t schedule()
{
    return Scheduler::me().doSchedule();
}


CorotId create(const std::function<void(void)>& func)
{
    return Scheduler::me().create(std::forward<const std::function<void(void)>&>(func));
}
/*
   void destroy(CorotId coid);
   void destroy(Corot* co);
   */


bool resume(CorotId coid)
{
    return Scheduler::me().resumeCorot(coid);
}

}

//#include <iostream>
//namespace corot{
//void dumy()
//{
//    std::cout << "TLS TEST, ptr=" << ((void*)&Scheduler::me()) << std::endl;
//}
//}

namespace corot{
namespace this_corot{
CorotId getId()
{
    return Scheduler::me().getRunningCorotId();
}

void yield()
{
    Scheduler::me().yieldRunningCorot();
}

}}


