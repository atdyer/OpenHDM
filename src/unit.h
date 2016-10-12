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

#ifndef UNIT_H
#define UNIT_H

#include <climits>

namespace OpenHDM {

// --------------------------------------------------------------------
// Unit: The abstract class "Unit" is aimed to be used as a base class
//   for data structures encapsulating the discrete mesh data. Example
//   derived unit types are nodes, elements, cells, interfaces, etc.
// --------------------------------------------------------------------

class Unit{
    template <class patchType, class ...unitTypes> friend class Grid;
    template <class ...UnitTypes> friend class Patch;

public:
    // Constructors & Operators:
    Unit(int id_);
    Unit(const Unit&) = default;
    Unit& operator=(const Unit&) = default;
    Unit(Unit&&) = default;
    Unit& operator=(Unit&&) = default;
    virtual ~Unit()=0;

    // attribute accessors:
    int      getID()const{return id;}
    unsigned getPos()const{return pos;}
    unsigned getPatchPos()const{return patchPos;}
    unsigned getActivationTimestep()const{return activationTimestep;}
    bool     isActive()const{return active;}
    bool     isBoundary()const{return boundary;}
    unsigned getPatchID()const{return patchID;}

protected:

    void deactivate();
    void activate(unsigned ts=0);
    void setPos(unsigned pos_in);

private:

    bool active          = false;
    bool boundary        = false;
    unsigned patchPos; // the position of the unit within a patch
    unsigned pos;   // index of the unit
    unsigned activationTimestep;
    const int id;       // constant id used for input/output

    unsigned patchID = UINT_MAX; // The id of the patch the node is included in.
                                 // Note that Unit::patchPos is used to indentify a unit,
                                 // wheras Unit::patchID is used to indentify a patch

};

} // end of namespace OpenHDM

#endif // UNIT_H
