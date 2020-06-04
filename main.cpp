#define __USE_POSIX
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <X11/Xutil.h>
#include <X11/Xlib.h>
#include <X11/extensions/dpms.h>
#include <unistd.h>
#include <cstring>
#include <sys/types.h>
#include <limits.h>
#include <sys/stat.h>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <signal.h>

struct Atoms
{
    Atom netActiveWindow = 0;
    Atom netWmPid = 0;
    Atom wmClientMachine = 0;
};

class XInstance
{

public:
    
    Atoms atoms;
    
    static constexpr unsigned long MAX_BYTES = 1048576;
    
    int screen = 0;
    Display *display = nullptr;
    
private:
    unsigned long readProparty(Window wid, Atom atom, unsigned char** prop, int* format)
    {
        Atom returnedAtom;
        unsigned long nitems;
        unsigned long bytes_after;
            
        int ret = XGetWindowProperty(
            display, 
            wid, 
            atom,
            0, XInstance::MAX_BYTES/4, false,
            AnyPropertyType, 
            &returnedAtom, 
            format, 
            &nitems, 
            &bytes_after,
            prop);
        if (ret != Success) 
        {
            std::cerr<<"XGetWindowProperty failed!\n";
            return 0;
        }
        else return std::min((*format)/8*nitems, XInstance::MAX_BYTES);
    }
    
    Atom getAtom(const std::string& atomName)
    {
        return XInternAtom(display, atomName.c_str(), true);
    }
    
public:
    bool open(const std::string& xDisplayName)
    {
        display = XOpenDisplay(xDisplayName.c_str());
        if (display == nullptr) 
        {
            std::cerr<<"Can not open display "<<xDisplayName<<'\n';
            return false;
        }
        screen = XDefaultScreen(display);
        
        atoms.netActiveWindow = getAtom("_NET_ACTIVE_WINDOW");
        if(atoms.netActiveWindow == 0)
        {
            std::cerr<<"_NET_ACTIVE_WINDOW is required\n";
            return false;
        }
        
        atoms.netWmPid = getAtom("_NET_WM_PID");
        if(atoms.netActiveWindow == 0)
        {
            std::cerr<<"_NET_WM_PID is required\n";
            return false;
        }
        
        atoms.wmClientMachine = getAtom("WM_CLIENT_MACHINE");
        if(atoms.netActiveWindow == 0)
        {
            std::cerr<<"WM_CLIENT_MACHINE is required\n";
            return false;
        }
        
        return true;
    }
    Window getActiveWindow()
    {
        unsigned char* data = nullptr;
        int format;
        unsigned long length = readProparty(RootWindow(display, screen), atoms.netActiveWindow, &data, &format);
        Window wid = 0;
        if(format == 32 && length == 4)  wid = *reinterpret_cast<Window*>(data);
        XFree(data);
        return wid;
    }
    pid_t getPid(Window wid)
    {
        XTextProperty xWidHostNameTextProperty;
        bool ret = XGetTextProperty(display, wid, &xWidHostNameTextProperty, atoms.wmClientMachine); 
        if (!ret) 
        {
            char errorString[1024];
            XGetErrorText(display, ret, errorString, 1024);
            
            std::cerr<<"XGetWMClientMachine failed! "<<errorString<<std::endl;
            return -1;
        }
        char** xWidHostNameStringList;
        int nStrings;
        ret = XTextPropertyToStringList(&xWidHostNameTextProperty, &xWidHostNameStringList, &nStrings);
        if (!ret || nStrings == 0) 
        {
            std::cerr<<"XTextPropertyToStringList failed!\n";
            return -1;
        }
        char hostName[HOST_NAME_MAX+1]={0};
        if(gethostname(hostName, HOST_NAME_MAX) != 0)
        {
            std::cerr<<"Can't get host name\n";
            return -1;
        }
        pid_t pid = -1;
        if(strcmp(hostName, xWidHostNameStringList[0]) == 0)
        {
            unsigned char* data = nullptr;
            int format;
            unsigned long length = readProparty(wid, atoms.netWmPid, &data, &format);
            if(format == 32 && length == 4)  pid = *reinterpret_cast<pid_t*>(data);
            XFree(data);

        }
        else
        {
            std::cout<<"Window "<<wid<<" is a remote window\n";
        }
        XFreeStringList(xWidHostNameStringList);
        return pid;
    }
};

std::vector<std::string> split(const std::string& in, char delim = '\n' )
{
   std::stringstream ss(in); 
   std::vector<std::string> tokens;
   std::string temp_str;
   while(std::getline(ss, temp_str, delim))
   {
      tokens.push_back(temp_str);
   }
   return tokens;
}

class Process
{
public:
    pid_t pid = -1;
    std::string name;
    
    bool operator==(const Process& in)
    {
        return pid == in.pid;
    }
    Process(){}
    Process(pid_t pidIn)
    {
        std::fstream statusFile;
        std::string statusString;
        statusFile.open(std::string("/proc/") + std::to_string(pidIn)+ "/status", std::fstream::in);
        if(statusFile.is_open())
        {
            std::string statusString((std::istreambuf_iterator<char>(statusFile)),
            std::istreambuf_iterator<char>());
            std::vector<std::string> lines = split(statusString);
            if(lines.size() > 0)
            {
                pid = pidIn;
                std::vector<std::string> tokens = split(lines[0], '\t');
                if(tokens.size() > 1)
                {
                    name = tokens[1];
                }
            }
            statusFile.close();
            
        }
        else
        {
            std::cout<<"cant open "<<"/proc/" + std::to_string(pidIn)+ "/status"<<std::endl;
        }
    }
};

int main(int argc, char* argv[])
{
    XInstance xinstance;
    
    const char* homeDir = getenv("HOME");
    if(homeDir == nullptr) 
    {
        std::cerr<<"HOME enviroment variable must be set.\n";
        return 1;
    }
    
    std::fstream blacklistFile;
    blacklistFile.open(std::string(homeDir)+"/.config/sigstoped/blacklist", std::fstream::in);
    std::string blacklistString;
    if(!blacklistFile.is_open())
    {
        std::cout<<std::string(homeDir)+"/.config/sigstoped/blacklist must exsist\n";
    }
    else
    {
        blacklistString.assign((std::istreambuf_iterator<char>(blacklistFile)),
                              std::istreambuf_iterator<char>());
    }
    
    std::vector<std::string> applicationNames = split(blacklistString);
    
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
    
    if(!xinstance.open(xDisplayName)) exit(1);
    
	XSelectInput(xinstance.display, RootWindow(xinstance.display, xinstance.screen), PropertyChangeMask | StructureNotifyMask);
    
    XEvent event;
    Process prevProcess;
    Window prevWindow = 0;
    while(true)
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
                
                for(size_t i = 0; i < applicationNames.size(); ++i)
                {
                    if(process.name == applicationNames[i]) 
                    {
                        kill(process.pid, SIGCONT);
                        std::cout<<"Resumeing: "<<wid<<" pid: "<<windowPid<<" name: "<<Process(windowPid).name<<'\n';
                    }
                    else if(prevProcess.name == applicationNames[i] && prevWindow != 0) kill(prevProcess.pid, SIGSTOP);
                    {
                        kill(prevProcess.pid, SIGSTOP);
                        std::cout<<"Stoping: "<<wid<<" pid: "<<windowPid<<" name: "<<Process(windowPid).name<<'\n';
                    }
                }
                prevProcess = process;
                prevWindow = wid;
                std::cout<<"Active window: "<<wid<<" pid: "<<windowPid<<" name: "<<Process(windowPid).name<<'\n';
            }
        }
    }
    return 0;
}
