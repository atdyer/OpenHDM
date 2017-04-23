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

#ifndef GRID_H
#define GRID_H

#include <iostream>
#include <climits>
#include <memory>
#include <typeinfo>
#include <typeindex>
#include <tuple>
#include <vector>
#include <deque>
#include <list>
#include <map>
#include <unordered_map>
#include "report.h"

namespace OpenHDM {

// --------------------------------------------------------------------
// Grid: The variadic abstract class template Grid is the container and
//   manager of discrete model data for individual domain instances. A
//   derived grid class can have an arbitrary number of unit types,
//   such as nodes, elements, cells, etc., depending on the type of the
//   spatial discretization. A grid instance also owns one or more
//   patches as members, which designate the active regions within the
//   grid.
// --------------------------------------------------------------------

template <class patchType, class ...unitTypes>
class Grid{
public:

    // Constructors & operators:
    Grid(){};
    Grid(const Grid&) = default;
    Grid& operator=(const Grid&) = default;
    Grid(Grid&&) = default;
    Grid& operator=(Grid&&) = default;
    virtual ~Grid() = default;


    // Functions for unit management:

    template <class unitType>
    void insertUnit(unitType u);

    template <class unitType>
    void copyFromParent(unitType const & parentUnit);

    template <class unitType>
    void removeUnit(unitType const & u);

    template <class unitType>
    void setUnitPosition(unitType &u, std::vector<unitType> &units, std::type_index &typeIndex);

    template <class unitType>
    bool confirmUnitPosition(unitType const &u)const;

    bool unitExists(int id, std::type_index &typeIndex);

    // Functions for patch management:
    virtual void initializePatches()=0; // for parent domains
    patchType & addPatch();
    void removePatch(patchType &patch);
    void setPatchID(patchType &patch);
    patchType & getPatch(unsigned id);

    // attribute accessors:
    unsigned int get_id2pos(int id, std::type_index &typeIndex){return id2pos[typeIndex][id];}

protected:

    // Computational patches:
    std::deque<patchType> patches;
    std::list<int> vpids;  // vacant patch id's

    // Units:
    std::tuple<std::vector<unitTypes>...> unitsTuple;           // the container for all of the grid units (of any type)
    std::map< std::type_index, std::list<unsigned int> > upos;  // unit positions
    std::map< std::type_index, std::list<unsigned int> > vpos;  // vacant unit positions

    // mapping from id's of units to their position in vector "units":
    mutable std::map< std::type_index, std::unordered_map<int,unsigned int> > id2pos;

    // mapping from positions of child domain units to positions of corresponding parent domain units
    std::map< std::type_index, std::unordered_map<unsigned int,unsigned int> > cp2pp;

