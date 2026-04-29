// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/impl/interactive/debugConsole.h"

#include "sst/core/baseComponent.h"
#include "sst/core/simulation.h"
#include "sst/core/stringize.h"
#include "sst/core/timeConverter.h"

#include <cstddef>
#include <cstdio>
#include <ios>
#include <iostream>
#include <list>
#include <sstream>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>
#include <utility>

namespace SST::IMPL::Interactive {

// Static Initialization
unsigned                 DebugConsole::current_thread = 0;
unsigned                 DebugConsole::current_rank   = 0;
std::vector<std::string> DebugConsole::tokens;
std::stringstream        DebugConsole::result;


DebugConsole::DebugConsole(Params& params) :
    InteractiveConsole(),
    dout(std::cout, 50, 160)
{
    // registerAsPrimaryComponent();
    num_ranks_ = Simulation_impl::getSimulation()->getNumRanks();
    rank_      = Simulation_impl::getSimulation()->getRank();

    // Serial (single rank, single thread)
    if ( num_ranks_.rank == 1 && num_ranks_.thread == 1 ) {
        exec_type = ExecutionType::SERIAL;
    }
    // Thread (single rank, multiple threads)
    else if ( num_ranks_.rank == 1 ) {
        exec_type = ExecutionType::THREAD;
    }
    // Rank Serial (multiple ranks, single thread per rank)
    else if ( num_ranks_.thread == 1 ) {
        exec_type = ExecutionType::RANK_SERIAL;
    }
    // Rank Parallel (multiple ranks, multiple threads per rank)
    else {
        exec_type = ExecutionType::RANK_PARALLEL;
    }

    // We can specify a replay file from the sst command line.
    std::string sstReplayFilePath = params.find<std::string>("replayFile", "");
    if ( sstReplayFilePath.size() > 0 ) injectedCommand << "replay " << sstReplayFilePath << std::endl;

    // Populate the command registry
    cmdRegistry = CommandRegistry({

        // Navigation
        {
            "help",
            "?",
            "<[CMD]>: show this help or detailed command help",
            ConsoleCommandGroup::GENERAL,
            [this](std::string& cmd_str) { return cmd_help(cmd_str); },
        },
        { "verbose", "v", "[mask]: set verbosity mask or print if no mask specified", ConsoleCommandGroup::GENERAL,
            exec_type, [this](std::string& cmd_str) { return cmd_verbose_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_verbose_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_verbose_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_verbose_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_verbose_remote(tokens); } },

        {
            "info",
            "info",
            "\"current\"|\"all\" print summary for current thread or all threads",
            ConsoleCommandGroup::GENERAL,
            exec_type,
            [this](std::string& cmd_str) { return cmd_info_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_info_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_info_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_info_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_info_remote(tokens); },
        },
        { "thread", "thd", "[threadID]: switch to specified thread ID", ConsoleCommandGroup::GENERAL, exec_type,
            [this](std::string& cmd_str) { return cmd_thread_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_thread_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_thread_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_thread_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_thread_remote(tokens); } },
        { "rank", "rank", "[rankID]: switch to specified rank ID, same thread", ConsoleCommandGroup::GENERAL, exec_type,
            [this](std::string& cmd_str) { return cmd_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_rank_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_rank_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_rank_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_rank_remote(tokens); } },
        { "confirm", "cfm", "<true/false>: set confirmation requests on (default) or off", ConsoleCommandGroup::GENERAL,
            [this](std::string& cmd_str) { return cmd_setConfirm(cmd_str); } },
        { "pwd", "pwd", "print the current working directory in the object map", ConsoleCommandGroup::NAVIGATION,
            exec_type, [this](std::string& cmd_str) { return cmd_pwd_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_pwd_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_pwd_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_pwd_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_pwd_remote(tokens); } },
        { "chdir", "cd", "change 1 directory level in the object map", ConsoleCommandGroup::NAVIGATION, exec_type,
            [this](std::string& cmd_str) { return cmd_cd_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_cd_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_cd_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_cd_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_cd_remote(tokens); } },
        { "list", "ls", "list the objects in the current level of the object map", ConsoleCommandGroup::NAVIGATION,
            exec_type, [this](std::string& cmd_str) { return cmd_ls_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_ls_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_ls_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_ls_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_ls_remote(tokens); } },

        // State
        { "time", "tm", "print current simulation time in cycles", ConsoleCommandGroup::STATE,
            [this](std::string& cmd_str) { return cmd_time(cmd_str); } },
        {
            "print",
            "p",
            "[-rN] [<obj>]: print objects at the current level",
            ConsoleCommandGroup::STATE,
            exec_type,
            [this](std::string& cmd_str) { return cmd_print_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_print_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_print_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_print_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_print_remote(tokens); },
        },
        {
            "set",
            "s",
            "var value: set value for a variable at the current level",
            ConsoleCommandGroup::STATE,
            exec_type,
            [this](std::string& cmd_str) { return cmd_set_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_set_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_set_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_set_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_set_remote(tokens); },
        },
        {
            "watch",
            "w",
            "<trig>: adds watchpoint to the watchlist",
            ConsoleCommandGroup::WATCH,
            exec_type,
            [this](std::string& cmd_str) { return cmd_watch_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_watch_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_watch_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_watch_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_watch_remote(tokens); },
        },
        {
            "trace",
            "t",
            "<trig> : <bufSize> <postDelay> : <v1> ... <vN> : <action>",
            ConsoleCommandGroup::WATCH,
            exec_type,
            [this](std::string& cmd_str) { return cmd_trace_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_trace_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_trace_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_trace_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_trace_remote(tokens); },
        },
        {
            "watchlist",
            "wl",
            "prints the current list of watchpoints",
            ConsoleCommandGroup::WATCH,
            exec_type,
            [this](std::string& cmd_str) { return cmd_watchlist_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_watchlist_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_watchlist_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_watchlist_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_watchlist_remote(tokens); },
        },
        {
            "addTraceVar",
            "add",
            "<watchpointIndex> <var1> ... <varN>",
            ConsoleCommandGroup::WATCH,
            exec_type,
            [this](std::string& cmd_str) { return cmd_addTraceVar_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_addTraceVar_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_addTraceVar_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_addTraceVar_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_addTraceVar_remote(tokens); },
        },
        {
            "printWatchPoint",
            "prw",
            "<watchpointIndex>: prints a watchpoint",
            ConsoleCommandGroup::WATCH,
            exec_type,
            [this](std::string& cmd_str) { return cmd_printWatchpoint_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_printWatchpoint_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_printWatchpoint_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_printWatchpoint_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_printWatchpoint_remote(tokens); },
        },
        {
            "printTrace",
            "prt",
            "<watchpointIndex>: prints trace buffer for a watchpoint",
            ConsoleCommandGroup::WATCH,
            exec_type,
            [this](std::string& cmd_str) { return cmd_printTrace_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_printTrace_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_printTrace_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_printTrace_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_printTrace_remote(tokens); },
        },
        {
            "resetTrace",
            "rst",
            "<watchpointIndex>: reset trace buffer for a watchpoint",
            ConsoleCommandGroup::WATCH,
            exec_type,
            [this](std::string& cmd_str) { return cmd_resetTraceBuffer_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_resetTraceBuffer_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_resetTraceBuffer_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_resetTraceBuffer_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_resetTraceBuffer_remote(tokens); },
        },
        {
            "setHandler",
            "shn",
            "<idx> <t1> ... <t2>: trigger check/sampling handler",
            ConsoleCommandGroup::WATCH,
            exec_type,
            [this](std::string& cmd_str) { return cmd_setHandler_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_setHandler_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_setHandler_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_setHandler_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_setHandler_remote(tokens); },
        },
        {
            "unwatch",
            "uw",
            "<watchpointIndex>: remove 1 or all watchpoints",
            ConsoleCommandGroup::WATCH,
            exec_type,
            [this](std::string& cmd_str) { return cmd_unwatch_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_unwatch_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_unwatch_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_unwatch_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return cmd_unwatch_remote(tokens); },
        },
        // Simulation
        {
            "run",
            "r",
            "[TIME]: continues the simulation",
            ConsoleCommandGroup::SIMULATION,
            [this](std::string& cmd_str) { return cmd_run(cmd_str); },
        },
        {
            "continue",
            "c",
            "alias for run",
            ConsoleCommandGroup::SIMULATION,
            [this](std::string& cmd_str) { return cmd_run(cmd_str); },
        },

        { "exit", "e", "exit debugger and continue simulation", ConsoleCommandGroup::SIMULATION, exec_type,
            [this](std::string& cmd_str) { return cmd_exit_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_exit_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_exit_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_exit_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return clear_watchlist(tokens); } },
        { "quit", "q", "alias for exit", ConsoleCommandGroup::SIMULATION, exec_type,
            [this](std::string& cmd_str) { return cmd_exit_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_exit_thread(cmd_str); },
            [this](std::string& cmd_str) { return cmd_exit_rank_serial(cmd_str); },
            [this](std::string& cmd_str) { return cmd_exit_rank_parallel(cmd_str); },
            [this](std::vector<std::string>& tokens) { return clear_watchlist(tokens); } },
        { "shutdown", "shutd", "exit the debugger and cleanly shutdown simulator", ConsoleCommandGroup::SIMULATION,
            [this](std::string& cmd_str) { return cmd_shutdown(cmd_str); } },
        // Logging/Replay
        { "logging", "log", "<filepath>: log command line entires to file", ConsoleCommandGroup::LOGGING,
            [this](std::string& cmd_str) { return cmd_logging(cmd_str); } },
        { "replay", "rep", "<filepath>: run commands from a file. See also: sst --replay", ConsoleCommandGroup::LOGGING,
            [this](std::string& cmd_str) { return cmd_replay(cmd_str); } },
        { "history", "h", "[N]: display all or last N unique commands", ConsoleCommandGroup::LOGGING,
            [this](std::string& cmd_str) { return cmd_history(cmd_str); } },
        // Misc
        { "autoComplete", "ac", "toggle command line auto-completion enable", ConsoleCommandGroup::MISC,
            [this](std::string& cmd_str) { return cmd_autoComplete(cmd_str); } },
        { "clear", "clr", "reset terminal", ConsoleCommandGroup::MISC,
            [this](std::string& cmd_str) { return cmd_clear(cmd_str); } },
        { "define", "def", "define a user command sequence", ConsoleCommandGroup::MISC,
            [this](std::string& cmd_str) { return cmd_define(cmd_str); } },
        { "document", "doc", "document help for a user defined command", ConsoleCommandGroup::MISC,
            [this](std::string& cmd_str) { return cmd_document(cmd_str); } },
    });

    // Detailed help from some commands. Can also add general things like 'help navigation'
    cmdRegistry.cmdHelp = {
        { "verbose", "[mask]: set verbosity mask or print if no mask specified\n"
                     "\tA mask is used to select which features to enable verbosity.\n"
                     "\tTo turn on all features set the mask to 0xffffffff\n"
                     "\t\t0x10: Show trigger details" },
        { "print", "[-rN][<obj>]: print objects in the current level of the object map\n"
                   "\tif -rN is provided print recursive N levels (default N=4)" },
        { "set", "<obj> <value>: sets an object in the current scope to the provided value\n"
                 "\tobject must be a 'fundamental type' (arithmetic or string)\n"
                 "\t e.g. set mystring hello world" },
        { "examine", "[e][<obj>]: prints object in the current scope\n" },
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
            "\t  interactive, printTrace, checkpoint, set <var> <val>, printStatus, or shutdown"
            "\t  Note: checkpoint action must be enabled at startup via the '--checkpoint-enable' command line "
            "option\n" },
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
                     "\tctrl-d: delete character at cursor or quit debugger\n"
                     "\tctrl-e: move cursor to end of line\n"
                     "\tctrl-f: move cursor to the right\n" },
        { "define", "<cmd-name>: enter a command sequence for a user defined command.\n"
                    "Terminate the sequence by typing \"end\"\n" },
        { "document", "<cmd-name>: provide help documentation for a user defined command.\n"
                      "The first line will be summarized in the short help text.\n"
                      "Remaining lines will be provided in detailed help\n"
                      "Terminate the sequence by typing \"end\"\n" },
    };

    // Command autofill strings
    std::list<std::string> cmdStrings;
    for ( const ConsoleCommand& c : cmdRegistry.getRegistryVector() ) {
        cmdStrings.emplace_back(c.str_long());
        cmdStrings.emplace_back(c.str_short());
    }
    cmdStrings.sort();
    cmdLineEditor.set_cmd_strings(cmdStrings);

    // Callback for directory listing strings
    cmdLineEditor.set_listing_callback([this](std::list<std::string>& vec) { get_listing_strings(vec); });

    struct winsize size;
    if ( ioctl(STDERR_FILENO, TIOCGWINSZ, &size) == 0 ) {
        dout.setLineWidth(size.ws_col);
        dout.setLineCount(size.ws_row);
    }
}

DebugConsole::~DebugConsole()
{
    if ( loggingFile.is_open() ) loggingFile.close();
    if ( replayFile.is_open() ) replayFile.close();
}

