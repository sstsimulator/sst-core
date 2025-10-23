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
#include "sst/core/simulation_impl.h"
#include "sst/core/stringize.h"
#include "sst/core/timeConverter.h"

#include <cstddef>
#include <cstdio>
#include <iostream>
#include <list>
#include <sstream>
#include <stdexcept>
#include <unistd.h>
#include <utility>

#include "simpleDebug.h"

namespace SST::IMPL::Interactive {

SimpleDebugger::SimpleDebugger(Params& params) :
    InteractiveConsole()
{
    // registerAsPrimaryComponent();

    // We can specify a replay file from the sst command line.
    std::string sstReplayFilePath = params.find<std::string>("replayFile", "");
    if ( sstReplayFilePath.size() > 0 ) injectedCommand << "replay " << sstReplayFilePath << std::endl;

    // Populate the command registry
    cmdRegistry = {
        { "help", "?", "<[CMD]>: show this help or detailed command help", ConsoleCommandGroup::GENERAL,
            [this](std::vector<std::string>& tokens) { cmd_help(tokens); } },
        { "verbose", "v", "[mask]: set verbosity mask or print if no mask specified", ConsoleCommandGroup::GENERAL,
            [this](std::vector<std::string>& tokens) { cmd_verbose(tokens); } },
        { "confirm", "cfm", "<true/false>: set confirmation requests on (default) or off", ConsoleCommandGroup::GENERAL,
            [this](std::vector<std::string>& tokens) { cmd_setConfirm(tokens); } },
        { "pwd", "pwd", "print the current working directory in the object map", ConsoleCommandGroup::NAVIGATION,
            [this](std::vector<std::string>& tokens) { cmd_pwd(tokens); } },
        { "chdir", "cd", "change 1 directory level in the object map", ConsoleCommandGroup::NAVIGATION,
            [this](std::vector<std::string>& tokens) { cmd_cd(tokens); } },
        { "list", "ls", "list the objects in the current level of the object map", ConsoleCommandGroup::NAVIGATION,
            [this](std::vector<std::string>& tokens) { cmd_ls(tokens); } },
        { "time", "tm", "print current simulation time in cycles", ConsoleCommandGroup::STATE,
            [this](std::vector<std::string>& tokens) { cmd_time(tokens); } },
        { "print", "p", "[-rN] [<obj>]: print objects at the current level", ConsoleCommandGroup::STATE,
            [this](std::vector<std::string>& tokens) { cmd_print(tokens); } },
        { "set", "s", "var value: set value for a variable at the current level", ConsoleCommandGroup::STATE,
            [this](std::vector<std::string>& tokens) { cmd_set(tokens); } },
        { "watch", "w", "<trig>: adds watchpoint to the watchlist", ConsoleCommandGroup::WATCH,
            [this](std::vector<std::string>& tokens) { cmd_watch(tokens); } },
        { "trace", "t", "<trig> : <bufSize> <postDelay> : <v1> ... <vN> : <action>", ConsoleCommandGroup::WATCH,
            [this](std::vector<std::string>& tokens) { cmd_trace(tokens); } },
        { "watchlist", "wl", "prints the current list of watchpoints", ConsoleCommandGroup::WATCH,
            [this](std::vector<std::string>& tokens) { cmd_watchlist(tokens); } },
        { "addTraceVar", "add", "<watchpointIndex> <var1> ... <varN>", ConsoleCommandGroup::WATCH,
            [this](std::vector<std::string>& tokens) { cmd_addTraceVar(tokens); } },
        { "printWatchPoint", "prw", "<watchpointIndex>: prints a watchpoint", ConsoleCommandGroup::WATCH,
            [this](std::vector<std::string>& tokens) { cmd_printWatchpoint(tokens); } },
        { "printTrace", "prt", "<watchpointIndex>: prints trace buffer for a watchpoint", ConsoleCommandGroup::WATCH,
            [this](std::vector<std::string>& tokens) { cmd_printTrace(tokens); } },
        { "resetTrace", "rst", "<watchpointIndex>: reset trace buffer for a watchpoint", ConsoleCommandGroup::WATCH,
            [this](std::vector<std::string>& tokens) { cmd_resetTraceBuffer(tokens); } },
        { "setHandler", "shn", "<idx> <t1> ... <t2>: trigger check/sampling handler", ConsoleCommandGroup::WATCH,
            [this](std::vector<std::string>& tokens) { cmd_setHandler(tokens); } },
        { "unwatch", "uw", "<watchpointIndex>: remove 1 or all watchpoints", ConsoleCommandGroup::WATCH,
            [this](std::vector<std::string>& tokens) { cmd_unwatch(tokens); } },
        { "run", "r", "[TIME]: continues the simulation", ConsoleCommandGroup::SIMULATION,
            [this](std::vector<std::string>& tokens) { cmd_run(tokens); } },
        { "continue", "c", "alias for run", ConsoleCommandGroup::SIMULATION,
            [this](std::vector<std::string>& tokens) { cmd_run(tokens); } },
        { "exit", "e", "exit debugger and continue simulation", ConsoleCommandGroup::SIMULATION,
            [this](std::vector<std::string>& tokens) { cmd_exit(tokens); } },
        { "quit", "q", "alias for exit", ConsoleCommandGroup::SIMULATION,
            [this](std::vector<std::string>& tokens) { cmd_exit(tokens); } },
        { "shutdown", "shutd", "exit the debugger and cleanly shutdown simulator", ConsoleCommandGroup::SIMULATION,
            [this](std::vector<std::string>& tokens) { cmd_shutdown(tokens); } },
        { "logging", "log", "<filepath>: log command line entires to file", ConsoleCommandGroup::LOGGING,
            [this](std::vector<std::string>& tokens) { cmd_logging(tokens); } },
        { "replay", "rep", "<filepath>: run commands from a file. See also: sst --replay", ConsoleCommandGroup::LOGGING,
            [this](std::vector<std::string>& tokens) { cmd_replay(tokens); } },
        { "history", "h", "[N]: display all or last N unique commands", ConsoleCommandGroup::LOGGING,
            [this](std::vector<std::string>& tokens) { cmd_history(tokens); } },
        { "autoComplete", "ac", "toggle command line auto-completion enable", ConsoleCommandGroup::MISC,
            [this](std::vector<std::string>& tokens) { cmd_autoComplete(tokens); } },
        { "clear", "clr", "reset terminal", ConsoleCommandGroup::MISC,
            [this](std::vector<std::string>& tokens) { cmd_clear(tokens); } },
        { "spinThread", "spin", "enter spin loop. See SimpleDebugger::cmd_spinThread", ConsoleCommandGroup::MISC,
            [this](std::vector<std::string>& tokens) { cmd_spinThread(tokens); } },
    };

    // Detailed help from some commands. Can also add general things like 'help navigation'
    cmdHelp = {
        { "verbose", "[mask]: set verbosity mask or print if no mask specified\n"
                     "\tA mask is used to select which features to enable verbosity.\n"
                     "\tTo turn on all features set the mask to 0xffffffff\n"
                     "\t\t0x10: Show trigger details" },
        { "print", "[-rN][<obj>]: print objects in the current level of the object map\n"
                   "\tif -rN is provided print recursive N levels (default N=4)" },
        { "set", "<obj> <value>: sets an object in the current scope to the provided value\n"
                 "\tobject must be a 'fundamental type' (arithmetic or string)\n"
                 "\t e.g. set mystring hello world" },
        { "watchpoints",
            "Manage watchpoints (with or without tracing)\n"
            "\tA <trigger> can be a <comparison> or a sequence of comparisons combined with a <logicOp>\n"
            "\tE.g. <trigger> = <comparison> or <comparison1> <logicOp> <comparison2> ...\n"
            "\tA <comparision> can be '<var> changed' which checks whether the value has changed\n"
            "\tor '<var> <op> <val>' which compares the variable to a given value\n"
            "\tAn <op> can be <, <=, >, >=, ==, or !=\n"
            "\tA <logicOp> can be && or ||\n"
            "\t'watch' creates a default watchpoint that breaks into an interactive console when triggered\n"
            "\t'trace' creates a watchpoint with a trace buffer to trace a set of variables and trigger an <action>\n"
            "\tAvailable actions include: \n"
            "\t  interactive, printTrace, checkpoint, set <var> <val>, printStatus, or shutdown" },
        { "watch", "<trigger>: adds watchpoint to the watchlist; breaks into interactive console when triggered\n"
                   "\tExample: watch var1 > 90 && var2 < 100 || var3 changed" },
        { "trace",
            "<trigger> : <bufferSize> <postDelay> : <var1> ... <varN> : <action>\n"
            "\tAdds watchpoint to the watchlist with a trace buffer of <bufferSize> and a post trigger delay of "
            "<postDelay>\n"
            "\tTraces all of the variables specified in the var list and invokes the <action> after postDelay "
            "when triggered\n"
            "\tAvailable actions include: \n"
            "\t  interactive, printTrace, checkpoint, set <var> <val>, printStatus, or shutdown\n"
            "\t  Note: checkpoint action must be enabled at startup via the '--checkpoint-enable' command line option\n"
            "\tExample: trace var1 > 90 || var2 == 100 : 32 4 : size count state : printTrace" },
        { "watchlist", "prints the current list of watchpoints and their associated indices" },
        { "addtracevar", "<watchpointIndex> <var1> ... <varN> : adds the specified variables to the specified "
                         "watchpoint's trace buffer" },
        { "printwatchpoint", "<watchpointIndex>: prints the watchpoint based on the index specified by watchlist" },
        { "printtrace", "<watchpointIndex>: prints the trace buffer for the specified watchpoint" },
        { "resettrace", "<watchpointIndex>: resets the trace buffer for the specified watchpoint" },
        { "sethandler", "<wpIndex> <handlerType1> ... <handlerTypeN>\n"
                        "\tset where to do trigger checks and sampling (before/after clock/event handler)" },
        { "unwatch", "<watchpointIndex>: removes the specified watchpoint from the watch list.\n"
                     "\tIf no index is provided, all watchpoints are removed." },
        { "run", "[TIME]: runs the simulation from the current point for TIME and then returns to\n"
                 "\tinteractive mode; if no time is given, the simulation runs to completion;\n"
                 "\tTIME is of the format <Number><unit> e.g. 4us" },
        { "history", "[N]: list previous N instructions. If N is not set list all\n"
                     "\tSupports bash-style commands:\n"
                     "\t!!   execute previous command\n"
                     "\t!n   execute command at index n\n"
                     "\t!-n  execute commad n lines back in history\n"
                     "\t!string  execute the most recent command starting with `string`\n"
                     "\t?string execute the most recent command containing `string`\n"
                     "\t!...:p  print the instruction but not execute it." },
        { "editing", ": bash style command line editing using arrow and control keys:\n"
                     "\tUp/Down keys: navigate command history\n"
                     "\tLeft/Right keys: navigate command string\n"
                     "\tbackspace: delete characters to the left\n"
                     "\ttab: auto-completion\n"
                     "\tctrl-a: move cursor to beginning of line\n"
                     "\tctrl-b: move cursor to the left\n"
                     "\tctrl-d: delete character at cursor\n"
                     "\tctrl-e: move cursor to end of line\n"
                     "\tctrl-f: move cursor to the right\n" },
    };

    // Command autofill strings
    std::list<std::string> cmdStrings;
    for ( const ConsoleCommand& c : cmdRegistry ) {
        cmdStrings.emplace_back(c.str_long());
        cmdStrings.emplace_back(c.str_short());
    }
    cmdStrings.sort();
    cmdLineEditor.set_cmd_strings(cmdStrings); // could also realize as callback to generalize

    // Callback for directory listing strings
    cmdLineEditor.set_listing_callback([this](std::list<std::string>& vec) { get_listing_strings(vec); });
}

SimpleDebugger::~SimpleDebugger()
{
    if ( loggingFile.is_open() ) loggingFile.close();
    if ( replayFile.is_open() ) replayFile.close();
}

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

