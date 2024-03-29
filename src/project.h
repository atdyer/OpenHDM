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

#ifndef PROJECT_H
#define PROJECT_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>
#include <sstream>
#include "projectinput.h"
#include "report.h"

namespace OpenHDM {

// --------------------------------------------------------------------
// Project: The class template Project is the main driver of concurrent
//   simulations. It can own multiple domain instances that can be
//   simulated concurrently. The implementation of this class template
//   is complete and requires no user modification or derivation. For
//   each concurrent execution, a single Project instance is created,
//   which is responsible for coordinating the domains and the
//   timestepping routine.
// --------------------------------------------------------------------

template <class domainClass>
class Project
{
public:

    // Constructors & Operators
    Project(ProjectInput &projectInput);
    Project(const Project&)             = delete;
    Project& operator=(const Project&)  = delete;
    Project(Project&&)                  = delete;
    Project& operator=(Project&&)       = delete;
    ~Project(){}

    void run(unsigned nProcTotal=0, unsigned nProcChild=0);

private:

    // Runtime:
    void initializeRun(unsigned nProcTotal, unsigned nProcChild);
    void processTimesteppingParams();
    void initiateTimestepping();
    void finalizeRun();

    // Domain Management:
    void addDomain(const std::shared_ptr<domainClass> &domain);
    void removeDomain(const std::string domainID);
    void setDomainHierarchy();
    void processDomainsList(ProjectInput &projectInput);
    unsigned getDomainPosition(const std::string domainID);

    // Inter-Domain Concurrency:
    void setDomainConcurrency(unsigned nProcTotal, unsigned nProcChild);
    void check_nProc(unsigned &nProcTotal, unsigned &nProcChild);
    void check_multipleParents();
    std::vector<std::thread> threads;

    // get functions:
    size_t      nd() const;
    bool        domainID_isAvailable(std::string newDomainID);
    bool        outputDir_isAvailable(std::string newOutputDir);
    std::string getProjectID() const;
    std::shared_ptr<domainClass> getDomain(std::string domainID);

