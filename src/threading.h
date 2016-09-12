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


#ifndef THREADING_H
#define THREADING_H

#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>

// forward declare Domain template outside the scope of Threading namespace
template <class SolverType> class Domain;


namespace Threading {

struct ControlPoint{
    template <class SolverType> friend class ::Domain;

    ControlPoint();

    void increment();
    void markDone();
    unsigned int getVal()const;
    bool isDone()const;

private:
    unsigned int ncp    = 0;    // Number of ctrl pts at which domains synchronize
    unsigned int val    = -1;   // Current control point
    bool done           = true; // ready to move to the next control point?

    mutable std::shared_timed_mutex mtx;    // mutex for "val" and "done"
};


struct Pool{
    Pool(unsigned int nProcs_);

    void acquire();
    void release();

private:
    mutable std::mutex mtx;
    std::condition_variable cond;
    const unsigned int nProcs;
    unsigned int remainingThreads = 0;

};


}

#endif // THREADING_H
