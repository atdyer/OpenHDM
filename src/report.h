// OpenHDM: An Open-Source Sofware Framework for Hydrodynamic Models
// Copyright (C) 2015-2017 
// Alper Altuntas <alperaltuntas@gmail.com>

// This file is part of OpenHDM.

// OpenHDM is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// OpenHDM is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with OpenHDM.  If not, see <http://www.gnu.org/licenses/>.

#ifndef REPORT_H
#define REPORT_H

#include <iostream>
#include <string>
#include <algorithm>

namespace OpenHDM {

// --------------------------------------------------------------------
// Report: An auxiliary concrete class aimed to be used for reporting
//   errors, warnings, and logs.
// --------------------------------------------------------------------

class Report
{
public:

    static void error(std::string source, std::string dscrpt);
    static void warning(std::string source, std::string dscrpt, unsigned short severity=1);
    static void log(std::string msg, unsigned short level=1);

    // Prints multiple number of variables (of any type) to the screen:
    // The implementation is provided at the end of this header file.
    // ( This variadic function template is mostly used for debugging purposes)
    template <typename FirstVarType, typename ...Rest>
    static void printv(FirstVarType v1, Rest... remainingVars);

    // Prints the last variable for printv(FirstVarType v1, Rest... remainingVars);
    template <typename VarType>
    static void printv(VarType var);

private:
    static void printMessage(std::string source, std::string msg);
};

template <typename FirstVarType, typename ...Rest>
void Report::printv(FirstVarType v1, Rest... remainingVars){

    // Print the next variable:
    std::cout << v1 << " ";

    // Calls itself recursively to print the remaining variables,
    // If there is only one variable left, calls printv(VarType varLast)
    printv(remainingVars...);

}

template <typename VarType>
void Report::printv(VarType varLast){
    // Print the last variable:
    std::cout << varLast << std::endl;
}

} // end of namespace OpenHDM


// Let other files use OpenHDM::Report functions without having to
// specify OpenHDM namespace explicitly:
using OpenHDM::Report;

#endif // REPORT_H
