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

#ifndef SST_CORE_IMPL_INTERACTIVE_DEBUGCONSOLE_H
#define SST_CORE_IMPL_INTERACTIVE_DEBUGCONSOLE_H

#include "sst/core/eli/elementinfo.h"
#include "sst/core/impl/interactive/cmdLineEditor.h"
#include "sst/core/impl/interactive/debugStream.h"
#include "sst/core/interactiveConsole.h"
#include "sst/core/serialization/objectMapDeferred.h"
#include "sst/core/threadsafe.h"
#include "sst/core/watchPoint.h"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <ostream>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "sst/core/sst_mpi.h"

namespace SST::IMPL::Interactive {

enum class ConsoleCommandGroup { GENERAL, NAVIGATION, STATE, WATCH, SIMULATION, LOGGING, MISC, USER};
enum class ExecutionType { SERIAL, THREAD, RANK_SERIAL, RANK_PARALLEL };

const std::map<ConsoleCommandGroup, std::string> GroupText {
    { ConsoleCommandGroup::GENERAL, "General" },
    { ConsoleCommandGroup::NAVIGATION, "Navigation" },
    { ConsoleCommandGroup::STATE, "State" },
    { ConsoleCommandGroup::WATCH, "Watch/Trace" },
    { ConsoleCommandGroup::SIMULATION, "Simulation" },
    { ConsoleCommandGroup::LOGGING, "Logging" },
    { ConsoleCommandGroup::MISC, "Misc" },
    { ConsoleCommandGroup::USER, "User Defined" },
};

// Each bit of mask enables verbosity for different debug features.
// This is primarily for developers.
enum class VERBOSITY_MASK : uint32_t {
    WATCHPOINTS = 0b0001'0000 // 0x10
};

enum class LINE_ENTRY_MODE : int {
    NORMAL,   // line is executed as a command
    DEFINE,   // line is captured in user defined command sequence
    DOCUMENT, // documenting a user defined command
};

// Encapsulate a console command.
class ConsoleCommand
{
public:
    // Constructor for built-in commands has callback - console only
    ConsoleCommand(std::string str_long, std::string str_short, std::string str_help, ConsoleCommandGroup group,
        std::function<bool(std::string& cmd_str)> func ) :
        str_long_(str_long),
        str_short_(str_short),
        str_help_(str_help),
        group_(group),
        func_(func)
    {}

    // Constructor for built-in commands has callback - remote calls
    ConsoleCommand(std::string str_long, std::string str_short, std::string str_help, ConsoleCommandGroup group,
        ExecutionType exec_type,
        std::function<bool(std::string& cmd_str)> func_serial, 
        std::function<bool(std::string& cmd_str)> func_thread,
        std::function<bool(std::string& cmd_str)> func_rank_serial,
        std::function<bool(std::string& cmd_str)> func_rank_parallel, 
        std::function<bool(std::vector<std::string>& UNUSED(tokens))> func_remote
        ) :
        str_long_(str_long),
        str_short_(str_short),
        str_help_(str_help),
        group_(group),
        exec_type_(exec_type),
        func_serial_(func_serial), 
        func_thread_(func_thread),
        func_rank_serial_(func_rank_serial),
        func_rank_parallel_(func_rank_parallel),
        func_remote_(func_remote)
    { 
        // Serial
        if (exec_type_ == ExecutionType::SERIAL) {
            func_ = func_serial_;
        }
        // Thread (single rank, multiple threads)
        else if (exec_type_ == ExecutionType::THREAD) {
            func_ = func_thread_;
        }
        // Rank Serial (multiple ranks, single thread per rank)
        else if (exec_type_ == ExecutionType::RANK_SERIAL) {
            func_ = func_rank_serial_;
        }
        // Rank Parallel (multiple ranks, multiple threads per rank)
        else  {
            func_ = func_rank_parallel_;
        }
    }

    // Constructor for user-defined commands
    ConsoleCommand(std::string str_long) :
        str_long_(str_long),
        str_short_(str_long),
        str_help_("user defined command"),
        group_(ConsoleCommandGroup::USER)
    {}