void
DebugConsole::summary()
{
    Simulation_impl* sim_ = Simulation_impl::getSimulation();
    result << "-- Rank:" << rank_.rank << "/" << num_ranks_.rank << " Thread:" << rank_.thread << "/"
           << num_ranks_.thread;
    //<< " (Process " << getpid() << ")";
    if ( sim_->enter_interactive_ ) {
        result << " (Triggered)" << std::endl;
    }
    else {
        result << " (Not Triggered)" << std::endl;
    }

    result << "-- Component Summary\n";
    SST::Core::Serialization::ObjectMap* baseObj = getComponentObjectMap();
    auto&                                vars    = baseObj->getVariables();
    for ( auto& x : vars ) {
        if ( x.second->isFundamental() ) {
            result << x.first << " = " << x.second->get() << " (" << x.second->getType() << ")" << std::endl;
        }
        else {
            result << x.first.c_str() << "/ (" << x.second->getType() << ")\n";
        }
    }
    result << std::endl;
}


int
DebugConsole::consoleExecute(const std::string& msg)
{

    struct winsize size;
    if ( ioctl(STDERR_FILENO, TIOCGWINSZ, &size) == 0 ) {
        dout.setLineWidth(size.ws_col);
        dout.setLineCount(size.ws_row);
    }

    std::cout << "---- Rank" << current_rank << ":Thread" << current_thread << ": Entering interactive mode at time "
              << getCurrentSimCycle() << std::endl;
    std::cout << msg << std::endl;

    // Create a new ObjectMap
    obj_ = getComponentObjectMap();

    // Descend into the name_stack
    cd_name_stack();

    exit_console = false;
    done         = false;
    retState     = DONE;

    // Select the input source and next command line
    std::string line;
    while ( !exit_console ) {
        try {
            // User input prompt (except during user command)
            if ( eStack.size() == 0 ) std::cout << "> " << std::flush;

            // Logging disable has edge cases for stack push/pop
            bool squashLogging = false;

            // User input prompt

            if ( !injectedCommand.str().empty() ) {
                // Injected commands allow sst command line options to cause actions (currently only replay)
                line = injectedCommand.str();
                injectedCommand.str("");
                std::cout << line << std::endl;
            }
            else if ( eStack.size() > 0 ) {
                // Do no log internals of user defined command
                squashLogging = true;
                // Execute next instruction in a user defined command
                line          = eState.next();
                if ( eState.ret() ) {
                    eState = eStack.top();
                    eStack.pop();
                    // back to normal command entry
                    if ( eStack.size() == 0 ) cmdHistoryBuf.enable(true);
                }
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

            // We have a constructed command line. Ship it
            dispatch_cmd(line);

            // Log commands if enabled and not executing a user defined command
            if ( enLogging && !squashLogging ) loggingFile << line.c_str() << std::endl;
            // This prevents logging the 'logging' command
            if ( loggingFile.is_open() ) enLogging = true;
        }
        catch ( const std::runtime_error& e ) {
            std::cout << "Parsing error. Ignoring " << line << std::endl;
        }
    }

    // Save the position on the name_stack, and clear obj_
    save_name_stack();
    done = true;
    return retState;
}

// Save the name stack of the current position, and clear obj_
void
DebugConsole::save_name_stack()
{
    name_stack.clear();
    for ( ;; ) {
        // Get the name of the current node
        std::string name = obj_->getName();

        // Get the parent of the current node
        Core::Serialization::ObjectMap* parent = obj_->selectParent();

        // If the parent is nullptr, we have reached the top and can stop
        if ( !parent ) break;

        // Push the name on the name_stack
        name_stack.push_front(std::move(name));

        // See if this is the top level component, and if so, set it to nullptr
        if ( dynamic_cast<Core::Serialization::ObjectMap*>(base_comp_) == obj_ ) base_comp_ = nullptr;

        // Move up to the parent
        obj_ = parent;
    }

    obj_->decRefCount();
    obj_ = nullptr;
}

// Descend into the name_stack
void
DebugConsole::cd_name_stack()
{
    for ( const std::string& name : name_stack ) {
        std::vector<std::string> tokens { "cd", name };
        if ( !cmd_cd_remote(tokens) ) break; // Stop if we cannot descend any further
    }
}

// Invoke the command.
// Substitution actions (!!, !?, ...) can modify the command.
// This ensure the final, resolved, command is captured in the command log
bool
DebugConsole::dispatch_cmd(std::string& cmd)
{
    // empty command
    if ( cmd.size() == 0 ) return true;

    tokenize(tokens, cmd);

    // just whitespace
    if ( tokens.size() == 0 ) return true;

    // comment
    if ( tokens[0][0] == '#' ) return true;

    // History !! and friends
    if ( tokens[0][0] == '!' ) {
        std::string newcmd;
        auto        rc = cmdHistoryBuf.bang(tokens[0], newcmd);
        if ( rc == CommandHistoryBuffer::BANG_RC::ECHO_ONLY ) {
            // replace, print, save command in history
            cmd = newcmd;
            std::cout << cmd << std::endl;
            cmdHistoryBuf.append(cmd);
            return true;
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
            return true;
        }
    }

    // Check for 'end' string to terminate special line entry modes
    if ( (line_entry_mode != LINE_ENTRY_MODE::NORMAL) && (cmd == "end") ) {
        if ( line_entry_mode == LINE_ENTRY_MODE::DEFINE )
            cmdRegistry.commitUserCommand();
        else if ( line_entry_mode == LINE_ENTRY_MODE::DOCUMENT )
            cmdRegistry.commitDocCommand();
        else {
            std::cout << "Error: unknown line entry mode" << std::endl;
            assert(false);
        }

        line_entry_mode = LINE_ENTRY_MODE::NORMAL;
        std::cout << "[ returning to normal line entry mode ]" << std::endl;
        return true;
    }

    // Do the right thing based on the entry mode
    switch ( line_entry_mode ) {
    case LINE_ENTRY_MODE::NORMAL:
    {
        bool succeed        = true;
        // normal execution
        auto consoleCommand = cmdRegistry.seek(tokens[0], CommandRegistry::SEARCH_TYPE::BUILTIN);
        if ( consoleCommand.second ) {
            // exec() will choose correct cmd_foo_* function based on exec_type
            succeed = consoleCommand.first.exec(cmd);
            cmdHistoryBuf.append(cmd);
            return succeed;
        }
        // user defined entry
        consoleCommand = cmdRegistry.seek(tokens[0], CommandRegistry::SEARCH_TYPE::USER);
        if ( consoleCommand.second ) {
            cmdHistoryBuf.append(cmd);
            // Do nothing if user command is empty
            if ( cmdRegistry.commandIsEmpty(tokens[0]) ) return true;
            // save current context
            eStack.push(eState);
            // new context for user call
            eState = { consoleCommand.first, tokens, cmdRegistry.userCommandInsts(tokens[0]) };
            // History capture disabled when stack size > 0
            cmdHistoryBuf.enable(false);
            return true;
        }

        // No matching command found but keep in history so we can fix it
        std::cout << "Unknown command: " << tokens[0].c_str() << std::endl;
        cmdHistoryBuf.append(cmd);
        return false;
    }
    case LINE_ENTRY_MODE::DEFINE:
        // entering a user defined command
        cmdRegistry.appendUserCommand(tokens[0], cmd);
        return true;
    case LINE_ENTRY_MODE::DOCUMENT:
        cmdRegistry.appendDocCommand(cmd);
        return true;
    default:
        std::cout << "INTERNAL ERROR: unhandled line entry mode" << std::endl;
        return false;
    } // switch (line_entry_mode)
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
DebugConsole::tokenize(std::vector<std::string>& tokens, const std::string& input)
{
    std::istringstream iss(input);
    std::string        token;

    // since tokens is now shared, clear it
    tokens.clear();
    while ( iss >> token ) {
        tokens.push_back(token);
    }

    return tokens;
}

bool
DebugConsole::cmd_help(std::string& UNUSED(cmd_str))
{
    // First check for specific command help
    if ( tokens.size() == 1 ) {
        for ( const auto& g : GroupText ) {
            if ( g.first != ConsoleCommandGroup::USER ) {
                std::cout << "--- " << g.second << " ---" << std::endl;
                for ( const auto& c : cmdRegistry.getRegistryVector() ) {
                    if ( g.first == c.group() ) std::cout << c << std::endl;
                }
            }
            else if ( cmdRegistry.getUserRegistryVector().size() > 0 ) {
                std::cout << "--- " << g.second << " ---" << std::endl;
                for ( const auto& c : cmdRegistry.getUserRegistryVector() ) {
                    if ( g.first == c.group() ) {
                        std::cout << c << std::endl;
                    }
                }
            }
        }
        std::cout << "\nMore detailed help available for:\n";
        std::stringstream s;
        for ( const auto& pair : cmdRegistry.cmdHelp ) {
            if ( (s.str().length() + pair.first.length() > 39) ) {
                std::cout << "\t" << s.str() << std::endl;
                s.str("");
                s.clear();
            }
            s << pair.first << " ";
        }
        std::cout << "\t" << s.str() << std::endl;
        std::cout << std::endl;
        return true;
    }

    if ( tokens.size() > 1 ) {
        std::string c = tokens[1];
        if ( cmdRegistry.cmdHelp.find(c) != cmdRegistry.cmdHelp.end() ) {
            std::cout << c << " " << cmdRegistry.cmdHelp.at(c) << std::endl;
        }
        else {
            for ( auto& creg : cmdRegistry.getRegistryVector() ) {
                if ( creg.match(c) ) std::cout << creg << std::endl;
            }
        }
    }
    return true;
}

// verbose [mask] : set verbosity mask or print if no mask specified
bool
DebugConsole::cmd_verbose_query()
{
    if ( tokens.size() > 1 ) {
        try {
            verbosity = SST::Core::from_string<uint32_t>(tokens[1]);
        }
        catch ( const std::invalid_argument& e ) {
            std::cout << "Invalid mask " << tokens[1] << std::endl;
            return false;
        }
    }

    // This messes up the auto-complete keyboard input
    std::cout << "verbose=0x" << std::hex << verbosity << std::endl;

    return true;
}

bool
DebugConsole::cmd_verbose_remote(std::vector<std::string>& tokens)
{
    verbosity = SST::Core::from_string<uint32_t>(tokens[1]);

    // update watchpoint verbosity in all ranks/threads
    for ( auto& x : watch_points_ ) {
        if ( x.first ) x.first->setVerbosity(verbosity);
    }
    result << "R" << rank_.rank << " T" << rank_.thread << ": verbose_remote" << std::endl;
    return true;
}

bool
DebugConsole::cmd_verbose_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    // Valid verbosity?
    if ( !confirm_ || cmd_verbose_query() ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        // Update verbosity for watchpoints
        succeed = cmd_verbose_remote(tokens);
        std::cout << result.str();
        return succeed;
    }

    return false;
}

bool
DebugConsole::cmd_verbose_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    // Valid verbosity?
    if ( !confirm_ || cmd_verbose_query() ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        // Update watchpoint verbosity for all rank 0 threads
        succeed = handleCommandAll();
        std::cout << result.str();
        return succeed;
    }

    return false;
}

bool
DebugConsole::cmd_verbose_rank_serial(std::string& cmd_str)
{
    // Valid verbosity?
    if ( !confirm_ || cmd_verbose_query() ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        // Update watchpoint verbosity for rank 0
        bool succeed = cmd_verbose_remote(tokens);
        std::cout << result.str();

        // Clear result string
        result.str("");
        result.clear();
        // Update watchpoint verbosity for other ranks
        bool succeed2 = sendCommandAll(cmd_str);
        std::cout << result.str();
        return succeed || succeed2;
    }

    return false;
}

bool
DebugConsole::cmd_verbose_rank_parallel(std::string& cmd_str)
{
    // Valid verbosity?
    if ( !confirm_ || cmd_verbose_query() ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        // Update watchpoint verbosity for rank 0
        bool succeed = handleCommandAll();
        std::cout << result.str();

        // Clear R0 result string
        result.str("");
        result.clear();
        // Update watchpoint verbosity for other ranks
        bool succeed2 = sendCommandAll(cmd_str);
        std::cout << result.str();
        return succeed || succeed2;
    }

    return false;
}

// info current|all
// print summary for current thread or all threads
bool
DebugConsole::cmd_info_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for info command (info \"current\"|\"all\")" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( tokens[1] == "current" || tokens[1] == "all" ) {
        succeed = cmd_info_remote(tokens);
        std::cout << result.str();
        return succeed;
    }
    else {
        std::cout << "Invalid argument for info command: " << tokens[1] << " (info \"current\"|\"all\")" << std::endl;
        return false;
    }
}

bool
DebugConsole::cmd_info_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for info command (info \"current\"|\"all\")" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( tokens[1] == "current" ) {
        succeed = handleCommand();
        std::cout << result.str();
    }
    else if ( tokens[1] == "all" ) {
        // Print info for all threads
        succeed = handleCommandAll();
        std::cout << result.str();
    }
    else {
        std::cout << "Invalid argument for info command: " << tokens[1] << " (info \"current\"|\"all\")" << std::endl;
        return false;
    }

    return succeed;
}

