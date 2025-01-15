// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/impl/interactive/simpleDebug.h"

#include "sst/core/stringize.h"
#include "sst/core/timeConverter.h"

namespace SST {
namespace IMPL {
namespace Interactive {

SimpleDebugger::SimpleDebugger(Params& UNUSED(params)) : InteractiveConsole() {}

void
SimpleDebugger::execute(const std::string& msg)
{
    printf("Entering interactive mode at time %" PRI_SIMTIME " \n", getCurrentSimCycle());
    printf("%s\n", msg.c_str());
    if ( nullptr == obj_ ) { obj_ = getComponentObjectMap(); }
    done = false;

    std::string line;
    while ( !done ) {
        printf("> ");
        std::getline(std::cin, line);
        dispatch_cmd(line);
    }
}


// Functions for the Explorer

std::vector<std::string>
SimpleDebugger::tokenize(std::vector<std::string>& tokens, const std::string& input)
{
    std::istringstream iss(input);
    std::string        token;

    while ( iss >> token ) {
        tokens.push_back(token);
    }

    return tokens;
}

void
SimpleDebugger::cmd_pwd(std::vector<std::string>& UNUSED(tokens))
{
    // std::string path = obj_->getName();
    // std::string slash("/");
    // // path = slash + path;
    // SST::Core::Serialization::ObjectMap* curr = obj_->getParent();
    // while ( curr != nullptr ) {
    //     path = curr->getName() + slash + path;
    //     curr = curr->getParent();
    // }

    printf("%s (%s)\n", obj_->getFullName().c_str(), obj_->getType().c_str());
}

void
SimpleDebugger::cmd_ls(std::vector<std::string>& UNUSED(tokens))
{
    std::vector<std::pair<std::string, SST::Core::Serialization::ObjectMap*>> vars = obj_->getVariables();
    for ( auto& x : vars ) {
        if ( x.second->isFundamental() ) {
            printf("%s = %s (%s)\n", x.first.c_str(), x.second->get().c_str(), x.second->getType().c_str());
        }
        else {
            printf("%s/ (%s)\n", x.first.c_str(), x.second->getType().c_str());
        }
    }
}

void
SimpleDebugger::cmd_cd(std::vector<std::string>& tokens)
{
    if ( tokens.size() != 2 ) {
        printf("Invalid format for cd command (cd <obj>)\n");
        return;
    }

    // Check for ..
    if ( tokens[1] == ".." ) {
        auto* parent = obj_->selectParent();
        if ( parent == nullptr ) {
            printf("Already at top of object hierarchy\n");
            return;
        }
        obj_ = parent;
        return;
    }

    bool                                 loop_detected = false;
    SST::Core::Serialization::ObjectMap* new_obj       = obj_->selectVariable(tokens[1], loop_detected);

    if ( !new_obj ) {
        printf("Unknown object in cd command: %s\n", tokens[1].c_str());
        return;
    }

    if ( new_obj->isFundamental() ) {
        printf("Object %s is a fundamental type so you cannot cd into it\n", tokens[1].c_str());
        new_obj->selectParent();
        return;
    }

    if ( loop_detected ) {
        printf(
            "Loop detected in cd.  New working directory will be set to level of looped object: %s\n",
            new_obj->getFullName().c_str());
    }
    obj_ = new_obj;
}

void
SimpleDebugger::cmd_print(std::vector<std::string>& tokens)
{
    // Index in tokens array where we may find the variable name
    size_t var_index = 1;

    if ( tokens.size() < 2 ) {
        printf("Invalid format for print command (print [-rN] [<obj>])\n");
        return;
    }

    // See if have a -r or not
    int         recurse = 0;
    std::string tok     = tokens[1];
    if ( tok.size() >= 2 && tok[0] == '-' && tok[1] == 'r' ) {
        // Got a -r
        std::string num = tok.substr(2);
        if ( num.size() != 0 ) {
            try {
                recurse = SST::Core::from_string<int>(num);
            }
            catch ( std::invalid_argument& e ) {
                printf("Invalid number format specified with -r: %s\n", tok.c_str());
                return;
            }
        }
        else {
            recurse = 4; // default -r depth
        }

        var_index = 2;
    }

    if ( tokens.size() == var_index ) {
        // Print current object
        obj_->list(recurse);
        return;
    }

    if ( tokens.size() != (var_index + 1) ) {
        printf("Invalid format for print command (print [-rN] [<obj>])\n");
        return;
    }

    bool        found;
    std::string listing = obj_->listVariable(tokens[var_index], found, recurse);

    if ( !found ) {
        printf("Unknown object in print command: %s\n", tokens[1].c_str());
        return;
    }
    else {
        printf("%s", listing.c_str());
    }
}


void
SimpleDebugger::cmd_set(std::vector<std::string>& tokens)
{
    if ( tokens.size() != 3 ) {
        printf("Invalid format for set command (set <obj> <value>)\n");
        return;
    }

    if ( obj_->isContainer() ) {
        bool found     = false;
        bool read_only = false;
        obj_->set(tokens[1], tokens[2], found, read_only);
        if ( !found ) printf("Unknown object in set command: %s\n", tokens[1].c_str());
        if ( read_only ) printf("Object specified in set command is read-only: %s\n", tokens[1].c_str());
        return;
    }

    bool  loop_detected = false;
    auto* var           = obj_->selectVariable(tokens[1], loop_detected);
    if ( !var ) {
        printf("Unknown object in set command: %s\n", tokens[1].c_str());
        return;
    }

    if ( var->isReadOnly() ) {
        printf("Object specified in set command is read-only: %s\n", tokens[1].c_str());
        var->selectParent();
        return;
    }

    if ( !var->isFundamental() ) {
        printf("Invalid object in set command: %s is not a fundamental type\n", tokens[1].c_str());
        var->selectParent();
        return;
    }

    try {
        var->set(tokens[2]);
    }
    catch ( std::exception& e ) {
        printf("Invalid format: %s\n", tokens[2].c_str());
    }
    var->selectParent();
}

void
SimpleDebugger::cmd_time(std::vector<std::string>& UNUSED(tokens))
{
    printf("current time = %" PRI_SIMTIME "\n", getCurrentSimCycle());
}

void
SimpleDebugger::cmd_run(std::vector<std::string>& tokens)
{
    if ( tokens.size() == 2 ) {
        try {
            TimeConverter* tc  = getTimeConverter(tokens[1]);
            std::string    msg = format_string("Running clock %" PRI_SIMTIME " sim cycles", tc->getFactor());
            schedule_interactive(tc->getFactor(), msg);
        }
        catch ( std::exception& e ) {
            printf("Unknown time in call to run: %s\n", tokens[1].c_str());
            return;
        }
    }


    done = true;
    return;
}


void
SimpleDebugger::dispatch_cmd(std::string cmd)
{
    std::vector<std::string> tokens;
    tokenize(tokens, cmd);

    if ( tokens[0] == "exit" || tokens[0] == "quit" ) {
        printf("Exiting ObjectExplorer\n");
        done = true;
    }
    else if ( tokens[0] == "pwd" ) {
        cmd_pwd(tokens);
    }
    else if ( tokens[0] == "ls" ) {
        cmd_ls(tokens);
    }
    else if ( tokens[0] == "cd" ) {
        cmd_cd(tokens);
    }
    else if ( tokens[0] == "print" ) {
        cmd_print(tokens);
    }
    else if ( tokens[0] == "set" ) {
        cmd_set(tokens);
    }
    else if ( tokens[0] == "time" ) {
        cmd_time(tokens);
    }
    else if ( tokens[0] == "run" ) {
        cmd_run(tokens);
    }
    else {
        printf("Unknown command: %s\n", tokens[0].c_str());
    }
}


} // namespace Interactive
} // namespace IMPL
} // namespace SST
