/* Exercise the private parser/accounting directly with deterministic damage. */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>
#include "../src/mage.c"

void send_to_char(const char *text, CHAR_DATA *ch)
{
    (void)text;
    (void)ch;
}
int number_range(int from, int to) { (void)from; return to; }
void hurt_person(CHAR_DATA *ch, CHAR_DATA *victim, long damage)
{
    (void)ch;
    victim->hit -= damage;
}
int format_buffer(char *dest, size_t capacity, const char *format, ...)
{
    int result;
    va_list ap;
    va_start(ap, format);
    result = vsnprintf(dest, capacity, format, ap);
    va_end(ap);
    return result;
}

int main(void)
{
    CHAR_DATA ch;
    long value;
    unsigned int i;
    char *invalid[] = {
        "-1", "1junk", "abc", "9999999999999999999999999999999999999999999"
    };
    memset(&ch, 0, sizeof(ch));
    for (i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
        assert(!mage_quintessence(&ch, invalid[i], 0, &value));
    assert(mage_quintessence(&ch, "", 0, &value) && value == 0);
    assert(mage_quintessence(&ch, "0", 0, &value) && value == 0);
    assert(!mage_quintessence(&ch, "0", 1, &value));
    assert(mage_quintessence(&ch, "2", 1, &value) && value == 2);
    ch.mana = 10000;
    ch.hit = 10000;
    ch.quint[BODY] = 10;
    assert(mage_pay_cost(&ch, 2, MANA_INVIS, MANA_QUINT));
    assert(ch.mana == 4400 && ch.quint[BODY] == 8 && ch.hit == 10000);
    assert(!mage_pay_cost(&ch, 9, MANA_INVIS, MANA_QUINT));
    assert(ch.mana == 4400 && ch.quint[BODY] == 8 && ch.hit == 10000);
    SET_BIT(ch.xtra, XTRA_HEARTS_BLOOD);
    /* Mana failure must not spend lifeforce. */
    assert(!mage_pay_cost(&ch, 9, MANA_INVIS, MANA_QUINT));
    assert(ch.mana == 4400 && ch.quint[BODY] == 8 && ch.hit == 10000);
    ch.mana = 10000;
    assert(mage_pay_cost(&ch, 10, MANA_INVIS, MANA_QUINT));
    assert(ch.mana == 2000 && ch.quint[BODY] == 0 && ch.hit == 9000);
    ch.mana = 10000;
    assert(!mage_pay_cost(&ch, 6, MANA_INVIS, MANA_QUINT));
    assert(ch.mana == 10000 && ch.hit == 9000 && ch.quint[BODY] == 0);
    ch.hit = 500;
    assert(!mage_pay_cost(&ch, 1, MANA_INVIS, MANA_QUINT));
    assert(ch.mana == 10000 && ch.hit == 500 && ch.quint[BODY] == 0);
    ch.hit = 10000;
    ch.quint[BODY] = -10; /* Old invalid state cannot create quintessence. */
    assert(mage_pay_cost(&ch, 1, MANA_INVIS, MANA_QUINT));
    assert(ch.quint[BODY] == 0 && ch.mana == 4700 && ch.hit == 9500);
    ch.mana = 10000;
    ch.quint[BODY] = 2;
    assert(mage_pay_cost(&ch, 1, MANA_RIFT, 0));
    assert(ch.mana == 9500 && ch.quint[BODY] == 1);
    puts("Mage parser/accounting passed: malformed/negative/overflow input, exact costs, atomic failures, Heart's Blood, Rift");
    return 0;
}
