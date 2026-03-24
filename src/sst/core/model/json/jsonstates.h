// -*- c++ -*-

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

#ifndef SST_CORE_MODEL_JSON_JSONSTATES_H
#define SST_CORE_MODEL_JSON_JSONSTATES_H

namespace SST::Core::JSONParser {

// X-macros to generate state transition tables
// Hierarchical states are expanded to form a single state name by appending the nested state names with an underscore
// State tuple is: state, parent state, next state on object/array open or Invalid, next state on 'params' key
// Note that 'Invalid' just means that either (a) there is no transition or (b) the transition table can't be used to
// determine the transition
// '*_key' states indicate that the keyword for the next state has been seen and an object or array will be opened next

#define X_STATES                                                                                    \
    /* Entry (start state) */                                                                       \
    X(Entry, Invalid, Root, Invalid)                                                                \
    /* In root object */                                                                            \
    X(Root, Entry, Invalid, Invalid)                                                                \
    /* ------------------------- */                                                                 \
    /* Major Section: 'program_options' */                                                          \
    /* ------------------------- */                                                                 \
    /* Section keyword seen */                                                                      \
    X(ProgramOptions_key, Root, ProgramOptions, Invalid)                                            \
    /* In section object */                                                                         \
    X(ProgramOptions, Root, Invalid, Invalid)                                                       \
    /* ------------------------- */                                                                 \
    /* Major Section: 'shared_params' - object containing name/object pairs */                      \
    /* ------------------------- */                                                                 \
    /* Section keyword seen */                                                                      \
    X(SharedParams_key, Root, SharedParams, Invalid)                                                \
    /* In section's root object */                                                                  \
    X(SharedParams, Root, SharedParams_object_key, Invalid)                                         \
    /* Got name of next shared_param object */                                                      \
    X(SharedParams_object_key, SharedParams, SharedParams_object, Invalid)                          \
    /* In a shared_param object */                                                                  \
    X(SharedParams_object, SharedParams, Invalid, Invalid)                                          \
    /* ------------------------- */                                                                 \
    /* Major section: 'statistics_options' */                                                       \
    /* ------------------------- */                                                                 \
    /* Section keyword seen */                                                                      \
    X(StatOptions_key, Root, StatOptions, Invalid)                                                  \
    /* In 'statistics_options' object*/                                                             \
    X(StatOptions, Root, Invalid, StatOptions_Params_key)                                           \
    /* 'params' keyword seen */                                                                     \
    X(StatOptions_Params_key, StatOptions, StatOptions_Params, Invalid)                             \
    /* In params object*/                                                                           \
    X(StatOptions_Params, StatOptions, Invalid, Invalid)                                            \
    /* ------------------------- */                                                                 \
    /* Major section: 'statistics_group' */                                                         \
    /* ------------------------- */                                                                 \
    /* 'statistics_group' keyword seen */                                                           \
    X(StatGroupsArray_key, Root, StatGroupsArray, Invalid)                                          \
    /* In 'statistics_group' array */                                                               \
    X(StatGroupsArray, Root, StatGroup, Invalid)                                                    \
    /* In a statistics group object */                                                              \
    X(StatGroup, StatGroupsArray, Invalid, Invalid)                                                 \
    /* 'output' keyword seen */                                                                     \
    X(StatGroup_Output_key, StatGroup, StatGroup_Output, Invalid)                                   \
    /* In output object */                                                                          \
    X(StatGroup_Output, StatGroup, Invalid, StatGroup_Output_Params_key)                            \
    /* 'params' keyword seen */                                                                     \
    X(StatGroup_Output_Params_key, StatGroup_Output, StatGroup_Output_Params, Invalid)              \
    /* In params object */                                                                          \
    X(StatGroup_Output_Params, StatGroup_Output, Invalid, Invalid)                                  \
    /* 'statistics' keyword seen */                                                                 \
    X(StatGroup_StatArray_key, StatGroup, StatGroup_StatArray, Invalid)                             \
    /* In statistics array */                                                                       \
    X(StatGroup_StatArray, StatGroup, StatGroup_Stat, Invalid)                                      \
    /* In a statistic object */                                                                     \
    X(StatGroup_Stat, StatGroup_StatArray, Invalid, StatGroup_Stat_Params_key)                      \
    /* 'params' keyword seen */                                                                     \
    X(StatGroup_Stat_Params_key, StatGroup_Stat, StatGroup_Stat_Params, Invalid)                    \
    /* In params object */                                                                          \
    X(StatGroup_Stat_Params, StatGroup_Stat, Invalid, Invalid)                                      \
    /* 'components' keyword seen */                                                                 \
    X(StatGroup_CompArray_key, StatGroup, StatGroup_CompArray, Invalid)                             \
    /* In components array */                                                                       \
    X(StatGroup_CompArray, StatGroup, Invalid, Invalid)                                             \
    /* ------------------------- */                                                                 \
    /* Major section: 'components'*/                                                                \
    /* ------------------------- */                                                                 \
    /* 'components' keyword seen */                                                                 \
    X(CompArray_key, Root, CompArray, Invalid)                                                      \
    /* In components array */                                                                       \
    X(CompArray, Root, Comp, Invalid)                                                               \
    /* In a component object */                                                                     \
    X(Comp, CompArray, Invalid, Comp_Params_key)                                                    \
    /* 'params' keyword seen */                                                                     \
    X(Comp_Params_key, Comp, Comp_Params, Invalid)                                                  \
    /* In params object */                                                                          \
    X(Comp_Params, Comp, Invalid, Invalid)                                                          \
    /* 'statistics_options' keyword seen */                                                         \
    X(Comp_StatOptions_key, Comp, Comp_StatOptions, Invalid)                                        \
    /* In 'statistics_options' object*/                                                             \
    X(Comp_StatOptions, Comp, Invalid, Comp_StatOptions_Params_key)                                 \
    /* 'params' keyword seen */                                                                     \
    X(Comp_StatOptions_Params_key, Comp_StatOptions, Comp_StatOptions_Params, Invalid)              \
    /* In params object*/                                                                           \
    X(Comp_StatOptions_Params, Comp_StatOptions, Invalid, Invalid)                                  \
    /* 'statistics' keyword seen */                                                                 \
    X(Comp_StatArray_key, Comp, Comp_StatArray, Invalid)                                            \
    /* 'In statistics arrray */                                                                     \
    X(Comp_StatArray, Comp, Comp_Stat, Invalid)                                                     \
    /* In statistic object */                                                                       \
    X(Comp_Stat, Comp_StatArray, Invalid, Comp_Stat_Params_key)                                     \
    /* 'params' keyword seen */                                                                     \
    X(Comp_Stat_Params_key, Comp_Stat, Comp_Stat_Params, Invalid)                                   \
    /* In params object */                                                                          \
    X(Comp_Stat_Params, Comp_Stat, Invalid, Invalid)                                                \
    /* 'shared_params' keyword seen */                                                              \
    X(Comp_SharedParamsArray_key, Comp, Comp_SharedParamsArray, Invalid)                            \
    /* In shared_params array */                                                                    \
    X(Comp_SharedParamsArray, Comp, Invalid, Invalid)                                               \
    /* In partition object */                                                                       \
    X(Comp_Partition_key, Comp, Comp_Partition, Invalid)                                            \
    /* 'partition' keyword seen */                                                                  \
    X(Comp_Partition, Comp, Invalid, Invalid)                                                       \
    /* ------------------------- */                                                                 \
    /* Major SubSection: 'port_modules' in a component or subcomponent */                           \
    /* ------------------------- */                                                                 \
    /* 'port_module' keyword seen */                                                                \
    X(PortModArray_key, Invalid, PortModArray, Invalid)                                             \
    /* In port_module array */                                                                      \
    X(PortModArray, Invalid, PortMod, Invalid)                                                      \
    /* In port_module object */                                                                     \
    X(PortMod, PortModArray, Invalid, PortMod_Params)                                               \
    /* 'params' keyword seen*/                                                                      \
    X(PortMod_Params_key, PortMod, PortMod_Params, Invalid)                                         \
    /* In params object */                                                                          \
    X(PortMod_Params, PortMod, Invalid, Invalid)                                                    \
    /* 'shared_params' keyword seen */                                                              \
    X(PortMod_SharedParamsArray_key, PortMod, PortMod_SharedParamsArray, Invalid)                   \
    /* In shared_params array */                                                                    \
    X(PortMod_SharedParamsArray, PortMod, Invalid, Invalid)                                         \
    /* 'statistics' keyword seen */                                                                 \
    X(PortMod_StatArray_key, PortMod, PortMod_StatArray, Invalid)                                   \
    /* In statistics section */                                                                     \
    X(PortMod_StatArray, PortMod, PortMod_Stat, Invalid)                                            \
    /* In statistic object */                                                                       \
    X(PortMod_Stat, PortMod_StatArray, Invalid, PortMod_Stat_Params_key)                            \
    /* 'params' keyword seen */                                                                     \
    X(PortMod_Stat_Params_key, PortMod_Stat, PortMod_Stat_Params, Invalid)                          \
    /* In params object */                                                                          \
    X(PortMod_Stat_Params, PortMod_Stat, Invalid, Invalid)                                          \
    /* 'statistics_options' keyword seen */                                                         \
    X(PortMod_StatOptions_key, PortMod, PortMod_StatOptions, Invalid)                               \
    /* In 'statistics_options' object*/                                                             \
    X(PortMod_StatOptions, PortMod, Invalid, PortMod_StatOptions_Params_key)                        \
    /* 'params' keyword seen */                                                                     \
    X(PortMod_StatOptions_Params_key, PortMod_StatOptions, PortMod_StatOptions_Params, Invalid)     \
    /* In params object*/                                                                           \
    X(PortMod_StatOptions_Params, PortMod_StatOptions, Invalid, Invalid)                            \
    /* ------------------------- */                                                                 \
    /* Major SubSection: 'subcomponents' in a component or subcomponent */                          \
    /* Stack of currently open component objects indicates the nesting of parent (sub)components */ \
    /* ------------------------- */                                                                 \
    /* 'subcomponents' keyword seen */                                                              \
    X(SubCompArray_key, Invalid, SubCompArray, Invalid)                                             \
    /* In subcomponents array */                                                                    \
    X(SubCompArray, Invalid, SubComp, Invalid)                                                      \
    /* In subcomponent object */                                                                    \
    X(SubComp, SubCompArray, Invalid, SubComp_Params_key)                                           \
    /* 'params' keyword seen */                                                                     \
    X(SubComp_Params_key, SubComp, SubComp_Params, Invalid)                                         \
    /* In params object */                                                                          \
    X(SubComp_Params, SubComp, Invalid, Invalid)                                                    \
    /* 'statistics_options' keyword seen */                                                         \
    X(SubComp_StatOptions_key, SubComp, SubComp_StatOptions, Invalid)                               \
    /* In 'statistics_options' object*/                                                             \
    X(SubComp_StatOptions, SubComp, Invalid, SubComp_StatOptions_Params_key)                        \
    /* 'params' keyword seen */                                                                     \
    X(SubComp_StatOptions_Params_key, SubComp_StatOptions, SubComp_StatOptions_Params, Invalid)     \
    /* In params object*/                                                                           \
    X(SubComp_StatOptions_Params, SubComp_StatOptions, Invalid, Invalid)                            \
    /* 'statistics' keyword seen */                                                                 \
    X(SubComp_StatArray_key, SubComp, SubComp_StatArray, Invalid)                                   \
    /* 'In statistics arrray */                                                                     \
    X(SubComp_StatArray, SubComp, SubComp_Stat, Invalid)                                            \
    /* In statistic object */                                                                       \
    X(SubComp_Stat, SubComp_StatArray, Invalid, SubComp_Stat_Params_key)                            \
    /* 'params' keyword seen */                                                                     \
    X(SubComp_Stat_Params_key, SubComp_Stat, SubComp_Stat_Params, Invalid)                          \
    /* In params object */                                                                          \
    X(SubComp_Stat_Params, SubComp_Stat, Invalid, Invalid)                                          \
    /* 'shared_params' keyword seen */                                                              \
    X(SubComp_SharedParamsArray_key, SubComp, SubComp_SharedParamsArray, Invalid)                   \
    /* In shared_params array */                                                                    \
    X(SubComp_SharedParamsArray, SubComp, Invalid, Invalid)                                         \
    /* ------------------------- */                                                                 \
    /* Major section: 'links' */                                                                    \
    /* ------------------------- */                                                                 \
    /* 'links' key seen */                                                                          \
    X(LinkArray_key, Root, LinkArray, Invalid)                                                      \
    /* In links array */                                                                            \
    X(LinkArray, Root, Link, Invalid)                                                               \
    /* In a link object */                                                                          \
    X(Link, LinkArray, Invalid, Invalid)                                                            \
    /* 'left' key seen */                                                                           \
    X(Link_Left_key, Link, Link_Left, Invalid)                                                      \
    /* In left object */                                                                            \
    X(Link_Left, Link, Invalid, Invalid)                                                            \
    /* 'right' key seen */                                                                          \
    X(Link_Right_key, Link, Link_Right, Invalid)                                                    \
    /* In right object */                                                                           \
    X(Link_Right, Link, Invalid, Invalid)                                                           \
    /* ------------------------- */                                                                 \
    /* Invalid state */                                                                             \
    /* ------------------------- */                                                                 \
    X(Invalid, Invalid, Invalid, Invalid)

// Parser states
enum class State {
#define X(a, b, c, d) a,
    X_STATES
#undef X
};

// Parent states for each parser state
static const State ParentState[] = {
#define X(a, b, c, d) State::b,
    X_STATES
#undef X
};

// Next states when an array or object is started for each parser state
static const State NextStateObjOrArray[] = {
#define X(a, b, c, d) State::c,
    X_STATES
#undef X
};

// Next states when 'params' key is encountered for each parser state
static const State NextStateParams[] = {
#define X(a, b, c, d) State::d,
    X_STATES
#undef X
};

// Strings for error output
static const std::string StateString[] = {
#define X(a, b, c, d) #a,
    X_STATES
#undef X
};

#undef X_STATES

} // namespace SST::Core::JSONParser

#endif
