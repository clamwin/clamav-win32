/*
 * Clamav Native Windows Port: File Signature Check
 *
 * Copyright (c) 2006-2025 Gianluigi Tiesi <sherpya@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; if not, write to the
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "platform.h"
#include "osdeps.h"

#include <clamav.h>

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
    wchar_t *message;
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, (LPWSTR)&message, 0, NULL))
    {
        printf("Sigcheck result: 0x%08x - %ls", code, message);
        LocalFree(message);
    }
    else
        printf("Sigcheck result: 0x%08x - %s\n", code, fmtfallback(code));
}

int main(int argc, char *argv[])
{
    int fd, result;

    if (argc != 2)
    {
        printf("Usage: %s file_to_check\n", argv[0]);
        return 1;
    }

    cl_init(CL_INIT_DEFAULT);
#ifndef _WIN64
    disablefsredir();
#endif
    cl_debug();

    if ((fd = safe_open(argv[1], O_RDONLY | O_BINARY)) == -1)
    {
        perror("open");
        return 1;
    }

    result = cw_sigcheck(fd, NULL, false);
    formatmessage(result);
    close(fd);

    return 0;
}
