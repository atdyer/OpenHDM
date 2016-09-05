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


#include "threading.h"

Threading::ControlPoint::ControlPoint()
{

}

void Threading::ControlPoint::increment(){
    std::unique_lock<std::shared_timed_mutex> uLock(mtx, std::defer_lock);
    val = (val+1)%ncp;
    done = false;
}

void Threading::ControlPoint::markDone(){
    std::unique_lock<std::shared_timed_mutex> uLock(mtx, std::defer_lock);
    done = true;
}

unsigned int Threading::ControlPoint::getVal()const{
    std::shared_lock<std::shared_timed_mutex> sLock(mtx, std::defer_lock);
    return val;
}

bool Threading::ControlPoint::isDone()const{
    std::shared_lock<std::shared_timed_mutex> sLock(mtx, std::defer_lock);
    return done;
}


Threading::Pool::Pool(unsigned int nProcs_):
    nProcs(nProcs_),
    remainingThreads(nProcs)
{

}

void Threading::Pool::acquire(){
    std::unique_lock<std::mutex> uLock(mtx);
    cond.wait(uLock, [&]{return remainingThreads>0;});
    remainingThreads--;
}

void Threading::Pool::release(){
    std::unique_lock<std::mutex> uLock(mtx);
    remainingThreads++;
    cond.notify_all();
}
