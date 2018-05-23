#include "deck.h"
#include "componet/tools.h"
#include <array>

namespace lobby{

Deck::BrandInfo Deck::brandInfo(Card* c, uint32_t size)
{
    std::array<Deck::Card, 5> cards;
    int idx = 0;
    for(uint32_t i = 0; i < size; ++i)
    {
        if(c[i] <= 52)
        {
            cards[idx++] = c[i];
        }
    }

    if (size == 5)
    {
        if(idx != 5)
        {
            return ghostInfo5(cards.data(), idx);
        }
        else
        {
            return brandInfo5(c);
        }
    }
    else if (size == 3)
    {
        if(idx != 3)
        {
            return ghostInfo3(cards.data(), idx);
        }
        else
        {
            return brandInfo3(c);
        }
    }

    return {Brand::hightCard, 0};
}


Deck::BrandInfo Deck::brandInfo5(Card* c)
{
    const uint32_t size = 5;
    using IntOfRank = typename std::underlying_type<Rank>::type;


    //排序并提取suit 和 rank
    Card* end = c + size;
    std::sort(c, end, &Deck::lessCard);
    std::vector<Rank> r = { rank(c[0]), rank(c[1]), rank(c[2]), rank(c[3]), rank(c[4]) };
    std::vector<Suit> s = { suit(c[0]), suit(c[1]), suit(c[2]), suit(c[3]), suit(c[4]) };
//    std::vector<int32_t> powR = { 7529536, 537824, 38416, 2744, 196, 14, 1 }; //14的
    std::vector<int32_t> pow = { 1, 14, 196, 2744, 38416, 537824, 7529536 };
    auto rtoi = [&r](uint32_t i)->IntOfRank
    {
        return static_cast<IntOfRank>(r[i]) + 1;
    };

    // check 9, 五同
    if (r[0] == r[4])
        return {Brand::fiveOfKind, rtoi(0)};

    // 检查花色和连号情况
    bool isStright = true;
    bool isFlush  = true;
    for (uint32_t i = 1; i < size; ++i)
    {
        if (rtoi(i - 1) + 1 != rtoi(i))
            isStright = false;
        if (s[i - 1] != s[i])
            isFlush = false;
    }

    //特殊的顺子, a2345
    if (r[0] == Rank::r2 && 
        r[1] == Rank::r3 &&
        r[2] == Rank::r4 &&
        r[3] == Rank::r5 &&
        r[4] == Rank::rA)
        isStright = true;

    // check 8, 同花顺
    if (isStright && isFlush)
    {
        int32_t point = rtoi(4);
        if (r[4] == Rank::rA)
            point = (r[0] == Rank::r2) ? 14 : 15;
        return {Brand::straightFlush, point};
    }

    {// check 7, 铁支, 四条
        int32_t point = 0;
        if (r[0] == r[3])
            point = rtoi(3);
        else if(r[1] == r[4])
            point = rtoi(4);
        if (point > 0)
            return {Brand::fourOfKind, point};
    }

    {// check 6, 葫芦, 满堂红
        int32_t point = 0;
        if (r[0] == r[2] && r[3] == r[4])
           point = rtoi(2);
        else if (r[2] == r[4] && r[0] == r[1])
            point = rtoi(4);
        if (point > 0)
            return {Brand::fullHouse, point};
    }

    // check 5, 同花
    if (isFlush)
    {
        //return {Brand::flush, rtoi(4)}; //最大的牌
        {//判断其中是否存在两对
            int32_t point = 0; //rank(大对子) * 14^^6 + rank(小对子) * 14^^5 + rank(单张)
            if (r[0] == r[1] && r[2] == r[3])
                point = rtoi(3) * pow[6] + rtoi(1) * pow[5] + rtoi(4);
            else if (r[0] == r[1] && r[3] == r[4])
                point = rtoi(4) * pow[6] + rtoi(1) * pow[5] + rtoi(2);
            else if (r[1] == r[2] && r[3] == r[4])
                point = rtoi(4) * pow[6] + rtoi(2) * pow[5] + rtoi(0);
            if (point > 0)
                return {Brand::flush, point};
        }
        {//判断其中是否存在对子
            uint32_t pairIndex = -1;
            for (uint32_t i = 1; i < size; ++i)
            {
                if (r[i - 1] == r[i])
                {
                    pairIndex = i;
                    break;
                }
            }
            if (pairIndex != uint32_t(-1))
            {
                int32_t point = 0; //rank(对子) * 14^^5 + point(3张乌龙)
                point += rtoi(pairIndex) * pow[5];
                uint32_t hightCounter = 0;
                for (uint32_t i = 0; i < size; ++i)
                {
                    if (i == pairIndex || i == pairIndex - 1u)
                        continue;
                    point += rtoi(i) * pow[hightCounter++];
                }
                return {Brand::flush, point};
            }
        }

        //不包含两对也不包含对子, 那就只能是包含乌龙了
        int32_t point = 0;
        for (uint32_t i = 0; i < size; ++i)
            point += rtoi(i) * pow[i];
        return {Brand::flush, point};
    }

    // check 4, 顺子
    if (isStright)
    {
        int32_t point = rtoi(4);
        if (r[4] == Rank::rA)
            point = (r[0] == Rank::r2) ? 14 : 15;
        return {Brand::straight, point};
    }

    {// check 3, 三条
        int32_t point = 0;
        if (r[0] == r[2])
            point = rtoi(2);
        else if (r[1] == r[3])
            point = rtoi(3);
        else if (r[2] == r[4])
            point = rtoi(4);
        if (point > 0)
            return {Brand::threeOfKind, point};
    }

    {// check 2, 两对
        int32_t point = 0; //rank(大对子) * 14^^6 + rank(小对子) * 14^^5 + rank(单张)
        if (r[0] == r[1] && r[2] == r[3])
            point = rtoi(3) * pow[6] + rtoi(1) * pow[5] + rtoi(4);
        else if (r[0] == r[1] && r[3] == r[4])
            point = rtoi(4) * pow[6] + rtoi(1) * pow[5] + rtoi(2);
        else if (r[1] == r[2] && r[3] == r[4])
            point = rtoi(4) * pow[6] + rtoi(2) * pow[5] + rtoi(0);
        if (point > 0)
            return {Brand::twoPairs, point};
    }
    {// check 1, 对子
        uint32_t pairIndex = -1u;
        for (uint32_t i = 1; i < size; ++i)
        {
            if (r[i - 1] == r[i])
            {
                pairIndex = i;
                break;
            }
        }
        if (pairIndex != -1u)
        {
            int32_t point = 0; //rank(对子) * 14^^5 + point(3张乌龙)
            point += rtoi(pairIndex) * pow[5];
            uint32_t hightCounter = 0;
            for (uint32_t i = 0; i < size; ++i)
            {
                if (i == pairIndex || i == pairIndex - 1)
                    continue;
                point += rtoi(i) * pow[hightCounter++];
            }
            return {Brand::onePair, point};
        }
    }

    // check 0, 乌龙, 散牌, 高牌
    int32_t point = 0;
    for (uint32_t i = 0; i < size; ++i)
        point += rtoi(i) * pow[i];
    return {Brand::hightCard, point};
}

Deck::BrandInfo Deck::brandInfo3(Card* c)
{
    const uint32_t size = 3;
    using IntOfRank = typename std::underlying_type<Rank>::type;

    //排序并提取suit 和 rank
    Card* end = c + size;
    std::sort(c, end, &Deck::lessCard);
    std::vector<Rank> r = { rank(c[0]), rank(c[1]), rank(c[2]) };
    std::vector<Suit> s = { suit(c[0]), suit(c[1]), suit(c[2]) };
    std::vector<int32_t> pow = { 1, 14, 196, 2744, 38416, 537824, 7529536 };
    auto rtoi = [&r](uint32_t i)->IntOfRank
    {
        return static_cast<IntOfRank>(r[i]) + 2;
    };

    // check 1, 三条
    if (r[0] == r[2])
        return {Brand::threeOfKind, rtoi(2)};
    
    {// check 2, 对子
        int32_t point = 0;
        if (r[0] == r[1])
            point = rtoi(1) * pow[3] + rtoi(2);
        else if (r[1] == r[2])
            point = rtoi(2) * pow[3] + rtoi(0);
        if (point > 0)
            return {Brand::onePair, point};
    }

    // check 3, 乌龙, 散牌, 高牌
    int32_t point = 0;
    for (uint32_t i = 0; i < size; ++i)
        point += rtoi(i) * pow[i];
    return {Brand::hightCard, point};
}

bool Deck::hasGhost(Card* c, uint32_t size)
{
    for(uint32_t i = 0; i < size; ++i)
    {
        if(c[i] > 52)
            return true;
    }
    return false;
}


Deck::Brand Deck::brand(Card* c, uint32_t size)
{
    std::array<Deck::Card, 5> cards;
    int idx = 0;
    for(uint32_t i = 0; i < size; ++i)
    {
        if(c[i] <= 52)
        {
            cards[idx++] = c[i];
        }
    }

    if (size == 5)
    {
        if(idx != 5)
        {
            return ghost5(cards.data(), idx);
        }
        else
        {
            return brand5(c);
        }
    }
    else if (size == 3)
    {
        if(idx != 3)
        {
            return ghost3(cards.data(), idx);
        }
        else
        {
            return brand3(c);
        }
    }
    return Brand::hightCard;
}

//排除鬼牌计算最大牌型,size=不带鬼的牌数
Deck::BrandInfo Deck::ghostInfo3(Card* c, uint32_t size)
{
    Card* end = c + size;
    std::sort(c, end, &Deck::lessCard);

    using IntOfRank = typename std::underlying_type<Rank>::type;
    std::vector<int32_t> pow = { 1, 14, 196, 2744, 38416, 537824, 7529536 };
    auto rtoi = [&c](uint32_t i)->IntOfRank
    {
        return static_cast<IntOfRank>(rank(c[i])) + 2;
    };
    
    if(size == 0)
    {
        //3鬼变成3个A
        return {Brand::threeOfKind, static_cast<IntOfRank>(rank(52)) + 2, true};
    }

    //两鬼及以上 冲三
    if(size < 2)
        return {Brand::threeOfKind, rtoi(size - 1), true};
    
    //不相等,配鬼一对 相等,配鬼三条
    if(rank(c[0]) != rank(c[1]))
    {
        int32_t point = rtoi(1) * pow[3] + rtoi(0);
        return {Brand::onePair, point, true};
    }
    else
        return {Brand::threeOfKind, rtoi(size - 1), true}; 
}

Deck::BrandInfo Deck::ghostInfo5(Card* c, uint32_t size)
{
    Card* end = c + size;
    std::sort(c, end, &Deck::lessCard);

    using IntOfRank = typename std::underlying_type<Rank>::type;
    std::vector<int32_t> pow = { 1, 14, 196, 2744, 38416, 537824, 7529536 };
    auto rtoi = [&c](uint32_t i)->IntOfRank
    {
        return static_cast<IntOfRank>(rank(c[i])) + 1;
    };

    int32_t ghostBrandCount = 5 - size;

    //4个鬼以上,直接5同,暂时没有5个鬼
    if(size < 2)
        return {Brand::fiveOfKind, rtoi(size - 1), true};

    //首尾相同,5同
    if(rank(c[0]) == rank(c[size - 1]))
    {
        return {Brand::fiveOfKind, rtoi(size - 1), true};
    }

    //使用鬼牌检查连号
    bool isStright = true;
    bool isFlush  = true;

    for (uint32_t i = 1; i < size; ++i)
    {
        int32_t prev = static_cast<IntOfRank>(rank(c[i - 1]));
        int32_t cur = static_cast<IntOfRank>(rank(c[i]));
        int32_t v = cur - prev - 1;
        if(v == 0 || v > ghostBrandCount)
        {
            isStright = false;
        }
        else
        {
            ghostBrandCount -= v;
        }

        if(suit(c[i - 1]) != suit(c[i]))
        {
            isFlush = false;
        }
    }

    int32_t straight_point = 0;
    if(isStright)
    {
        IntOfRank maxRankVal = static_cast<IntOfRank>(rank(c[size - 1]));
        do
        {
            if(ghostBrandCount > 0)
            {
                while(ghostBrandCount > 0)
                {
                    if(static_cast<Rank>(maxRankVal) == Rank::rA)
                        break;

                    maxRankVal += 1;
                    ghostBrandCount--;
                }
            }
        }while(false);

        if(static_cast<Rank>(maxRankVal) == Rank::rA)
        {
            straight_point = 15;
        }
        else
        {
            straight_point = maxRankVal + 1;         //兼容旧函数,旧函数做了+1处理
        }
    }

    //特殊的顺子A2345
    std::vector<Rank> smallStright = { Rank::r2, Rank::r3, Rank::r4, Rank::r5, Rank::rA };
    if(!isStright && isSubset(smallStright, c, size))
    {
        isStright = true;
        straight_point = 14;
    }
    
    //同花顺
    if(isStright && isFlush)
        return {Brand::straightFlush, straight_point, true};

    //铁支
    if(size == 2)        //3鬼,不相同,直接返回铁支
    {
        return {Brand::fourOfKind, rtoi(size - 1), true};
    }

    if(rank(c[0]) == rank(c[size - 2]) || rank(c[1]) == rank(c[size - 1]))
    {
        int32_t point = 0;
        if(rank(c[0]) == rank(c[size - 2]))
            point = rtoi(size - 2);
        if(rank(c[1]) == rank(c[size - 1]))
            point = rtoi(size - 1);

        if(point > 0)
            return {Brand::fourOfKind, point, true};
    }

    //葫芦.这里逻辑属于硬写了,实在没找到规律,1鬼需要2对,2鬼需要一对,3鬼直接就是铁支了
    int32_t pairCount = 0;
    uint32_t pairIndex = 0;
    for (uint32_t i = 1; i < size; ++i)
    {
        if (rank(c[i - 1]) == rank(c[i]))
        {
            pairCount++;
            pairIndex = i;
        }
    }
    if(ghostBrandCount == 1 && pairCount == 2)
        return {Brand::fullHouse, rtoi(size - 1), true};

    if(ghostBrandCount == 2 && pairCount == 1)
        return {Brand::fullHouse, rtoi(size - 1), true};        //一对小,组单张为葫芦, 一对大,组一对为葫芦

    //同花
    if(isFlush)
    {
        if(ghostBrandCount == 2)
        {
            //2鬼,这里不会有对子
            return {Brand::flush, rtoi(size - 1) * pow[6] + rtoi(1) * pow[5] + rtoi(0), true};
        }

        if(ghostBrandCount == 1 && pairCount > 0)
        {
            //1鬼有对
            if(rank(c[size - 1]) == rank(c[size - 2]))
            {
                return {Brand::flush, rtoi(size - 1) * pow[6] + rtoi(1) * pow[5] + rtoi(0), true};
            }
            else
            {
                int32_t point = rtoi(size - 1) * pow[6];
                if(size - 2 == pairIndex)
                {
                    point += rtoi(pairIndex) * pow[5] + rtoi(0);
                }
                else
                {
                    point += rtoi(pairIndex) * pow[5] + rtoi(size - 2);
                }
                return {Brand::flush, point, true};
            }
        }
        else
        {
            //1鬼没对
            int32_t point = rtoi(size - 1) * pow[5];
            for(int i = 0; i < 3; ++i)
            {
                point += rtoi(i) * pow[i];
            }
            return {Brand::flush, point, true};
        }
    }

    //顺子
    if(isStright)
        return {Brand::straight, straight_point, true};

    //三条
    if(ghostBrandCount == 2)
    {
        return {Brand::threeOfKind, rtoi(size - 1), true};
    }
    if(pairCount > 0)   //1鬼需要对子, 2鬼最小都是三条
        return {Brand::threeOfKind, rtoi(pairIndex), true};

    //一对
    if(ghostBrandCount == 1)
        return {Brand::onePair, rtoi(size - 1), true};

    //如果可以执行到这里,肯定是异常了,不计算了
    return {Brand::onePair, 0, true};
}

//排除鬼牌计算最大牌型,size=不带鬼的牌数
Deck::Brand Deck::ghost3(Card* c, uint32_t size)
{
    //两鬼以上 冲三
    if(size < 2)
        return Brand::threeOfKind;
    
    //不相等,配鬼一对 相等,配鬼三条
    if(rank(c[0]) != rank(c[1]))
        return Brand::onePair;
    else
        return Brand::threeOfKind; 
}

Deck::Brand Deck::ghost5(Card* c, uint32_t size)
{
    using IntOfRank = typename std::underlying_type<Rank>::type;

    //4个鬼及以上,5同
    if(size < 2)
        return Brand::fiveOfKind;

    Card* end = c + size;
    std::sort(c, end, &Deck::lessCard);

    int32_t ghostBrandCount = 5 - size;

    //检查5同
    if(rank(c[0]) == rank(c[size - 1]))
    {
        return Brand::fiveOfKind;
    }

    //使用鬼牌检查连号
    bool isStright = true;
    bool isFlush  = true;

    for (uint32_t i = 1; i < size; ++i)
    {
        int32_t prev = static_cast<IntOfRank>(rank(c[i - 1]));
        int32_t cur = static_cast<IntOfRank>(rank(c[i]));
        int32_t v = cur - prev - 1;
        if(v == 0 || v > ghostBrandCount)
        {
            isStright = false;
        }
        else
        {
            ghostBrandCount -= v;
        }

        if(suit(c[i - 1]) != suit(c[i]))
        {
            isFlush = false;
        }
    }

    //特殊的顺子A2345
    std::vector<Rank> smallStright = { Rank::r2, Rank::r3, Rank::r4, Rank::r5, Rank::rA };
    if(!isStright && isSubset(smallStright, c, size))
    {
        isStright = true;
    }
    
    //同花顺
    if(isStright && isFlush)
        return Brand::straightFlush;

    //铁支
    if(size == 2)        //3鬼,不相同,直接返回铁支
    {
        return Brand::fourOfKind;
    }

    //铁支
    if(rank(c[0]) == rank(c[size - 2]) || rank(c[1]) == rank(c[size - 1]))
        return Brand::fourOfKind;

    //葫芦.这里逻辑属于硬写了,实在没找到规律,1鬼需要2对,2鬼需要一对,3鬼直接就是铁支了
    int32_t pairCount = 0;
    for (uint32_t i = 1; i < size; ++i)
    {
        if (rank(c[i - 1]) == rank(c[i]))
        {
            pairCount++;
        }
    }
    if(ghostBrandCount == 1 && pairCount == 2)
        return Brand::fullHouse;

    if(ghostBrandCount == 2 && pairCount == 1)
        return Brand::fullHouse;

    //同花
    if(isFlush)
        return Brand::flush;

    //顺子
    if(isStright)
        return Brand::straight;

    //三条
    if(pairCount > 0 || ghostBrandCount == 2)   //1鬼需要对子, 2鬼最小都是三条
        return Brand::threeOfKind;

    //一对
    if(ghostBrandCount == 1)
        return Brand::onePair;

    //如果可以执行到这里,肯定是异常了
    return Brand::hightCard;
}

bool Deck::isSubset(std::vector<Rank> v1, Card* card, int32_t size)
{
    int i = 0, j = 0;
    int m = v1.size();
    int n = size;

    if(m < n)
        return 0;

    //排过序了,两个都要排
    //Card* end = card + size;
    //std::sort(card, end, &Deck::lessCard);

    while(i < n && j < m){
        if(v1[j] < rank(card[i]))
        {
            j++;
        }
        else if(v1[j] == rank(card[i])){
            j++;
            i++;
        }
        else if(v1[j] > rank(card[i])){
            return 0;
        }
    }

    if(i < n){
        return 0;
    }
    else{
        return 1;
    }
}

Deck::Brand Deck::brand5(Card* c)
{
    const uint32_t size = 5;

    //排序并提取suit 和 rank
    Card* end = c + size;
    std::sort(c, end, &Deck::lessCard);
    std::vector<Rank> r = { rank(c[0]), rank(c[1]), rank(c[2]), rank(c[3]), rank(c[4]) };
    std::vector<Suit> s = { suit(c[0]), suit(c[1]), suit(c[2]), suit(c[3]), suit(c[4]) };

    // check 9, 五同
    if (r[0] == r[4])
        return Brand::fiveOfKind;

    // 检查花色和连号情况
    bool isStright = true;
    bool isFlush  = true;
    for (uint32_t i = 1; i < size; ++i)
    {
        using T = typename std::underlying_type<Rank>::type;
        if (static_cast<T>(r[i - 1]) + 1 != static_cast<T>(r[i]))
            isStright = false;
        if (s[i - 1] != s[i])
            isFlush = false;
    }

    //特殊的顺子, a2345
    if (r[0] == Rank::r2 && 
        r[1] == Rank::r3 &&
        r[2] == Rank::r4 &&
        r[3] == Rank::r5 &&
        r[4] == Rank::rA)
        isStright = true;

    // check 8, 同花顺
    if (isStright && isFlush)
        return Brand::straightFlush;

    // check 7, 铁支, 四条
    if ( r[0] == r[3] || r[1] == r[4] )
        return Brand::fourOfKind;

    // check 6, 葫芦, 满堂红
    if ( (r[0] == r[2] && r[3] == r[4]) ||
         (r[2] == r[4] && r[0] == r[1]) )
        return Brand::fullHouse;

    // check 5, 同花
    if (isFlush)
        return Brand::flush;

    // check 4, 顺子
    if (isStright)
        return Brand::straight;

    // check 3, 三条
    if ( r[0] == r[2] || r[2] == r[4] || r[1] == r[3] )
        return Brand::threeOfKind;

    // check 2, 两对
    if ( (r[0] == r[1] && r[2] == r[3]) || 
         (r[0] == r[1] && r[3] == r[4]) || 
         (r[1] == r[2] && r[3] == r[4]) )
        return Brand::twoPairs;

    // check 1, 对子
    for (uint32_t i = 1; i < size; ++i)
    {
        if (r[i - 1] == r[i])
            return Brand::onePair;
    }

    // check 0, 乌龙, 散牌, 高牌
    return Brand::hightCard;
}

Deck::Brand Deck::brand3(Card* c)
{
    const uint32_t size = 3;

    //排序并提取suit 和 rank
    Card* end = c + size;
    std::sort(c, end, &Deck::lessCard);
    std::vector<Rank> r = { rank(c[0]), rank(c[1]), rank(c[2]) };
    std::vector<Suit> s = { suit(c[0]), suit(c[1]), suit(c[2]) };

    // check 1, 三条
    if (r[0] == r[2])
        return Brand::threeOfKind;
    
    // check 2, 对子
    if (r[0] == r[1] || r[1] == r[2])
        return Brand::onePair;

    // check 3, 乌龙, 散牌, 高牌
    return Brand::hightCard;
}

Deck::G13SpecialBrand Deck::g13SpecialBrand(Card* c, Brand b2, Brand b3)
{
    G13SpecialBrand dun = g13SpecialBrandByDun(c, b2, b3);
    return g13SpecialBrandByAll(c, dun);
}

Deck::G13SpecialBrand Deck::g13SpecialBrandByDun(Card* c, Brand b2, Brand b3)
{
    const uint32_t size = 3;

    //排序第一墩
    std::sort(c, c + size, &Deck::lessCard);
    std::vector<Rank> r = { rank(c[0]), rank(c[1]), rank(c[2]) };
    std::vector<Suit> s = { suit(c[0]), suit(c[1]), suit(c[2]) };

    // 检查花色和连号情况
    bool isStright = ((underlying(r[0]) + 1 == underlying(r[1]) && underlying(r[1]) + 1 == underlying(r[2])) ||
                      (r[0] == Rank::r2 && r[1] == Rank::r3 && r[2] == Rank::rA));
    const bool isFlush = (s[0] == s[1] && s[1] == s[2]);

    // check 10, 三同花顺 
    if (isStright && isFlush && b2 == Brand::straightFlush && b3 == Brand::straightFlush)
        return G13SpecialBrand::tripleStraightFlush;

    // check 2, 三顺子
    if (isStright && (b2 == Brand::straight || b2 == Brand::straightFlush) && (b3 == Brand::straight || b3 == Brand::straightFlush))
        return G13SpecialBrand::tripleStraight;

    // check 1, 三同花
    if (isFlush && (b2 == Brand::flush || b2 == Brand::straightFlush) && (b3 == Brand::flush || b3 == Brand::straightFlush))
        return G13SpecialBrand::tripleFlush;

    return G13SpecialBrand::none;
}

Deck::G13SpecialBrand Deck::g13SpecialBrandByAll(Card* c, G13SpecialBrand dun)
{
    //排序全部
    const uint32_t size = 13;
    std::sort(c, c + size, &Deck::lessCard);
    std::vector<Rank> r(size);
    std::vector<Suit> s(size);
    for (uint32_t i = 0; i < size; ++i)
    {
        r[i] = rank(c[i]);
        s[i] = suit(c[i]);
    }

    // 检查花色和连号情况
    bool isStright = true;
    bool isFlush  = true;
    for (uint32_t i = 1; i < size; ++i)
    {
        using T = typename std::underlying_type<Rank>::type;
        if (static_cast<T>(r[i - 1]) + 1 != static_cast<T>(r[i]))
            isStright = false;
        if (s[i - 1] != s[i])
            isFlush = false;
    }

    // check 13, 青龙
    if (isStright && isFlush)
        return G13SpecialBrand::flushStriaght;

    // check 12, 一条龙
    if (isStright)
        return G13SpecialBrand::straight;

    // check 11, 12皇族
    if (r[0] >= Rank::rJ)
        return G13SpecialBrand::royal;

    // check 10, 三墩同花顺
    if (dun == G13SpecialBrand::tripleStraightFlush)
        return dun;

    // check 9, 三个四条, 以下判断单张分别在 0, 4, 8, 12位置
    if ( (r[1]==r[4] && r[5]==r[8] && r[9]==r[12]) ||
         (r[0]==r[3] && r[5]==r[8] && r[9]==r[12]) ||
         (r[0]==r[3] && r[4]==r[7] && r[9]==r[12]) ||
         (r[0]==r[3] && r[4]==r[7] && r[8]==r[11]) )
        return G13SpecialBrand::tripleBombs;

    // check 8, 全大
    if (r[0] >= Rank::r8)
        return G13SpecialBrand::allBig;

    // check 7, 全小
    if (r[12] <= Rank::r8)
        return G13SpecialBrand::allLittle;

    // check 6, 全红或全黑
    bool allRed = true;
    bool allBlack = true;
    for (uint32_t i = 0; i < size; ++i)
    {
        if (s[i] != Suit::heart && s[i] != Suit::diamond)
            allRed = false;
        if (s[i] != Suit::spade && s[i] != Suit::club)
            allBlack = false;
    }
    if (allRed || allBlack)
        return G13SpecialBrand::redOrBlack;

    // check 5, 四个三条
    if ( (r[1]==r[3] && r[4]==r[6] && r[7]==r[9] && r[10]==r[12]) ||
         (r[0]==r[2] && r[4]==r[6] && r[7]==r[9] && r[10]==r[12]) ||
         (r[0]==r[2] && r[3]==r[5] && r[7]==r[9] && r[10]==r[12]) ||
         (r[0]==r[2] && r[3]==r[5] && r[6]==r[8] && r[10]==r[12]) ||
         (r[0]==r[2] && r[3]==r[5] && r[6]==r[8] &&  r[9]==r[11]) )
        return G13SpecialBrand::quradThreeOfKind;

    // check 4, 五对+三条, (这个3条不在4条中)
    uint32_t threeOfKindBegin = (r[9] != r[10] && r[10] == r[11] && r[11] == r[12]) ? 10 : -1u;
    if (threeOfKindBegin == -1u)
    {
        for (uint32_t i = 0; i < size - 3;)
        {
            if (r[i] == r[i + 1] && r[i + 1] == r[i + 2])
            {
                if (r[i] != r[i + 3])
                {
                    threeOfKindBegin = i;
                    break;
                }
                i += 4;
            }
            else
            {
                i += 1;
            }
        }
    }
    if (threeOfKindBegin != -1u)//除了这3张, 剩下的正好要配5对
    {
        bool isPentaPairsAndThreeOfKind = true;
        for (uint32_t i = 0; i < size - 1; )
        {
            if (threeOfKindBegin == i) //对子的第一张牌, 直接跳过这个3条
            {
                i += 3;
                continue;
            }
            uint32_t j = i + 1;
            if (j == threeOfKindBegin) //对子的第二张牌, 也直接跳过这个3条
                j += 3;

            if (r[i] != r[j])
            {
                isPentaPairsAndThreeOfKind = false;
                break;
            }
            i = j + 1;
        }
        if (isPentaPairsAndThreeOfKind)
            return G13SpecialBrand::pentaPairsAndThreeOfKind;
    }

    // check 3, 六对半
    uint32_t pairSize = 0; 
    for (uint32_t i = 0; i < size - 1; )
    {
        if (r[i] == r[i + 1]) //当前张和下一张放一起是对子
        {
            pairSize += 1;
            i += 2;
        }
        else
        {
            i += 1;
        }
    }
    if (pairSize == 6u)
        return G13SpecialBrand::sixPairs;

    // check 1 &  check 2, 直接取dun的值即可
    return dun;
}

void Deck::getGroupCards(std::vector<int32_t>& cards, CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& stArr, BRAND_LIST& tzArr, BRAND_LIST& wtArr, BRAND_LIST& szArr, CARD_INFO_LIST& decodeArr)
{
    std::vector<int32_t> tmpArr(cards);
    sortPoker(1, tmpArr);       //TODO 鬼牌排序要处理
    cardsDecode(tmpArr, decodeArr);     //TODO 鬼牌type=4 value = 1 or 2

    int32_t num = decodeArr.size();

    CARD_INFO_LIST same, notsame, alldz;

    for(int i = num - 1; i >= 0; )
    {
        auto last = decodeArr[i];
        
        --i;
        bool find = false;
        while(i >= 0 && last.value == decodeArr[i].value)
        {
            find = true;
            same.push_back(last);
            last = decodeArr[i];
            --i;
        }

        if(find)
        {
            same.push_back(last);
            
            if(same.size() > 1)
            {
                CARD_INFO_LIST v;
                if(same.size() >= 2 && same.size() <= 5)
                {
                    for(int j = same.size() - 1; j >= 0; j--)
                    {
                        v.push_back(same[j]);
                    }
                }

                switch(same.size())
                {
                    case 2:
                        dzArr.push_back(v);
                    break;
                    case 3:
                        stArr.push_back(v);
                    break;
                    case 4:
                        tzArr.push_back(v);
                    break;
                    case 5:
                        wtArr.push_back(v);
                    break;
                    default:
                    break;
                }
            }

            for(uint32_t idx = 0; idx < same.size(); ++idx)
            {
                alldz.push_back(same[idx]);
            }

            same.clear();
        }
        notsame.push_back(last);
    }

    if(notsame.size() > 0)
    {
        for(int i = notsame.size() - 1; i >= 0; )
        {
            auto last = notsame[i];

            --i;
            CARD_INFO_LIST tmp;
            while(i >= 0 && last.value == notsame[i].value + 1)
            {
                tmp.push_back(last);
                last = notsame[i];
                --i;
            }
            tmp.push_back(last);
            if(tmp.size() >= 5) {
                szArr.push_back(tmp);
            }
            else
            {
                for(uint32_t j = 0; j < tmp.size(); ++j)
                {
                    CardInfo& ele = tmp[j];
                    bool fnd = false;
                    for(uint32_t m = 0; m < alldz.size(); ++m)
                    {
                        if(alldz[m].type == ele.type && alldz[m].value == ele.value)
                        {
                            fnd = true;
                            break;
                        }
                    }

                    if(!fnd)
                    {
                        wlArr.push_back(tmp[j]);
                    }

                }
            }
        }
    }

    reverse(wlArr.begin(), wlArr.end());
}

//该函数需要测试一下,排序结果可能和客户端js版不同
void Deck::sortPoker(int32_t sortType, std::vector<int32_t>& cardList)
{
    std::sort(cardList.begin(), cardList.end(), [&](int32_t x, int32_t y)->bool
            {
                if(sortType == 2)
                {
                    return x < y;
                }
                else if(sortType == 1)
                {
                    CardInfo xCard = singleCardsDecode(x);
                    CardInfo yCard = singleCardsDecode(y);

                    if(xCard.value < yCard.value)
                    return true;
                    if(xCard.value > yCard.value)
                    return false;

                    if(xCard.type < yCard.type)
                    return true;
                    //if(xCard.type > yCard.type)
                    //    return false;

                    return false;
                }
                return false;
            });
}

Deck::CardInfo Deck::singleCardsDecode(int32_t c)
{
    CardInfo ret;
    ret.type = floor((c - 1) / 13);         //花色
    ret.value = c - ret.type * 13;          //牌面值
    return ret;
}

int32_t Deck::singleCardsEncode(CardInfo& info)
{
    return info.type * 13 + info.value;
}

void Deck::cardsDecode(std::vector<int32_t>& cArr, CARD_INFO_LIST& arr)
{
    for(uint32_t i = 0; i < cArr.size(); ++i)
    {
        CardInfo c;
        c.type = floor((cArr[i] - 1) / 13);
        c.value = cArr[i] - c.type * 13;
        arr.push_back(c);
    }
}
void Deck::cardsEncode(CARD_INFO_LIST& arr, std::vector<int32_t>& cArr)
{
    for(auto& info : arr)
    {
        cArr.push_back(info.type * 13 + info.value);
    }
}

std::map<int32_t, Deck::CARD_INFO_LIST> Deck::getSuit(CARD_INFO_LIST& cards)
{
    std::map<int32_t, CARD_INFO_LIST> arr;
    for(uint32_t i = 0; i < cards.size(); ++i)
    {
        int32_t suit = cards[i].type;
        arr[suit].push_back(cards[i]);
    }
    return arr;
}

void Deck::_delCards(std::vector<int32_t>& src, std::vector<int32_t>& del)
{
    for(auto& lt : src)
    {
        for(auto& rt : del)
        {
            if(lt != 99 && rt != 99 && lt == rt)
            {
                lt = 99;
                rt = 99;
            }
        }
    }
    
    for(auto it = src.begin(); it != src.end(); )
    {
        if(*it == 99)
            it = src.erase(it);
        else
            ++it;
    }
}

//去重
void Deck::delSamePoint(Deck::CARD_INFO_LIST& cArr)
{
    sort(cArr.begin(), cArr.end(), [](CardInfo& lf, CardInfo& rf)->bool
            {
                return lf.value < rf.value;
            });

    cArr.erase(unique(cArr.begin(), cArr.end()), cArr.end());
}


//五同
void Deck::wutong(BRAND_LIST& wtArr, BRAND_LIST& cardsArr)
{
    if(wtArr.size() > 0)
    {
        for(int i = wtArr.size() - 1; i >= 0; --i)
        {
            cardsArr.push_back(wtArr[i]);
        }
    }
}

void Deck::tiezhi(CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& stArr, BRAND_LIST& tzArr, BRAND_LIST& cardsArr)
{
    //小于五张牌返回空
    if(tzArr.size() > 0)
    {
        if(wlArr.size() > 0)
        {
            for(int i = tzArr.size() - 1; i >= 0; --i)
            {
                CARD_INFO_LIST cardsList;
                cardsList.push_back(tzArr[i][0]);
                cardsList.push_back(tzArr[i][1]);
                cardsList.push_back(tzArr[i][2]);
                cardsList.push_back(tzArr[i][3]);
                cardsList.push_back(wlArr[0]);
                cardsArr.push_back(cardsList);
            }
        }
        else if(dzArr.size() > 0)
        {
            for(int i = tzArr.size() - 1; i >= 0; --i)
            {
                CARD_INFO_LIST cardsList;
                cardsList.push_back(tzArr[i][0]);
                cardsList.push_back(tzArr[i][1]);
                cardsList.push_back(tzArr[i][2]);
                cardsList.push_back(tzArr[i][3]);
                cardsList.push_back(dzArr[0][0]);
                cardsArr.push_back(cardsList);
            }
        }
        else if(stArr.size() > 0)
        {
            for(int i = tzArr.size() - 1; i >= 0; --i)
            {
                CARD_INFO_LIST cardsList;
                cardsList.push_back(tzArr[i][0]);
                cardsList.push_back(tzArr[i][1]);
                cardsList.push_back(tzArr[i][2]);
                cardsList.push_back(tzArr[i][3]);
                cardsList.push_back(stArr[0][0]);
                cardsArr.push_back(cardsList);
            }
        }
        else if(tzArr.size() > 1)
        {
            for(int i = tzArr.size() - 1; i >= 1; --i)
            {
                CARD_INFO_LIST cardsList;
                cardsList.push_back(tzArr[i][0]);
                cardsList.push_back(tzArr[i][1]);
                cardsList.push_back(tzArr[i][2]);
                cardsList.push_back(tzArr[i][3]);
                cardsList.push_back(tzArr[0][0]);
                cardsArr.push_back(cardsList);
            }
        }
    }
}

void Deck::tonghuashun(CARD_INFO_LIST& decodeArr, BRAND_LIST& cardsArr)
{
    //小于五张牌返回空
    //BRAND_LIST cardsList;
    std::map<int32_t, Deck::CARD_INFO_LIST> suitMap = getSuit(decodeArr);

    for(int32_t i = 3; i >= 0; --i)
    {
        delSamePoint(suitMap[i]);
        auto& suitArr = suitMap[i];
        if(suitArr.size() > 4)
        {
            if(suitArr[0].value == 13 && suitArr[1].value == 12 && suitArr[2].value == 11 && suitArr[3].value == 10 && suitArr[4].value == 9)
            {
                CARD_INFO_LIST cardsList;
                //A K Q J 10
                for(int j = 0; j < 5; ++j)
                {
                    cardsList.push_back(suitArr[j]);
                }
                cardsArr.push_back(cardsList);

            }

            int32_t len = suitArr.size();
            if(suitArr[0].value == 13 && suitArr[len - 4].value == 4 && suitArr[len - 3].value == 3 && suitArr[len - 2].value == 2 && suitArr[len - 1].value == 1)
            {
                CARD_INFO_LIST cardsList;
                //A 2 3 4 5
                cardsList.push_back(suitArr[0]);
                cardsList.push_back(suitArr[len - 1]);
                cardsList.push_back(suitArr[len - 2]);
                cardsList.push_back(suitArr[len - 3]);
                cardsList.push_back(suitArr[len - 4]);
                cardsArr.push_back(cardsList);
            }

            for(uint32_t j = 0; j < suitArr.size() - 4; ++j)
            {
                if(suitArr[j].value != 13)
                {
                    if(suitArr[j].value == (suitArr[j + 1].value + 1) && suitArr[j + 1].value == (suitArr[j + 2].value + 1) && suitArr[j + 2].value == (suitArr[j + 3].value + 1) && suitArr[j + 3].value == (suitArr[j + 4].value + 1))
                    {
                        CARD_INFO_LIST cardsList;
                        for(int k = 0; k < 5; k++)
                        {
                            cardsList.push_back(suitArr[j + k]);
                        }
                        cardsArr.push_back(cardsList);
                    }
                }
            }
        }
    }
}

//同花检测, 不存在3条
void Deck::tonghua(CARD_INFO_LIST& decodeArr, BRAND_LIST& cardsArr)
{
    std::vector<PairMix> dzNumArr;
    //小于五张牌返回空
    std::map<int32_t, Deck::CARD_INFO_LIST> suitArr = getSuit(decodeArr);
    for(int i = 3; i >= 0; --i)
    {
        if(suitArr[i].size() > 4)
        {
            PairMix pm;
            _dz(suitArr[i], pm);
            dzNumArr.push_back(pm);
        }
    }

    sort(dzNumArr.begin(), dzNumArr.end(), [](PairMix& lt, PairMix& rt)->bool
            {
                return lt.dznum > rt.dznum;
            });

    BRAND_LIST liangdui;
    BRAND_LIST yidui;
    BRAND_LIST wld;
    for(int e = dzNumArr.size() - 1; e >= 0; --e)
    {
        PairMix& pm = dzNumArr[e];
        if(pm.dznum >= 2)
        {
            if(pm.ndz.size() > 0)
            {
                //两对+1
                for(int i = 0; i < pm.dznum - 1; ++i)
                {
                    for(int j = i + 1; j < pm.dznum; ++j)
                    {
                        CARD_INFO_LIST cardsList;
                        cardsList.push_back(pm.dz[i][0]);
                        cardsList.push_back(pm.dz[i][0]);    //TODO 这是不是错了?照搬客户端
                        cardsList.push_back(pm.dz[j][0]);
                        cardsList.push_back(pm.dz[j][1]);
                        cardsList.push_back(pm.ndz[pm.ndz.size() - 1]);
                        liangdui.push_back(cardsList);
                    }
                }
            }
            else
            {
                //对子太多拆对子
                for(int i = 0; i < pm.dznum - 2; ++i)
                {
                    for(int j = i + 1; j < pm.dznum - 1; ++j)
                    {
                        CARD_INFO_LIST cardsList;
                        cardsList.push_back(pm.dz[i][0]);
                        cardsList.push_back(pm.dz[i][0]);    //TODO 这是不是错了?照搬客户端
                        cardsList.push_back(pm.dz[j][0]);
                        cardsList.push_back(pm.dz[j][1]);
                        cardsList.push_back(pm.dz[pm.dz.size() - 1][0]);
                        liangdui.push_back(cardsList);
                    }
                }
            }
        }
        else if(pm.dznum == 1)
        {
            int32_t len = pm.ndz.size();
            for(int i = 0; i < len - 2; ++i)
            {
                CARD_INFO_LIST cardsList;
                cardsList.push_back(pm.dz[0][0]);
                cardsList.push_back(pm.dz[0][1]);
                cardsList.push_back(pm.ndz[i]);
                cardsList.push_back(pm.ndz[len - 2]);
                cardsList.push_back(pm.ndz[len - 1]);
                yidui.push_back(cardsList);
            }
        }
        else
        {
            int32_t len = pm.ndz.size();
            for(int i = 0; i < len - 4; ++i)
            {
                CARD_INFO_LIST cardsList;
                cardsList.push_back(pm.ndz[i]);
                cardsList.push_back(pm.ndz[len - 4]);
                cardsList.push_back(pm.ndz[len - 3]);
                cardsList.push_back(pm.ndz[len - 2]);
                cardsList.push_back(pm.ndz[len - 1]);
                wld.push_back(cardsList);
            }
        }
    }

    sort(liangdui.begin(), liangdui.end(), [](CARD_INFO_LIST& lt, CARD_INFO_LIST& rt)->bool
            {
                if(lt[0].value > rt[0].value)
                {
                    return true;
                }
                else if(lt[0].value == rt[0].value)
                {
                    if(lt[2].value > rt[2].value)
                    {
                        return true;
                    }
                    else if(lt[2].value == rt[2].value)
                    {
                        return lt[4].value > rt[4].value;
                    }
                }
                return false;
            });

    sort(yidui.begin(), yidui.end(), [](CARD_INFO_LIST& lt, CARD_INFO_LIST& rt)->bool
            {
                if(lt[0].value > rt[0].value)
                    return true;
                else if(lt[0].value == rt[0].value)
                {
                    for(int i = 2; i < 5; ++i)
                    {
                        return lt[i].value > rt[i].value;
                    }
                }
                return false;
            });

    sort(wld.begin(), wld.end(), [](CARD_INFO_LIST& lt, CARD_INFO_LIST& rt)->bool
            {
                for(uint32_t i = 0; i < 5; ++i)
                {
                    return lt[i].value > rt[i].value;
                }
                return false;
            });

    reverse(liangdui.begin(), liangdui.end());
    reverse(yidui.begin(), yidui.end());
    reverse(wld.begin(), wld.end());

    cardsArr.insert(cardsArr.end(), liangdui.begin(), liangdui.end());
    cardsArr.insert(cardsArr.end(), yidui.begin(), yidui.end());
    cardsArr.insert(cardsArr.end(), wld.begin(), wld.end());
}

void Deck::shunzi(CARD_INFO_LIST& decodeArr, BRAND_LIST& cardsArr)
{
    std::map<int32_t, CARD_INFO_LIST> pl;
    for(uint32_t i = 0; i < decodeArr.size(); ++i)
    {
        int32_t val = 13 - decodeArr[i].value;
        if(val >= 0 && val < 13)
        {
            pl[val].push_back(decodeArr[i]);
        }
    }

    if(pl.size() >= 5)
    {
        bool akqjx = false;
        for(uint32_t j = 0; j < pl.size() - 4; ++j)
        {
            bool yes = true;
            CARD_INFO_LIST yesArr;
            for(int m = 0; m < 5; ++m)
            {
                if(pl[m + j].size() == 0)
                {
                    yes = false;
                    break;
                }
                else
                {
                    yesArr.push_back(pl[m + j][0]);
                }
            }

            if(yes)
            {
                if(j == 0)      //akqjx
                {
                    cardsArr.push_back(yesArr);
                    akqjx = true;
                }
                else
                {
                    cardsArr.push_back(yesArr);
                }
            }
        }

        if(pl[0].size() >= 1 && pl[12].size() >= 1 && pl[11].size() >= 1 && pl[10].size() >= 1 && pl[9].size() >= 1)
        {
            CARD_INFO_LIST cardsList;
            cardsList.push_back(pl[0][0]);
            cardsList.push_back(pl[12][0]);
            cardsList.push_back(pl[11][0]);
            cardsList.push_back(pl[10][0]);
            cardsList.push_back(pl[9][0]);
            if(akqjx)
            {
                cardsArr.insert(cardsArr.begin() + 1, cardsList);
            }
            else
            {
                cardsArr.insert(cardsArr.begin(), cardsList);
            }
        }
    }
}

//五张牌版三条
void Deck::santiaoFive(CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& stArr, BRAND_LIST& cardsArr)
{
    if(stArr.size() > 0)
    {
        if(wlArr.size() > 1)
        {
            for(int i = stArr.size() - 1; i >= 0; --i)
            {
                CARD_INFO_LIST cardsList;
                cardsList.push_back(stArr[i][0]);
                cardsList.push_back(stArr[i][1]);
                cardsList.push_back(stArr[i][2]);
                cardsList.push_back(wlArr[1]);
                cardsList.push_back(wlArr[0]);
                cardsArr.push_back(cardsList);
            }
        }
        else if(dzArr.size() > 1)
        {
            for(int i = stArr.size() - 1; i >= 0; --i)
            {
                CARD_INFO_LIST cardsList;
                cardsList.push_back(stArr[i][0]);
                cardsList.push_back(stArr[i][1]);
                cardsList.push_back(stArr[i][2]);
                cardsList.push_back(dzArr[1][0]);
                cardsList.push_back(dzArr[0][0]);
                cardsArr.push_back(cardsList);
            }
        }
        else if(stArr.size() > 2)
        {
            for(int i = stArr.size() - 1; i >= 2; --i)
            {
                CARD_INFO_LIST cardsList;
                cardsList.push_back(stArr[i][0]);
                cardsList.push_back(stArr[i][1]);
                cardsList.push_back(stArr[i][2]);
                cardsList.push_back(stArr[1][0]);
                cardsList.push_back(stArr[0][0]);
                cardsArr.push_back(cardsList);
            }
        }
    }
}

void Deck::liangduiFive(CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& cardsArr)
{
    if((dzArr.size() > 1 && wlArr.size() > 0) || dzArr.size() > 2)
    {
        int condition = 1;
        if(wlArr.size() == 0)
        {
            condition = 2;
        }
        for(int j = dzArr.size() - 1; j >= condition; --j)
        {
            CARD_INFO_LIST cardsList;
            cardsList.push_back(dzArr[j][0]);
            cardsList.push_back(dzArr[j][1]);
            cardsList.push_back(dzArr[0][0]);
            cardsList.push_back(dzArr[0][1]);
            if(wlArr.size() == 0)
            {
                cardsList.push_back(dzArr[1][0]);
            }
            else
            {
                cardsList.push_back(wlArr[0]);
            }
            cardsArr.push_back(cardsList);
        }
    }
}

void Deck::yiduiFive(CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& cardsArr)
{
    if(dzArr.size() > 0)
    {
        if(wlArr.size() > 2)
        {
            for(int i = dzArr.size() - 1; i >= 0; --i)
            {
                CARD_INFO_LIST cardsList;
                cardsList.push_back(dzArr[i][0]);
                cardsList.push_back(dzArr[i][1]);
                cardsList.push_back(wlArr[2]);
                cardsList.push_back(wlArr[1]);
                cardsList.push_back(wlArr[0]);
                cardsArr.push_back(cardsList);
            }
        }
        else if(wlArr.size() > 3)
        {
            for(int i = dzArr.size() - 1; i >= 3; --i)
            {
                CARD_INFO_LIST cardsList;
                cardsList.push_back(dzArr[i][1]);
                cardsList.push_back(dzArr[i][0]);
                cardsList.push_back(dzArr[2][1]);
                cardsList.push_back(dzArr[1][1]);
                cardsList.push_back(dzArr[0][1]);
                cardsArr.push_back(cardsList);
            }
        }
    }
}

void Deck::wulongFive(CARD_INFO_LIST& wlArr, BRAND_LIST& cardsArr)
{
    if(wlArr.size() > 4)
    {
        for(int i = wlArr.size() - 1; i >= 4; --i)
        {
            CARD_INFO_LIST cardsList;
            cardsList.push_back(wlArr[i]);
            cardsList.push_back(wlArr[3]);
            cardsList.push_back(wlArr[2]);
            cardsList.push_back(wlArr[1]);
            cardsList.push_back(wlArr[0]);

            bool cflag = false;
            for(uint32_t j = 1; j < cardsList.size(); ++j)
            {
                if(cardsList[j].type != cardsList[j - 1].type)      //看乌龙花色是否一样,排除清一色
                {
                    cflag = true;
                    break;
                }
            }
            if(cflag)
            {
                cflag = false;
                for(uint32_t j = 1; j < cardsList.size(); ++j)
                {
                    if(cardsList[j].value != cardsList[j - 1].value + 1)
                    {
                        cflag = true;       //看是否是顺子
                        break;
                    }
                }

                //A2345
                if(cflag && cardsList[0].value == 13 && cardsList[1].value == 4 && cardsList[2].value == 3 && cardsList[3].value == 2 && cardsList[4].value == 1)
                {
                    cflag = false;
                    
                    if(cflag)
                    {
                        cardsArr.push_back(cardsList);
                    }
                }
            }
        }
    }
}

void Deck::hulu(BRAND_LIST& dzArr, BRAND_LIST& stArr, BRAND_LIST& cardsArr)
{
    if(stArr.size() > 0)
    {
        if(dzArr.size() > 0)
        {
            for(int i = stArr.size() - 1; i >= 0; --i)
            {
                for(uint32_t j = 0; j < dzArr.size(); ++j)
                {
                    CARD_INFO_LIST cardsList;
                    cardsList.push_back(stArr[i][0]);
                    cardsList.push_back(stArr[i][1]);
                    cardsList.push_back(stArr[i][2]);
                    cardsList.push_back(dzArr[j][0]);
                    cardsList.push_back(dzArr[j][1]);
                    cardsArr.push_back(cardsList);
                }
            }
        }
        else
        {
            //没有对子,拆对子
            for(uint32_t i = stArr.size() - 1; i >= 0; --i)
            {
                for(uint32_t l = 0; l < stArr.size(); ++l)
                {
                    if(l >= i)
                    {
                        break;
                    }

                    CARD_INFO_LIST cardsList;
                    cardsList.push_back(stArr[i][0]);
                    cardsList.push_back(stArr[i][1]);
                    cardsList.push_back(stArr[i][2]);
                    cardsList.push_back(stArr[l][0]);
                    cardsList.push_back(stArr[l][1]);
                    cardsArr.push_back(cardsList);
                }
            }
        }
    }
}

void Deck::santiaoThree(BRAND_LIST& stArr, BRAND_LIST& cardsArr)
{
    for(int i = stArr.size() - 1; i >= 0; --i)
    {
        cardsArr.push_back(stArr[i]);
    }
}

void Deck::yiduiThree(CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& cardsArr)
{
    if(dzArr.size() > 0 && wlArr.size() > 0)
    {
        for(int i = dzArr.size() - 1; i >= 0; --i)
        {
            CARD_INFO_LIST cardsList;
            cardsList.push_back(dzArr[i][0]);
            cardsList.push_back(dzArr[i][1]);
            cardsList.push_back(wlArr[0]);
            cardsArr.push_back(cardsList);
        }
    }
}

void Deck::wulongThree(CARD_INFO_LIST& wlArr, BRAND_LIST& cardsArr)
{
    for(int i = wlArr.size() - 1; i >= 2; --i)
    {
        CARD_INFO_LIST cardsList;
        cardsList.push_back(wlArr[i]);
        cardsList.push_back(wlArr[i - 1]);
        cardsList.push_back(wlArr[i - 2]);
        cardsArr.push_back(cardsList);
    }
}

void Deck::_dz(CARD_INFO_LIST& arr, PairMix& pm)
{
    for(uint32_t i = 0; i < arr.size(); )
    {
        if(i < arr.size() - 1 && arr[i].value == arr[i + 1].value)
        {
            CARD_INFO_LIST cardsList;
            cardsList.push_back(arr[i]);
            cardsList.push_back(arr[i + 1]);
            pm.dz.push_back(cardsList);
            i += 2;
        }
        else
        {
            pm.ndz.push_back(arr[i]);
            i++;
        }
    }
}

void Deck::_getDuizi(std::vector<int32_t>& arr, std::vector<std::vector<int32_t>>& dz)
{
    for(uint32_t i = 0; i < arr.size() - 1; )
    {
        if(i < arr.size() - 1 && arr[i] == arr[i + 1])
        {
            std::vector<int32_t> vec;
            vec.push_back(arr[i]);
            vec.push_back(arr[i + 1]);
            dz.push_back(vec);
            i += 2;
        }
        else
        {
            i++;
        }
    }
}

bool Deck::isMessireFive(int i, CARD_INFO_LIST& a, CARD_INFO_LIST& b)
{
    std::vector<int32_t> v;
    std::vector<int32_t> s;
    v.reserve(5);
    s.reserve(5);
    for(int32_t q = 0; q < 5; ++q)
    {
        v[q] = a[q].value;
        s[q] = b[q].value;
    }

    sort(v.begin(), v.end());
    sort(s.begin(), s.end());

    if(i == 0 || i == 2)    //五同或者铁支
    {
        return v[2] < s[2];
    }
    else if(i == 3)         //葫芦(三带二)
    {
        if(v[2] == s[2])
        {
            if(v[0] == v[2] && s[0] == s[2])        //三个都在前
            {
                return v[3] < s[3];
            }
            if(v[0] == v[2] && s[2] == s[4])        //三个在前段, 三个在后端
            {
                return v[3] < s[0];
            }
            if(v[2] == v[4] && s[0] == s[2])
            {
                return v[0] < s[3];
            }
            if(v[2] == v[4] && s[2] == s[4])
            {
                return v[0] < s[0];
            }
        }
        return v[2] < s[2];
    }
    else if(i == 4)     //同花
    {
        std::vector<std::vector<int32_t>> vdz;
        std::vector<std::vector<int32_t>> sdz;
        _getDuizi(v, vdz);
        _getDuizi(s, sdz);

        if(vdz.size() == sdz.size())
        {
            if(vdz.size() > 0)
            {
                //对子数量相同
                for(int i = vdz.size() - 1; i >= 0; --i)
                {
                    if(sdz[i][0] > vdz[i][0])
                    {
                        return true;
                    }
                }
                return false;
            }

            if(vdz.size() == 0)
            {
                for(int j = 0; j < 5; ++j)
                {
                    if(v[j] == s[j])
                    {
                        continue;
                    }
                    return v[j] < s[j];
                }
            }
        }
        else
        {
            if(vdz.size() < sdz.size())
            {
                return true;
            }
        }
        return false;
    }
    else if(i == 1 || i == 5 || i == 9)     //同花顺, 顺子, 乌龙
    {
        for(int j = 0; j < 5; ++j)
        {
            if(v[j] == s[j])
            {
                continue;
            }
            return v [j] < s[j];
        }
        return false;
    }
    else if(i == 6 || i == 7 || i == 8)     //三条, 两对, 一对
    {
        std::vector<int32_t> dz_1, dz_2, wl_1, wl_2;
        for(int j = 0; j < 5; ++j)
        {
            if((j < 4 && v[j] == v[j + 1]) || (dz_1.size() > 0 && v[j] == dz_1[dz_1.size() - 1]))
            {
                dz_1.push_back(v[j]);
            }
            else
            {
                wl_1.push_back(v[j]);
            }

            if((j < 4 && s[j] == s[j + 1]) || (dz_2.size() > 0 && s[j] == dz_2[dz_2.size() - 1]))
            {
                dz_2.push_back(s[j]);
            }
            else
            {
                wl_2.push_back(s[j]);
            }
        }

        if(dz_1[0] == dz_2[0])
        {
            if(i == 6)  //三条
            {
                if(wl_1[0] == wl_2[0])
                {
                    return wl_1[1] < wl_2[1];
                }
                else
                {
                    return wl_1[0] < wl_2[0];
                }
            }
            else if(i == 7)     //两对
            {
                if(dz_1[2] == dz_2[2])
                {
                    return wl_1[0] < wl_2[0];
                }
                else
                {
                    return dz_1[2] < dz_2[2];
                }
            }
            else if(i == 8)     //一对
            {
                for(int m = 2; m < 5; ++m)
                {
                    if(wl_1[m] < wl_2[m])
                    {
                        return true;
                    }

                }
            }
        }
        else if(dz_1.size() > 0 && dz_2.size() > 0)
        {
            return dz_1[0] < dz_2[0];
        }
    }
    return false;
}

bool Deck::isMessireThree(int32_t i, CARD_INFO_LIST& a, CARD_INFO_LIST& b, int32_t j)
{
    if(i == 6)
    {
        if(a[2].value < b[2].value)
        {
            return true;
        }
    }

    if(i == 8)
    {
        if(a[0].value == b[0].value)
        {
            if(a[2].value < b[2].value)
            {
                return true;
            }
        }
        else if(a[0].value < b[0].value)
        {
            return true;
        }
    }

    if(i == 9)
    {
        if(a[0].value == b[0].value)
        {
            if(a[1].value == b[1].value)
            {
                if(a[2].value < b[2].value)
                {
                    return true;
                }
            }
            else if(a[1].value < b[1].value)
            {
                return true;
            }
        }
        else if(a[0].value < b[0].value)
        {
            return true;
        }
    }
    return false;
}

//void Deck::getRecommendPokerSet(std::vector<int32_t> cards_ori)
//{
//    CARD_INFO_LIST wlArr;       //乌龙
//    BRAND_LIST dzArr, stArr, tzArr, wtArr, szArr;       //对子, 三条, 铁支, 五同, 顺子
//    CARD_INFO_LIST decodeArr;   //牌的花色与点数
//
//    bool existArr[1000] = {0};
//
//    //遍历出13张牌中的各牌型
//    getGroupCards(cards_ori, wlArr, dzArr, stArr, tzArr, wtArr, szArr, decodeArr);
//
//    for(int32_t i = 0; i < 9; ++i)
//    {
//        BRAND_LIST cards_1; 
//
//        switch(i)
//        {
//            case 0:
//                wutong(wtArr, cards_1);
//            break;
//            case 1:
//                tonghuashun(decodeArr, cards_1);
//            break;
//            case 2:
//                tiezhi(wlArr, dzArr, stArr, tzArr, cards_1);
//            break;
//            case 3:
//                hulu(dzArr, stArr, cards_1);
//            break;
//            case 4:
//                tonghua(decodeArr, cards_1);
//            break;
//            case 5:
//                shunzi(decodeArr, cards_1);
//            break;
//            case 6:
//                santiaoFive(wlArr, dzArr, stArr, cards_1);
//            break;
//            case 7:
//                liangduiFive(wlArr, dzArr, cards_1);
//            break;
//            case 8:
//                yiduiFive(wlArr, dzArr, cards_1);
//            break;
//            default:
//            break;
//        }
//
//        for(uint32_t l = 0; l < cards_1.size(); ++l)
//        {
//            std::vector<int32_t> cards(cards_ori);
//            std::vector<int32_t> tmpCards_1;
//            cardsEncode(cards_1[l], tmpCards_1);
//            _delCards(cards, tmpCards_1);       //直接从cards中删除
//
//            wlArr.clear();
//            dzArr.clear();
//            stArr.clear();
//            tzArr.clear();
//            wtArr.clear();
//            szArr.clear();
//            decodeArr.clear();
//            getGroupCards(cards, wlArr, dzArr, stArr, tzArr, wtArr, szArr, decodeArr);
//
//            bool breakfor = false;
//            for(int32_t n = 0; n < 10; ++n)
//            {
//                BRAND_LIST cards_2; 
//
//                switch(n)
//                {
//                    case 0:
//                        wutong(wtArr, cards_2);
//                        break;
//                    case 1:
//                        tonghuashun(decodeArr, cards_2);
//                        break;
//                    case 2:
//                        tiezhi(wlArr, dzArr, stArr, tzArr, cards_2);
//                        break;
//                    case 3:
//                        hulu(dzArr, stArr, cards_2);
//                        break;
//                    case 4:
//                        tonghua(decodeArr, cards_2);
//                        break;
//                    case 5:
//                        shunzi(decodeArr, cards_2);
//                        break;
//                    case 6:
//                        santiaoFive(wlArr, dzArr, stArr, cards_2);
//                        break;
//                    case 7:
//                        liangduiFive(wlArr, dzArr, cards_2);
//                        break;
//                    case 8:
//                        yiduiFive(wlArr, dzArr, cards_2);
//                        break;
//                    case 9:
//                        wulongFive(wlArr, cards_2);
//                        break;
//                    default:
//                        break;
//                }
//
//                if(cards_2.size() > 0)
//                {
//                    if(n < i)
//                    {
//                        //牌型中墩大于尾墩了
//                        breakfor = true;
//                        break;
//                    }
//                    for(uint32_t j = 0; j < cards_2.size(); ++j)
//                    {
//                        bool cflag = false;
//                        if(i == n)          //中墩尾墩牌型一样
//                        {
//                            cflag = isMessireFive(i, cards_1[l], cards_2[j]);
//                        }
//                        if(!cflag)
//                        {
//                            std::vector<int32_t> cards_in(cards);
//                            std::vector<int32_t> tmpCards_2;
//                            cardsEncode(cards_2[j], tmpCards_2);
//                            _delCards(cards_in, tmpCards_2);
//
//                            wlArr.clear();
//                            dzArr.clear();
//                            stArr.clear();
//                            tzArr.clear();
//                            wtArr.clear();
//                            szArr.clear();
//                            decodeArr.clear();
//                            getGroupCards(cards_in, wlArr, dzArr, stArr, tzArr, wtArr, szArr, decodeArr);
//
//                            for(int k = 0; k < 3; ++k)
//                            {
//                                BRAND_LIST cards_3; 
//                                switch(k)
//                                {
//                                    case 0:
//                                        santiaoThree(stArr, cards_3);
//                                        break;
//                                    case 1:
//                                        yiduiThree(wlArr, dzArr, cards_3);
//                                        break;
//                                    case 2:
//                                        wulongThree(wlArr, cards_3);
//                                        break;
//                                    default:
//                                        break;
//                                }
//
//                                bool cflag2 = false;
//                                if(cards_3.size() > 0)
//                                {
//                                    if((k == 0 && n < 6) || (k == 1 && n < 8) || (k == 2 && n < 9))
//                                    {
//                                        cflag2 = true;
//                                    }
//                                    if((k == 0 && n == 6) || (k == 1 && n == 8) || (k == 2 && n == 9))
//                                    {
//                                        bool fl = isMessireThree(n, cards_2[j], cards_3[0], 0);
//                                        cflag2 = (fl == false);
//                                    }
//                                }
//
//                                if(cflag2)
//                                {
//                                    //同一类型只取一个
//                                    if(existArr[i * 100 + n * 10 + k] == false)
//                                    {
//                                        existArr[i * 100 + n * 10 + k] = true;
//                                        //存下来
//                                    }
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//            if(breakfor)
//            {
//                break;
//            }
//        }
//    }
//}

////带鬼牌的理牌算法
//void Deck::filterGhost(const std::vector<uint16_t>& cards, std::vector<uint16_t>& ret, int32_t& small, int32_t& big)
//{
//    ret.clear();
//    for(auto card : cards)
//    {
//        if(card == 53)
//            small++;
//        else if(card == 54)
//            big++;
//        else
//            ret.push_back(card);
//    }
//}

//bool Deck::isSubset(std::vector<Rank> v1, std::vector<Rank> v2)
//{
//    int i = 0, j = 0;
//    int m = v1.size();
//    int n = v2.size();
//
//    if(m < n)
//        return 0;
//
//    std::sort(v2.begin(), v2.end(), &Deck::lessCard);
//
//    while(i < n && j < m){
//        if(v1[j] < v2[i])
//        {
//            j++;
//        }
//        else if(v1[j] == v2[i]){
//            j++;
//            i++;
//        }
//        else if(v1[j] > v2[i]){
//            return 0;
//        }
//    }
//
//    if(i < n){
//        return 0;
//    }
//    else{
//        return 1;
//    }
//}

////arr原始数组
////start遍历起始位置
////result保存结果
////count为result数组的索引值，起辅助作用
////NUM为要选取的元素个数
//void Deck::combine_decrease(const std::vector<Card>& arr, int start, int* result, int count, const int NUM, const std::vector<Rank>& smallStright, std::vector<std::vector<Card>>& ret, bool checkSuit, bool checkRank)
//{
//	int i;
//	for (i = start; i >= count; i--)
//	{
//		result[count - 1] = i - 1;
//		if (count > 1)
//		{
//			combine_decrease(arr, i - 1, result, count - 1, NUM, smallStright, ret, checkSuit, checkRank);
//		}
//		else
//		{
//            bool flag = true;
//            if(checkSuit)                           //检查花色
//            {
//                for(int z = 0; z < NUM - 1; ++z)
//                {
//                    if(suit(arr[result[z]]) != suit(arr[result[z + 1]]))
//                    {
//                        flag = false;
//                        break;
//                    }
//                }
//            }
//            if(checkRank)                           //检查顺子
//            {
//                do
//                {
//                    //排序,头尾差值不超过4,并且没有重复
//                    Card rcard[NUM];
//                    std::set<Rank> rset;
//                    for(int k = 0; k < NUM; ++k)
//                    {
//                        rcard[k] = arr[result[k]];
//                        rset.insert(rank(arr[result[k]]));
//                    }
//                    if(rset.size() != NUM)
//                    {
//                        flag = false;
//                        break;
//                    }
//
//                    //差值大于5 并且 不是A 2 3 4 5
//                    if(abs((int32_t)rank(arr[result[NUM - 1]]) - (int32_t)rank(arr[result[0]])) > 4 && !isSubset(smallStright, rcard, NUM))
//                    {
//                        flag = false;
//                    }
//                }while(false);
//            }
//
//			if(flag)
//			{
//				std::vector<Card> v;
//				for (int j = NUM - 1; j >= 0; j--)
//					v.push_back(arr[result[j]]);
//				ret.push_back(v);
//			}
//		}
//	}
//}
//
////获取相同花色,可以组合成同花顺的
////void Deck::getTwoSuit(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret)
////{
////    std::vector<Rank> smallStright = { Rank::r2, Rank::r3, Rank::r4, Rank::r5, Rank::rA };
////    std::vector<Rank> rset;
////    std::map<int32_t, std::vector<int32_t>> suitList;
////    for(auto card : cards)
////    {
////        suitList[(int32_t)suit(card)].push_back(card);
////    }
////
////    for(auto it : suitList)
////    {
////        if(it.second.size() > 1)
////        {
////            for(uint32_t i = 0; i < it.second.size() - 1; ++i)
////            {
////                for(uint32_t j = i + 1; j < it.second.size(); ++j)
////                {
////                    rset.clear();
////                    rset.push_back(rank(it.second[i]));
////                    rset.push_back(rank(it.second[j]));
////                    if(abs((int32_t)rank(it.second[i]) - (int32_t)rank(it.second[j])) <= 4 || isSubset(smallStright, rset))
////                    {
////                        std::vector<int32_t> v;
////                        v.push_back(it.second[i]);
////                        v.push_back(it.second[j]);
////                        ret.push_back(v);
////                    }
////                }
////            }
////        }
////    }
////}
//
////TODO 这里暂不考虑3张点数相同,花色不同的牌可以组成3个不同对子的情况
////升序对子列表
//void Deck::getPair(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret)
//{
//    for(uint32_t i = 0; i < cards.size() - 1; )
//    {
//        if(rank(cards[i]) == rank(cards[i + 1]))
//        {
//            std::vector<int32_t> v;
//            v.push_back(cards[i]);
//            v.push_back(cards[i + 1]);
//            ret.push_back(v);
//            i += 2;
//        }
//        else
//        {
//            i++;
//        }
//    }
//}
//
////升序三条列表
//void Deck::getThreeBrand(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret)
//{
//    for(uint32_t i = 0; i < cards.size() - 2; )
//    {
//        if(rank(cards[i]) == rank(cards[i + 1]) && rank(cards[i]) == rank(cards[i + 2]))
//        {
//            std::vector<int32_t> v;
//            v.push_back(cards[i]);
//            v.push_back(cards[i + 1]);
//            v.push_back(cards[i + 2]);
//            ret.push_back(v);
//            i += 3;
//        }
//        else
//        {
//            i++;
//        }
//    }
//}
//
////升序四张列表
//void Deck::getFourBrand(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret)
//{
//    for(uint32_t i = 0; i < cards.size() - 3; )
//    {
//        if(rank(cards[i]) == rank(cards[i + 1]) && rank(cards[i]) == rank(cards[i + 2]) && rank(cards[i]) == rank(cards[i + 3]))
//        {
//            std::vector<int32_t> v;
//            v.push_back(cards[i]);
//            v.push_back(cards[i + 1]);
//            v.push_back(cards[i + 2]);
//            v.push_back(cards[i + 3]);
//            ret.push_back(v);
//            i += 4;
//        }
//        else
//        {
//            i++;
//        }
//    }
//}
//
//void Deck::addGhost(std::vector<int32_t>& list, int32_t small, int32_t big)
//{
//    for(int i = 0; i < (small + big); ++i)
//    {
//        if(small > 0)
//        {
//            list.push_back(53);
//            small--;
//        }
//        else if(big > 0)
//        {
//            list.push_back(54);
//            big--;
//        }
//    }
//}
//
////五同
//void Deck::getFiveOfKind(const std::vector<Card>& cards, ResultSet& rs, int32_t small, int32_t big, CARD_NUM_LIST& ret)
//{
//    ret.clear();
//    if(cards.size() < 5)
//        return;
//
//    int32_t count = small + big;
//
//    if(count == 4)
//    {
//        auto rit = noGhost.rbegin();
//        for( ; rit != noGhost.rend(); ++rit)
//        {
//            std::vector<int32_t> list;
//            list.push_back(*rit);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 3)
//    {
//        std::vector<std::vector<int32_t>> brandList;
//        getPair(noGhost, brandList);
//        auto rit = brandList.rbegin();
//        for( ; rit != brandList.rend(); ++rit)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*rit)[0]);
//            list.push_back((*rit)[1]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 2)
//    {
//        std::vector<std::vector<int32_t>> brandList;
//        getThreeBrand(noGhost, brandList);
//        auto rit = brandList.rbegin();
//        for( ; rit != brandList.rend(); ++rit)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*rit)[0]);
//            list.push_back((*rit)[1]);
//            list.push_back((*rit)[2]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 1)
//    {
//        std::vector<std::vector<int32_t>> brandList;
//        getFourBrand(noGhost, brandList);
//        auto rit = brandList.rbegin();
//        for( ; rit != brandList.rend(); ++rit)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*rit)[0]);
//            list.push_back((*rit)[1]);
//            list.push_back((*rit)[2]);
//            list.push_back((*rit)[3]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 0)
//    {
//        if(rs.wt.size() > 0)
//        {
//            for(int i = rs.wt.size() - 1; i >= 0; --i)
//            {
//                ret.push_back(rs.wt[i]);            //牌型从小到大,花色也是,最后调整鬼牌
//            }
//        }
//    }
//}
//
//void Deck::getStraightFlush(const std::vector<Card>& cards, ResultSet& rs, int32_t small, int32_t big, CARD_NUM_LIST& ret)
//{
//    ret.clear();
//    if(cards.size() < 5)
//        return;
//
//    int32_t count = small + big;
//	std::vector<Rank> smallStright = { Rank::r2, Rank::r3, Rank::r4, Rank::r5, Rank::rA };
//    
//    if(count == 4)
//    {
//        auto rit = noGhost.rbegin();
//        for( ; rit != noGhost.rend(); ++rit)
//        {
//            std::vector<int32_t> list;
//            list.push_back(*rit);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 3)
//    {
//		int num = 2;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, true, true);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//
//    }
//
//    if(count == 2)
//    {
//		int num = 3;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, true, true);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            list.push_back((*it)[2]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 1)
//    {
//		int num = 4;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, true, true);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            list.push_back((*it)[2]);
//            list.push_back((*it)[3]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 0)
//    {
//		int num = 5;
//		int result[num];
//        CARD_NUM_LIST brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, true, true);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<Card> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            list.push_back((*it)[2]);
//            list.push_back((*it)[3]);
//            list.push_back((*it)[4]);
//            ret.push_back(list);
//        }
//    }
//}
//
//void Deck::getFourOfKind(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret)
//{
//    ret.clear();
//    if(cards.size() < 5)
//        return;
//
//    int32_t small = 0;
//    int32_t big = 0;
//    std::vector<int32_t> noGhost;
//    filterGhost(cards, noGhost, small, big);
//    sort(noGhost.begin(), noGhost.end(), &Deck::lessCard);
//    int32_t count = small + big;
//	std::vector<Rank> smallStright;
//    
//    //!!!按照筛选顺序, 4张鬼, 应该直接组了五同
//    if(count == 4)
//    {
//        auto rit = noGhost.rbegin();
//        for( ; rit != noGhost.rend(); ++rit)
//        {
//            std::vector<int32_t> list;
//            list.push_back(*rit);
//            addGhost(list, small, big);			//当作4张A
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 3)
//    {
//		int num = 2;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, false, false);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            int32_t lt = (*it)[0];
//            int32_t rt = (*it)[1];
//            list.push_back(rank(lt) > rank(rt) ? lt : rt);			//取两张牌中大的那张变成铁支
//            addGhost(list, small, big);
//            list.push_back(rank(lt) > rank(rt) ? rt : lt);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 2)
//    {
//        std::vector<int32_t> wlArr;
//        CARD_NUM_LIST dzArr, stArr, tzArr, wtArr, szArr;
//        getBaseBrandType(noGhost, wlArr, dzArr, stArr, tzArr, wtArr, szArr); 
//        for(auto& dz : dzArr)
//        {
//            for(auto& wl : wlArr)
//            {
//                std::vector<int32_t> list;
//                list.push_back(dz[0]);
//                list.push_back(dz[1]);
//                addGhost(list, small, big);
//                list.push_back(wl);
//                ret.push_back(list);
//            }
//        }
//    }
//
//    if(count == 1)
//    {
//        std::vector<int32_t> wlArr;
//        CARD_NUM_LIST dzArr, stArr, tzArr, wtArr, szArr;
//        getBaseBrandType(noGhost, wlArr, dzArr, stArr, tzArr, wtArr, szArr); 
//        for(auto& st : stArr)
//        {
//            for(auto& wl : wlArr)
//            {
//                std::vector<int32_t> list;
//                list.push_back(st[0]);
//                list.push_back(st[1]);
//                list.push_back(st[2]);
//                addGhost(list, small, big);
//                list.push_back(wl);
//                ret.push_back(list);
//            }
//        }
//    }
//}
//
//void Deck::getFullHouse(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret)
//{
//    ret.clear();
//    if(cards.size() < 5)
//        return;
//
//    int32_t small = 0;
//    int32_t big = 0;
//    std::vector<int32_t> noGhost;
//    filterGhost(cards, noGhost, small, big);
//    sort(noGhost.begin(), noGhost.end(), &Deck::lessCard);
//    int32_t count = small + big;
//	//std::vector<Rank> smallStright;
//    
//    //!!!按照筛选顺序, 4张鬼, 应该直接组了五同
//    if(count == 4)
//    {
//        auto rit = noGhost.rbegin();
//        for( ; rit != noGhost.rend(); ++rit)
//        {
//            std::vector<int32_t> list;
//            list.push_back(*rit);
//            addGhost(list, small, big);     //当作3张A, 还有一鬼与其他牌组对子
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 3)
//    {
//        std::vector<int32_t> wlArr;
//        CARD_NUM_LIST dzArr, stArr, tzArr, wtArr, szArr;
//        getBaseBrandType(noGhost, wlArr, dzArr, stArr, tzArr, wtArr, szArr); 
//        for(auto& dz : dzArr)
//        {
//            std::vector<int32_t> list;
//            list.push_back(dz[0]);          //3A + 一对
//            list.push_back(dz[1]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 2)
//    {
//        std::vector<int32_t> wlArr;
//        CARD_NUM_LIST dzArr, stArr, tzArr, wtArr, szArr;
//        getBaseBrandType(noGhost, wlArr, dzArr, stArr, tzArr, wtArr, szArr); 
//
//        for(auto& dz : dzArr)
//        {
//            for(auto& wl : wlArr)
//            {
//                std::vector<int32_t> list;
//                bool flag = false;
//                if(dz[0] > wl)
//                    flag = true;
//
//                if(flag)
//                {
//                    list.push_back(dz[0]);
//                    list.push_back(dz[1]);
//                } 
//                else
//                {
//                    list.push_back(wl);
//                }
//
//                addGhost(list, small, big);
//
//                if(flag)
//                {
//                    list.push_back(wl);
//                } 
//                else
//                {
//                    list.push_back(dz[0]);
//                    list.push_back(dz[1]);
//                }
//                ret.push_back(list);
//            }
//        }
//    }
//
//    if(count == 1)
//    {
//        std::vector<int32_t> wlArr;
//        CARD_NUM_LIST dzArr, stArr, tzArr, wtArr, szArr;
//        getBaseBrandType(noGhost, wlArr, dzArr, stArr, tzArr, wtArr, szArr); 
//        if(dzArr.size() > 1 || stArr.size() > 0)
//        {
//            for(auto& st : stArr)
//            {
//                //有些逻辑应该到不了的,例如这个 3条+1鬼,走了铁支
//                for(auto& wl : wlArr)
//                {
//                    std::vector<int32_t> list;
//                    list.push_back(st[0]);
//                    list.push_back(st[1]);
//                    list.push_back(st[2]);
//                    list.push_back(wl);
//                    addGhost(list, small, big);
//                    ret.push_back(list);
//                }
//            }
//
//            for(uint32_t i = 0; i < dzArr.size() - 1; ++i)
//            {
//                for(uint32_t j = i + 1; j < dzArr.size(); ++i)
//                {
//                    std::vector<int32_t> list;
//                    int32_t lt = dzArr[i][0];
//                    int32_t rt = dzArr[j][0];
//                    list.push_back(rank(lt) > rank(rt) ? lt : rt);
//                    list.push_back(rank(lt) > rank(rt) ? dzArr[i][1] : dzArr[j][1]);
//                    addGhost(list, small, big);
//                    list.push_back(rank(lt) > rank(rt) ? rt : lt);
//                    list.push_back(rank(lt) > rank(rt) ? dzArr[j][1] : dzArr[i][1]);
//                    ret.push_back(list);
//                }
//            }
//        }
//    }
//}
//
//void Deck::getFlush(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret)
//{
//    ret.clear();
//    if(cards.size() < 5)
//        return;
//
//    int32_t small = 0;
//    int32_t big = 0;
//    std::vector<int32_t> noGhost;
//    filterGhost(cards, noGhost, small, big);
//    sort(noGhost.begin(), noGhost.end(), &Deck::lessCard);
//    int32_t count = small + big;
//
//	std::vector<Rank> smallStright;
//    
//    //!!!按照筛选顺序, 4张鬼, 应该直接组了五同
//    if(count == 4)
//    {
//        auto rit = noGhost.rbegin();
//        for( ; rit != noGhost.rend(); ++rit)
//        {
//            std::vector<int32_t> list;
//            list.push_back(*rit);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 3)
//    {
//		int num = 2;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, true, false);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            auto lt = (*it)[0];
//            auto rt = (*it)[1];
//            list.push_back(lt > rt ? lt : rt);
//            list.push_back(lt > rt ? rt : lt);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 2)
//    {
//		int num = 3;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, true, false);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            list.push_back((*it)[2]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 1)
//    {
//		int num = 4;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, true, false);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            list.push_back((*it)[2]);
//            list.push_back((*it)[3]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//}
//
////代码与同花顺没什么区别,不筛选花色
//void Deck::getStraight(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret)
//{
//    ret.clear();
//    if(cards.size() < 5)
//        return;
//
//    int32_t small = 0;
//    int32_t big = 0;
//    std::vector<int32_t> noGhost;
//    filterGhost(cards, noGhost, small, big);
//    sort(noGhost.begin(), noGhost.end(), &Deck::lessCard);
//    int32_t count = small + big;
//
//	std::vector<Rank> smallStright = { Rank::r2, Rank::r3, Rank::r4, Rank::r5, Rank::rA };
//    
//    //!!!按照筛选顺序, 4张鬼, 应该直接组了五同
//    if(count == 4)
//    {
//        //用任意单张组成最大的顺子, 例:X J X X X -> 10 J Q K A
//        auto rit = noGhost.rbegin();
//        for( ; rit != noGhost.rend(); ++rit)
//        {
//            std::vector<int32_t> list;
//            list.push_back(*rit);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 3)
//    {
//		int num = 2;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, false, true);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//
//    }
//
//    if(count == 2)
//    {
//		int num = 3;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, false, true);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            list.push_back((*it)[2]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 1)
//    {
//		int num = 4;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, false, true);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            list.push_back((*it)[2]);
//            list.push_back((*it)[3]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//}
//
//void Deck::getThreeOfKind(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret)
//{
//    ret.clear();
//    if(cards.size() < 5)
//        return;
//
//    int32_t small = 0;
//    int32_t big = 0;
//    std::vector<int32_t> noGhost;
//    filterGhost(cards, noGhost, small, big);
//    sort(noGhost.begin(), noGhost.end(), &Deck::lessCard);
//    int32_t count = small + big;
//
//	std::vector<Rank> smallStright;
//    
//    //!!!按照筛选顺序, 4张鬼, 应该直接组了五同
//    if(count == 4)
//    {
//        auto rit = noGhost.rbegin();
//        for( ; rit != noGhost.rend(); ++rit)
//        {
//            std::vector<int32_t> list;
//            list.push_back(*rit);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 3)
//    {
//		int num = 2;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, false, true);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//
//    }
//
//    if(count == 2)
//    {
//		int num = 3;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, false, true);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            list.push_back((*it)[2]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//
//    if(count == 1)
//    {
//		int num = 4;
//		int result[num];
//        std::vector<std::vector<int32_t>> brandList;
//		combine_decrease(noGhost, noGhost.size(), result, num, num, smallStright, brandList, false, true);
//		auto it = brandList.begin();
//        for( ; it != brandList.end(); ++it)
//        {
//            std::vector<int32_t> list;
//            list.push_back((*it)[0]);
//            list.push_back((*it)[1]);
//            list.push_back((*it)[2]);
//            list.push_back((*it)[3]);
//            addGhost(list, small, big);
//            ret.push_back(list);
//        }
//    }
//}
//
//void Deck::getBaseBrandType(std::vector<Card>& cards, ResultSet& rs)
//{
//	int32_t num = cards.size();
//    if(num < 5)
//        return;
//
//    std::vector<Card> same, notsame, alldz;
//
//    for(int32_t i = num - 1; i >= 0; )
//    {
//        auto last = cards[i];
//
//        --i;
//        bool find = false;
//        while(i >= 0 && rank(last) == rank(cards[i]))
//        {
//            find = true;
//            same.push_back(last);
//            last = cards[i];
//            --i;
//        }
//
//        if(find)
//        {
//            same.push_back(last);
//
//            if(same.size() > 1)
//            {
//                std::vector<Card> v;
//                if(same.size() >= 2 && same.size() <= 5)
//                {
//                    for(int j = same.size() - 1; j >= 0; j--)
//                    {
//                        v.push_back(same[j]);
//                    }
//                }
//                
//                switch(same.size())
//                {
//                    case 2:
//                        rs.dz.push_back(v);
//                    break;
//                    case 3:
//                        rs.st.push_back(v);
//                    break;
//                    case 4:
//                        rs.tz.push_back(v);
//                    break;
//                    case 5:
//                        rs.wt.push_back(v);
//                    break;
//                    default:
//                    break;
//                }
//            }
//
//            for(uint32_t idx = 0; idx < same.size(); ++idx)
//            {
//                alldz.push_back(same[idx]);
//            }
//
//            same.clear();
//        }
//        notsame.push_back(last);
//    }
//
//    if(notsame.size() > 0)
//    {
//        for(int i = notsame.size() - 1; i >= 0; )
//        {
//            auto last = notsame[i];
//
//            --i;
//            std::vector<Card> tmp;
//            while(i >= 0 && (int32_t)rank(last) == (int32_t)rank(notsame[i]) + 1)
//            {
//                tmp.push_back(last);
//                last = notsame[i];
//                --i;
//            }
//
//            tmp.push_back(last);
//            if(tmp.size() >= 5)
//            {
//                szArr.push_back(tmp);
//            }
//            else
//            {
//                for(uint32_t j = 0; j < tmp.size(); ++j)
//                {
//                    bool fnd = false;
//                    for(uint32_t m = 0; m < alldz.size(); ++m)
//                    {
//                        if(alldz[m] ==  tmp[j])
//                        {
//                            fnd = true;
//                            break;
//                        }
//                    }
//
//                    if(!fnd)
//                    {
//                        rs.wl.push_back(tmp[j]);
//                    }
//                }
//            }
//        }
//    }
//
//    reverse(rs.wl.begin(), rs.wl.end());
//}


