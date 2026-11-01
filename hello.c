/* Nimbus 360 - etapa 2 (sondagem). Grava NIMBUS_LOG.txt no Usb0. */
#include <stdint.h>
#include <stddef.h>
#include <xecore/xboxkrnl.h>

extern uint32_t XamUserGetSigninState(uint32_t user_index);
extern uint32_t XamUserGetName(uint32_t user_index, char *buffer, uint32_t size);
extern uint32_t XamInputGetState(uint32_t user_index, uint32_t flags, void *state);
extern void XNotifyQueueUI(uint32_t type, uint32_t user_index, uint64_t areas,
                           uint16_t *text, void *context);

#define LOG_PATH "\\Device\\Mass0\\NIMBUS_LOG.txt"

static char g_log[3072];
static uint32_t g_len = 0;

static void flush_log(void)
{
    ANSI_STRING name;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    HANDLE h = 0;

    RtlInitAnsiString(&name, LOG_PATH);
    oa.root_directory = 0;
    oa.name_ptr = &name;
    oa.attributes = OBJ_CASE_INSENSITIVE;

    if (NtCreateFile(&h, GENERIC_WRITE | SYNCHRONIZE, &oa, &iosb, 0,
                     FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, FILE_SUPERSEDE,
                     FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE) != 0)
        return;
    NtWriteFile(h, 0, 0, 0, &iosb, g_log, g_len, 0);
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

static void to_wide(const char *s, uint16_t *out, int max)
{
    int i = 0;
    while (s[i] && i < max - 1) { out[i] = (uint16_t)(uint8_t)s[i]; i++; }
    out[i] = 0;
}

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

int main(void)
{
    char gt[32];
    char msg[96];
    uint16_t wmsg[96];
    XSTATE st;
    uint32_t i, r, signed_user = 0xFF, last_packet = 0, presses = 0;

    log_line("NIMBUS etapa 2 - inicio");

    for (i = 0; i < 4; i++) {
        r = XamUserGetSigninState(i);
        put_str("signin "); put_hex(i); put_str(" = "); put_hex(r); put_str("\n");
        if (r != 0 && signed_user == 0xFF) signed_user = i;
    }
    flush_log();

    gt[0] = 0;
    if (signed_user != 0xFF) {
        r = XamUserGetName(signed_user, gt, sizeof(gt));
        log_val("XamUserGetName ret = ", r);
        gt[sizeof(gt) - 1] = 0;
        put_str("gamertag = "); put_str(gt); put_str("\n");
        flush_log();
    } else {
        log_line("nenhum perfil logado");
    }

    {
        const char *pre = "Nimbus 360 - ola, ";
        int n = 0, k = 0;
        while (pre[k] && n < 60) msg[n++] = pre[k++];
        k = 0;
        if (gt[0]) while (gt[k] && n < 90) msg[n++] = gt[k++];
        else { const char *a = "jogador"; while (a[k]) msg[n++] = a[k++]; }
        msg[n] = 0;
        to_wide(msg, wmsg, 96);
        log_line("chamando XNotifyQueueUI");
        XNotifyQueueUI(0, 0xFF, 2, wmsg, 0);
        log_line("XNotifyQueueUI voltou");
    }

    log_line("lendo controle por 20 s (A ou B = sair)");
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
                if (st.pad.buttons & 0x1000 || st.pad.buttons & 0x2000) {
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
    log_line("NIMBUS etapa 2 - fim");
    return 0;
}
