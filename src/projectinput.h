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


#ifndef PROJECTINPUT_H
#define PROJECTINPUT_H

#include "input.h"

/*
 * ProjectInput: Main input of a Project Class object
 */

class ProjectInput: public Input
{
    template <class domainClass> friend class Project;

public:
    ProjectInput(std::string projectFilePath);
    ~ProjectInput();

    virtual void readInputFile()override;

private:

    struct domainsListRow{

        // constructor:
        domainsListRow(std::string domainID_,
                       std::string domainPath_,
                       std::string outputDir_):
            domainID(domainID_),
            domainPath(domainPath_),
            outputDir(outputDir_)
        {}

        std::string domainID        = "";
        std::string domainPath      = "";
        std::string outputDir       = "";
        std::string parentID        = "";
    };

    unsigned int nd         = 0;
    std::string projectID   = "";
    std::vector<domainsListRow> domainsList;

};

#endif // PROJECTINPUT_H