    ConsoleCommand() {}; // default constructor
    const std::string&         str_long() const { return str_long_; }
    const std::string&         str_short() const { return str_short_; }
    const std::string&         str_help() const { return str_help_; }
    void                       setUserHelp(std::string& help) { str_help_ = help; }
    const ConsoleCommandGroup& group() const { return group_; }
    // Command Execution
    bool                       exec(std::string& cmd_str) { return func_(cmd_str); }
    bool                       exec_serial(std::string& cmd_str)  { return func_serial_(cmd_str); }                   
    bool                       exec_thread(std::string& cmd_str) { return func_thread_(cmd_str); }
    bool                       exec_rank_serial(std::string& cmd_str) { return func_rank_serial_(cmd_str); }
    bool                       exec_rank_parallel(std::string& cmd_str) { return func_rank_parallel_(cmd_str); }
    bool                       exec_remote(std::vector<std::string>& UNUSED(tokens)) { return func_remote_(tokens); }
    bool                       match(const std::string& token)
    {
        std::string lctoken = toLower(token);
        if ( lctoken.size() == str_long_.size() && lctoken == toLower(str_long_) ) return true;
        if ( lctoken.size() == str_short_.size() && lctoken == toLower(str_short_) ) return true;
        return false;
    }
    friend std::ostream& operator<<(std::ostream& os, const ConsoleCommand c)
    {
        os << c.str_long_ << " (" << c.str_short_ << ") " << c.str_help_;
        return os;
    }

private:
    std::string                                           str_long_;
    std::string                                           str_short_;
    std::string                                           str_help_;
    ConsoleCommandGroup                                   group_;
    ExecutionType                                         exec_type_;

    std::function<bool(std::string& cmd_str)> func_;
    std::function<bool(std::string& cmd_str)> func_serial_;
    std::function<bool(std::string& cmd_str)> func_thread_;
    std::function<bool(std::string& cmd_str)> func_rank_serial_;
    std::function<bool(std::string& cmd_str)> func_rank_parallel_;
    std::function<bool(std::vector<std::string>& UNUSED(tokens))> func_remote_;

    std::string                                           toLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }

}; // class ConsoleCommand

class CommandHistoryBuffer
{
public:
    const int MAX_CMDS = 200;
    CommandHistoryBuffer() { buf_.resize(MAX_CMDS); }
    void                      append(std::string s);
    void                      print(int num);
    std::vector<std::string>& getBuffer();
    enum BANG_RC { INVALID, ECHO_ONLY, EXEC, NOP };
    BANG_RC bang(const std::string& token, std::string& newcmd);
    void    enable(bool en) { en_ = en; }

private:
    bool                                             en_    = true;
    int                                              cur_   = 0;
    int                                              nxt_   = 0;
    int                                              sz_    = 0;
    int                                              count_ = 0;
    std::vector<std::pair<std::size_t, std::string>> buf_; // actual history with index number
    std::vector<std::string> stringBuffer_;                // copy of history strings provided to command line editor
    // support for ! history retrieval
    bool                     findEvent(const std::string& s, std::string& newcmd);
    bool                     findOffset(const std::string& s, std::string& newcmd);
    bool                     searchFirst(const std::string& s, std::string& newcmd);
    bool                     searchAny(const std::string& s, std::string& newcmd);
};

class CommandRegistry
{

public:
    // Construction
    CommandRegistry() {}
    CommandRegistry(const std::vector<ConsoleCommand> in) :
        registry(in)
    {}
    // Access
    std::vector<ConsoleCommand>& getRegistryVector() { return registry; }
    std::vector<ConsoleCommand>& getUserRegistryVector() { return user_registry; }
    enum SEARCH_TYPE { ALL, BUILTIN, USER };
    std::pair<ConsoleCommand, bool> const seek(std::string token, SEARCH_TYPE search_type);
    // User defined command entry
    bool                                  beginUserCommand(std::string name);
    void                                  appendUserCommand(std::string token0, std::string line);
    void                                  commitUserCommand();
    std::vector<std::string>*             userCommandInsts(std::string key)
    {
        if ( user_defined_commands.find(key) == user_defined_commands.end() ) return nullptr;
        return &user_defined_commands[key];
    }
    // User defined command help doc entry
    bool beginDocCommand(std::string name);
    void appendDocCommand(std::string line);
    void commitDocCommand();
    bool commandIsEmpty(const std::string key)
    {
        if ( user_defined_commands.find(key) == user_defined_commands.end() ) return true;
        if ( user_defined_commands[key].size() == 0 ) return true;
        return false;
    };

    // User defined command help from vector
    void                               addHelp(std::string key, std::vector<std::string>& vec);
    // Detailed Command Help (public for now)
    std::map<std::string, std::string> cmdHelp;

private:
    // built-in commands
    std::vector<ConsoleCommand>                     registry              = {};
    // user defined commands
    std::vector<ConsoleCommand>                     user_registry         = {};
    std::map<std::string, std::vector<std::string>> user_defined_commands = {};
    std::string                                     user_command_wip      = "";
    std::vector<std::string>                        user_doc_wip          = {};

