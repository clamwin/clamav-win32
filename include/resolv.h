/*
 * Clamav Native Windows Port: dns queries for freshclam
 *
 * Copyright (c) 2011 Gianluigi Tiesi <sherpya@netfarm.it>
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

#ifndef __RESOLV_H
#define __RESOLV_H

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#include <windns.h>

#define T_A DNS_TYPE_A
#define T_TXT DNS_TYPE_TEXT

#endif
