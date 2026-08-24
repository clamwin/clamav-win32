/*
 * Clamav Native Windows Port: actions replacement for win32
 *
 * Copyright (c) 2009-2025 Gianluigi Tiesi <sherpya@gmail.com>
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

#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
#include "actions.c"
#else

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#include <libgen.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdbool.h>

// libclamav
#include "clamav.h"
#include "others.h"
#include "optparser.h"
#include "misc.h"
#include "output.h"
#include "actions.h"

#define traverse_unlink cli_unlink

void (*action)(const action_source_t *) = NULL;
unsigned int notmoved = 0, notremoved = 0;

static char *actarget;
static int targlen;

void action_source_init(action_source_t *source)
{
    if (NULL == source)
        return;

    memset(source, 0, sizeof(*source));
    source->scan_fd = -1;
#ifdef _WIN32
    source->handle = INVALID_HANDLE_VALUE;
#endif
}

void action_source_close(action_source_t *source);

static cl_error_t action_source_set_paths(action_source_t *source, const char *display_path, const char *action_path)
{
    source->display_path = strdup(display_path);
    if (NULL == source->display_path)
        return CL_EMEM;

    if (NULL != action_path) {
        source->action_path = strdup(action_path);
        if (NULL == source->action_path)
            return CL_EMEM;
    }

    return CL_SUCCESS;
}

static cl_error_t action_source_attach_fd(action_source_t *source, int fd)
{
    HANDLE handle     = INVALID_HANDLE_VALUE;
    HANDLE dup_handle = INVALID_HANDLE_VALUE;

    if (0 != FSTAT(fd, &source->statbuf))
        return CL_EOPEN;
    source->has_stat = true;

    if (!S_ISREG(source->statbuf.st_mode))
        return CL_EOPEN;

    handle = (HANDLE)_get_osfhandle(fd);
    if (INVALID_HANDLE_VALUE == handle)
        return CL_EOPEN;

    if (!DuplicateHandle(GetCurrentProcess(), handle, GetCurrentProcess(), &dup_handle, 0, FALSE, DUPLICATE_SAME_ACCESS))
        return CL_EOPEN;

    source->scan_fd           = fd;
    source->handle            = dup_handle;
    source->handle_can_delete = false;
    return CL_SUCCESS;
}

cl_error_t action_source_open_path(const char *display_path, const char *open_path, action_source_t *source)
{
    int fd            = -1;
    cl_error_t status = CL_EARG;

    if ((NULL == display_path) || (NULL == open_path) || (NULL == source))
        return CL_EARG;

    action_source_init(source);

    status = action_source_set_paths(source, display_path, open_path);
    if (CL_SUCCESS != status)
        goto done;

    fd = safe_open(open_path, O_RDONLY | O_BINARY);
    if (fd < 0) {
        status = CL_EOPEN;
        goto done;
    }

    status = action_source_attach_fd(source, fd);
    if (CL_SUCCESS != status)
        goto done;
    fd = -1;

done:
    if (fd >= 0)
        close(fd);
    if (CL_SUCCESS != status)
        action_source_close(source);
    return status;
}

cl_error_t action_source_open(const char *display_path, action_source_t *source)
{
    return action_source_open_path(display_path, display_path, source);
}

cl_error_t action_source_from_fd(const char *display_path, int fd, action_source_t *source)
{
    int dup_fd        = -1;
    cl_error_t status = CL_EARG;

    if ((NULL == display_path) || (fd < 0) || (NULL == source))
        return CL_EARG;

    action_source_init(source);

    status = action_source_set_paths(source, display_path, display_path);
    if (CL_SUCCESS != status)
        goto done;

    dup_fd = _dup(fd);
    if (dup_fd < 0) {
        status = CL_EOPEN;
        goto done;
    }

    status = action_source_attach_fd(source, dup_fd);
    if (CL_SUCCESS != status)
        goto done;
    dup_fd = -1;

done:
    if (dup_fd >= 0)
        close(dup_fd);
    if (CL_SUCCESS != status)
        action_source_close(source);
    return status;
}

void action_source_close(action_source_t *source)
{
    if (NULL == source)
        return;

    if (-1 != source->scan_fd)
        close(source->scan_fd);
#ifdef _WIN32
    if ((NULL != source->handle) && (INVALID_HANDLE_VALUE != source->handle))
        CloseHandle((HANDLE)source->handle);
#endif
    if (NULL != source->display_path)
        free(source->display_path);
    if (NULL != source->action_path)
        free(source->action_path);

    action_source_init(source);
}

static const char *action_source_filename(const action_source_t *source)
{
    if (NULL == source)
        return NULL;
    if ((NULL != source->action_path) && ('\0' != source->action_path[0]))
        return source->action_path;
    return source->display_path;
}

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

static void action_move(const action_source_t *source)
{
    const char *filename = action_source_filename(source);
    char *nuname         = NULL;
    char *real_filename  = NULL;
    int fd               = -1;
    int copied           = 0;

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

static void action_copy(const action_source_t *source)
{
    const char *filename = action_source_filename(source);
    char *nuname         = NULL;
    int fd               = -1;

    if (NULL == filename) {
        notmoved++;
        return;
    }

    fd = getdest(filename, &nuname);

    if (fd < 0 || filecopy(filename, nuname)) {
        logg(LOGG_ERROR, "Can't copy file '%s'\n", filename);
        notmoved++;
        if (nuname) traverse_unlink(nuname);
    } else
        logg(LOGG_INFO, "%s: copied to '%s'\n", filename, nuname);

    if (fd >= 0) close(fd);
    if (nuname) free(nuname);
}

static void action_remove(const action_source_t *source)
{
    const char *filename = action_source_filename(source);
    char *real_filename  = NULL;

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
        actarget = optget(opts, move ? "move" : "copy")->strarg;
        if (!isdir()) return 1;
        action  = move ? action_move : action_copy;
        targlen = strlen(actarget);
    } else if (optget(opts, "remove")->enabled)
        action = action_remove;
    return 0;
}

#endif /* _WIN32_WINNT >= 0x0600 */
