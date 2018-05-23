#include "../fast_travel_unordered_map.h"

#include "test.h"


using namespace water;
using namespace componet;


int main()
{
    FastTravelUnorderedMap<int, int> ftum;
    ftum[1] = 1111;

    for(auto iter = ftum.begin(); iter != ftum.end();)
    {
        iter = ftum.erase(iter);
    }
    return 0;
}