        try {
            // User input prompt
            std::cout << "> " << std::flush;

            if ( !injectedCommand.str().empty() ) {
                // Injected command stream (currently just one command)
                line = injectedCommand.str();
                injectedCommand.str("");
                std::cout << line << std::endl;
            }
            else if ( replayFile.is_open() ) {
                // Replay commands from file
                if ( std::getline(replayFile, line) ) {
                    std::cout << line << std::endl;
                }
                else {
                    if ( replayFile.eof() )
                        std::cout << "(Finished reading from " << replayFilePath << ")" << std::endl;
                    else
                        std::cout << "An error occured reading from " << replayFilePath << std::endl;
                    replayFile.close();
                }
            }
            else {
                // Standard Input
                if ( !std::cin ) std::cin.clear(); // fix corrupted input after process resumed
                std::cout.flush();
                if ( autoCompleteEnable && isatty(STDIN_FILENO) )
                    cmdLineEditor.getline(cmdHistoryBuf.getBuffer(), line);
                else
                    std::getline(std::cin, line);
            }

            dispatch_cmd(line);

            // Command Logging
            if ( enLogging ) loggingFile << line.c_str() << std::endl;
            // This prevents logging the 'logging' command
            if ( loggingFile.is_open() ) enLogging = true;
        }
        catch ( const std::runtime_error& e ) {
            std::cout << "Parsing error. Ignoring " << line << std::endl;
        }
    }
}

