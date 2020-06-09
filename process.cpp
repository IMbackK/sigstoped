#include <iostream>
#include <fstream>
#include <filesystem>
#include <signal.h>
#include "process.h"
#include "split.h"
#include "debug.h"

bool Process::operator==(const Process& in)
{
    return pid_ == in.pid_;
}
    
bool Process::operator!=(const Process& in)
{
    return pid_ != in.pid_;
}

pid_t Process::getPid()
{
    return pid_;
}

void Process::stop(bool children)
{
    kill(pid_, SIGSTOP);
    if(children)
    {
        std::vector<Process> children = getChildren();
        for(auto& child : children) child.stop(true);
    }
}

void Process::resume(bool children)
{
    kill(pid_, SIGCONT);
    if(children)
    {
        std::vector<Process> children = getChildren();
        for(auto& child : children) child.resume(true);
    }
}

bool Process::getStoped()
{
    return stoped_;
}

std::vector<std::string> Process::openStatus()
{
    std::fstream statusFile;
    statusFile.open(std::string("/proc/") + std::to_string(pid_)+ "/status", std::fstream::in);
    std::vector<std::string> lines;
    if(statusFile.is_open())
    {
        std::string statusString((std::istreambuf_iterator<char>(statusFile)),
        std::istreambuf_iterator<char>());
        lines = split(statusString);
        statusFile.close();
    }
    else
    {
        std::cout<<"cant open "<<"/proc/" + std::to_string(pid_)+ "/status"<<std::endl;
    }
    return lines;
}

std::vector<Process> Process::getChildren()
{
    std::vector<Process> ret;
    std::vector<pid_t> processes = getAllProcessPids();
    for(const auto & pid : processes)
    {
        Process process(pid);
        if(process.getPPid() == pid_) ret.push_back(process);
    }
    return ret;
}

std::vector<pid_t> Process::getAllProcessPids()
{
    std::vector<pid_t> processes;
    for(const auto & entry : std::filesystem::directory_iterator("/proc"))
    {
        pid_t pid = convertPid(entry.path().stem());
        if(pid > 0) processes.push_back(pid);
    }
    return processes;
}

std::string Process::getName()
{
    if(name_.size() == 0)
    {
        std::vector<std::string> lines = openStatus();
        if(lines.size() > 0)
        {
            std::vector<std::string> tokens = split(lines[0], '\t');
            if(tokens.size() > 1)
            {
                name_ = tokens[1];
            }
        }
    }
    return name_;
}

pid_t Process::convertPid(const std::string& pid)
{
    pid_t ret;
    try
    {
        ret = std::stol(pid);
    }
    catch(const std::invalid_argument& exception) 
    {
        debug(exception.what());
        ret = -1;
    }
    return ret;
}

pid_t Process::getPPid()
{
    if(ppid_ < 0)
    {
        std::vector<std::string> lines = openStatus();
        if(lines.size() > 7)
        {
           std::vector<std::string> tokens = split(lines[6] , '\t');
           if(tokens.size() > 1) ppid_ = convertPid(tokens[1]);
        }
    }
    return ppid_;
}
    
Process::Process(pid_t pidIn): pid_(pidIn)
{
}
