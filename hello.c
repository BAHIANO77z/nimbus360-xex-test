/* Nimbus 360 - teste de gravacao de arquivos.
 * Sem funcoes do XAM. Tenta 4 jeitos de gravar e escreve o resultado
 * no NOME de arquivos vazios, na raiz do pendrive (Mass1):
 *   NB_inicio.txt
 *   NB_V<n>_cri_<codigo>.txt   (codigo do NtCreateFile; 00000000 = ok)
 *   NB_V<n>_gra_<codigo>.txt   (codigo do NtWriteFile;  00000000 = ok)
 *   NB_fim.txt
 * Se o app cair, o ultimo arquivo criado mostra onde parou. */
#include <stdint.h>
#include <stddef.h>
#include <xecore/xboxkrnl.h>

#define DIR_OUT "\\Device\\Mass1\\"

static void hex8(uint32_t v, char *out)
{
    static const char hx[] = "0123456789ABCDEF";
    int i;
    for (i = 0; i < 8; i++) out[i] = hx[(v >> (28 - 4 * i)) & 0xF];
    out[8] = 0;
}

static void cat(char *dst, const char *src)
{
    while (*dst) dst++;
    while ((*dst++ = *src++)) {}
}

static uint32_t open_file(const char *path, uint32_t access, uint32_t disp, HANDLE *out)
{
    ANSI_STRING name;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;

    RtlInitAnsiString(&name, path);
    oa.root_directory = 0;
    oa.name_ptr = &name;
    oa.attributes = OBJ_CASE_INSENSITIVE;
    *out = 0;
    return (uint32_t)NtCreateFile(out, access, &oa, &iosb, 0,
                                  FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ, disp,
                                  FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
}

static void marker(const char *label, int has_code, uint32_t code)
{
    char path[96];
    char h[9];
    HANDLE f;

    path[0] = 0;
    cat(path, DIR_OUT "NB_");
    cat(path, label);
    if (has_code) {
        cat(path, "_");
        hex8(code, h);
        cat(path, h);
    }
    cat(path, ".txt");
    if (open_file(path, GENERIC_WRITE | SYNCHRONIZE, FILE_OVERWRITE_IF, &f) == 0)
        NtClose(f);
}

static void try_variant(int n, const char *path, uint32_t access, uint32_t disp, int use_offset)
{
    static const char text[] = "NIMBUS OK\n";
    HANDLE f;
    IO_STATUS_BLOCK iosb;
    int64_t off = 0;
    uint32_t st;
    char tag_c[8], tag_w[8];

    tag_c[0] = 'V'; tag_c[1] = (char)('0' + n); tag_c[2] = '_';
    tag_c[3] = 'c'; tag_c[4] = 'r'; tag_c[5] = 'i'; tag_c[6] = 0;
    tag_w[0] = 'V'; tag_w[1] = (char)('0' + n); tag_w[2] = '_';
    tag_w[3] = 'g'; tag_w[4] = 'r'; tag_w[5] = 'a'; tag_w[6] = 0;

    st = open_file(path, access, disp, &f);
    marker(tag_c, 1, st);
    if (st != 0) return;

    st = (uint32_t)NtWriteFile(f, 0, 0, 0, &iosb, (void *)text, sizeof(text) - 1,
                               use_offset ? &off : 0);
    marker(tag_w, 1, st);
    NtClose(f);
}

int main(void)
{
    marker("inicio", 0, 0);

    try_variant(1, DIR_OUT "NB_dados1.txt", GENERIC_WRITE | SYNCHRONIZE, FILE_OVERWRITE_IF, 1);
    try_variant(2, DIR_OUT "NB_dados2.txt", 0x00100116, FILE_OVERWRITE_IF, 1);
    try_variant(3, DIR_OUT "NB_dados3.txt", GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, FILE_OPEN_IF, 1);
    try_variant(4, DIR_OUT "NB_dados4.txt", GENERIC_WRITE | SYNCHRONIZE, FILE_SUPERSEDE, 0);

    marker("fim", 0, 0);
    return 0;
}