// Invoke the command.
// Substitution actions (!!, !?, ...) can modify the command.
// This ensure the final, resolved, command is captured in the command log
void
SimpleDebugger::dispatch_cmd(std::string& cmd)
{
    // empty command
    if ( cmd.size() == 0 ) return;

    std::vector<std::string> tokens;
    tokenize(tokens, cmd);

    // just whitespace
    if ( tokens.size() == 0 ) return;

    // comment
    if ( tokens[0][0] == '#' ) return;

    // History !! and friends
    if ( tokens[0][0] == '!' ) {
        std::string newcmd;
        auto        rc = cmdHistoryBuf.bang(tokens[0], newcmd);
        if ( rc == CommandHistoryBuffer::BANG_RC::ECHO_ONLY ) {
            // replace, print, save command in history
            cmd = newcmd;
            std::cout << cmd << std::endl;
            cmdHistoryBuf.append(cmd);
            return;
        }
        else if ( rc == CommandHistoryBuffer::BANG_RC::EXEC ) {
            // replace and print new command then let it flow through
            std::cout << newcmd << std::endl;
            tokens.clear();
            cmd = newcmd;
            tokenize(tokens, cmd);
        }
        else if ( rc == CommandHistoryBuffer::BANG_RC::NOP ) {
            // invalid search, just return
            return;
        }
    }

    // Search for the requested command and execute it if found.
    for ( auto consoleCommand : cmdRegistry ) {
        if ( consoleCommand.match(tokens[0]) ) {
            consoleCommand.exec(tokens); // TODO prefer having return code to know if succeeded
            cmdHistoryBuf.append(cmd);
            return;
        }
    }

    // No matching command found
    std::cout << "Unknown command: " << tokens[0].c_str() << std::endl;
    cmdHistoryBuf.append(cmd); // want garbled command so we can fix using command line editor
}

