/*
 * Clamav Native Windows Port: actions replacement for win32
 *
 * Copyright (c) 2009-2011 Gianluigi Tiesi <sherpya@netfarm.it>
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

#include <windows.h>

#if _WIN32_WINNT >= 0x0600
#include "common/actions.c"
#else

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#include <libgen.h>

// libclamav
#include "clamav.h"
#include "others.h"
#include "optparser.h"
#include "misc.h"
#include "output.h"

#include "osdeps.h"
#define traverse_unlink cw_unlink

void (*action)(const char *) = NULL;
unsigned int notmoved = 0, notremoved = 0;

static char *actarget;
static int targlen;

static int getdest(const char *fullpath, char **newname)
{
    char *tmps, *filename;
    int fd, i;

    tmps = strdup(fullpath);
    if (!tmps) {
        *newname = NULL;
        return -1;
    }
    filename = basename(tmps);

    if (!(*newname = (char *)malloc(targlen + strlen(filename) + 6))) {
        free(tmps);
        return -1;
    }
    sprintf(*newname, "%s" PATHSEP "%s", actarget, filename);
    for (i = 1; i < 1000; i++) {
        fd = open(*newname, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) {
            free(tmps);
            return fd;
        }
        if (errno != EEXIST) break;
        sprintf(*newname, "%s" PATHSEP "%s.%03u", actarget, filename, i);
    }
    free(tmps);
    free(*newname);
    *newname = NULL;
    return -1;
}

static void action_move(const char *filename)
{
    char *nuname        = NULL;
    char *real_filename = NULL;
    int fd              = -1;
    int copied          = 0;

    if (NULL == filename) {
        goto done;
    }

    fd = getdest(filename, &nuname);

    if (fd < 0 || (((copied = 1)) && filecopy(filename, nuname))) {
        logg(LOGG_ERROR, "Can't move file %s to %s\n", filename, nuname);
        notmoved++;
        if (nuname) traverse_unlink(nuname);
    } else {
        if (copied && (0 != traverse_unlink(filename)))
            logg(LOGG_ERROR, "Can't unlink '%s' after copy: %s\n", filename, strerror(errno));
        else
            logg(LOGG_INFO, "%s: moved to '%s'\n", filename, nuname);
    }

done:
    if (NULL != real_filename) free(real_filename);
    if (fd >= 0) close(fd);
    if (NULL != nuname) free(nuname);
    return;
}

static void action_copy(const char *filename)
{
    char *nuname;
    int fd = getdest(filename, &nuname);

    if (fd < 0 || filecopy(filename, nuname)) {
        logg(LOGG_ERROR, "Can't copy file '%s'\n", filename);
        notmoved++;
        if (nuname) traverse_unlink(nuname);
    } else
        logg(LOGG_INFO, "%s: copied to '%s'\n", filename, nuname);

    if (fd >= 0) close(fd);
    if (nuname) free(nuname);
}

static void action_remove(const char *filename)
{
    char *real_filename = NULL;

    if (NULL == filename) {
        goto done;
    }

    if (0 != traverse_unlink(filename)) {
        logg(LOGG_ERROR, "Can't remove file '%s'\n", filename);
        notremoved++;
    } else {
        logg(LOGG_INFO, "%s: Removed.\n", filename);
    }

done:
    if (NULL != real_filename) free(real_filename);
    return;
}

static int isdir(void)
{
    STATBUF sb;
    if (CLAMSTAT(actarget, &sb) || !S_ISDIR(sb.st_mode)) {
        logg(LOGG_ERROR, "'%s' doesn't exist or is not a directory\n", actarget);
        return 0;
    }
    return 1;
}

/*
 * Call this function at the beginning to configure the user preference.
 * Later, call the "action" callback function to perform the selection action.
 */
int actsetup(const struct optstruct *opts)
{
    int move = optget(opts, "move")->enabled;
    if (move || optget(opts, "copy")->enabled) {
#ifndef _WIN32
        cl_error_t ret;
#endif
        actarget = optget(opts, move ? "move" : "copy")->strarg;
#ifndef _WIN32
        ret = cli_realpath((const char *)actarget, &actarget);
        if (CL_SUCCESS != ret || NULL == actarget) {
            logg(LOGG_INFO, "action_setup: Failed to get realpath of %s\n", actarget);
            return 0;
        }
#endif
        if (!isdir()) return 1;
        action  = move ? action_move : action_copy;
        targlen = strlen(actarget);
    } else if (optget(opts, "remove")->enabled)
        action = action_remove;
    return 0;
}
#endif /* _WIN32_WINNT >= 0x0600 */
