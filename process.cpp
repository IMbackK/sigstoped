#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "process.h"
#include "split.h"

bool Process::operator==(const Process& in)
{
    return pid == in.pid;
}
    
bool Process::operator!=(const Process& in)
{
    return pid != in.pid;
}
    
Process::Process(pid_t pidIn)
{
    if(pidIn > 0)
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
}
