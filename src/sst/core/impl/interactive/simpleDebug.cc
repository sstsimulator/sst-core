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

// #include "simpleDebug.h"
#include "sst_config.h"

#include "sst/core/impl/interactive/simpleDebug.h"

#include "sst/core/baseComponent.h"
#include "sst/core/stringize.h"
#include "sst/core/timeConverter.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <unistd.h>
#include "simpleDebug.h"

namespace SST::IMPL::Interactive {

SimpleDebugger::SimpleDebugger(Params& params) :
    InteractiveConsole()
{
    // registerAsPrimaryComponent();

    // We can specify a replay file from the sst command line.
    std::string sstReplayFilePath = params.find<std::string>("replayFile", "");
    if (sstReplayFilePath.size()>0)
        injectedCommand << "replay " << sstReplayFilePath << std::endl;

    // Populate the command registry
    cmdRegistry = {
        {"help",  "?",      "[CMD]: show this help or detailed command help",            ConsoleCommandGroup::GENERAL,    [this](std::vector<std::string>& tokens){ cmd_help(tokens);  }},
        {"pwd",   "pwd",    "print the current working directory in the object map",     ConsoleCommandGroup::NAVIGATION, [this](std::vector<std::string>& tokens){ cmd_pwd(tokens);   }},
        {"chdir", "cd",     "change directory level in the object map",                  ConsoleCommandGroup::NAVIGATION, [this](std::vector<std::string>& tokens){ cmd_cd(tokens);    }},
        {"list",  "ls",     "list the objects in the current level of the object map",   ConsoleCommandGroup::NAVIGATION, [this](std::vector<std::string>& tokens){ cmd_ls(tokens);    }},
        {"time",  "tm",     "print current simulation time in cycles",                   ConsoleCommandGroup::STATE,      [this](std::vector<std::string>& tokens){ cmd_time(tokens);  }},
        {"print", "p",      "[-rN] [<obj>]: print objects at the current level",         ConsoleCommandGroup::STATE,      [this](std::vector<std::string>& tokens){ cmd_print(tokens); }},
        {"set",   "s",      "[-rN] [<obj>]: print objects at the current level",         ConsoleCommandGroup::STATE,      [this](std::vector<std::string>& tokens){ cmd_set(tokens);   }},
        {"watch", "w",      "<trig>: adds watchpoint to the watchlist",                  ConsoleCommandGroup::WATCH,      [this](std::vector<std::string>& tokens){ cmd_watch(tokens); }},
        {"trace", "t",      "<trig> : <bufSize> <postDelay> : <v1> ... <vN> : <action>", ConsoleCommandGroup::WATCH,      [this](std::vector<std::string>& tokens){ cmd_trace(tokens); }},
        {"watchlist", "wl", "prints the current list of watchpoints",                    ConsoleCommandGroup::WATCH,      [this](std::vector<std::string>& tokens){ cmd_watchlist(tokens); }},
        {"addTraceVar", "add", "<watchpointIndex> <var1> ... <varN>",                    ConsoleCommandGroup::WATCH,      [this](std::vector<std::string>& tokens){ cmd_addTraceVar(tokens); }},
        {"printWatchPoint", "prw", "<watchpointIndex>: prints a watchpoint",             ConsoleCommandGroup::WATCH,      [this](std::vector<std::string>& tokens){ cmd_printWatchpoint(tokens); }},
        {"printTrace", "prt", "<watchpointIndex>: prints trace buffer for a watchpoint", ConsoleCommandGroup::WATCH,      [this](std::vector<std::string>& tokens){ cmd_printTrace(tokens); }},
        {"resetTrace", "rst", "<watchpointIndex>: reset trace buffer for a watchpoint",  ConsoleCommandGroup::WATCH,      [this](std::vector<std::string>& tokens){ cmd_resetTraceBuffer(tokens); }},
        {"setHandler", "shn", "<idx> <t1> ... <t2>: trigger check/sampling handler",     ConsoleCommandGroup::WATCH,      [this](std::vector<std::string>& tokens){ cmd_setHandler(tokens); }},
        {"unwatch", "uw", "<watchpointIndex>: remove 1 or all watchpoints",              ConsoleCommandGroup::WATCH,      [this](std::vector<std::string>& tokens){ cmd_unwatch(tokens); }},
        {"run", "r",  "[TIME]: continues the simulation",                                ConsoleCommandGroup::SIMULATION, [this](std::vector<std::string>& tokens){ cmd_run(tokens); }},
        {"continue", "c",  "alias for run",                                              ConsoleCommandGroup::SIMULATION, [this](std::vector<std::string>& tokens){ cmd_run(tokens); }},
        {"exit", "e",  "exit debugger and continue simulation",                          ConsoleCommandGroup::SIMULATION, [this](std::vector<std::string>& tokens){ cmd_exit(tokens); }},
        {"quit", "q",  "alias for exit",                                                 ConsoleCommandGroup::SIMULATION, [this](std::vector<std::string>& tokens){ cmd_exit(tokens); }},
        {"shutdown", "shutdown",  "exit the debugger and cleanly shutdown simulator",    ConsoleCommandGroup::SIMULATION, [this](std::vector<std::string>& tokens){ cmd_shutdown(tokens); }},
        {"logging", "log",  "<filepath>: log command line entires to file",              ConsoleCommandGroup::LOGGING,    [this](std::vector<std::string>& tokens){ cmd_logging(tokens); }},
        {"replay", "rep", "<filepath>: run commands from a file (See also: sst --replay",ConsoleCommandGroup::LOGGING,    [this](std::vector<std::string>& tokens){ cmd_replay(tokens); }},
        {"spinThread", "spin",  "enter spin loop. See SimpleDebugger::cmd_spinThread",  ConsoleCommandGroup::MISC,        [this](std::vector<std::string>& tokens){ cmd_spinThread(tokens); }},
    };

    // Detailed help from some commands. Can also add general things like 'help navigation'
    cmdHelp = {
        {"print","[-rN][<obj>]: print objects in the current level of the object map\n"
                 "\tif -rN is provided print recursive N levels (default N=4)"},
        {"set","<obj> <value>: sets an object in the current scope to the provided value;\n"
                "\tobject must be a 'fundamental type' e.g. int" },
        {"watchpoints", "Manage watchpoints (with or without tracing)\n"
                "\tA <trigger> can be a <comparison> or a sequence of comparisons combined with a <logicOp>\n"
                "\tE.g. <trigger> = <comparison> or <comparison1> <logicOp> <comparison2> ...\n"
                "\tA <comparision> can be '<var> changed' which checks whether the value has changed\n"
                "\tor '<var> <comp> <val>' which compares the variable to a given value\n"
                "\tA <comp> can be <, <=, >, >=, ==, or !=\n"
                "\tA <logicOp> can be && or ||\n"
                "\t'watch' creates a default watchpoint that breaks into an interactive console when triggered\n"
                "\t'trace' creates a watchpoint with a trace buffer to trace a set of variables and trigger an <action>\n"
                "\tAvailable actions include: interactive, printTrace, checkpoint, set, or printStatus"},
        {"watch", "<trigger>: adds watchpoint to the watchlist; breaks into interactive console when triggered\n"
                "\tExample: watch size > 90 && count < 100 || status changed"},
        {"trace", "<trigger> : <bufferSize> <postDelay> : <var1> ... <varN> : <action> "
                "\tAdds watchpoint to the watchlist with a trace buffer of <bufferSize> and a post trigger delay of <postDelay>"
                "\tTraces all of the variables specified in the var list and invokes the <action> after postDelay when triggered"
                "\tExample: trace size > 90 || count == 100 : 32 4 : size count state : printTrace"},
        {"watchlist", "prints the current list of watchpoints and their associated indices\n"
                      "\tNote: a watchpoint's index may change as watchpoints are deleted"},
        {"addTraceVar", "<watchpointIndex> <var1> ... <varN> : adds the specified variables to the specified watchpoint's trace buffer"},
        {"printWatchpoint", "<watchpointIndex>: prints the watchpoint based on the index specified by watchlist"},
        {"printTrace", "<watchpointIndex>: prints the trace buffer for the specified watchpoint"},
        {"resetTrace", "<watchpointIndex>: resets the trace buffer for the specified watchpoint"},
        {"setHandler", "<wpIndex> <handlerType1> ... <handlerTypeN>\n"
                       "\tset where to do trigger checks and sampling (before/after clock/event handler)"},
        {"unwatch", "<watchpointIndex>: removes the specified watchpoint from the watch list. If no index is provided, all watchpoints are removed."},
        {"run", "[TIME]: runs the simulation from the current point for TIME and then returns to\n"
                "\tinteractive mode; if no time is given, the simulation runs to completion;\n"
                "\tTIME is of the format <Number><unit> e.g. 4us"},
    };

}

SimpleDebugger::~SimpleDebugger() {
    if (loggingFile.is_open())
        loggingFile.close();
    if (replayFile.is_open())
        replayFile.close();
}

void
SimpleDebugger::execute(const std::string& msg)
{
    printf("Entering interactive mode at time %" PRI_SIMTIME " \n", getCurrentSimCycle());
    printf("%s\n", msg.c_str());

    // if (cmdRegistery.size()==0) {
    //     cmdRegistery = {
    //         {"help", "?", "Show help", ConsoleCommandGroup::GENERAL, [this](std::vector<std::string>& tokens){ cmd_help(tokens); }}
    //     };
    // }

    if ( nullptr == obj_ ) {
        obj_ = getComponentObjectMap();
    }
    done = false;

    std::string line;
    while ( !done ) {
        
        try {
            // User input prompt
            std::cout << "> " << std::flush;

            if (! injectedCommand.str().empty()) {
                // Injected command stream (currently just one command)
                line = injectedCommand.str();
                injectedCommand.str("");
                std::cout << line << std::endl;
            } else if ( replayFile.is_open() ) {
                // Replay commands from file
                std::getline(replayFile, line);
                std::cout << line << std::endl;
            } else { 
                // Standard Input
                if (!std::cin) 
                    std::cin.clear(); // fix corrupted input after process resumed
                std::getline(std::cin, line);
            }

            dispatch_cmd(line);

            // Command Logging
            if (enLogging)
                loggingFile << line.c_str() << std::endl;
            // This prevents logging the 'logging' command
            if (loggingFile.is_open())
                enLogging = true; 
                
        } catch (const std::runtime_error& e) {
            std::cout << "Parsing error. Ignoring " << line << std::endl;
        }
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
SimpleDebugger::cmd_help(std::vector<std::string>& tokens)
{   
    // First check for specific command help
    if (tokens.size()==1) {
        for (const auto& g : GroupText ) {
            std::cout << "--- " << g.second << " ---" << std::endl;
            for ( const auto& c : cmdRegistry ) {
                if ( g.first == c.group())
                    std::cout << c << std::endl;
            }
        }
        std::cout << "\nMore detailed help also available for: ";
        for (const auto& pair : cmdHelp)
            std::cout << pair.first << " ";
        std::cout << std::endl;
        return;
    }

    if (tokens.size()>1) {
        std::string c = tokens[1];
        if (cmdHelp.find(c) != cmdHelp.end()) {
            std::cout << c << " " << cmdHelp.at(c) << std::endl;
        }
    }
}

// pwd: print current working directory
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

// ls: list current directory
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

// cd <path>: change to new directory
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
        printf("Loop detected in cd.  New working directory will be set to level "
               "of looped object: %s\n",
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

// print [-rN] [<obj>]: print object
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

// set <obj> <value>: set object to value
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
        return;
    }
    var->selectParent();
}

// time: print current simulation cycle
void
SimpleDebugger::cmd_time(std::vector<std::string>& UNUSED(tokens))
{
    printf("current time = %" PRI_SIMTIME "\n", getCurrentSimCycle());
}

// run <time>: run simulation for time
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

// setHandler <wpIndex> <handlerType1> ... <handlerTypeN>
// set where to do trigger checks and sampling (before/after clock/event handler)
void
SimpleDebugger::cmd_setHandler(std::vector<std::string>& tokens)
{
    if ( tokens.size() < 3 ) {
        printf("Invalid format: setHandler <watchpointIndex> <handlerType1> ... <handlerTypeN>\n");
        return;
    }
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cout << "Error: Invalid argument for buffer size: " << tokens[5] << std::endl;
        return;
    }
    catch ( const std::out_of_range& e ) {
        std::cout << "Error: Out of range for buffer size: " << tokens[5] << std::endl;
        return;
    }
    if ( wpIndex >= watch_points_.size() ) {
        std::cout << " Invalid watchpoint index: << " << wpIndex << std::endl;
        return;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    printf("WP %ld - %s\n", wpIndex, wp->getName().c_str());

    // Get handlerTypes and add associated objectBuffers
    size_t   tindex  = 2;
    unsigned handler = 0;
    while ( tindex < tokens.size() ) {
        std::string type = tokens[tindex++];
        // printf("%s ", type.c_str());

        if ( type == "bc" )
            handler = handler | (unsigned)WatchPoint::BEFORE_CLOCK;
        else if ( type == "ac" )
            handler = handler | (unsigned)WatchPoint::AFTER_CLOCK;
        else if ( type == "be" )
            handler = handler | (unsigned)WatchPoint::BEFORE_EVENT;
        else if ( type == "ae" )
            handler = handler | (unsigned)WatchPoint::AFTER_EVENT;
        else if ( type == "all" )
            handler = handler | (unsigned)WatchPoint::ALL;
        else
            printf(" Invalid handler type: %s\n", type.c_str());
    }

    wp->setHandler(handler);
    return;
}

// addTraceVar <wpIndex> <var1> ... <varN>
void
SimpleDebugger::cmd_addTraceVar(std::vector<std::string>& tokens)
{
    if ( tokens.size() < 3 ) {
        printf("Invalid format: addTraceVar <watchpointIndex> <var1> ... <varN>\n");
        return;
    }
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cerr << "Error: Invalid argument for buffer size: " << tokens[5] << std::endl;
        return;
    }
    catch ( const std::out_of_range& e ) {
        std::cerr << "Error: Out of range for buffer size: " << tokens[5] << std::endl;
        return;
    }
    if ( wpIndex >= watch_points_.size() ) {
        printf(" Invalid watchpoint index\n");
        return;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    printf("WP %ld - %s\n", wpIndex, wp->getName().c_str());

    // Get trace vars and add associated objectBuffers
    size_t tindex = 2;
    while ( tindex < tokens.size() ) {
        std::string tvar = tokens[tindex++];
        // printf("%s ", tvar.c_str());

        // Find and check trace variable
        Core::Serialization::ObjectMap* map = obj_->findVariable(tvar);
        if ( nullptr == map ) {
            printf("Unknown variable: %s\n", tvar.c_str());
            return;
        }

        // Is variable fundamental
        if ( !map->isFundamental() ) {
            printf("Traces can only be placed on fundamental types; %s is not "
                   "fundamental\n",
                tvar.c_str());
            return;
        }
        size_t bufsize = wp->getBufferSize();
        if ( bufsize == 0 ) {
            printf("Watchpoint %ld does not have tracing enabled\n", wpIndex);
            return;
        }
        auto* ob = map->getObjectBuffer(obj_->getFullName() + "/" + tvar, bufsize);
        wp->addObjectBuffer(ob);
    }
    return;
}

// resetTraceBuffer <wpIndex>
void
SimpleDebugger::cmd_resetTraceBuffer(std::vector<std::string>& tokens)
{
    if ( tokens.size() != 2 ) {
        printf("Invalid format: resetTraceBuffer <watchpointIndex>\n");
        return;
    }
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cerr << "Error: Invalid argument for buffer size: " << tokens[5] << std::endl;
        return;
    }
    catch ( const std::out_of_range& e ) {
        std::cerr << "Error: Out of range for buffer size: " << tokens[5] << std::endl;
        return;
    }
    if ( wpIndex >= watch_points_.size() ) {
        printf(" Invalid watchpoint index\n");
        return;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    // printf("WP %ld - %s\n", wpIndex, wp->getName().c_str());
    wp->resetTraceBuffer();

    return;
}

// printTrace <wpIndex>
void
SimpleDebugger::cmd_printTrace(std::vector<std::string>& tokens)
{
    if ( tokens.size() != 2 ) {
        printf("Invalid format: printTrace <watchpointIndex>\n");
        return;
    }
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cerr << "Error: Invalid argument for buffer size: " << tokens[5] << std::endl;
        return;
    }
    catch ( const std::out_of_range& e ) {
        std::cerr << "Error: Out of range for buffer size: " << tokens[5] << std::endl;
        return;
    }
    if ( wpIndex >= watch_points_.size() ) {
        printf(" Invalid watchpoint index\n");
        return;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    // printf("WP %ld - %s\n", wpIndex, wp->getName().c_str());
    wp->printTrace();

    return;
}

// printWatchpoint <wpIndex>
void
SimpleDebugger::cmd_printWatchpoint(std::vector<std::string>& tokens)
{
    if ( tokens.size() != 2 ) {
        printf("Invalid format: printWatchpoint <watchpointIndex>\n");
        return;
    }
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cerr << "Error: Invalid argument for buffer size: " << tokens[5] << std::endl;
        return;
    }
    catch ( const std::out_of_range& e ) {
        std::cerr << "Error: Out of range for buffer size: " << tokens[5] << std::endl;
        return;
    }
    if ( wpIndex >= watch_points_.size() ) {
        printf(" Invalid watchpoint index\n");
        return;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    // printf("WP %ld: - %s\n", wpIndex, wp->getName().c_str());
    std::cout << "WP" << wpIndex << ": ";
    wp->printWatchpoint();

    return;
}


// logging <filepath>
void
SimpleDebugger::cmd_logging(std::vector<std::string>& tokens)
{
    if (loggingFile.is_open()) {
        std::cout << "Logging file is already set to " << loggingFilePath << std::endl;
        return;
    }
    if (tokens.size() > 1 ) {
        loggingFilePath = tokens[1];
    }
    // Attempt to open an SST output file
    loggingFile.open(loggingFilePath);
    if (! loggingFile.is_open()) {
        std::cout << "Could not open %s\n" << loggingFilePath.c_str() << std::endl;
        return;
    }
    std::cout << "sst console commands will be logged to " << loggingFilePath << std::endl;
    return;
}

// replay <filepath>
void
SimpleDebugger::cmd_replay(std::vector<std::string>& tokens)
{
    if (replayFile.is_open()) {
        std::cout << "Replay file is already set to " << replayFilePath << std::endl;
        return;
    }
    if (tokens.size() > 1 ) {
        replayFilePath = tokens[1];
    }

    replayFile.open(replayFilePath);
    if (! replayFile.is_open()) 
        std::cout << "Could not open replay file: " << replayFilePath << std::endl;

    return;
}

WatchPoint::LogicOp
getLogicOpFromString(const std::string& opStr)
{
    if ( opStr == "&&" ) {
        return WatchPoint::LogicOp::AND;
    }
    else if ( opStr == "||" ) {
        return WatchPoint::LogicOp::OR;
    }
    else {
        return WatchPoint::LogicOp::UNDEFINED;
    }
}

// watchlist
void
SimpleDebugger::cmd_watchlist(std::vector<std::string>& UNUSED(tokens))
{
    // Print the watch points
    printf("Current watch points:\n");
    int count = 0;
    for ( auto& x : watch_points_ ) {
        // printf("  %d - %s\n", count++, x.first->getName().c_str());
        std::cout << count++ << ": ";
        x.first->printWatchpoint();
    }
    return;
}

// gdb helper. Recommended SST configuration
// CXXFLAGS="-g3 -O0" CFLAGS="-g3 -O0"  ../configure --prefix=$SST_CORE_HOME --enable-debug'
void
SimpleDebugger::cmd_spinThread(std::vector<std::string>& UNUSED(tokens))
{
    // Print the watch points
    std::cout << "Spinning PID " << getpid() << std::endl;
    while( spinner > 0 ) {
        spinner++;
        usleep(100000);
        // set debug breakpoint here and set spinner to 0 to continue
        if( spinner % 10 == 0 )                                                                                                                 
            std::cout << "." << std::flush;
    }
    spinner=1; // reset spinner
    std::cout << std::endl;
    return;
}

Core::Serialization::ObjectMapComparison*
parseComparison(std::vector<std::string>& tokens, size_t& index, Core::Serialization::ObjectMap* obj, std::string& name)
{

    std::string                                  var("");
    Core::Serialization::ObjectMapComparison::Op op = Core::Serialization::ObjectMapComparison::Op::INVALID;
    std::string                                  v2("");
    std::string                                  name2("");
    std::string                                  opstr("");

    // Get first comparison
    var = tokens[index++];
    if ( index == tokens.size() ) {
        printf("Invalid format for trigger test\n");
        return nullptr;
    }
    opstr = tokens[index++];
    op    = Core::Serialization::ObjectMapComparison::getOperationFromString(opstr);
    if ( op != Core::Serialization::ObjectMapComparison::Op::CHANGED ) {
        if ( index == tokens.size() ) {
            printf("Invalid format for trigger test. Valid formats are <var> changed"
                   " and <var> <op> <val>\n");
            return nullptr;
        }
        v2 = tokens[index++];
    }

    // Check for errors and build ObjectMapComparison
    // Look for variable
    Core::Serialization::ObjectMap* map = obj->findVariable(var);

    // Valid variable name
    if ( nullptr == map ) {
        printf("Unknown variable: %s\n", var.c_str());
        return nullptr;
    }

    // Is variable fundamental
    if ( !map->isFundamental() ) {
        printf("Triggers can only use fundamental types; %s is not "
               "fundamental\n",
            var.c_str());
        return nullptr;
    }

    // Is operator valid
    if ( op == Core::Serialization::ObjectMapComparison::Op::INVALID ) {
        printf("Unknown comparison operation specified in trigger test\n");
        return nullptr;
    }

    name = obj->getFullName() + "/" + var;

    // Check if v2 is a variable
    Core::Serialization::ObjectMap* map2 = obj->findVariable(v2);

    // V2 is valid variable
    if ( nullptr != map2 ) {
        // printf("v2 is variable\n");

        // Is variable fundamental
        if ( !map2->isFundamental() ) {
            printf("Triggers can only use fundamental types; %s is not "
                   "fundamental\n",
                v2.c_str());
            return nullptr;
        }

        name2 = obj->getFullName() + "/" + v2;
        try {
            auto* c = map->getComparisonVar(name, op, name2, map2); // Can throw an exception
            return c;
        }
        catch ( std::exception& e ) {
            printf("Invalid argument passed to trigger test: %s %s %s\n", var.c_str(), opstr.c_str(), v2.c_str());
            return nullptr;
        }
    }
    else { // V2 is value string
        // printf("v2 is value string\n");
        try {
            auto* c = map->getComparison(name, op, v2); // Can throw an exception
            return c;
        }
        catch ( std::exception& e ) {
            printf("Invalid argument passed to trigger test: %s %s %s\n", var.c_str(), opstr.c_str(), v2.c_str());
            return nullptr;
        }
    }
}


WatchPoint::WPAction*
parseAction(std::vector<std::string>& tokens, size_t& index, Core::Serialization::ObjectMap* obj)
{
    std::string action = tokens[index++];

    if ( action == "interactive" ) {
        return new WatchPoint::InteractiveWPAction();
    }
    else if ( action == "printTrace" ) {
        return new WatchPoint::PrintTraceWPAction();
    }
    else if ( action == "checkpoint" ) {
        return new WatchPoint::CheckpointWPAction();
    }
    else if ( action == "printStatus" ) {
        return new WatchPoint::PrintStatusWPAction();
    }
    else if ( action == "set" ) {
        if ( index >= tokens.size() ) {
            printf("Missing variable for set command\n");
            return nullptr;
        }
        std::string tvar = tokens[index++];
        // printf("%s ", tvar.c_str());

        if ( index >= tokens.size() ) {
            printf("Missing value for set command\n");
            return nullptr;
        }
        std::string tval = tokens[index++];

        // Find and check variable
        Core::Serialization::ObjectMap* map = obj->findVariable(tvar);
        if ( nullptr == map ) {
            printf("Unknown variable: %s\n", tvar.c_str());
            return nullptr;
        }

        // Is variable fundamental
        if ( !map->isFundamental() ) {
            printf("Can only set fundamental variable, %s is not fundamental\n", tvar.c_str());
            return nullptr;
        }

        // Is variable read-only
        if ( map->isReadOnly() ) {
            printf("Object specified in set command is read-only: %s\n", tvar.c_str());
            return nullptr;
        }

        // Check for valid value
        if ( !map->checkValue(tval) ) {
            // printf("Invalid value specified in set command: %s\n", tval.c_str());
            return nullptr;
        }

        std::string name = obj->getFullName() + "/" + tvar;

        return new WatchPoint::SetVarWPAction(name, map, tval);
    }
#if 0
    else if (action == "heartbeat") {
        return WatchPoint::HeartbeatAction();
    }
#endif
    else {
        return nullptr;
    }
}

// watch <trigger>   where
//  <trigger> is <comparison> OR <comparison> <logicOp> <comparison> ...
//  <comparison> is <var> changed OR <var> <op> <val> OR <var> <op> <var>
//  <logicOp> is one of:  &&,  ||
//  <op> is one of:  <, <=, >, >=, ==, !=
// Examples
//  watch size changed
//  watch size > 90
//  watch size > max
//  watch size > 90 && value == 100
//  watch size changed || value == 100 && index == 55
void
SimpleDebugger::cmd_watch(std::vector<std::string>& tokens)
{
    size_t      index = 1;
    std::string name  = "";
    try {
        // Get first comparison
        Core::Serialization::ObjectMapComparison* c = parseComparison(tokens, index, obj_, name);
        if ( c == nullptr ) {
            printf("Invalid comparison argument passed to watch command\n");
            return;
        }
        auto* pt = new WatchPoint(name, c);

#if 0 // watch variables currently don't trace, but they could automatically
      // trace test vars
        auto* tb = map->getTraceBuffer(obj_, 32, 4);
        pt->addTraceBuffer(tb);

        auto* ob = map->getObjectBuffer(obj_->getFullName() + "/" + var, 32);
        pt->addObjectBuffer(ob);
#endif

        // Get the top level component to set the watch point
        BaseComponent* comp = static_cast<BaseComponent*>(base_comp_->getAddr());
        if ( comp ) {
            comp->addWatchPoint(pt);
            watch_points_.emplace_back(pt, comp);
        }
        else
            printf("Not a component\n");

        // Add additional comparisons and logical ops
        while ( index < tokens.size() ) {

            // Get Logical Operator
            WatchPoint::LogicOp logicOp = getLogicOpFromString(tokens[index++]);
            if ( logicOp == WatchPoint::LogicOp::UNDEFINED ) {
                printf("Invalid logic operator: %s", tokens[index - 1].c_str());
            }
            else {
                pt->addLogicOp(logicOp);
            }
            if ( index == tokens.size() ) {
                printf("Invalid format for watch command\n");
                return;
            }

            // Get next comparison
            Core::Serialization::ObjectMapComparison* c = parseComparison(tokens, index, obj_, name);
            if ( c == nullptr ) {
                printf("Invalid comparison argument passed to watch command\n");
                return;
            }
            pt->addComparison(c);

        } // while index < tokens.size(), add another logic op and test comparision

        // Parse action
        std::string           action    = "interactive";
        WatchPoint::WPAction* actionObj = new WatchPoint::InteractiveWPAction();
        if ( actionObj == nullptr ) {
            printf("Error in action: %s\n", action.c_str());
            return;
        }
        else {
            pt->setAction(actionObj);
        }
    } // try/catch  TODO: need to revisit what can actually throw an exception
    catch ( std::exception& e ) {
        printf("Invalid format for watch command\n");
        return;
    }

    // Check for extra arguments
    if ( index != tokens.size() ) {
        printf("Invalid format for watch command: too many arguments\n");
        return;
    }
}

// unwatch <wpIndex>
void
SimpleDebugger::cmd_unwatch(std::vector<std::string>& tokens)
{

    // If no arguments, unwatch all watchpoints
    if ( tokens.size() == 1 ) {
        for ( std::pair<WatchPoint*, BaseComponent*>& wp : watch_points_ ) {
            WatchPoint*    pt   = wp.first;
            BaseComponent* comp = wp.second;
            comp->removeWatchPoint(pt);
        }
        watch_points_.clear();
        return;
    }

    if ( tokens.size() != 2 ) {
        printf("Invalid format for unwatch command\n");
        return;
    }

    long unsigned int index = 0;

    try {
        index = (long unsigned int)SST::Core::from_string<int>(tokens[1]);
    }
    catch ( std::invalid_argument& e ) {
        printf("Invalid index format specified. The unwatch command requires that "
               "one of the index shown when "
               "\"watch\" is run with no arguments be specified\n");
        return;
    }

    if ( watch_points_.size() <= index ) {
        printf("Watch point %s not found. The unwatch command requires that one of "
               "the index shown when \"watchlist\" is run be specified\n",
            tokens[1].c_str());
        return;
    }

    WatchPoint*    pt   = watch_points_[index].first;
    BaseComponent* comp = watch_points_[index].second;

    comp->removeWatchPoint(pt);

    watch_points_.erase(watch_points_.begin() + index);
}


Core::Serialization::TraceBuffer*
parseTraceBuffer(std::vector<std::string>& tokens, size_t& index, Core::Serialization::ObjectMap* obj)
{
    size_t bufsize = 32;
    size_t pdelay  = 0;

    // Get buffer config
    if ( tokens[index++] != ":" ) {
        printf("Invalid format: trace <trigger> : <bufsize> <postdelay> : <v1> ... "
               "<vN> : <action>\n");
        return nullptr;
    }
    // Could check for ":" here and assume that means they just want default
    // values for buffer size and post delay

    try {
        bufsize = std::stoi(tokens[index++]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cerr << "Error: Invalid argument for buffer size: " << tokens[5] << std::endl;
        return nullptr;
    }
    catch ( const std::out_of_range& e ) {
        std::cerr << "Error: Out of range for buffer size: " << tokens[5] << std::endl;
        return nullptr;
    }

    // Get post delay
    try {
        pdelay = std::stoi(tokens[index++]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cerr << "Error: Invalid argument for post trigger delay: " << tokens[6] << std::endl;
        return nullptr;
    }
    catch ( const std::out_of_range& e ) {
        std::cerr << "Error: Out of range for post trigger delay: " << tokens[6] << std::endl;
        return nullptr;
    }

    if ( tokens[index++] != ":" ) {
        printf("Invalid format: trace <var> <op> <value> : <bufsize> <postdelay> : "
               "<v1> ... <vN> : <action>\n");
        return nullptr;
    }

    try {
        // Setup Trace Buffer
        return new Core::Serialization::TraceBuffer(obj, bufsize, pdelay);
    }
    catch ( std::exception& e ) {
        printf("HERE: Invalid buffer argument passed to trace command\n");
        return nullptr;
    }
}

Core::Serialization::ObjectBuffer*
parseTraceVar(std::string& tvar, Core::Serialization::ObjectMap* obj, Core::Serialization::TraceBuffer* tb)
{
    // Find and check trace variable
    Core::Serialization::ObjectMap* map = obj->findVariable(tvar);
    if ( nullptr == map ) {
        printf("Unknown variable: %s\n", tvar.c_str());
        return nullptr;
    }

    // Is variable fundamental
    if ( !map->isFundamental() ) {
        printf("Traces can only be placed on fundamental types; %s is not "
               "fundamental\n",
            tvar.c_str());
        return nullptr;
    }
    std::string name = obj->getFullName() + "/" + tvar;
    return map->getObjectBuffer(name, tb->getBufferSize());
}

// trace <trigger> : <bufsize> <postdelay> : <v1> ... <vN> :  <action>
// <trigger> is defined in cmd_watch above
// <action> is optional - default is break to interactive console
// Could also consider having multiple actions and/or default buffer config
// TODO check at each step that we haven't exceeded token size
void
SimpleDebugger::cmd_trace(std::vector<std::string>& tokens)
{
    if ( tokens.size() < 9 ) {
        printf("Invalid format: trace <var> <op> <value> : <bufsize> <postdelay> : "
               "<v1> ... <vN> : <action>\n");
        return;
    }

    size_t      index = 1;
    std::string name  = "";

    // Get first comparison
    Core::Serialization::ObjectMapComparison* c = parseComparison(tokens, index, obj_, name);
    if ( c == nullptr ) {
        printf("Invalid argument passed in comparison trigger command\n");
        return;
    }
    auto* pt = new WatchPoint(name, c);

    // Add additional comparisons and logical ops
    while ( index < tokens.size() ) {

        if ( tokens[index] == ":" ) { // end of trace vars
            break;
        }

        // Get Logical Operator
        WatchPoint::LogicOp logicOp = getLogicOpFromString(tokens[index++]);
        if ( logicOp == WatchPoint::LogicOp::UNDEFINED ) {
            printf("Invalid logic operator: %s", tokens[index - 1].c_str());
        }
        else {
            pt->addLogicOp(logicOp);
        }
        if ( index == tokens.size() ) {
            printf("Invalid format for trace command\n");
            return;
        }

        // Get next comparison
        Core::Serialization::ObjectMapComparison* c = parseComparison(tokens, index, obj_, name);
        if ( c == nullptr ) {
            printf("Invalid argument in comparison of trace command\n");
            return;
        }
        pt->addComparison(c);

    } // while index < tokens.size(), add another logic op and test comparision

    try {
        auto* tb = parseTraceBuffer(tokens, index, obj_);
        if ( tb == nullptr ) {
            printf("Invalid trace buffer argument in trace command\n");
            return;
        }
        pt->addTraceBuffer(tb);

        // Get trace vars and add associated objectBuffers
        while ( index < tokens.size() ) {

            std::string tvar = tokens[index++];
            if ( tvar == ":" ) { // end of trace vars
                break;
            }

            auto* objBuf = parseTraceVar(tvar, obj_, tb);
            if ( objBuf == nullptr ) {
                printf("Invalid trace variable argument passed to trace command\n");
                return;
            }
            pt->addObjectBuffer(objBuf);
        } // end while get trace vars

        // Parse action
        std::string action = tokens[index];

        WatchPoint::WPAction* actionObj = parseAction(tokens, index, obj_);
        if ( actionObj == nullptr ) {
            printf("Error in action: %s\n", action.c_str());
            return;
        }
        else {
            pt->setAction(actionObj);
        }

        // Check for extra arguments
        if ( index != tokens.size() ) {
            printf("Error, too many arguments\n");
            return;
        }

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
        printf("Invalid format for trace command\n");
        return;
    }
};

// exit OR quit
void
SimpleDebugger::cmd_exit(std::vector<std::string>& UNUSED(tokens))
{
    // Remove all watchpoints
    for ( std::pair<WatchPoint*, BaseComponent*>& wp : watch_points_ ) {
        WatchPoint*    pt   = wp.first;
        BaseComponent* comp = wp.second;
        comp->removeWatchPoint(pt);
    }
    watch_points_.clear();
    printf("Removing all watchpoints and exiting ObjectExplorer\n");
    done = true;
    return;
}


// shutdown
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
    // empty command
    if (cmd.size()==0) return;
    
    std::vector<std::string> tokens;
    tokenize(tokens, cmd);

    // just whitespace
    if (tokens.size()==0) return;

    // comment
    if ( tokens[0][0]=='#') 
        return;

    // The business
    for ( auto consoleCommand : cmdRegistry ) {
        if (consoleCommand.match(tokens[0])) {
            consoleCommand.exec(tokens);
            return;
        }
    }

    // Oops
    printf("Unknown command: %s\n", tokens[0].c_str());
}

} // namespace SST::IMPL::Interactive
