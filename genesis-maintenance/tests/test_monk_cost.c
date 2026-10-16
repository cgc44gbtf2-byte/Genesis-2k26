#include <assert.h>
#include "../src/monk.c"

int main(void)
{
    long heal;
    for (heal = 1600; heal <= 4800; heal++)
    {
        assert(10000 - touch_mana_cost(heal) == (long)(10000 - heal * 1.5));
        assert(touch_mana_cost(heal) * 2 >= heal * 3);
    }
    assert(touch_mana_cost(4800) == 7200);
    puts("Healing Touch exact-cost regression passed, including odd-heal rounding");
    return 0;
}
