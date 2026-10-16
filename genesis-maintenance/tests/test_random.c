#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <time.h>

time_t current_time = 1700000000;

void init_mm(void);
int number_range(int from, int to);

static void check_range(int from, int to)
{
    int i, value;
    for (i = 0; i < 10000; ++i)
    {
        value = number_range(from, to);
        assert(value >= from);
        assert(value <= to);
    }
}

int main(void)
{
    int i, saw_high = 0, saw_negative = 0, saw_positive = 0;
    init_mm();
    assert(number_range(7, 7) == 7);
    assert(number_range(10, -10) == 10);
    check_range(0, 1);
    check_range(-100, 100);
    check_range(INT_MIN, INT_MIN + 100);
    check_range(INT_MAX - 100, INT_MAX);
    check_range(-1924009439, 893896836);
    check_range(0, INT_MAX);
    for (i = 0; i < 10000; ++i)
    {
        if (number_range(0, INT_MAX) > (1 << 24))
            saw_high = 1;
        if (number_range(INT_MIN, INT_MAX) < 0)
            saw_negative = 1;
        if (number_range(INT_MIN, INT_MAX) >= 0)
            saw_positive = 1;
    }
    assert(saw_high && saw_negative && saw_positive);
    puts("number_range regression tests passed");
    return 0;
}
