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
#include "sst/core/impl/interactive/debugCommands.h"
#include "sst/core/impl/interactive/debugStream.h"
#include "sst/core/interactiveConsole.h"
#include "sst/core/serialization/objectMapDeferred.h"
#include "sst/core/sst_mpi.h"
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

namespace SST::IMPL::Interactive {


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

    int  execute(const std::string& msg) override;
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

    SST::Core::Serialization::ObjectMap* obj_         = nullptr;
    bool                                 done         = false;
    bool                                 exit_console = false;
    int                                  retState     = -1; // -1 DONE, -2 SUMMARY, positive number is threadID

    void save_name_stack();
    void cd_name_stack();

    ExecutionType exec_type;
    RankInfo      num_ranks_;
    RankInfo      rank_;

    bool autoCompleteEnable = true;

    // logging support
    std::ofstream loggingFile;
    std::ifstream replayFile;
    std::string   loggingFilePath = "sst-console.out";
    std::string   replayFilePath  = "sst-console.in";
    bool          enLogging       = false;

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
    bool                                                clear_watchlist(std::vector<std::string>& tokens);
    bool                                                confirm_ = true; // Ask for confirmation to clear watchlist

    std::vector<std::string> tokenize(std::vector<std::string>& tokens, const std::string& input);

    // Navigation
    bool cmd_help(std::string& cmd_str);

    bool cmd_verbose_query();
    bool cmd_verbose_serial(std::string& cmd_str);
    bool cmd_verbose_thread(std::string& cmd_str);
    bool cmd_verbose_rank_serial(std::string& cmd_str);
    bool cmd_verbose_rank_parallel(std::string& cmd_str);
    bool cmd_verbose_remote(std::vector<std::string>&(tokens));

    bool cmd_info_serial(std::string& cmd_str);
    bool cmd_info_thread(std::string& cmd_str);
    bool cmd_info_rank_serial(std::string& cmd_str);
    bool cmd_info_rank_parallel(std::string& cmd_str);
    bool cmd_info_remote(std::vector<std::string>& tokens);

    int  parse_thread();
    bool cmd_thread_serial(std::string& cmd_str);
    bool cmd_thread_thread(std::string& cmd_str);
    bool cmd_thread_rank_serial(std::string& cmd_str);
    bool cmd_thread_rank_parallel(std::string& cmd_str);
    bool cmd_thread_remote(std::vector<std::string>& tokens);

    int  parse_rank();
    bool cmd_rank_serial(std::string& cmd_str);
    bool cmd_rank_thread(std::string& cmd_str);
    bool cmd_rank_rank_serial(std::string& cmd_str);
    bool cmd_rank_rank_parallel(std::string& cmd_str);
    bool cmd_rank_remote(std::vector<std::string>& tokens);

    bool cmd_setConfirm(std::string& cmd_str);

    bool cmd_pwd_serial(std::string& cmd_str);
    bool cmd_pwd_thread(std::string& cmd_str);
    bool cmd_pwd_rank_serial(std::string& cmd_str);
    bool cmd_pwd_rank_parallel(std::string& cmd_str);
    bool cmd_pwd_remote(std::vector<std::string>& tokens);

    bool cmd_cd_serial(std::string& cmd_str);
    bool cmd_cd_thread(std::string& cmd_str);
    bool cmd_cd_rank_serial(std::string& cmd_str);
    bool cmd_cd_rank_parallel(std::string& cmd_str);
    bool cmd_cd_remote(std::vector<std::string>& tokens);

    bool cmd_ls_serial(std::string& cmd_str);
    bool cmd_ls_thread(std::string& cmd_str);
    bool cmd_ls_rank_serial(std::string& cmd_str);
    bool cmd_ls_rank_parallel(std::string& cmd_str);
    bool cmd_ls_remote(std::vector<std::string>& tokens);

    // State
    bool cmd_time(std::string& cmd_str);

    bool cmd_print_serial(std::string& cmd_str);
    bool cmd_print_thread(std::string& cmd_str);
    bool cmd_print_rank_serial(std::string& cmd_str);
    bool cmd_print_rank_parallel(std::string& cmd_str);
    bool cmd_print_remote(std::vector<std::string>& tokens);

    bool cmd_set_serial(std::string& cmd_str);
    bool cmd_set_thread(std::string& cmd_str);
    bool cmd_set_rank_serial(std::string& cmd_str);
    bool cmd_set_rank_parallel(std::string& cmd_str);
    bool cmd_set_remote(std::vector<std::string>& tokens);

    bool cmd_watch_serial(std::string& cmd_str);
    bool cmd_watch_thread(std::string& cmd_str);
    bool cmd_watch_rank_serial(std::string& cmd_str);
    bool cmd_watch_rank_parallel(std::string& cmd_str);
    bool cmd_watch_remote(std::vector<std::string>& tokens);

