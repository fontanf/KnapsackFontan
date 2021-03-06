#include "knapsacksolver/instance.hpp"
#include "knapsacksolver/solution.hpp"
#include "knapsacksolver/algorithms/dembo.hpp"

#include <sstream>
#include <iomanip>

using namespace knapsacksolver;

/****************************** Create instances ******************************/

Instance::Instance(): f_(0), l_(-1) { }

void Instance::add_item(Weight w, Profit p)
{
    ItemIdx j = items_.size();
    items_.push_back({j, w, p});
    l_ = j;
}

void Instance::clear()
{
    items_.clear();
    c_orig_ = 0;
    sol_opt_ = NULL;
    b_ = -1;
    f_ = 0;
    l_ = -1;
    s_init_ = -1;
    t_init_ = -1;
    s_prime_ = -1;
    t_prime_ = -1;
    sort_type_ = 0;
    int_right_.clear();
    int_left_.clear();
    sol_red_ = NULL;
    sol_break_ = NULL;
}

Instance::Instance(Weight c, const std::vector<std::pair<Weight, Profit>>& wp):
    c_orig_(c), f_(0), l_(-1)
{
    for (const auto& p: wp)
        add_item(p.first, p.second);
}

void Instance::set_optimal_solution(Solution& sol)
{
    sol_opt_ = std::make_unique<Solution>(sol);
}

Instance::Instance(std::string filepath, std::string format)
{
    std::ifstream file(filepath);
    std::ifstream file_sol(filepath + ".sol");
    if (!file.good()) {
        std::cerr << "\033[31m" << "ERROR, unable to open file \"" << filepath << "\"" << "\033[0m" << std::endl;
        assert(false);
        return;
    }

    if (format == "standard") {
        read_standard(file);
    } else if (format == "pisinger") {
        read_pisinger(file);
    } else if (format == "subsetsum_standard") {
        read_subsetsum_standard(file);
    } else {
        std::cerr << "\033[31m" << "ERROR, unknown instance format: \"" << format << "\"" << "\033[0m" << std::endl;
        assert(false);
    }

    f_ = 0;
    l_ = item_number() - 1;
}

void Instance::read_standard(std::ifstream& file)
{
    ItemIdx n;
    file >> n >> c_orig_;

    items_.reserve(n);
    Weight w;
    Profit p;
    for (ItemPos j = 0; j < n; ++j) {
        file >> w >> p;
        add_item(w, p);
    }
}

static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

std::vector<std::string> split(std::string line)
{
    std::vector<std::string> v;
    std::stringstream ss(line);
    std::string tmp;
    while (getline(ss, tmp, ',')) {
        rtrim(tmp);
        v.push_back(tmp);
    }
    return v;
}

void Instance::read_pisinger(std::ifstream& file)
{
    ItemIdx n;
    std::string tmp;
    Profit opt;
    file >> tmp >> tmp >> n >> tmp >> c_orig_ >> tmp >> opt >> tmp >> tmp;

    items_.reserve(n);
    std::vector<int> x(n);
    getline(file, tmp);
    for (ItemPos j = 0; j < n; ++j) {
        getline(file, tmp);
        std::vector<std::string> line = split(tmp);
        add_item(std::stol(line[2]), std::stol(line[1]));
        x[j] = std::stol(line[3]);
    }

    sol_opt_ = std::make_unique<Solution>(*this);
    for (ItemPos j = 0; j < n; ++j)
        sol_opt_->set(j, x[j]);
    assert(sol_opt_->profit() == opt);
}

void Instance::read_subsetsum_standard(std::ifstream& file)
{
    ItemIdx n;
    file >> n >> c_orig_;

    items_.reserve(n);
    Weight w;
    for (ItemPos j = 0; j < n; ++j) {
        file >> w;
        add_item(w,w);
    }
}

Instance::Instance(const Instance& instance):
    items_(instance.items_),
    c_orig_(instance.c_orig_),
    b_(instance.b_),
    f_(instance.f_),
    l_(instance.l_),
    s_init_(instance.s_init_),
    t_init_(instance.t_init_),
    s_prime_(instance.s_prime_),
    t_prime_(instance.t_prime_),
    sort_type_(instance.sort_type_),
    int_right_(instance.int_right_),
    int_left_(instance.int_left_)
{
    if (instance.optimal_solution() != NULL) {
        sol_opt_ = std::make_unique<Solution>(*this);
        *sol_opt_ = *instance.optimal_solution();
    }
    if (instance.break_solution() != NULL) {
        sol_break_ = std::make_unique<Solution>(*this);
        *sol_break_ = *instance.break_solution();
    }
    if (instance.reduced_solution() != NULL) {
        sol_red_ = std::make_unique<Solution>(*this);
        *sol_red_ = *instance.reduced_solution();
    }
}

Instance& Instance::operator=(const Instance& instance)
{
    if (this != &instance) {
        items_ = instance.items_;
        c_orig_ = instance.c_orig_;

        b_ = instance.b_;
        f_ = instance.f_;
        l_ = instance.l_;
        s_init_ = instance.s_init_;
        t_init_ = instance.t_init_;
        s_prime_ = instance.s_prime_;
        t_prime_ = instance.t_prime_;
        sort_type_ = instance.sort_type_;
        int_right_ = instance.int_right_;
        int_left_ = instance.int_left_;

        if (instance.optimal_solution() != NULL) {
            sol_opt_ = std::make_unique<Solution>(*this);
            *sol_opt_ = *instance.optimal_solution();
        }
        if (instance.break_solution() != NULL) {
            sol_break_ = std::make_unique<Solution>(*this);
            *sol_break_ = *instance.break_solution();
        }
        if (instance.reduced_solution() != NULL) {
            sol_red_ = std::make_unique<Solution>(*this);
            *sol_red_ = *instance.reduced_solution();
        }
    }
    return *this;
}

