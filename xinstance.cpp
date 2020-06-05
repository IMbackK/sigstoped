#include "xinstance.h"
#include <X11/Xutil.h>
#include <iostream>
#include <limits.h>
#include <cstring>
#include <unistd.h>
#include "debug.h"

unsigned long XInstance::readProparty(Window wid, Atom atom, unsigned char** prop, int* format)
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
    
Atom XInstance::getAtom(const std::string& atomName)
{
    return XInternAtom(display, atomName.c_str(), true);
}
    
bool XInstance::open(const std::string& xDisplayName)
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
        std::cerr<<"  is required\n";
        return false;
    }
    
    return true;
}

Window XInstance::getActiveWindow()
{
    unsigned char* data = nullptr;
    int format;
    unsigned long length = readProparty(RootWindow(display, screen), atoms.netActiveWindow, &data, &format);
    Window wid = 0;
    if(format == 32 && length == 4)  wid = *reinterpret_cast<Window*>(data);
    XFree(data);
    return wid;
}

std::vector<Window> XInstance::getTopLevelWindows()
{
    Window root_return;
    Window parent_return;
    Window* windows = nullptr;
    unsigned int nwindows;
    XQueryTree(display, RootWindow(display, screen), &root_return, &parent_return, &windows, &nwindows);
    
    std::vector<Window> out;
    out.reserve(nwindows);
    for(unsigned int i; i < nwindows; ++i)
    {
        out.push_back(windows[i]);
    }
    return out;
}

void XInstance::flush()
{
     XFlush(display);
}

pid_t XInstance::getPid(Window wid)
{
    XTextProperty xWidHostNameTextProperty;
    bool ret = XGetTextProperty(display, wid, &xWidHostNameTextProperty, atoms.wmClientMachine); 
    if (!ret) 
    {
        char errorString[1024];
        XGetErrorText(display, ret, errorString, 1024);
        debug("XGetWMClientMachine failed! " + std::string(errorString));
        return -1;
    }
    char** xWidHostNameStringList;
    int nStrings;
    ret = XTextPropertyToStringList(&xWidHostNameTextProperty, &xWidHostNameStringList, &nStrings);
    if (!ret || nStrings == 0) 
    {
        char errorString[1024];
        XGetErrorText(display, ret, errorString, 1024);
        debug("XTextPropertyToStringList failed! " + std::string(errorString));
        return -1;
    }
    char hostName[HOST_NAME_MAX+1]={0};
    if(gethostname(hostName, HOST_NAME_MAX) != 0)
    {
        debug("Can't get host name");
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
        debug("Window "+std::to_string(wid)+" is a remote window");
    }
    XFreeStringList(xWidHostNameStringList);
    return pid;
}

 XInstance::~XInstance()
 {
     if(display) XCloseDisplay(display);
 }