//arr原始数组
//start遍历起始位置
//result保存结果
//count为result数组的索引值，起辅助作用
//NUM为要选取的元素个数
//void Deck::combine_decrease(const std::vector<Card>& arr, int start, int* res, int count, const int NUM, CARD_NUM_LIST& ret, int16_t type)
//{
//	int i;
//	for (i = start; i >= count; i--)
//	{
//		res[count - 1] = i - 1;
//		if (count > 1)
//		{
//			combine_decrease(arr, i - 1, res, count - 1, NUM, ret, type);
//		}
//		else
//		{
//            //穷举不同牌型
//            bool flag = true;
//            const std::vector<Rank>& smallStright = { Rank::r2, Rank::r3, Rank::r4, Rank::r5, Rank::rA };
//            switch(type)
//            {
//                //要替换成枚举
//                case 0:     //五同
//                    for(int i = 0; i < NUM - 1; ++i)
//                    {
//                        if(!compareWithGhostTrue(arr[res[i]], arr[res[i + 1]]))
//                        {
//                            flag = false;
//                            break;
//                        }
//                    }
//                    break;
//                case 1:     //同花顺
//                    std::vector<Card> vec;
//                    Card max = 0; 
//                    Card min = 0;
//                    bool isSameSuit = true;
//                    for(int i = 0; i < NUM; ++i)
//                    {
//                        if(i != NUM - 1)
//                        {
//                            //花色必须一样
//                            if(!compareWithGhostSuit(arr[res[i]], arr[res[i + 1]]))
//                            {
//                                isSameSuit = false;
//                                flag = false;
//                            }
//                            //牌面不能一样
//                            if(compareWithGhostFalse(arr[res[i]], arr[res[i + 1]]))
//                            {
//                                flag = false;
//                            }
//                        }
//
//                        Card val = arr[res[i]];
//
//                        if(val == 53 || val == 54)
//                            continue;
//
//                        if(rank(val) > rank(max))
//                            max = val;
//
//                        if(min == 0 || rank(val) < rank(min))
//                            min = val;
//
//                        vec.push_back(val);
//                    }
//
//                    //TODO 去掉强转
//                    if((int)rank(max) - (int)rank(min) > 4)
//                        flag = false;
//
//                    //特殊的顺子
//                    if(!flag && isSameSuit)
//                    {
//                        if(isSubset(smallStright, vec))
//                            flag = true;
//                    }
//                    break;
//                case 2:         //铁支
//                    flag = false;
//                    std::map<int, Card> face;
//                    for(int i = 0; i < NUM; ++i)
//                    {
//                        face[rank(arr[res[i]])]++;
//                    }
//
//                    for(auto& p : face)
//                    {
//                        if(p.first == 15)
//                        {
//                            if(p.second >= 4)
//                                flag = true;
//                        }
//                        else
//                        {
//                            if(face[15] + p.second >= 4)
//                            {
//                                flag = true;
//                                break;
//                            }
//                        }
//                    }
//                    break;
//                case 3:     //葫芦
//                    flag = false;
//                    std::map<int, Card> vec;
//                    for(int i = 0; i < NUM; ++i)
//                    {
//                        vec[rank(arr[result[i]])]++;
//                    }
//
//                    //TODO 有空优化下重复
//                    for(int i = 0; i < 13; ++i)
//                    {
//                        if(vec[i] + vec[15] >= 3)
//                        {
//                            for(int j = 0; j < 13; ++j)
//                            {
//                                if(i == j)
//                                    continue;
//
//                                int remainGhost = vec[15] - (3 - vec[i] <= 0 ? 0 : 3 - vec[i]);
//                                if(vec[j] + remainGhost >= 2)
//                                {
//                                    flag = true;
//                                }
//                            }
//                        }
//                    }
//                break;
//                case 4:         //同花
//                    std::vector<Card> vec;
//                    Card max = 0; 
//                    Card min = 0;
//                    for(int i = 0; i < NUM; ++i)
//                    {
//                        if(i != NUM - 1)
//                        {
//                            //花色必须一样
//                            if(!compareWithGhostSuit(arr[res[i]], arr[res[i + 1]]))
//                            {
//                                flag = false;
//                            }
//                        }
//                    }
//                break;
//                case 5:     //顺子
//                    std::vector<Card> vec;
//                    Card max = 0; 
//                    Card min = 0;
//                    for(int i = 0; i < NUM; ++i)
//                    {
//                        if(i != NUM - 1)
//                        {
//                            //牌面不能一样
//                            if(compareWithGhostFalse(arr[res[i]], arr[res[i + 1]]))
//                            {
//                                flag = false;
//                            }
//                        }
//
//                        Card val = arr[res[i]];
//
//                        if(val == 53 || val == 54)
//                            continue;
//
//                        if(rank(val) > rank(max))
//                            max = val;
//
//                        if(min == 0 || rank(val) < rank(min))
//                            min = val;
//
//                        vec.push_back(val);
//                    }
//
//                    if(rank(max) - rank(min) > 4)
//                        flag = false;
//
//                    //特殊的顺子
//                    if(!flag)
//                    {
//                        if(isSubset(smallStright, vec))
//                            flag = true;
//                    }
//                    break;
//                break;
//                case 6:         //三条
//                    flag = false;
//                    std::map<int, Card> vec;
//                    for(int i = 0; i < NUM; ++i)
//                    {
//                        vec[rank(arr[result[i]])]++;
//                    }
//
//                    for(int i = 0; i < 13; ++i)
//                    {
//                        if(vec[i] + vec[15] >= 3)
//                        {
//                            flag = true;
//                        }
//                    }
//                break;
//                case 7:         //两对
//                    flag = false;
//                    std::map<int, Card> vec;
//                    for(int i = 0; i < NUM; ++i)
//                    {
//                        vec[rank(arr[result[i]])]++;
//                    }
//
//                    for(int i = 0; i < 13; ++i)
//                    {
//                        if(vec[i] + vec[15] >= 2)
//                        {
//                            for(int j = 0; j < 13; ++j)
//                            {
//                                if(i == j)
//                                    continue;
//
//                                int remainGhost = vec[15] - (2 - vec[i] <= 0 ? 0 : 2 - vec[i]);
//                                if(vec[j] + remainGhost >= 2)
//                                {
//                                    flag = true;
//                                }
//                            }
//                        }
//                    }
//                break;
//                case 8:     //对子
//                    flag = false;
//                    std::map<int, Card> vec;
//                    for(int i = 0; i < NUM; ++i)
//                    {
//                        vec[rank(arr[result[i]])]++;
//                    }
//
//                    for(int i = 0; i < 13; ++i)
//                    {
//                        if(vec[i] + vec[15] >= 2)
//                        {
//                            flag = true;
//                        }
//                    }
//                break;
//            }
//            //end
//			if(flag)
//			{
//				std::vector<Card> v;
//				for (int j = NUM - 1; j >= 0; j--)
//					v.push_back(arr[result[j]]);
//				ret.push_back(v);
//			}
//		}
//	}
//}
//
////鬼牌可以等于任意牌, 下面2个函数在不同情况下使用
//bool Deck::compareWithGhostTrue(int lt, int rt)
//{
//    if(lt == 53 || lt == 54 || rt == 53 || rt == 54)
//    {
//        return true;
//    }
//
//    return rank(lt) == rank(rt);
//}
//
//bool Deck::compareWithGhostFalse(int lt, int rt)
//{
//    if(lt == 53 || lt == 54 || rt == 53 || rt == 54)
//    {
//        return false;
//    }
//
//    return rank(lt) == rank(rt);
//}
//
//bool Deck::compareWithGhostSuit(int lt, int rt)
//{
//    if(lt == 53 || lt == 54 || rt == 53 || rt == 54)
//        return true;
//
//    return suit(lt) == suit(rt);
//}
//
//void Deck::getRecommendPokerSet2(std::vector<Card> cards)
//{
//    for(int32_t i = 0; i < 9; ++i)
//    {
//        CARD_NUM_LIST cards_1;
//        int num = 5;
//        int result[num];
//        combine_decrease(cards, cards.size(), result, num, num, cards_1, i);
//
//        for(uint32_t l = 0; l < cards_1.size(); ++l)
//        {
//            std::vector<int32_t> cards_inner(cards);
//            std::vector<int32_t> tmpCards_1(cards_1[l]);
//            _delCards(cards_inner, tmpCards_1);
//
//            bool breakfor = false;
//            for(int32_t n = 0; n < 10; ++n)
//            {
//                CARD_NUM_LIST cards_2;
//                combine_decrease(cards_inner, cards_inner.size(), result, num, num, cards_2, i);
//
//                if(cards_2.size() > 0)
//                {
//                    if(n < i)
//                    {
//                        //牌型中墩大于尾墩了
//                        breakfor = true;
//                        break;
//                    }
//                    for(uint32_t j = 0; j < cards_2.size(); ++j)
//                    {
//                        bool cflag = false;
//                        if(i == n)          //中墩尾墩牌型一样
//                        {
//                            cflag = isMessireFive(i, cards_1[l], cards_2[j]);
//                        }
//                        if(!cflag)
//                        {
//                            std::vector<int32_t> cards_in(cards);
//                            std::vector<int32_t> tmpCards_2;
//                            cardsEncode(cards_2[j], tmpCards_2);
//                            _delCards(cards_in, tmpCards_2);
//
//                            wlArr.clear();
//                            dzArr.clear();
//                            stArr.clear();
//                            tzArr.clear();
//                            wtArr.clear();
//                            szArr.clear();
//                            decodeArr.clear();
//                            getGroupCards(cards_in, wlArr, dzArr, stArr, tzArr, wtArr, szArr, decodeArr);
//
//                            for(int k = 0; k < 3; ++k)
//                            {
//                                BRAND_LIST cards_3; 
//                                switch(k)
//                                {
//                                    case 0:
//                                        santiaoThree(stArr, cards_3);
//                                        break;
//                                    case 1:
//                                        yiduiThree(wlArr, dzArr, cards_3);
//                                        break;
//                                    case 2:
//                                        wulongThree(wlArr, cards_3);
//                                        break;
//                                    default:
//                                        break;
//                                }
//
//                                bool cflag2 = false;
//                                if(cards_3.size() > 0)
//                                {
//                                    if((k == 0 && n < 6) || (k == 1 && n < 8) || (k == 2 && n < 9))
//                                    {
//                                        cflag2 = true;
//                                    }
//                                    if((k == 0 && n == 6) || (k == 1 && n == 8) || (k == 2 && n == 9))
//                                    {
//                                        bool fl = isMessireThree(n, cards_2[j], cards_3[0], 0);
//                                        cflag2 = (fl == false);
//                                    }
//                                }
//
//                                if(cflag2)
//                                {
//                                    //同一类型只取一个
//                                    if(existArr[i * 100 + n * 10 + k] == false)
//                                    {
//                                        existArr[i * 100 + n * 10 + k] = true;
//                                        //存下来
//                                    }
//                                }
//                            }
//                        }
//                    }
//                }
//            }
//            if(breakfor)
//            {
//                break;
//            }
//        }
//    }
//}
//
//bool Deck::isMessireFive(int i, CARD_NUM_LIST& a, CARD_NUM_LIST& b)
//{
//    std::vector<int32_t> v;
//    std::vector<int32_t> s;
//    v.reserve(5);
//    s.reserve(5);
//
//    int lgc = 0;        //鬼牌数量
//    int rgc = 0;
//
//    for(int32_t q = 0; q < 5; ++q)
//    {
//        v[q] = rank(a[q]);
//        if(v[q] == 15)
//            lgc++;
//        s[q] = rank(b[q]);
//        if(s[q] == 15)
//            rgc++;
//    }
//
//    sort(v.begin(), v.end());
//    sort(s.begin(), s.end());
//
//    if(i == 0)
//    {
//        if(v[0] == s[0])
//        {
//            if(lgc > 0 && rgc == 0)
//                return false;
//            else if(rgc > 0 && lgc == 0)
//                return true;
//            else
//                return true;        //都有鬼,或都没鬼,相等不算相公
//        }
//        else
//        {
//            return v[0] < s[0];
//        }
//            
//    }
//
//    if(i == 2)
//    {
//        //铁支的带鬼牌比对
//        std::map<int, int> lm;
//        std::map<int, int> rm;
//        int lbigv = -1;
//        int lmaxv = -1;
//        int lminv = -1;
//        int rbigv = -1;
//        int rmaxv = -1;
//        int rminv = -1;
//        for(int i = 0; i < 5; ++i)
//        {
//            if(v[i] != 15)
//            {
//                lm[v[i]]++;
//                if(lm[v[i]] > 1)
//                    lbigv = v[i];
//
//                if(v[i] > lmaxv)
//                    lmaxv = v[i];
//
//                if(lminv == - 1 || v[i] < lminv)
//                    lminv = v[i];
//            }
//
//            if(s[i] != 15)
//            {
//                rm[s[i]]++;
//                if(rm[s[i]] > 1)
//                    rbigv = s[i];
//
//                if(s[i] > rmaxv)
//                    rmaxv = s[i];
//
//                if(rminv == - 1 || s[i] < rminv)
//                    rminv = s[i];
//            }
//        }
//        
//        if(lbigv == -1)
//            lbigv = lmaxv;      //如果没有数量超过1的,比最大的牌
//
//        if(rbigv == -1)
//            rbigv = rmaxv;
//
//        if(lbigv == rbigv)      //点数相同,比另一张牌
//        {
//            int lothv = (lbigv == lmaxv ? lminv : lmaxv);
//            int rothv = (rbigv == rmaxv ? rminv : rmaxv);
//            if(lothv == rothv)
//            {
//                //牌型相等情况下的鬼牌处理
//                if(lgc > 0 && rgc == 0)
//                    return false;
//                else if(rgc > 0 && lgc == 0)
//                    return true;
//                else
//                    return true;
//            }
//            else
//            {
//                return lothv < rothv;
//            }
//        }
//        else
//        {
//            return lbigv < rbigv;
//        }
//    }
//
//    //同花顺 顺子
//    if(i == 1 || i == 5)
//    {
//        //同花与顺子, 最小牌加4(不超过A),就是最大牌型,所以直接比最小牌就ok
//        //这里没有考虑特殊牌型 2 3 4 5 A, TODO
//        if(v[0] == s[0])
//        {
//            if(lgc > 0 && rgc == 0)
//                return false;
//            else if(rgc > 0 && lgc == 0)
//                return true;
//            else
//                return true;
//        }
//        else
//            return v[0] < s[0];
//    }
//
//    //葫芦
//    if(i == 3)
//    {
//        //按特定牌型排序, 对子在前,三条在后
//        setGhostToBrand(v, lgc, i);
//        setGhostToBrand(s, rgc, i);
//        if(v[4] == s[4])
//        {
//            if(v[0] == s[0])
//            {
//                if(lgc > 0 && rgc == 0)
//                    return false;
//                else if(rgc > 0 && lgc == 0)
//                    return true;
//                else
//                    return true;
//            }
//            else
//                return v[0] < s[0];
//        }
//        else
//            return v[4] < s[4];
//    }
//
//    if(i == 4)
//    {
//        //TODO 这里的同花按照对子数量比牌
//        setGhostToBrand(v, lgc, i);
//        setGhostToBrand(s, rgc, i);
//        int lc = 0;
//        int rc = 0;
//        if(v[3] == v[4])
//            lc++;
//
//        if(v[1] == v[2])
//            lc++;
//
//        if(s[3] == s[4])
//            rc++;
//
//        if(s[1] == s[2])
//            rc++;
//
//        if(lc == 0 && rc == 0)
//        {
//            return v[0] < s[0];
//        }
//
//        if(lc == rc)
//        {
//            if(lgc > 0 && rgc == 0)
//                return false;
//            else if(rgc > 0 && lgc == 0)
//                return true;
//            else
//                return true;
//        }
//        else
//            return lc < rc;
//    }
//
//    if(i == 6)
//    {
//        setGhostToBrand(v, lgc, i);
//        setGhostToBrand(s, rgc, i);
//        if(v[4] == s[4])
//        {
//            if(v[1] == s[1])
//            {
//                if(v[0] == s[1])
//                {
//                    if(lgc > 0 && rgc == 0)
//                        return false;
//                    else if(rgc > 0 && lgc == 0)
//                        return true;
//                    else
//                        return true;
//                }
//                else
//                    return v[0] < s[0];
//            }
//            else
//                return v[1] < s[1];
//        }
//        else
//        {
//            return v[4] < s[4];
//        }
//        
//    }
//    if(i == 7)
//    {
//        setGhostToBrand(v, lgc, i);
//        setGhostToBrand(s, rgc, i);
//        if(v[4] == s[4])
//        {
//            if(v[2] == s[2])
//            {
//                if(v[0] == s[0])
//                {
//                    if(lgc > 0 && rgc == 0)
//                        return false;
//                    else if(rgc > 0 && lgc == 0)
//                        return true;
//                    else
//                        return true;
//                }
//                else
//                    return v[0] < s[0];
//            }
//            else
//                return v[2] < s[2];
//        }
//        else
//            return v[4] < s[4];
//    }
//    if(i == 8)
//    {
//        setGhostToBrand(v, lgc, i);
//        setGhostToBrand(s, rgc, i);
//        if(v[4] == s[4])
//        {
//            if(v[2] == s[2])
//            {
//                if(v[1] == s[1])
//                {
//                    if(v[0] == s[0])
//                    {
//                        if(lgc > 0 && rgc == 0)
//                            return false;
//                        else if(rgc > 0 && lgc == 0)
//                            return true;
//                        else
//                            return true;
//                    }
//                    else
//                        return v[0] < s[0];
//                }
//                else
//                    return v[1] < s[1];
//            }
//            else
//                return v[2] < s[2];
//        }
//        else
//            return v[4] < s[4];
//    }
//    if(i == 9)
//    {
//        setGhostToBrand(v, lgc, i);
//        setGhostToBrand(s, rgc, i);
//        for(int j = 4; j >= 0; j--)
//        {
//            if(v[j] == s[j])
//            {
//                continue;
//            }
//            return v [j] < s[j];
//        }
//
//        if(lgc > 0 && rgc == 0)
//            return false;
//        else if(rgc > 0 && lgc == 0)
//            return true;
//        else
//            return true;
//    }
//
//    return false;
//}
//
//void Deck::setGhostToBrand(std::vector<int>& v, int gc, int type)
//{
//    if(type == 3)       //葫芦          对子在前
//    {
//        if(gc == 4)
//        {
//            v[1] = v[0];
//            v[2] = v[3] = v[4] = 12;        //A
//        }
//
//        if(gc == 3)
//        {
//            if(v[0] == v[1])
//            {
//                v[2] = v[3] = v[4] = 12;        //A
//            }
//            else
//            {
//                if(v[0] < v[1])
//                {
//                    v[2] = v[3] = v[4] = v[1];
//                    v[1] = v[0];
//                }
//                else
//                {
//                    v[2] = v[3] = v[4] = v[0];
//                    v[0] = v[1];
//                }
//            }
//        }
//
//        if(gc == 2)
//        {
//            //3条
//            if(v[0] == v[2])            //3条 + 2A          实际可以组5同, 但按都是葫芦的比法,就小了
//            {
//                v[0] = v[1] = 12;
//                v[4] = v[3] = v[2];
//            }
//            else
//            {
//                //一定有对子
//                int dz = (v[0] == v[1] ? v[0] : v[1]);
//                int one = (v[0] == v[1] ? v[2] : v[0]);
//                if(one > dz)
//                {
//                    v[0] = v[1] = dz;
//                    v[2] = v[3] = v[4] = one;
//                }
//                else
//                {
//                    v[0] = v[1] = one;
//                    v[2] = v[3] = v[4] = dz;
//                }
//            }
//        }
//
//        if(gc == 1)
//        {
//            v[4] = v[3];
//        }
//
//        if(gc == 0)
//        {
//            if(v[0] == v[2])
//            {
//                int val = v[4];
//                v[2] = v[3] = v[4] = v[0];
//                v[0] = v[1] = val;
//            }
//        }
//    }
//
//    if(type == 4)       //同花, 单张 小对 大对
//    {
//        if(gc == 4)
//        {
//            v[1] = v[2] = v[3] = v[4] = 12;
//        }
//
//        if(gc == 3)
//        {
//            v[3] = v[4] = 12;
//            v[2] = v[1];
//
//            if(v[0] == v[1])
//            {
//                v[0] = 12;
//            }
//        }
//
//        if(gc == 2)
//        {
//            if(v[0] == v[1] || v[1] == v[2])
//            {
//                //有对子
//                v[3] = v[4] = 12;
//                if(v[0] == v[1])
//                {
//                    int min = v[2];
//                    v[2] = v[0];
//                    v[0] = min;
//                }
//            }
//            else
//            {
//                v[3] = v[4] = v[2];
//                v[2] = v[1];
//            }
//        }
//
//        if(gc == 1)
//        {
//            //TODO 同花不会有3条出现,鬼牌也不能组3条,出现了调整穷举算法
//            if(v[0] == v[1] || v[1] == v[2] || v[2] == v[3])
//            {
//                v[4] = v[3];
//
//                if(v[0] == v[1])
//                {
//                    int min = v[2];
//                    v[2] = v[0];
//                    v[0] = min;
//                }
//                else if(v[2] == v[3])
//                {
//                    v[2] = v[1];
//                }
//            }
//            else
//            {
//                v[4] = v[3];
//            }
//        }
//
//        if(gc == 0)
//        {
//            if(v[0] == v[1])
//            {
//                int o4 = v[4];
//                int o3 = v[3];
//                v[4] = v[3] = v[0];
//                v[0] = v[2];
//                v[1] = o3;
//                v[2] = o4;
//            }
//
//            if(v[1] == v[2])
//            {
//                int o3 = v[3];
//                v[2] = v[4];
//                v[3] = v[4] = v[1];
//                v[1] = o3;
//            }
//
//            if(v[2] == v[3])
//            {
//                int o4 = v[4];
//                v[4] = v[3];
//                v[2] = o4;
//            }
//        }
//    }
//
//    if(type == 6)       //三条, 单张 单张 三条
//    {
//        if(gc == 4)
//        {
//            v[1] = v[2] = v[3] = v[4] = 12;
//        }
//
//        if(gc == 3)
//        {
//            v[2] = v[3] = v[4] = 12;
//        }
//
//        if(gc == 2)
//        {
//            v[3] = v[4] = v[2];
//        }
//
//        if(gc == 1)
//        {
//            if(v[0] == v[1])
//            {
//                v[0] = v[2];
//                int o3 = v[3];
//                v[4] = v[3] = v[2] = v[1];
//                v[1] = o3;
//            }
//
//            if(v[1] == v[2])
//            {
//                int o3 = v[3];
//                v[4] = v[3] = v[1];
//                v[1] = o3;
//            }
//
//            if(v[2] == v[3])
//            {
//                v[4] = v[3];
//            }
//        }
//
//        if(gc == 0)
//        {
//            if(v[0] == v[1] && v[1] == v[2])
//            {
//                v[0] = v[3];
//                v[1] = v[4];
//                v[4] = v[3] = v[2];
//            }
//            if(v[1] == v[2] && v[2] == v[3])
//            {
//                int o4 = v[4];
//                v[4] = v[3];
//                v[3] = v[2];
//                v[2] = v[1];
//                v[1] = o4;
//            }
//        }
//    }
//
//    if(type == 7)       //两对 单张 小对子 大对子
//    {
//        if(gc == 4)
//        {
//            v[1] = v[2] = v[3] = v[4] = 12;
//        }
//
//        if(gc == 3)
//        {
//            if(v[0] == v[1])
//            {
//                v[4] = v[3] = 12;
//                v[2] = v[1];
//                v[0] = 12;
//            }
//        }
//
//        if(gc == 2)
//        {
//            if(v[0] == v[1] || v[1] == v[2])
//            {
//                v[4] = v[3] = 12;
//                if(v[0] == v[1])
//                {
//                    int o2 = v[2];
//                    v[2] = v[0];
//                    v[0] = o2;
//                }
//            }
//            else
//            {
//                v[4] = v[3] = v[2];
//                v[2] = v[1];
//            }
//        }
//
//        if(gc == 1)
//        {
//            if(v[0] == v[1])
//            {
//                v[4] = v[3];
//                int o2 = v[2];
//                v[2] = v[0];
//                v[0] = o2;
//            }
//
//            if(v[1] == v[2])
//            {
//                v[4] = v[3];
//            }
//
//            if(v[2] == v[3])
//            {
//                v[4] = v[3];
//                v[2] = v[1];
//            }
//        }
//
//        if(gc == 0)
//        {
//            if(v[0] == v[1] && v[2] == v[3])
//            {
//                v[0] = v[4];
//                v[4] = v[3];
//                v[2] = v[1];
//            }
//        }
//    }
//
//    if(type == 8)       //一对 单牌 单牌 单牌 一对
//    {
//        if(gc == 4)
//        {
//            v[1] = v[2] = v[3] = v[4] = 12;
//        }
//
//        if(gc == 3)
//        {
//            v[4] = v[3] = v[2] = 12;
//        }
//
//        if(gc == 2)
//        {
//            v[4] = v[3] = 12;
//        }
//
//        if(gc == 1)
//        {
//            v[4] = v[3];
//        }
//
//        if(gc == 0)
//        {
//            if(v[0] == v[1])
//            {
//                v[0] = v[2];
//                int o4 = v[4];
//                int o3 = v[3];
//                v[4] = v[3] = v[1];
//                v[1] = o3;
//                v[2] = o4;
//            }
//
//            if(v[1] == v[2])
//            {
//                v[1] = v[3];
//                int o4 = v[4];
//                v[4] = v[3] = v[2];
//                v[2] = o4;
//                
//            }
//
//            if(v[2] == v[3])
//            {
//                v[2] = v[4];
//                v[4] = v[3];
//            }
//        }
//    }
//
//    
//    if(type == 9)       //
//    {
//        if(gc == 4)
//        {
//            v[1] = v[2] = v[3] = v[4] = 12;
//        }
//
//        if(gc == 3)
//        {
//            v[4] = v[3] = v[2] = 12;
//        }
//
//        if(gc == 2)
//        {
//            v[4] = v[3] = 12;
//        }
//
//        if(gc == 1)
//        {
//            v[4] = 12;
//        }
//    }
//}