Instance::~Instance() { }

Instance Instance::reset(const Instance& instance)
{
    Instance instance_new;
    instance_new.items_  = instance.items_;
    instance_new.c_orig_ = instance.c_orig_;
    instance_new.f_ = 0;
    instance_new.l_ = instance.items_.size() - 1;
    return instance;
}

bool Instance::check()
{
    for (ItemPos j = 0; j < reduced_item_number(); ++j)
        if (item(j).w <= 0 || item(j).w > reduced_capacity())
            return false;
    return true;
}

/******************************************************************************/

ItemPos Instance::max_efficiency_item(Info& info) const
{
    ItemPos k = -1;
    for (ItemPos j = first_item(); j <= last_item(); ++j)
        if (k == -1 || item(j).p * item(k).w > item(k).p * item(j).w)
            k = j;
    LOG(info, "max_efficiency_item " << k << std::endl);
    return k;
}

ItemPos Instance::before_break_item(Info& info) const
{
    ItemPos k = -1;
    for (ItemPos j = first_item(); j < break_item(); ++j)
        if (k == -1 || (item(k).p * item(j).w > item(j).p * item(k).w))
            k = j;
    LOG(info, "before_break_item " << k << std::endl);
    return k;
}

ItemPos Instance::max_profit_item(Info& info) const
{
    ItemPos k = -1;
    for (ItemPos j = first_item(); j <= last_item(); ++j)
        if (k == -1 || item(j).p > item(k).p)
            k = j;
    LOG(info, "max_profit_item " << k << std::endl);
    return k;
}

ItemPos Instance::min_profit_item(Info& info) const
{
    ItemPos k = -1;
    for (ItemPos j = first_item(); j <= last_item(); ++j)
        if (k == -1 || item(j).p < item(k).p)
            k = j;
    LOG(info, "min_profit_item " << k << std::endl);
    return k;
}

ItemPos Instance::max_weight_item(Info& info) const
{
    ItemPos k = -1;
    for (ItemPos j = first_item(); j <= last_item(); ++j)
        if (k == -1 || item(j).w > item(k).w)
            k = j;
    LOG(info, "max_weight_item " << k << std::endl);
    return k;
}

ItemPos Instance::min_weight_item(Info& info) const
{
    ItemPos k = -1;
    for (ItemPos j = first_item(); j <= last_item(); ++j)
        if (k == -1 || item(j).w < item(k).w)
            k = j;
    LOG(info, "min_weight_item " << k << std::endl);
    return k;
}

std::vector<Weight> Instance::min_weights() const
{
    ItemIdx n = item_number();
    std::vector<Weight> min_weight(n);
    min_weight[n - 1] = item(n - 1).w;
    for (ItemIdx i = n - 2; i >= 0; --i)
        min_weight[i] = std::min(item(i).w, min_weight[i + 1]);
    return min_weight;
}

std::vector<Item> Instance::get_isum() const
{
    assert(sort_type() == 2);
    std::vector<Item> isum;
    isum.reserve(item_number()+1);
    isum.clear();
    isum.push_back({0,0,0});
    for (ItemPos j = 1; j <= item_number(); ++j)
        isum.push_back({j, isum[j - 1].w + item(j - 1).w, isum[j - 1].p + item(j - 1).p});
    return isum;
}

ItemPos Instance::gamma1(Info& info) const
{
    Weight w = break_weight() - item(break_item() - 1).w;
    ItemPos k = -1;
    for (ItemPos j = break_item() + 1; j <= last_item(); ++j)
        if ((k == -1 || item(k).p < item(j).p) && w + item(j).w <= capacity())
            k = j;
    LOG(info, "gamma1 " << k << std::endl);
    return k;

}

ItemPos Instance::gamma2(Info& info) const
{
    Weight w = break_weight() + item(break_item()).w;
    ItemPos k = -1;
    for (ItemPos j = first_item(); j < break_item(); ++j)
        if ((k == -1 || item(k).p > item(j).p) && w - item(j).w <= capacity())
            k = j;
    LOG(info, "gamma2 " << k << std::endl);
    return k;
}

ItemPos Instance::beta1(Info& info) const
{
    ItemPos k = -1;
    for (ItemPos j = break_item() + 1; j <= last_item(); ++j)
        if ((k == -1 || item(k).p < item(j).p) && break_weight() + item(j).w <= capacity())
            k = j;
    LOG(info, "beta1 " << k << std::endl);
    return k;

}

ItemPos Instance::beta2(Info& info) const
{
    ItemPos k = -1;
    if (break_item() + 1 <= last_item()) {
        Weight w = break_weight() + item(break_item()).w + item(break_item() + 1).w;
        for (ItemPos j = first_item(); j < break_item(); ++j)
            if ((k == -1 || item(k).p > item(j).p) && w - item(j).w <= capacity())
                k = j;
    }
    LOG(info, "beta2 " << k << std::endl);
    return k;
}

Profit Instance::optimum() const
{
    if (optimal_solution() == NULL)
        return -1;
    return optimal_solution()->profit();
}

/******************************************************************************/

void Instance::compute_break_item(Info& info)
{
    LOG_FOLD_START(info, "compute_break_item f " << first_item() << std::endl);
    LOG(info, "reduced solution " << reduced_solution()->to_string_items() << std::endl);

    if (sol_break_ == NULL) {
        sol_break_ = std::make_unique<Solution>(*reduced_solution());
    } else {
        *sol_break_ = *reduced_solution();
    }
    for (b_ = first_item(); b_ <= last_item(); ++b_) {
        if (item(b_).w > sol_break_->remaining_capacity())
            break;
        sol_break_->set(b_, true);
    }
    LOG(info, "break solution " << sol_break_->to_string_items() << std::endl);
    LOG_FOLD_END(info, "compute_break_item b " << b_);
}

