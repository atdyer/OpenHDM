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

#ifndef DOMAIN_H
#define DOMAIN_H

#include <iostream>
#include <memory>
#include <vector>
#include <list>
#include <string>
#include <sstream>
#include "threading.h"
#include "report.h"

namespace OpenHDM {

template <class SolverType> class Project; // forward declaration

// --------------------------------------------------------------------
// Domain: The abstract class template "Domain" is aimed to be used as
//   a base class for derived domain classes. Each Domain object
//   encapsulates an individual model domain instance that has a
//   distinct set of inputs a computational grid, and outputs. The
//   abstract class template leaves the implementation of tasks that
//   fully depend on the model details, such as reading input files and
//   lazy initialization of derived-type members, to the developer of
//   the derived model through pure virtual functions. General tasks
//   regarding domain hierarchy, phasing, and mutithreading are
//   implemented in this abstract template.
// --------------------------------------------------------------------

template <class SolverType>
class Domain: public std::enable_shared_from_this<Domain<SolverType>>
{
    template <class domainType> friend class OpenHDM::Project;

public:

    // Constructors & Operators
    Domain(std::string  id_,
           std::string  path_,
           std::string  outputDir_);
    Domain(const Domain&) = default;
    Domain& operator=(const Domain&) = default;
    Domain(Domain&&) = default;
    Domain& operator=(Domain&&) = default;
    virtual ~Domain(){}

    // attribute accessors:
    bool            isParent()const;
    bool            isChild()const;
    bool            isInitialized()const;
    bool            hierarchyIsSet()const;
    std::string     getID()const;
    std::string     getOutputDir()const;
    int             get_nChild() const;
    std::shared_ptr<Domain> getChild(unsigned int i);
    std::shared_ptr<Domain> getParent();
    short get_nPhases();
    virtual unsigned int    get_nts()const=0;
    const std::shared_ptr<SolverType>& getSolver()const;

protected:

    // initializations:
    virtual void instantiateMembers()=0;
    virtual void readInputs()=0;
    virtual void doInitialize()=0;
    void initialize();

    // runtime:
    void timestepping(unsigned nts);

    // hierarchy:
    virtual bool setParent(std::shared_ptr<Domain> parent);
    bool addChild(std::shared_ptr<Domain> newChild);
    void setHierarchy(std::shared_ptr<Domain> parent=nullptr);

    // concurrency and phases:
    void setConcurrency(unsigned nProc=0);
    void insertPhase(std::function<void(unsigned int)> phase);

    // post-processing routines after the run is completed:
    virtual void postProcess()=0;

    // input parameters:
    std::string id              = "";
    std::string path            = "";
    std::string outputDir       = "";

    std::shared_ptr<Domain> parent;
    std::vector<std::weak_ptr<Domain>> childDomains;
    std::shared_ptr<SolverType> solver;

    // multithreading:
    void phaseCheck();
    void completePhase();

private:

    bool initialized    = false;
    bool hierarchySet   = false;

    // phase container:
    std::vector<std::function<void(unsigned int)>> phases;

