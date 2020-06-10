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
#include <X11/Xlib.h>
#include <string>
#include <vector>

struct Atoms
{
    Atom netActiveWindow = 0;
    Atom netWmPid = 0;
    Atom wmClientMachine = 0;
};

class XInstance
{

public:
    
    static constexpr unsigned long MAX_BYTES = 1048576;
    
    inline static bool ignoreClientMachine = false;
    
    Atoms atoms;
    int screen = 0;
    Display *display = nullptr;
    
private:
    
    unsigned long readProparty(Window wid, Atom atom, unsigned char** prop, int* format);
    Atom getAtom(const std::string& atomName);
    
public:
    
    ~XInstance();
    bool open(const std::string& xDisplayName);
    Window getActiveWindow();
    pid_t getPid(Window wid);
    std::vector<Window> getTopLevelWindows();
    void flush();
};
