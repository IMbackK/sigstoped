#pragma once
#include <string>
#include <sys/types.h>

class Process
{
public:
    pid_t pid = -1;
    std::string name;
    
    bool operator==(const Process& in);
    bool operator!=(const Process& in);
    Process(){}
    Process(pid_t pidIn);
};
