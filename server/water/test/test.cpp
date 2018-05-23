//#include "test.h"

extern "C" {

int g()
{
    return 0;
}

int f()
{
      g();
    int a[] = {1,2,3,4};
//    int b;
//    int c;
//    a = 11;
//    b = 22;
//    c = 33;
//    return c;
    //std::vector<int> vec{1, 2};
    //cout << a << "," << b << endl;
    return 0;
}

int main()
{
    f();
}

}
