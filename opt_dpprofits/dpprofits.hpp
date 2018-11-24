#pragma once

#include "knapsack/lib/instance.hpp"
#include "knapsack/lib/solution.hpp"

namespace knapsack
{

Profit    opt_dpprofits_array(Instance& ins, Info& info);
Solution sopt_dpprofits_array_all(Instance& ins, Info& info);
Solution sopt_dpprofits_array_one(Instance& ins, Info& info);
Solution sopt_dpprofits_array_part(Instance& ins, ItemIdx k, Info& info);
Solution sopt_dpprofits_array_rec(Instance& ins, Info& info);

Profit    opt_dpprofits_list(Instance& ins, Info& info);
Solution sopt_dpprofits_list_all(Instance& ins, Info& info);
Solution sopt_dpprofits_list_one(Instance& ins, Info& info);
Solution sopt_dpprofits_list_part(Instance& ins, Info& info, ItemPos k=64);
Solution sopt_dpprofits_list_rec(Instance& ins, Info& info);

}
