#include "knapsack/ub_surrogate/surrogate.hpp"

#include "knapsack/ub_dantzig/dantzig.hpp"

#include <stdlib.h>

using namespace knapsack;

ItemIdx max_card(const Instance& ins, Info& info)
{
    LOG_FOLD_START(info, "max_card" << std::endl);
    if (ins.item_number() == 1) {
        return 1;
    }

    std::vector<ItemIdx> index(ins.total_item_number());
    std::iota(index.begin(), index.end(), 0);
    ItemIdx f = ins.first_item();
    ItemIdx l = ins.last_item();
    Weight  w = 0;
    Weight  c = ins.capacity();
    while (f < l) {
        if (l - f < 128) {
            std::sort(index.begin()+f, index.begin()+l+1,
                    [&ins](ItemPos j1, ItemPos j2) {
                    return ins.item(j1).w < ins.item(j2).w;});
            break;
        }

        ItemIdx pivot = f + 1 + rand() % (l - f); // Select pivot

        iter_swap(index.begin() + pivot, index.begin() + l);
        ItemIdx j = f;
        for (ItemIdx i=f; i<l; ++i) {
            if (ins.item(index[i]).w > ins.item(index[l]).w)
                continue;
            iter_swap(index.begin() + i, index.begin() + j);
            j++;
        }
        iter_swap(index.begin() + j, index.begin() + l);

        Weight w_curr = w;
        for (ItemIdx i=f; i<j; ++i)
            w_curr += ins.item(index[i]).w;

        if (w_curr + ins.item(index[j]).w <= c) {
            f = j+1;
            w = w_curr + ins.item(index[j]).w;
        } else if (w_curr > c) {
            l = j-1;
        } else {
            break;
        }
    }

    ItemPos k = ins.reduced_solution()->item_number();
    Weight r = ins.capacity();
    for (ItemPos j=ins.first_item(); j<=ins.last_item(); ++j) {
        if (r < ins.item(index[j]).w) {
            k = j;
            break;
        }
        r -= ins.item(index[j]).w;
    }

    LOG_FOLD_END(info, "");
    return k;
}

ItemIdx min_card(const Instance& ins, Info& info, Profit lb)
{
    LOG_FOLD_START(info, "min_card" << std::endl);

    lb -= ins.reduced_solution()->profit();
    if (ins.item_number() <= 1)
        return (ins.item(1).p <= lb)? 1: 0;

    std::vector<ItemIdx> index(ins.total_item_number());
    std::iota(index.begin(), index.end(), 0);
    ItemIdx f = ins.first_item();
    ItemIdx l = ins.last_item();
    Profit p = 0;
    while (f < l) {
        if (l - f < 128) {
            std::sort(index.begin()+f, index.begin()+l+1,
                    [&ins](ItemPos j1, ItemPos j2) {
                    return ins.item(j1).p > ins.item(j2).p;});
            break;
        }

        ItemIdx pivot = f + 1 + rand() % (l - f); // Select pivot

        iter_swap(index.begin() + pivot, index.begin() + l);
        ItemIdx j = f;
        for (ItemIdx i=f; i<l; ++i) {
            if (ins.item(index[i]).p < ins.item(index[l]).p)
                continue;
            iter_swap(index.begin() + i, index.begin() + j);
            j++;
        }
        iter_swap(index.begin() + j, index.begin() + l);

        Profit p_curr = p;
        for (ItemIdx i=f; i<j; ++i)
            p_curr += ins.item(index[i]).p;

        if (p_curr > lb) {
            l = j-1;
        } else if (p_curr + ins.item(index[j]).p <= lb) {
            f = j+1;
            p = p_curr + ins.item(index[j]).p;
        } else {
            break;
        }
    }

    ItemPos k = -1;
    Profit z = 0;
    for (ItemPos j=ins.first_item(); j<=ins.last_item(); ++j) {
        if (z + ins.item(index[j]).p > lb) {
            k = j+1;
            break;
        }
        z += ins.item(index[j]).p;
    }

    LOG_FOLD_END(info, "");
    return k;
}

