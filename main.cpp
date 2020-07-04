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

#define __USE_POSIX
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <X11/Xlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <fstream>
#include <string>
#include <vector>
#include <list>
#include <signal.h>
#include <sys/stat.h>
#include <cstring>
#include <filesystem>

#include "xinstance.h"
#include "process.h"
#include "split.h"
#include "debug.h"
#include "argpopt.h"
#include "CppTimer.h"

Window intraCommesWindow;
volatile bool configStale = false;
XInstance xinstance;

constexpr char configPrefix[] = "/.config/sigstoped/";
constexpr char STOP_EVENT = 65;
constexpr char PROC_STOP_EVENT = 1;

void sigTerm(int dummy) 
{
    XClientMessageEvent event;
    memset(&event, 0, sizeof(XClientMessageEvent));
    event.type = ClientMessage;
    event.window = intraCommesWindow;
    event.format = 8;
    event.data.b[0] = STOP_EVENT;
    XLockDisplay(xinstance.display);
    XSendEvent(xinstance.display, intraCommesWindow, 0, 0, (XEvent*)&event);
    XUnlockDisplay(xinstance.display);
    xinstance.flush();
}

void sigUser1(int dummy)
{
    configStale = true;
}


std::string getConfdir()
{
    const char* homeDir = getenv("HOME");
    if(homeDir == nullptr) 
    {
        std::cerr<<"HOME enviroment variable must be set.\n";
        return std::string();
    }
    const std::string configDir(std::string(homeDir)+configPrefix);
    if (std::filesystem::exists(configDir)) 
    {
        return std::string(homeDir)+configPrefix;
    }
    else if(!std::filesystem::create_directory(configDir, homeDir))
    {
        std::cout<<"Can't create "<<configDir<<'\n';
        return std::string();
    }
    else return std::string(homeDir)+configPrefix;
}

std::vector<std::string> getApplicationlist(const std::string& fileName)
{
    std::fstream blacklistFile;
    blacklistFile.open(fileName, std::fstream::in);
    std::string blacklistString;
    if(!blacklistFile.is_open())
    {
        std::cout<<fileName+" dose not exist\n";
        blacklistFile.open(fileName, std::fstream::out);
        if(blacklistFile.is_open()) blacklistFile.close();
        else  
        {
            std::cout<<"Can't create "<<fileName<<'\n';
            return std::vector<std::string>();
        }
    }
    else
    {
        blacklistString.assign((std::istreambuf_iterator<char>(blacklistFile)),
                            std::istreambuf_iterator<char>());
    }
    return split(blacklistString);
}

bool createPidFile(const std::string& fileName)
{
    if(std::filesystem::exists(fileName))
    {
        std::cerr<<fileName<<" pid file exsists, only one instance may run at once\n";
        std::string sigstopedName = Process(getpid()).getName();
        if(Process::byName(sigstopedName).size() > 1) return false;
        else
        {
            std::cerr<<"Only one "
                     <<sigstopedName
                     <<" process exists, either sigstoped died or you have several diferently named binarys\n";
                     
            std::filesystem::remove(fileName);
            return createPidFile(fileName);
        }
    }
    else
    {
        std::fstream pidFile;
        pidFile.open(fileName, std::fstream::out);
        if(!pidFile.is_open())
        {
            std::cerr<<"Can not create "<<fileName<<std::endl;
            return false;
        }
        else
        {
            pidFile<<getpid();
            pidFile.close();
            return true;
        }
    }
}


void sendEventProcStop()
{
    XClientMessageEvent event;
    memset(&event, 0, sizeof(XClientMessageEvent));
    event.type = ClientMessage;
    event.window = intraCommesWindow;
    event.format = 8;
    event.data.b[0] = PROC_STOP_EVENT;
    XLockDisplay(xinstance.display);
    XSendEvent(xinstance.display, intraCommesWindow, 0, 0, (XEvent*)&event);
    XUnlockDisplay(xinstance.display);
    xinstance.flush();
}

bool stopProcess(Process process, XInstance* xinstance)
{
    bool hasTopLevelWindow = false;
    std::vector<Window> tlWindows = xinstance->getTopLevelWindows();
    for(auto& window : tlWindows) 
    {
        if(xinstance->getPid(window) == process.getPid()) hasTopLevelWindow = true;
    }
    if(hasTopLevelWindow)
    {
        process.stop(true);
        std::cout<<"Stoping pid: "+std::to_string(process.getPid())+" name: "+process.getName()<<'\n';
        return true;
    }
    else  
    {
        std::cout<<"not Stoping pid: "+std::to_string(process.getPid())+" name: "+process.getName()<<'\n';
        return false;
    }
}