Profit Instance::break_profit() const
{
    return break_solution()->profit() - reduced_solution()->profit();
}

Weight Instance::break_weight() const
{
    return break_solution()->weight() - reduced_solution()->weight();
}

Weight Instance::break_capacity() const
{
    return break_solution()->remaining_capacity();
}

Weight Instance::reduced_capacity() const
{
    return (reduced_solution() == NULL)?
        capacity(): capacity() - reduced_solution()->weight();
}

/******************************************************************************/

void Instance::sort(Info& info)
{
    LOG_FOLD_START(info, "sort" << std::endl);
    if (sort_type() == 2) {
        LOG_FOLD_END(info, "sort already sorted");
        return;
    }
    if (reduced_solution() == NULL)
        sol_red_ = std::make_unique<Solution>(*this);
    sort_type_ = 2;
    if (reduced_item_number() > 1)
        std::sort(items_.begin()+first_item(), items_.begin()+last_item()+1,
                [](const Item& i1, const Item& i2) {
                return i1.p * i2.w > i2.p * i1.w;});

    compute_break_item(info);
    LOG_FOLD_END(info, "sort");
}

void Instance::remove_big_items(Info& info)
{
    LOG_FOLD_START(info, "remove_big_items" << std::endl);
    if (b_ != -1 && item(b_).w > reduced_capacity())
        b_ = -1;

    if (sort_type() == 2) {
        std::vector<Item> not_fixed;
        std::vector<Item> fixed_0;
        for (ItemPos j = first_item(); j <= last_item(); ++j) {
            if (item(j).w > reduced_capacity()) {
                fixed_0.push_back(item(j));
                LOG(info, "remove " << j << " (" << item(j) << ")" << std::endl);
            } else {
                not_fixed.push_back(item(j));
            }
        }
        if (fixed_0.size() != 0) {
            ItemPos j = not_fixed.size();
            std::copy(not_fixed.begin(), not_fixed.end(), items_.begin()+f_);
            std::copy(fixed_0.begin(), fixed_0.end(), items_.begin()+f_+j);
            l_ = f_+j-1;
        }

        if (b_ == -1)
            compute_break_item(info);
    } else {
        for (ItemPos j = first_item(); j <= last_item();) {
            if (item(j).w > reduced_capacity()) {
                swap(j, l_);
                l_--;
            } else {
                j++;
            }
        }

        sort_type_ = 0;
        sort_partially(info);
    }
    LOG_FOLD_END(info, "remove_big_items");
}

std::pair<ItemPos, ItemPos> Instance::partition(ItemPos f, ItemPos l, Info& info)
{
    LOG_FOLD_START(info, "partition f " << f << " l " << l << std::endl);
    ItemPos pivot = f + 1 + rand() % (l - f); // Select pivot
    Weight w = item(pivot).w;
    Profit p = item(pivot).p;
    LOG(info, "pivot " << pivot
            << " w_pivot " << w << " p_pivot " << p
            << " e_pivot " << item(pivot).efficiency() << std::endl);

    // Partition
    swap(pivot, l);
    ItemPos j = f;
    while (j <= l) {
        if (item(j).p * w > p * item(j).w) {
            swap(j, f);
            f++;
            j++;
        } else if (item(j).p * w < p * item(j).w) {
            swap(j, l);
            l--;
        } else {
            j++;
        }
    }

    // | | | | | | | | | | | | | | | | | | | | |
    //          f       j           l
    // -------|                       |-------
    // > pivot                         < pivot

    LOG(info, "f " << f << " l " << l << std::endl);

    LOG_FOLD_END(info, "partition");
    return {f,l};
}

void Instance::sort_partially(Info& info, ItemIdx limit)
{
    LOG_FOLD_START(info, "sort_partially limit " << limit << std::endl);

    if (sort_type_ >= 1) {
        LOG_FOLD_END(info, "sort_partially already sorted");
        return;
    }

    if (reduced_solution() == NULL)
        sol_red_ = std::make_unique<Solution>(*this);

    srand(0);
    int_right_.clear();
    int_left_.clear();

    // Quick sort like algorithm
    ItemPos f = first_item();
    ItemPos l = last_item();
    Weight c = reduced_capacity();
    while (f < l) {
        LOG(info, "f " << f << " l " << l << std::endl);
        if (l - f < limit) {
            std::sort(items_.begin() + f, items_.begin() + l + 1,
                    [](const Item& i1, const Item& i2) {
                    return i1.p * i2.w > i2.p * i1.w;});
            break;
        }

        std::pair<ItemPos, ItemPos> fl = partition(f, l, info);
        ItemPos w = 0;
        for (ItemPos k = f; k < fl.first; ++k)
            w += item(k).w;

        if (w > c) {
            if (fl.second + 1 <= l)
                int_right_.push_back({fl.second + 1, l});
            int_right_.push_back({fl.first, fl.second});
            l = fl.first - 1;
            continue;
        }

        for (ItemPos k = fl.first; k <= fl.second; ++k)
            w += item(k).w;
        if (w > c) {
            break;
        } else {
            c -= w;
            if (f <= fl.first - 1)
                int_left_.push_back({f, fl.first - 1});
            int_left_.push_back({fl.first, fl.second});
            f = fl.second + 1;
        }
    }

    sort_type_ = 1;

    compute_break_item(info);

    if (f < b_)
        int_left_.push_back({f, b_ - 1});
    if (b_ < l)
        int_right_.push_back({b_ + 1, l});
    s_prime_ = b_;
    t_prime_ = b_;
    s_init_ = b_;
    t_init_ = b_;

    assert(check_partialsort(info));
    LOG_FOLD(info, *this);
    LOG_FOLD_END(info, "sort_partially");
}

