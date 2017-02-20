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

#ifndef UREF_H
#define UREF_H

#include <climits>
#include <vector>
#include <algorithm>

namespace OpenHDM {

// uref-related exception definitions:
class invalidated_ref: public std::exception{
    virtual const char* what() const throw(){
      return "A dereferencing is attempted for an invalidated ref.";
    }
};
class assignToSelf_ref: public std::exception{
    virtual const char* what() const throw(){
      return "Move assignment is called for self";
    }
};

template <class UnitType>
class mref;

// --------------------------------------------------------------------
// cref: The template class cref is used to store pointers to units in
//   a Grid instance. cref instances are maintained by the grid.
// --------------------------------------------------------------------

template <class UnitType>
class cref{
    template <class patchType, class ...unitTypes> friend class Grid;
    template <class unitType> friend class mref;

public:

    // Dereferencer:
    UnitType& operator()()const {

        if (unitPtr==nullptr){
            throw(invalidated_ref());
        }
        return *unitPtr;
    }

    // move-constructor:
    //   Called when the vector in crefsTuple is resized as a result of unit insertion.
    cref(cref&& original){

        // Pointer to unit
        unitPtr = original.unitPtr;

        // Update the root:
        root = this;

        // Copy the vector of duplicates
        duplicates = original.duplicates;

        // Update the root of duplicates:
        for (auto& duplicate: duplicates){
            duplicate->root = root;
        }
    }

    // move-assignment:
    //   When a unit is removed from a grid, the move-assignment operator is called
    //   for all crefs following the removed unit.
    cref& operator=(cref&& original){

        if (this == &original){
            throw(assignToSelf_ref());
        }

        // copy the pointer to unit:
        unitPtr = original.unitPtr;

        // Update the root:
        root = this;

        // Copy the vector of duplicates
        duplicates = original.duplicates;

        // Update the root of duplicates:
        for (auto &duplicate: duplicates){
            duplicate->root = root;
        }

        return *this;
    }


private:

    // Constructor:
    cref(UnitType& unit):
        unitPtr(&unit)
    {}

    // Delete copy-constructor and copy-assignment operator
    cref(const cref &old)         = delete;
    cref& operator=(const cref&)  = delete;

    // Adds a duplicate (of type mref)
    void addDuplicate(mref<UnitType>& d)const{
        duplicates.push_back(&d);
    }

    // Removes a duplicate (of type mref)
    void removeDuplicate(mref<UnitType>& d)const{
        duplicates.erase(std::remove(duplicates.begin(),
                                     duplicates.end(),
                                     &d),
                         duplicates.end());
    }

    // When the address of the pointed unit changes, this function is called by the grid
    // to update the address.
    void revalidate(UnitType& unit_moved){
        unitPtr = &unit_moved;

        // Inform the duplicates:
        for (auto &duplicate: duplicates){
            duplicate->revalidate(unit_moved);
        }
    }

    // Data members:
    cref * root = nullptr;
    UnitType * unitPtr;
    mutable std::vector<mref<UnitType>*> duplicates;
};



// --------------------------------------------------------------------
// mref: The template class mref is used to store pointers to units in
//   a Grid instance. mref instances may be maintained by any client.
// --------------------------------------------------------------------

template <class UnitType>
class mref{

    template <class patchType, class ...unitTypes> friend class Grid;
    template <class unitType> friend class cref;

public:

    // Dereferencer:
    UnitType& operator()()const{

        if (unitPtr==nullptr){
            throw(invalidated_ref());
        }
        return *unitPtr;
    }

    // Copy-constructor (from cref)
    mref(const cref<UnitType>& original)
    {
        // copy pointer to unit:
        unitPtr = original.unitPtr;

        // set the root:
        root = original.root;

        // add this new copy to the duplicates of root:
        original.addDuplicate(*this);

    }

    // Copy-constructor (from mref)
    mref(const mref &original)
    {
        // copy pointer to unit:
        unitPtr = original.unitPtr;

        // set the root:
        root = original.root;

        // add this new copy to the duplicates of root:
        root->addDuplicate(*this);
    }

    // move-constructor:
    mref(mref&& original){

        // copy pointer to unit:
        unitPtr = original.unitPtr;

        // set the root:
        root = original.root;

        // add this new copy to the duplicates of root:
        root->addDuplicate(*this);

        // Reset the original instance
        original.reset();
    }

    // move-assignment:
    mref& operator=(mref&& original){

        if (this == &original){
            throw(assignToSelf_ref());
        }

        // copy pointer to unit:
        unitPtr = original.unitPtr;

        // set the root:
        root = original.root;

        // add this new copy to the duplicates of root:
        root->addDuplicate(*this);

        // Reset the original instance
        original.reset();

        return *this;
    }

    // comparison operators:
    bool operator ==(const mref& other) const{
        if (unitPtr == other.unitPtr){
            return true;
        }
        return false;
    }
    bool operator !=(const mref& other) const{
        if (unitPtr == other.unitPtr){
            return false;
        }
        return true;
    }


    ~mref(){
        reset();
    }

private:

    // When the address of the pointed unit changes, this function is called by the grid
    // to update the address.
    void revalidate(UnitType &unit_moved){
        unitPtr = &unit_moved;
    }

    // When the mref object is destructed or moved,inform the root.
    void reset(){

        if (root!=nullptr){
            root->removeDuplicate(*this);
        }
        unitPtr = nullptr;
        root = nullptr;
    }

    // Data members:
    cref<UnitType> * root = nullptr;
    UnitType * unitPtr;
};

}


#endif // UREF_H