    // mapping from positions of parent domain units to positions of corresponding child domain units
    std::map< std::type_index, std::unordered_map<unsigned int,unsigned int> > pp2cp;
};


// Inserts a unit of any type to unitsTuple
template <class patchType, class ...unitTypes>
template <class unitType>
void Grid<patchType,unitTypes...>::insertUnit(unitType u){

    auto typeIndex = std::type_index(typeid(unitType));
    auto& units = std::get<std::vector<unitType>>(unitsTuple);

    // Assign a position to the unit to be inserted:
    setUnitPosition(u, units, typeIndex);

    // Add the unit to the end of the "units" vector
    units.push_back(u);

    // Update positions list:
    std::list<unsigned int> &up = upos[typeIndex];
    up.push_back(units.back().getPos());

    // Update id2pos
    id2pos[typeIndex][units.back().getID()] = units.back().getPos();

    // Adding to and removing from "units" vector may invalidate references to elements of the
    // vector which are stored in "unitptrs" of patches. Inform all the patches of the grid:
    for (auto &patch : patches){
        if (patch.isUpToDate()){
            patch.invalidate();
        }
    }
}

// Copies a parent unit and inserts it to unitsTuple
template <class patchType, class ...unitTypes>
template <class unitType>
void Grid<patchType,unitTypes...>::copyFromParent(unitType const & parentUnit){

    auto typeIndex = std::type_index(typeid(unitType));
    auto& units = std::get<std::vector<unitType>>(unitsTuple);

    // Insert a new unit which is copy constructed using the parentUnit as argument.
    insertUnit(unitType(parentUnit));

    unsigned int parentPos = parentUnit.getPos();
    unsigned int childPos = units.back().getPos();

    // Construct the mapping from child unit positions to parent unit positions
    cp2pp[typeIndex][childPos] = parentPos;
    pp2cp[typeIndex][parentPos] = childPos;

}


// Removes a given unit
// Note: Avoid using this function. Always prefer to deactivate the unit instead.
template <class patchType, class ...unitTypes>
template <class unitType>
void Grid<patchType,unitTypes...>::removeUnit(const unitType &u){

    Report::warning("Removing Unit at position "+std::to_string(u.getPos())+"\n",1);

    auto typeIndex = std::type_index(typeid(unitType));
    auto& units = std::get<std::vector<unitType>>(unitsTuple);

    // get the position of the unit:
    if ( not confirmUnitPosition(u) )
        Report::error("Grid::removeUnit","Unit position to be removed is incorrect.");
    unsigned int pos = u.getPos();

    // Update position lists:
    std::list<unsigned int> &up = upos[typeIndex];
    up.remove(pos);
    std::list<unsigned int> &vp = vpos[typeIndex];
    vp.push_back(pos);

    // remove the unit
    units.erase(units.begin()+pos);

    // Update positions of the units after "u":
    for (size_t i=pos; i<units.size(); i++){
        units[i]->pos--;
        // Update id2pos
        (id2pos[typeIndex][units[i].getID()] )--;
    }

    // Adding to and removing from "units" vector may invalidate references to elements of the
    // vector which are stored in "unitptrs" of patches. Inform all the patches of the grid:
    for (auto &patch : patches){
        if (patch.isUpToDate()){
            patch.invalidate();
        }
    }

}

// Sets the position of a new unit.
template <class patchType, class ...unitTypes>
template <class unitType>
void Grid<patchType,unitTypes...>::setUnitPosition(unitType &u,
                                                   std::vector<unitType> &units,
                                                   std::type_index &typeIndex){

    std::list<unsigned int> &vp = vpos[typeIndex];

    if (vp.size()>0){
        u.pos = vp.front();
        vp.pop_front();
    }
    else{
        u.pos = unsigned(units.size());
    }
}


// Ensures that the position of a unit is accurate
template <class patchType, class ...unitTypes>
template <class unitType>
bool Grid<patchType,unitTypes...>::confirmUnitPosition(unitType const &u)const{

    auto& units = std::get<std::vector<unitType>>(unitsTuple);

    auto it = std::find( units.begin(), units.end(), u);
    unsigned int pos = std::distance(units.begin(),it);
    return (pos==u.getPos());
}


// Checks if a unit exists by searching the corresponging vector in "id2pos" map
template <class patchType, class ...unitTypes>
bool Grid<patchType,unitTypes...>::unitExists(int id, std::type_index &typeIndex){

    if (id2pos[typeIndex].find(id) == id2pos[typeIndex].end()){
        return false;
    }
    else{
        return true;
    }

}

// Adds a new patch in "patches" vector
template <class patchType, class ...unitTypes>
patchType& Grid<patchType,unitTypes...>::addPatch(){

    // Add a new patch to the "patches" vector:
    patches.emplace_back();

    // Assign an id to the patch to be added:
    setPatchID(patches.back());

    // Return the new patch id:
    return patches.back();

}

// Removes a given patch from "patches" deque
template <class patchType, class ...unitTypes>
void Grid<patchType,unitTypes...>::removePatch(patchType &patch){

    // Get the index of the patch to remove:
    unsigned pi = 0;
    for (auto &p: patches){
        if (p.getID()==patch.getID()){
            break;
        }
        pi++;
    }

    // Remove the patch;
    patches.erase(patches.begin()+pi);

}

// Sets the position of a new patch.
template <class patchType, class ...unitTypes>
void Grid<patchType,unitTypes...>::setPatchID(patchType &patch){

    // Check if an id is already assigned:
    if (patch.getID()!=UINT_MAX){
        Report::error("Grid::setPatchID","An ID is already assigned to patch");
    }

    // Assign an id:
    if (vpids.size()>0){
        patch.setID(vpids.front());
        vpids.pop_front();
    }
    else{
        patch.setID(int(patches.size()-1));
    }

}

// Return a reference to the patch with given ID
template <class patchType, class ...unitTypes>
patchType& Grid<patchType,unitTypes...>::getPatch(unsigned id){

    patchType * patchToReturn = nullptr;

    for (auto &patch: patches){
        if (patch.getID()==id){
            patchToReturn = &(patch);
        }
    }

    if (not patchToReturn){
        Report::error("Grid::getPatch","No patch with the given id exists: "+std::to_string(id));
    }

    return std::ref(*patchToReturn);

}

} // end of namespace OpenHDM

#endif // GRID_H