bool
DebugConsole::cmd_info_rank_serial(std::string& cmd_str)
{
    bool succeed, succeed2;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for info command (info \"current\"|\"all\")" << std::endl;
        return false;
    }

    if ( tokens[1] == "current" ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        if ( current_rank == 0 ) {
            succeed = cmd_info_remote(tokens);
            std::cout << result.str();
        }
        else {
            // Send to remote rank
            succeed = sendCommand(current_rank, current_thread, cmd_str);
            std::cout << result.str();
        }
        return succeed;
    }
    else if ( tokens[1] == "all" ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        // Print info for rank 0
        succeed = cmd_info_remote(tokens);
        std::cout << result.str();

        // Clear R0 result string
        result.str("");
        result.clear();
        // Send to remote ranks, all threads
        succeed2 = sendCommandAll(cmd_str);
        std::cout << result.str();
        return succeed || succeed2;
    }
    else {
        std::cout << "Invalid argument for info command: " << tokens[1] << " (info \"current\"|\"all\")" << std::endl;
        return false;
    }
}

bool
DebugConsole::cmd_info_rank_parallel(std::string& cmd_str)
{
    bool succeed, succeed2;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for info command (info \"current\"|\"all\")" << std::endl;
        return false;
    }

    // Must be executed by target rank because otherwise the process ID won't be correct
    if ( tokens[1] == "current" ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        if ( current_rank == 0 ) {
            succeed = handleCommand();
            std::cout << result.str();
        }
        else {
            succeed = sendCommand(current_rank, current_thread, cmd_str);
            std::cout << result.str();
        }
        return succeed;
    }
    else if ( tokens[1] == "all" ) {
        // Clear result string
        result.str("");
        result.clear();
        // Print info for rank 0
        succeed = handleCommandAll();
        std::cout << result.str();

        // Clear result string
        result.str("");
        result.clear();
        // Print info for other ranks, all threads
        succeed2 = sendCommandAll(cmd_str);
        std::cout << result.str();
        return succeed || succeed2;
    }
    else {
        std::cout << "Invalid argument for info command: " << tokens[1] << " (info \"current\"|\"all\")" << std::endl;
        return false;
    }
}

bool
DebugConsole::cmd_info_remote(std::vector<std::string>& UNUSED(tokens))
{
    result << "Rank " << rank_.rank << "/" << num_ranks_.rank << " Thread " << rank_.thread << "/" << num_ranks_.thread
           << " (Process " << getpid() << ")" << std::endl;
    return true;
}


// thread <threadID> : switches to new thread
int
DebugConsole::parse_thread()
{
    int threadID; // Used int because converting from string

    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for thread command (thread <threadID>)" << std::endl;
        return -1;
    }

    // Get threadID
    try {
        threadID = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cout << "Invalid argument for threadID: " << tokens[1] << std::endl;
        return -1;
    }
    catch ( const std::out_of_range& e ) {
        std::cout << "Out of range for threadID: " << tokens[1] << std::endl;
        return -1;
    }

    // Check if valid threadID
    if ( threadID < 0 || threadID >= static_cast<int>(num_ranks_.thread) ) {
        std::cout << "ThreadID " << threadID << " out of range (0:" << num_ranks_.thread - 1 << ")" << std::endl;
        return -1;
    }

    return threadID;
}


bool
DebugConsole::cmd_thread_serial(std::string& UNUSED(cmd_str))
{
    int threadID = parse_thread();

    if ( threadID == -1 ) return false;

    // Set current thread and get interactive msg
    if ( threadID != static_cast<int>(current_thread) ) {
        // Clear result string
        result.str("");
        result.clear();
        current_thread = threadID;
        std::cout << "---- Rank" << current_rank << ":Thread" << current_thread
                  << ": Entering interactive mode at time " << getCurrentSimCycle() << std::endl;
        cmd_thread_remote(tokens);
        std::cout << result.str();
        // May also need to do something to update the listings
        // and object map for the first time through
    }
    return true;
}

bool
DebugConsole::cmd_thread_thread(std::string& UNUSED(cmd_str))
{
    int threadID = parse_thread();
    if ( threadID == -1 ) return false;

    // Set current thread and get interactive msg
    if ( threadID != static_cast<int>(current_thread) ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        current_thread = threadID;
        std::cout << "---- Rank" << current_rank << ":Thread" << current_thread
                  << ": Entering interactive mode at time " << getCurrentSimCycle() << std::endl;
        handleCommand();
        std::cout << result.str();
        // May also need to do something to update the listings
        // and object map for the first time through
    }
    return true;
}

bool
DebugConsole::cmd_thread_rank_serial(std::string& cmd_str)
{
    int threadID = parse_thread();
    if ( threadID == -1 ) return false;

    // Set current thread and get interactive msg
    if ( threadID != static_cast<int>(current_thread) ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        current_thread = threadID;
        std::cout << "---- Rank" << current_rank << ":Thread" << current_thread
                  << ": Entering interactive mode at time " << getCurrentSimCycle() << std::endl;
        if ( current_rank == 0 ) {
            cmd_thread_remote(tokens);
            std::cout << result.str();
        }
        else {
            // Send to remote rank
            sendCommand(current_rank, current_thread, cmd_str);
            std::cout << result.str();
        }
        // May also need to do something to update the listings
        // and object map for the first time through
    }
    return true;
}

bool
DebugConsole::cmd_thread_rank_parallel(std::string& cmd_str)
{
    int threadID = parse_thread();
    if ( threadID == -1 ) return false;

    // Set current thread and get interactive msg
    if ( threadID != static_cast<int>(current_thread) ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        current_thread = threadID;
        std::cout << "---- Rank" << current_rank << ":Thread" << current_thread
                  << ": Entering interactive mode at time " << getCurrentSimCycle() << std::endl;
        if ( current_rank == 0 ) {
            handleCommand();
            std::cout << result.str();
        }
        else {
            // Send to remote rank
            sendCommand(current_rank, current_thread, cmd_str);
            std::cout << result.str();
        }
        // May also need to do something to update the listings
        // and object map for the first time through
    }
    return true;
}

bool
DebugConsole::cmd_thread_remote(std::vector<std::string>& UNUSED(tokens))
{
    result << Simulation_impl::getSimulation()->interactive_msg_ << std::endl;
    return true;
}

// rank <rankID> : switches to new rank (same thread)
int
DebugConsole::parse_rank()
{
    int rankID; // Used int because converting from string

    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for rankcommand (rank <rankID>)" << std::endl;
        return -1;
    }

    // Get rankID
    try {
        rankID = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cout << "Invalid argument for rankID: " << tokens[1] << std::endl;
        return -1;
    }
    catch ( const std::out_of_range& e ) {
        std::cout << "Out of range for rankID: " << tokens[1] << std::endl;
        return -1;
    }

    // Check if valid rankID
    if ( rankID < 0 || rankID >= static_cast<int>(num_ranks_.rank) ) {
        std::cout << "RankID " << rankID << " out of range (0:" << num_ranks_.rank - 1 << ")" << std::endl;

        return -1;
    }

    return rankID;
}

bool
DebugConsole::cmd_rank_serial(std::string& UNUSED(cmd_str))
{
    int rankID = parse_rank();
    if ( rankID == -1 ) return false;

    // Set current thread and get interactive msg
    if ( rankID != static_cast<int>(current_rank) ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        current_rank = rankID;
        std::cout << "---- Rank" << current_rank << ":Thread" << current_thread
                  << ": Entering interactive mode at time " << getCurrentSimCycle() << std::endl;
        cmd_rank_remote(tokens);
        std::cout << result.str();
        // May also need to do something to update the listings
        // and object map for the first time through
    }
    return true;
}

bool
DebugConsole::cmd_rank_thread(std::string& UNUSED(cmd_str))
{
    int rankID = parse_rank();
    if ( rankID == -1 ) return false;

    // Set current thread and get interactive msg
    if ( rankID != static_cast<int>(current_rank) ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        current_rank = rankID;
        std::cout << "---- Rank" << current_rank << ":Thread" << current_thread
                  << ": Entering interactive mode at time " << getCurrentSimCycle() << std::endl;
        handleCommand();
        std::cout << result.str();
        // May also need to do something to update the listings
        // and object map for the first time through
    }
    return true;
}

bool
DebugConsole::cmd_rank_rank_serial(std::string& cmd_str)
{
    int rankID = parse_rank();
    if ( rankID == -1 ) return false;

    // Set current thread and get interactive msg
    if ( rankID != static_cast<int>(current_rank) ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        current_rank = rankID;
        std::cout << "---- Rank" << current_rank << ":Thread" << current_thread
                  << ": Entering interactive mode at time " << getCurrentSimCycle() << std::endl;
        if ( current_rank == 0 ) {
            cmd_rank_remote(tokens);
            std::cout << result.str();
        }
        else {
            // Send to remote rank
            sendCommand(current_rank, current_thread, cmd_str);
            std::cout << result.str();
        }
        // May also need to do something to update the listings
        // and object map for the first time through
    }
    return true;
}

bool
DebugConsole::cmd_rank_rank_parallel(std::string& cmd_str)
{
    int rankID = parse_rank();
    if ( rankID == -1 ) return false;

    // Set current thread and get interactive msg
    if ( rankID != static_cast<int>(current_rank) ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        current_rank = rankID;
        std::cout << "---- Rank" << current_rank << ":Thread" << current_thread
                  << ": Entering interactive mode at time " << getCurrentSimCycle() << std::endl;
        if ( current_rank == 0 ) {
            handleCommand();
            std::cout << result.str();
        }
        else {
            // Send to remote rank
            sendCommand(current_rank, current_thread, cmd_str);
            std::cout << result.str();
        }
        // May also need to do something to update the listings
        // and object map for the first time through
    }
    return true;
}

bool
DebugConsole::cmd_rank_remote(std::vector<std::string>& UNUSED(tokens))
{
    result << Simulation_impl::getSimulation()->interactive_msg_ << std::endl;
    return true;
}

// pwd: print current working directory
bool
DebugConsole::cmd_pwd_serial(std::string& UNUSED(cmd_str))
{
    // Clear R0 result string
    result.str("");
    result.clear();

    cmd_pwd_remote(tokens);
    std::cout << result.str();

    return true;
}

bool
DebugConsole::cmd_pwd_thread(std::string& UNUSED(cmd_str))
{
    // Clear R0 result string
    result.str("");
    result.clear();

    handleCommand();
    std::cout << result.str();

    return true;
}

bool
DebugConsole::cmd_pwd_rank_serial(std::string& cmd_str)
{
    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        cmd_pwd_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return true;
}

bool
DebugConsole::cmd_pwd_rank_parallel(std::string& cmd_str)
{
    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return true;
}

bool
DebugConsole::cmd_pwd_remote(std::vector<std::string>& UNUSED(tokens))
{
    // std::string path = obj_->getName();
    // std::string slash("/");
    // // path = slash + path;
    // SST::Core::Serialization::ObjectMap* curr = obj_->getParent();
    // while ( curr != nullptr ) {
    //     path = curr->getName() + slash + path;
    //     curr = curr->getParent();
    // }

    result << obj_->getFullName() << " (" << obj_->getType() << ")\n";
    return true;
}

// ls: list current directory
bool
DebugConsole::cmd_ls_serial(std::string& UNUSED(cmd_str))
{
    // Clear R0 result string
    result.str("");
    result.clear();

    cmd_ls_remote(tokens);
    // Use debug out to support paging
    dout << result.str();
    dout << dreset;

    return true;
}

bool
DebugConsole::cmd_ls_thread(std::string& UNUSED(cmd_str))
{
    // Clear R0 result string
    result.str("");
    result.clear();

    handleCommand();
    // Use debug out to support paging
    dout << result.str();
    dout << dreset;

    return true;
}

bool
DebugConsole::cmd_ls_rank_serial(std::string& cmd_str)
{
    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        cmd_ls_remote(tokens);
        // Use debug out to support paging
        dout << result.str();
    }
    else {
        // Send to remote rank
        sendCommand(current_rank, current_thread, cmd_str);
        dout << result.str();
    }
    dout << dreset;
    return true;
}

bool
DebugConsole::cmd_ls_rank_parallel(std::string& cmd_str)
{
    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        handleCommand();
        // Use debug out to support pagin
        dout << result.str();
    }
    else {
        // Send to remote rank
        sendCommand(current_rank, current_thread, cmd_str);
        dout << result.str();
    }
    dout << dreset;
    return true;
}

bool
DebugConsole::cmd_ls_remote(std::vector<std::string>& UNUSED(tokens))
{
    auto& vars = obj_->getVariables();
    for ( auto& x : vars ) {
        if ( x.second->isFundamental() ) {
            result << x.first << " = " << x.second->get() << " (" << x.second->getType() << ")" << std::endl;
        }
        else {
            result << x.first.c_str() << "/ (" << x.second->getType() << ")\n";
        }
    }
    return true;
}

