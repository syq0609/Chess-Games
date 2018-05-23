#ifndef LOBBY_DECK_HPP
#define LOBBY_DECK_HPP


#include <vector>
#include <string>
#include <algorithm>
#include <map>
#include <math.h>

#include <assert.h>

namespace lobby{

class Deck
{
public:
    friend class Water13;

    using Card = uint16_t;

    enum class Suit : uint8_t
    {
        spade   = 3,
        heart   = 2,
        club    = 1,
        diamond = 0
    };

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

    enum class Brand : int16_t //枚举还是取德州的叫法了, 规则是一样的
    {
        fiveOfKind      = 9,  //五同(类似4条, 5张同点数的)
        straightFlush   = 8,  //同花顺
        fourOfKind      = 7,  //四条, 铁支
        fullHouse       = 6,  //葫芦, 满堂红, 三带二
        flush           = 5,  //同花
        straight        = 4,  //顺子
        threeOfKind     = 3,  //三条, 三一一
        twoPairs        = 2,  //双对, 两对
        onePair         = 1,  //单对, 对子, 一对
        hightCard       = 0,  //乌龙, 高牌, 散牌
    };

    enum class G13SpecialBrand : int16_t //这些枚举名字就自己随意定啦, 牌型大小依据枚举值的个位数来比较
    {
        flushStriaght            = 135, //青龙,          13张同花顺
        straight                 = 124, //一条龙         13张顺子
        royal                    = 113, //12皇族         13张最小是J(属于JQKA)
        tripleStraightFlush      = 103, //三个同花顺     3墩都是同花顺
        tripleBombs              =  93, //三个炸弹       13张含3个四条
        allBig                   =  82, //全大           13张中最小的是8
        allLittle                =  72, //全小           13张中最大的是8
        redOrBlack               =  62, //全红或全黑     13张全红或全黑
        quradThreeOfKind         =  52, //四个三条       13张含4个三条
        pentaPairsAndThreeOfKind =  42, //五对 + 三条    13张含1个三条+5个对子
        sixPairs                 =  32, //六对           13张含6个对子
        tripleStraight           =  22, //三顺子         3墩都是顺子
        tripleFlush              =  12, //三同花         3墩都是同花
        none                     =   0,

        
    };

    struct BrandInfo
    {
        Brand b;
        int32_t point = 0;
        bool isGhost = false;
    };

    //从客户端移植过来的理牌算法(大部分变量名保留客户端名称,方便对比)
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

    //0 相等, 1 bi1较大, 2 bi2较大
    static int32_t cmpBrandInfo(const BrandInfo& bi1, const BrandInfo& bi2);


    static Suit suit(Card card);
    static Rank rank(Card card);

    static bool lessSuit(Card c1, Card c2);
    static bool lessRank(Card c1, Card c2);
    static bool lessCard(Card c1, Card c2);

    static bool hasGhost(Card* c, uint32_t size);
    static Brand brand(Card* c, uint32_t size);
    static Brand brand3(Card* c);
    static Brand brand5(Card* c);
    static BrandInfo brandInfo(Card* c, uint32_t size);
    static BrandInfo brandInfo3(Card* c);
    static BrandInfo brandInfo5(Card* c);

    static Brand ghost3(Card* c, uint32_t size);
    static Brand ghost5(Card* c, uint32_t size);
    static BrandInfo ghostInfo3(Card* c, uint32_t size);
    static BrandInfo ghostInfo5(Card* c, uint32_t size);

    static bool isSubset(std::vector<Rank> v1, Card* card, int32_t size);
    //static bool isSubset(std::vector<Rank> v1, std::vector<Rank> v2);

    static G13SpecialBrand g13SpecialBrand(Card* c, Brand b2, Brand b3);
    static G13SpecialBrand g13SpecialBrandByDun(Card* c, Brand b2, Brand b3);
    static G13SpecialBrand g13SpecialBrandByAll(Card* c, G13SpecialBrand dun);

