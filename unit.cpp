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


#include <iostream>

#include "report.h"
#include "patch.h"
#include "unit.h"

Unit::Unit(int id_):
    id(id_)
{

}

Unit::~Unit(){

}

// Deactivates the unit to remove it from calculations
void Unit::deactivate(){

    if (not active){
        Report::error("Unit deactivation","Unit "+std::to_string(id)+" is already deactivated.");
    }
    active = false;
}

// Activates the unit to include it in calculations
void Unit::activate(unsigned int ts){

    if (active){
        Report::error("Unit activation","Unit "+std::to_string(id)+" is already active."
                      " Activation timestep: "+std::to_string(activationTimestep));
    }

    active = true;
    activationTimestep = ts;

    if (ts==0){
        initiallyActive = true;
    }
}


void Unit::setPos(unsigned int pos_in){
    pos = pos_in;
}

