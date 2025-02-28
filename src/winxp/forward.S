#ifndef _WIN64
    .section .rdata,"dr"
    .global _CreateSymbolicLinkW@12
    .global _CreateWaitableTimerExW@16
    .align 4
_CreateSymbolicLinkW@12:
    .long _CreateSymbolicLinkW
_CreateWaitableTimerExW@16:
    .long _CreateWaitableTimerExW
#endif
