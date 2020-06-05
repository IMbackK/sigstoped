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
