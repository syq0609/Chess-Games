#include "deck.h"
#include "componet/tools.h"
#include <array>
#include "componet/logger.h"

void Deck::_delCards(std::vector<Card>& src, std::vector<Card>& del)
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

bool Deck::isSubset(std::vector<Card> v1, std::vector<Card> v2)
{
    int i = 0, j = 0;
    int m = v1.size();
    int n = v2.size();

    if(m < n)
        return 0;

    //std::sort(v2.begin(), v2.end(), &Deck::lessCard);

    while(i < n && j < m){
        if(rank(v1[j]) < rank(v2[i]))
        {
            j++;
        }
        else if(rank(v1[j]) == rank(v2[i])){
            j++;
            i++;
        }
        else if(rank(v1[j]) > rank(v2[i])){
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

//arr原始数组
//start遍历起始位置
//result保存结果
//count为result数组的索引值，起辅助作用
//NUM为要选取的元素个数
void Deck::combine_decrease(const std::vector<Card>& arr, int start, int* res, int count, const int NUM, CARD_NUM_LIST& ret, int16_t type)
{
	int i;
	for (i = start; i >= count; i--)
	{
		res[count - 1] = i - 1;
		if (count > 1)
		{
			combine_decrease(arr, i - 1, res, count - 1, NUM, ret, type);
		}
		else
		{
            //穷举不同牌型
            bool flag = true;
            const std::vector<Card>& smallStright = { 0, 1, 2, 3, 12 };
            //有空优化下重复
            switch(type)
            {
                //要替换成枚举
                case 0:     //五同
                    {
                        for(int i = 0; i < NUM - 1; ++i)
                        {
                            if(!compareWithGhostTrue(arr[res[i]], arr[res[i + 1]]))
                            {
                                flag = false;
                                break;
                            }
                        }
                    }
                break;
                case 1:     //同花顺
                {
                    std::vector<Card> vec;
                    Card max = 0; 
                    Card min = 0;
                    bool isSameSuit = true;
                    for(int i = 0; i < NUM; ++i)
                    {
                        Card val = arr[res[i]];

                        if(val == 53 || val == 54)
                            continue;

                        if(max == 0 || rank(val) > rank(max))
                            max = val;

                        if(min == 0 || rank(val) < rank(min))
                            min = val;

                        vec.push_back(val);
                    }

                    sort(vec.begin(), vec.end(), &Deck::lessCard);

                    for(uint32_t j = 0; j < vec.size() - 1; ++j)
                    {
                        //花色必须一样
                        if(!compareSuit(vec[j], vec[j + 1]))
                        {
                            isSameSuit = false;
                            flag = false;
                        }

                        //牌面不能一样
                        if(compareRank(vec[j], vec[j + 1]))
                        {
                            flag = false;
                        }
                    }

                    if(rank(max) - rank(min) > 4)
                        flag = false;

                    //特殊的顺子
                    if(!flag && isSameSuit)
                    {
                        if(isSubset(smallStright, vec))
                            flag = true;
                    }
                }
                break;
                case 2:         //铁支
                {
                    flag = false;
                    std::map<int, Card> face;
                    for(int i = 0; i < NUM; ++i)
                    {
                        face[rank(arr[res[i]])]++;
                    }

                    for(auto& p : face)
                    {
                        if(p.first != 15)
                        {
                            //限制五同牌型作为铁支出现
                            if(face[15] + p.second == 4)
                            {
                                flag = true;
                                break;
                            }
                        }
                    }
                }
                break;
                case 3:     //葫芦
                {
                    flag = false;
                    std::map<int, Card> vec;
                    for(int i = 0; i < NUM; ++i)
                    {
                        vec[rank(arr[res[i]])]++;
                    }

                    for(int i = 0; i < 13; ++i)
                    {
                        if(vec[i] + vec[15] == 3)
                        {
                            for(int j = 0; j < 13; ++j)
                            {
                                if(i == j)
                                    continue;

                                int remainGhost = vec[15] - (3 - vec[i] <= 0 ? 0 : 3 - vec[i]);
                                if(vec[j] + remainGhost == 2)
                                {
                                    flag = true;
                                }
                            }
                        }
                    }
                }
                break;
                case 4:         //同花
                {
                    flag = false;
                    bool isSameSuit = true;
                    bool bigBrand = false;
                    Card max = 0; 
                    Card min = 0;
                    std::map<int, Card> m_cards;
                    std::vector<Card> vec;
                    for(int i = 0; i < NUM; ++i)
                    {
                        if(i != NUM - 1)
                        {
                            //花色必须一样
                            if(!compareWithGhostSuit(arr[res[i]], arr[res[i + 1]]))
                                isSameSuit = false;
                        }

                        m_cards[rank(arr[res[i]])]++;

                        Card val = arr[res[i]];
                        if(val == 53 || val == 54)
                            continue;

                        if(max == 0 || rank(val) > rank(max))
                            max = val;

                        if(min == 0 || rank(val) < rank(min))
                            min = val;

                        vec.push_back(val);
                    }

                    if(isSameSuit)
                    {
                        bool hasSame = false;
                        //排除同花情况下的大牌可能
                        for(int i = 0; i < 13; ++i)
                        {
                            if(m_cards[i] + m_cards[15] >= 4)
                            {
                                bigBrand = true;
                                break;
                            }
                            if(m_cards[i] + m_cards[15] == 3)
                            {
                                for(int j = 0; j < 13; ++j)
                                {
                                    if(i == j)
                                        continue;

                                    int remainGhost = m_cards[15] - (3 - m_cards[i] <= 0 ? 0 : 3 - m_cards[i]);
                                    if(m_cards[j] + remainGhost == 2)
                                    {
                                        bigBrand = true;
                                        break;
                                    }
                                }
                            }

                            if(m_cards[i] > 1)
                            {
                                hasSame = true;
                            }
                        }

                        //检测顺子
                        if(!bigBrand && !hasSame)
                        {
                            if(rank(max) - rank(min) <= 4)
                                bigBrand = true;

                            if(!bigBrand)
                            {
                                if(isSubset(smallStright, vec))
                                    bigBrand = true;
                            }
                        }
                    }

                    if(isSameSuit && !bigBrand)
                    {
                        flag = true;
                    }
                }
                break;
                case 5:     //顺子
                {
                    std::vector<Card> vec;
                    Card max = 0; 
                    Card min = 0;
                    bool isSameSuit = true;
                    for(int i = 0; i < NUM; ++i)
                    {
                        Card val = arr[res[i]];

                        if(val == 53 || val == 54)
                            continue;

                        if(max == 0 || rank(val) > rank(max))
                            max = val;

                        if(min == 0 || rank(val) < rank(min))
                            min = val;

                        vec.push_back(val);
                    }

                    sort(vec.begin(), vec.end(), &Deck::lessCard);

                    for(uint32_t j = 0; j < vec.size() - 1; ++j)
                    {
                        if(compareRank(vec[j], vec[j + 1]))
                        {
                            flag = false;
                        }

                        if(!compareSuit(vec[j], vec[j + 1]))
                        {
                            isSameSuit = false;
                        }
                    }

                    if(rank(max) - rank(min) > 4)
                        flag = false;

                    //特殊的顺子
                    if(!flag)
                    {
                        if(isSubset(smallStright, vec))
                            flag = true;
                    }

                    if(isSameSuit)
                        flag = false;
                }
                break;
                case 6:         //三条
                {
                    flag = false;
                    bool isSameSuit = true;
                    std::map<int, Card> vec;
                    for(int i = 0; i < NUM; ++i)
                    {
                        vec[rank(arr[res[i]])]++;
                        if(i != NUM - 1)
                        {
                            if(!compareWithGhostSuit(arr[res[i]], arr[res[i + 1]]))
                                isSameSuit = false;
                        }
                    }

                    for(int i = 0; i < 13; ++i)
                    {
                        if(vec[i] + vec[15] == 3)
                        {
                            flag = true;

                            for(int j = 0; j < 13; ++j)
                            {
                                if(i == j)
                                    continue;

                                int remainGhost = vec[15] - (3 - vec[i] <= 0 ? 0 : 3 - vec[i]);
                                if(vec[j] + remainGhost == 2)
                                {
                                    flag = false;
                                }
                            }
                        }
                    }
                    if(isSameSuit)
                        flag = false;
                }
                break;
                case 7:         //两对
                {
                    flag = false;
                    bool isSameSuit = true;
                    std::map<int, Card> vec;
                    for(int i = 0; i < NUM; ++i)
                    {
                        vec[rank(arr[res[i]])]++;
                        if(i != NUM - 1)
                        {
                            if(!compareWithGhostSuit(arr[res[i]], arr[res[i + 1]]))
                                isSameSuit = false;
                        }
                    }

                    for(int i = 0; i < 13; ++i)
                    {
                        if(vec[i] + vec[15] == 2)
                        {
                            for(int j = 0; j < 13; ++j)
                            {
                                if(i == j)
                                    continue;

                                int remainGhost = vec[15] - (2 - vec[i] <= 0 ? 0 : 2 - vec[i]);
                                if(vec[j] + remainGhost == 2)
                                {
                                    flag = true;
                                }
                            }
                        }
                    }
                    if(isSameSuit)
                        flag = false;
                }
                break;
                case 8:     //对子
                    {
                        flag = false;
                        std::map<int, Card> vec;
                        bool isSameSuit = true;
                        for(int i = 0; i < NUM; ++i)
                        {
                            vec[rank(arr[res[i]])]++;
                            if(i != NUM - 1)
                            {
                                if(!compareWithGhostSuit(arr[res[i]], arr[res[i + 1]]))
                                    isSameSuit = false;
                            }
                        }

                        for(int i = 0; i < 13; ++i)
                        {
                            if(vec[i] + vec[15] == 2)
                            {
                                flag = true;
                                for(int j = 0; j < 13; ++j)
                                {
                                    if(i == j)
                                        continue;

                                    int remainGhost = vec[15] - (2 - vec[i] <= 0 ? 0 : 2 - vec[i]);
                                    if(vec[j] + remainGhost > 1)
                                    {
                                        flag = false;
                                    }
                                }
                            }
                        }

                        if(isSameSuit)
                            flag = false;
                    }
                break;
                case 9:
                    {
                        bool isSameSuit = true;
                        Card max = 0; 
                        Card min = 0;
                        std::map<int, Card> m_cards;
                        std::vector<Card> vec;
                        for(int i = 0; i < NUM; ++i)
                        {
                            if(i != NUM - 1)
                            {
                                if(!compareWithGhostSuit(arr[res[i]], arr[res[i + 1]]))
                                    isSameSuit = false;
                            }

                            m_cards[rank(arr[res[i]])]++;

                            Card val = arr[res[i]];
                            if(val == 53 || val == 54)
                                continue;

                            if(max == 0 || rank(val) > rank(max))
                                max = val;

                            if(min == 0 || rank(val) < rank(min))
                                min = val;

                            vec.push_back(val);
                        }

                        if(isSameSuit)
                        {
                            flag = false;
                        }
                        else
                        {
                            //带鬼就不可能是乌龙了
                            for(int i = 0; i < 13; ++i)
                            {
                                if(m_cards[i] + m_cards[15] > 1)
                                    flag = false;
                            }

                            if(flag)
                            {
                                if(rank(max) - rank(min) <= 4 || isSubset(smallStright, vec))
                                    flag = false;
                            }
                        }
                    }
                break;
                default:
                break;
            }
            //end
			if(flag)
			{
				std::vector<Card> v;
				for (int j = NUM - 1; j >= 0; j--)
					v.push_back(arr[res[j]]);
				ret.push_back(v);
			}
		}
	}
}

//鬼牌可以等于任意牌, 下面2个函数在不同情况下使用
bool Deck::compareWithGhostTrue(int lt, int rt)
{
    if(lt == 53 || lt == 54 || rt == 53 || rt == 54)
    {
        return true;
    }

    return rank(lt) == rank(rt);
}

//bool Deck::compareWithGhostFalse(int lt, int rt)
//{
//    if(lt == 53 || lt == 54 || rt == 53 || rt == 54)
//    {
//        return false;
//    }
//
//    return rank(lt) == rank(rt);
//}

bool Deck::compareWithGhostSuit(int32_t lt, int32_t rt)
{
    if(lt == 53 || lt == 54 || rt == 53 || rt == 54)
        return true;

    return suit(lt) == suit(rt);
}

bool Deck::compareRank(int32_t lt, int32_t rt)
{
    return rank(lt) == rank(rt);
}

bool Deck::compareSuit(int32_t lt, int32_t rt)
{
    return suit(lt) == suit(rt);
}

void Deck::getRecommendPokerSet(const std::vector<Card>& cards, std::vector<Card>& ret)
{
    bool existArr[1000] = {0};
    for(int32_t i = 0; i < 9; ++i)
    {
        CARD_NUM_LIST cards_1;
        const int32_t num = 5;
        int32_t result[num];

        combine_decrease(cards, cards.size(), result, num, num, cards_1, i);

        for(uint32_t l = 0; l < cards_1.size(); ++l)
        {
            std::vector<Card> cards_inner(cards);
            std::vector<Card> tmpCards_1(cards_1[l]);
            _delCards(cards_inner, tmpCards_1);

            bool breakfor = false;
            for(int32_t n = 0; n < 10; ++n)
            {
                CARD_NUM_LIST cards_2;
                combine_decrease(cards_inner, cards_inner.size(), result, num, num, cards_2, n);

                if(cards_2.size() > 0)
                {
                    if(n < i)
                    {
                        breakfor = true;
                        break;
                    }
                    for(uint32_t j = 0; j < cards_2.size(); ++j)
                    {
                        bool cflag = false;
                        if(i == n)          //中墩尾墩牌型一样
                        {
                            cflag = isMessireFive(i, cards_1[l], cards_2[j]);
                        }
                        if(!cflag)
                        {
                            std::vector<Card> cards_core(cards_inner);
                            std::vector<Card> tmpCards_2(cards_2[j]);
                            _delCards(cards_core, tmpCards_2);

                            //只检测,三条,对子,乌龙
                            for(int32_t k = 6; k < 10; ++k)
                            {
                                if(k == 7)
                                    continue;

                                int32_t num3 = 3;
                                int32_t assist[num3];
                                CARD_NUM_LIST cards_3;

                                combine_decrease(cards_core, cards_core.size(), assist, num3, num3, cards_3, k);
                                bool cflag2 = false;
                                if(cards_3.size() > 0)
                                {
                                    if(n < k)
                                    {
                                        cflag2 = true;
                                    }
                                    if(k == n)
                                    {
                                        bool fl = isMessireThree(n, cards_2[j], cards_3[0]);
                                        cflag2 = (fl == false);
                                    }
                                }

                                if(cflag2)
                                {
                                    //同一类型只取一个
                                    if(existArr[i * 100 + n * 10 + k] == false)
                                    {
                                        for(int32_t q = 0; q < 3; ++q)
                                        {
                                            ret.push_back(cards_3[0][q]);
                                        }
                                        for(int32_t q = 0; q < 5; ++q)
                                        {
                                            ret.push_back(cards_2[j][q]);
                                        }
                                        for(int32_t q = 0; q < 5; ++q)
                                        {
                                            ret.push_back(cards_1[l][q]);
                                        }
                                        existArr[i * 100 + n * 10 + k] = true;
                                        //TODO 临时只返回一个
                                        return;
                                        //存下来
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if(breakfor)
                break;
        }
    }
}

bool Deck::isMessireFive(int i, const std::vector<Card>& a, const std::vector<Card>& b)
{
    int32_t v[5] = {0};
    int32_t s[5] = {0};
    //std::vector<int32_t> v;
    //std::vector<int32_t> s;
    //v.reserve(5);
    //s.reserve(5);

    int lgc = 0;        //鬼牌数量
    int rgc = 0;

    for(int q = 0; q < 5; ++q)
    {
        v[q] = rank(a[q]);
        if(v[q] == 15)
            lgc++;
        s[q] = rank(b[q]);
        if(s[q] == 15)
            rgc++;
    }

    std::sort(v, v + 5);
    std::sort(s, s + 5);

    if(i == 0)
    {
        if(v[0] == s[0])
        {
            if(lgc > 0 && rgc == 0)
                return false;
            else if(rgc > 0 && lgc == 0)
                return true;
            else
                return true;        //都有鬼,或都没鬼,相等不算相公
        }
        else
        {
            return v[0] < s[0];
        }
            
    }

    if(i == 2)
    {
        //铁支的带鬼牌比对
        std::map<int, int> lm;
        std::map<int, int> rm;
        int lbigv = -1;
        int lmaxv = -1;
        int lminv = -1;
        int rbigv = -1;
        int rmaxv = -1;
        int rminv = -1;
        for(int i = 0; i < 5; ++i)
        {
            if(v[i] != 15)
            {
                lm[v[i]]++;
                if(lm[v[i]] > 1)
                    lbigv = v[i];

                if(v[i] > lmaxv)
                    lmaxv = v[i];

                if(lminv == - 1 || v[i] < lminv)
                    lminv = v[i];
            }

            if(s[i] != 15)
            {
                rm[s[i]]++;
                if(rm[s[i]] > 1)
                    rbigv = s[i];

                if(s[i] > rmaxv)
                    rmaxv = s[i];

                if(rminv == - 1 || s[i] < rminv)
                    rminv = s[i];
            }
        }
        
        if(lbigv == -1)
            lbigv = lmaxv;      //如果没有数量超过1的,比最大的牌

        if(rbigv == -1)
            rbigv = rmaxv;

        if(lbigv == rbigv)      //点数相同,比另一张牌
        {
            int lothv = (lbigv == lmaxv ? lminv : lmaxv);
            int rothv = (rbigv == rmaxv ? rminv : rmaxv);
            if(lothv == rothv)
            {
                //牌型相等情况下的鬼牌处理
                if(lgc > 0 && rgc == 0)
                    return false;
                else if(rgc > 0 && lgc == 0)
                    return true;
                else
                    return true;
            }
            else
            {
                return lothv < rothv;
            }
        }
        else
        {
            return lbigv < rbigv;
        }
    }

    //同花顺 顺子
    if(i == 1 || i == 5)
    {
        //同花与顺子, 最小牌加4(不超过A),就是最大牌型,所以直接比最小牌就ok
        //这里没有考虑特殊牌型 2 3 4 5 A, TODO
        if(v[0] == s[0])
        {
            if(lgc > 0 && rgc == 0)
                return false;
            else if(rgc > 0 && lgc == 0)
                return true;
            else
                return true;
        }
        else
            return v[0] < s[0];
    }

    //葫芦
    if(i == 3)
    {
        //按特定牌型排序, 对子在前,三条在后
        setGhostToBrand(v, lgc, i);
        setGhostToBrand(s, rgc, i);
        if(v[4] == s[4])
        {
            if(v[0] == s[0])
            {
                if(lgc > 0 && rgc == 0)
                    return false;
                else if(rgc > 0 && lgc == 0)
                    return true;
                else
                    return true;
            }
            else
                return v[0] < s[0];
        }
        else
            return v[4] < s[4];
    }

    if(i == 4)
    {
        //TODO 这里的同花按照对子数量比牌
        setGhostToBrand(v, lgc, i);
        setGhostToBrand(s, rgc, i);
        int lc = 0;
        int rc = 0;
        if(v[3] == v[4])
            lc++;

        if(v[1] == v[2])
            lc++;

        if(s[3] == s[4])
            rc++;

        if(s[1] == s[2])
            rc++;

        if(lc == rc)
        {
            //没对子或对子数量一样,从大到小比牌
            for(int j = 4; j >= 0; j--)
            {
                if(v[j] == s[j])
                {
                    continue;
                }
                return v [j] < s[j];
            }

            if(lgc > 0 && rgc == 0)
                return false;
            else if(rgc > 0 && lgc == 0)
                return true;
            else
                return true;
        }
        else
            return lc < rc;
    }

    if(i == 6)
    {
        setGhostToBrand(v, lgc, i);
        setGhostToBrand(s, rgc, i);
        if(v[4] == s[4])
        {
            if(v[1] == s[1])
            {
                if(v[0] == s[1])
                {
                    if(lgc > 0 && rgc == 0)
                        return false;
                    else if(rgc > 0 && lgc == 0)
                        return true;
                    else
                        return true;
                }
                else
                    return v[0] < s[0];
            }
            else
                return v[1] < s[1];
        }
        else
        {
            return v[4] < s[4];
        }
        
    }
    if(i == 7)
    {
        setGhostToBrand(v, lgc, i);
        setGhostToBrand(s, rgc, i);
        if(v[4] == s[4])
        {
            if(v[2] == s[2])
            {
                if(v[0] == s[0])
                {
                    if(lgc > 0 && rgc == 0)
                        return false;
                    else if(rgc > 0 && lgc == 0)
                        return true;
                    else
                        return true;
                }
                else
                    return v[0] < s[0];
            }
            else
                return v[2] < s[2];
        }
        else
            return v[4] < s[4];
    }
    if(i == 8)
    {
        setGhostToBrand(v, lgc, i);
        setGhostToBrand(s, rgc, i);
        if(v[4] == s[4])
        {
            if(v[2] == s[2])
            {
                if(v[1] == s[1])
                {
                    if(v[0] == s[0])
                    {
                        if(lgc > 0 && rgc == 0)
                            return false;
                        else if(rgc > 0 && lgc == 0)
                            return true;
                        else
                            return true;
                    }
                    else
                        return v[0] < s[0];
                }
                else
                    return v[1] < s[1];
            }
            else
                return v[2] < s[2];
        }
        else
            return v[4] < s[4];
    }
    if(i == 9)
    {
        setGhostToBrand(v, lgc, i);
        setGhostToBrand(s, rgc, i);
        for(int j = 4; j >= 0; j--)
        {
            if(v[j] == s[j])
            {
                continue;
            }
            return v [j] < s[j];
        }

        if(lgc > 0 && rgc == 0)
            return false;
        else if(rgc > 0 && lgc == 0)
            return true;
        else
            return true;
    }

    return false;
}

bool Deck::isMessireThree(int32_t i, const std::vector<Card>& a, const std::vector<Card>& b)
{
    int32_t v[5] = {0};
    int32_t s[3] = {0};
    //std::vector<int32_t> v;
    //std::vector<int32_t> s;
    //v.reserve(5);
    //s.reserve(3);

    int lgc = 0;        //鬼牌数量
    int rgc = 0;

    for(int32_t q = 0; q < 5; ++q)
    {
        v[q] = rank(a[q]);
        if(v[q] == 15)
            lgc++;

        if(q <= 2)
        {
            s[q] = rank(b[q]);
            if(s[q] == 15)
                rgc++;
        }
    }

    std::sort(v, v + 5);
    std::sort(s, s + 3);

    if(i == 6)
    {
        setGhostToBrand(v, lgc, i);
        setGhostToThreeBrand(s, rgc, i);
        if(v[4] == s[2])
        {
            if(lgc > 0 && rgc == 0)
                return false;
            else if(rgc > 0 && lgc == 0)
                return true;
            else
                return true;        //都有鬼,或都没鬼,相等不算相公
        }
        else
            return v[4] < s[2];
    }

    if(i == 8)
    {
        setGhostToBrand(v, lgc, i);
        setGhostToThreeBrand(s, rgc, i);

        if(v[4] == s[2])
        {
            if(v[2] == s[0])
            {
                if(lgc > 0 && rgc == 0)
                    return false;
                else if(rgc > 0 && lgc == 0)
                    return true;
                else
                    return true;        //都有鬼,或都没鬼,相等不算相公
            }
            else
                return v[2] < s[0];
        }
        else
            return v[4] < s[2];
    }

    if(i == 9)
    {
        for(int j = 4; j >= 2; j--)
        {
            if(v[j] == s[j - 2])
            {
                continue;
            }
            return v [j] < s[j - 2];
        }

        if(lgc > 0 && rgc == 0)
            return false;
        else if(rgc > 0 && lgc == 0)
            return true;
        else
            return true;
    }
    return false;
}

void Deck::setGhostToThreeBrand(int32_t* v, int gc, int type)
{
    if(type == 6)       //三张一样的
    {
        if(gc == 3)
        {
            v[2] = v[3] = v[4] = 12;
        }

        if(gc == 2)
        {
            v[2] = v[1] = v[0];
        }

        if(gc == 1)
        {
            v[2] = v[1];
        }
    }

    if(type == 8)
    {
        if(gc == 3)
        {
            v[2] = v[1] = v[0] = 12;
        }

        if(gc == 2)
        {
            v[2] = v[1] = 12;
        }

        if(gc == 1)
        {
            v[2] = v[1];
        }

        if(gc == 0)
        {
            if(v[0] == v[1])
            {
                v[0] = v[2];
                v[2] = v[1];
            }
        }
    }
    
    if(type == 9)
    {
        if(gc == 3)
        {
            v[2] = v[1] = v[0] = 12;
        }

        if(gc == 2)
        {
            v[2] = v[1] = 12;
        }

        if(gc == 1)
        {
            v[2] = 12;
        }
    }
}

void Deck::setGhostToBrand(int32_t* v, int gc, int type)
{
    if(type == 3)       //葫芦          对子在前
    {
        if(gc == 4)
        {
            v[1] = v[0];
            v[2] = v[3] = v[4] = 12;        //A
        }

        if(gc == 3)
        {
            if(v[0] == v[1])
            {
                v[2] = v[3] = v[4] = 12;        //A
            }
            else
            {
                if(v[0] < v[1])
                {
                    v[2] = v[3] = v[4] = v[1];
                    v[1] = v[0];
                }
                else
                {
                    v[2] = v[3] = v[4] = v[0];
                    v[0] = v[1];
                }
            }
        }

        if(gc == 2)
        {
            //3条
            if(v[0] == v[2])            //3条 + 2A          实际可以组5同, 但按都是葫芦的比法,就小了
            {
                v[0] = v[1] = 12;
                v[4] = v[3] = v[2];
            }
            else
            {
                //一定有对子
                int dz = (v[0] == v[1] ? v[0] : v[1]);
                int one = (v[0] == v[1] ? v[2] : v[0]);
                if(one > dz)
                {
                    v[0] = v[1] = dz;
                    v[2] = v[3] = v[4] = one;
                }
                else
                {
                    v[0] = v[1] = one;
                    v[2] = v[3] = v[4] = dz;
                }
            }
        }

        if(gc == 1)
        {
            v[4] = v[3];
        }

        if(gc == 0)
        {
            if(v[0] == v[2])
            {
                int val = v[4];
                v[2] = v[3] = v[4] = v[0];
                v[0] = v[1] = val;
            }
        }
    }

    if(type == 4)       //同花, 单张 小对 大对
    {
        if(gc == 4)
        {
            v[1] = v[2] = v[3] = v[4] = 12;
        }

        if(gc == 3)
        {
            v[3] = v[4] = 12;
            v[2] = v[1];

            if(v[0] == v[1])
            {
                v[0] = 12;
            }
        }

        if(gc == 2)
        {
            if(v[0] == v[1] || v[1] == v[2])
            {
                //有对子
                v[3] = v[4] = 12;
                if(v[0] == v[1])
                {
                    int min = v[2];
                    v[2] = v[0];
                    v[0] = min;
                }
            }
            else
            {
                v[3] = v[4] = v[2];
                v[2] = v[1];
            }
        }

        if(gc == 1)
        {
            //TODO 同花不会有3条出现,鬼牌也不能组3条,出现了调整穷举算法
            if(v[0] == v[1] || v[1] == v[2] || v[2] == v[3])
            {
                v[4] = v[3];

                if(v[0] == v[1])
                {
                    int min = v[2];
                    v[2] = v[0];
                    v[0] = min;
                }
                else if(v[2] == v[3])
                {
                    v[2] = v[1];
                }
            }
            else
            {
                v[4] = v[3];
            }
        }

        if(gc == 0)
        {
            if(v[0] == v[1] && v[2] == v[3])
            {
                //两对
                v[0] = v[4];
                v[4] = v[3];
                v[2] = v[1];
            }
            else if(v[1] == v[2] && v[3] == v[4])
            {
                //正常位置,不处理
            }
            else
            {
                //一对
                if(v[0] == v[1])
                {
                    int o4 = v[4];
                    int o3 = v[3];
                    v[4] = v[3] = v[0];
                    v[0] = v[2];
                    v[1] = o3;
                    v[2] = o4;
                }

                if(v[1] == v[2])
                {
                    int o3 = v[3];
                    v[2] = v[4];
                    v[3] = v[4] = v[1];
                    v[1] = o3;
                }

                if(v[2] == v[3])
                {
                    int o4 = v[4];
                    v[4] = v[3];
                    v[2] = o4;
                }
            }
        }
    }

    if(type == 6)       //三条, 单张 单张 三条
    {
        if(gc == 4)
        {
            v[1] = v[2] = v[3] = v[4] = 12;
        }

        if(gc == 3)
        {
            v[2] = v[3] = v[4] = 12;
        }

        if(gc == 2)
        {
            v[3] = v[4] = v[2];
        }

        if(gc == 1)
        {
            if(v[0] == v[1])
            {
                v[0] = v[2];
                int o3 = v[3];
                v[4] = v[3] = v[2] = v[1];
                v[1] = o3;
            }

            if(v[1] == v[2])
            {
                int o3 = v[3];
                v[4] = v[3] = v[1];
                v[1] = o3;
            }

            if(v[2] == v[3])
            {
                v[4] = v[3];
            }
        }

        if(gc == 0)
        {
            if(v[0] == v[1] && v[1] == v[2])
            {
                v[0] = v[3];
                v[1] = v[4];
                v[4] = v[3] = v[2];
            }
            if(v[1] == v[2] && v[2] == v[3])
            {
                v[1] = v[4];
                v[4] = v[3];
            }
        }
    }

    if(type == 7)       //两对 单张 小对子 大对子
    {
        if(gc == 4)
        {
            v[1] = v[2] = v[3] = v[4] = 12;
        }

        if(gc == 3)
        {
            if(v[0] == v[1])
            {
                v[4] = v[3] = 12;
                v[2] = v[1];
                v[0] = 12;
            }
        }

        if(gc == 2)
        {
            if(v[0] == v[1] || v[1] == v[2])
            {
                v[4] = v[3] = 12;
                if(v[0] == v[1])
                {
                    int o2 = v[2];
                    v[2] = v[0];
                    v[0] = o2;
                }
            }
            else
            {
                v[4] = v[3] = v[2];
                v[2] = v[1];
            }
        }

        if(gc == 1)
        {
            if(v[0] == v[1])
            {
                v[4] = v[3];
                int o2 = v[2];
                v[2] = v[0];
                v[0] = o2;
            }

            if(v[1] == v[2])
            {
                v[4] = v[3];
            }

            if(v[2] == v[3])
            {
                v[4] = v[3];
                v[2] = v[1];
            }
        }

        if(gc == 0)
        {
            if(v[0] == v[1])
            {
                if(v[2] == v[3])
                {
                    v[0] = v[4];
                    v[4] = v[3];
                    v[2] = v[1];
                }

                if(v[3] == v[4])
                {
                    v[0] = v[2];
                    v[2] = v[1];
                }
            }
        }
    }

    if(type == 8)       //一对 单牌 单牌 单牌 一对
    {
        if(gc == 4)
        {
            v[1] = v[2] = v[3] = v[4] = 12;
        }

        if(gc == 3)
        {
            v[4] = v[3] = v[2] = 12;
        }

        if(gc == 2)
        {
            v[4] = v[3] = 12;
        }

        if(gc == 1)
        {
            v[4] = v[3];
        }

        if(gc == 0)
        {
            if(v[0] == v[1])
            {
                v[0] = v[2];
                int o4 = v[4];
                int o3 = v[3];
                v[4] = v[3] = v[1];
                v[1] = o3;
                v[2] = o4;
            }

            if(v[1] == v[2])
            {
                v[1] = v[3];
                int o4 = v[4];
                v[4] = v[3] = v[2];
                v[2] = o4;
                
            }

            if(v[2] == v[3])
            {
                v[2] = v[4];
                v[4] = v[3];
            }
        }
    }

    
    if(type == 9)       //
    {
        if(gc == 4)
        {
            v[1] = v[2] = v[3] = v[4] = 12;
        }

        if(gc == 3)
        {
            v[4] = v[3] = v[2] = 12;
        }

        if(gc == 2)
        {
            v[4] = v[3] = 12;
        }

        if(gc == 1)
        {
            v[4] = 12;
        }
    }
}

