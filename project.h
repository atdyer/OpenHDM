/*
 * OpenHDM: An Open-Source Sofware Framework for Hydrodynamic Models
 * Copyright (C) 2015-2017 
 * Alper Altuntas <alperaltuntas@gmail.com>
 *
 * This file is part of OpenHDM.
 *
 * OpenHDM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenHDM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenHDM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PROJECT_H
#define PROJECT_H

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>
#include <sstream>

#include "threading.h"
#include "projectinput.h"
#include "report.h"

/*
 *  "Project" class template is responsible for managing the domains of the project to
 *  be simulated. The tasks of the Project class includes instantiating domains,
 *  constructing domain hierarchy, performing the timestepping process, and regulating
 *  input/output procedures of the simulation. Constructor of the Project class template
 *  requires a projectInput object as an argument to instantiate the project.
 */

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
    ~Project();

    // parallel run:
    void run(unsigned int nProcs=0);

private:

    // runtime:
    void initializeRun();
    void processTimesteppingParams();
    void timestepping();                    // serial timestepping
    void timestepping(unsigned int nProcs); // parallel timestepping
    void finalizeRun();

    // domain management:
    void addDomain(const std::shared_ptr<domainClass> &domain);
    void removeDomain(const std::string domainID);
    void setDomainHierarchy();
    void processDomainsList(ProjectInput &projectInput);
    unsigned int getDomainPosition(const std::string domainID);

    // attribute accessor functions:
    size_t      nd() const;
    bool        domainID_isAvailable(std::string newDomainID);
    bool        outputDir_isAvailable(std::string newOutputDir);
    std::string getProjectID() const;
    std::shared_ptr<domainClass> getDomain(std::string domainID);

    // input parameters:
    std::string projectID = "";

    // internal members:
    unsigned int    nts = 0;        // total no. of timesteps
    unsigned short  nPhases = 0;    // no. of phases at a timestep.
    std::vector<std::shared_ptr<domainClass>> domains;
    std::map<std::string,std::string> hierarchyTable;


    // multithreading:
    void configThreadingHierarchy(unsigned int nProcs);
    void check_nProcs(unsigned int &nProcs);
    void check_multipleParents();
    std::vector<std::thread> threads;

};

// Project Template Member Function Implementations:

// Project constructor
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


template <class domainClass>
Project<domainClass>::~Project()
{

}


// Performs the serial|parallel simulation of all domains in the project.
template <class domainClass>
void Project<domainClass>::run(unsigned int nProcs){

    // 1. Initialize:
    Report::log("Run is initializing:",1);
    initializeRun();

    // 2. Timestepping:
    if (nProcs<2)   timestepping();         // serial
    else            timestepping(nProcs);   // parallel

    // 3. Finalize:
    Report::log("\n  Run is finalizing:",1);
    finalizeRun();

}