//获取5张牌里的对子,三条与四同
//void Deck::getAllSame(const std::vector<int32_t>& cards,std::vector<int>& notsame, std::vector<int>& dz, int& st, int& tz, int& wt)
//{
//    std::vector<int> same;
//    for(int32_t i = num - 1; i >= 0; )
//    {
//        auto last = cards[i];
//
//        --i;
//        bool find = false;
//        while(i >= 0 && last == cards[i])
//        {
//            find = true;
//            same.push_back(last);
//            last = cards[i];
//            --i;
//        }
//
//        if(find)
//        {
//            same.push_back(last);
//
//            if(same.size() > 1)
//            {
//                if(same.size() == 2)
//                {
//                    dz.push_back(same[0]);
//                }
//
//                if(same.size() == 3)
//                {
//                    st = same[0];
//                }
//
//                if(same.size() == 4)
//                {
//                    tz = same[0];
//                }
//                if(same.size() == 5)
//                {
//                    wt = same[0];
//                }
//            }
//
//            same.clear();
//        }
//        notsame.push_back(last);
//    }
//}


}



/****************** 单元测试, 代码风格就不讲究了 ******************/
#ifdef DECK_UNIT_TEST

#include "deck.h"
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>

namespace deck_unit_test {
using namespace std;
using namespace placeholders;

using namespace lobby;


/*
void table()
{
    std::string suitsName[] = {"    ", "方块", "梅花",  "红桃", "黑桃"};
    std::string ranksName[] = {" ", "2", "3", "4", "5", "6", "7", "8", "9", "X", "J", "Q", "K", "A"};
    for (int s = 0; s < 5; ++s)
    {
        cout << suitsName[s] << ": ";
        if (s != 0)
        {
            for(int r = 1; r <= 13; ++r)
            {
                cout <<  setw(8) << ((s - 1) * 13 + r) << "   ";
            }
        }
        else
        {
            for(int r = 1; r <= 13; ++r)
            {
                cout <<  setw(8) <<  ranksName[r] << "   ";
            }
        }
        cout << endl;
    }
}
*/
/*
 *         :        2          3          4          5          6          7          8          9          X          J          Q          K          A   
 *
 *     方块:        1          2          3          4          5          6          7          8          9         10         11         12         13   
 *
 *     梅花:       14         15         16         17         18         19         20         21         22         23         24         25         26   
 *
 *     红桃:       27         28         29         30         31         32         33         34         35         36         37         38         39   
 *
 *     黑桃:       40         41         42         43         44         45         46         47         48         49         50         51         52 
 */

std::string suitsName[] = {"♦", "♣", "♥", "♠"};
std::string ranksName[] = {"2", "3", "4", "5", "6", "7", "8", "9", "X", "J", "Q", "K", "A"};


using Card = Deck::Card;
using H = std::vector<Card>;

string cardName(Card c)
{
    return suitsName[(c - 1)/13] + ranksName[(c - 1) % 13];
}

string readAbleH(const H& h)
{
    string ret;
    for(Card c : h)
        ret += (cardName(c) + " ");
    return ret;
}

string strH(const H& h)
{
    stringstream ss;
    for(Card c : h)
        ss << setw(2) << ((int)c) << " ";
    return ss.str();
}

string strBrandInfo(const Deck::BrandInfo& bi)
{
    stringstream ss;
    ss << (int)(bi.b) <<  ", " << bi.point;
    return ss.str();
}

string detailH(const H& h)
{
    string ret = strH(h) + "   " + readAbleH(h);
    return ret;
}



void showDeck()
{
    H all(52);
    for (int i = 0; i < 52; ++i)
        all[i] = i + 1;

    cout << readAbleH(all) << endl;
}


H dunArr[] = 
{
//    {6, 19, 32, 45, 45},  //9
//    {34, 35, 36 ,37, 38}, //8
//    {22, 23, 24, 25, 26}, //8
//    {27, 28 ,29, 30, 39}, //8
//    {3, 16, 29, 42, 1},   //7
//    {3, 16, 29, 42, 48},   //7
//    {1, 40, 11, 24, 50},  //6
//    {11, 24, 50, 52, 52},  //6
//    {29, 31, 35, 38, 33}, //5
//    {40, 15, 16, 30, 13}, //4
//    {35, 36 ,37, 25 ,39}, //4
//    {12, 11, 24, 50, 13},  //3
//    {1, 11, 24, 50, 13},  //3
//    {1, 45, 11, 24, 50},  //3
//    {14, 4, 30, 36, 49}, //2
//    {4, 30, 7, 36, 49}, //2
//    {4, 30, 36, 49, 12}, //2
//    {20, 33, 21, 35, 36}, //1
//    {40, 20, 33, 21, 35}, //1
//    {2, 42, 20, 33, 21}, //1
//    {1 , 41, 42, 20, 33}, //1
//    {1, 2, 3, 4, 37}, //0
//    {13,13,26}, //3
//    {1, 15, 27},          //1
//    {10, 37, 52}, //0
    {11, 24, 13}, //1
    {50, 37, 20}, //1
};

H allArr[] =
{
//    {27,28,29,30,31,32,33,34,35,36,37,38,39}, //青龙
    { 1,14, 2,15, 3,16, 4,43, 5,44,39,52,50}, //6对 + 1
    { 1,14, 2,15, 3,16, 4,43, 5,44,50,11,52}, //6对 + 1
    { 1,14,27,40, 3,16, 4,43, 5,13,26,39,52}, //6对 + 1
    { 1,14, 2,15, 3,16, 4,43, 5,44,39,52,13}, //5对 + 3条
    {14, 2,15, 3,16,17, 4,43, 5,40,44,39,52}, //5对 + 3条
    {14, 2,15,41,28,17, 4,43, 5,40,44,39,52}, //5对 + 3条
    { 1, 2,15, 3,16,17,30, 4,43, 5,44,39,52}, //6对半,  (4条 + 3对 + 1)
    { 1, 2 ,4,15,17,18,21,23,44,50,51,52,43}, //3同花
    { 1, 2 ,3,16,17,18,19,20,47,48,49,50,51}, //3同花顺
    { 1, 2 ,16,16,17,18,19,20,47,48,49,50,51}, //3顺子
};


void testBrand()
{
    for (auto& h : dunArr)
    {
        cout << "--------------------------" << endl;
        cout << "收到: " << detailH(h) << endl;
        std::shuffle(h.begin(), h.end(), std::default_random_engine(::time(0)));
        cout << "洗牌: " << detailH(h) << endl;
        auto info = Deck::brandInfo(h.data(), h.size());
        cout << "整理: " << detailH(h) << endl;
        cout << "牌型:  " << (int)(info.b) <<  ", " << info.point << endl;
        Deck::cmpBrandInfo(info, info);
    }
}

void testAll()
{
    for(auto& h : allArr)
    {
        cout << "--------------------------" << endl;
        cout << "收到: " << detailH(h) << endl;
        auto d1 = Deck::brandInfo(h.data(), 3);
        auto d2 = Deck::brandInfo(h.data() + 3, 5);
        auto d3 = Deck::brandInfo(h.data() + 8, 5);
        auto s = Deck::g13SpecialBrand(h.data(), d2.b, d3.b);
        cout << "头墩: " << strBrandInfo(d1) << endl;
        cout << "中墩: " << strBrandInfo(d2) << endl;
        cout << "尾墩: " << strBrandInfo(d3) << endl;
        cout << "全序: " << detailH(h) << endl;
        cout << "特殊: " << (int)s << endl;
    }
}


}

int main()
{
    using namespace deck_unit_test;
    testAll();
    //testBrand();
    return 0;

}
#endif //#ifdef DECK_UNIT_TEST

