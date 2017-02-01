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

#ifndef PATCH_H
#define PATCH_H

#include <iostream>
#include <memory>
#include <typeinfo>
#include <typeindex>
#include <tuple>
#include <climits>
#include <vector>
#include <algorithm>

namespace OpenHDM {

// --------------------------------------------------------------------
// Patch: The variadic abstract class template "Patch" is aimed to be
//   used as a base class for specialized patch types which are used to
//   designate the active regions of grids, i.e, the regions at which
//   the numerical computations are to be carried out. Patch is
//   designed to maintain pointers (as references) to the objects
//   stored in Grid, and so similar to grids, patches can have an
//   arbitrary number of unit types.
// --------------------------------------------------------------------

template <class ...unitTypes>
class Patch{
    template <class pType, class ...uTypes> friend class Grid;

public:

    Patch(){}
    Patch(const Patch&)             = default;
    Patch& operator=(const Patch&)  = default;
    Patch(Patch&&)                  = default;
    Patch& operator=(Patch&&)       = default;
    virtual ~Patch()                = default;


    template <class unitType>
    void includeUnit(const std::shared_ptr<unitType> unit, unsigned ts);

    template <class unitType>
    void excludeUnit(const std::shared_ptr<unitType> unit);

    void invalidate();

    // attribute accessors:
    bool isLocked()const{return locked;}
    bool isUpToDate()const{return upToDate;}
    unsigned getID()const{return id;}

protected:

    void setID(int patchID){id=patchID;}
    void validate(){upToDate=true;}

    std::tuple<std::vector<std::shared_ptr<unitTypes>>...> unitsTuple;
    // ^ This vector of tuples contains pointers to units of the patch instance.

private:
    bool upToDate   = false;
    bool locked     = false;
    unsigned id     = UINT_MAX;

};

// Inserts a unit ptr to the patch, assigns patchPos of the unit, and activates the unit.
template <class ...unitTypes>
template <class unitType>
void Patch<unitTypes...>::includeUnit(const std::shared_ptr<unitType> unit, unsigned ts){

    // Get a reference to the vector of units of type "unitType":
    auto& units = std::get<std::vector<std::shared_ptr<unitType>>>(unitsTuple);

    // Determine and assign the position of the unitptr in units vector:
    unit->patchPos = unsigned(units.size());

    // Activate the unit:
    unit->activate(ts);

    // Let unit store the id of the patch it is included in
    unit->patchID = getID();

    // Insert the unitptr to units vector
    units.push_back(unit);

}

// Removes a unit ptr from the patch, deactivates the unit and updates patchPos of units
// placed after the unit removed from the patch.
template <class ...unitTypes>
template <class unitType>
void Patch<unitTypes...>::excludeUnit(const std::shared_ptr<unitType> unit){

    // Get a reference to the vector of units of type "unitType":
    auto& units = std::get<std::vector<std::shared_ptr<unitType>>>(unitsTuple);

    // Unit position inside units:
    unsigned patchPos = unit->getPatchPos();

    // Deactivate the unit:
    unit->deactivate();

    // Exclude the unit
    units.erase(std::remove(units.begin(), units.end(), unit), units.end());

    // Update the positions of units after the deleted unitptr:
    unsigned newSize = (unsigned)(units.size());
    for (unsigned i=patchPos; i<newSize; i++){
        (units[i]->patchPos)--;
    }
}


// This function is called by the associated grid object whenever an operation that may invalidate
// the pointers of the elements of tuple "unitsTuple" is performed.
template <class ...unitTypes>
void Patch<unitTypes...>::invalidate(){
    upToDate = false;
    locked = true;
}

} // end of namespace OpenHDM


#endif // PATCH_H
