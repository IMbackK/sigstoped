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
#include <string>
#include <vector>
#include <sys/types.h>

class Process
{
private:
    pid_t pid_ = -1;
    pid_t ppid_ = -1;
    std::string name_;
    bool stoped_ = false;
    
private:
    std::vector<std::string> openStatus();
    static std::vector<pid_t> getAllProcessPids();
    static pid_t convertPid(const std::string& pid);
    
public:
    
    bool operator==(const Process& in);
    bool operator!=(const Process& in);
    std::string getName();
    void stop(bool children = false);
    void resume(bool children = false);
    bool getStoped();
    pid_t getPid();
    pid_t getPPid();
    Process getParent(){return Process(getPPid());}
    std::vector<Process> getChildren();
    static std::vector<Process> byName(const std::string& name);
    Process(){}
    Process(pid_t pidIn);
};
