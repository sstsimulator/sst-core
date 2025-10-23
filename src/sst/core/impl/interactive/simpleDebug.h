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

#ifndef SST_CORE_IMPL_INTERACTIVE_SIMPLEDEBUG_H
#define SST_CORE_IMPL_INTERACTIVE_SIMPLEDEBUG_H

// clang-format off
#include "sst/core/eli/elementinfo.h"
#include <cstdint>
#include <ostream>
#include <sstream>
#include <sst/core/interactiveConsole.h>
#include "sst/core/serialization/objectMapDeferred.h"
#include "sst/core/impl/interactive/cmdLineEditor.h"
#include <sst/core/watchPoint.h>

#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>
// clang-format on

namespace SST::IMPL::Interactive {

enum class ConsoleCommandGroup { GENERAL, NAVIGATION, STATE, WATCH, SIMULATION, LOGGING, MISC };

const std::map<ConsoleCommandGroup, std::string> GroupText {
    { ConsoleCommandGroup::GENERAL, "General" },
    { ConsoleCommandGroup::NAVIGATION, "Navigation" },
    { ConsoleCommandGroup::STATE, "State" },
    { ConsoleCommandGroup::WATCH, "Watch/Trace" },
    { ConsoleCommandGroup::SIMULATION, "Simulation" },
    { ConsoleCommandGroup::LOGGING, "Logging" },
    { ConsoleCommandGroup::MISC, "Misc" },
};

enum class VERBOSITY_MASK : uint32_t {
    WATCHPOINTS = 0b0001'0000 // 0x10
};

// Encapsulate a console command.
class ConsoleCommand
{
public:
    ConsoleCommand(std::string str_long, std::string str_short, std::string str_help, ConsoleCommandGroup group,
        std::function<void(std::vector<std::string>& tokens)> func) :
        str_long_(str_long),
        str_short_(str_short),
        str_help_(str_help),
        group_(group),
        func_(func)
    {}
    const std::string&         str_long() const { return str_long_; }
    const std::string&         str_short() const { return str_short_; }
    const std::string&         str_help() const { return str_help_; }
    const ConsoleCommandGroup& group() const { return group_; }
    void                       exec(std::vector<std::string>& tokens) { return func_(tokens); }
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
    std::function<void(std::vector<std::string>& tokens)> func_;
    std::string                                           toLower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }
};

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

private:
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

class SimpleDebugger : public SST::InteractiveConsole
{

public:
    SST_ELI_REGISTER_INTERACTIVE_CONSOLE(
      SimpleDebugger,   // class
      "sst",     // library
      "interactive.simpledebug", // name
      SST_ELI_ELEMENT_VERSION(1, 0, 0),
      "{EXPERIMENTAL} Interactive console debug probe")

    SST_ELI_DOCUMENT_PARAMS(
      {"replayFile", "script for playback upon entering interactive debug console", ""}
    )

    /**
           Creates a new self partition scheme.
    */
    explicit SimpleDebugger(Params& params);
    ~SimpleDebugger();

    void execute(const std::string& msg) override;

    // Callbacks from command line completions
    void get_listing_strings(std::list<std::string>&);

private:
    // This is the stack of where we are in the class hierarchy.  This
    // is needed because when we advance time, we'll need to delete
    // any ObjectMap because they could change during execution.
    // After running, this will allow us to recreate the working
    // directory as far as we can.
    std::vector<std::string> name_stack;

    SST::Core::Serialization::ObjectMap* obj_ = nullptr;
    bool                                 done = false;

    bool autoCompleteEnable = true;

    // gdb/lldb thread spin support
    uint64_t spinner = 1;

    // logging support
    std::ofstream loggingFile;
    std::ifstream replayFile;
    std::string   loggingFilePath = "sst-console.out";
    std::string   replayFilePath  = "sst-console.in";
    bool          enLogging       = false;

    // command injection
    std::stringstream injectedCommand; // TODO use ConsoleCommand object

    // Keep a pointer to the ObjectMap for the top level Component
    SST::Core::Serialization::ObjectMapDeferred<BaseComponent>* base_comp_ = nullptr;

    // Keep track of all the WatchPoints
    std::vector<std::pair<WatchPoint*, BaseComponent*>> watch_points_;
    bool                                                clear_watchlist();
    bool                                                confirm = true; // Ask for confirmation to clear watchlist

    std::vector<std::string> tokenize(std::vector<std::string>& tokens, const std::string& input);

    // Navigation
    void cmd_help(std::vector<std::string>& UNUSED(tokens));
    void cmd_verbose(std::vector<std::string>&(tokens));
    void cmd_pwd(std::vector<std::string>& UNUSED(tokens));
    void cmd_ls(std::vector<std::string>& UNUSED(tokens));
    void cmd_cd(std::vector<std::string>& tokens);

    // Variable Access
    void cmd_print(std::vector<std::string>& tokens);
    void cmd_set(std::vector<std::string>& tokens);
    void cmd_time(std::vector<std::string>& tokens);
    void cmd_watch(std::vector<std::string>& tokens);
    void cmd_unwatch(std::vector<std::string>& tokens);

    // Simulation Control
    void cmd_run(std::vector<std::string>& tokens);
    void cmd_shutdown(std::vector<std::string>& tokens);
    void cmd_exit(std::vector<std::string>& UNUSED(tokens));

    // Watch/Trace
    void cmd_watchlist(std::vector<std::string>& tokens);
    void cmd_trace(std::vector<std::string>& tokens);
    void cmd_setHandler(std::vector<std::string>& tokens);
    void cmd_addTraceVar(std::vector<std::string>& tokens);
    void cmd_resetTraceBuffer(std::vector<std::string>& tokens);
    void cmd_printTrace(std::vector<std::string>& tokens);
    void cmd_printWatchpoint(std::vector<std::string>& tokens);
    void cmd_setConfirm(std::vector<std::string>& tokens);

    // Logging/Replay
    void cmd_logging(std::vector<std::string>& tokens);
    void cmd_replay(std::vector<std::string>& tokens);
    void cmd_history(std::vector<std::string>& tokens);

    // Auto-completion toggle
    void cmd_autoComplete(std::vector<std::string>& UNUSED(tokens));

    // Reset terminal
    void cmd_clear(std::vector<std::string>& UNUSED(tokens));

    // LLDB/GDB helper
    void cmd_spinThread(std::vector<std::string>& tokens);

    void dispatch_cmd(std::string& cmd);

    // Command Registry
    std::vector<ConsoleCommand> cmdRegistry;

    // Detailed Command Help
    std::map<std::string, std::string> cmdHelp;

    // Command History
    CommandHistoryBuffer cmdHistoryBuf;

    // Command Line Editor
    CmdLineEditor cmdLineEditor;

    // Verbosity controlled console printing
    uint32_t verbosity = 0;
    void     msg(VERBOSITY_MASK mask, std::string message);
};

} // namespace SST::IMPL::Interactive

#endif
