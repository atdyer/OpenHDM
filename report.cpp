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

#include "report.h"

__attribute__((noreturn))
void Report::error(std::string source, std::string dscrpt){

    if ( source!="" or dscrpt!=""){
        std::cerr << std::endl;
        std::cerr << "\t" << "ERROR: " << source << std::endl;
        std::cerr << "\t" << dscrpt << std::endl << std::endl;
    }

    //throw std::runtime_error ("dscrpt");
    exit(EXIT_FAILURE);
}

void Report::warning(std::string source, std::string dscrpt, unsigned short severity){
    switch (severity) {
    case 9999:
        printMessage( ("Warning: "+source), dscrpt );
        break;
    default:
        printMessage( ("Warning: "+source), dscrpt );
        break;
    }
}

void Report::log(std::string msg, unsigned short level){
    std::string indent = "  ";
    for (unsigned short l=0; l<level; l++)
        indent = indent+"  ";

    std::clog << indent << msg << std::endl;
}

void Report::printMessage(std::string source, std::string msg){
    std::cout << std::endl;
    std::cout << "\t " << source << std::endl;
    std::cout << "\t " << msg << std::endl << std::endl;
}
