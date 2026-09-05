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

/* Legacy-only state; modern Windows uses this member for an OS handle. */
struct legacy_action_state {
    bool remove;
    char *destination;
};

/* Use CRT descriptors and ANSI paths: Win9x has no NT handle-relative I/O. */
void action_source_init(action_source_t *source)
{
    if (source) {
        memset(source, 0, sizeof(*source));
        source->scan_fd = -1;
    }
}

void action_source_close(action_source_t *source)
{
    if (!source) return;
    if (source->scan_fd >= 0) close(source->scan_fd);
    if (source->handle) {
        struct legacy_action_state *state = source->handle;
        if (state->remove) {
            if (traverse_unlink(source->action_path)) {
                logg(LOGG_ERROR, "Can't remove file '%s': %s\n", source->display_path, strerror(errno));
                if (state->destination) notmoved++;
                else notremoved++;
            } else if (state->destination) {
                logg(LOGG_INFO, "%s: moved to '%s'\n", source->display_path, state->destination);
            } else {
                logg(LOGG_INFO, "%s: Removed.\n", source->display_path);
            }
        }
        free(state->destination);
        free(state);
    }
    free(source->display_path);
    free(source->action_path);
    action_source_init(source);
}

cl_error_t action_source_from_fd(const char *path, int fd, action_source_t *source)
{
    if (!path || fd < 0 || !source) return CL_EARG;
    action_source_init(source);
    source->handle = calloc(1, sizeof(struct legacy_action_state));
    source->display_path = strdup(path);
    source->action_path = strdup(path);
    if (!source->handle || !source->display_path || !source->action_path) {
        action_source_close(source);
        return CL_EMEM;
    }
    source->scan_fd = dup(fd);
    if (source->scan_fd < 0 || FSTAT(source->scan_fd, &source->statbuf)) {
        action_source_close(source);
        return CL_EOPEN;
    }
    if (!S_ISREG(source->statbuf.st_mode)) {
        action_source_close(source);
        return CL_EOPEN;
    }
    source->has_stat = true;
    return CL_SUCCESS;
}

cl_error_t action_source_open_path(const char *display_path, const char *path, action_source_t *source)
{
    int fd;
    cl_error_t result;
    if (!display_path || !path || !source) return CL_EARG;
    action_source_init(source);
    fd = safe_open(path, O_RDONLY | O_BINARY);
    if (fd < 0) return CL_EOPEN;
    result = action_source_from_fd(path, fd, source);
    close(fd);
    if (result == CL_SUCCESS) {
        free(source->display_path);
        source->display_path = strdup(display_path);
        if (!source->display_path) {
            action_source_close(source);
            result = CL_EMEM;
        }
    }
    return result;
}

cl_error_t action_source_open(const char *path, action_source_t *source)
{
    return action_source_open_path(path, path, source);
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
        fd = open(*newname, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0600);
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

/* Copy from the descriptor submitted to the scanner, never reopen its name. */
static int copy_source(const action_source_t *source, int dest)
{
    char buffer[65536];
    int count;
    if (_lseeki64(source->scan_fd, 0, SEEK_SET) < 0) return -1;
    while ((count = read(source->scan_fd, buffer, sizeof(buffer))) != 0) {
        int offset = 0;
        if (count < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        while (offset < count) {
            int written = write(dest, buffer + offset, count - offset);
            if (written < 0 && errno == EINTR) continue;
            if (written <= 0) return -1;
            offset += written;
        }
    }
    return 0;
}

static void legacy_copy(const action_source_t *source, bool move)
{
    char *destination = NULL;
    int fd = getdest(source->display_path, &destination);
    int failed = fd < 0;
    if (!failed) {
        failed = copy_source(source, fd);
        if (close(fd)) failed = 1;
    }
    if (failed) {
        logg(LOGG_ERROR, "Can't %s file '%s'\n", move ? "move" : "copy", source->display_path);
        notmoved++;
        if (destination) traverse_unlink(destination);
    } else if (move) {
        struct legacy_action_state *state = source->handle;
        /* Win9x cannot unlink an open file; close the scan descriptor first. */
        state->remove = true;
        state->destination = destination;
        destination = NULL;
    } else {
        logg(LOGG_INFO, "%s: copied to '%s'\n", source->display_path, destination);
    }
    free(destination);
}

static void action_move(const action_source_t *source)
{
    legacy_copy(source, true);
}

static void action_copy(const action_source_t *source)
{
    legacy_copy(source, false);
}

static void action_remove(const action_source_t *source)
{
    struct legacy_action_state *state = source->handle;
    state->remove = true;
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