    // Last searched command with valid indicator
    std::pair<ConsoleCommand, bool> last_seek_command = {};
}; // class CommandRegistry

// Support for nesting user defined commands
class ExecState
{
public:
    // Constructor for entering a new user command
    ExecState(ConsoleCommand cmd, std::vector<std::string> tokens, std::vector<std::string>* insts) :
        cmd_(cmd),
        tokens_(tokens),
        insts_(insts),
        index_(0),
        user_(true)
    {
        assert(insts_->size() > 0);
    };
    ExecState() {};
    bool        ret() { return ret_; }
    // Advance state and return the next user instruction
    std::string next()
    {
        assert(user_);
        assert(!ret_);
        assert(insts_);
        assert(index_ < insts_->size());
        ret_ = (index_ + 1) == insts_->size();
        return insts_->at(index_++);
    }

private:
    ConsoleCommand            cmd_    = {};      // user command in progress
    std::vector<std::string>  tokens_ = {};      // command args
    std::vector<std::string>* insts_  = nullptr; // command sequence
    size_t                    index_  = 0;       // command pointer
    bool                      user_   = false;   // in user command
    bool                      ret_    = false;   // user command complete, return to caller
}; // class ExecState

class DebugConsole : public SST::InteractiveConsole
{

public:
    SST_ELI_REGISTER_INTERACTIVE_CONSOLE(
      DebugConsole,   // class
      "sst",     // library
      "interactive.debugger", // name
      SST_ELI_ELEMENT_VERSION(1, 0, 0),
      "{EXPERIMENTAL} Interactive console debug probe")

    SST_ELI_DOCUMENT_PARAMS(
      {"replayFile", "script for playback upon entering interactive debug console", ""}
    )

    /**
           Creates a new self partition scheme.
    */
    explicit DebugConsole(Params& params);
    ~DebugConsole();

    int  execute(const std::string& msg) override; // SKK separate to thread, rank execute 
    void summary() override;

    // Callbacks from command line completions
    void get_listing_strings(std::list<std::string>&);

private:
    // This is the stack of where we are in the class hierarchy.  This
    // is needed because when we advance time, we'll need to delete
    // any ObjectMap because they could change during execution.
    // After running, this will allow us to recreate the working
    // directory as far as we can.
    std::deque<std::string> name_stack;

    SST::Core::Serialization::ObjectMap* obj_     = nullptr;
    bool                                 done     = false;
    bool                                 exit_console = false;
    int                                  retState = -1; // -1 DONE, -2 SUMMARY, positive number is threadID

    void save_name_stack();
    void cd_name_stack();

    ExecutionType exec_type;
    RankInfo num_ranks_;
    RankInfo rank_;

    bool autoCompleteEnable = true;

    // logging support
    std::ofstream loggingFile;
    std::ifstream replayFile;
    std::string   loggingFilePath = "sst-console.out";
    std::string   replayFilePath = "sst-console.in";
    bool          enLogging = false;

    // command injection (for sst --replay option)
    std::stringstream injectedCommand;

    // execution state management for nested user commands
    ExecState             eState = {};
    std::stack<ExecState> eStack = {};

    // Keep a pointer to the ObjectMap for the top level Component
    SST::Core::Serialization::ObjectMapDeferred<BaseComponent>* base_comp_ = nullptr;

    // Keep track of all the WatchPoints
    std::vector<std::pair<WatchPoint*, BaseComponent*>> watch_points_;
    bool                                                query_clear_watchlist();
    bool                                                clear_watchlist(std::vector<std::string>& UNUSED(tokens));
    bool confirm_ = true; // Ask for confirmation to clear watchlist

    std::vector<std::string> tokenize(std::vector<std::string>& tokens, const std::string& input);

    // Navigation
    bool cmd_help(std::string& UNUSED(cmd_str));

    bool cmd_verbose_query();
    bool cmd_verbose_serial(std::string& UNUSED(cmd_str));
    bool cmd_verbose_thread(std::string& UNUSED(cmd_str));
    bool cmd_verbose_rank_serial(std::string& cmd_str);
    bool cmd_verbose_rank_parallel(std::string& cmd_str);
    bool cmd_verbose_remote(std::vector<std::string>&(tokens));
    
