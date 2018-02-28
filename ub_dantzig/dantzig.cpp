#include "dantzig.hpp"

#define DBG(x)
//#define DBG(x) x

Profit ub_dantzig(const Instance& instance, Info* info)
{
    DBG(std::cout << "UBDANTZIG..." << std::endl;)
    assert(instance.sort_type() == "eff" || instance.sort_type() == "peff");

    ItemPos b = instance.break_item();
    Weight  r = instance.break_capacity();
    Profit  p = instance.reduced_solution()->profit() + instance.break_profit();
    DBG(std::cout << "b " << b << " r " << r << std::endl;)
    if (b != instance.item_number() && r > 0)
        p += (instance.item(b).p * r) / instance.item(b).w;

    DBG(std::cout << "UB " << p << std::endl;)
    assert(instance.check_ub(p));
    DBG(std::cout << "UBDANTZIG... END" << std::endl;)
    return p;
}

#undef DBG

#define DBG(x)
//#define DBG(x) x

Profit ub_trivial_from(const Instance& instance, ItemIdx j, const Solution& sol_curr)
{
    return ub_trivial_from(instance, j, sol_curr.profit(), sol_curr.remaining_capacity());
}

Profit ub_trivial_from(const Instance& instance, ItemIdx j, Profit p, Weight r)
{
    Profit u = p;
    if (j < instance.item_number())
        u += (r * instance.item(j).p) / instance.item(j).w;
    assert(j > 0 || instance.check_ub(u));
    return u;
}

Profit ub_trivial_from_rev(const Instance& instance, ItemIdx j, const Solution& sol_curr)
{
    return ub_trivial_from_rev(instance, j, sol_curr.profit(), sol_curr.remaining_capacity());
}

Profit ub_trivial_from_rev(const Instance& instance, ItemIdx j, Profit p, Weight r)
{
    DBG(std::cout << "UBTRIVIALFROMREV... j " << j << std::endl;)
    Profit u = p;
    if (j >= 0)
        u += (r * instance.item(j).p + 1) / instance.item(j).w - 1;
    DBG(std::cout << "UBTRIVIALFROMREV... END" << std::endl;)
    return u;
}

#undef DBG

#define DBG(x)
//#define DBG(x) x

Profit ub_dantzig_from(const Instance& instance, ItemIdx j, const Solution& sol_curr)
{
    return ub_dantzig_from(instance, j, sol_curr.profit(), sol_curr.remaining_capacity());
}

Profit ub_dantzig_from(const Instance& instance, ItemIdx j, Profit p, Weight r)
{
    DBG(std::cout << "UBDANTZIGFROM... j " << j << " r " << sol_curr.remaining_capacity() << std::endl;)
    assert(instance.sort_type() == "eff");
    Profit  u = p;
    ItemPos b;

    //Weight r = sol_curr.remaining_capacity();
    //Item ubitem = {0, instance.isum(j).w + r, 0};
    //b  = instance.ub_item(ubitem);
    //u  = instance.isum(b).p - instance.isum(j).p;
    //r += instance.isum(j).w - instance.isum(b).w;
    //DBG(std::cout << "ubitem " << instance.isum(b) << std::endl;)
    //DBG(std::cout << "u " << u << " b " << b << " r " << r << std::endl;)

    for (b=j; b<instance.item_number(); b++) {
        if (instance.item(b).w > r)
            break;
        u += instance.item(b).p;
        r -= instance.item(b).w;
    }
    DBG(std::cout << "u " << u << " b " << b << " r " << r << std::endl;)

    if (b != instance.item_number() && r > 0)
        u += (instance.item(b).p * r) / instance.item(b).w;
    assert(j > 0 || instance.check_ub(u));
    DBG(std::cout << "UBDANTZIGFROM... END" << std::endl;)
    return u;
}

Profit ub_dantzig_from_rev(const Instance& instance, ItemIdx j, const Solution& sol_curr)
{
    return ub_dantzig_from_rev(instance, j, sol_curr.profit(), sol_curr.remaining_capacity());
}