// Callback for autofill of object string (similar to ls)
void
DebugConsole::get_listing_strings(std::list<std::string>& list)
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
bool
DebugConsole::cmd_cd_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for cd command (cd <obj>)" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = cmd_cd_remote(tokens);
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_cd_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for cd command (cd <obj>)" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = handleCommand();
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_cd_rank_serial(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for cd command (cd <obj>)" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        succeed = cmd_cd_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return succeed;
}

bool
DebugConsole::cmd_cd_rank_parallel(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for cd command (cd <obj>)" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        succeed = handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return succeed;
}

bool
DebugConsole::cmd_cd_remote(std::vector<std::string>& tokens)
{
    // Allow for trailing '/'
    std::string selection = tokens[1];
    if ( !selection.empty() && selection.back() == '/' ) selection.pop_back();

    // Check for ..
    if ( selection == ".." ) {
        auto* parent = obj_->selectParent();
        if ( parent == nullptr ) {
            result << "Already at top of object hierarchy" << std::endl;
            return false;
        }

        // See if this is the top level component, and if so, set it to nullptr
        if ( dynamic_cast<Core::Serialization::ObjectMap*>(base_comp_) == obj_ ) base_comp_ = nullptr;

        obj_ = parent;
        return true;
    }

    bool                                 loop_detected = false;
    SST::Core::Serialization::ObjectMap* new_obj       = obj_->selectVariable(selection, loop_detected, confirm_);
    if ( !new_obj || (new_obj == obj_) ) {
        result << "Unknown object in cd command: " << selection << std::endl;
        return false;
    }

    if ( new_obj->isFundamental() ) {
        result << "Object" << selection << " is a fundamental type so you cannot cd into it" << std::endl;
        new_obj->selectParent();
        return false;
    }

    if ( loop_detected ) {
        result << "Loop detected in cd.  New working directory will be set to level of looped object: "
               << new_obj->getFullName() << std::endl;
    }
    obj_ = new_obj;

    // If we don't already have the top level component, check to see
    // if this is it
    if ( nullptr == base_comp_ ) {
        Core::Serialization::ObjectMapDeferred<BaseComponent>* base_comp =
            dynamic_cast<Core::Serialization::ObjectMapDeferred<BaseComponent>*>(obj_);
        if ( base_comp ) base_comp_ = base_comp;
    }
    return true;
}

// time: print current simulation cycle
bool
DebugConsole::cmd_time(std::string& UNUSED(cmd_str))
{
    std::cout << "current time = " << getCurrentSimCycle() << std::endl;
    return true;
}

// print [-rN] [<obj>]: print object
bool
DebugConsole::cmd_print_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 2 ) {
        std::cout << "Invalid format for print command (print [-rN] [<obj>])" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = cmd_print_remote(tokens);
    std::cout << result.str();
    return succeed;
}

bool
DebugConsole::cmd_print_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 2 ) {
        std::cout << "Invalid format for print command (print [-rN] [<obj>])" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = handleCommand();
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_print_rank_serial(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 2 ) {
        std::cout << "Invalid format for print command (print [-rN] [<obj>])" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        succeed = cmd_ls_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return succeed;
}

bool
DebugConsole::cmd_print_rank_parallel(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 2 ) {
        std::cout << "Invalid format for print command (print [-rN] [<obj>])" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        succeed = handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return succeed;
}

bool
DebugConsole::cmd_print_remote(std::vector<std::string>& tokens)
{
    // Index in tokens array where we may find the variable name
    size_t var_index = 1;

    // See if have a -r or not
    int         recurse = 4; // default -r depth
    std::string tok     = tokens[1];
    if ( (tok.size() >= 2) && (tok[0] == '-') && (tok[1] == 'r') ) {
        // Got a -r
        std::string num = tok.substr(2);
        if ( num.size() != 0 ) {
            try {
                recurse = SST::Core::from_string<int>(num);
            }
            catch ( const std::invalid_argument& e ) {
                result << "Invalid number format specified with -r: " << tok << std::endl;
                return false;
            }
        }
        else {
            std::string num = tok.substr(2);
            if ( num.size() != 0 ) {
                try {
                    recurse = SST::Core::from_string<int>(num);
                }
                catch ( std::invalid_argument& e ) {
                    result << "Invalid number format specified with -r: " << tok << std::endl;
                    return false;
                }
            }
        }
        var_index = 2;
    }

    if ( tokens.size() == var_index ) {
        // Print current object
        obj_->list(recurse);
        return true;
    }

    if ( tokens.size() != (var_index + 1) ) {
        result << "Invalid format for print command (print [-rN] [<obj>])" << std::endl;
        return false;
    }

    bool        found;
    std::string listing = obj_->listVariable(tokens[var_index], found, recurse);
    if ( !found ) {
        result << "Unknown object in print command: " << tokens[var_index] << std::endl;
        return false;
    }
    else {
        result << listing;
    }
    return true;
}

// set <obj> <value>: set object to value
bool
DebugConsole::cmd_set_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format for set command (set <obj> <value>)" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = cmd_set_remote(tokens);
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_set_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format for set command (set <obj> <value>)" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = handleCommand();
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_set_rank_serial(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format for set command (set <obj> <value>)" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        succeed = cmd_set_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return succeed;
}

bool
DebugConsole::cmd_set_rank_parallel(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format for set command (set <obj> <value>)" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        succeed = handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return succeed;
}

bool
DebugConsole::cmd_set_remote(std::vector<std::string>& tokens)
{
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
        if ( !found ) {
            result << "Unknown object in set command for container: " << tokens[1] << std::endl;
            return false;
        }
        if ( read_only ) {
            result << "Object specified in set command is read-only for container: " << tokens[1] << std::endl;
            return false;
        }
        // TODO do we need var->selectParent() here?
        return true;
    }

    bool  loop_detected = false;
    auto* var           = obj_->selectVariable(tokens[1], loop_detected);
    assert(var);
    if ( !var || (var == obj_) ) {
        result << "Unknown object in set command: " << tokens[1] << std::endl;
        // TODO make sure selectVariable hasn't altered any state.
        return false;
    }

    // Once we have a valid object, be sure to use var->selectParent() or
    // future commands may attempt to use free'd memory.

    if ( var->isReadOnly() ) {
        result << "Object specified in set command is read-only: " << tokens[1] << std::endl;
        var->selectParent();
        return false;
    }

    if ( !var->isFundamental() ) {
        result << "Invalid object in set command: " << tokens[1] << " is not a fundamental type" << std::endl;
        var->selectParent();
        return false;
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
    catch ( const std::exception& e ) {
        result << "Invalid format: " << tokens[2] << std::endl;
        return false;
    }
    var->selectParent();
    return true;
}

// run <time>: run simulation for time
bool
DebugConsole::cmd_run(std::string& UNUSED(cmd_str))
{
    if ( tokens.size() == 2 ) {
        try {
            TimeConverter tc  = getTimeConverter(tokens[1]);
            std::string   msg = format_string("Ran clock for %" PRI_SIMTIME " sim cycles", tc.getFactor());
            schedule_interactive(tc.getFactor(), msg);
        }
        catch ( const std::exception& e ) {
            std::cout << "Unknown time in call to run: " << tokens[1] << std::endl;
            return false;
        }
    }
    else if ( tokens.size() == 3 ) {
        std::string time = tokens[1] + tokens[2];
        try {
            TimeConverter tc  = getTimeConverter(time);
            std::string   msg = format_string("Ran clock for %" PRI_SIMTIME " sim cycles", tc.getFactor());
            schedule_interactive(tc.getFactor(), msg);
        }
        catch ( std::exception& e ) {
            std::cout << "Unknown time in call to run: " << time << std::endl;
            return false;
        }
    }
    else if ( tokens.size() != 1 ) {
        std::cout << "Too many arguments for 'run <time>'" << std::endl;
        return false;
    }
    exit_console = true;
    return true;
}

// setHandler <wpIndex> <handlerType1> ... <handlerTypeN>
// set where to do trigger checks and sampling (before/after clock/event handler)
bool
DebugConsole::cmd_setHandler_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format: setHandler <watchpointIndex> <handlerType1> ... <handlerTypeN>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = cmd_setHandler_remote(tokens);
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_setHandler_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format: setHandler <watchpointIndex> <handlerType1> ... <handlerTypeN>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = handleCommand();
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_setHandler_rank_serial(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format: setHandler <watchpointIndex> <handlerType1> ... <handlerTypeN>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        succeed = cmd_setHandler_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return succeed;
}

bool
DebugConsole::cmd_setHandler_rank_parallel(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format: setHandler <watchpointIndex> <handlerType1> ... <handlerTypeN>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        succeed = handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return succeed;
}

bool
DebugConsole::cmd_setHandler_remote(std::vector<std::string>& tokens)
{
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        result << "Invalid argument for buffer size: " << tokens[5] << std::endl;
        return false;
    }
    catch ( const std::out_of_range& e ) {
        result << "Out of range for buffer size: " << tokens[5] << std::endl;
        return false;
    }
    if ( wpIndex >= watch_points_.size() ) {
        result << "Invalid watchpoint index: " << wpIndex << std::endl;
        return false;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    if ( wp == nullptr ) {
        result << "Invalid watchpoint index: " << wpIndex << std::endl;
        return false;
    }
    result << "WP " << wpIndex << " - " << wp->getName() << std::endl;

    // Get handlerTypes and add associated objectBuffers
    size_t   tindex  = 2;
    unsigned handler = 0;
    while ( tindex < tokens.size() ) {
        const std::string& type = tokens[tindex++];

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
            result << " Invalid handler type: " << type << std::endl;
    }
    if ( handler ) {
        wp->setHandler(handler);
        return true;
    }
    return false;
}

// addTraceVar <wpIndex> <var1> ... <varN>
bool
DebugConsole::cmd_addTraceVar_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format: addTraceVar <watchpointIndex> <var1> ... <varN>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = cmd_addTraceVar_remote(tokens);
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_addTraceVar_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format: addTraceVar <watchpointIndex> <var1> ... <varN>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = handleCommand();
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_addTraceVar_rank_serial(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format: addTraceVar <watchpointIndex> <var1> ... <varN>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        succeed = cmd_addTraceVar_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return succeed;
}

bool
DebugConsole::cmd_addTraceVar_rank_parallel(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format: addTraceVar <watchpointIndex> <var1> ... <varN>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        succeed = handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return succeed;
}

bool
DebugConsole::cmd_addTraceVar_remote(std::vector<std::string>& tokens)
{
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        result << "Invalid argument for buffer size: " << tokens[5] << std::endl;
        return false;
    }
    catch ( const std::out_of_range& e ) {
        result << "Out of range for buffer size: " << tokens[5] << std::endl;
        return false;
    }
    if ( wpIndex >= watch_points_.size() ) {
        result << " Invalid watchpoint index: " << wpIndex << std::endl;
        return false;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    if ( wp == nullptr ) {
        result << " Invalid watchpoint index: " << wpIndex << std::endl;
        return false;
    }
    result << "WP " << wpIndex << " - " << wp->getName() << std::endl;

    // Get trace vars and add associated objectBuffers
    size_t tindex = 2;
    while ( tindex < tokens.size() ) {
        const std::string& tvar = tokens[tindex++];

        // Find and check trace variable
        Core::Serialization::ObjectMap* map = obj_->findVariable(tvar);
        if ( nullptr == map ) {
            result << "Unknown variable: " << tvar << std::endl;
            return false;
        }

        // Is variable fundamental
        if ( !map->isFundamental() ) {
            result << "Traces can only be placed on fundamental types; " << tvar << " is not fundamental" << std::endl;
            return false;
        }
        size_t bufsize = wp->getBufferSize();
        if ( bufsize == 0 ) {
            result << "Watchpoint " << wpIndex << " does not have tracing enabled" << std::endl;
            return false;
        }
        auto* ob = map->getObjectBuffer(obj_->getFullName() + "/" + tvar, bufsize);
        wp->addObjectBuffer(ob);
    }
    return true;
}

// resetTraceBuffer <wpIndex> : resets trace buffer for indexed watchpoint
bool
DebugConsole::cmd_resetTraceBuffer_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: resetTraceBuffer <watchpointIndex>\n";
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = cmd_resetTraceBuffer_remote(tokens);
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_resetTraceBuffer_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: resetTraceBuffer <watchpointIndex>\n";
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = handleCommand();
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_resetTraceBuffer_rank_serial(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: resetTraceBuffer <watchpointIndex>\n";
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        succeed = cmd_resetTraceBuffer_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return succeed;
}

bool
DebugConsole::cmd_resetTraceBuffer_rank_parallel(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: resetTraceBuffer <watchpointIndex>\n";
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        succeed = handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return succeed;
}
bool
DebugConsole::cmd_resetTraceBuffer_remote(std::vector<std::string>& tokens)
{
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        result << "Invalid argument for buffer size: " << tokens[5] << std::endl;
        return false;
    }
    catch ( const std::out_of_range& e ) {
        result << "Out of range for buffer size: " << tokens[5] << std::endl;
        return false;
    }
    if ( wpIndex >= watch_points_.size() ) {
        result << "Invalid watchpoint index: " << wpIndex << std::endl;
        return false;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    if ( wp == nullptr ) {
        result << "Invalid watchpoint index: " << wpIndex << std::endl;
        return false;
    }
    wp->resetTraceBuffer();

    return true;
}

// printTrace <wpIndex> : prints trace buffer for indexed watchpoint
bool
DebugConsole::cmd_printTrace_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: printTrace <watchpointIndex>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = cmd_printTrace_remote(tokens);
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_printTrace_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: printTrace <watchpointIndex>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = handleCommand();
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_printTrace_rank_serial(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: printTrace <watchpointIndex>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        succeed = cmd_printTrace_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return succeed;
}

bool
DebugConsole::cmd_printTrace_rank_parallel(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: printTrace <watchpointIndex>" << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        succeed = handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return succeed;
}

bool
DebugConsole::cmd_printTrace_remote(std::vector<std::string>& tokens)
{
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        result << "Invalid argument for buffer size: " << tokens[5] << std::endl;
        return false;
    }
    catch ( const std::out_of_range& e ) {
        result << "Out of range for buffer size: " << tokens[5] << std::endl;
        return false;
    }
    if ( wpIndex >= watch_points_.size() ) {
        result << "Invalid watchpoint index: " << wpIndex << std::endl;
        return false;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    if ( wp == nullptr ) {
        result << "Invalid watchpoint index: " << wpIndex << std::endl;
        return false;
    }

    wp->printTrace();

    return true;
}

// printWatchpoint <wpIndex> : prints detailed info for indexed watchpoint
bool
DebugConsole::cmd_printWatchpoint_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: printWatchpoint <watchpointIndex>\n";
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = cmd_printWatchpoint_remote(tokens);
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_printWatchpoint_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: printWatchpoint <watchpointIndex>\n";
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = handleCommand();
    std::cout << result.str();
    return succeed;
}

