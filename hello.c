/* Nimbus 360 - TESTE 1 de video: so PERGUNTA ao console o modo de video. Nao desenha nada. */
#include <stdint.h>
#include <stddef.h>
#include <xecore/xboxkrnl.h>

extern void XNotifyQueueUI(uint32_t type, uint32_t user_index, uint32_t areas,
                           uint16_t *text, void *context);

static void sleep_ms(int ms)
{
    int64_t t = -(int64_t)ms * 10000;
    KeDelayExecutionThread(0, 0, &t);
}

static uint16_t g_tb[8][96];
static int g_tn = 0;

static void toast(const char *s)
{
    uint16_t *b = g_tb[g_tn];
    int o = 0;
    g_tn = (g_tn + 1) & 7;
    while (*s && o < 95) b[o++] = (uint8_t)*s++;
    b[o] = 0;
    XNotifyQueueUI(0, 0xFF, 2, b, 0);
}

static int cat(char *d, int n, const char *s)
{
    while (*s && n < 90) d[n++] = *s++;
    d[n] = 0;
    return n;
}

static int cat_hex(char *d, int n, uint32_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    int i;
    for (i = 28; i >= 0; i -= 4) d[n++] = hx[(v >> i) & 0xF];
    d[n] = 0;
    return n;
}

/* mostra 3 palavras de 32 bits por balao: "rotulo a b c" */
static void dump(const char *label, const uint32_t *w, int first, int count)
{
    char m[96];
    int n = 0, i;
    n = cat(m, n, label);
    for (i = 0; i < count; i++) {
        n = cat(m, n, " ");
        n = cat_hex(m, n, w[first + i]);
    }
    toast(m);
    sleep_ms(3500);
}

static uint32_t g_vm[16];      /* modo de video */
static uint32_t g_di[24];      /* informacao do display atual */

int main(void)
{
    uint32_t i;

    toast("Nimbus 360 - teste de video 1");
    sleep_ms(3000);

    for (i = 0; i < 16; i++) g_vm[i] = 0;
    toast("1: VdQueryVideoMode...");
    sleep_ms(2000);
    VdQueryVideoMode(g_vm);
    dump("modo a:", g_vm, 0, 3);
    dump("modo b:", g_vm, 3, 3);
    dump("modo c:", g_vm, 6, 3);

    for (i = 0; i < 24; i++) g_di[i] = 0;
    toast("2: VdGetCurrentDisplayInformation...");
    sleep_ms(2000);
    VdGetCurrentDisplayInformation(g_di);
    dump("disp a:", g_di, 0, 3);
    dump("disp b:", g_di, 3, 3);
    dump("disp c:", g_di, 6, 3);
    dump("disp d:", g_di, 9, 3);

    toast("Fim do teste 1");
    sleep_ms(3000);
    return 0;
}
