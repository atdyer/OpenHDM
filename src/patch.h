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
#include "uref.h"

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

template <class ...UnitTypes>
class Patch{
    template <class pType, class ...uTypes> friend class Grid;

public:

    Patch(){}
    Patch(const Patch&) = default;
    Patch& operator=(const Patch&) = default;
    Patch(Patch&&) = default;
    Patch& operator=(Patch&&) = default;
    virtual ~Patch(){}


    template <class UnitType>
    void includeUnit(const mref<UnitType>& unit, unsigned ts);

    template <class UnitType>
    void excludeUnit(const mref<UnitType>& unit);

    void invalidate();

    // Returns the vector of unit references:
    template <class unitType>
    const auto& get_units()const{
        return std::get<std::vector<mref<unitType>>>(mrefsTuple);
    }

    // attribute accessors:
    bool isLocked()const{return locked;}
    bool isUpToDate()const{return upToDate;}
    unsigned getID()const{return id;}

protected:

    void setID(int patchID){id=patchID;}
    void validate(){upToDate=true;}

private:

    // references to grid units included in the patch
    std::tuple<std::vector<mref<UnitTypes>>...> mrefsTuple;

    bool upToDate   = false;
    bool locked     = false;
    unsigned id     = UINT_MAX;

};


// Includes a unit in the patch and activates the unit. (The parameter "ts":
// timestep at which the unit is included in the patch.)
template <class ...UnitTypes>
template <class UnitType>
void Patch<UnitTypes...>::includeUnit(const mref<UnitType>& unit, unsigned ts){

    // Get a reference to the corresponding unitptrs vector in unitptrsTuple tuple:
    auto& mrefs = std::get<std::vector<mref<UnitType>>>(mrefsTuple);

    // Determine and assign the position of the unitptr in unitptrs vector:
    unit().patchPos = unsigned(mrefs.size());

    // Activate the unit:
    unit().activate(ts);

    // Let unit store the id of the patch it is included in:
    unit().patchID = getID();

    // Add a copy of unit reference to this patch:
    mrefs.emplace_back(mref<UnitType>(unit));
}

// Removes a unit ptr from the patch, deactivates the unit and updates patchPos of units
// placed after the unit removed from the patch.
template <class ...UnitTypes>
template <class UnitType>
void Patch<UnitTypes...>::excludeUnit(const mref<UnitType>& unit){

    // Get a reference to the corresponding unitptrs vector in unitptrsTuple tuple:
    auto& mrefs = std::get<std::vector<mref<UnitType>>>(mrefsTuple);

    // Unit position inside unitptrs:
    unsigned int patchPos = unit().getPatchPos();

    // Deactivate the unit:
    unit().deactivate();

    // Erase the pointer from unitptrs:
    mrefs.erase(mrefs.begin()+patchPos);

    // Update the positions of units after the deleted unitptr:
    unsigned newSize = (unsigned)(mrefs.size());
    for (unsigned i=patchPos; i<newSize; i++){
        (mrefs[i]().patchPos)--;
    }
}


// This function is called by the associated grid object whenever an operation that may invalidate
// the pointers of the elements of tuple "unitsTuple" is performed.
template <class ...UnitTypes>
void Patch<UnitTypes...>::invalidate(){
    upToDate = false;
    locked = true;
}

} // end of namespace OpenHDM


#endif // PATCH_H
