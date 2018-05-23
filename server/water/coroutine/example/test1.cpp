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
    {
        m[corot::this_corot::getId()] = 0;
        cout << "tid:" << corot::this_corot::getId() << ", produce 1 goods, total=" << g << endl;
        //corot::resume(coc);
        m[corot::this_corot::getId()] = 0;
        corot::this_corot::yield();
    }
}

void c()
{
    for (auto iter = m.begin(); iter != m.end();)
    {
        iter = m.erase(iter);
    }

    cout << "main corot id: " << corot::this_corot::getId() << endl;
}




int main()
{
//    cop = corot::create(p);
//    coc = corot::create(c);
 /*   corot::run(cop);
a*/
    for(int i = 0; i < 1000; ++i)
        corot::create(p);

    while(corot::schedule())
    {
        cout << "main corot id while: " << corot::this_corot::getId() << endl;
        corot::create(p);
        corot::create(p);
        std::this_thread::sleep_for(std::chrono::seconds(5));
        c();
    }
    return 0;
}
