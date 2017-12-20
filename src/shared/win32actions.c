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

#include <platform.h>
#include <osdeps.h>
#include <unistd.h>

#include "shared/optparser.h"
#include "shared/output.h"
#include "shared/misc.h"

void (*action)(const char *) = NULL;
unsigned int notmoved = 0, notremoved = 0;

static char *actarget;
static int targlen;

int getdest(const char *fullpath, char **newname)
{
    char *destdir, *name, *fp, *ext;
    char numext[4 + 1];
    int i = 0;
    size_t len = 0;
    struct stat of;

    if (!(fp = strdup(fullpath)))
    {
        *newname = NULL;
        return 0;
    }

    name = strrchr(fp, '\\');
    if (!name) name = fp;
    if (*name == '\\') name++;

    if (!(destdir = cw_normalizepath(actarget)))
    {
        logg("!Internal Error please report this path %s\n", actarget);
        free(fp);
        notmoved++;
        return 0;
    }

    len = strlen(destdir) + strlen(name) + sizeof(numext) + 10;
    CW_CHECKALLOC(*newname, malloc(len), return 0);

    ext = strrchr(name, '.');
    if (ext && !strncmp(ext, ".infected", 9))
        ext = "";
    else
        ext = ".infected";

    snprintf(*newname, len - 1, "%s\\%s%s", destdir, name, ext);
    free(destdir);
    free(fp);

    if (!_stricmp(fullpath, *newname))
    {
        logg("~%s not moved/copied since already in quarantine\n", PATH_PLAIN(fullpath));
        free(*newname);
        notmoved++;
        return 0;
    }

    len = strlen(*newname);
    for (i = 0; (stat(*newname, &of) != -1) && (i < 1000); i++)
    {
        (*newname)[len] = 0;
        snprintf(numext, len - 1, ".%03d", i);
        strcat(*newname, numext);
    }

    if (stat(*newname, &of) != -1)
    {
        logg("!Unable to find a suitable filename to move/copy %s\n", PATH_PLAIN(fullpath));
        free(*newname);
        notmoved++;
        return 0;
    }
    return 1;
}

static void action_move(const char *filename)
{
    char *nuname, *txt;
    FILE *f;

    if (!getdest(filename, &nuname))
        return;

    if (cw_movefile(filename, nuname, 0))
        logg("~%s: moved to '%s'\n", PATH_PLAIN(filename), PATH_PLAIN(nuname));
    else
        notmoved++;

    /* write a txt with the original location to being able to restore the file */
    if ((txt = malloc(strlen(nuname) + 5)))
    {
        strcpy(txt, nuname);
        strcat(txt, ".txt");

        if ((f = fopen(txt, "w")))
        {
            if (fprintf(f, "%s\t%s", filename, nuname) != (strlen(filename) + 1 + strlen(nuname)))
                logg("!Unable to write quaratine information file %s\n", strerror(errno));
            fclose(f);
        }
        else
            logg("!Unable to write quaratine information file %s\n", strerror(errno));

        free(txt);
    }
    else
        logg("!Unable to allocate memory for quaratine information file\n");

    free(nuname);
}

static void action_copy(const char *filename)
{
    char *nuname;
    if (!getdest(filename, &nuname)) return;
    if (CopyFileA(filename, nuname, FALSE))
        logg("~%s: copied to '%s'\n", PATH_PLAIN(filename), PATH_PLAIN(nuname));
    else
    {
        logg("!Cannot copy file %s (%lu)\n", PATH_PLAIN(filename), GetLastError());
        notmoved++;
    }
    free(nuname);
}

static void action_remove(const char *filename)
{
    if (cw_unlink(filename))
    {
        logg("!Can't remove file '%s' (%s).\n", PATH_PLAIN(filename), strerror(errno));
        notremoved++;
    }
    else
        logg("~%s: Removed.\n", PATH_PLAIN(filename));
}

/*
 *  Copyright (C) 2009 Sourcefire, Inc.
 *
 *  Author: aCaB
 */

int isdir() {
    struct stat sb;
    if(stat(actarget, &sb) || !S_ISDIR(sb.st_mode)) {
    logg("!'%s' doesn't exist or is not a directory\n", actarget);
    return 0;
    }
    return 1;
}

int actsetup(const struct optstruct *opts) {
    int move = optget(opts, "move")->enabled;
    if(move || optget(opts, "copy")->enabled) {
    actarget = optget(opts, move ? "move" : "copy")->strarg;
    if(!isdir()) return 1;
    action = move ? action_move : action_copy;
    targlen = strlen(actarget);
    } else if(optget(opts, "remove")->enabled)
    action = action_remove;
    return 0;
}
