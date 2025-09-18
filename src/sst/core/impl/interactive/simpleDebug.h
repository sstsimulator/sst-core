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

#include "sst/core/eli/elementinfo.h"
#include "sst/core/serialization/objectMapDeferred.h"

#include <sst/core/interactiveConsole.h>
#include <sst/core/watchPoint.h>
#include <string>
#include <vector>
// #include "probe.h"
// using namespace SSTDEBUG::Probe;

namespace SST::IMPL::Interactive {

class SimpleDebugger : public SST::InteractiveConsole
{

public:
    SST_ELI_REGISTER_INTERACTIVE_CONSOLE(
      SimpleDebugger,   // class
      "sst",     // library
      "interactive.simpledebug", // name
      SST_ELI_ELEMENT_VERSION(1, 0, 0),
      "{EXPERIMENTAL} Interactive console debug probe")

    /**
           Creates a new self partition scheme.
    */
    explicit SimpleDebugger(Params& params);
    ~SimpleDebugger() {}

    void execute(const std::string& msg) override;

private:
    // This is the stack of where we are in the class hierarchy.  This
    // is needed because when we advance time, we'll need to delete
    // any ObjectMap because they could change during execution.
    // After running, this will allow us to recreate the working
    // directory as far as we can.
    std::vector<std::string> name_stack;

    SST::Core::Serialization::ObjectMap* obj_ = nullptr;
    bool                                 done = false;

    // Keep a pointer to the ObjectMap for the top level Component
    SST::Core::Serialization::ObjectMapDeferred<BaseComponent>* base_comp_ = nullptr;

    // Keep track of all the WatchPoints
    std::vector<std::pair<WatchPoint*, BaseComponent*>> watch_points_;

    std::vector<std::string> tokenize(std::vector<std::string>& tokens, const std::string& input);

    void cmd_help(std::vector<std::string>& UNUSED(tokens));
    void cmd_pwd(std::vector<std::string>& UNUSED(tokens));
    void cmd_ls(std::vector<std::string>& UNUSED(tokens));
    void cmd_cd(std::vector<std::string>& tokens);
    void cmd_print(std::vector<std::string>& tokens);
    void cmd_set(std::vector<std::string>& tokens);
    void cmd_time(std::vector<std::string>& tokens);
    void cmd_run(std::vector<std::string>& tokens);
    void cmd_watch(std::vector<std::string>& tokens);
    void cmd_unwatch(std::vector<std::string>& tokens);
    void cmd_shutdown(std::vector<std::string>& tokens);
    // New functionality
    void cmd_exit(std::vector<std::string>& UNUSED(tokens));
    void cmd_watchlist(std::vector<std::string>& tokens);
    void cmd_trace(std::vector<std::string>& tokens);
    void cmd_setHandler(std::vector<std::string>& tokens);
    void cmd_addTraceVar(std::vector<std::string>& tokens);
    void cmd_resetTraceBuffer(std::vector<std::string>& tokens);
    void cmd_printTrace(std::vector<std::string>& tokens);
    void cmd_printWatchpoint(std::vector<std::string>& tokens);

    void dispatch_cmd(std::string cmd);
};

} // namespace SST::IMPL::Interactive

#endif