    // internal data members:
    unsigned        nts         = 0;    // total no. of timesteps
    unsigned short  nPhases     = 0;    // no. of phases at a timestep.
    std::string     projectID   = "";
    std::vector<std::shared_ptr<domainClass>> domains;
    std::map<std::string,std::string> hierarchyTable;

};

// Project Template Member Function Implementations:

// Project constructor:
template <class domainClass>
Project<domainClass>::Project(ProjectInput &projectInput):
    projectID(projectInput.projectID)
{

    Report::log("Project "+projectID+" is initializing",0);

    // Construct the domains that are predefined in the project input file:
    if (projectInput.nd > 0){
        Report::log("Constructing domains listed in "+projectInput.getFileTitle()+":",1);
        processDomainsList(projectInput);
    }

}


// Performs the serial|parallel execution of a project:
template <class domainClass>
void Project<domainClass>::run(unsigned nProcTotal, unsigned nProcChild){

    // 1. Initialize:
    Report::log("Run is initializing:",1);
    initializeRun(nProcTotal, nProcChild);

    // 2. Timestepping:
    initiateTimestepping();

    // 3. Finalize:
    Report::log("\n Run is finalizing:",1);
    finalizeRun();

}


// Prepares the domains of the project for timestepping procedure. The initialization
// includes configuring domain hierarchy and concurrency, instantiating grids, solvers,
// outputs, reading inputs, etc. This function is called in Project<>::run()
template <class domainClass>
void Project<domainClass>::initializeRun(unsigned nProcTotal, unsigned nProcChild){

    // Construct the domain hierarchy:
    setDomainHierarchy();

    // Configure multithreaded domain concurrency:
    setDomainConcurrency(nProcTotal,nProcChild);

    // Lazy initialization of members such as solvers, grids, outputs, etc.
    Report::log("Setting up the simulation",2);
    for (auto &domain : domains){
        domain->instantiateMembers();
    }

    // Read inputs
    Report::log("Reading domain inputs",2);
    for (auto &domain : domains){
        domain->readInputs();
    }

    // Complete the domain initializations before timestepping begins;
    Report::log("Completing domain initializations",2);
    for (auto &domain : domains){
        domain->initialize();
    }

    // For each domain, obtain and compare timestepping params; nts and nPhases.
    processTimesteppingParams();

}


// Instantiates a timestepping thread for each domain. The concurrent execution of these
// threads depend on available processors and relative progression of parent/children.
// (See phasing mechanism.)
template <class domainClass>
void Project<domainClass>::initiateTimestepping(){

    // Instantiate and execute timestepping thread for each domain:
    for (auto &domain: domains){

        threads.emplace_back( [&](){
            domain->timestepping(nts);
        });
    }

    // Join and destruct the threads:
    for (auto &thread: threads){
            thread.join();
    }

}


// Post-processes the domains of the project:
template <class domainClass>
void Project<domainClass>::finalizeRun(){

    // Post-processing"
    Report::log("Post-processing domains...",2);

    for (auto &domain : domains){
        domain->postProcess();
    }

    Report::log("Run has finished.",2);
}


// Adds an instantiated domain to domains list of the project
template <class domainClass>
void Project<domainClass>::addDomain(const std::shared_ptr<domainClass> &domain){

    // Check if domainID was used before:
    std::string newDomainID = domain->getID();
    if ( not domainID_isAvailable(newDomainID) ){
        Report::error("Domain ID!",
                      "Domain ID " + newDomainID + " is used multiple times.");
    }

    // Check if outputDir was used before:
    std::string newOutputDir = domain->getOutputDir();
    if ( not outputDir_isAvailable(newOutputDir) ){
        Report::error("Output Directory!",
                       "Output directory " + newOutputDir + " is used multiple times.");
    }

    // add the domain to the project:
    domains.push_back(domain);

}


// Removes a given domain from the project
template <class domainClass>
void Project<domainClass>::removeDomain(const std::string domainID){

    unsigned pos = getDomainPosition(domainID);
    domains.erase(domains.begin()+pos);
}


// Constructs the domain hierarchy
template <class domainClass>
void Project<domainClass>::setDomainHierarchy(){

    Report::log("Constructing domain hierarchy",2);

    for (auto &domain : domains){
        std::string domainID = domain->getID();

        if (hierarchyTable.find(domainID) != hierarchyTable.end() ){

            // Get a pointer to parent domain:
            std::string parentID = hierarchyTable[domainID];
            std::shared_ptr<domainClass> parent = getDomain(parentID);

            // Set child domain hierarchy:
            domain->setHierarchy(parent);
        }
        else{

            // Set parent domain hierarchy:
            domain->setHierarchy();
        }
    }

}


// Reads predefined domains from a project input file, instantiates the domains,
// and adds them to the project
template <class domainClass>
void Project<domainClass>::processDomainsList(ProjectInput &projectInput){

    // Number of domains defined in the project input file:
    size_t ndpi = projectInput.domainsList.size();

    // Check if number of domains in projectInput is consistent
    if (projectInput.nd != ndpi){
        Report::error("Project Input!",
                      "Number of domains set in projectInput file is not equal to"
                       "the number of domains that are defined.");
    }

    for (auto &row : projectInput.domainsList){

        // If the domain is a child domain, set its parent id;
        if (row.parentID != ""){
            std::shared_ptr<domainClass> parent = getDomain(row.parentID);
            if (!parent){
                Report::error("Parent Domain!",
                              "Parent domain "+row.parentID+" of child domain "
                              +row.domainID+" is not initialized yet. Ensure that "+row.parentID+
                              " is declared before "+row.domainID);
            }
            hierarchyTable[row.domainID] = row.parentID;
        }

        // Instantiate the domain:
        auto domain = std::make_shared<domainClass>(row.domainID,
                                                    row.domainPath,
                                                    row.outputDir);

        // Add the domain to project
        addDomain(domain);
    }

}


// Configures inter-domain concurrency
template <class domainClass>
void Project<domainClass>::setDomainConcurrency(unsigned nProcTotal, unsigned nProcChild){

    // Check nProc and update if necessary
    check_nProc(nProcTotal, nProcChild);

    // Check if there are multiple parent domains
    check_multipleParents();

    // Determine number of processors to be dedicated to inter- and intra-domain
    // parallelism. Initialize the thread pool and the mutex of the parent domain,
    // and provide child domains with pointers to concurrency constructs.
    for (auto &domain : domains){
        if (domain->isParent()){

            // Configure parent domain concurrency settings:
            domain->setConcurrency(nProcTotal, nProcChild);

            // Configure child domains' concurrency settings:
            for (auto &childWP: domain->childDomains){
                auto child = childWP.lock();
                child->setConcurrency();
            }
        }
    }
}


// Sets the number of timesteps and number of phases of a project. Ensures that these
// parameters are same for all domains. This function must be called after the domains
// are initialized.
template <class domainClass>
void Project<domainClass>::processTimesteppingParams(){

    if (not (nd()>0) ){
        Report::error("Timestepping Parameters",
                      "The project has no domains instantiated.");
    }

    // Get the number of timesteps and number of phases of the first domain
    nts = domains[0]->get_nts();
    nPhases = domains[0]->get_nPhases();

    // Ensure that nts and nPhases are same for all domains
    for (auto &domain : domains){
        if (domain->get_nts() != nts){
            Report::error("Timestepping Parameters",
                          "nts of "+domain->getID()+"is not the same as the previous domain(s).");
        }
        if (domain->get_nPhases() != nPhases){
            Report::error("Timestepping Parameters",
                          "nPhases of "+domain->getID()+"is not the same as the previous domain(s).");
        }
    }

}


// Returns the position of a domain in domains vector of the project
template <class domainClass>
unsigned Project<domainClass>::getDomainPosition(const std::string domainID){

    std::shared_ptr<domainClass> domain = getDomain(domainID);

    auto domainIt = std::find(domains.begin(), domains.end(), domain);
    unsigned pos = std::distance( domains.begin(),domainIt );
    return pos;

}


// Checks if nProc is greater than the number of threads available.
// If so, sets it to (available threads)-1.
template <class domainClass>
void Project<domainClass>::check_nProc(unsigned &nProcTotal, unsigned &nProcChild){

    if (nProcTotal>std::thread::hardware_concurrency()){
        Report::warning("Concurrency!","Number of processors specified in CL ="
                        +std::to_string(nProcTotal)+" is greater than the number of available"
                        " threads ="+std::to_string(std::thread::hardware_concurrency())
                        +"\n\t Setting number of processors to "
                        +std::to_string(std::thread::hardware_concurrency()-1),1);
        nProcTotal = std::thread::hardware_concurrency()-1;
    }

    if (nProcChild>=nProcTotal){
        Report::warning("Concurrency!","Number of processors allocated for children must be "
                        "less than the number of total processors ="
                        +std::to_string(nProcTotal)
                        +"\n\t Setting number of processors for children to "
                        +std::to_string(nProcTotal-1),1);
        nProcChild = nProcTotal-1;
    }

    if (domains.size()==0 and nProcChild>0){
        Report::warning("Concurrency!","Number of processors allocated for children is greater than "
                        "zero but there are not any child domains."
                        "\n\t Setting number of processors for children to zero.");
        nProcChild = 0;
    }
}


// Checks if there are multiple domains.
template <class domainClass>
void Project<domainClass>::check_multipleParents(){

    int nParents = 0;
    for (auto &domain : domains){
        nParents += domain->isParent();
    }

    if (nParents>1){
        Report::error("Concurrency!",
                  "Only one parent domain can be executed during parallel runs");
    }
}


// attribute accessors:

template <class domainClass>
std::string Project<domainClass>::getProjectID() const{
    return projectID;
}

template <class domainClass>
std::shared_ptr<domainClass> Project<domainClass>::getDomain(std::string domainID){

    for (auto &domain : domains){
        if (domain->getID()==domainID)  return domain;
    }

    return nullptr;
}

template <class domainClass>
size_t Project<domainClass>::nd() const{
    return domains.size();
}

template <class domainClass>
bool Project<domainClass>::domainID_isAvailable(std::string newDomainID){

    for (auto &domain : domains){
        if (domain->getID()==newDomainID)  return false;
    }
    return true;
}

template <class domainClass>
bool Project<domainClass>::outputDir_isAvailable(std::string newOutputDir){

    for (auto &domain : domains){
        if (domain->getOutputDir()==newOutputDir)  return false;
    }
    return true;
}

} // end of namespace OpenHDM

#endif // PROJECT_H