Profit ub_dantzig_from_rev(const Instance& instance, ItemIdx j, Profit p, Weight r)
{
    assert(instance.sort_type() == "eff");
    assert(r < 0);
    ItemIdx i = j;
    for (; i>=0; i--) {
        if (instance.item(i).w + r > 0)
            break;
        p -= instance.item(i).p;
        r += instance.item(i).w;
    }
    if (i != -1 && r < 0)
        p += (instance.item(i).p * r + 1) / instance.item(i).w - 1;
    return p;
}

#undef DBG

#define DBG(x)
//#define DBG(x) x

Profit ub_dantzig_2_from(const Instance& instance, ItemIdx j, const Solution& sol_curr)
{
    DBG(std::cout << "UBDANTZIGFROM2... j " << j << " r " << sol_curr.remaining_capacity() << std::endl;)
    assert(instance.sort_type() == "eff");
    Profit  u = sol_curr.profit();
    Weight  r = sol_curr.remaining_capacity();
    ItemPos b;

    for (b=j; b<instance.item_number(); b++) {
        if (instance.item(b).w > r)
            break;
        u += instance.item(b).p;
        r -= instance.item(b).w;
    }
    DBG(std::cout << "u " << u << " b " << b << " r " << r << std::endl;)

    if (r == 0 || b == instance.item_number())
        return u;

    Profit pb = instance.item(b).p;
    Profit wb = instance.item(b).w;
    if (b > 0 && b < instance.item_number()-1) {
        DBG(std::cout << 0 << std::endl;)
        Item bm1 = instance.item(b-1);
        Item bp1 = instance.item(b+1);
        Profit ub1 = u +      ( r       * bp1.p    ) / bp1.w;
        Profit ub2 = u + pb + ((r - wb) * bm1.p + 1) / bm1.w - 1;
        u = (ub1 > ub2)? ub1: ub2;
        DBG(std::cout << "UB1 " << ub1 << " UB2 " << ub2 << " UB " << u << std::endl;)
    } else if (b > 0) {
        DBG(std::cout << 1 << std::endl;)
        Item bm1 = instance.item(b-1);
        Profit ub1 = u;
        Profit ub2 = u + pb + ((r - wb) * bm1.p + 1) / bm1.w - 1;
        u = (ub1 > ub2)? ub1: ub2;
        DBG(std::cout << "UB1 " << ub1 << " UB2 " << ub2 << " UB " << u << std::endl;)
    } else {
        DBG(std::cout << 2 << std::endl;)
        u += (r * pb) / wb;
    }
    assert(u <= ub_dantzig_from(instance, j, sol_curr));
    assert(j > 0 || instance.check_ub(u));
    DBG(std::cout << "UBDANTZIGFROM2... END" << std::endl;)
    return u;
}

#undef DBG

#define DBG(x)
//#define DBG(x) x

Profit ub_dantzig_except(const Instance& instance,
        ItemIdx n1, ItemIdx i1, ItemIdx i2, ItemIdx n2, Weight c)
{
    assert(instance.sort_type() == "eff");
    DBG(std::cout << "ub_dantzig_except " << n1 << " " << i1 << " " << i2 << " " << n2 << " " << c << std::endl;)
    ItemIdx i = n1;
    if (i == i1)
        i = i2+1;
    Profit  p = 0;
    Weight remaining_capacity = c;
    for (; i<=n2; i++) {
        DBG(std::cout << "i " << i << std::endl;)
        Weight wi = instance.item(i).w;
        if (wi > remaining_capacity)
            break;
        p += instance.item(i).p;
        remaining_capacity -= wi;
        if (i == i1-1)
            i = i2;
    }
    if (i != instance.item_number() && remaining_capacity > 0)
        p += (instance.item(i).p * remaining_capacity) / instance.item(i).w;
    DBG(std::cout << "p " << p << std::endl;)
    return p;
}

#undef DBG
