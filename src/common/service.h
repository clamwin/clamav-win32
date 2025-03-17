/*
 *  Copyright (C) 2021-2024 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 *  Copyright (C) 2008-2025 Gianluigi Tiesi <sherpya@netfarm.it>
 *
 *  Authors: Gianluigi Tiesi
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#ifndef __SERVICE_H
#define __SERVICE_H

#include <windows.h>
#include <winsvc.h>
#include <stdbool.h>
#include <tchar.h>

bool svc_uninstall(const TCHAR *name, bool verbose);
bool svc_install(const TCHAR *name, const TCHAR *dname, TCHAR *desc);
static void svc_getcpvalue(const TCHAR *name);
void svc_register(TCHAR *name);
void svc_ready(void);
int svc_checkpoint(const char *type, const char *name, unsigned int custom, void *context);
void WINAPI ServiceCtrlHandler(DWORD code);
BOOL WINAPI cw_stop_ctrl_handler(DWORD CtrlType);
void WINAPI ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv);
#endif