//
/*
    !!:  Executes the previous command
    !n:  Executes the command at history index n.
    !-n: Executes the command n lines back in history.
    !string: Executes the most recent command starting with "string".
    !?string: Executes the most recent command containing "string" anywhere.
*/

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
    if ( tokens.size() == 1 ) {
        for ( const auto& g : GroupText ) {
            std::cout << "--- " << g.second << " ---" << std::endl;
            for ( const auto& c : cmdRegistry ) {
                if ( g.first == c.group() ) std::cout << c << std::endl;
            }
        }
        std::cout << "\nMore detailed help also available for:\n";
        std::stringstream s;
        for ( const auto& pair : cmdHelp ) {
            if ( (s.str().length() + pair.first.length() > 39) ) {
                std::cout << "\t" << s.str() << std::endl;
                s.str("");
                s.clear();
            }
            s << pair.first << " ";
        }
        std::cout << "\t" << s.str() << std::endl;
        std::cout << std::endl;
        return;
    }

    if ( tokens.size() > 1 ) {
        std::string c = tokens[1];
        if ( cmdHelp.find(c) != cmdHelp.end() ) {
            std::cout << c << " " << cmdHelp.at(c) << std::endl;
        }
        else {
            for ( auto& creg : cmdRegistry ) {
                if ( creg.match(c) ) std::cout << creg << std::endl;
            }
        }
    }
}

