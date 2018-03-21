#include "greedy.hpp"

#define DBG(x)
//#define DBG(x) x

Solution sol_greedy(const Instance& ins, Info* info)
{
    DBG(std::cout << "GREEDYBEST..." << std::endl;)
    assert(ins.break_item_found());

    Solution sol = *ins.break_solution();
    std::string best_algo = "Break";
    ItemPos b = ins.break_item();

    if (b < ins.last_item()) {
        Profit  p = 0;
        ItemPos j = -1;

        DBG(std::cout << "BACKWARD GREEDY" << std::endl;)
        Weight rb = sol.remaining_capacity() - ins.item(b).w;
        for (ItemPos i=ins.first_item(); i<=b; ++i) {
            if (rb + ins.item(i).w >= 0 && ins.item(b).p - ins.item(i).p > p) {
                p = ins.item(b).p - ins.item(i).p;
                j = i;
            }
        }

        DBG(std::cout << "FORWARD GREEDY" << std::endl;)
        Weight rf = sol.remaining_capacity();
        for (ItemPos i=b+1; i<=ins.last_item(); ++i) {
            if (ins.item(i).w <= rf && ins.item(i).p > p) {
                p = ins.item(i).p;
                j = i;
            }
        }

        DBG(std::cout << "B " << b << " J " << j << std::endl;)
        if (j == -1) {
            best_algo = "Break";
        } else if (j <= b) {
            best_algo = "Backward";
            sol.set(b, true);
            sol.set(j, false);
        } else {
            best_algo = "Forward";
            sol.set(j, true);
        }
    }

    if (Info::verbose(info))
        std::cout << "ALGO " << best_algo << std::endl;
    if (info != NULL)
        info->pt.put("Solution.Algo", best_algo);

    ins.check_sol(sol);
    DBG(std::cout << "GREEDY... END" << std::endl;)
    return sol;
}

#undef DBG

