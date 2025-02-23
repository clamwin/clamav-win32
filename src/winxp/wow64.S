#ifndef _WIN64
.global pIsWow64Process
.global pWow64DisableWow64FsRedirection
.global pWow64RevertWow64FsRedirection

.align 4
IsWow64Process:
    jmp pIsWow64Process

Wow64DisableWow64FsRedirection:
    jmp pWow64DisableWow64FsRedirection

Wow64RevertWow64FsRedirection:
    jmp pWow64RevertWow64FsRedirection
#endif
