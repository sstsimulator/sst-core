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

#ifndef SST_CORE_IMPL_INTERACTIVE_DEBUGCOMMANDS_H
#define SST_CORE_IMPL_INTERACTIVE_DEBUGCOMMANDS_H

#include <cassert>
#include <cstddef>
#include <functional>
#include <map>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace SST::IMPL::Interactive {

enum class ConsoleCommandGroup { GENERAL, NAVIGATION, STATE, WATCH, SIMULATION, LOGGING, MISC, USER };
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
        std::function<bool(std::string& cmd_str)> func) :
        str_long_(str_long),
        str_short_(str_short),
        str_help_(str_help),
        group_(group),
        func_(func)
    {}

    // Constructor for built-in commands has callback - remote calls
    ConsoleCommand(std::string str_long, std::string str_short, std::string str_help, ConsoleCommandGroup group,
        ExecutionType exec_type, std::function<bool(std::string& cmd_str)> func_serial,
        std::function<bool(std::string& cmd_str)>                     func_thread,
        std::function<bool(std::string& cmd_str)>                     func_rank_serial,
        std::function<bool(std::string& cmd_str)>                     func_rank_parallel,
        std::function<bool(std::vector<std::string>& UNUSED(tokens))> func_remote) :
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
        if ( exec_type_ == ExecutionType::SERIAL ) {
            func_ = func_serial_;
        }
        // Thread (single rank, multiple threads)
        else if ( exec_type_ == ExecutionType::THREAD ) {
            func_ = func_thread_;
        }
        // Rank Serial (multiple ranks, single thread per rank)
        else if ( exec_type_ == ExecutionType::RANK_SERIAL ) {
            func_ = func_rank_serial_;
        }
        // Rank Parallel (multiple ranks, multiple threads per rank)
        else {
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
    bool                       exec_serial(std::string& cmd_str) { return func_serial_(cmd_str); }
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
    std::string         str_long_;
    std::string         str_short_;
    std::string         str_help_;
    ConsoleCommandGroup group_;
    ExecutionType       exec_type_;

    std::function<bool(std::string& cmd_str)>                     func_;
    std::function<bool(std::string& cmd_str)>                     func_serial_;
    std::function<bool(std::string& cmd_str)>                     func_thread_;
    std::function<bool(std::string& cmd_str)>                     func_rank_serial_;
    std::function<bool(std::string& cmd_str)>                     func_rank_parallel_;
    std::function<bool(std::vector<std::string>& UNUSED(tokens))> func_remote_;

    std::string toLower(std::string s)
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

} // namespace SST::IMPL::Interactive

#endif
