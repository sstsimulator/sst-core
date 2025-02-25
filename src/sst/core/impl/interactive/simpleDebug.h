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
#include "sst/core/interactiveConsole.h"

namespace SST::IMPL::Interactive {

/**
   Self partitioner actually does nothing.  It is simply a pass
   through for graphs which have been partitioned during graph
   creation.
*/
class SimpleDebugger : public SST::InteractiveConsole
{

public:
    SST_ELI_REGISTER_INTERACTIVE_CONSOLE(
        SimpleDebugger, "sst", "interactive.simpledebug", SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "{EXPERIMENTAL} Basic interactive debugging console for interactive mode.")

    /**
       Creates a new self partition scheme.
    */
    SimpleDebugger(Params& params);

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

    std::vector<std::string> tokenize(std::vector<std::string>& tokens, const std::string& input);

    void cmd_pwd(std::vector<std::string>& UNUSED(tokens));
    void cmd_ls(std::vector<std::string>& UNUSED(tokens));
    void cmd_cd(std::vector<std::string>& tokens);
    void cmd_print(std::vector<std::string>& tokens);
    void cmd_set(std::vector<std::string>& tokens);
    void cmd_time(std::vector<std::string>& tokens);
    void cmd_run(std::vector<std::string>& tokens);

    void dispatch_cmd(std::string cmd);
};

} // namespace SST::IMPL::Interactive

#endif
