/* Nimbus 360 - sondagem por etapas. Grava NIMBUS_LOG_<etapa>.txt no pendrive. */
#include <stdint.h>
#include <stddef.h>
#include <xecore/xboxkrnl.h>

#ifndef STAGE
#define STAGE 4
#endif

#define STR2(x) #x
#define STR(x) STR2(x)

extern uint32_t XamUserGetSigninState(uint32_t user_index);
extern uint32_t XamUserGetName(uint32_t user_index, char *buffer, uint32_t size);
extern uint32_t XamInputGetState(uint32_t user_index, uint32_t flags, void *state);
extern void XNotifyQueueUI(uint32_t type, uint32_t user_index, uint32_t areas,
                           uint16_t *text, void *context);

static char g_log[3072];
static uint32_t g_len = 0;

static void flush_log(void)
{
    ANSI_STRING name;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    HANDLE h = 0;
    int64_t off = 0;

    RtlInitAnsiString(&name, "\\Device\\Mass1\\NIMBUS_LOG_" STR(STAGE) ".txt");
    oa.root_directory = 0;
    oa.name_ptr = &name;
    oa.attributes = OBJ_CASE_INSENSITIVE;

    if (NtCreateFile(&h, GENERIC_WRITE | SYNCHRONIZE, &oa, &iosb, 0,
                     FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_OVERWRITE_IF,
                     FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE) != 0)
        return;
    NtWriteFile(h, 0, 0, 0, &iosb, g_log, g_len, &off);
    NtClose(h);
}

static void put_str(const char *s)
{
    while (*s && g_len < sizeof(g_log) - 1) g_log[g_len++] = *s++;
}

static void put_hex(uint32_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    int i;
    put_str("0x");
    for (i = 28; i >= 0; i -= 4)
        if (g_len < sizeof(g_log) - 1) g_log[g_len++] = hx[(v >> i) & 0xF];
}

static void log_val(const char *label, uint32_t v)
{
    put_str(label);
    put_hex(v);
    put_str("\n");
    flush_log();
}

static void log_line(const char *s)
{
    put_str(s);
    put_str("\n");
    flush_log();
}

static void sleep_ms(int ms)
{
    int64_t t = -(int64_t)ms * 10000;
    KeDelayExecutionThread(0, 0, &t);
}

#if STAGE >= 4
static void to_wide(const char *s, uint16_t *out, int max)
{
    int i = 0;
    while (s[i] && i < max - 1) { out[i] = (uint16_t)(uint8_t)s[i]; i++; }
    out[i] = 0;
}
#endif

#if STAGE >= 3
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
#endif

int main(void)
{
    char gt[32];
    uint32_t i, r, signed_user = 0xFF;
#if STAGE >= 3
    XSTATE st;
    uint32_t last_packet = 0, presses = 0;
#endif
#if STAGE >= 4
    char msg[96];
    uint16_t wmsg[96];
#endif

    gt[0] = 0;
    log_line("NIMBUS etapa " STR(STAGE) " - inicio");

#if STAGE >= 1
    for (i = 0; i < 4; i++) {
        log_val("signin: chamando usuario ", i);
        r = XamUserGetSigninState(i);
        log_val("signin: retorno = ", r);
        if (r != 0 && signed_user == 0xFF) signed_user = i;
    }
#endif

#if STAGE >= 2
    if (signed_user != 0xFF) {
        log_line("gamertag: chamando");
        r = XamUserGetName(signed_user, gt, sizeof(gt));
        log_val("gamertag: retorno = ", r);
        gt[sizeof(gt) - 1] = 0;
        put_str("gamertag = "); put_str(gt); put_str("\n");
        flush_log();
    } else {
        log_line("nenhum perfil logado");
    }
#endif

#if STAGE >= 4
    {
        const char *pre = "Nimbus 360 - ola, ";
        int n = 0, k = 0;
        while (pre[k] && n < 60) msg[n++] = pre[k++];
        k = 0;
        if (gt[0]) while (gt[k] && n < 90) msg[n++] = gt[k++];
        else { const char *a = "jogador"; while (a[k]) msg[n++] = a[k++]; }
        msg[n] = 0;
        to_wide(msg, wmsg, 96);
        log_line("aviso: chamando XNotifyQueueUI");
        XNotifyQueueUI(0, 0xFF, 2, wmsg, 0);
        log_line("aviso: voltou");
    }
#endif

#if STAGE >= 3
    log_line("controle: lendo por 20 s (A ou B = sair)");
    for (i = 0; i < 200; i++) {
        uint32_t u;
        for (u = 0; u < 4; u++) {
            st.packet = 0; st.pad.buttons = 0;
            r = XamInputGetState(u, 1, &st);
            if (r == 0 && st.packet != last_packet) {
                last_packet = st.packet;
                if (st.pad.buttons) {
                    presses++;
                    put_str("botoes = "); put_hex(st.pad.buttons); put_str("\n");
                    flush_log();
                }
                if ((st.pad.buttons & 0x1000) || (st.pad.buttons & 0x2000)) {
                    log_line("A/B apertado, saindo");
                    goto fim;
                }
            }
        }
        sleep_ms(100);
    }
    log_line("tempo esgotado");
fim:
    log_val("apertos de botao = ", presses);
#else
    (void)i; (void)r; (void)signed_user;
    sleep_ms(3000);
#endif
    log_line("NIMBUS etapa " STR(STAGE) " - fim");
    return 0;
}
