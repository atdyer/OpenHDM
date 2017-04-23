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
#include <deque>
#include "report.h"

namespace OpenHDM {

// --------------------------------------------------------------------
// Solver: The abstract class template Solver is used as a base class
//   for derived solver types that implement the computational tasks
//   specific to the model and include the numerical libraries to be
//   used.
// --------------------------------------------------------------------

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
    bool    isChild() const;
    size_t  nGrids() const{return grids.size();}
    const auto& getGrids()const{return grids;}

protected:

    std::deque<GridType> grids;
    std::shared_ptr<Solver<GridType>> parent;

};

template<class GridType>
Solver<GridType>::Solver(std::shared_ptr<Solver<GridType>> parent_):
    parent(parent_)
{

}

template<class GridType>
Solver<GridType>::~Solver()
{

}

template<class GridType>
bool Solver<GridType>::isChild() const{
    if (parent) return true;
    return false;
}

} // end of namespace OpenHDM

#endif // SOLVER_H