void Instance::sort_right(Info& info, Profit lb)
{
    LOG_FOLD_START(info, "sort_right lb " << lb << std::endl);
    Interval in = int_right_.back();
    int_right_.pop_back();
    ItemPos k = t_prime();
    LOG(info, "in.f " << in.f << " in.l " << in.l << std::endl);
    for (ItemPos j = in.f; j <= in.l; ++j) {
        LOG(info, "j " << j << " (" << item(j) << ")");
        Profit p = break_solution()->profit() + item(break_item()).p + item(j).p;
        Weight r = break_capacity() - item(break_item()).w - item(j).w;
        assert(r < 0);
        Profit ub = ub_dembo_rev(*this, break_item(), p, r);
        LOG(info, " ub " << ub);
        if (item(j).w <= reduced_capacity() && ub > lb) {
            k++;
            swap(k, j);
            LOG(info, " swap j " << j << " k " << k << std::endl);
        } else {
            LOG(info, " set 0" << std::endl);
        }
    }
    std::sort(items_.begin() + t_prime() + 1, items_.begin() + k + 1,
            [](const Item& i1, const Item& i2) {
            return i1.p * i2.w > i2.p * i1.w;});
    t_prime_ = k;
    if (int_right_.size() == 0) {
        l_ = t_prime();
        LOG(info, "l_ " << l_ << std::endl);
    }
    if (first_item() >= s_prime() && last_item() <= t_prime()) {
        if (s_init_ == t_init_) {
            sort_type_ = 2;
        } else {
            sort_type_ = 0;
        }
    }
    LOG_FOLD_END(info, "sort_right");
}

void Instance::sort_left(Info& info, Profit lb)
{
    LOG_FOLD_START(info, "sort_left lb " << lb << std::endl);
    LOG(info, "s_prime " << s_prime() << std::endl);
    Interval in = int_left_.back();
    int_left_.pop_back();
    ItemPos k = s_prime();
    LOG(info, "in.l " << in.f << " in.f " << in.l << " b " << break_item() << std::endl);
    for (ItemPos j = in.l; j >= in.f; --j) {
        LOG(info, "j " << j << " (" << item(j) << ")");
        Profit p = break_solution()->profit() - item(j).p;
        Weight r = break_capacity() + item(j).w;
        assert(r > 0);
        Profit ub = ub_dembo(*this, break_item(), p, r);
        LOG(info, " ub " << ub);
        if (item(j).w <= reduced_capacity() && ub > lb) {
            k--;
            swap(k, j);
            LOG(info, " swap j " << j << " k " << k << std::endl);
        } else {
            LOG(info, " set 1" << std::endl);
            sol_red_->set(j, true);
        }
    }
    std::sort(items_.begin() + k, items_.begin() + s_prime(),
            [](const Item& i1, const Item& i2) {
            return i1.p * i2.w > i2.p * i1.w;});
    s_prime_ = k;
    LOG(info, "s_prime " << s_prime() << std::endl);
    if (int_left_.size() == 0) {
        f_ = s_prime();
        LOG(info, "f_ " << f_ << std::endl);
    }
    if (first_item() >= s_prime() && last_item() <= t_prime()) {
        if (s_init_ == t_init_) {
            sort_type_ = 2;
        } else {
            sort_type_ = 0;
        }
    }
    LOG_FOLD_END(info, "sort_left");
}

ItemPos Instance::bound_item_left(ItemPos s, Profit lb, Info& info)
{
    LOG_FOLD_START(info, "bound_item_left s " << s << std::endl);
    while (s < s_prime() && int_left().size() > 0)
        sort_left(info, lb);
    if (s < first_item()) {
        LOG_FOLD_END(info, "bound_item_left " << first_item() + 1);
        return first_item() - 1;
    } else if (s >= s_init()) {
        LOG_FOLD_END(info, "bound_item_left " << break_item());
        return break_item();
    } else {
        LOG_FOLD_END(info, "bound_item_left " << s);
        return s;
    }
}

ItemPos Instance::bound_item_right(ItemPos t, Profit lb, Info& info)
{
    LOG_FOLD_START(info, "bound_item_right t " << t << std::endl);
    while (t > t_prime() && int_right().size() > 0)
        sort_right(info, lb);
    if (t >= last_item() + 1) {
        LOG_FOLD_END(info, "bound_item_right " << last_item() + 1);
        return last_item() + 1;
    } else if (t <= t_init()) {
        LOG_FOLD_END(info, "bound_item_right " << break_item());
        return break_item();
    } else {
        LOG_FOLD_END(info, "bound_item_right " << t);
        return t;
    }
}