void ub_surrogate_solve(Instance& ins, Info& info, ItemIdx k,
        Weight s_min, Weight s_max, SurrogateOut& out)
{
    LOG_FOLD_START(info, "ub_surrogate_solve k " << k << " s_min " << s_min << " s_max " << s_max << std::endl);
    out.bound = k;
    ItemPos first = ins.first_item();
    Weight  s_prec = 0;
    Weight  s = 0;
    Weight s1 = s_min;
    Weight s2 = s_max;
    Weight wmax = ins.item(ins.first_item()).w;
    Weight wmin = ins.item(ins.first_item()).w;
    Profit pmax = ins.item(ins.first_item()).p;
    for (ItemPos j=ins.first_item()+1; j<=ins.last_item(); ++j) {
        if (ins.item(j).w > wmax)
            wmax = ins.item(j).w;
        if (ins.item(j).w < wmin)
            wmin = ins.item(j).w;
        if (ins.item(j).p > pmax)
            pmax = ins.item(j).p;
    }
    Weight wabs = wmax;
    Weight wlim = INT_FAST64_MAX / pmax;

    while (s1 <= s2) {
        s = (s1 + s2) / 2;
        LOG_FOLD_START(info, "s1 " << s1 << " s " << s << " s2 " << s2 << std::endl);

        // Avoid INT overflow
        if (s_min == 0 && s != 0) {
            if (INT_FAST64_MAX / s < k
                    || ins.total_capacity() > INT_FAST64_MAX - s * k
                    || INT_FAST64_MAX / ins.total_item_number() < wmax+s
                    || wmax + s > wlim) {
                s2 = s - 1;
                LOG_FOLD_END(info, "");
                continue;
            } else {
                wmax += s - s_prec;
            }
        }
        if (s_max == 0 && s != 0) {
            wabs = (wmax+s > -wmin+s)? wmax+s: wmin+s;
            if (INT_FAST64_MAX / -s < k
                    || INT_FAST64_MAX / ins.total_item_number() < wabs
                    || wabs > wlim) {
                s1 = s + 1;
                LOG_FOLD_END(info, "");
                continue;
            } else {
                wmax += s - s_prec;
                wmin += s - s_prec;
            }
        }

        ins.surrogate(info, s-s_prec, k, first);
        LOG_FOLD(info, ins);
        Info info_tmp(info.logger);
        Profit p = ub_dantzig(ins, info_tmp);
        ItemPos b = ins.break_item();

        LOG(info, "b " << b << " p " << p << std::endl);

        if (p < out.ub) {
            out.ub         = p;
            out.multiplier = s;
        }

        if (b == k && ins.break_capacity() == 0) {
            LOG_FOLD_END(info, "");
            break;
        }

        if ((s_min == 0 && b >= k) || (s_max == 0 && b >= k)) {
            s1 = s + 1;
        } else {
            s2 = s - 1;
        }
        s_prec = s;
        LOG_FOLD_END(info, "");
    }
    ins.surrogate(info, -s, k, first);
    assert(ins.first_item() == first);
    LOG_FOLD_END(info, "");
}

SurrogateOut knapsack::ub_surrogate(const Instance& instance, Profit lb, Info& info)
{
    LOG_FOLD_START(info, "surrogate relaxation lb " << lb << std::endl);
    VER(info, "*** surrogate relaxation ***" << std::endl);
    std::string best = "";

    Instance ins(instance);
    ins.sort_partially(info);
    ItemPos b = ins.break_item();

    SurrogateOut out;

    // Trivial cases
    if (ins.item_number() == 0) {
        algorithm_end(out.ub, info);
        LOG_FOLD_END(info, "");
        return out;
    }
    Info info_tmp(info.logger);
    out.ub = ub_dantzig(ins, info_tmp);
    if (ins.break_capacity() == 0 || b == ins.last_item() + 1) {
        algorithm_end(out.ub, info);
        LOG_FOLD_END(info, "");
        return out;
    }

    // Compte s_min and s_max
    // s_min and s_max should ideally be (-)pmax*wmax, but this may cause
    // overflow
    Weight wmax = ins.item(ins.max_weight_item(info)).w;
    Profit pmax = ins.item(ins.max_profit_item(info)).p;
    Weight s_max = (INT_FAST64_MAX / pmax > wmax)?  pmax*wmax:  INT_FAST64_MAX;
    Weight s_min = (INT_FAST64_MAX / pmax > wmax)? -pmax*wmax: -INT_FAST64_MAX;

    if (max_card(ins, info) == b) {
        ub_surrogate_solve(ins, info, b, 0, s_max, out);
        best = "max";
    } else if (min_card(ins, info, lb) == b + 1) {
        ub_surrogate_solve(ins, info, b + 1, s_min, 0, out);
        if (out.ub < lb) {
            assert(ins.optimal_solution() == NULL || lb == ins.optimal_solution()->profit());
            out.ub = lb;
        }
        best = "min";
    } else {
        SurrogateOut out2;
        out2.ub = out.ub;
        ub_surrogate_solve(ins, info, b,     0,     s_max, out);
        ub_surrogate_solve(ins, info, b + 1, s_min, 0,     out2);
        if (out2.ub < lb) {
            out2.ub = lb;
        }
        if (out2.ub > out.ub) {
            out.ub         = out2.ub;
            out.multiplier = out2.multiplier;
            out.bound      = out2.bound;
        }
        best = "maxmin";
    }

    VER(info, "Best bound: " << best << std::endl);
    PUT(info, "Algorithm.Best", best);
    algorithm_end(out.ub, info);
    LOG_FOLD_END(info, "");
    return out;
}

