/**
 * Sigstoped
 * Copyright (C) 2020 Carl Klemm
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#pragma once
#include<argp.h>

struct Config
{
    bool ignoreClientMachine = false;
};

const char *argp_program_version = "1.0.4";
const char *argp_program_bug_address = "<carl@uvos.xyz>";
static char doc[] = "Deamon that stops programms via SIGSTOP when their X11 windows lose focus.";
static char args_doc[] = "";

static struct argp_option options[] = 
{
  {"ignore-client-machine",  'i', 0,      0,  "Also stop programs associated with windows that fail to set WM_CLIENT_MACHINE" },
  { 0 }
};


error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    Config* config = reinterpret_cast<Config*>(state->input);
    switch (key)
    {
        case 'i':
        config->ignoreClientMachine = true;
        break;
        default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };


