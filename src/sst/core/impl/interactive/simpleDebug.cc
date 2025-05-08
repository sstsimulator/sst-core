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

#include "sst/core/baseComponent.h"
#include "sst/core/stringize.h"
#include "sst/core/timeConverter.h"

namespace SST::IMPL::Interactive {

SimpleDebugger::SimpleDebugger(Params& UNUSED(params)) :
    InteractiveConsole()
{}

void
SimpleDebugger::execute(const std::string& msg)
{
    printf("Entering interactive mode at time %" PRI_SIMTIME " \n", getCurrentSimCycle());
    printf("%s\n", msg.c_str());
    if ( nullptr == obj_ ) {
        obj_ = getComponentObjectMap();
    }
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
SimpleDebugger::cmd_help(std::vector<std::string>& UNUSED(tokens))
{
    std::string help = "SimpleDebug Console Commands\n";
    help.append("  Navigation: Navigate the current object map\n");
    help.append("   - pwd: print the current working directory in the object map\n");
    help.append("   - cd: change directory level in the object map\n");
    help.append("   - ls: list the objects in the current level of the object map\n");

    help.append("  Current State: Print information about the current simulation state\n");
    help.append("   - time: print current simulation time in cycles\n");
    help.append("   - print [-rN][<obj>]: print objects in the current level of the object map;\n");
    help.append("                         if -rN is provided print recursive N levels (default N=4)\n");

    help.append("  Modify State: Modify simulation variables\n");
    help.append("   - set <obj> <value>: sets an object in the current scope to the provided value;\n");
    help.append("                        object must be a \"fundamental type\" e.g. int \n");

    help.append("  Watch Points: Manage watch points which break into interactive console when triggered\n");
    help.append("   - watch: prints the current list of watch points and their associated indices\n");
    help.append("     watch <var>: adds var to the watch list; triggered when value changes\n");
    help.append("     watch <var> <comp> <val>: add var to watch list; triggered when comparison with val is true\n");
    help.append("                 Valid <comp> operators: <, <=, >, >=, ==, !=\n");
    help.append("   - unwatch <index>: removes the indexed watch point from the watch list;\n");
    help.append("                 <index> is the associated index from the list of watch points\n");

    help.append("  Execute: Execute the simulation for a specified duration\n");
    help.append("   - run [TIME]: runs the simulation from the current point for TIME and then returns to\n");
    help.append("                 interactive mode; if no time is given, the simulation runs to completion;\n");
    help.append("                 TIME is of the format <Number><unit> e.g. 4us\n");

    help.append("  Exit: Exit the interactive console\n");
    help.append("   - exit or quit: exits the interactive console and resumes simulation execution\n");
    help.append("   - shutdown: exits the interactive console and does a clean shutdown of the simulation\n");

    printf("%s", help.c_str());
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
    auto& vars = obj_->getVariables();
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
        // See if this is the top level component, and if so, set it
        // to nullptr
        if ( dynamic_cast<Core::Serialization::ObjectMap*>(base_comp_) == obj_ ) {
            base_comp_ = nullptr;
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
        printf("Loop detected in cd.  New working directory will be set to level of looped object: %s\n",
            new_obj->getFullName().c_str());
    }
    obj_ = new_obj;

    // If we don't already have the top level component, check to see
    // if this is it
    if ( nullptr == base_comp_ ) {
        Core::Serialization::ObjectMapDeferred<BaseComponent>* base_comp =
            dynamic_cast<Core::Serialization::ObjectMapDeferred<BaseComponent>*>(obj_);
        if ( base_comp ) base_comp_ = base_comp;
    }
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
SimpleDebugger::cmd_watch(std::vector<std::string>& tokens)
{
    if ( tokens.size() == 1 ) {
        // Just print the watch points
        printf("Current watch points:\n");
        int count = 0;
        for ( auto& x : watch_points_ ) {
            printf("  %d - %s\n", count++, x.first->getName().c_str());
        }
        return;
    }

    std::string                                  var("");
    Core::Serialization::ObjectMapComparison::Op op = Core::Serialization::ObjectMapComparison::Op::INVALID;
    std::string                                  val("");

    if ( tokens.size() == 2 ) {
        var = tokens[1];
        op  = Core::Serialization::ObjectMapComparison::Op::CHANGED;
    }
    else if ( tokens.size() == 4 ) {
        var = tokens[1];
        op  = Core::Serialization::ObjectMapComparison::getOperationFromString(tokens[2]);
        val = tokens[3];
    }
    else {
        printf("Invalid format for watch command. Valid formats are watch <var> and watch <var> <comp> <val>\n");
        return;
    }

    // Look for variable
    Core::Serialization::ObjectMap* map = obj_->findVariable(var);

    // Check for errors

    // Valid variable name
    if ( nullptr == map ) {
        printf("Unknown variable: %s\n", var.c_str());
        return;
    }

    // Is variable fundamental
    if ( !map->isFundamental() ) {
        printf("Watches can only be placed on fundamental types; %s is not fundamental\n", var.c_str());
        return;
    }

    // Is operator valid
    if ( op == Core::Serialization::ObjectMapComparison::Op::INVALID ) {
        printf("Unknown comparison operation specified in watch command\n");
        return;
    }

    // Variable is a fundamental, set up the watch point

    // Setup the watch point
    try {
        auto* c  = map->getComparison(obj_->getFullName() + "/" + var, op, val);
        auto* pt = new WatchPoint(obj_->getFullName() + "/" + var, c);

        // Get the top level component to set the watch point
        BaseComponent* comp = static_cast<BaseComponent*>(base_comp_->getAddr());
        if ( comp ) {
            comp->addWatchPoint(pt);
            watch_points_.emplace_back(pt, comp);
        }
        else
            printf("Not a component\n");
    }
    catch ( std::exception& e ) {
        printf("Invalid argument passed to watch command\n");
        return;
    }
}

void
SimpleDebugger::cmd_unwatch(std::vector<std::string>& tokens)
{
    if ( tokens.size() != 2 ) {
        printf("Invalid format for unwatch command\n");
        return;
    }

    size_t index = 0;

    try {
        index = SST::Core::from_string<int>(tokens[1]);
    }
    catch ( std::invalid_argument& e ) {
        printf("Invalid index format specified.  The unwatch command requires that one of the index shown when "
               "\"watch\" is run with no arguments be specified\n");
        return;
    }

    if ( watch_points_.size() <= index ) {
        printf(
            "Watch point %s not found. The unwatch command requires that one of the index shown when \"watch\" is run "
            "with no arguments be specified\n",
            tokens[1].c_str());
        return;
    }

    WatchPoint*    pt   = watch_points_[index].first;
    BaseComponent* comp = watch_points_[index].second;

    comp->removeWatchPoint(pt);

    watch_points_.erase(watch_points_.begin() + index);
}

void
SimpleDebugger::cmd_shutdown(std::vector<std::string>& UNUSED(tokens))
{
    simulationShutdown();
    done = true;
    printf("Exiting ObjectExplorer and shutting down simulation\n");
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
    else if ( tokens[0] == "watch" ) {
        cmd_watch(tokens);
    }
    else if ( tokens[0] == "unwatch" ) {
        cmd_unwatch(tokens);
    }
    else if ( tokens[0] == "shutdown" ) {
        cmd_shutdown(tokens);
    }
    else if ( tokens[0] == "help" ) {
        cmd_help(tokens);
    }
    else {
        printf("Unknown command: %s\n", tokens[0].c_str());
    }
}

} // namespace SST::IMPL::Interactive