    bool cmd_trace_serial(std::string& cmd_str);
    bool cmd_trace_thread(std::string& cmd_str);
    bool cmd_trace_rank_serial(std::string& cmd_str);
    bool cmd_trace_rank_parallel(std::string& cmd_str);
    bool cmd_trace_remote(std::vector<std::string>& tokens);

    bool cmd_watchlist_serial(std::string& cmd_str);
    bool cmd_watchlist_thread(std::string& cmd_str);
    bool cmd_watchlist_rank_serial(std::string& cmd_str);
    bool cmd_watchlist_rank_parallel(std::string& cmd_str);
    bool cmd_watchlist_remote(std::vector<std::string>& tokens);

    bool cmd_addTraceVar_serial(std::string& cmd_str);
    bool cmd_addTraceVar_thread(std::string& cmd_str);
    bool cmd_addTraceVar_rank_serial(std::string& cmd_str);
    bool cmd_addTraceVar_rank_parallel(std::string& cmd_str);
    bool cmd_addTraceVar_remote(std::vector<std::string>& tokens);

    bool cmd_printWatchpoint_serial(std::string& cmd_str);
    bool cmd_printWatchpoint_thread(std::string& cmd_str);
    bool cmd_printWatchpoint_rank_serial(std::string& cmd_str);
    bool cmd_printWatchpoint_rank_parallel(std::string& cmd_str);
    bool cmd_printWatchpoint_remote(std::vector<std::string>& tokens);

    bool cmd_printTrace_serial(std::string& cmd_str);
    bool cmd_printTrace_thread(std::string& cmd_str);
    bool cmd_printTrace_rank_serial(std::string& cmd_str);
    bool cmd_printTrace_rank_parallel(std::string& cmd_str);
    bool cmd_printTrace_remote(std::vector<std::string>& tokens);

    bool cmd_resetTraceBuffer_serial(std::string& cmd_str);
    bool cmd_resetTraceBuffer_thread(std::string& cmd_str);
    bool cmd_resetTraceBuffer_rank_serial(std::string& cmd_str);
    bool cmd_resetTraceBuffer_rank_parallel(std::string& cmd_str);
    bool cmd_resetTraceBuffer_remote(std::vector<std::string>& tokens);

    bool cmd_setHandler_serial(std::string& cmd_str);
    bool cmd_setHandler_thread(std::string& cmd_str);
    bool cmd_setHandler_rank_serial(std::string& cmd_str);
    bool cmd_setHandler_rank_parallel(std::string& cmd_str);
    bool cmd_setHandler_remote(std::vector<std::string>& tokens);

    bool cmd_unwatch_serial(std::string& cmd_str);
    bool cmd_unwatch_thread(std::string& cmd_str);
    bool cmd_unwatch_rank_serial(std::string& cmd_str);
    bool cmd_unwatch_rank_parallel(std::string& cmd_str);
    bool cmd_unwatch_remote(std::vector<std::string>& tokens);

    // Simulation
    bool cmd_run(std::string& cmd_str);

    bool cmd_exit_serial(std::string& cmd_str);
    bool cmd_exit_thread(std::string& cmd_str);
    bool cmd_exit_rank_serial(std::string& cmd_str);
    bool cmd_exit_rank_parallel(std::string& cmd_str);

    bool cmd_shutdown(std::string& cmd_str);

    // Logging/Replay
    bool cmd_logging(std::string& cmd_str);
    bool cmd_replay(std::string& cmd_str);
    bool cmd_history(std::string& cmd_str);

    // Auto-completion toggle
    bool cmd_autoComplete(std::string& cmd_str);

    // Reset terminal
    bool cmd_clear(std::string& cmd_str);

    // User defined commands
    bool cmd_define(std::string& cmd_str);
    bool cmd_document(std::string& cmd_str);

    // command entry point
    bool dispatch_cmd(std::string& cmd);

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
    static uint32_t                 current_thread;
    static uint32_t                 current_rank;
    static std::vector<std::string> tokens;
    static std::stringstream        result;

    int  consoleExecute(const std::string& msg);
    int  executeThread(const std::string& msg);
    int  executeRankSerial(const std::string& msg);
    int  executeRankParallel(const std::string& msg);
    bool handleCommand();
    bool handleCommandAll();
    bool sendCommand(uint32_t rank_id, uint32_t thread_id, const std::string& cmd);
    bool sendCommandAll(const std::string& cmd);
    void receiveCommandRankSerial();
    void receiveCommandRankParallel();
    bool sendDone();
};

} // namespace SST::IMPL::Interactive

#endif
