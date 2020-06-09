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

constexpr char configPrefix[] = "/.config/sigstoped/";

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
        return false;
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

int main(int argc, char* argv[])
{
 
    std::string confDir = getConfdir();
    if(confDir.size() == 0) return 1;
    
    createPidFile(confDir+"pidfile");
    
    std::vector<std::string> applicationNames = getApplicationlist(confDir+"blacklist");
    
    if(applicationNames.size() == 0) std::cout<<"WARNIG: no application names configured.\n";
    
    if( !std::filesystem::exists("/proc") )
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
    
    std::list<Process> stoppedProcs;
    
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
                            process.resume(true);
                            stoppedProcs.remove(process);
                            std::cout<<"Resumeing wid: "+std::to_string(wid)+" pid: "+std::to_string(process.getPid())+" name: "+process.getName()<<'\n';
                        }
                        else if(prevProcess.getName() == applicationNames[i] && 
                                prevWindow != 0 && 
                                prevProcess.getName() != "" && 
                                prevProcess.getPid() > 0) 
                        {
                            sleep(5); //give the process some time to close its other windows
                            bool hasTopLevelWindow = false;
                            std::vector<Window> tlWindows = xinstance.getTopLevelWindows();
                            for(auto& window : tlWindows) 
                            {
                                if(xinstance.getPid(window) == prevProcess.getPid()) hasTopLevelWindow = true;
                            }
                            if(hasTopLevelWindow)
                            {
                                prevProcess.stop(true);
                                std::cout<<"Stoping wid: "+std::to_string(prevWindow)+" pid: "+std::to_string(prevProcess.getPid())+" name: "+prevProcess.getName()<<'\n';
                                stoppedProcs.push_back(prevProcess);
                            }
                            else  std::cout<<"not Stoping wid: "+std::to_string(prevWindow)+" pid: "+std::to_string(prevProcess.getPid())+" name: "+prevProcess.getName()<<'\n';
                        }
                    }
                }
                prevProcess = process;
                prevWindow = wid;
            }
        }
    }
    for(auto& process : stoppedProcs) process.resume(true);
    std::filesystem::remove(confDir+"pidfile");
    return 0;
}