    bool cmd_info_serial(std::string& UNUSED(cmd_str));  
    bool cmd_info_thread(std::string& UNUSED(cmd_str));
    bool cmd_info_rank_serial(std::string& cmd_str);
    bool cmd_info_rank_parallel(std::string& cmd_str);
    bool cmd_info_remote(std::vector<std::string>& UNUSED(tokens));
   
    int  parse_thread();
    bool cmd_thread_serial(std::string& UNUSED(cmd_str));
    bool cmd_thread_thread(std::string& UNUSED(cmd_str));
    bool cmd_thread_rank_serial(std::string& cmd_str);
    bool cmd_thread_rank_parallel(std::string& cmd_str);
    bool cmd_thread_remote(std::vector<std::string>& UNUSED(tokens));

    int  parse_rank();
    bool cmd_rank_serial(std::string& UNUSED(cmd_str));
    bool cmd_rank_thread(std::string& UNUSED(cmd_str));
    bool cmd_rank_rank_serial(std::string& cmd_str);
    bool cmd_rank_rank_parallel(std::string& cmd_str);
    bool cmd_rank_remote(std::vector<std::string>& UNUSED(tokens));

    bool cmd_setConfirm(std::string& UNUSED(cmd_str));

    bool cmd_pwd_serial(std::string& UNUSED(cmd_str));
    bool cmd_pwd_thread(std::string& UNUSED(cmd_str));
    bool cmd_pwd_rank_serial(std::string& cmd_str);
    bool cmd_pwd_rank_parallel(std::string& cmd_str);
    bool cmd_pwd_remote(std::vector<std::string>& UNUSED(tokens));

    bool cmd_cd_serial(std::string& UNUSED(cmd_str));
    bool cmd_cd_thread(std::string& UNUSED(cmd_str));
    bool cmd_cd_rank_serial(std::string& cmd_str);
    bool cmd_cd_rank_parallel(std::string& cmd_str);
    bool cmd_cd_remote(std::vector<std::string>& tokens);

    bool cmd_ls_serial(std::string& UNUSED(cmd_str));
    bool cmd_ls_thread(std::string& UNUSED(cmd_str));
    bool cmd_ls_rank_serial(std::string& cmd_str);
    bool cmd_ls_rank_parallel(std::string& cmd_str);
    bool cmd_ls_remote(std::vector<std::string>& UNUSED(tokens));

    // State
    bool cmd_time(std::string& UNUSED(cmd_str));

    bool cmd_print_serial(std::string& UNUSED(cmd_str));
    bool cmd_print_thread(std::string& UNUSED(cmd_str));
    bool cmd_print_rank_serial(std::string& cmd_str);
    bool cmd_print_rank_parallel(std::string& cmd_str);
    bool cmd_print_remote(std::vector<std::string>& tokens);

    bool cmd_set_serial(std::string& UNUSED(cmd_str));
    bool cmd_set_thread(std::string& UNUSED(cmd_str));
    bool cmd_set_rank_serial(std::string& cmd_str);
    bool cmd_set_rank_parallel(std::string& cmd_str);
    bool cmd_set_remote(std::vector<std::string>& tokens);
    
    bool cmd_watch_serial(std::string& UNUSED(cmd_str));
    bool cmd_watch_thread(std::string& UNUSED(cmd_str));
    bool cmd_watch_rank_serial(std::string& cmd_str);
    bool cmd_watch_rank_parallel(std::string& cmd_str);
    bool cmd_watch_remote(std::vector<std::string>& tokens);

    bool cmd_trace_serial(std::string& UNUSED(cmd_str));
    bool cmd_trace_thread(std::string& UNUSED(cmd_str));
    bool cmd_trace_rank_serial(std::string& cmd_str);
    bool cmd_trace_rank_parallel(std::string& cmd_str);
    bool cmd_trace_remote(std::vector<std::string>& tokens);

    bool cmd_watchlist_serial(std::string& UNUSED(cmd_str));
    bool cmd_watchlist_thread(std::string& UNUSED(cmd_str));
    bool cmd_watchlist_rank_serial(std::string& cmd_str);
    bool cmd_watchlist_rank_parallel(std::string& cmd_str);
    bool cmd_watchlist_remote(std::vector<std::string>& tokens);
    
    bool cmd_addTraceVar_serial(std::string& UNUSED(cmd_str));
    bool cmd_addTraceVar_thread(std::string& UNUSED(cmd_str));
    bool cmd_addTraceVar_rank_serial(std::string& cmd_str);
    bool cmd_addTraceVar_rank_parallel(std::string& cmd_str);
    bool cmd_addTraceVar_remote(std::vector<std::string>& tokens);

