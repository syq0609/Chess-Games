#include "../coroutine.h"

#include <thread>
#include <iostream>
#include <map>
using namespace std;

int g = 10;
bool running = true;

void c();

corot::CorotId cop;
corot::CorotId coc;

std::map<int, int> m;

void p()
{
//    corot::create(c);
    {
        m[corot::this_corot::getId()] = 0;
        cout << "tid:" << corot::this_corot::getId() << ", produce 1 goods, total=" << g << endl;
        //corot::resume(coc);
        m[corot::this_corot::getId()] = 0;
        cout << "tid:" <<  corot::this_corot::getId() << ", before yield" <<  endl;
        corot::this_corot::yield();
        cout << "tid:" <<  corot::this_corot::getId() << ", aftter yield" <<  endl;
    }
}

void c()
{
    for (auto iter = m.begin(); iter != m.end();)
    {
        iter = m.erase(iter);
    }
}




int main()
{
//    cop = corot::create(p);
//    coc = corot::create(c);
 /*   corot::run(cop);
a*/

//    for(int i = 0; i < 1000; ++i)
//        corot::create(p);


    while(true)
    {
        int i;
        cout << "timer call schedule, this_corotid=" << corot::this_corot::getId() << endl;
        cin >> i;
        corot::schedule();
        cout << "thimer call create, this_corotid=" << corot::this_corot::getId() << endl;
        cin >> i;
        corot::create(p);
//        corot::create(p);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        cout << "thimer call func(), this_corotid=" << corot::this_corot::getId() << endl;
        cin >> i;
        c();
    }
    return 0;
}
