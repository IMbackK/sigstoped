#pragma once
#include <sstream>
#include <string>

inline std::vector<std::string> split(const std::string& in, char delim = '\n' )
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
