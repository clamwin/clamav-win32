#include <platform.h>
#include <fmap.h>
#include <filetypes.h>
#include <others.h>
#undef main

const char *fmtfallback(int code)
{
    switch (code)
    {
        case 0x80096010:
            return "TRUST_E_BAD_DIGEST";
        case 0x80092026:
            return "CRYPT_E_SECURITY_SETTINGS";
        case 0x800b0001:
            return "TRUST_E_PROVIDER_UNKNOWN";
        case 0x800b0003:
            return "TRUST_E_SUBJECT_FORM_UNKNOWN";
        case 0x800b0004:
            return "TRUST_E_SUBJECT_NOT_TRUSTED";
        case 0x800b0100:
            return "TRUST_E_NOSIGNATURE";
        case 0x800b010e:
            return "CERT_E_REVOCATION_FAILURE";
        case 0x800b0111:
            return "TRUST_E_EXPLICIT_DISTRUST";
        default:
            return "UNKNOWN";
    }
}

void formatmessage(int code)
{
    char *message;
    if (FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, (LPSTR) &message, 0, NULL))
    {
        printf("Sigcheck result: 0x%08x - %s", code, message);
        LocalFree(message);
    }
    else
        printf("Sigcheck result: 0x%08x - %s\n", code, fmtfallback(code));
}

extern int cw_sigcheck(int fd);

int main(int argc, char *argv[])
{
    int fd, result;

    if (argc != 2)
    {
        printf("Usage: %s file_to_check\n", argv[0]);
        return 1;
    }

    cl_init(CL_INIT_DEFAULT);
    cw_disablefsredir();
    cl_debug();

    if ((fd = safe_open(argv[1], O_RDONLY|O_BINARY)) == -1)
    {
        perror("open");
        return 1;
    }

    result = cw_sigcheck(fd);
    if (result)
        printf("LE: 0x%08x ", GetLastError());
    formatmessage(result);
    
    close(fd);
    
    return 0;
}