bool
DebugConsole::cmd_printWatchpoint_rank_serial(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: printWatchpoint <watchpointIndex>\n";
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        succeed = cmd_printWatchpoint_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return succeed;
}

bool
DebugConsole::cmd_printWatchpoint_rank_parallel(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format: printWatchpoint <watchpointIndex>\n";
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        succeed = handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return succeed;
}

bool
DebugConsole::cmd_printWatchpoint_remote(std::vector<std::string>& tokens)
{
    size_t wpIndex = watch_points_.size();
    try {
        wpIndex = std::stoi(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        result << "Invalid argument for buffer size: " << tokens[5] << std::endl;
        return false;
    }
    catch ( const std::out_of_range& e ) {
        result << "Out of range for buffer size: " << tokens[5] << std::endl;
        return false;
    }
    if ( wpIndex >= watch_points_.size() ) {
        result << "Invalid watchpoint index: " << wpIndex << std::endl;
        return false;
    }

    WatchPoint* wp = watch_points_.at(wpIndex).first;
    if ( wp == nullptr ) {
        result << "Invalid watchpoint index: " << wpIndex << std::endl;
        return false;
    }
    else {
        result << "WP" << wpIndex << ": ";
        wp->printWatchpoint(result);
    }

    return true;
}

// logging <filepath> : logs commands to the specified file
bool
DebugConsole::cmd_logging(std::string& UNUSED(cmd_str))
{
    if ( loggingFile.is_open() ) {
        std::cout << "Logging file is already set to " << loggingFilePath << std::endl;
        return false;
    }
    if ( tokens.size() > 1 ) {
        loggingFilePath = tokens[1];
    }
    // Attempt to open an SST output file
    loggingFile.open(loggingFilePath);
    if ( !loggingFile.is_open() ) {
        std::cout << "Could not open %s\n" << loggingFilePath.c_str() << std::endl;
        return false;
    }
    std::cout << "sst console commands will be logged to " << loggingFilePath << std::endl;
    return true;
}

// replay <filepath> : replays commands from the specified file
bool
DebugConsole::cmd_replay(std::string& UNUSED(cmd_str))
{
    if ( replayFile.is_open() ) {
        std::cout << "Replay file is already set to " << replayFilePath << std::endl;
        return false;
    }
    if ( tokens.size() > 1 ) {
        replayFilePath = tokens[1];
    }

    replayFile.open(replayFilePath);
    if ( !replayFile.is_open() ) {
        std::cout << "Could not open replay file: " << replayFilePath << std::endl;
        return false;
    }

    return true;
}

// history : prints command history
bool
DebugConsole::cmd_history(std::string& UNUSED(cmd_str))
{
    int recs = 0; // 0 indicates all history
    if ( tokens.size() > 1 ) {
        try {
            recs = (int)SST::Core::from_string<int>(tokens[1]);
        }
        catch ( const std::invalid_argument& e ) {
            std::cout << "history: Ignoring arg1 (" << tokens[1] << ")" << std::endl;
        }
    }
    cmdHistoryBuf.print(recs);
    return true;
}

// helper function for parsing logic operator
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

// watchlist : prints the watchlists from all threads
bool
DebugConsole::cmd_watchlist_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = cmd_watchlist_remote(tokens);
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_watchlist_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = handleCommandAll();
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_watchlist_rank_serial(std::string& cmd_str)
{
    // Clear R0 result string
    result.str("");
    result.clear();
    bool succeed = cmd_watchlist_remote(tokens);
    std::cout << result.str();

    // Send to remote rank
    result.str("");
    result.clear();
    bool succeed2 = sendCommandAll(cmd_str);
    std::cout << result.str();

    return succeed || succeed2;
}

bool
DebugConsole::cmd_watchlist_rank_parallel(std::string& cmd_str)
{
    // Clear R0 result string
    result.str("");
    result.clear();
    // Handle local threads
    bool succeed = handleCommandAll();
    std::cout << result.str();

    // Clear R0 result string
    result.str("");
    result.clear();
    // Send to remote ranks
    bool succeed2 = sendCommandAll(cmd_str);
    std::cout << result.str();

    return succeed || succeed2;
}

bool
DebugConsole::cmd_watchlist_remote(std::vector<std::string>& UNUSED(tokens))
{
    // Print the watch points
    result << "R" << rank_.rank << ",T" << rank_.thread << ": Current watch points:" << std::endl;
    int count = 0;
    for ( auto& x : watch_points_ ) {
        if ( x.first == nullptr ) {
            count++;
        }
        else {
            result << count++ << ": ";
            x.first->printWatchpoint(result);
        }
    }
    return true;
}

// autoComplete : toggle command line auto-completion enable
bool
DebugConsole::cmd_autoComplete(std::string& UNUSED(cmd_str))
{
    autoCompleteEnable = !autoCompleteEnable;
    std::cout << "auto completion is now " << autoCompleteEnable << std::endl;
    return true;
}

// clear : reset terminal
bool
DebugConsole::cmd_clear(std::string& UNUSED(cmd_str))
{
    // clear screen and move cursor to (0,0)
    std::cout << "\033[2J\033[1;1H";
    return true;
}

// define <foo> : define a user command sequence
bool
DebugConsole::cmd_define(std::string& UNUSED(cmd_str))
{
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid\nsyntax: define <cmd_name>" << std::endl;
        return false;
    }

    // Create a user command entry (or clear existing one)
    if ( cmdRegistry.beginUserCommand(tokens[1]) ) line_entry_mode = LINE_ENTRY_MODE::DEFINE;

    return true;
}

// document <foo> : document help for a user defined command
bool
DebugConsole::cmd_document(std::string& UNUSED(cmd_str))
{
    if ( tokens.size() != 2 ) {
        std::cout << "Invalid\nsyntax: document <cmd_name>" << std::endl;
        return false;
    }
    if ( cmdRegistry.beginDocCommand(tokens[1]) ) line_entry_mode = LINE_ENTRY_MODE::DOCUMENT;

    return true;
}

// helper function to parse watchpoint comparison string
Core::Serialization::ObjectMapComparison*
parseComparison(std::vector<std::string>& tokens, size_t& index, Core::Serialization::ObjectMap* obj, std::string& name)
{
    // Get first comparison
    const std::string& var = tokens[index++];
    if ( index >= tokens.size() ) {
        std::cout << "Invalid format for trigger test" << std::endl;
        return nullptr;
    }
    const std::string&                           opstr = tokens[index++];
    Core::Serialization::ObjectMapComparison::Op op =
        Core::Serialization::ObjectMapComparison::getOperationFromString(opstr);

    std::string v2;
    if ( op != Core::Serialization::ObjectMapComparison::Op::CHANGED ) {
        if ( index >= tokens.size() ) {
            std::cout << "Invalid format for trigger test. Valid formats are <var> changed and <var> <op> <val>"
                      << std::endl;
            return nullptr;
        }
        v2 = tokens[index++];
    }

    // Check for errors and build ObjectMapComparison
    // Look for variable
    Core::Serialization::ObjectMap* map = obj->findVariable(var);

    // Valid variable name
    if ( nullptr == map ) {
        std::cout << "Unknown variable: " << var << std::endl;
        return nullptr;
    }

    // Is variable fundamental
    if ( !map->isFundamental() ) {
        std::cout << "Triggers can only use fundamental types; " << var << " is not fundamental" << std::endl;
        return nullptr;
    }

    // Is operator valid
    if ( op == Core::Serialization::ObjectMapComparison::Op::INVALID ) {
        std::cout << "Unknown comparison operation specified in trigger test" << std::endl;
        return nullptr;
    }

    name = obj->getFullName() + "/" + var;

    // In changed mode, we do not use v2
    if ( op == Core::Serialization::ObjectMapComparison::Op::CHANGED ) {
        try {
            return map->getComparison(name, op, ""); // Can throw an exception
        }
        catch ( const std::exception& e ) {
            std::cout << "Invalid argument passed to trigger test: " << var << " " << opstr << std::endl;
            return nullptr;
        }
    }

    // Check if v2 is a variable
    Core::Serialization::ObjectMap* map2 = obj->findVariable(v2);

    // V2 is valid variable
    if ( nullptr != map2 ) {

        // Is variable fundamental
        if ( !map2->isFundamental() ) {
            std::cout << "Triggers can only use fundamental types; " << var << " is not fundamental" << std::endl;
            return nullptr;
        }

        std::string name2 = obj->getFullName() + "/" + v2;
        try {
            return map->getComparisonVar(name, op, name2, map2); // Can throw an exception
        }
        catch ( const std::exception& e ) {
            std::cout << "Invalid argument passed to trigger test: " << var << " " << opstr << " " << v2 << std::endl;
            return nullptr;
        }
    }
    else { // V2 is value string
        try {
            return map->getComparison(name, op, v2); // Can throw an exception
        }
        catch ( const std::exception& e ) {
            std::cout << "Invalid argument passed to trigger test: " << var << " " << opstr << " " << v2 << std::endl;
            return nullptr;
        }
    }
}

// helper function to parse watchpoint action string
WatchPoint::WPAction*
parseAction(std::vector<std::string>& tokens, size_t& index, Core::Serialization::ObjectMap* obj)
{
    const std::string& action = tokens[index++];

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
            std::cout << "Missing variable for set command" << std::endl;
            return nullptr;
        }
        const std::string& tvar = tokens[index++];
        if ( index >= tokens.size() ) {
            std::cout << "Missing variable for set command" << std::endl;
            return nullptr;
        }
        const std::string& tval = tokens[index++];

        // Find and check variable
        Core::Serialization::ObjectMap* map = obj->findVariable(tvar);
        if ( nullptr == map ) {
            std::cout << "Unknown variable: " << tvar << std::endl;
            return nullptr;
        }

        // Is variable fundamental
        if ( !map->isFundamental() ) {
            std::cout << "Can only set fundamental variable, " << tvar << " is not fundamental" << std::endl;
            return nullptr;
        }

        // Is variable read-only
        if ( map->isReadOnly() ) {
            std::cout << "Object specified in set command is read-only: " << tvar << std::endl;
            return nullptr;
        }

        // Check for valid value
        if ( !map->checkValue(tval) ) {
            return nullptr;
        }

        std::string name = obj->getFullName() + "/" + tvar;

        return new WatchPoint::SetVarWPAction(name, map, tval);
    }
#if 0 // Do users want a heartbeat action?
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
bool
DebugConsole::cmd_watch_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format for watch command" << std::endl;
        return false;
    }
    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = cmd_watch_remote(tokens);
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_watch_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format for watch command" << std::endl;
        return false;
    }
    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = handleCommand();
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_watch_rank_serial(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format for watch command" << std::endl;
        return false;
    }
    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        succeed = cmd_watch_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return succeed;
}

bool
DebugConsole::cmd_watch_rank_parallel(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 3 ) {
        std::cout << "Invalid format for watch command" << std::endl;
        return false;
    }
    // Clear result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        succeed = handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return succeed;
}

