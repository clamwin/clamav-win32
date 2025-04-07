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

#ifndef _SERVICE_H_
#define _SERVICE_H_

#include <windows.h>
#include <stdbool.h>
#include <tchar.h>

bool svc_uninstall(const TCHAR *name, bool verbose);
bool svc_install(const TCHAR *name, const TCHAR *dname, TCHAR *desc);
void svc_register(TCHAR *name);
void svc_ready(void);
int svc_checkpoint(const char *type, const char *name, unsigned int custom, void *context);

#define svc_install(name, dname, desc) svc_install(TEXT(name), TEXT(dname), TEXT(desc))
#define svc_uninstall(name, verbose) svc_uninstall(TEXT(name), verbose)
#define svc_register(name) svc_register(TEXT(name))

#endif // _SERVICE_H_
