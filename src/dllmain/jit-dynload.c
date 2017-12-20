/*
 * Clamav Native Windows Port: dynamic loading of llvm jit engine
 *
 * Copyright (c) 2010 Gianluigi Tiesi <sherpya@netfarm.it>
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

#include <osdeps.h>

#if defined(_MSC_VER) && defined(_DEBUG)
#define LIBCLAMAV_LLVM "libclamav_llvmd.dll"
#else
#define LIBCLAMAV_LLVM "libclamav_llvm.dll"
#endif

/* no jit functions from bytecode_nojit.c */
#define cli_bytecode_prepare_jit nojit_cli_bytecode_prepare_jit
#define cli_vm_execute_jit nojit_cli_vm_execute_jit
#define cli_bytecode_init_jit nojit_cli_bytecode_init_jit
#define cli_bytecode_done_jit nojit_cli_bytecode_done_jit
#define cli_bytecode_debug nojit_cli_bytecode_debug
#define bytecode_init nojit_bytecode_init
#define cli_bytecode_debug_printsrc nojit_cli_bytecode_debug_printsrc
#define cli_bytecode_printversion nojit_cli_bytecode_printversion
#define cli_printcxxver nojit_cli_printcxxver
#define cli_detect_env_jit nojit_cli_detect_env_jit
#include <libclamav/bytecode_nojit.c>
#undef cli_bytecode_prepare_jit
#undef cli_vm_execute_jit
#undef cli_bytecode_init_jit
#undef cli_bytecode_done_jit
#undef cli_bytecode_debug
#undef bytecode_init
#undef cli_bytecode_debug_printsrc
#undef cli_bytecode_printversion
#undef cli_printcxxver
#undef cli_detect_env_jit
/* end */

#define JITCALL __cdecl

typedef int (JITCALL *imp_cli_bytecode_prepare_jit)(struct cli_all_bc *);
typedef int (JITCALL *imp_cli_vm_execute_jit)(const struct cli_all_bc *, struct cli_bc_ctx *, const struct cli_bc_func *);
typedef int (JITCALL *imp_cli_bytecode_init_jit)(struct cli_all_bc *, unsigned);
typedef int (JITCALL *imp_cli_bytecode_done_jit)(struct cli_all_bc *, int);
typedef void (JITCALL *imp_cli_bytecode_debug)(int, char **);
typedef int (JITCALL *imp_bytecode_init)(void);
typedef void (JITCALL *imp_cli_bytecode_debug_printsrc)(const struct cli_bc_ctx *);
typedef void (JITCALL *imp_cli_bytecode_printversion)(void);
typedef void (JITCALL *imp_cli_printcxxver)();
typedef void (JITCALL *imp_cli_detect_env_jit)(struct cli_environment *);

static imp_cli_bytecode_prepare_jit pf_cli_bytecode_prepare_jit = NULL;
static imp_cli_vm_execute_jit pf_cli_vm_execute_jit = NULL;
static imp_cli_bytecode_init_jit pf_cli_bytecode_init_jit = NULL;
static imp_cli_bytecode_done_jit pf_cli_bytecode_done_jit = NULL;
static imp_cli_bytecode_debug pf_cli_bytecode_debug = NULL;
static imp_bytecode_init pf_bytecode_init = NULL;;
static imp_cli_bytecode_debug_printsrc pf_cli_bytecode_debug_printsrc = NULL;
static imp_cli_bytecode_printversion pf_cli_bytecode_printversion = NULL;
static imp_cli_printcxxver pf_cli_printcxxver = NULL;
static imp_cli_detect_env_jit pf_cli_detect_env_jit = NULL;

int cli_bytecode_prepare_jit(struct cli_all_bc *bcs)
{
    return pf_cli_bytecode_prepare_jit(bcs);
}

int cli_vm_execute_jit(const struct cli_all_bc *bcs, struct cli_bc_ctx *ctx, const struct cli_bc_func *func)
{
    return pf_cli_vm_execute_jit(bcs, ctx, func);
}

int cli_bytecode_init_jit(struct cli_all_bc *allbc, unsigned dconfmask)
{
    return pf_cli_bytecode_init_jit(allbc, dconfmask);
}

int cli_bytecode_done_jit(struct cli_all_bc *allbc, int partial)
{
    return pf_cli_bytecode_done_jit(allbc, partial);
}

void cli_bytecode_debug(int argc, char **argv)
{
    pf_cli_bytecode_debug(argc, argv);
}

int bytecode_init(void)
{
    return pf_bytecode_init();
}

void cli_bytecode_debug_printsrc(const struct cli_bc_ctx *ctx)
{
    pf_cli_bytecode_debug_printsrc(ctx);
}

void cli_bytecode_printversion(void)
{
    pf_cli_bytecode_printversion();
}

void cli_printcxxver()
{
    pf_cli_printcxxver();
}

void cli_detect_env_jit(struct cli_environment *env)
{
    pf_cli_detect_env_jit(env);
}

#define Q(string) # string
#define IMPORT_FUNC(x) \
    pf_##x = ( imp_##x ) GetProcAddress(llvm, Q(x)); if (!pf_##x) { break; }

static HMODULE llvm = NULL;

void jit_init(void)
{
    have_clamjit = 0;

    do
    {
        if (!(llvm = LoadLibraryA(LIBCLAMAV_LLVM)))
            break;

        IMPORT_FUNC(cli_bytecode_prepare_jit);
        IMPORT_FUNC(cli_vm_execute_jit);
        IMPORT_FUNC(cli_bytecode_init_jit);
        IMPORT_FUNC(cli_bytecode_done_jit);
        IMPORT_FUNC(cli_bytecode_debug);
        IMPORT_FUNC(bytecode_init);
        IMPORT_FUNC(cli_bytecode_debug_printsrc);
        IMPORT_FUNC(cli_bytecode_printversion);
        IMPORT_FUNC(cli_printcxxver);
        IMPORT_FUNC(cli_detect_env_jit);

        have_clamjit = 1;
    }
    while (0);

    if (!have_clamjit)
    {
        if (llvm) FreeLibrary(llvm);
        llvm = NULL;

        pf_cli_bytecode_prepare_jit = nojit_cli_bytecode_prepare_jit;
        pf_cli_vm_execute_jit = nojit_cli_vm_execute_jit;
        pf_cli_bytecode_init_jit = nojit_cli_bytecode_init_jit;
        pf_cli_bytecode_done_jit = nojit_cli_bytecode_done_jit;
        pf_cli_bytecode_debug = nojit_cli_bytecode_debug;
        pf_bytecode_init = nojit_bytecode_init;
        pf_cli_bytecode_debug_printsrc = nojit_cli_bytecode_debug_printsrc;
        pf_cli_bytecode_printversion = nojit_cli_bytecode_printversion;
        pf_cli_printcxxver = nojit_cli_printcxxver;
        pf_cli_detect_env_jit = nojit_cli_detect_env_jit;
    }
}

void jit_uninit(void)
{
    if (llvm) FreeLibrary(llvm);
}
