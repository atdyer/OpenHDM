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

#include <sys/types.h>
#include <sys/stat.h>
#include "report.h"
#include "output.h"

using namespace OpenHDM;

// --------------------------------------------------------------------
// This source file includes some of the function implementations of
// the "Output" class defined in output.h
// --------------------------------------------------------------------

Output::Output(bool isChild_):
    fileDir(""),
    isChild(isChild_)
{

}

Output::~Output()
{
    if (ofs.is_open()){
        closeOutputFile();
    }
}

void Output::openOutputFile(){

    // Note: This function is Linux-specific !!!

    if (fileDir == ""){
        Report::error("Output file couldn't be opened.", "File directory is not provided.");
    }

    struct stat s;

    if ( stat(fileDir.c_str(), &s) != 0){
        // Directory is not found. Create the directory:
        Report::log("Couldn't find '"+fileDir+"'. Creating the directory:",3);
        if ( mkdir(fileDir.c_str(), 0744) != 0 and errno != EEXIST)
            Report::error("Output File!",fileDir+" directory could not be created");
        else
            Report::log("The directory is successfully created.",4);
    }
    else if ( not (s.st_mode & S_IFDIR) ){
        Report::error("Output File!",fileDir+" is not a directory");

    }

    if (fileName == ""){
        Report::error("Output file couldn't be opened.", "File name is not provided.");
    }

    filePath = fileDir+"/"+fileName;

    ofs.open(filePath,std::ofstream::out);
    if (not ofs.is_open()){
        Report::error("Output File!",
                      fileTitle+" at "+filePath+" could not be opened.");
    }
}


void Output::closeOutputFile(){
    ofs.close();
}
