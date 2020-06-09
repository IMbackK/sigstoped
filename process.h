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
    std::vector<pid_t> getAllProcessPids();
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
    Process(){}
    Process(pid_t pidIn);
};