// Prepares the domains of the project for timestepping procedure. The initialization
// includes instantiating domain grids, solvers, and outputs.
// This function is called in Project<>::run()
template <class domainClass>
void Project<domainClass>::initializeRun(){

    // Construct the domain hierarchy:
    setDomainHierarchy();

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


// Serial Timestepping Routine
template <class domainClass>
void Project<domainClass>::timestepping(){

    Report::log("Serial timestepping is starting...",1);

    for (unsigned int ts=1; ts<=nts; ts++){

        // Print timestep
        if(ts>99 and ts%(nts/100)==0) std::cout << "\r \t %" << ts*100/nts << std::flush;

        // Sequentially execute the phase functions of domains.
        for (int p=0; p<nPhases; p++){
            for (auto &domain : domains){
                domain->phases[p](ts);
            }
        }

    }// timestepping loop

}

// Parallel Timestepping Routine
template <class domainClass>
void Project<domainClass>::timestepping(unsigned int nProcs){

    Report::log("Parallel timestepping is starting...",1);

    configThreadingHierarchy(nProcs);

    // Instantiate and execute the threads:
    for (auto &domain: domains){
        threads.emplace_back( [&](){
            Report::log("Executing the domain "+domain->getID(),1);

            for (unsigned int ts=1; ts<=nts; ts++){
                for (int p=0; p<nPhases; p++){

                    // check if ready to execute the phase
                    domain->phaseCheck();

                    // execute the phase
                    domain->phases[p](ts);

                    // signal phase completion
                    domain->completePhase();

                }// phase loop
            }// timestepping loop

        });
    }


    // Join and destruct the threads:
    for (auto &thread: threads){
            thread.join();
    }

}


// Post-processes the run
template <class domainClass>
void Project<domainClass>::finalizeRun(){

    // Post-processing"
    Report::log("Post-processing domains...",2);

    for (auto &domain : domains){
        domain->postProcess();
    }

    Report::log("Run has finished.",2);
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


// Constructs the domain hierarchy
template <class domainClass>
void Project<domainClass>::setDomainHierarchy(){

    Report::log("Constructing domain hierarchy",2);

    for (auto &domain : domains){

        std::string domainID = domain->getID();
        if (hierarchyTable.find(domainID) != hierarchyTable.end() ){

            std::string parentID = hierarchyTable[domainID];
            Report::log("Child: "+domainID+"  Parent: "+parentID,3);
            std::shared_ptr<domainClass> child = getDomain(domainID);
            std::shared_ptr<domainClass> parent = getDomain(parentID);

            // Add the child to childDomains vector of the parent domain:
            parent->addChild(child);

        }
        domain->hierarchySet = true;
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


// Removes a given domain from the project
template <class domainClass>
void Project<domainClass>::removeDomain(const std::string domainID){

    unsigned int pos = getDomainPosition(domainID);
    domains.erase(domains.begin()+pos);
}


// Returns the position of a domain in domains vector of the project
template <class domainClass>
unsigned int Project<domainClass>::getDomainPosition(const std::string domainID){

    std::shared_ptr<domainClass> domain = getDomain(domainID);

    auto domainIt = std::find(domains.begin(), domains.end(), domain);
    unsigned int pos = std::distance( domains.begin(),domainIt );
    return pos;

}



template <class domainClass>
void Project<domainClass>::configThreadingHierarchy(unsigned int nProcs){

    // Check nProcs and update if necessary
    check_nProcs(nProcs);

    // Check if there are multiple parent domains
    check_multipleParents();

    // Initialize the thread pool and the mutex of the parent domain,
    // fill in the vector of references to child domain cp's,
    // and assign the corresponding pointers of child domains
    for (auto &domain : domains){
        if (domain->isParent()){
            domain->threadPool          = std::make_shared<Threading::Pool>(nProcs);
            domain->mtx                 = std::make_shared<std::mutex>();
            domain->condition           = std::make_shared<std::condition_variable>();
            domain->condition4children  = std::make_shared<std::condition_variable>();


            for (auto &childDomainWP: domain->childDomains){
                auto childDomain = childDomainWP.lock();
                childDomain->threadPool = domain->threadPool;
                childDomain->mtx        = domain->mtx;
                childDomain->condition  = domain->condition4children;

                domain->childCPs.push_back(std::ref(childDomain->cp));
            }
        }
    }

}


// Check if nProcs is greater than the number of threads available.
// If so, set it to (available threads)-1.
template <class domainClass>
void Project<domainClass>::check_nProcs(unsigned int &nProcs){

    if (nProcs>std::thread::hardware_concurrency()){
        Report::warning("Concurrency!","Number of processors specified in CL ="
                        +std::to_string(nProcs)+" is greater than the number of available"
                        " threads ="+std::to_string(std::thread::hardware_concurrency())
                        +"\n\t Setting number of processors to "
                        +std::to_string(std::thread::hardware_concurrency()-1),1);
        nProcs = std::thread::hardware_concurrency()-1;
    }

}




// Check if nProcs is greater than the number of threads available.
// If so, set it to (available threads)-1.
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



#endif // PROJECT_H