    bool cmd_printWatchpoint_serial(std::string& UNUSED(cmd_str));
    bool cmd_printWatchpoint_thread(std::string& UNUSED(cmd_str));
    bool cmd_printWatchpoint_rank_serial(std::string& cmd_str);
    bool cmd_printWatchpoint_rank_parallel(std::string& cmd_str);
    bool cmd_printWatchpoint_remote(std::vector<std::string>& tokens);

    bool cmd_printTrace_serial(std::string& UNUSED(cmd_str));
    bool cmd_printTrace_thread(std::string& UNUSED(cmd_str));
    bool cmd_printTrace_rank_serial(std::string& cmd_str);
    bool cmd_printTrace_rank_parallel(std::string& cmd_str);
    bool cmd_printTrace_remote(std::vector<std::string>& tokens);

    bool cmd_resetTraceBuffer_serial(std::string& UNUSED(cmd_str));
    bool cmd_resetTraceBuffer_thread(std::string& UNUSED(cmd_str));
    bool cmd_resetTraceBuffer_rank_serial(std::string& cmd_str);
    bool cmd_resetTraceBuffer_rank_parallel(std::string& cmd_str);
    bool cmd_resetTraceBuffer_remote(std::vector<std::string>& tokens);

    bool cmd_setHandler_serial(std::string& UNUSED(cmd_str));
    bool cmd_setHandler_thread(std::string& UNUSED(cmd_str));
    bool cmd_setHandler_rank_serial(std::string& cmd_str);
    bool cmd_setHandler_rank_parallel(std::string& cmd_str);
    bool cmd_setHandler_remote(std::vector<std::string>& tokens);
    
    bool cmd_unwatch_serial(std::string& UNUSED(cmd_str));
    bool cmd_unwatch_thread(std::string& UNUSED(cmd_str));
    bool cmd_unwatch_rank_serial(std::string& cmd_str);
    bool cmd_unwatch_rank_parallel(std::string& cmd_str);
    bool cmd_unwatch_remote(std::vector<std::string>& tokens);
    
    // Simulation
    bool cmd_run(std::string& UNUSED(cmd_str));

    bool cmd_exit_serial(std::string& UNUSED(cmd_str));
    bool cmd_exit_thread(std::string& UNUSED(cmd_str));
    bool cmd_exit_rank_serial(std::string& cmd_str);
    bool cmd_exit_rank_parallel(std::string& cmd_str);

    bool cmd_shutdown(std::string& UNUSED(cmd_str));
    
    // Logging/Replay
    bool cmd_logging(std::string& UNUSED(cmd_str));
    bool cmd_replay(std::string& UNUSED(cmd_str));
    bool cmd_history(std::string& UNUSED(cmd_str));

    // Auto-completion toggle
    bool cmd_autoComplete(std::string& UNUSED(cmd_str));

    // Reset terminal
    bool cmd_clear(std::string& UNUSED(cmd_str));

    // User defined commands
    bool cmd_define(std::string& UNUSED(cmd_str));
    bool cmd_document(std::string& UNUSED(cmd_str));

    // command entry point
    bool dispatch_cmd(std::string& UNUSED(cmd));

    // Command Registry
    CommandRegistry cmdRegistry;

    // Command History
    CommandHistoryBuffer cmdHistoryBuf;

    // Command Line Editor
    CmdLineEditor   cmdLineEditor;
    LINE_ENTRY_MODE line_entry_mode = LINE_ENTRY_MODE::NORMAL;

    // Verbosity controlled console printing
    uint32_t verbosity = 0;
    void     msg(VERBOSITY_MASK mask, std::string message);

    // Pagination support
    DebuggerStream dout;

    // Support for serial, threaded, rank serial, rank parallel execution
    static uint32_t current_thread;
    static uint32_t current_rank;
    static std::vector<std::string> tokens;
    static std::stringstream result;
    
    int consoleExecute(const std::string& msg);
    int executeThread(const std::string& msg);
    int executeRankSerial(const std::string& msg);
    int executeRankParallel(const std::string& msg);
    bool handleCommand();
    bool handleCommandAll();
    bool sendCommand( uint32_t rank_id, uint32_t thread_id,  const std::string& cmd);
    bool sendCommandAll(const std::string&  cmd);
    void receiveCommandRankSerial();
    void receiveCommandRankParallel();
    bool sendDone();
};

} // namespace SST::IMPL::Interactive

#endif
