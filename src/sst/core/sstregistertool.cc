// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/env/envconfig.h"
#include "sst/core/env/envquery.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// Function declarations
static void              print_usage();
void                     sstRegister(char* argv[]);
void                     sstUnregister(const std::string& element);
std::vector<std::string> listModels(int option);
void                     sstUnregisterMultiple(std::vector<std::string> elementsArray);
void                     autoUnregister();
bool                     validModel(const std::string& s);

// Global constants
const std::string START_DELIMITER = "[";
const std::string STOP_DELIMITER  = "]";
// Global path to configuration file
char*             cfgPath;

int
main(int argc, char* argv[])
{
    int                      option = 0;
    std::vector<std::string> elementsArray;
    cfgPath = (char*)malloc(sizeof(char) * PATH_MAX);

    if ( argc < 2 ) {
        print_usage();
        exit(-1);
    }

    // Check for configuration file
    sprintf(cfgPath, SST_INSTALL_PREFIX "/etc/sst/sstsimulator.conf");
    FILE* cfgFile = fopen(cfgPath, "r+");
    if ( nullptr == cfgFile ) {
        char* envHome = getenv("HOME");

        if ( nullptr == envHome ) { sprintf(cfgPath, "~/.sst/sstsimulator.conf"); }
        else {
            sprintf(cfgPath, "%s/.sst/sstsimulator.conf", envHome);
        }

        cfgFile = fopen(cfgPath, "r+");

        if ( nullptr == cfgFile ) {
            fprintf(
                stderr, "Unable to open configuration at either: %s or %s, one of these files must be editable.\n",
                SST_INSTALL_PREFIX "/etc/sst/sstsimulator.conf", cfgPath);
            exit(-1);
        }
    }
    fclose(cfgFile);

    if ( !strcmp(argv[1], "-u") ) { // Unregister
        std::string element = argv[2];
        sstUnregister(element);
    }
    else if ( !strcmp(argv[1], "-l") ) { // List all of the registered components
        std::cout << "\nA model labeled INVALID means it is registered in\n";
        std::cout << "SST, but no longer exists in the specified path.\n";
        listModels(option); // if param = 0, listModels function does NOT put elements in array
    }
    else if ( !strcmp(argv[1], "-m") ) { // unregister multiple components
        std::cout
            << "\nChoose which models you would like to unregister. \nSeparate your choices with a space. Ex: 1 2 3\n";
        std::cout << "Note: This does not delete the model files.\n";
        elementsArray = listModels(option + 1); // if param = 1, listModels puts elements in array
        sstUnregisterMultiple(elementsArray);
    }
    else if ( !strcmp(argv[1], "-au") ) { // auto-unregister invalid components
        autoUnregister();
    }
    else if ( !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help") )
        print_usage();
    else
        sstRegister(argv);

    free(cfgPath);
    return 0;
}

// sstRegister
// Registers a model with SST. Puts model name and location in the sst config file
// Input: char pointer to the command line arguments
void
sstRegister(char* argv[])
{
    std::string groupName(argv[1]);
    std::string keyValPair(argv[2]);

    size_t equalsIndex = 0;

    for ( equalsIndex = 0; equalsIndex < keyValPair.size(); equalsIndex++ ) {
        if ( '=' == argv[2][equalsIndex] ) { break; }
    }

    std::string key   = keyValPair.substr(0, equalsIndex);
    std::string value = keyValPair.substr(equalsIndex + 1);

    SST::Core::Environment::EnvironmentConfiguration* database = new SST::Core::Environment::EnvironmentConfiguration();

    FILE* cfgFile = fopen(cfgPath, "r+");
    populateEnvironmentConfig(cfgFile, database, true);

    database->getGroupByName(groupName)->setValue(key, value);
    fclose(cfgFile);

    cfgFile = fopen(cfgPath, "w+");

    if ( nullptr == cfgFile ) {
        fprintf(stderr, "Unable to open: %s for writing.\n", cfgPath);
        exit(-1);
    }

    database->writeTo(cfgFile);

    fclose(cfgFile);
}

// sstUnregister
// Takes a string argument and searches the sstsimulator config file for that name.
// Removes the component from the file - unregistering it from SST
// Input: command line arguments
void
sstUnregister(const std::string& element)
{
    std::string str1;
    std::string s = "";
    std::string tempfile;
    int         found = 0;

    // setup element names to look for
    str1     = START_DELIMITER + element + STOP_DELIMITER;
    tempfile = "/tmp/sstsimulator.conf";

    std::ifstream infile(cfgPath);
    std::ofstream outfile(tempfile);

    // grab each line and compare to element name stored in str1
    // if not the same, then print the line to the temp file.
    while ( getline(infile, s) ) {
        if ( str1 == s ) {
            found = 1;
            // Grab the _LIBDIR= line to remove it from config
            getline(infile, s);
        }
        else
            outfile << s << "\n";
    }

    if ( found ) { std::cout << "\tModel " << element << " has been unregistered!\n"; }
    else
        std::cout << "Model " << element << " not found\n\n";

    infile.close();
    outfile.close();
    rename(tempfile.c_str(), cfgPath);
}

// listModels
// Prints to STDOUT all of the registered models
// Input: an int option that determines what will be returned:
//    option == 0: nullptr vector
//    option == 1: a vector containing all of the registered components (both valid and invalid)
//    option == 2: a vector containing only the INVALID components
// Returns: a vector of strings.
std::vector<std::string>
listModels(int option)
{
    std::string              s = "";
    std::string              strNew;
    std::vector<std::string> elements;
    int                      found = 0, count = 1;

    std::ifstream infile(cfgPath);

    // Begin search of sstconf for models
    std::cout << "\nList of registered models:\n";
    while ( getline(infile, s) ) {
        std::size_t first = s.find(START_DELIMITER);

        if ( first != std::string::npos ) {
            std::size_t last = s.find(STOP_DELIMITER);
            strNew           = s.substr(first + 1, last - (first + 1)); // The +1 removes the brackets from substring
            // disregard SSTCore and default
            if ( strNew != "SSTCore" && strNew != "default" ) {
                found = 1;
                // check if the model is valid by confirming it is located in the path registered in the sst config file
                getline(infile, s); // grab the next line containing the model location

                if ( s.find("/") != std::string::npos ) { // check to see if there is a path
                    if ( validModel(s) ) {
                        std::cout << count << ". " << std::setw(25) << std::left << strNew << "VALID" << std::endl;
                    }
                    else {
                        std::cout << count << ". " << std::setw(25) << std::left << strNew << "INVALID" << std::endl;
                        if ( option == 2 ) // if option = 2, then we only push the invalid models into the vector
                            elements.push_back(strNew);
                    }

                    if ( option == 1 ) // if option = 1, then we push ALL of the models to the vector
                        elements.push_back(strNew);
                    count++;
                }
            }
        }
    }

    if ( !found ) std::cout << "No models registered\n";
    std::cout << "\n";
    infile.close();

    return elements;
}

// sstUnregisterMultiple
// Lists the registered models and gives the user the
// option to choose multiple models to unregister.
// Input: a vector of strings
void
sstUnregisterMultiple(std::vector<std::string> elementsArray)
{
    unsigned         temp;
    std::vector<int> elementsToRemove;
    std::string      line;

    if ( elementsArray.size() != 0 ) {
        std::cout << ">";
        getline(std::cin, line);
        std::stringstream ss(line);
        // push the options chosen to a vector
        while ( ss >> temp ) {
            // Check for valid inputs
            if ( temp > elementsArray.size() ) {
                std::cerr << "\nError: A number you entered is not in the list.\n";
                exit(-1);
            }
            elementsToRemove.push_back(temp);
        }
        // go through the vector of items to be removed and unregister them
        for ( unsigned i = 0; i < elementsToRemove.size(); i++ )
            sstUnregister(elementsArray[elementsToRemove[i] - 1]); //-1 because our displayed list starts at 1 and not 0
    }
    else
        std::cout << "Nothing to unregister.\n\n";
}

// validModel
// Checks the path of the model to determine if it physically exists on the drive
// Input: a string containing the path
// Returns: a true or false
bool
validModel(const std::string& s)
{
    std::size_t locationStart = s.find("/");
    std::string str1          = s.substr(locationStart); // grabs the rest of the line from / to the end
    char*       path          = new char[str1.length() + 1];
    std::strcpy(path, str1.c_str());

    struct stat statbuf;
    if ( stat(path, &statbuf) != -1 ) {
        delete[] path;
        if ( S_ISDIR(statbuf.st_mode) ) return true;
    }

    return false;
}

// autoUnregister
// Unregisters all INVALID components from the SST config file
// Input: none
// Returns: none
void
autoUnregister()
{
    std::vector<std::string> elementsArray =
        listModels(2); // passes 2 to listModels to tell the function to only store INVALID models
    for ( unsigned i = 0; i < elementsArray.size(); i++ ) {
        sstUnregister(elementsArray[i]);
    }
}

// print_usage
// Displays proper syntax to be used when running the feature
// Input: none
// Returns: none
void
print_usage()
{
    std::cout << "To register a component:\n";
    std::cout << "\nsst-register <Dependency Name> (<VAR>=<VALUE>)*\n";
    std::cout << "\n";
    std::cout << "<Dependency Name>   : Name of the Third Party Dependency\n";
    std::cout << "<VAR>=<VALUE>       : Configuration variable and associated value to add to registry.\n";
    std::cout << "                      If <VAR>=<VALUE> pairs are not provided, the tool will attempt\n";
    std::cout << "                      to auto-register $PWD/include and $PWD/lib to the name\n";
    std::cout << "\n";
    std::cout << "                      Example: sst-register DRAMSim CPPFLAGS=\"-I$PWD/include\"\n";
    std::cout << "\n";
    std::cout << "To unregister a known component:\tsst-register -u <component name>\n";
    std::cout << "To list all registered components:\tsst-register -l\n";
    std::cout << "To choose components to unregister:\tsst-register -m\n\n";
    std::cout << "Unregister all INVALID components:\tsst-register -au\n\n";
}
