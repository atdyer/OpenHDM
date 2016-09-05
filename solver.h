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


#ifndef SOLVER_H
#define SOLVER_H

#include <list>
#include <map>
#include <unordered_map>
#include <memory>

#include "report.h"

/* Solver: an abstract class template that accepts a type of a Grid as its template
 * parameter.
 *
 * A derived solver class instance is owned by a domain and it contains a grid and
 * a set of inputs/outputs. The implementations of numerical methods are provided in
 * the corresponding member functions of derived solver classes.
 */
template<class GridType>
class Solver
{
public:
    // Constructors & operators:
    Solver(std::shared_ptr<Solver<GridType>> parent_);
    Solver(const Solver&) = default;
    Solver& operator=(const Solver&) = default;
    Solver(Solver&&) = default;
    Solver& operator=(Solver&&) = default;
    virtual ~Solver()=0;

    // initializations:
    virtual void initialize()=0;

    // timestep initialization:
    virtual void adjustPatches(unsigned int ts)=0;
    virtual void imposePatchBCs(unsigned int phase)=0;

    // attribute accessors:
    const std::shared_ptr<GridType> &getGrid();       // returns a void pointer to its grid
    bool       isChild();

protected:

    std::shared_ptr<GridType> grid;
    std::shared_ptr<Solver<GridType>> parent;

};

template<class GridType>
Solver<GridType>::Solver(std::shared_ptr<Solver<GridType>> parent_):
    grid(nullptr),
    parent(parent_)
{

}

template<class GridType>
Solver<GridType>::~Solver()
{

}

template<class GridType>
const std::shared_ptr<GridType>& Solver<GridType>::getGrid(){
    return grid;
}


template<class GridType>
bool Solver<GridType>::isChild(){
    if (parent) return true;
    return false;
}



#endif // SOLVER_H