bool
DebugConsole::cmd_watch_remote(std::vector<std::string>& tokens)
{
    size_t      index = 1;
    std::string name  = "";

    try {
        // Get first comparison
        Core::Serialization::ObjectMapComparison* c = parseComparison(tokens, index, obj_, name);
        if ( c == nullptr ) {
            result << "Invalid comparison argument passed to watch command" << std::endl;
            return false;
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
                result << "Invalid logic operator: " << tokens[index - 1] << std::endl;
                return false;
            }
            else {
                pt->addLogicOp(logicOp);
            }
            if ( index == tokens.size() ) {
                result << "Invalid format for watch command" << std::endl;
                return false;
            }

            // Get next comparison
            Core::Serialization::ObjectMapComparison* c = parseComparison(tokens, index, obj_, name);
            if ( c == nullptr ) {
                result << "Invalid comparison argument passed to watch command" << std::endl;
                return false;
            }
            pt->addComparison(c);

        } // while index < tokens.size(), add another logic op and test comparision

        // Parse action
        std::string           action    = "interactive";
        WatchPoint::WPAction* actionObj = new WatchPoint::InteractiveWPAction();
        if ( actionObj == nullptr ) {
            result << "Error in action: " << action << std::endl;
            return false;
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
            result << "Added watchpoint #" << wpIndex << std::endl;
        }
        else {
            result << "Not a component" << std::endl;
            return false;
        }
    } // try/catch  TODO: need to revisit what can actually throw an exception
    catch ( const std::exception& e ) {
        result << "Invalid format for watch command" << std::endl;
        return false;
    }

    // Check for extra arguments
    if ( index != tokens.size() ) {
        result << "Invalid format for watch command: too many arguments" << std::endl;
        return false;
    }

    return true;
}

// confirm <true/false> : set confirmation requests on (default) or off
bool
DebugConsole::cmd_setConfirm(std::string& UNUSED(cmd_str))
{

    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for confirm command: confirm <true/false>\n";
        return false;
    }

    if ( (tokens[1] == "true") || (tokens[1] == "t") || (tokens[1] == "T") || (tokens[1] == "1") ) {
        confirm_ = true;
        dout.setConfirm(true);
        return true;
    }
    else if ( (tokens[1] == "false") || (tokens[1] == "f") || (tokens[1] == "F") || (tokens[1] == "0") ) {
        confirm_ = false;
        dout.setConfirm(false);
        return true;
    }

    std::cout << "Invalid argument for confirm: must be true or false. <" << tokens[1] << ">" << std::endl;
    return false;
}

// prompt user to delete all watchpoints
bool
DebugConsole::query_clear_watchlist()
{
    if ( confirm_ ) {
        std::string line;
        std::cout << "Do you want to delete all watchpoints? [yes, no]\n";
        std::getline(std::cin, line);
        std::vector<std::string> tokens;
        tokenize(tokens, line);

        if ( tokens.size() == 0 ) return false;
        if ( !(tokens[0] == "yes") ) return false;
        return true;
    }
    else {
        return false;
    }
}

// clear all watchlists at all threads
bool
DebugConsole::clear_watchlist(std::vector<std::string>& UNUSED(tokens))
{
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

// unwatch <wpIndex> : remove 1 or all watchpoints
bool
DebugConsole::cmd_unwatch_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    // If no arguments, unwatch all watchpoints
    if ( tokens.size() == 1 ) {
        if ( !confirm_ || query_clear_watchlist() ) {
            succeed = clear_watchlist(tokens);
            std::cout << "Watchlist cleared\n";
            return succeed;
        }
        return true;
    }

    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for unwatch command" << std::endl;
        return false;
    }

    long unsigned int index = 0;

    try {
        index = (long unsigned int)SST::Core::from_string<int>(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        std::cout << "Invalid index format specified. The unwatch command requires"
                     "a watchpoint index from the \"watchlist\" command"
                  << std::endl;
        return false;
    }

    if ( watch_points_.size() <= index ) {
        std::cout << "Watch point " << tokens[1]
                  << " not found. The unwatch command requires"
                     "a watchpoint index from the \"watchlist\" command"
                  << std::endl;
        return false;
    }

    WatchPoint* pt = watch_points_[index].first;
    if ( pt != nullptr ) { // already removed
        BaseComponent* comp = watch_points_[index].second;

        // Remove and mark as unused
        comp->removeWatchPoint(pt);
        watch_points_[index].first  = nullptr;
        watch_points_[index].second = nullptr;
    }
    return true;
}


bool
DebugConsole::cmd_unwatch_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    // If no arguments, unwatch all watchpoints
    if ( tokens.size() == 1 ) {
        if ( !confirm_ || query_clear_watchlist() ) {
            // Clear R0 result string
            result.str("");
            result.clear();
            succeed = handleCommandAll(); // clear all watchpoints
            std::cout << result.str();
            return succeed;
        }
        return true;
    }

    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for unwatch command" << std::endl;
        return false;
    }
    // Clear result string
    result.str("");
    result.clear();

    // Clear specific watchpoint
    succeed = handleCommand();
    std::cout << result.str();

    return succeed;
}

bool
DebugConsole::cmd_unwatch_rank_serial(std::string& cmd_str)
{
    bool succeed, succeed2;
    // If no arguments, unwatch all watchpoints
    if ( tokens.size() == 1 ) {
        if ( !confirm_ || query_clear_watchlist() ) {
            // Clear R0 result string
            result.str("");
            result.clear();

            // Clear local watchlist
            succeed  = cmd_unwatch_remote(tokens);
            // Send to remote ranks
            succeed2 = sendCommandAll(cmd_str);
            std::cout << result.str();
            return succeed || succeed2;
        }
        return true;
    }

    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for unwatch command" << std::endl;
        return false;
    }
    // Clear R0 result string
    result.str("");
    result.clear();

    // Clear specific watchpoint
    if ( current_rank == 0 ) {
        succeed = cmd_unwatch_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return succeed;
}

bool
DebugConsole::cmd_unwatch_rank_parallel(std::string& cmd_str)
{
    bool succeed, succeed2;
    // If no arguments, unwatch all watchpoints
    if ( tokens.size() == 1 ) {
        if ( !confirm_ || query_clear_watchlist() ) {
            // Clear R0 result string
            result.str("");
            result.clear();
            // Clear local watchlists
            succeed = handleCommandAll();
            std::cout << result.str();

            result.str("");
            result.clear();
            // Send to remote ranks
            succeed2 = sendCommandAll(cmd_str);
            std::cout << result.str();
            return succeed || succeed2;
        }
        return true;
    }

    if ( tokens.size() != 2 ) {
        std::cout << "Invalid format for unwatch command" << std::endl;
        return false;
    }
    // Clear R0 result string
    result.str("");
    result.clear();

    // Clear specific watchpoint
    if ( current_rank == 0 ) {
        succeed = handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return succeed;
}

bool
DebugConsole::cmd_unwatch_remote(std::vector<std::string>& tokens)
{
    // If no arguments, unwatch all watchpoints
    if ( tokens.size() == 1 ) {
        clear_watchlist(tokens);
        result << "Watchlist cleared\n";
        return true;
    }

    // Remove specified watchpoint
    long unsigned int index = 0;
    try {
        index = (long unsigned int)SST::Core::from_string<int>(tokens[1]);
    }
    catch ( const std::invalid_argument& e ) {
        result << "Invalid index format specified. The unwatch command requires"
                  "a watchpoint index from the \"watchlist\" command"
               << std::endl;
        return false;
    }

    if ( watch_points_.size() <= index ) {
        result << "Watch point " << tokens[1]
               << " not found. The unwatch command "
                  "requires a watchpoint index from the \"watchlist\" command"
               << std::endl;
        return false;
    }

    WatchPoint* pt = watch_points_[index].first;
    if ( pt != nullptr ) { // already removed
        BaseComponent* comp = watch_points_[index].second;

        // Remove and mark as unused
        comp->removeWatchPoint(pt);
        watch_points_[index].first  = nullptr;
        watch_points_[index].second = nullptr;
    }
    return true;
}

// parse trace buffer string to create new trace
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
    catch ( const std::exception& e ) {
        std::cout << "Invalid buffer argument passed to trace command\n";
        return nullptr;
    }
}

// Parse trace variable string
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
bool
DebugConsole::cmd_trace_serial(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 9 ) {
        std::cout << "Invalid format: trace <var> <op> <value> : <bufsize> <postdelay> : "
                     "<v1> ... <vN> : <action>"
                  << std::endl;
        return false;
    }

    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = cmd_trace_remote(tokens);
    std::cout << result.str();
    return succeed;
}

bool
DebugConsole::cmd_trace_thread(std::string& UNUSED(cmd_str))
{
    bool succeed;
    if ( tokens.size() < 9 ) {
        std::cout << "Invalid format: trace <var> <op> <value> : <bufsize> <postdelay> : "
                     "<v1> ... <vN> : <action>"
                  << std::endl;
        return false;
    }
    // Clear R0 result string
    result.str("");
    result.clear();

    succeed = handleCommand();
    std::cout << result.str();
    return succeed;
}

bool
DebugConsole::cmd_trace_rank_serial(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 9 ) {
        std::cout << "Invalid format: trace <var> <op> <value> : <bufsize> <postdelay> : "
                     "<v1> ... <vN> : <action>"
                  << std::endl;
        return false;
    }
    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        succeed = cmd_trace_remote(tokens);
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }
    return succeed;
}

bool
DebugConsole::cmd_trace_rank_parallel(std::string& cmd_str)
{
    bool succeed;
    if ( tokens.size() < 9 ) {
        std::cout << "Invalid format: trace <var> <op> <value> : <bufsize> <postdelay> : "
                     "<v1> ... <vN> : <action>"
                  << std::endl;
        return false;
    }
    // Clear R0 result string
    result.str("");
    result.clear();

    if ( current_rank == 0 ) {
        // Execute in correct local thread
        succeed = handleCommand();
        std::cout << result.str();
    }
    else {
        // Send to remote rank
        succeed = sendCommand(current_rank, current_thread, cmd_str);
        std::cout << result.str();
    }

    return succeed;
}


bool
DebugConsole::cmd_trace_remote(std::vector<std::string>& tokens)
{
    size_t      index = 1;
    std::string name  = "";

    // Get first comparison
    Core::Serialization::ObjectMapComparison* c = parseComparison(tokens, index, obj_, name);
    if ( c == nullptr ) {
        std::cout << "Invalid argument passed in comparison trigger command" << std::endl;
        return false;
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
            return false;
        }
        else {
            pt->addLogicOp(logicOp);
        }
        if ( index == tokens.size() ) {
            std::cout << "Invalid format for trace command\n";
            return false;
        }

        // Get next comparison
        Core::Serialization::ObjectMapComparison* c = parseComparison(tokens, index, obj_, name);
        if ( c == nullptr ) {
            std::cout << "Invalid argument in comparison of trace command\n";
            return false;
        }
        pt->addComparison(c);

    } // while index < tokens.size(), add another logic op and test comparision

    try {
        auto* tb = parseTraceBuffer(tokens, index, obj_);
        if ( tb == nullptr ) {
            std::cout << "Invalid trace buffer argument in trace command\n";
            return false;
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
                return false;
            }
            pt->addObjectBuffer(objBuf);
        } // end while get trace vars

        // Parse action
        std::string action = tokens[index];

        WatchPoint::WPAction* actionObj = parseAction(tokens, index, obj_);
        if ( actionObj == nullptr ) {
            std::cout << "Error in action: " << action << std::endl;
            return false;
        }
        else {
            pt->setAction(actionObj);
        }

        // Check for extra arguments
        if ( index != tokens.size() ) {
            std::cout << "Error, too many arguments" << std::endl;
            return false;
        }

        // Get the top level component to set the watch point
        BaseComponent* comp = static_cast<BaseComponent*>(base_comp_->getAddr());
        if ( comp ) {
            comp->addWatchPoint(pt);
            watch_points_.emplace_back(pt, comp);
            result << "Added watchpoint #" << wpIndex << std::endl;
        }
        else {
            std::cout << "Not a component\n";
            return false;
        }
    }
    catch ( const std::exception& e ) {
        std::cout << "Invalid format for trace command" << std::endl;
        return false;
    }
    return true;
};

// exit OR quit : clears watchlists if prompted (or confirm off) then continues execution
bool
DebugConsole::cmd_exit_serial(std::string& UNUSED(cmd_str))
{
    // Remove all watchpoints before exit?
    if ( !confirm_ || query_clear_watchlist() ) {
        clear_watchlist(tokens);
        std::cout << "Removing all watchpoints and exiting ObjectExplorer\n";
    }
    else {
        std::cout << "Exiting ObjectExplorer without clearing watchpoints\n";
    }
    exit_console = true;
    return true;
}

bool
DebugConsole::cmd_exit_thread(std::string& UNUSED(cmd_str))
{
    // Remove all watchpoints?
    if ( !confirm_ || query_clear_watchlist() ) {
        // Clear watchlists for rank 0 threads
        handleCommandAll();
        std::cout << "Removing all watchpoints and exiting ObjectExplorer\n";
    }
    else {
        std::cout << "Exiting ObjectExplorer without clearning watchpoints\n";
    }
    exit_console = true;
    return true;
}

