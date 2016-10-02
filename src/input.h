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

#ifndef INPUT_H
#define INPUT_H

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>

namespace OpenHDM {

// --------------------------------------------------------------------
// Input: The concrete "Input" class is aimed to be used as a base
//   class for derived classes encapsulating model input files.
// --------------------------------------------------------------------

class Input
{
public:
    // Constructors & Operators:
    Input(std::string fileFormat_, std::string filePath_);
    Input(const Input&) = default;
    Input& operator=(const Input&) = default;
    Input(Input&&) = default;
    Input& operator=(Input&&) = default;
    virtual ~Input();

    // Functions to be overridden:
    virtual void openInputFile();
    virtual void readInputFile() = 0;
    virtual void closeInputFile();


    // Variadic function template to read parameters from an input file
    // (Function template implementations are at the end of this file)
    template <typename ...ParamTypes>
    static void readParams(std::ifstream& ifs, ParamTypes&... params);

    // auxiliary
    static std::vector<std::string> splitLine(std::string line);
    static void trimString(std::string &s);
    static std::string getDir(std::string Path);

    // attribute accessors
    std::string getFileFormat(){return fileFormat;}
    std::string getFilePath(){return filePath;}
    std::string getFileTitle(){return fileTitle;}

protected:

    int type                = 0;
    std::ifstream ifs;
    std::string header      = "";
    std::string fileFormat  = "";
    std::string filePath    = "";
    std::string fileTitle   = "";

private:
    // The following function templates are required by the variadic function template readLine()
    // (Function template implementations are at the end of this file)
    template <typename ParamType>
    static void readParams(std::stringstream &sline, ParamType &param);
    template <typename FirstPType, typename ... Rest>
    static void readParams(std::stringstream &sline, FirstPType &param, Rest&... remainingParams);

};

// Reads the values of arbitrary number of parameters from a line of an input file
// Note: May be expensive if called within an exhaustive loop.
template <typename ...ParamTypes>
void Input::readParams(std::ifstream& ifs, ParamTypes&... params){

        std::string line;
        std::stringstream sline;

        std::getline(ifs,line);
        sline.str(line);

        // Call the recursive function to read in all the parameters
        readParams(sline, params...);
}

template <typename FirstPType, typename ... Rest>
void Input::readParams(std::stringstream &sline, FirstPType &param, Rest&... remainingParams){
    // Set the parameter:
    sline >> param;
    // Calls itself recursively until one parameter remains in remainingParams:
    readParams(sline, remainingParams...);
}

template <typename ParamType>
void Input::readParams(std::stringstream &sline, ParamType &param){
    // Set the last parameter and end recursion
    sline >> param;
}

} // end of namespace OpenHDM

#endif // INPUT_H
