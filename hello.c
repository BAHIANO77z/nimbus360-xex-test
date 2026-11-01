/* Nimbus 360 - menu pelo controle + horario. So usa chamadas ja testadas no console. */
#include <stdint.h>
#include <stddef.h>
#include <xecore/xboxkrnl.h>

extern uint32_t XamUserGetSigninState(uint32_t user_index);
extern uint32_t XamUserGetName(uint32_t user_index, char *buffer, uint32_t size);
extern uint32_t XamInputGetState(uint32_t user_index, uint32_t flags, void *state);
extern void XNotifyQueueUI(uint32_t type, uint32_t user_index, uint32_t areas,
                           uint16_t *text, void *context);

typedef struct {
    uint16_t buttons;
    uint8_t  ltrig, rtrig;
    int16_t  lx, ly, rx, ry;
} PAD;

typedef struct {
    uint32_t packet;
    PAD pad;
    uint32_t pad2;
} XSTATE;

#define BTN_UP    0x0001
#define BTN_DOWN  0x0002
#define BTN_A     0x1000
#define BTN_B     0x2000

static void sleep_ms(int ms)
{
    int64_t t = -(int64_t)ms * 10000;
    KeDelayExecutionThread(0, 0, &t);
}

/* UTF-8 (1 ou 2 bytes) -> UTF-16. */
static void to_wide(const char *s, uint16_t *out, int max)
{
    int o = 0;
    while (*s && o < max - 1) {
        uint8_t c = (uint8_t)*s++;
        if (c < 0x80) out[o++] = c;
        else if ((c & 0xE0) == 0xC0 && *s) out[o++] = (uint16_t)(((c & 0x1F) << 6) | ((uint8_t)*s++ & 0x3F));
        else out[o++] = '?';
    }
    out[o] = 0;
}

/* Balao do sistema. Usa varios buffers para nao sobrescrever um aviso ainda na fila. */
static uint16_t g_tb[8][96];
static int g_tn = 0;

static void toast(const char *s)
{
    uint16_t *b = g_tb[g_tn];
    g_tn = (g_tn + 1) & 7;
    to_wide(s, b, 96);
    XNotifyQueueUI(0, 0xFF, 2, b, 0);
}

static int cat(char *d, int n, const char *s)
{
    while (*s && n < 90) d[n++] = *s++;
    d[n] = 0;
    return n;
}

static int cat2(char *d, int n, uint32_t v)
{
    d[n++] = (char)('0' + (v / 10) % 10);
    d[n++] = (char)('0' + v % 10);
    d[n] = 0;
    return n;
}

static void show_clock(void)
{
    int64_t t = 0;
    TIME_FIELDS tf;
    char m[96];
    int n = 0;

    KeQuerySystemTime(&t);
    t -= (int64_t)3 * 3600 * 10000000LL;          /* UTC-3 (Brasilia) */
    RtlTimeToTimeFields(&t, &tf);

    n = cat(m, n, "Hora: ");
    n = cat2(m, n, tf.Hour);   n = cat(m, n, ":");
    n = cat2(m, n, tf.Minute); n = cat(m, n, ":");
    n = cat2(m, n, tf.Second); n = cat(m, n, "   ");
    n = cat2(m, n, tf.Day);    n = cat(m, n, "/");
    n = cat2(m, n, tf.Month);  n = cat(m, n, "/");
    n = cat2(m, n, tf.Year / 100);
    n = cat2(m, n, tf.Year % 100);
    toast(m);
}

static const char *g_items[4] = { "Relógio", "Perfil", "Sobre", "Sair" };

int main(void)
{
    char gt[32], m[96];
    uint32_t i, r, signed_user = 0xFF, prev = 0, cur, pressed, loops;
    int sel = 0, n, done = 0;
    XSTATE st;

    gt[0] = 0;
    for (i = 0; i < 4; i++)
        if (XamUserGetSigninState(i) != 0) { signed_user = i; break; }
    if (signed_user != 0xFF) {
        XamUserGetName(signed_user, gt, sizeof(gt));
        gt[sizeof(gt) - 1] = 0;
    }

    n = cat(m, 0, "Nimbus 360 - olá, ");
    cat(m, n, gt[0] ? gt : "jogador");
    toast(m);
    toast("Cima/baixo escolhe  -  A confirma  -  B sai");
    n = cat(m, 0, "> ");
    cat(m, n, g_items[sel]);
    toast(m);

    for (loops = 0; loops < 12000 && !done; loops++) {   /* ~10 minutos */
        cur = 0;
        for (i = 0; i < 4; i++) {
            st.packet = 0; st.pad.buttons = 0;
            r = XamInputGetState(i, 1, &st);
            if (r == 0) cur |= st.pad.buttons;
        }
        pressed = cur & ~prev;
        prev = cur;

        if (pressed & BTN_B) {
            done = 1;
        } else if (pressed & (BTN_UP | BTN_DOWN)) {
            if (pressed & BTN_UP)   sel = (sel + 3) & 3;
            if (pressed & BTN_DOWN) sel = (sel + 1) & 3;
            n = cat(m, 0, "> ");
            cat(m, n, g_items[sel]);
            toast(m);
        } else if (pressed & BTN_A) {
            if (sel == 0) {
                show_clock();
            } else if (sel == 1) {
                n = cat(m, 0, "Perfil: ");
                n = cat(m, n, gt[0] ? gt : "sem perfil");
                cat(m, n, signed_user != 0xFF ? "  (conectado)" : "");
                toast(m);
            } else if (sel == 2) {
                toast("Nimbus 360 - NOT FOUND 404");
                toast("Versão 0.2 beta - sem anúncios");
            } else {
                done = 1;
            }
        }
        sleep_ms(50);
    }
    toast("Nimbus 360 encerrado");
    sleep_ms(1500);
    return 0;
}