void Instance::add_item_to_core(ItemPos s, ItemPos t, ItemPos j, Info& info)
{
    LOG_FOLD_START(info, "add_item_to_initial_core j " << j << std::endl);
    if (j == -1) {
        LOG_FOLD_END(info, "add_item_to_initial_core j = -1");
        return;
    }
    LOG(info, "item " << item(j) << std::endl);
    if (s <= j && j <= t) {
        LOG_FOLD_END(info, "add_item_to_initial_core j already in core");
        return;
    }

    Item it_tmp = items_[j];
    if (j < break_item()) {
        LOG(info, "step 1" << std::endl);
        if (j < s_second()) {
            for (auto in = int_left_.begin(); in != int_left_.end();) {
                if (in->l < j) {
                    in++;
                    continue;
                }
                LOG(info, "move " << in->l << " (" << item(in->l) << ") to " << j << std::endl);
                items_[j] = items_[in->l];
                j = in->l;
                in->l--;
                if (std::next(in) != int_left_.end())
                    std::next(in)->f--;
                if (in->f > in->l) {
                    in = int_left_.erase(in);
                } else {
                    in++;
                }
            }
            s_prime_--;
        }

        LOG(info, "step 2" << std::endl);
        if (j < s_prime()) {
            LOG(info, "move " << s_prime() << " (" << item(s_prime()) << ") to " << j << std::endl);
            items_[j] = items_[s_prime()];
            j = s_prime();
        }

        LOG(info, "step 3" << std::endl);
        while (j != s) {
            LOG(info, "move " << j + 1 << " (" << item(j + 1) << ") to " << j << std::endl);
            items_[j] = items_[j + 1];
            j++;
        }

        items_[s] = it_tmp;

        s_init_--;
    } else {
        LOG(info, "step 1" << std::endl);
        if (j > t_second()) {
            for (auto in = int_right_.begin(); in != int_right_.end();) {
                LOG(info, "interval " << *in << std::endl);
                if (in->f > j) {
                    in++;
                    continue;
                }
                LOG(info, "move " << in->f << " (" << item(in->f) << ") to " << j << std::endl);
                items_[j] = items_[in->f];
                j = in->f;
                in->f++;
                if (std::next(in) != int_right_.end())
                    std::next(in)->l++;
                if (in->f > in->l) {
                    in = int_right_.erase(in);
                } else {
                    in++;
                }
            }
            t_prime_++;
        }

        LOG(info, "step 2" << std::endl);
        if (j > t_prime()) {
            LOG(info, "move " << t_prime() << " (" << item(t_prime()) << ") to " << j << std::endl);
            items_[j] = items_[t_prime()];
            j = t_prime();
        }

        LOG(info, "step 3" << std::endl);
        while (j != t) {
            LOG(info, "move " << j - 1 << " (" << item(j - 1) << ") to " << j << std::endl);
            items_[j] = items_[j - 1];
            j--;
        }

        items_[t] = it_tmp;

        t_init_++;
    }
    sort_type_ = 1;

    LOG_FOLD(info, *this);
    LOG_FOLD_END(info, "add_item_to_initial_core");
}

bool Instance::check_partialsort(Info& info) const
{
    LOG_FOLD(info, *this);

    if (reduced_item_number() == 0) {
        if (break_item() != last_item() + 1) {
            std::cout << 0 << std::endl;
            LOG_FOLD_END(info, "b " << break_item() << " != l + 1 " << last_item() + 1);
            return false;
        }
        return true;
    }

    Weight w_total = reduced_solution()->weight();
    for (ItemPos j = first_item(); j <= last_item(); ++j)
        w_total += item(j).w;
    if (w_total <= capacity()) {
        if (break_item() != last_item() + 1) {
            std::cout << 1 << std::endl;
            LOG_FOLD_END(info, "w_red " << reduced_solution()->weight()
                    << " w_tot " << w_total
                    << " c_tot " << capacity()
                    << " b " << break_item()
                    << " != l + 1 " << last_item() + 1);
            return false;
        }
        return true;
    }

    if (break_item() < 0 || break_item() >= item_number()) {
        std::cout << 2 << std::endl;
        LOG_FOLD_END(info, "b " << break_item());
        return false;
    }
    if (break_solution()->weight() > capacity()) {
        std::cout << *this << std::endl;
        std::cout << 3 << std::endl;
        LOG_FOLD_END(info, "wbar " << break_solution()->weight() << " c " << capacity());
        return false;
    }
    if (break_solution()->weight() + item(break_item()).w <= capacity()) {
        std::cout << 4 << std::endl;
        LOG_FOLD_END(info, "wbar + wb " << break_solution()->weight() + item(break_item()).w << " c " << capacity());
        return false;
    }
    for (ItemPos j = first_item(); j < break_item(); ++j) {
        if (item(j).p * item(break_item()).w < item(break_item()).p * item(j).w) {
            std::cout << 5 << std::endl;
            LOG_FOLD_END(info, "j " << j << "(" << item(j) << ") b " << break_item() << "(" << item(break_item()) << ")");
            return false;
        }
    }
    for (ItemPos j = break_item() + 1; j <= last_item(); ++j) {
        if (item(j).p * item(break_item()).w > item(break_item()).p * item(j).w) {
            std::cout << 6 << std::endl;
            LOG_FOLD_END(info, "j " << j << "(" << item(j) << ") b " << break_item() << "(" << item(break_item()) << ")");
            return false;
        }
    }

    if (int_left().size() != 0) {
        if (int_left().back().l > s_prime() - 1) {
            std::cout << 7 << std::endl;
            LOG_FOLD_END(info, "int_left().back().l " << int_left().back().l << " s " << s_prime());
            return false;
        }
        for (auto it = int_left().begin(); it != std::prev(int_left().end()); ++it)
            if (it->l != std::next(it)->f - 1) {
                std::cout << 8 << std::endl;
                return false;
            }
        Effciency emin_prev = INT_FAST64_MAX;
        for (auto i: int_left_) {
            if (i.f > i.l) {
                std::cout << 9 << std::endl;
                return false;
            }
            Effciency emax = 0;
            Effciency emin = INT_FAST64_MAX;
            for (ItemPos j = i.f; j <= i.l; ++j) {
                if (emax < item(j).efficiency())
                    emax = item(j).efficiency();
                if (emin > item(j).efficiency())
                    emin = item(j).efficiency();
            }
            if (emax > emin_prev) {
                std::cout << 10 << std::endl;
                return false;
            }
            emin_prev = emin;
        }
    }
    if (int_right().size() != 0) {
        if (int_right_.back().f < t_prime() + 1) {
            std::cout << 11 << std::endl;
            return false;
        }
        for (auto it = int_right().rbegin(); it != std::prev(int_right().rend()); ++it)
            if (it->l != std::next(it)->f - 1) {
                std::cout << 12 << std::endl;
                return false;
            }
        Effciency emax_prev = 0;
        for (auto i: int_right_) {
            if (i.f > i.l) {
                std::cout << 13 << std::endl;
                return false;
            }
            Effciency emax = 0;
            Effciency emin = INT_FAST64_MAX;
            for (ItemPos j = i.f; j <= i.l; ++j) {
                if (emax < item(j).efficiency())
                    emax = item(j).efficiency();
                if (emin > item(j).efficiency())
                    emin = item(j).efficiency();
            }
            if (emin < emax_prev) {
                std::cout << 14 << std::endl;
                return false;
            }
            emax_prev = emax;
        }
    }
    return true;
}