bool
DebugConsole::cmd_exit_rank_serial(std::string& cmd_str)
{
    // Remove all watchpoints?
    if ( !confirm_ || query_clear_watchlist() ) {
        // Clear watchlist for rank 0
        clear_watchlist(tokens);

        // Clear watchlists for other ranks
        sendCommandAll(cmd_str);

        std::cout << "Removing all watchpoints and exiting ObjectExplorer\n";
    }
    else {
        std::cout << "Exiting ObjectExplorer without clearning watchpoints\n";
    }
    exit_console = true;
    return true;
}

bool
DebugConsole::cmd_exit_rank_parallel(std::string& cmd_str)
{
    // Remove all watchpoints?
    if ( !confirm_ || query_clear_watchlist() ) {
        // Clear watchlists for rank 0 threads
        handleCommandAll();

        // Clear watchlists for other ranks
        sendCommandAll(cmd_str);

        std::cout << "Removing all watchpoints and exiting ObjectExplorer\n";
    }
    else {
        std::cout << "Exiting ObjectExplorer without clearning watchpoints\n";
    }
    exit_console = true;
    return true;
}

// shutdown : exit the debugger and cleanly shutdown simulator
bool
DebugConsole::cmd_shutdown(std::string& UNUSED(cmd_str))
{
    simulationShutdown();
    exit_console = true;
    std::cout << "R" << rank_.rank << ", T" << rank_.thread << ": Exiting ObjectExplorer and shutting down simulation"
              << std::endl;
    return true;
}