    // Multithreading
    Threading::ControlPoint             cp;
    std::shared_ptr<Threading::Pool>    threadPool;
    std::shared_ptr<std::mutex>         mtx;
    std::shared_ptr<std::condition_variable> condition;
    std::shared_ptr<std::condition_variable> condition4children;
    std::vector<std::reference_wrapper<Threading::ControlPoint>> childCPs;

};

// Domain Constructor:
template <class SolverType>
Domain<SolverType>::Domain(std::string  id_,
                            std::string  path_,
                            std::string  outputDir_):
    id(id_),
    path(path_),
    outputDir(outputDir_),
    parent(nullptr),
    solver(nullptr)
{
    Report::log("Domain "+id+" is constructed.",2);

}

// Finalizes the domain initialization. Called before timestepping begins.
template <class SolverType>
void Domain<SolverType>::initialize(){

    // Call the virtual function doInitialize(). The implementation of this function must
    // be provided in derived classes.
    doInitialize();

    // Set the domain as initialized.
    initialized = true;
}


// Executes timestep "ts"
template <class SolverType>
void Domain<SolverType>::timestepping(unsigned nts){

    Report::log("Initiating timestepping for the domain "+getID(),1);

    for (unsigned ts=1; ts<=nts; ts++){
        for (auto &phase : phases){

            // Check if ready to execute the phase
            phaseCheck();

            // Execute the phase
            phase(ts);

            // Notify phase completion
            completePhase();

        }
    }
}

// Adds a child to a parent domain.
template <class SolverType>
bool Domain<SolverType>::addChild(std::shared_ptr<Domain<SolverType>> newChild){

    // Create a weak pointer to the child domain from the passed shared pointer
    std::weak_ptr<Domain<SolverType>> newChild_weak(newChild);

    // Add the new weak pointer to childDomains vector
    childDomains.push_back(newChild_weak);

    // Set the parent of the child
    newChild->setParent(this->shared_from_this());

    // Set hierarchySet to true
    hierarchySet = true;

    return true;
}

// Inserts a phase to the vector of phases. Each phase is sequentially executed at every timestep
template <class SolverType>
void Domain<SolverType>::insertPhase(std::function<void(unsigned int)> phase){

    // add the function to the "phases" vector
    phases.emplace_back(phase);

    // increment the number of control points
    (cp.ncp)++;

    // check consistency
    if (cp.ncp != phases.size()){
        Report::error("Phasing!",
                      "The number of phases and the number of control points are inconsistent");
    }
}


// Assigns the parent domain of a child domain.
template <class SolverType>
bool Domain<SolverType>::setParent(std::shared_ptr<Domain<SolverType>> pd){

    // check if a parent domain is assigned before:
    if (parent){
        Report::error("Domain-setParent",
                      "Parent Domain of "+id+" is already set!");
    }

    // assign the parent domain:
    parent = pd;

    // set hierarchySet to true
    hierarchySet = true;

    return true;
}


// Checks if the domain is ready to execute the next phase
template <class SolverType>
void Domain<SolverType>::phaseCheck(){

    if (isParent()){
        std::unique_lock<std::mutex> parentLock(*mtx);
        condition->wait(parentLock, [&]{
            for (auto &childCPref: childCPs){
                auto &childCP = childCPref.get();
                if ( (cp.ncp + cp.getVal() - childCP.getVal())%cp.ncp > 0 ) return false;
            }
            return true;
        });

        cp.increment();
        condition4children->notify_all();
        threadPool->acquire();
    }
    else{ // child
        std::unique_lock<std::mutex> childLock(*mtx);
        condition->wait(childLock, [&]{
            if ( ( (cp.ncp + parent->cp.getVal() - cp.getVal())%cp.ncp > 1 ) or
                 ( (cp.ncp + parent->cp.getVal() - cp.getVal())%cp.ncp == 1 and parent->cp.isDone() ) ){
                return true;
            }
            return false;
        });
        cp.increment();
        parent->condition->notify_one();
        threadPool->acquire();
    }

}


// Signal phase completion
template <class SolverType>
void Domain<SolverType>::completePhase(){

    threadPool->release();
    std::unique_lock<std::mutex> lock(*mtx);
    cp.markDone();

    if (isParent()){
        condition4children->notify_all();
    }
    else{ // child
        parent->condition->notify_one();
    }

}

// Configures hierarchy of a domain within a project
template <class SolverType>
void Domain<SolverType>::setHierarchy(std::shared_ptr<Domain> parent){

    if (not (parent==nullptr)){
        parent->addChild(this->shared_from_this());
    }

    hierarchySet = true;

    Report::log("Child: "+getID()+"  Parent: "+parent->getID(),3);
}

// Configures domain concurrency constructs, e.g., mutexes and condition variables
// of provides child domains with pointers to these constructs.
template <class SolverType>
void Domain<SolverType>::setConcurrency(unsigned nProc){

    if (not hierarchyIsSet()){
        Report::error("Domain Concurrency Configuration",
                      "Domain hierarchy is not set yet.");
    }

    if (isParent()){
        threadPool          = std::make_shared<Threading::Pool>(nProc);
        mtx                 = std::make_shared<std::mutex>();
        condition           = std::make_shared<std::condition_variable>();
        condition4children  = std::make_shared<std::condition_variable>();
    }
    else{
        threadPool = parent->threadPool;
        mtx        = parent->mtx;
        condition  = parent->condition4children;
        parent->childCPs.push_back(std::ref(cp));
    }

}


// attribute accessors:

template <class SolverType>
bool Domain<SolverType>::isParent()const{

    // Check if hierarchy is assigned already
    if (not hierarchySet)
        Report::error("Domain-isChild()", "Hierarchy of "+id+" is not set yet.");

    if (parent) return false;
    return true;

}

template <class SolverType>
bool Domain<SolverType>::isChild()const{

    // Check if hierarchy is assigned already
    if (not hierarchySet)
        Report::error("Domain-isChild()", "Hierarchy of "+id+" is not set yet.");

    if (parent) return true;
    return false;

}

template <class SolverType>
bool Domain<SolverType>::isInitialized()const{
    return initialized;
}

template <class SolverType>
bool Domain<SolverType>::hierarchyIsSet()const{
    return hierarchySet;
}

template <class SolverType>
std::string Domain<SolverType>::getID()const{
    return id;
}

template <class SolverType>
std::string Domain<SolverType>::getOutputDir()const{
    return outputDir;
}

template <class SolverType>
const std::shared_ptr<SolverType> &Domain<SolverType>::getSolver()const{
    return solver;
}

template <class SolverType>
std::shared_ptr<Domain<SolverType>> Domain<SolverType>::getParent(){
    return parent;
}

template <class SolverType>
short Domain<SolverType>::get_nPhases(){
    return short(phases.size());
}

template <class SolverType>
int Domain<SolverType>::get_nChild()const{
    return childDomains.size();
}

template <class SolverType>
std::shared_ptr<Domain<SolverType>> Domain<SolverType>::getChild(unsigned int i){

    if ( i>=childDomains.size() ){
        std::stringstream s;
        s << i;
        Report::error("Domain! "+id,
                      "Child domain index "+s.str()+" is invalid");
    }

    std::shared_ptr<Domain<SolverType>> childDomain_i = (childDomains[i]).lock();
    return childDomain_i;
}

} // end of namespace OpenHDM

#endif // DOMAIN_H