void Instance::init_combo_core(Info& info)
{
    LOG_FOLD_START(info, "init_combo_core" << std::endl);
    assert(sort_type_ >= 1);
    add_item_to_core(s_init_ - 1, t_init_ + 1, before_break_item(info), info);
    add_item_to_core(s_init_ - 1, t_init_ + 1, gamma1(info), info);
    add_item_to_core(s_init_ - 1, t_init_ + 1, gamma2(info), info);
    add_item_to_core(s_init_ - 1, t_init_ + 1, beta1(info), info);
    add_item_to_core(s_init_ - 1, t_init_ + 1, beta2(info), info);
    add_item_to_core(s_init_ - 1, t_init_ + 1, max_weight_item(info), info);
    add_item_to_core(s_init_ - 1, t_init_ + 1, min_weight_item(info), info);
    assert(check_partialsort(info));
    LOG_FOLD_END(info, "init_combo_core");
}

void Instance::reduce1(Profit lb, Info& info)
{
    LOG_FOLD_START(info, "reduce1 - lb " << lb << " b_ " << b_ << std::endl;);

    assert(b_ != l_+1);

    for (ItemIdx j = f_; j < b_;) {
        LOG(info, "j " << j << " (" << item(j) << ") f_ " << f_);

        Profit ub = reduced_solution()->profit() + break_profit() - item(j).p
                + ((break_capacity() + item(j).w) * item(b_).p) / item(b_).w;
        LOG(info, " ub " << ub);

        if (ub <= lb) {
            LOG(info, " 1" << std::endl);
            sol_red_->set(j, true);
            if (j != f_)
                swap(j, f_);
            f_++;
            if (reduced_capacity() < 0)
                return;
        } else {
            LOG(info, " ?" << std::endl);
        }
        j++;
    }
    for (ItemPos j = l_; j > b_;) {
        LOG(info, "j " << j << " (" << item(j) << ") l_ " << l_);

        Profit ub = reduced_solution()->profit() + break_profit() + item(j).p
                + ((break_capacity() - item(j).w) * item(b_).p) / item(b_).w;
        LOG(info, " ub " << ub)

        if (ub <= lb) {
            LOG(info, " 0" << std::endl);
            if (j != l_)
                swap(j, l_);
            l_--;
        } else {
            LOG(info, " ?" << std::endl);
        }
        j--;
    }

    remove_big_items(info);

    VER(info, "Reduction: " << lb << " - "
            << "n " << reduced_item_number() << "/" << item_number()
            << " ("  << ((double)reduced_item_number() / (double)item_number()) << ") -"
            << " c " << ((double)reduced_capacity()    / (double)capacity()) << std::endl);
    LOG(info, "n " << reduced_item_number() << "/" << item_number() << std::endl);
    LOG(info, "c " << reduced_capacity() << "/" << capacity() << std::endl);
    LOG_FOLD_END(info, "reduce1");
}

ItemPos Instance::ub_item(const std::vector<Item>& isum, Item item) const
{
    assert(sort_type() == 2);
    auto s = std::upper_bound(isum.begin() + f_, isum.begin() + l_ + 1, item,
            [](const Item& i1, const Item& i2) { return i1.w < i2.w;});
    if (s == isum.begin() + l_ + 1)
        return l_ + 1;
    return s->j - 1;
}

