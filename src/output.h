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

#ifndef OUTPUT_H
#define OUTPUT_H

#include <iostream>
#include <fstream>
#include <vector>

namespace OpenHDM {

// --------------------------------------------------------------------
// Output: The concrete "Output" class is aimed to be used as a base
//   class for derived classes encapsulating model output files.
// --------------------------------------------------------------------

class Output
{
public:
    // Constructors & Operators:
    Output(bool isChild_);
    Output(const Output&) = default;
    Output& operator=(const Output&) = default;
    Output(Output&&) = default;
    Output& operator=(Output&&) = default;
    virtual ~Output();

    // Functions to be overridden:
    virtual void openOutputFile();
    virtual void writeHeader()=0;
    virtual void writeOutput(unsigned int ts=0)=0;
    virtual void closeOutputFile();

protected:
    int type                = 0;
    std::string fileDir     = ""; // the directory at which the output file will be created
    std::string fileName    = "";
    std::string filePath    = ""; // fileDir+fileName
    std::string fileTitle   = "";
    std::ofstream ofs;

    bool isChild;

};

} // end of namespace OpenHDM

#endif // OUTPUT_H
