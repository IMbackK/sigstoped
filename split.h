/**
 * Sigstoped
 * Copyright (C) 2020 Carl Klemm
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 3 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

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
