/*
 * OpenHDM: An Open-Source Sofware Framework for Hydrodynamic Models
 * Copyright (C) 2015-2017 
 * Alper Altuntas <alperaltuntas@gmail.com>
 *
 * This file is part of OpenHDM.
 *
 * OpenHDM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenHDM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenHDM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sstream>
#include <iterator>

#include "report.h"
#include "projectinput.h"

ProjectInput::ProjectInput(std::string projectFilePath):
    Input("",projectFilePath)
{
    type = 1;
    filePath = projectFilePath;
    fileTitle = "Project File";

    readInputFile();
}

ProjectInput::~ProjectInput()
{

}


void ProjectInput::readInputFile(){

    std::string line = "";

    openInputFile();

    // header
    readParams(ifs, header);

    // project id:
    readParams(ifs,projectID);

    // domains:
    readParams(ifs,nd);
    for (unsigned int d=0; d<nd; d++){

        // Read the current line in domains list, and split it into columns
        std::getline(ifs ,line);
        std::vector<std::string> sLine = splitLine(line);

        // Generate a new domainsList entry, and initialize it using the parameters
        // from the list row.
        domainsList.emplace_back(sLine[0],  // domainID
                                 sLine[1],  // domainPath
                                 sLine[2]); // outputDir

        // If the current row has 4 columns, i.e., if the domain is a child domain,
        // then set its parent id.
        switch (sLine.size()) {
        case 3:
            // parent domain. skip.
            break;
        case 4:
            // child domain. set its parentID.
            domainsList.back().parentID = sLine[3];
            break;
        default:
            Report::error("Project Input!",
                          "Invalid number of parameters for Domain: "+sLine[0]);
            break;
        }

    }

}

