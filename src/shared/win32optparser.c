/*
 * Clamav Native Windows Port: optget with hooks
 *
 * Copyright (c) 2009 Gianluigi Tiesi <sherpya@netfarm.it>
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

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#include "shared/optparser.h"

struct cw_wild
{
    const char *option;
    const char *arg;
    char *(*handler)(const char *, const char *);
};

static const struct cw_wild expansions[] =
{
    /* CONFDIR */
    { "NotifyClamd",        "ConfigDir",    cw_getpath  },
    { "config-dir",         "ConfigDir",    cw_getpath  },
    { "config-file",        "ConfigDir",    cw_getpath  },
    { "daemon-notify",      "ConfigDir",    cw_getpath  },
    { "submit-stats",       "ConfigDir",    cw_getpath  },

    /* DATADIR */
    { "DatabaseDirectory",  "DataDir",      cw_getpath  },
    { "database",           "DataDir",      cw_getpath  },
    { "datadir",            "DataDir",      cw_getpath  },
    { "list-sigs",          "DataDir",      cw_getpath  },

    { NULL,                 NULL,           NULL        }
};

const struct optstruct *win32_optget(const struct optstruct *opts, const char *name)
{
    while (opts)
    {
        if ((opts->name && !strcmp(opts->name, name)) || (opts->cmd && !strcmp(opts->cmd, name)))
        {
            size_t magiclen = strlen(MAGICOPTVALUE);
            if (opts->strarg && !strncmp(opts->strarg, MAGICOPTVALUE, magiclen))
            {
                struct optstruct *o = (struct optstruct *) opts;
                char *newval = NULL;
                char *val = opts->strarg + magiclen;
                int i;
                for (i = 0; expansions[i].option; i++)
                {
                    if (strcmp(name, expansions[i].option))
                        continue;
                    o->strarg = expansions[i].handler(expansions[i].arg, val);
                    cw_pathtowin32(o->strarg);
                    cw_rmtrailslashes(o->strarg);
                    return o;
                }
                fprintf(stderr, "Warning: Magic option value used in a wrong option '%s'\n\n", name);
                return opts;
            }
            return opts;
        }
        opts = opts->next;
    }
    return NULL;
}

#undef optget
#include <shared/optparser.c>
