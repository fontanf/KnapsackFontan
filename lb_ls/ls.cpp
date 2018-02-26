#include "ls.hpp"

#include "../lb_greedy/greedy.hpp"

#define DBG(x)
//#define DBG(x) x

std::vector<Item> remove_dominated_items(std::vector<Item>& v)
{
    DBG(std::cout << "REMOVEDOM..." << std::endl;)
    std::vector<Item> t;
    for (Item item: v) {
        if (t.size() == 0
                || (item.w > t.back().w && item.p > t.back().p)) {
            t.push_back(item);
        } else if (item.w == t.back().w && item.p > t.back().p) {
            t.back() = item;
        }
    }
    DBG(std::cout << "REMOVEDOM... END" << std::endl;)
    return t;
}

bool best_exchange(Solution& sol, Info* info)
{
    DBG(std::cout << "BESTEXCHANGE..." << std::endl;)

    const Instance& ins = sol.instance();

    std::vector<Item> taken;
    std::vector<Item> left;
    for (ItemPos i=0; i<ins.item_number(); ++i)
    {
        Item item = ins.item(i);
        item.i = i;
        if (sol.contains(i)) {
            taken.push_back(item);
        } else {
            left.push_back(item);
        }
    }
    std::sort(taken.begin(), taken.end(),
            [](const Item& i1, const Item& i2) {
            return i1.w < i2.w;});
    std::sort(left.begin(), left.end(),
            [](const Item& i1, const Item& i2) {
            return i1.w < i2.w;});
    std::vector<Item> t = remove_dominated_items(taken);
    std::vector<Item> l = remove_dominated_items(left);

    DBG(std::cout << "FIND i1max i2max" << std::endl;)
    Weight r = sol.remaining_capacity();
    Profit pmax = -1;
    auto i1_max = t.end();
    auto i2_max = l.end();
    auto i2 = l.begin();
    for (auto i1 = t.begin(); i1 != t.end(); ++i1) {
        while (i2 != l.end() && i2->w <= i1->w + r)
            i2++;
        if (i2 == l.begin())
            continue;
        Profit p = std::prev(i2)->p - i1->p;
        if (p > pmax) {
            pmax = p;
            i1_max = i1;
            i2_max = std::prev(i2);
            DBG(std::cout << "pmax " << pmax << " i1 " << i1_max->i << " i2 " << i2_max->i << std::endl;)
            //DBG(std::cout << "w1 " << i1->w << " w1+r " << i1->w + r << " i2 " << i2_max->i << std::endl;)
        }
    }
    if (pmax == -1) {
        DBG(std::cout << "pmax == -1" << std::endl;)
        DBG(std::cout << "BESTEXCHANGE... END" << std::endl;)
        return false;
    }
    sol.set(i1_max->i, false);
    sol.set(i2_max->i, true);
    if (Info::verbose(info))
        std::cout
            <<  "LB "  << sol.profit()
            << " GAP " << sol.instance().optimum() - sol.profit()
            << std::endl;

    DBG(std::cout << "BESTEXCHANGE... END" << std::endl;)
    return true;
}

Solution sol_forwardgreedybest(const Instance& ins, Info* info)
{
    DBG(std::cout << "FORWARDGREEDY..." << std::endl;)
    Solution sol = sol_break(ins);
    if (Info::verbose(info))
        std::cout
            <<  "LB "  << sol.profit()
            << " GAP " << sol.instance().optimum() - sol.profit()
            << std::endl;
    best_exchange(sol, info);
    DBG(std::cout << "FORWARDGREEDYBEST... END" << std::endl;)
    return sol;
}

////////////////////////////////////////////////////////////////////////////////

bool best_exchangeback(Solution& sol, Info* info)
{
    DBG(std::cout << "BESTEXCHANGEBACK..." << std::endl;)

    const Instance& ins = sol.instance();

    std::vector<Item> taken;
    std::vector<Item> left;
    for (ItemPos i=0; i<ins.item_number(); ++i)
    {
        Item item = ins.item(i);
        item.i = i;
        if (sol.contains(i)) {
            taken.push_back(item);
        } else {
            left.push_back(item);
        }
    }
    std::sort(taken.begin(), taken.end(),
            [](const Item& i1, const Item& i2) {
            return i1.w < i2.w;});
    std::sort(left.begin(), left.end(),
            [](const Item& i1, const Item& i2) {
            return i1.w < i2.w;});
    std::vector<Item> t = remove_dominated_items(taken);
    std::vector<Item> l = remove_dominated_items(left);

    DBG(std::cout << "FIND i1max i2max" << std::endl;)
    Weight r = sol.remaining_capacity();
    assert(r < 0);
    Profit pmax = -INT_FAST64_MIN;
    auto i1_max = t.end();
    auto i2_max = l.end();
    auto i2 = l.begin();
    for (auto i1 = t.begin(); i1 != t.end(); ++i1) {
        if (i1->w <= -r)
            continue;
        while (i2 != l.end() && i2->w <= i1->w + r)
            i2++;
        if (i2 == l.begin())
            continue;
        Profit p = std::prev(i2)->p - i1->p;
        if (p > pmax) {
            pmax = p;
            i1_max = i1;
            i2_max = std::prev(i2);
            DBG(std::cout << "pmax " << pmax << " i1 " << i1_max->i << " i2 " << i2_max->i << std::endl;)
            //DBG(std::cout << "w1 " << i1->w << " w1+r " << i1->w + r << " i2 " << i2_max->i << std::endl;)
        }
    }
    if (pmax == INT_FAST64_MIN) {
        DBG(std::cout << "pmax == -1" << std::endl;)
        DBG(std::cout << "BESTEXCHANGE... END" << std::endl;)
        return false;
    }
    sol.set(i1_max->i, false);
    sol.set(i2_max->i, true);
    if (Info::verbose(info))
        std::cout
            <<  "LB "  << sol.profit()
            << " GAP " << sol.instance().optimum() - sol.profit()
            << std::endl;

    DBG(std::cout << "BESTEXCHANGEBACK... END" << std::endl;)
    return true;
}

Solution sol_backwardgreedybest(const Instance& ins, Info* info)
{
    DBG(std::cout << "BACKWARDGREEDYBEST..." << std::endl;)
    Solution sol = sol_break(ins);
    Solution sol0 = sol;
    if (Info::verbose(info))
        std::cout
            <<  "LB "  << sol.profit()
            << " GAP " << sol.instance().optimum() - sol.profit()
            << std::endl;
    sol.set(ins.break_item(), true);
    bool b = best_exchangeback(sol, info);
    DBG(std::cout << "BACKWARDGREEDYBEST... END" << std::endl;)
    return (b)? sol: sol0;
}

////////////////////////////////////////////////////////////////////////////////

Solution sol_bestgreedyplus(const Instance& ins, Info* info)
{
    DBG(std::cout << "GREEDYBESTPLUS..." << std::endl;)
    Solution sol = sol_greedy(ins);
    std::string best = "Greedy";
    if (ins.item_number() == 0)
        return sol;
    if (sol.update(sol_greedymax(ins)))
        best = "Max";
    if (sol.update(sol_forwardgreedy(ins)))
        best = "Forward";
    if (sol.update(sol_backwardgreedy(ins)))
        best = "Backward";
    if (sol.update(sol_forwardgreedybest(ins)))
        best = "ForwardBest";
    if (sol.update(sol_backwardgreedybest(ins)))
        best = "BackwardBest";
    if (Info::verbose(info))
        std::cout << "ALG " << best << std::endl;
    DBG(std::cout << "GREEDYBESTPLUS... END" << std::endl;)
    return sol;
}

#undef DBG
