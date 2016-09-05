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


#include <sstream>
#include <iterator>

using namespace std;

#include "report.h"
#include "input.h"

Input::Input(std::string fileFormat_, std::string filePath_):
    fileFormat(fileFormat_),
    filePath(filePath_)
{

}

Input::~Input()
{
    if (ifs.is_open()){
        closeInputFile();
    }
}

void Input::openInputFile(){

    if (filePath == ""){
        Report::error("Input: "+fileTitle, "Input file path is empty.");
    }
    else{
        ifs.open(filePath,std::ifstream::in);
        if ( not (ifs.is_open()) ){
            Report::error("Input File: "+fileTitle,
                                 "Cannot open input file at "+filePath);
        }
    }
}


void Input::closeInputFile(){
    ifs.close();
}


// Splits a string into vector of strings
vector<string> Input::splitLine(string line){

    vector<string> columns;
    istringstream iss(line);
    copy(istream_iterator<string>(iss),
              istream_iterator<string>(),
              back_inserter<vector <string> >(columns) );
    return columns;
}

// Removes whitespaces from the beginning and end of a string
void Input::trimString(string &s){
    auto iFirst = s.find_first_not_of(' ');
    auto iLast = s.find_last_not_of(' ');
    s = s.substr(iFirst,iLast-iFirst+1);
}

// Returns the directory of a file at a given path
string Input::getDir(string path){

    string dir;

    size_t findAnySlash = path.find('/');  // Linux !
    if (findAnySlash != string::npos){
        size_t findLast = path.find_last_of('/');
        dir = path.substr(0,findLast+1);
    }
    else{
        dir = "./";
    }
    return dir;
}

