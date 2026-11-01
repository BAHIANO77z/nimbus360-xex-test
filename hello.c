/* Nimbus 360 - teste minimo de XEX (OpenXeChain).
 * Nao desenha nada na tela. Se o XEX abrir, ele grava um arquivo de texto
 * no pendrive e/ou no HD. Esse arquivo e a prova de que o XEX rodou. */
#include <stdint.h>
#include <stddef.h>
#include <xecore/xboxkrnl.h>

static int write_marker(const char *path, const char *text, uint32_t len)
{
    ANSI_STRING name;
    OBJECT_ATTRIBUTES oa;
    IO_STATUS_BLOCK iosb;
    HANDLE h = 0;
    NTSTATUS st;

    RtlInitAnsiString(&name, path);
    oa.root_directory = 0;
    oa.name_ptr = &name;
    oa.attributes = OBJ_CASE_INSENSITIVE;

    st = NtCreateFile(&h, GENERIC_WRITE | SYNCHRONIZE, &oa, &iosb, 0,
                      FILE_ATTRIBUTE_NORMAL, FILE_SHARE_READ,
                      FILE_SUPERSEDE, FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    if (st != 0) return 0;

    NtWriteFile(h, 0, 0, 0, &iosb, (void *)text, len, 0);
    NtClose(h);
    return 1;
}

int main(void)
{
    static const char usb_msg[] = "NIMBUS XEX OK (pendrive)\n";
    static const char hdd_msg[] = "NIMBUS XEX OK (HD)\n";

    write_marker("\\Device\\Mass0\\NIMBUS_USB.txt", usb_msg, sizeof(usb_msg) - 1);
    write_marker("\\Device\\Harddisk0\\Partition1\\NIMBUS_HDD.txt", hdd_msg, sizeof(hdd_msg) - 1);
    return 0;
}