void Instance::reduce2(Profit lb, Info& info)
{
    LOG_FOLD_START(info, "Reduce 2: lb " << lb << " b_ " << b_ << std::endl);
    assert(sort_type() == 2);

    std::vector<Item> isum = get_isum();

    std::vector<Item> not_fixed;
    std::vector<Item> fixed_1;
    std::vector<Item> fixed_0;

    for (ItemPos j = f_; j <= b_; ++j) {
        LOG(info, "j " << j << " (" << item(j) << ")");
        Item ubitem = {0, capacity() + item(j).w, 0};
        ItemPos bb = ub_item(isum, ubitem);
        LOG(info, " bb " << bb);
        Profit ub = 0;
        if (bb == last_item() + 1) {
            ub = isum[last_item()+1].p - item(j).p;
            LOG(info, " ub " << ub);
        } else if (bb == last_item()) {
            Profit ub1 = isum[bb  ].p - item(j).p;
            Profit ub2 = isum[bb+1].p - item(j).p + ((capacity() + item(j).w - isum[bb+1].w) * item(bb-1).p + 1) / item(bb-1).w - 1;
            ub = (ub1 > ub2)? ub1: ub2;
            LOG(info, " ub1 " << ub1 << " ub2 " << ub2 << " ub " << ub);
        } else {
            Profit ub1 = isum[bb  ].p - item(j).p + ((capacity() + item(j).w - isum[bb  ].w) * item(bb+1).p    ) / item(bb+1).w;
            Profit ub2 = isum[bb+1].p - item(j).p + ((capacity() + item(j).w - isum[bb+1].w) * item(bb-1).p + 1) / item(bb-1).w - 1;
            ub = (ub1 > ub2)? ub1: ub2;
            LOG(info, " ub1 " << ub1 << " ub2 " << ub2 << " ub " << ub);
        }
        if (ub <= lb) {
            LOG(info, " 1" << std::endl);
            sol_red_->set(j, true);
            fixed_1.push_back(item(j));
            if (reduced_capacity() < 0)
                return;
        } else {
            LOG(info, " ?" << std::endl);
            if (j != b_)
                not_fixed.push_back(item(j));
        }
    }
    for (ItemPos j = b_; j <= l_; ++j) {
        if (j == b_ && !fixed_1.empty() && fixed_1.back().j == item(b_).j)
            continue;
        LOG(info, "j " << j << " (" << item(j) << ")");

        Item ubitem = {0, capacity() - item(j).w, 0};
        ItemPos bb = ub_item(isum, ubitem);
        LOG(info, " bb " << bb);

        Profit ub = 0;
        if (bb == last_item() + 1) {
            ub = isum[last_item()+1].p + item(j).p;
            LOG(info, " ub " << ub);
        } else if (bb == last_item()) {
            Profit ub1 = isum[bb  ].p + item(j).p;
            Profit ub2 = isum[bb+1].p + item(j).p + ((capacity() - item(j).w - isum[bb+1].w) * item(bb-1).p + 1) / item(bb-1).w - 1;
            ub = (ub1 > ub2)? ub1: ub2;
            LOG(info, " ub1 " << ub1 << " ub2 " << ub2 << " ub " << ub);
        } else if (bb == 0) {
            ub = ((capacity() + item(j).w) * item(bb).p) / item(bb).w;
            LOG(info, " ub " << ub);
        } else {
            Profit ub1 = isum[bb  ].p + item(j).p + ((capacity() - item(j).w - isum[bb  ].w) * item(bb+1).p) / item(bb+1).w;
            Profit ub2 = isum[bb+1].p + item(j).p + ((capacity() - item(j).w - isum[bb+1].w) * item(bb-1).p + 1) / item(bb-1).w - 1;
            ub = (ub1 > ub2)? ub1: ub2;
            LOG(info, " ub1 " << ub1 << " ub2 " << ub2 << " ub " << ub);
        }

        if (ub <= lb) {
            LOG(info, " 0" << std::endl);
            fixed_0.push_back(item(j));
        } else {
            LOG(info, " ?" << std::endl);
            not_fixed.push_back(item(j));
        }
    }

    ItemPos j = not_fixed.size();
    ItemPos j0 = fixed_0.size();
    ItemPos j1 = fixed_1.size();
    LOG(info, "j " << j << " j0 " << j0 << " j1 " << j1 << " n " << reduced_item_number() << std::endl);

    std::copy(fixed_1.begin(), fixed_1.end(), items_.begin()+f_);
    std::copy(not_fixed.begin(), not_fixed.end(), items_.begin()+f_+j1);
    std::copy(fixed_0.begin(), fixed_0.end(), items_.begin()+f_+j1+j);
    f_ += j1;
    l_ -= j0;

    remove_big_items(info);
    compute_break_item(info);

    VER(info, "Reduction: " << lb << " - "
            << "n " << reduced_item_number() << "/" << item_number()
            << " ("  << ((double)reduced_item_number() / (double)item_number()) << ") -"
            << " c " << ((double)reduced_capacity()    / (double)capacity()) << std::endl);
    LOG(info, "n " << reduced_item_number() << "/" << item_number() << std::endl);
    LOG(info, "c " << reduced_capacity() << "/" << capacity() << std::endl);
    LOG(info, "reduced solution " << reduced_solution()->to_string_items() << std::endl);
    LOG_FOLD_END(info, "reduce2");
}

void Instance::set_first_item(ItemPos k, Info& info)
{
    LOG_FOLD_START(info, "set_first_item k " << k << std::endl);
    assert(k >= f_);
    for (ItemPos j = f_; j < k; ++j) {
        LOG(info, "set " << j << " (" << item(j) << ")" << std::endl);
        sol_red_->set(j, true);
    }
    f_ = k;
    LOG_FOLD_END(info, "set_first_item");
}

void Instance::set_last_item(ItemPos k)
{
    assert(k <= l_);
    l_ = k;
}

void Instance::fix(Info& info, const std::vector<int> vec)
{
    LOG_FOLD_START(info, "fix");
    DBG(
            for (int i: vec)
                LOG(info, " " << i);
            LOG(info, std::endl);
    )
    LOG(info, "reduced solution " << reduced_solution()->to_string_items() << std::endl);

    std::vector<Item> not_fixed;
    std::vector<Item> fixed_1;
    std::vector<Item> fixed_0;
    for (ItemPos j = f_; j <= l_; ++j) {
        if (vec[j] == 0) {
            not_fixed.push_back(item(j));
        } else if (vec[j] == 1) {
            LOG(info, "fix " << j << " (" << item(j) << ") 1" << std::endl);
            fixed_1.push_back(item(j));
            sol_red_->set(j, true);
        } else {
            assert(vec[j] == -1);
            LOG(info, "fix " << j << " (" << item(j) << ") 0" << std::endl);
            fixed_0.push_back(item(j));
        }
    }

    ItemPos j = not_fixed.size();
    ItemPos j1 = fixed_1.size();
    ItemPos j0 = fixed_0.size();
    std::copy(fixed_1.begin(), fixed_1.end(), items_.begin()+f_);
    std::copy(not_fixed.begin(), not_fixed.end(), items_.begin()+f_+j1);
    std::copy(fixed_0.begin(), fixed_0.end(), items_.begin()+f_+j1+j);

    f_ += j1;
    l_ -= j0;
    LOG(info, "reduced solution " << reduced_solution()->to_string_items() << std::endl);

    remove_big_items(info);

    if (sort_type() == 1) {
        sort_type_ = 0;
        sort_partially(info);
    } else {
        compute_break_item(info);
    }

    LOG(info, "reduced solution " << reduced_solution()->to_string_items() << std::endl);
    LOG_FOLD_END(info, "fix");
}

/******************************************************************************/

void Instance::surrogate(Info& info, Weight multiplier, ItemIdx bound)
{
    surrogate(info, multiplier, bound, first_item());
}