int main(int argc, char* argv[])
{
    char* xDisplayName = std::getenv( "DISPLAY" );
    if(xDisplayName == nullptr) 
    {
        std::cerr<<"DISPLAY enviroment variable must be set.\n";
        return 1;
    }

    if(!xinstance.open(xDisplayName)) exit(1);
    
    Config config;
    argp_parse(&argp, argc, argv, 0, 0, &config);

    if(config.ignoreClientMachine)
    {
        std::cout<<"WARNING: Ignoring WM_CLIENT_MACHINE is dangerous and may cause sigstoped to stop random pids if remote windows are present"<<std::endl;
        XInstance::ignoreClientMachine = true;
    }
    
    std::string confDir = getConfdir();
    if(confDir.size() == 0) return 1;
    
    if(!createPidFile(confDir+"pidfile")) return 1;
    
    std::vector<std::string> applicationNames = getApplicationlist(confDir+"blacklist");
    
    if(applicationNames.size() == 0) std::cout<<"WARNIG: no application names configured.\n";
    
    if( !std::filesystem::exists("/proc") )
    {
        std::cerr<<"proc must be mounted!\n";
        return 1;
    }
    
    std::list<Process> stoppedProcs;
    
    intraCommesWindow = XCreateSimpleWindow(xinstance.display, 
                                            RootWindow(xinstance.display, xinstance.screen),
                                            10, 10, 10, 10, 0, 0, 0);
    XSelectInput(xinstance.display, intraCommesWindow, StructureNotifyMask);
    XSelectInput(xinstance.display, RootWindow(xinstance.display, xinstance.screen), PropertyChangeMask | StructureNotifyMask);
    
    XEvent event;
    Process prevProcess;
    Process qeuedToStop;
    Window prevWindow = 0;
    
    signal(SIGINT, sigTerm);
    signal(SIGTERM, sigTerm);
    signal(SIGHUP, sigTerm);
    signal(SIGUSR1, sigUser1);
    
    CppTimer timer;
    
    while(true)
    {
        XNextEvent(xinstance.display, &event);
        if (event.type == DestroyNotify) break;
        else if (event.type == PropertyNotify && event.xproperty.atom == xinstance.atoms.netActiveWindow)
        {
            Window wid = xinstance.getActiveWindow();
            if(wid != 0 && wid != prevWindow)
            {
                pid_t windowPid = xinstance.getPid(wid);
                Process process(windowPid);
                std::cout<<"Active window: "<<wid<<" pid: "<<process.getPid()<<" name: "<<process.getName()<<'\n';
                
                if(process != prevProcess)
                {
                    if(configStale)
                    {
                        applicationNames = getApplicationlist(confDir+"blacklist");
                        configStale = false;
                        for(auto& process : stoppedProcs) process.resume(true);
                        stoppedProcs.clear();
                    }
                    for(size_t i = 0; i < applicationNames.size(); ++i)
                    {
                        if(process.getName() == applicationNames[i] &&  wid != 0 && process.getPid() > 0 && process.getName() != "") 
                        {
                            if(process == qeuedToStop)
                            {
                                std::cout<<"Canceling stop of wid: "+std::to_string(wid)+" pid: "+std::to_string(process.getPid())+" name: "+process.getName()<<'\n';
                                timer.stop();
                                qeuedToStop = Process();
                            }
                            process.resume(true);
                            stoppedProcs.remove(process);
                            std::cout<<"Resumeing wid: "+std::to_string(wid)+" pid: "+std::to_string(process.getPid())+" name: "+process.getName()<<'\n';
                        }
                        else if(prevProcess.getName() == applicationNames[i] && 
                                prevWindow != 0 && 
                                prevProcess.getName() != "" && 
                                prevProcess.getPid() > 0) 
                        {
                            timer.block();
                            std::cout<<"Will stop pid: "<<prevProcess.getPid()<<" name: "<<prevProcess.getName()<<'\n';
                            qeuedToStop = prevProcess;
                            timer.start(config.timeoutSecs, 0, sendEventProcStop, CppTimer::ONESHOT);
                            stoppedProcs.push_back(prevProcess);
                        }
                    }
                }
                prevProcess = process;
                prevWindow = wid;
            }
        }
        else if (event.type == ClientMessage && ((XClientMessageEvent*)&event)->window == intraCommesWindow)
        {
            XClientMessageEvent* clientEvent = (XClientMessageEvent*)&event;
            if (clientEvent->data.b[0] == STOP_EVENT) break;
            if (clientEvent->data.b[0] == PROC_STOP_EVENT)
            {
               stopProcess(qeuedToStop, &xinstance);
               qeuedToStop = Process();
            }
        }
    }
    for(auto& process : stoppedProcs) process.resume(true);
    std::filesystem::remove(confDir+"pidfile");
    return 0;
}
