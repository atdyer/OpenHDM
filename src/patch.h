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

template <class ...UnitTypes>
class Patch{
    template <class pType, class ...uTypes> friend class Grid;

public:

    Patch();
    Patch(const Patch&) = default;
    Patch& operator=(const Patch&) = default;
    Patch(Patch&&) = default;
    Patch& operator=(Patch&&) = default;
    virtual ~Patch();


    template <class UnitType>
    void insertUnitPtr(UnitType * unitptr, unsigned ts);

    template <class UnitType>
    void removeUnitPtr(UnitType * unitptr);

    void invalidate();

    // attribute accessors:
    bool isLocked()const{return locked;}
    bool isUpToDate()const{return upToDate;}
    unsigned getID()const{return id;}

protected:

    void setID(int patchID){id=patchID;}
    void validate(){upToDate=true;}

    std::tuple<std::vector<UnitTypes*>...> unitptrsTuple;
    // ^ This vector of tuples contains pointers to units of the patch instance. (The pointed units
    // are normally stored in "unitsTuple" container of the associated grid instance.)
    // Important: Before dereferencing the elements of unitptrs, ensure that the pointers
    // stored are not invalidated! (See invalidate()).

private:
    bool upToDate   = false;
    bool locked     = false;
    unsigned id     = UINT_MAX;

};

template <class ...UnitTypes>
Patch<UnitTypes...>::Patch()
{

}

template <class ...UnitTypes>
Patch<UnitTypes...>::~Patch(){

}

// Inserts a unit ptr to the patch, assigns patchPos of the unit, and activates the unit.
template <class ...UnitTypes>
template <class UnitType>
void Patch<UnitTypes...>::insertUnitPtr(UnitType * unitptr, unsigned ts){

    // Get a reference to the corresponding unitptrs vector in unitptrsTuple tuple:
    auto& unitptrs = std::get<std::vector<UnitType*>>(unitptrsTuple);

    // Determine and assign the position of the unitptr in unitptrs vector:
    unitptr->patchPos = unsigned(unitptrs.size());

    // Activate the unit:
    unitptr->activate(ts);

    // Let unit store the id of the patch it is included in
    unitptr->patchID = getID();

    // Insert the unitptr to unitptrs vector
    unitptrs.push_back(unitptr);

}

// Removes a unit ptr from the patch, deactivates the unit and updates patchPos of units
// placed after the unit removed from the patch.
template <class ...UnitTypes>
template <class UnitType>
void Patch<UnitTypes...>::removeUnitPtr(UnitType * unitptr){

    // Get a reference to the corresponding unitptrs vector in unitptrsTuple tuple:
    auto& unitptrs = std::get<std::vector<UnitType*>>(unitptrsTuple);

    // Unit position inside unitptrs:
    unsigned int patchPos = unitptr->getPatchPos();

    // Deactivate the unit:
    unitptr->deactivate();

    // Erase the pointer from unitptrs:
    unitptrs.erase(std::remove(unitptrs.begin(),unitptrs.end(),unitptr),unitptrs.end());

    // Update the positions of units after the deleted unitptr:
    unsigned int newSize = (unsigned)(unitptrs.size());
    for (unsigned int i=patchPos; i<newSize; i++){
        (unitptrs[i]->patchPos)--;
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