// ---- Command History Buffer Functions ----
void
CommandHistoryBuffer::append(std::string s)
{
    if ( !en_ ) return;
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
    catch ( const std::invalid_argument& e ) {
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
    catch ( const std::invalid_argument& e ) {
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

// print message based on verbosity
void
DebugConsole::msg(VERBOSITY_MASK mask, std::string message)
{
    if ( (!static_cast<uint32_t>(mask)) & verbosity ) return;
    std::cout << message << std::endl;
}

std::pair<ConsoleCommand, bool> const
CommandRegistry::seek(std::string token, SEARCH_TYPE search_type)
{
    last_seek_command.second = false;
    if ( search_type == SEARCH_TYPE::ALL || search_type == SEARCH_TYPE::BUILTIN ) {
        for ( auto consoleCommand : registry ) {
            if ( consoleCommand.match(token) ) {
                last_seek_command.first  = consoleCommand;
                last_seek_command.second = true;
                return last_seek_command;
            }
        }
    }
    if ( search_type == SEARCH_TYPE::ALL || search_type == SEARCH_TYPE::USER ) {
        for ( auto consoleCommand : user_registry ) {
            if ( consoleCommand.match(token) ) {
                last_seek_command.first  = consoleCommand;
                last_seek_command.second = true;
                return last_seek_command;
            }
        }
    }

    return last_seek_command;
}

bool
CommandRegistry::beginUserCommand(std::string name)
{
    // Make sure not a built-in command
    auto res = seek(name, CommandRegistry::SEARCH_TYPE::BUILTIN);
    if ( res.second ) {
        std::cout << "Cannot overwrite built-in command \"" << name << "\"" << std::endl;
        return false;
    }
    user_command_wip                        = name;
    // Create or overwrite existing user defined command
    user_defined_commands[user_command_wip] = {};
    std::cout << "Enter commands for \"" << user_command_wip << "\" terminated by \"end\"" << std::endl;
    return true;
}

void
CommandRegistry::appendUserCommand(std::string token0, std::string line)
{
    // No recursion
    if ( token0 == user_command_wip ) {
        std::cout << token0 << " cannot call itself" << std::endl;
        return;
    }

    // Commands not allowed: define, document, replay
    std::pair<ConsoleCommand, bool> res = seek(token0, CommandRegistry::SEARCH_TYPE::BUILTIN);
    if ( res.second ) {
        // Disallow nested `define` or `document` since these change user_command_wip
        if ( res.first.str_long() == "define" || res.first.str_long() == "document" ) {
            std::cout << "Ignoring entry: " << res.first.str_long() << "/" << res.first.str_short() << std::endl;
            return;
        }
        // Replay support requires changes to dispatch_cmd
        if ( res.first.str_long() == "replay" ) {
            std::cout << "Ignoring entry: " << res.first.str_long() << "/" << res.first.str_short() << std::endl;
            return;
        }
    }
    user_defined_commands[user_command_wip].emplace_back(line);
}

void
CommandRegistry::commitUserCommand()
{
    std::cout << "Committing definition for " << user_command_wip << std::endl;
    user_registry.emplace_back(ConsoleCommand(user_command_wip));
    user_command_wip = "";
}

bool
CommandRegistry::beginDocCommand(std::string name)
{
    // Make sure not a built-in command
    auto res = seek(name, CommandRegistry::SEARCH_TYPE::BUILTIN);
    if ( res.second ) {
        std::cout << "Cannot overwrite built-in command \"" << name << "\"" << std::endl;
        return false;
    }
    // Make sure user command is defined
    res = seek(name, CommandRegistry::SEARCH_TYPE::USER);
    if ( !res.second ) {
        std::cout << "\"" << name << "\" must be defined before documenting" << std::endl;
        return false;
    }

    user_command_wip = name;
    user_doc_wip     = {};
    std::cout << "Enter documentation for \"" << user_command_wip << "\" terminated by \"end\"" << std::endl;
    return true;
}

void
CommandRegistry::appendDocCommand(std::string line)
{
    user_doc_wip.emplace_back(line);
}

void
CommandRegistry::commitDocCommand()
{
    bool found = false;
    for ( ConsoleCommand& consoleCommand : user_registry ) {
        if ( consoleCommand.match(user_command_wip) ) {
            std::cout << "Committing documentation for " << user_command_wip << std::endl;
            consoleCommand.setUserHelp(user_doc_wip[0]);
            addHelp(user_command_wip, user_doc_wip);
            found = true;
        }
    }

    if ( !found )
        std::cout << "Unable to commit documentation. Could not locate definition for " << user_command_wip
                  << std::endl;

    user_command_wip = "";
    user_doc_wip     = {};
}

void
CommandRegistry::addHelp(std::string key, std::vector<std::string>& vec)
{
    std::stringstream s;
    for ( const auto& line : vec )
        s << line << "\n";
    cmdHelp[key] = s.str();
    ;
}


// Handle Command sends command to all threads on current rank
// If current_thread is total number of threads, all threads execute the function
// Otherwise just current_thread executes the function
bool
DebugConsole::handleCommand()
{
    static Core::ThreadSafe::Barrier exchange_barrier(num_ranks_.thread);
    static Core::ThreadSafe::Barrier process_barrier(num_ranks_.thread);

    // Wait for shared variables to be stored by T0 (unpack and tokenize)
    exchange_barrier.wait();
    bool succeed = true;

    if ( tokens[0] == "done" ) {
        done = true;
    }
    else if ( tokens[0] == "summary" ) {
        if ( current_thread == rank_.thread ) {
            summary();
        }
    }
    // If not DONE, process command
    else if ( !done ) {
        // If I am target thread, handle the incoming command
        if ( current_thread == rank_.thread ) {
            if ( obj_ == nullptr ) {
                // Create a new ObjectMap
                obj_ = getComponentObjectMap();
                // Descend into the name_stack
                cd_name_stack();
            }
            auto consoleCommand = cmdRegistry.seek(tokens[0], CommandRegistry::SEARCH_TYPE::BUILTIN);
            succeed             = consoleCommand.first.exec_remote(tokens);
        }
    }

    // Wait for result to be stored by target thread
    process_barrier.wait();
    return succeed;
}

// Handle command for all threads in current rank
bool
DebugConsole::handleCommandAll()
{
    bool    succeed = true;
    bool    ret;
    int32_t orig_thread = current_thread;
    for ( current_thread = 0; current_thread < num_ranks_.thread; current_thread++ ) {
        ret     = handleCommand();
        succeed = succeed || ret;
    }
    current_thread = orig_thread;
    return succeed;
}

bool
DebugConsole::sendCommand(uint32_t rank_id, uint32_t thread_id, const std::string& cmd)
{
#ifdef SST_CONFIG_HAVE_MPI
    char*      cmd_buffer;
    uint32_t   str_length;
    int        buf_size;
    int        position = 0;
    int        tag      = 0;
    int        rcv_buf_size;
    MPI_Status status;
    bool       succeed = true;
    char*      result_buffer;

    // Pack and Send message
    str_length = cmd.length() + 1;
    buf_size   = 3 * sizeof(uint32_t) + str_length;
    cmd_buffer = (char*)malloc(buf_size);

    // Pack rank_id, thread_id, cmd str length, and cmd string
    MPI_Pack(&rank_id, 1, MPI_UINT32_T, cmd_buffer, buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&thread_id, 1, MPI_UINT32_T, cmd_buffer, buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&str_length, 1, MPI_UINT32_T, cmd_buffer, buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(cmd.c_str(), str_length, MPI_CHAR, cmd_buffer, buf_size, &position, MPI_COMM_WORLD);
    // Send command buffer to destination rank
    MPI_Send(cmd_buffer, position, MPI_PACKED, rank_id, tag, MPI_COMM_WORLD);

    // Receive result string
    // Probe for incoming message to get its length
    MPI_Probe(rank_id, tag, MPI_COMM_WORLD, &status);
    // Get the actual number of elements (characters)
    MPI_Get_count(&status, MPI_PACKED, &rcv_buf_size);
    result_buffer = (char*)malloc(rcv_buf_size * sizeof(char));
    MPI_Recv(result_buffer, rcv_buf_size, MPI_PACKED, rank_id, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Unpack the succeed return code, string length, cmd string
    position = 0;
    MPI_Unpack(result_buffer, rcv_buf_size, &position, &succeed, 1, MPI_CXX_BOOL, MPI_COMM_WORLD);
    MPI_Unpack(result_buffer, rcv_buf_size, &position, &str_length, 1, MPI_UINT32_T, MPI_COMM_WORLD);
    if ( str_length > 1 ) { // not empty string
        char* result_str = (char*)malloc(str_length * sizeof(char));
        MPI_Unpack(result_buffer, rcv_buf_size, &position, result_str, str_length, MPI_CHAR, MPI_COMM_WORLD);
        result << result_str;
        free(result_str);
    }

    free(cmd_buffer);
    free(result_buffer);

    return succeed;
#endif
}

// Send command to all ranks and threads
bool
DebugConsole::sendCommandAll(const std::string& cmd_str)
{

    bool succeed = true;
    bool ret;

    for ( uint32_t rank_id = 1; rank_id < num_ranks_.rank; rank_id++ ) {
        ret     = sendCommand(rank_id, num_ranks_.thread, cmd_str);
        succeed = succeed || ret;
    }

    return succeed;
}

void
DebugConsole::receiveCommandRankSerial()
{
#ifdef SST_CONFIG_HAVE_MPI
    // const int default_size = 100;
    bool       succeed = true;
    char*      cmd_buffer;
    int        buf_size;
    int        position = 0;
    int        src      = 0;
    int        dst      = 0;
    int        tag      = 0;
    MPI_Status status;
    uint32_t   str_length;
    char*      cmd_str;
    uint32_t   rank_id, thread_id;

    // Receive the incoming command
    // Probe for incoming message to get its length
    MPI_Probe(src, tag, MPI_COMM_WORLD, &status);
    // Get the actual number of elements (characters)
    MPI_Get_count(&status, MPI_PACKED, &buf_size);
    cmd_buffer = (char*)malloc(buf_size * sizeof(char));
    MPI_Recv(cmd_buffer, buf_size, MPI_PACKED, src, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Unpack the rank_id, thread_id, string length, cmd string
    position = 0;
    MPI_Unpack(cmd_buffer, buf_size, &position, &rank_id, 1, MPI_UINT32_T, MPI_COMM_WORLD);
    MPI_Unpack(cmd_buffer, buf_size, &position, &thread_id, 1, MPI_UINT32_T, MPI_COMM_WORLD);
    MPI_Unpack(cmd_buffer, buf_size, &position, &str_length, 1, MPI_UINT32_T, MPI_COMM_WORLD);
    cmd_str = (char*)malloc(str_length * sizeof(char));
    MPI_Unpack(cmd_buffer, buf_size, &position, cmd_str, str_length, MPI_CHAR, MPI_COMM_WORLD);
    current_rank   = rank_id;
    current_thread = thread_id;

    // Tokenize cmd string (shared so all threads can access)
    std::string cmd(cmd_str);
    tokenize(tokens, cmd);

    // Clear local result string
    result.str("");
    result.clear();

    // Set done for all threads
    if ( tokens[0] == "done" ) {
        done = true;
    }
    else if ( tokens[0] == "summary" ) {
        summary();
    }
#if 0 // Will we need an init with object tree?
    else if (tokens[0] == "init") {
        if (obj_ == nullptr) {
            // Create a new ObjectMap
            obj_ = getComponentObjectMap();
            // Descend into the name_stack
            cd_name_stack();
        }
    }
#endif
    // Execute command
    else {
        auto consoleCommand = cmdRegistry.seek(tokens[0], CommandRegistry::SEARCH_TYPE::BUILTIN);
        if ( consoleCommand.second ) {
            // Execute in target thread
            if ( obj_ == nullptr ) {
                // Create a new ObjectMap
                obj_ = getComponentObjectMap();
                // Descend into the name_stack
                cd_name_stack();
            }
            succeed = consoleCommand.first.exec_remote(tokens);
            // Output::getDefaultObject().output("ReceiveCmdRankSerial: succeed %d\n", succeed);
        }
        else {
            std::cout << "Error: Command not found in remote rank: " << tokens[0] << std::endl;
            assert(false);
        }
    }

    // Send succeed and result string back to Rank 0 Thread 0
    char*       result_buffer;
    std::string result_str = result.str();
    str_length             = result_str.length() + 1;
    buf_size               = 2 * sizeof(uint32_t) + str_length;
    result_buffer          = (char*)malloc(buf_size);
    position               = 0;
    MPI_Pack(&succeed, 1, MPI_CXX_BOOL, result_buffer, buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&str_length, 1, MPI_UINT32_T, result_buffer, buf_size, &position, MPI_COMM_WORLD);
    if ( str_length > 1 ) { // not empty string
        MPI_Pack(result_str.c_str(), str_length, MPI_CHAR, result_buffer, buf_size, &position, MPI_COMM_WORLD);
    }
    MPI_Send(result_buffer, position, MPI_PACKED, dst, tag, MPI_COMM_WORLD);

    free(cmd_buffer);
    free(cmd_str);
    free(result_buffer);
#endif
}

void
DebugConsole::receiveCommandRankParallel()
{
#ifdef SST_CONFIG_HAVE_MPI
    // const int default_size = 100;
    bool       succeed = true;
    char*      cmd_buffer;
    int        buf_size;
    int        position = 0;
    int        src      = 0;
    int        dst      = 0;
    int        tag      = 0;
    MPI_Status status;
    uint32_t   str_length;
    char*      cmd_str;
    uint32_t   rank_id, thread_id;

    // Receive the incoming command
    // Probe for incoming message to get its length
    MPI_Probe(src, tag, MPI_COMM_WORLD, &status);
    // Get the actual number of elements (characters)
    MPI_Get_count(&status, MPI_PACKED, &buf_size);
    cmd_buffer = (char*)malloc(buf_size * sizeof(char));
    MPI_Recv(cmd_buffer, buf_size, MPI_PACKED, src, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    // Unpack the rank_id, thread_id, string length, cmd string
    MPI_Unpack(cmd_buffer, buf_size, &position, &rank_id, 1, MPI_UINT32_T, MPI_COMM_WORLD);
    MPI_Unpack(cmd_buffer, buf_size, &position, &thread_id, 1, MPI_UINT32_T, MPI_COMM_WORLD);
    MPI_Unpack(cmd_buffer, buf_size, &position, &str_length, 1, MPI_UINT32_T, MPI_COMM_WORLD);
    cmd_str = (char*)malloc(str_length * sizeof(char));
    MPI_Unpack(cmd_buffer, buf_size, &position, cmd_str, str_length, MPI_CHAR, MPI_COMM_WORLD);
    current_rank   = rank_id;
    current_thread = thread_id;

    // Tokenize cmd string (shared so all threads can access)
    std::string cmd(cmd_str);
    tokenize(tokens, cmd);

    // Clear local result string
    result.str("");
    result.clear();

    // Set done for all threads
    if ( tokens[0] == "done" ) {
        handleCommand();
    }
    else if ( tokens[0] == "summary" ) {
        handleCommandAll();
    }
#if 0 // Will we need an init with object tree?
    else if (tokens[0] == "init") {
        if (obj_ == nullptr) {
            // Create a new ObjectMap
            obj_ = getComponentObjectMap();
            // Descend into the name_stack
            cd_name_stack();
        }
    }
#endif

    // Execute command for target thread
    else {
        auto consoleCommand = cmdRegistry.seek(tokens[0], CommandRegistry::SEARCH_TYPE::BUILTIN);
        if ( consoleCommand.second ) {
            // Execute function for all threads
            if ( current_thread == num_ranks_.thread ) {
                for ( current_thread = 0; current_thread < num_ranks_.thread; current_thread++ ) {
                    bool ret = handleCommand();
                    succeed  = succeed || ret;
                }
            }
            // Execute function for current_thread
            else {
                succeed = handleCommand();
            }
        }
        else {
            std::cout << "Error: Command not found in remote rank: " << tokens[0] << std::endl;
            assert(false);
        }
    }

    // Send succeed and result string back to Rank 0 Thread 0
    char*       result_buffer;
    std::string result_str = result.str();
    str_length             = result_str.length() + 1;
    buf_size               = 2 * sizeof(uint32_t) + str_length;
    result_buffer          = (char*)malloc(buf_size);
    position               = 0;
    MPI_Pack(&succeed, 1, MPI_CXX_BOOL, result_buffer, buf_size, &position, MPI_COMM_WORLD);
    MPI_Pack(&str_length, 1, MPI_UINT32_T, result_buffer, buf_size, &position, MPI_COMM_WORLD);
    if ( str_length > 1 ) { // not empty string
        MPI_Pack(result_str.c_str(), str_length, MPI_CHAR, result_buffer, buf_size, &position, MPI_COMM_WORLD);
    }
    MPI_Send(result_buffer, position, MPI_PACKED, dst, tag, MPI_COMM_WORLD);

    free(cmd_buffer);
    free(cmd_str);
    free(result_buffer);
#endif
}

bool
DebugConsole::sendDone()
{
#ifdef SST_CONFIG_HAVE_MPI
    std::string cmd        = "done";
    char*       cmd_buffer = nullptr;
    uint32_t    str_length;
    int         buf_size;
    int         position;
    int         tag = 0;
    int         rcv_buf_size;
    int         prev_size = 0;
    MPI_Status  status;
    bool        succeed       = false;
    char*       result_buffer = nullptr;

    // Pack and Send message
    str_length = cmd.length() + 1;
    buf_size   = 3 * sizeof(int32_t) + str_length;
    cmd_buffer = (char*)malloc(buf_size);

    int32_t thread_id = 0;
    for ( uint32_t rank_id = 1; rank_id < num_ranks_.rank; rank_id++ ) {
        position = 0;

        // Pack rank_id, thread_id, cmd str length, and cmd string
        MPI_Pack(&rank_id, 1, MPI_UINT32_T, cmd_buffer, buf_size, &position, MPI_COMM_WORLD);
        MPI_Pack(&thread_id, 1, MPI_UINT32_T, cmd_buffer, buf_size, &position, MPI_COMM_WORLD);

        MPI_Pack(&str_length, 1, MPI_UINT32_T, cmd_buffer, buf_size, &position, MPI_COMM_WORLD);

        MPI_Pack(cmd.c_str(), str_length, MPI_CHAR, cmd_buffer, buf_size, &position, MPI_COMM_WORLD);

        // Send command buffer to destination rank
        MPI_Send(cmd_buffer, position, MPI_PACKED, rank_id, tag, MPI_COMM_WORLD);

        // Receive result string
        // Probe for incoming message to get its length
        MPI_Probe(rank_id, tag, MPI_COMM_WORLD, &status);
        // Get the actual number of elements (characters)
        MPI_Get_count(&status, MPI_CHAR, &rcv_buf_size);
        // Consider using this in sendCommand - all done messages should be the same size
        if ( rcv_buf_size > prev_size ) {
            if ( result_buffer != nullptr ) {
                free(result_buffer);
            }
            result_buffer = (char*)malloc(rcv_buf_size * sizeof(char));
            prev_size     = rcv_buf_size;
        }
        MPI_Recv(result_buffer, rcv_buf_size, MPI_CHAR, rank_id, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Unpack the rank_id, thread_id, string length, cmd string
        position = 0;
        MPI_Unpack(result_buffer, buf_size, &position, &succeed, 1, MPI_CXX_BOOL, MPI_COMM_WORLD);
    } // End for each rank

    free(cmd_buffer);
    return succeed;
#endif
}

// Could just call the correct execute directly now
int
DebugConsole::execute(const std::string& msg)
{
    int ret;
    // Serial (single rank, single thread)
    if ( num_ranks_.rank == 1 && num_ranks_.thread == 1 ) {
        // ExecutionType::SERIAL;
        // Currently we don't print summary because it would break existing tests
        // but we may want the components to print for consistency
        // std::cout << "Serial execute" << std::endl;
        // std::cout << "\nINTERACTIVE CONSOLE" << std::endl;
        // summary();
        ret = consoleExecute(msg);
    }
    // Thread (single rank, multiple threads)
    else if ( num_ranks_.rank == 1 ) {
        // ExecutionType::THREAD;
        // std::cout << "Thread execute" << std::endl;
        ret = executeThread(msg);
    }
    // Rank Serial (multiple ranks, single thread per rank)
    else if ( num_ranks_.thread == 1 ) {
        // ExecutionType::RANK_SERIAL;
        // std::cout << "RankSerial execute" << std::endl;
        ret = executeRankSerial(msg);
    }
    // Rank Parallel (multiple ranks, multiple threads per rank)
    else {
        // ExecutionType::RANK_PARALLEL;
        // std::cout << "RankParallel execute" << std::endl;
        ret = executeRankParallel(msg);
    }
    return ret; // Return code currently unused by sync manager
}

int
DebugConsole::executeThread(const std::string& msg)
{
    if ( rank_.thread == 0 ) {
        // Clear R0 result string
        result.str("");
        result.clear();

        // Print Summary
        std::cout << "\nINTERACTIVE CONSOLE" << std::endl;
        const std::string& str = "summary";
        tokenize(tokens, str);
        handleCommandAll();
        std::cout << result.str();

        // Enter console to manage commands
        consoleExecute(msg);

        // Set done in all threads
        tokens[0] = "done";
        handleCommand();
    }
    else {
        // Init object map
        if ( obj_ == nullptr ) {
            // Create a new ObjectMap
            obj_ = getComponentObjectMap();
            // Descend into the name_stack
            cd_name_stack();
        }

        // Enter done loop
        while ( !done ) {
            // Wait for commands
            handleCommand();
        }
    }
    done = false;
    return 0; // Return codes currently unused
}

int
DebugConsole::executeRankSerial(const std::string& msg)
{
#ifdef SST_CONFIG_HAVE_MPI
    // -- Rank 0
    // Executes the console and sends commands to other threads/ranks as needed
    if ( rank_.rank == 0 ) {
        // Clear R0 result string
        result.str("");
        result.clear();

        // Print Summary
        std::cout << "\nINTERACTIVE CONSOLE" << std::endl;
        const std::string& str = "summary";
        tokenize(tokens, str);
        summary();
        std::cout << result.str();

        // Clear R0 result string
        result.str("");
        result.clear();
        sendCommandAll(str);
        std::cout << result.str();

        // sendInit();

        // Enter console to handle commands
        consoleExecute(msg);

        // Set done locally
        done = true;

        // Send done to remote ranks
        sendDone();

    } // end Rank 0
    // -- Rank i!=0
    // Handles incoming/outgoing mpi messages and executes command
    else { // Other ranks
        while ( !done ) {
            receiveCommandRankSerial();
        } // while !done
    } // end Rank i!=0

    done = false; // Return codes currently unused
    return 0;
#endif // SST_CONFIG_HAVE_MPI
}

int
DebugConsole::executeRankParallel(const std::string& msg)
{
#ifdef SST_CONFIG_HAVE_MPI

    // -- Rank 0, Thread 0
    // Executes the console and sends commands to other threads/ranks as needed
    if ( rank_.rank == 0 && rank_.thread == 0 ) {
        // Clear R0 result string
        result.str("");
        result.clear();
        // Print Summary
        std::cout << "\nINTERACTIVE CONSOLE" << std::endl;
        const std::string& str = "summary";
        tokenize(tokens, str);
        handleCommandAll();
        std::cout << result.str();

        // Clear R0 result string
        result.str("");
        result.clear();
        sendCommandAll(str);
        std::cout << result.str();

        // sendInit();

        // Enter console to handle commands
        consoleExecute(msg);

        // Set done in local rank threads
        tokens.clear();
        tokens[0] = "done";
        handleCommand();

        // Send done to remote ranks
        sendDone();

    } // end Rank 0, Thread 0
    // -- Rank i!=0, Thread 0
    // Handles incoming/outgoing mpi messages and invokes command with target thread(s)
    else if ( rank_.rank != 0 && rank_.thread == 0 ) { // Other ranks, thread 0

        while ( !done ) {
            receiveCommandRankParallel();
        } // while !done
    } // end Rank i!=0, Thread 0
    // -- Rank i!=0, Thread j!=0
    // All other threads wait for commands
    else {
        while ( !done ) {
            handleCommand();
        }
    }

    done = false;
    // Maybe check shutdown here as well?
    return 0; // Return codes currently unused
#endif
} // end rankParallelExecute


} // namespace SST::IMPL::Interactive