void Instance::surrogate(Info& info, Weight multiplier, ItemIdx bound, ItemPos first)
{
    sol_break_->clear();
    if (sol_opt_ != NULL)
        sol_opt_ = NULL;
    f_ = first;
    l_ = last_item();
    for (ItemIdx j = f_; j <= l_; ++j)
        sol_red_->set(j, false);
    bound -= sol_red_->item_number();
    for (ItemIdx j = f_; j <= l_; ++j) {
        items_[j].w += multiplier;
        if (items_[j].w <= 0) {
            sol_red_->set(j, true);
            swap(j, f_);
            f_++;
        }
    }
    c_orig_ += multiplier * bound;
    if (c_orig_ <= reduced_solution()->weight())
        c_orig_ =  reduced_solution()->weight();

    sort_type_ = 0;
    sort_partially(info);
}

/******************************************************************************/

std::ostream& knapsacksolver::operator<<(std::ostream &os, const Interval& interval)
{
    os << "[" << interval.f << "," << interval.l << "]";
    return os;
}

std::ostream& knapsacksolver::operator<<(std::ostream& os, const Item& it)
{
    os << "j " << it.j << " w " << it.w << " p " << it.p << " e " << it.efficiency();
    return os;
}

std::ostream& knapsacksolver::operator<<(std::ostream& os, const Instance& instance)
{
    os
        <<  "n_total " << instance.item_number()
        << " c_total "   << instance.capacity()
        << " opt " << instance.optimum()
        << std::endl;
    if (instance.reduced_solution() != NULL)
        os
            <<  "n " << instance.reduced_item_number() << " c " << instance.reduced_capacity()
            << " f " << instance.first_item() << " l " << instance.last_item()
            << " p_red " << instance.reduced_solution()->profit()
            << std::endl;
    if (instance.break_solution() != NULL)
        os << "b " << instance.break_item()
            << " wsum " << instance.break_weight()
            << " psum " << instance.break_profit()
            << " p_break " << instance.break_solution()->profit()
            << std::endl;
    os << "sort_type " << instance.sort_type() << std::endl;

    os << "left";
    for (Interval in: instance.int_left())
        os << " " << in;
    os << std::endl;
    os << "right";
    for (Interval in: instance.int_right())
        os << " " << in;
    os << std::endl;

    os << std::left << std::setw(8) << "pos";
    os << std::left << std::setw(8) << "j";
    os << std::left << std::setw(12) << "w";
    os << std::left << std::setw(12) << "p";
    os << std::left << std::setw(12) << "eff";
    os << std::left << std::setw(4) << "b";
    os << std::left << std::setw(4) << "opt";
    os << std::left << std::setw(4) << "red";
    os << std::left << std::setw(4) << "b/f/l";
    os << std::endl;

    for (ItemPos j = 0; j < instance.item_number(); ++j) {
        const Item& it = instance.item(j);
        os << std::left << std::setw(8) << j;
        os << std::left << std::setw(8) << it.j;
        os << std::left << std::setw(12) << it.w;
        os << std::left << std::setw(12) << it.p;
        os << std::left << std::setw(12) << it.efficiency();

        os << std::left << std::setw(4);
        if (instance.break_solution() != NULL)
             os << instance.break_solution()->contains(j);

        os << std::left << std::setw(4);
        if (instance.optimal_solution() != NULL) {
            os << instance.optimal_solution()->contains(j);
        } else {
            os << ".";
        }

        os << std::left << std::setw(4);
        if (instance.reduced_solution() != NULL) {
            os << instance.reduced_solution()->contains(j);
        } else {
            os << ".";
        }

        if (j == instance.break_item())
            os << "b";
        if (j == instance.first_item())
            os << "f";
        if (j == instance.last_item())
            os << "l";
        if (j == instance.s_prime())
            os << "s'";
        if (j == instance.t_prime())
            os << "t'";
        if (j == instance.s_second())
            os << "s''";
        if (j == instance.t_second())
            os << "t''";

        os << std::endl;
    }
    return os;
}

void Instance::plot(std::string filepath)
{
    std::ofstream file(filepath);
    if (!file.good()) {
        std::cerr << "\033[31m" << "ERROR, unable to open file \"" << filepath << "\"" << "\033[0m" << std::endl;
        assert(false);
        return;
    }

    file << "w p" << std::endl;
    for (ItemIdx i = 0; i < reduced_item_number(); ++i)
        file << item(i).w << " " << item(i).p << std::endl;
    file.close();
}

void Instance::write(std::string filepath)
{
    std::ofstream file(filepath);
    if (!file.good()) {
        std::cerr << "\033[31m" << "ERROR, unable to open file \"" << filepath << "\"" << "\033[0m" << std::endl;
        assert(false);
        return;
    }

    file << item_number() << " " << capacity() << std::endl << std::endl;
    for (ItemIdx i = 0; i < item_number(); ++i)
        file << item(i).w << " " << item(i).p << std::endl;
    file.close();
}

void Instance::plot_reduced(std::string filepath)
{
    std::ofstream file(filepath);
    if (!file.good()) {
        std::cerr << "\033[31m" << "ERROR, unable to open file \"" << filepath << "\"" << "\033[0m" << std::endl;
        assert(false);
        return;
    }

    file << "w p" << std::endl;
    for (ItemIdx i = f_; i <= l_; ++i)
        file << item(i).w << " " << item(i).p << std::endl;
    file.close();
}

void Instance::write_reduced(std::string filepath)
{
    std::ofstream file(filepath);
    if (!file.good()) {
        std::cerr << "\033[31m" << "ERROR, unable to open file \"" << filepath << "\"" << "\033[0m" << std::endl;
        assert(false);
        return;
    }

    file << reduced_item_number() << " " << reduced_capacity() << std::endl << std::endl;
    for (ItemIdx i = f_; i <= l_; ++i)
        file << item(i).w << " " << item(i).p << std::endl;
    file.close();
}