    //理牌算法相关
    void getGroupCards(std::vector<int32_t>& cards, CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& stArr, BRAND_LIST& tzArr, BRAND_LIST& wtArr, BRAND_LIST& szArr, CARD_INFO_LIST& decodeArr);
    void sortPoker(int32_t sortType, std::vector<int32_t>& cardList);
    Deck::CardInfo singleCardsDecode(int32_t c);
    int32_t singleCardsEncode(CardInfo& info);
    void cardsDecode(std::vector<int32_t>& cArr, CARD_INFO_LIST& arr);
    void cardsEncode(CARD_INFO_LIST& arr, std::vector<int32_t>& cArr);
    std::map<int32_t, Deck::CARD_INFO_LIST> getSuit(CARD_INFO_LIST& cards);
    void _delCards(std::vector<int32_t>& src, std::vector<int32_t>& del);
    void delSamePoint(Deck::CARD_INFO_LIST& cArr);
    void wutong(BRAND_LIST& wtArr, BRAND_LIST& cardsArr);
    void tiezhi(CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& stArr, BRAND_LIST& tzArr, BRAND_LIST& cardsArr);
    void tonghuashun(CARD_INFO_LIST& decodeArr, BRAND_LIST& cardsArr);
    void tonghua(CARD_INFO_LIST& decodeArr, BRAND_LIST& cardsArr);
    void shunzi(CARD_INFO_LIST& decodeArr, BRAND_LIST& cardsArr);
    void santiaoFive(CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& stArr, BRAND_LIST& cardsArr);
    void liangduiFive(CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& cardsArr);
    void yiduiFive(CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& cardsArr);
    void wulongFive(CARD_INFO_LIST& wlArr, BRAND_LIST& cardsArr);
    void hulu(BRAND_LIST& dzArr, BRAND_LIST& stArr, BRAND_LIST& cardsArr);
    void santiaoThree(BRAND_LIST& stArr, BRAND_LIST& cardsArr);
    void yiduiThree(CARD_INFO_LIST& wlArr, BRAND_LIST& dzArr, BRAND_LIST& cardsArr);
    void wulongThree(CARD_INFO_LIST& wlArr, BRAND_LIST& cardsArr);
    void _dz(CARD_INFO_LIST& arr, PairMix& pm);
    void _getDuizi(std::vector<int32_t>& arr, std::vector<std::vector<int32_t>>& dz);
    bool isMessireFive(int i, CARD_INFO_LIST& a, CARD_INFO_LIST& b);
    bool isMessireThree(int32_t i, CARD_INFO_LIST& a, CARD_INFO_LIST& b, int32_t j);
    void getRecommendPokerSet(std::vector<int32_t> cards_ori);
    //end 理牌算法
    
    //带鬼牌的获取牌型
    //void addGhost(std::vector<int32_t>& list, int32_t small, int32_t big);
    //void filterGhost(const std::vector<uint16_t>& cards, std::vector<uint16_t>& ret, int32_t& small, int32_t& big);
    //void getFiveOfKind(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret);
    //void getPair(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret);
    //void getThreeBrand(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret);
    //void getFourBrand(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret);
    //void combine_decrease(std::vector<int32_t>& arr, int start, int* result, int count, const int NUM, const std::vector<Rank>& smallStright, std::vector<std::vector<int32_t>>& ret, bool checkSuit, bool checkRank);
    ////void getTwoSuit(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret);
    //void getBaseBrandType(std::vector<uint16_t>& cards, ResultSet& rs);
    //void getStraightFlush(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret);
    //void getFourOfKind(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret);
    //void getFullHouse(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret);
    //void getFlush(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret);
    //void getStraight(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret);
    //void getThreeOfKind(const std::vector<int32_t>& cards, std::vector<std::vector<int32_t>>& ret);

    //void getRecommendPokerSet2(std::vector<uint16_t> cards);
    //void combine_decrease(const std::vector<Card>& arr, int start, int* res, int count, const int NUM, CARD_NUM_LIST& ret, int16_t type);
    //bool compareWithGhostTrue(int lt, int rt);
    //bool compareWithGhostFalse(int lt, int rt);
    //bool compareWithGhostSuit(int lt, int rt);
    //void getRecommendPokerSet2(std::vector<Card> cards);
    //bool isMessireFive(int i, CARD_NUM_LIST& a, CARD_NUM_LIST& b);
    //void setGhostToBrand(std::vector<int>& v, int gc, int type);

    std::vector<Card> cards;
};

inline Deck::Suit Deck::suit(Card card)
{
    //定义牌组数值时, 端要求[1, 52], 不是从0开始, 所以数值算法要 (c-1) 然后再整除或取余 ╮(╯▽╰)╭
    assert(card >= 1 && card <= 52);
    return static_cast<Suit>((card - 1) / 13);
}

inline Deck::Rank Deck::rank(Card card)
{
    assert(card >= 1 && card <= 52);
    return static_cast<Rank>((card - 1) % 13);
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

inline int32_t Deck::cmpBrandInfo(const BrandInfo& bi1, const BrandInfo& bi2)
{

    if (bi1.b != bi2.b)
        return bi1.b > bi2.b ? 1 : 2;

    if( bi1.point != bi2.point )
       return bi1.point > bi2.point ? 1 : 2;

    return bi1.isGhost ? (bi2.isGhost ? 0 : 2) : (bi2.isGhost ? 1 : 0);

    return 0;
}

}

#endif
