#ifndef ROBOT_DECK_HPP
#define ROBOT_DECK_HPP


#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <math.h>

#include <assert.h>

class Deck
{
public:

    using Card = uint16_t;

    enum class Rank : uint8_t
    {
        r2 = 0,
        r3 = 1,
        r4 = 2,
        r5 = 3,
        r6 = 4,
        r7 = 5,
        r8 = 6,
        r9 = 7,
        rX = 8,
        rJ = 9,
        rQ = 10,
        rK = 11,
        rA = 12,
    };

    struct CardInfo
    {
        int32_t type;
        int32_t value;

        bool operator==(const CardInfo& rhs)        //去重使用,只考虑value
        {
            return value == rhs.value;
        }
    };
    typedef std::vector<CardInfo> CARD_INFO_LIST;
    typedef std::vector<CARD_INFO_LIST> BRAND_LIST;
    typedef std::vector<std::vector<Card>> CARD_NUM_LIST;

    struct PairMix
    {
        int32_t dznum = 0;      //对子个数
        BRAND_LIST dz;          //对子列表
        CARD_INFO_LIST ndz;     //没有组成对子的牌 
    };

    struct ResultSet
    {
        std::vector<Card> wl;           //乌龙
        CARD_NUM_LIST dz;               //对子
        CARD_NUM_LIST st;               //三条
        CARD_NUM_LIST tz;               //铁支
        CARD_NUM_LIST wt;               //五同
        CARD_NUM_LIST sz;               //顺子
    };
    //end
    static uint16_t suit(Card card);
    static uint16_t rank(Card card);

    static bool lessSuit(Card c1, Card c2);
    static bool lessRank(Card c1, Card c2);
    static bool lessCard(Card c1, Card c2);

    static bool isSubset(std::vector<Card> v1, std::vector<Card> v2);
    
    //带鬼牌的获取牌型
    void _delCards(std::vector<Card>& src, std::vector<Card>& del);
    void combine_decrease(const std::vector<Card>& arr, int start, int* res, int count, const int NUM, CARD_NUM_LIST& ret, int16_t type);
    bool compareWithGhostTrue(int lt, int rt);
    //bool compareWithGhostFalse(int lt, int rt);
    bool compareWithGhostSuit(int lt, int rt);
    bool compareSuit(int lt, int rt);
    bool compareRank(int lt, int rt);
    void getRecommendPokerSet(const std::vector<Card>& cards, std::vector<Card>& ret);
    bool isMessireFive(int i, const std::vector<Card>& a, const std::vector<Card>& b);
    bool isMessireThree(int i, const std::vector<Card>& a, const std::vector<Card>& b);
    void setGhostToBrand(int32_t* v, int gc, int type);
    void setGhostToThreeBrand(int32_t* v, int gc, int type);

    std::vector<Card> cards;
};

inline uint16_t Deck::suit(Card card)
{
    return (card - 1) / 13;
}

inline uint16_t Deck::rank(Card card)
{
    if(card == 53 || card == 54)
    {
        return 15;
    }
    return (card - 1) % 13;
}

inline bool Deck::lessSuit(Card c1, Card c2)
{
    return suit(c1) < suit(c2);
}

inline bool Deck::lessRank(Card c1, Card c2)
{
    return rank(c1) < rank(c2);
}

inline bool Deck::lessCard(Card c1, Card c2)
{
    return (rank(c1) != rank(c2)) ? lessRank(c1, c2) : lessSuit(c1, c2);
}

#endif
