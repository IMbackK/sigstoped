#define __USE_POSIX
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
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

Window intraCommesWindow;
XInstance xinstance;
volatile bool stop = false;
volatile bool configStale = false;

void sigTerm(int dummy) 
{
    stop = true;
    XClientMessageEvent dummyEvent;
    memset(&dummyEvent, 0, sizeof(XClientMessageEvent));
    dummyEvent.type = ClientMessage;
    dummyEvent.window = intraCommesWindow;
    dummyEvent.format = 32;
    XSendEvent(xinstance.display, intraCommesWindow, 0, 0, (XEvent*)&dummyEvent);
    xinstance.flush();
}

void sigUser1(int dummy)
{
    configStale = true;
}

std::vector<std::string> getBlacklist()
{
    const char* homeDir = getenv("HOME");
    if(homeDir == nullptr) 
    {
        std::cerr<<"HOME enviroment variable must be set.\n";
    }
    else
    {
        const std::string configDir(std::string(homeDir)+"/.config/sigstoped/");
        std::fstream blacklistFile;
        blacklistFile.open(configDir+"blacklist", std::fstream::in);
        std::string blacklistString;
        if(!blacklistFile.is_open())
        {
            std::cout<<configDir+"blacklist dose not exist\n";
            
            if(!std::filesystem::create_directory(configDir, homeDir))
            {
                std::cout<<"Can't create "<<configDir<<'\n';
            }
            else
            {
                blacklistFile.open(configDir+"blacklist", std::fstream::out);
                if(blacklistFile.is_open()) blacklistFile.close();
                else  std::cout<<"Can't create "<<configDir<<"blacklist \n";
            }
        }
        else
        {
            blacklistString.assign((std::istreambuf_iterator<char>(blacklistFile)),
                                std::istreambuf_iterator<char>());
        }
        return split(blacklistString);
    }
    return std::vector<std::string>();
}

int main(int argc, char* argv[])
{
    std::vector<std::string> applicationNames = getBlacklist();
    if(applicationNames.size() == 0) std::cout<<"WARNIG: no application names configured.\n";
    
    struct stat sb;
    if( stat("/proc/version", &sb) != 0)
    {
        std::cerr<<"proc must be mounted!\n";
        return 1;
    }
    
    char* xDisplayName = std::getenv( "DISPLAY" );
    if(xDisplayName == nullptr) 
    {
        std::cerr<<"DISPLAY enviroment variable must be set.\n";
        return 1;
    }
    
    std::list<pid_t> stoppedPids;
    
    if(!xinstance.open(xDisplayName)) exit(1);
    
    intraCommesWindow = XCreateSimpleWindow(xinstance.display, 
                                            RootWindow(xinstance.display, xinstance.screen),
                                            10, 10, 10, 10, 0, 0, 0);
    XSelectInput(xinstance.display, intraCommesWindow, StructureNotifyMask);
    XSelectInput(xinstance.display, RootWindow(xinstance.display, xinstance.screen), PropertyChangeMask | StructureNotifyMask);
    
    XEvent event;
    Process prevProcess;
    Window prevWindow = 0;
    
    signal(SIGINT, sigTerm);
    signal(SIGTERM, sigTerm);
    signal(SIGUSR1, sigUser1);
    
    while(!stop)
    {
        XNextEvent(xinstance.display, &event);
        if (event.type == DestroyNotify) break;
        if (event.type == PropertyNotify && event.xproperty.atom == xinstance.atoms.netActiveWindow)
        {
            Window wid = xinstance.getActiveWindow();
            if(wid != 0 && wid != prevWindow)
            {
                pid_t windowPid = xinstance.getPid(wid);
                Process process(windowPid);
                std::cout<<"Active window: "<<wid<<" pid: "<<process.pid<<" name: "<<process.name<<'\n';
                
                if(process != prevProcess)
                {
                    if(configStale)
                    {
                        applicationNames = getBlacklist();
                        configStale = false;
                        for(auto& pid : stoppedPids) 
                        {
                            kill(pid, SIGCONT);
                            debug("Resumeing pid: "+std::to_string(pid));
                        }
                        stoppedPids.clear();
                    }
                    for(size_t i = 0; i < applicationNames.size(); ++i)
                    {
                        if(process.name == applicationNames[i] && process.name != "" && wid != 0 && process.pid > 0) 
                        {
                            kill(process.pid, SIGCONT);
                            stoppedPids.remove(process.pid);
                            std::cout<<"Resumeing wid: "+std::to_string(wid)+" pid: "+std::to_string(process.pid)+" name: "+process.name<<'\n';
                        }
                        else if(prevProcess.name == applicationNames[i] && 
                                prevWindow != 0 && 
                                prevProcess.name != "" && 
                                prevProcess.pid > 0) 
                        {
                            sleep(0.5); //give the process some time to close its other windows
                            bool hasTopLevelWindow = false;
                            std::vector<Window> tlWindows = xinstance.getTopLevelWindows();
                            for(auto& window : tlWindows) 
                            {
                                if(xinstance.getPid(window) == prevProcess.pid) hasTopLevelWindow = true;
                            }
                            if(hasTopLevelWindow)
                            {
                                kill(prevProcess.pid, SIGSTOP);
                                std::cout<<"Stoping wid: "+std::to_string(prevWindow)+" pid: "+std::to_string(prevProcess.pid)+" name: "+prevProcess.name<<'\n';
                                stoppedPids.push_back(prevProcess.pid);
                            }
                            else  std::cout<<"not Stoping wid: "+std::to_string(prevWindow)+" pid: "+std::to_string(prevProcess.pid)+" name: "+prevProcess.name<<'\n';
                        }
                    }
                }
                prevProcess = process;
                prevWindow = wid;
            }
        }
    }
    for(auto& pid : stoppedPids) 
    {
        kill(pid, SIGCONT);
        std::cout<<"Resumeing pid: "+std::to_string(pid)<<'\n';
    }
    return 0;
}