void
SimpleDebugger::cmd_verbose(std::vector<std::string>& tokens)
{
    if ( tokens.size() > 1 ) {
        try {
            verbosity = SST::Core::from_string<uint32_t>(tokens[1]);
        }
        catch ( std::invalid_argument& e ) {
            std::cout << "Invalid mask " << tokens[1] << std::endl;
        }
    }
#if 1
    // This messes up the auto-complete keyboard input
    std::cout << "verbose=0x" << std::hex << verbosity << std::endl;
#else
    std::cout << "verbose=" << verbosity << std::endl;
#endif

    // update watchpoint verbosity
    for ( auto& x : watch_points_ ) {
        if ( x.first ) x.first->setVerbosity(verbosity);
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

    std::cout << obj_->getFullName() << " (" << obj_->getType() << ")\n";
}

// ls: list current directory
void
SimpleDebugger::cmd_ls(std::vector<std::string>& UNUSED(tokens))
{
    auto& vars = obj_->getVariables();
    for ( auto& x : vars ) {
        if ( x.second->isFundamental() ) {
            std::cout << x.first << " = " << x.second->get() << " (" << x.second->getType() << ")" << std::endl;
        }
        else {
            std::cout << x.first.c_str() << "/ (" << x.second->getType() << ")\n";
        }
    }
}

// callback for autofill of object string (similar to ls)
void
SimpleDebugger::get_listing_strings(std::list<std::string>& list)
{
    list.clear();
    auto& vars = obj_->getVariables();
    for ( auto& x : vars ) {
        std::stringstream s;
        s << x.first;
        if ( !x.second->isFundamental() ) s << "/";
        list.emplace_back(s.str());
    }
    list.sort();
}


// cd <path>: change to new directory
void
SimpleDebugger::cmd_cd(std::vector<std::string>& tokens)
{
    if ( tokens.size() != 2 ) {
        printf("Invalid format for cd command (cd <obj>)\n");
        return;
    }

    // Allow for trailing '/'
    std::string selection = tokens[1];
    if ( !selection.empty() && selection.back() == '/' ) selection.pop_back();

    // Check for ..
    if ( selection == ".." ) {
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
    SST::Core::Serialization::ObjectMap* new_obj       = obj_->selectVariable(selection, loop_detected);
    assert(new_obj);
    if ( !new_obj || (new_obj == obj_) ) {
        printf("Unknown object in cd command: %s\n", selection.c_str());
        return;
    }

    if ( new_obj->isFundamental() ) {
        printf("Object %s is a fundamental type so you cannot cd into it\n", selection.c_str());
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
    if ( tokens.size() < 3 ) {
        printf("Invalid format for set command (set <obj> <value>)\n");
        return;
    }
    // kg It may be safer to check for  exactly 3 params but because
    //    of strings we allow for more (until parser handled quoted strings)
    //    If var->selectParent() is not used prior to returning we
    //    can get a segmentation fault on a subsequent command. Address
    //    Sanitizer indicated use of previously freed memory.
    //

    if ( obj_->isContainer() ) {
        bool found     = false;
        bool read_only = false;
        obj_->set(tokens[1], tokens[2], found, read_only);
        if ( !found ) printf("Unknown object in set command for container: %s\n", tokens[1].c_str());
        if ( read_only ) printf("Object specified in set command is read-only for container: %s\n", tokens[1].c_str());
        // TODO do we need var->selectParent() here?
        return;
    }

    bool  loop_detected = false;
    auto* var           = obj_->selectVariable(tokens[1], loop_detected);
    assert(var);
    if ( !var || (var == obj_) ) {
        printf("Unknown object in set command: %s\n", tokens[1].c_str());
        // TODO make sure selectVariable hasn't altered any state.
        return;
    }

    // Once we have a valid object, be sure to use var->selectParent() or
    // future commands may attempt to use free'd memory.

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
    std::string value = tokens[2];
    if ( var->getType() == "std::string" ) {
        for ( size_t index = 3; index < tokens.size(); index++ ) {
            value = value + " " + tokens[index];
        }
    }
    else {
        value = tokens[2];
    }

    try {
        var->set(value);
    }
    catch ( std::exception& e ) {
        printf("Invalid format: %s\n", tokens[2].c_str());
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
            std::string    msg = format_string("Ran clock for %" PRI_SIMTIME " sim cycles", tc->getFactor());
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
        std::cout << "Invalid argument for buffer size: " << tokens[5] << std::endl;
        return;
    }
    catch ( const std::out_of_range& e ) {
        std::cout << "Out of range for buffer size: " << tokens[5] << std::endl;
        return;
    }
    if ( wpIndex >= watch_points_.size() ) {
        std::cout << "Invalid watchpoint index: " << wpIndex << std::endl;
        return;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    if ( wp == nullptr ) {
        std::cout << "Invalid watchpoint index: " << wpIndex << std::endl;
        return;
    }
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
        std::cerr << "Invalid argument for buffer size: " << tokens[5] << std::endl;
        return;
    }
    catch ( const std::out_of_range& e ) {
        std::cerr << "Out of range for buffer size: " << tokens[5] << std::endl;
        return;
    }
    if ( wpIndex >= watch_points_.size() ) {
        std::cout << " Invalid watchpoint index: " << wpIndex << std::endl;
        return;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    if ( wp == nullptr ) {
        std::cout << " Invalid watchpoint index: " << wpIndex << std::endl;
        return;
    }
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
        std::cout << "Invalid format: resetTraceBuffer <watchpointIndex>\n";
        return;
    }
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cerr << "Invalid argument for buffer size: " << tokens[5] << std::endl;
        return;
    }
    catch ( const std::out_of_range& e ) {
        std::cerr << "Out of range for buffer size: " << tokens[5] << std::endl;
        return;
    }
    if ( wpIndex >= watch_points_.size() ) {
        std::cout << "Invalid watchpoint index: " << wpIndex << std::endl;
        return;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    if ( wp == nullptr ) {
        std::cout << "Invalid watchpoint index: " << wpIndex << std::endl;
        return;
    }
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
        std::cerr << "Invalid argument for buffer size: " << tokens[5] << std::endl;
        return;
    }
    catch ( const std::out_of_range& e ) {
        std::cerr << "Out of range for buffer size: " << tokens[5] << std::endl;
        return;
    }
    if ( wpIndex >= watch_points_.size() ) {
        std::cout << "Invalid watchpoint index: " << wpIndex << std::endl;
        return;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    if ( wp == nullptr ) {
        std::cout << "Invalid watchpoint index: " << wpIndex << std::endl;
        return;
    }

    wp->printTrace();

    return;
}

// printWatchpoint <wpIndex>
void
SimpleDebugger::cmd_printWatchpoint(std::vector<std::string>& tokens)
{
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: printWatchpoint <watchpointIndex>\n";
        return;
    }
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cout << "Invalid argument for buffer size: " << tokens[5] << std::endl;
        return;
    }
    catch ( const std::out_of_range& e ) {
        std::cout << "Out of range for buffer size: " << tokens[5] << std::endl;
        return;
    }
    if ( wpIndex >= watch_points_.size() ) {
        std::cout << "Invalid watchpoint index: " << wpIndex << std::endl;
        return;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    if ( wp == nullptr ) {
        std::cout << "Invalid watchpoint index: " << wpIndex << std::endl;
        return;
    }
    else {
        std::cout << "WP" << wpIndex << ": ";
        wp->printWatchpoint();
    }

    return;
}


// logging <filepath>
void
SimpleDebugger::cmd_logging(std::vector<std::string>& tokens)
{
    if ( loggingFile.is_open() ) {
        std::cout << "Logging file is already set to " << loggingFilePath << std::endl;
        return;
    }
    if ( tokens.size() > 1 ) {
        loggingFilePath = tokens[1];
    }
    // Attempt to open an SST output file
    loggingFile.open(loggingFilePath);
    if ( !loggingFile.is_open() ) {
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
    if ( replayFile.is_open() ) {
        std::cout << "Replay file is already set to " << replayFilePath << std::endl;
        return;
    }
    if ( tokens.size() > 1 ) {
        replayFilePath = tokens[1];
    }

    replayFile.open(replayFilePath);
    if ( !replayFile.is_open() ) std::cout << "Could not open replay file: " << replayFilePath << std::endl;

    return;
}

void
SimpleDebugger::cmd_history(std::vector<std::string>& tokens)
{
    int recs = 0; // 0 indicates all history
    if ( tokens.size() > 1 ) {
        try {
            recs = (int)SST::Core::from_string<int>(tokens[1]);
        }
        catch ( std::invalid_argument& e ) {
            std::cout << "history: Ignoring arg1 (" << tokens[1] << ")" << std::endl;
        }
    }
    cmdHistoryBuf.print(recs);
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
        if ( x.first == nullptr ) {
            count++;
        }
        else {
            std::cout << count++ << ": ";
            x.first->printWatchpoint();
        }
    }
    return;
}

void
SimpleDebugger::cmd_autoComplete(std::vector<std::string>& UNUSED(tokens))
{
    autoCompleteEnable = !autoCompleteEnable;
    std::cout << "auto completion is now " << autoCompleteEnable << std::endl;
}

void
SimpleDebugger::cmd_clear(std::vector<std::string>& UNUSED(tokens))
{
    // clear screen and move cursor to (0,0)
    std::cout << "\033[2J\033[1;1H";
}

// gdb helper. Recommended SST configuration
// CXXFLAGS="-g3 -O0" CFLAGS="-g3 -O0"  ../configure --prefix=$SST_CORE_HOME --enable-debug'
void
SimpleDebugger::cmd_spinThread(std::vector<std::string>& UNUSED(tokens))
{
    // Print the watch points
    std::cout << "Spinning PID " << getpid() << std::endl;
    while ( spinner > 0 ) {
        spinner++;
        usleep(100000);
        // set debug breakpoint here and set spinner to 0 to continue
        if ( spinner % 10 == 0 ) std::cout << "." << std::flush;
    }
    spinner = 1; // reset spinner
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
        if ( Simulation_impl::getSimulation()->checkpoint_directory_ == "" ) {
            std::cout << "Invalid action: checkpointing not enabled (use --checkpoint-enable cmd line option)\n";
            return nullptr;
        }
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
        return new WatchPoint::HeartbeatWPAction();
    }
#endif
    else if ( action == "shutdown" ) {
        return new WatchPoint::ShutdownWPAction();
    }
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

    if ( tokens.size() < 3 ) {
        printf("Invalid format for watch command\n");
        return;
    }
    try {
        // Get first comparison
        Core::Serialization::ObjectMapComparison* c = parseComparison(tokens, index, obj_, name);
        if ( c == nullptr ) {
            printf("Invalid comparison argument passed to watch command\n");
            return;
        }
        size_t wpIndex = watch_points_.size();
        auto*  pt      = new WatchPoint(wpIndex, name, c);

#if 0 // watch variables currently don't trace, but they could automatically
      // trace test vars
        auto* tb = map->getTraceBuffer(obj_, 32, 4);
        pt->addTraceBuffer(tb);

        auto* ob = map->getObjectBuffer(obj_->getFullName() + "/" + var, 32);
        pt->addObjectBuffer(ob);
#endif

        // Add additional comparisons and logical ops
        while ( index < tokens.size() ) {

            // Get Logical Operator
            WatchPoint::LogicOp logicOp = getLogicOpFromString(tokens[index++]);
            if ( logicOp == WatchPoint::LogicOp::UNDEFINED ) {
                std::cout << "Invalid logic operator: " << tokens[index - 1] << std::endl;
                return;
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
            // Every action gets the same verbosity as the console object
            actionObj->setVerbosity(verbosity);
            pt->setAction(actionObj);
        }

        // Get the top level component to set the watch point
        BaseComponent* comp = static_cast<BaseComponent*>(base_comp_->getAddr());
        if ( comp ) {
            comp->addWatchPoint(pt);
            watch_points_.emplace_back(pt, comp);
            std::cout << "Added watchpoint #" << wpIndex << std::endl;
        }
        else {
            printf("Not a component\n");
            return;
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

// confirm <true/false>
void
SimpleDebugger::cmd_setConfirm(std::vector<std::string>& tokens)
{

    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for confirm command: confirm <true/false>\n";
        return;
    }

    if ( (tokens[1] == "true") || (tokens[1] == "t") || (tokens[1] == "T") || (tokens[1] == "1") ) {
        confirm = true;
    }
    else if ( (tokens[1] == "false") || (tokens[1] == "f") || (tokens[1] == "F") || (tokens[1] == "0") ) {
        confirm = false;
    }
    else {
        std::cout << "Invalid argument for confirm: must be true or false" << tokens[1] << std::endl;
    }
}

bool
SimpleDebugger::clear_watchlist()
{

    if ( confirm ) {
        std::string line;
        std::cout << "Do you want to delete all watchpoints? [yes, no]\n";
        std::getline(std::cin, line);
        std::vector<std::string> tokens;
        tokenize(tokens, line);

        if ( tokens.size() == 0 ) return false;
        if ( !(tokens[0] == "yes") ) return false;
    }

    // Remove watchpoints
    // Does this delete the objects correctly?
    for ( std::pair<WatchPoint*, BaseComponent*>& wp : watch_points_ ) {

        WatchPoint* pt = wp.first;
        if ( pt != nullptr ) { // already removed using unwatch <wpindex>
            BaseComponent* comp = wp.second;
            comp->removeWatchPoint(pt);
        }
    }
    watch_points_.clear();
    return true;
}


// unwatch <wpIndex>
void
SimpleDebugger::cmd_unwatch(std::vector<std::string>& tokens)
{
    // If no arguments, unwatch all watchpoints
    if ( tokens.size() == 1 ) {
        clear_watchlist();
        std::cout << "Watchlist cleared\n";
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

    WatchPoint* pt = watch_points_[index].first;
    if ( pt != nullptr ) { // already removed
        BaseComponent* comp = watch_points_[index].second;

        // Remove and mark as unused
        comp->removeWatchPoint(pt);
        watch_points_[index].first  = nullptr;
        watch_points_[index].second = nullptr;
    }
    return;
}


Core::Serialization::TraceBuffer*
parseTraceBuffer(std::vector<std::string>& tokens, size_t& index, Core::Serialization::ObjectMap* obj)
{
    size_t bufsize = 32;
    size_t pdelay  = 0;

    // Get buffer config
    if ( tokens[index++] != ":" ) {
        std::cout << "Invalid format: trace <trigger> : <bufsize> <postdelay> : <v1> ... "
                     "<vN> : <action>\n";
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
        std::cout << "Invalid format: trace <var> <op> <value> : <bufsize> <postdelay> : "
                     "<v1> ... <vN> : <action>\n";
        return nullptr;
    }

    try {
        // Setup Trace Buffer
        return new Core::Serialization::TraceBuffer(obj, bufsize, pdelay);
    }
    catch ( std::exception& e ) {
        std::cout << "Invalid buffer argument passed to trace command\n";
        return nullptr;
    }
}

Core::Serialization::ObjectBuffer*
parseTraceVar(std::string& tvar, Core::Serialization::ObjectMap* obj, Core::Serialization::TraceBuffer* tb)
{
    // Find and check trace variable
    Core::Serialization::ObjectMap* map = obj->findVariable(tvar);
    if ( nullptr == map ) {
        std::cout << "Unknown variable: " << tvar << std::endl;
        return nullptr;
    }

    // Is variable fundamental
    if ( !map->isFundamental() ) {
        std::cout << "Traces can only be placed on fundamental types; " << tvar << "is not fundamental\n";
        return nullptr;
    }
    std::string name = obj->getFullName() + "/" + tvar;
    return map->getObjectBuffer(name, tb->getBufferSize());
}

// trace <trigger> : <bufsize> <postdelay> : <v1> ... <vN> :  <action>
// <trigger> is defined in cmd_watch above
// <action> to execute on trigger
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
    size_t wpIndex = watch_points_.size();
    auto*  pt      = new WatchPoint(wpIndex, name, c);

    // Add additional comparisons and logical ops
    while ( index < tokens.size() ) {

        if ( tokens[index] == ":" ) { // end of trace vars
            break;
        }

        // Get Logical Operator
        WatchPoint::LogicOp logicOp = getLogicOpFromString(tokens[index++]);
        if ( logicOp == WatchPoint::LogicOp::UNDEFINED ) {
            std::cout << "Invalid logic operator: " << tokens[index - 1] << std::endl;
            return;
        }
        else {
            pt->addLogicOp(logicOp);
        }
        if ( index == tokens.size() ) {
            std::cout << "Invalid format for trace command\n";
            return;
        }

        // Get next comparison
        Core::Serialization::ObjectMapComparison* c = parseComparison(tokens, index, obj_, name);
        if ( c == nullptr ) {
            std::cout << "Invalid argument in comparison of trace command\n";
            return;
        }
        pt->addComparison(c);

    } // while index < tokens.size(), add another logic op and test comparision

    try {
        auto* tb = parseTraceBuffer(tokens, index, obj_);
        if ( tb == nullptr ) {
            std::cout << "Invalid trace buffer argument in trace command\n";
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
                std::cout << "Invalid trace variable argument passed to trace command\n";
                return;
            }
            pt->addObjectBuffer(objBuf);
        } // end while get trace vars

        // Parse action
        std::string action = tokens[index];

        WatchPoint::WPAction* actionObj = parseAction(tokens, index, obj_);
        if ( actionObj == nullptr ) {
            std::cout << "Error in action: " << action << std::endl;
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
            std::cout << "Added watchpoint #" << wpIndex << std::endl;
        }
        else {
            std::cout << "Not a component\n";
            return;
        }
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
    bool cleared = clear_watchlist();
    if ( cleared ) {
        std::cout << "Removing all watchpoints and exiting ObjectExplorer\n";
    }
    else {
        std::cout << "Exiting ObjectExplorer without clearning watchpoints\n";
    }
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
CommandHistoryBuffer::append(std::string s)
{
    buf_[nxt_] = std::make_pair(count_++, s);
    sz_        = sz_ < MAX_CMDS - 1 ? sz_ + 1 : MAX_CMDS;
    cur_       = nxt_;
    nxt_       = (nxt_ + 1) % MAX_CMDS;
}

void
CommandHistoryBuffer::print(int num)
{
    if ( sz_ == 0 ) return;
    int n   = num < sz_ ? num : sz_;
    n       = n <= 0 ? sz_ : n;
    int idx = (nxt_ - n) % sz_;
    if ( idx < 0 ) idx += sz_;
    for ( int i = 0; i < n; i++ ) {
        std::cout << buf_[idx].first << " " << buf_[idx].second << std::endl;
        idx = (idx + 1) % MAX_CMDS;
    }
}

std::vector<std::string>&
CommandHistoryBuffer::getBuffer()
{
    // TODO: combine for print
    stringBuffer_.clear();
    if ( sz_ == 0 ) return stringBuffer_;
    int n   = sz_;
    int idx = (nxt_ - n) % sz_;
    if ( idx < 0 ) idx += sz_;
    for ( int i = 0; i < n; i++ ) {
        stringBuffer_.emplace_back(buf_[idx].second);
        idx = (idx + 1) % MAX_CMDS;
    }
    return stringBuffer_;
}

CommandHistoryBuffer::BANG_RC
CommandHistoryBuffer::bang(const std::string& token, std::string& newcmd)
{
    auto rc = CommandHistoryBuffer::BANG_RC::INVALID;
    if ( this->sz_ == 0 ) return rc;

    // !!       Execute the previous instruction
    // !n      Execute instruction at history index n
    // !-n      Execute instruction n lines back in history
    // !string  Execute the most recent command starting with string
    // !?string Execute the most recent command containing string
    // !...:p   Find instruction in history but only print it

    // Check for :p  and strip it from token
    bool        echo = false;
    std::string base = token;
    if ( token.length() >= 2 ) {
        std::string last2chars = token.substr(token.length() - 2);
        if ( last2chars == ":p" ) {
            echo = true;
            base = token.substr(0, token.length() - 2);
        }
    }

    // grab first two chars and arg
    if ( base.length() < 2 ) return rc;

    // At this point we have a valid command. Worst case a NOP
    rc = CommandHistoryBuffer::BANG_RC::NOP;

    std::string cmd = base.substr(0, 2);
    std::string arg = "";
    if ( base.length() > 2 ) arg = base.substr(2);

    bool found = false;
    if ( cmd == "!!" ) {
        if ( arg.length() == 0 ) {
            newcmd = buf_[cur_].second;
            found  = true;
        }
        else {
            std::cout << "Invalid command: " << base << std::endl;
            return rc;
        }
    }
    else if ( cmd == "!-" ) {
        found = findOffset(arg, newcmd);
    }
    else if ( cmd == "!?" ) {
        found = searchAny(arg, newcmd);
    }
    else {
        // Either !n or !string
        arg   = base.substr(1);
        found = findEvent(arg, newcmd);
        if ( !found ) found = searchFirst(arg, newcmd);
        if ( !found ) std::cout << "history: event not found: " << arg << std::endl;
    }

    if ( found ) {
        rc = echo ? CommandHistoryBuffer::BANG_RC::ECHO_ONLY : CommandHistoryBuffer::BANG_RC::EXEC;
    }

    return rc;
}

bool
CommandHistoryBuffer::findEvent(const std::string& s, std::string& newcmd)
{
    // !n
    int event = -1;
    try {
        event = (int)SST::Core::from_string<int>(s);
    }
    catch ( std::invalid_argument& e ) {
        // std::cout << "history: invalid event: " << s << std::endl;
        return false;
    }
    if ( event < 0 ) {
        // std::cout << "history: invalid event: " << event << std::endl;
        return false;
    }
    // search backwards (most recent first)
    int idx = cur_;
    for ( int i = 0; i < sz_; i++ ) {
        if ( buf_[idx].first == (size_t)event ) {
            newcmd = buf_[idx].second;
            return true;
        }
        idx--;
        if ( idx < 0 ) idx = sz_ - 1;
    }
    return false;
}

bool
CommandHistoryBuffer::findOffset(const std::string& s, std::string& newcmd)
{
    // !-n
    int offset = -1;
    try {
        offset = (int)SST::Core::from_string<int>(s);
    }
    catch ( std::invalid_argument& e ) {
        std::cout << "history: invalid offset: " << s << std::endl;
        return false;
    }

    if ( offset > sz_ || offset < 1 ) {
        std::cout << "history: offset not found: " << offset << std::endl;
        return false;
    }

    int idx = cur_ - offset + 1;
    if ( idx < 0 ) idx += sz_;

    newcmd = buf_[idx].second;
    return true;
}

bool
CommandHistoryBuffer::searchFirst(const std::string& s, std::string& newcmd)
{
    // !string
    // search backwards. Most recent first
    int idx = cur_;
    for ( int i = 0; i < sz_; i++ ) {
        std::string chk = buf_[idx].second;
        idx--;
        if ( idx < 0 ) idx += sz_;
        if ( chk.length() < s.length() ) continue;
        if ( chk.substr(0, s.length()) == s ) {
            newcmd = chk;
            return true;
        }
    }
    // std::cout << "history: start string not found: " << s << std::endl;
    return false;
}

bool
CommandHistoryBuffer::searchAny(const std::string& s, std::string& newcmd)
{
    // !?string
    int idx = cur_;
    for ( int i = 0; i < sz_; i++ ) {
        std::string chk = buf_[idx].second;
        idx--;
        if ( idx < 0 ) idx += sz_;
        if ( chk.length() < s.length() ) continue;
        if ( chk.find(s) != std::string::npos ) {
            newcmd = chk;
            return true;
        }
    }
    std::cout << "history: string not found: " << s << std::endl;
    return false;
}


void
SimpleDebugger::msg(VERBOSITY_MASK mask, std::string message)
{
    if ( (!static_cast<uint32_t>(mask)) & verbosity ) return;
    std::cout << message << std::endl;
}

} // namespace SST::IMPL::Interactive
