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

#ifndef SST_CORE_SERIALIZATION_OBJECTTREEHELPERS_DEBUGGER_H
#define SST_CORE_SERIALIZATION_OBJECTTREEHELPERS_DEBUGGER_H

#include "sst/core/serialization/ObjTree.h"

#include <climits>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iostream>
#include <limits.h>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace SST::Core::Serialization {

class ObjTreeComparison
{
public:
    enum class Op : std::uint8_t { LT, LTE, GT, GTE, EQ, NEQ, CHANGED, INVALID };
    enum class valType : std::uint8_t { OBJ, CONST, UNKNOWN };
    static Op getOperationFromString(const std::string& op)
    {
        if ( op == "<" ) return Op::LT;
        if ( op == "<=" ) return Op::LTE;
        if ( op == ">" ) return Op::GT;
        if ( op == ">=" ) return Op::GTE;
        if ( op == "==" ) return Op::EQ;
        if ( op == "!=" ) return Op::NEQ;
        if ( op == "changed" ) return Op::CHANGED; // Could also use <>
        return Op::INVALID;
    }

    static std::string getStringFromOp(Op op)
    {
        switch ( op ) {
        case Op::LT:
            return "<";
        case Op::LTE:
            return "<+";
        case Op::GT:
            return ">";
        case Op::GTE:
            return ">=";
        case Op::EQ:
            return "==";
        case Op::NEQ:
            return "!=";
        case Op::CHANGED:
            return "CHANGED";
        case Op::INVALID:
            return "INVALID";
        default:
            return "Invalid Op";
        }
    }
    // TODO: add some limits on these
    // The full path and name to the objects for compairson. Vector contains the full path to each object stored as
    // individual
    //  elements within the dequeue. Ex: /comp0/subcomp/target --> [comp0][subcomp][target] within the dequeue
    std::vector<std::tuple<std::deque<std::string>, valType, std::unique_ptr<ObjTreeCont>>> objectsToCompare;
    std::vector<Op>                                                                         operators;

    void print(std::stringstream& s, unsigned startIdx, unsigned stopIdx) const
    {
        if ( objectsToCompare.empty() || operators.empty() ) {
            return;
        }
        if ( objectsToCompare.size() < startIdx || objectsToCompare.size() < stopIdx ) {
            return;
        }
        for ( size_t i = startIdx; i < stopIdx; i++ ) {
            if ( std::get<valType>(objectsToCompare[i]) == valType::UNKNOWN ) {
                continue;
            }
            if ( std::get<valType>(objectsToCompare[i]) == valType::CONST ) {
                s << std::get<std::deque<std::string>>(objectsToCompare[i]).back();
                s << " ";
                continue;
            }
            else {
                for ( size_t j = 0; j < std::get<std::deque<std::string>>(objectsToCompare[i]).size(); j++ ) {
                    s << "/";
                    s << std::get<std::deque<std::string>>(objectsToCompare[i])[j];
                }
            }
            if ( (i % 2 == 0) && (i / 2 < operators.size()) ) {
                s << " " << getStringFromOp(operators[i / 2]);
            }
            s << " ";
        }
        // s << std::endl;
    }

    bool evaluateComparison(unsigned index, [[maybe_unused]] SST::Core::Serialization::ObjTreeCont* treeRoot);

    ObjTreeComparison()  = default;
    ~ObjTreeComparison() = default;
};

class ObjTreeTraceBuffer
{

public:
    ObjTreeTraceBuffer(size_t sz, size_t pdelay) :
        bufSize_(sz),
        postDelay_(pdelay)
    {
        tagBuffer_.resize(bufSize_);
        cycleBuffer_.resize(bufSize_);
        handlerBuffer_.resize(bufSize_);
    }

    virtual ~ObjTreeTraceBuffer() = default;

    void setBufferReset() { reset_ = true; }

    void resetTraceBuffer()
    {
        postCount_   = 0;
        cur_         = 0;
        first_       = 0;
        numRecs_     = 0;
        samplesLost_ = 0;
        isOverrun_   = false;
        reset_       = false;
        state_       = CLEAR;

        // should we do a syncFromSim on the remaining objBuffers?
        for ( size_t obj = 0; obj < objBuffers_.size(); obj++ ) {
            auto& [bufVec, storedTriggerIdx] = objBuffers_[obj];
            auto e                           = bufVec.end();
            e--;
            bufVec.erase(bufVec.begin(), e);
            bufVec.front()->syncFromSim();
            storedTriggerIdx = INT_MAX;
        }
    }

    size_t getBufferSize() { return bufSize_; }

    void addObjectBuffer(std::unique_ptr<ObjTreeCont> vb)
    {
        std::vector<std::unique_ptr<ObjTreeCont>> newBuf;
        newBuf.push_back(std::move(vb));
        objBuffers_.emplace_back(std::move(newBuf), INT_MAX);
    }

    enum BufferState : int {
        CLEAR,       // 0 Pre Trigger
        TRIGGER,     // 1 Trigger
        POSTTRIGGER, // 2 Post Trigger
        OVERRUN      // 3 Overrun
    };

    const std::map<BufferState, char> state2char { { CLEAR, '-' }, { TRIGGER, '!' }, { POSTTRIGGER, '+' },
        { OVERRUN, 'o' } };

    bool sampleT(bool trigger, uint64_t cycle, const std::string& handler)
    {
        size_t start_state  = state_;
        bool   invokeAction = false;

        // if Trigger == TRUE
        if ( trigger ) {
            if ( start_state == CLEAR ) { // Not previously triggered
                state_ = TRIGGER;         // State becomes trigger record
            }
            // printf("    Sample: trigger\n");

        } // if trigger

        if ( start_state == TRIGGER || start_state == POSTTRIGGER ) { // trigger record or post trigger
            state_ = POSTTRIGGER;                                     // State becomes post trigger
            // printf("    Sample: post trigger\n");
        }

        // Circular buffer
        if ( !(state_ == POSTTRIGGER && postCount_ >= postDelay_) ) {
#ifdef _OBJMAP_DEBUG_
            std::cout << "    Sample:" << handler << ": numRecs:" << numRecs_ << " first:" << first_ << " cur:" << cur_
                      << " state:" << state2char.at(state_) << " isOverrun:" << isOverrun_
                      << " samplesLost:" << samplesLost_ << std::endl;
#endif
            cycleBuffer_[cur_]   = cycle;
            handlerBuffer_[cur_] = handler;
            if ( trigger ) {
                triggerCycle = cycle;
            }

            // Sample all the trace object buffers
            ObjTreeCont* varBuffer_;

            for ( size_t obj = 0; obj < objBuffers_.size(); obj++ ) {
                auto& [bufVec, storedTriggerIdx] = objBuffers_[obj];
                varBuffer_                       = bufVec.back().get();
                // varBuffer_->sample(cur_, trigger);
                if ( storedTriggerIdx != INT_MAX ) {
                    std::unique_ptr<ObjTreeCont> updatedBuf(varBuffer_->clone());
                    updatedBuf->syncFromSim();
                    bufVec.push_back(std::move(updatedBuf));
                    unsigned triggerIdx = trigger ? bufVec.size() - 1 : storedTriggerIdx;
                    storedTriggerIdx    = triggerIdx;
                }
                else {
                    bufVec.back()->syncFromSim();
                    storedTriggerIdx = 0;
                }
                // DDD: Add a way to flag which of these values was the trigger value
            }

            if ( numRecs_ < bufSize_ ) {
                tagBuffer_[cur_] = state_;
                numRecs_++;
                cur_ = (cur_ + 1) % bufSize_;
                if ( cur_ == 0 ) first_ = 0; // 1;
            }
            else { // Buffer full
                // Check to see if we are overwriting trigger
                if ( tagBuffer_[cur_] == TRIGGER ) {
                    // printf("    Sample Overrun\n");
                    isOverrun_ = true;
                }
                tagBuffer_[cur_] = state_;
                numRecs_++;
                cur_   = (cur_ + 1) % bufSize_;
                first_ = cur_;
                for ( size_t obj = 0; obj < objBuffers_.size(); obj++ ) {
                    auto& [bufVec, storedTriggerIdx] = objBuffers_[obj];
                    bufVec.erase(bufVec.begin());
                    if ( storedTriggerIdx == bufVec.size() ) {
                        storedTriggerIdx--;
                    }
                }
            }
        }

        if ( isOverrun_ ) {
            samplesLost_++;
        }

        if ( (state_ == TRIGGER) && (postDelay_ == 0) ) {
            invokeAction = true;
        }

        if ( state_ == POSTTRIGGER ) {
            postCount_++;
            if ( postCount_ >= postDelay_ ) {
                invokeAction = true;
            }
        }

        return invokeAction;
    }

    void dumpTraceBufferT(std::ostream& os = std::cout)
    {
        if ( numRecs_ == 0 ) return;

        size_t start;
        size_t end;

        start = first_;
        if ( cur_ == 0 ) {
            end = bufSize_ - 1;
        }
        else {
            end = cur_ - 1;
        }
        size_t buf = 0;
        for ( int j = start;; j++ ) {
            size_t i = j % bufSize_;

            os << "buf[" << i << "] " << handlerBuffer_.at(i) << " @" << cycleBuffer_.at(i) << " ("
               << state2char.at(tagBuffer_.at(i)) << ") ";

            std::ostringstream tmpBuf;
            std::string        objNames;
            for ( size_t obj = 0; obj < objBuffers_.size(); obj++ ) {
                ObjTreeCont* varBuffer_ =
                    std::get<std::vector<std::unique_ptr<ObjTreeCont>>>(objBuffers_[obj])[buf].get(); // DDD: Maybe?
                varBuffer_->Dump(1, std::ios_base::dec, tmpBuf);
                // std::cout << varBuffer_->getObjName() << "=" << varBuffer_->get() << " ";
                objNames.append(tmpBuf.str());
                if ( !objNames.empty() && objNames.back() == '\n' ) {
                    objNames.pop_back();
                    objNames.push_back(' ');
                }
                tmpBuf.str("");
                tmpBuf.clear();
            }
            os << objNames << std::endl;
            buf++;

            if ( i == end ) {
                break;
            }
        }
    }

    void dumpTriggerRecord(std::ostream& os = std::cout)
    {
        if ( numRecs_ == 0 ) {
            std::cout << "No trace samples in current buffer" << std::endl;
            return;
        }
        if ( state_ != CLEAR ) {
            std::stringstream  ss; // Need to buffer in a stream so it doesn't get split in parallel execution
            std::ostringstream tmpBuf;
            std::string        objNames;
            unsigned           idxToPrint = 0;
            // print trigger value for the one variable (variables?) that actually triggered
            ss << "LastTriggerRecord:@cycle" << triggerCycle << ": SamplesLost=" << samplesLost_ << ": ";
            for ( size_t obj = 0; obj < objBuffers_.size(); obj++ ) {
                unsigned triggerIdx = std::get<unsigned>(objBuffers_[obj]);
                auto*    record     = &std::get<std::vector<std::unique_ptr<ObjTreeCont>>>(objBuffers_[obj]);
                if ( triggerIdx < record->size() ) {
                    idxToPrint = triggerIdx;
                }
                else {
                    idxToPrint = record->size() - 1;
                }
                ObjTreeCont* varBuffer_ = (*record)[idxToPrint].get();
                // OK, so we have to use Dump() to get the value, but it adds a \n after each call, so strip it out
                varBuffer_->Dump(1, std::ios_base::dec, tmpBuf);
                objNames.append(tmpBuf.str());
                if ( !objNames.empty() && objNames.back() == '\n' ) {
                    objNames.pop_back();
                    objNames.push_back(' ');
                }
                tmpBuf.str("");
                tmpBuf.clear();
            }
            ss << objNames << std::endl;
            os << ss.str(); // Print everything at once to avoid splitting
        }
    }

    void printVars(std::stringstream& ss)
    {
        for ( size_t obj = 0; obj < objBuffers_.size(); obj++ ) {
            ObjTreeCont* varBuffer_ =
                std::get<std::vector<std::unique_ptr<ObjTreeCont>>>(objBuffers_[obj]).back().get();
            // ss << SST::Core::to_string(varBuffer_->getName()) << " ";
            ss << varBuffer_->getObjName() << " ";
        }
    }

    void printConfig(std::stringstream& ss)
    {
        ss << "bufsize = " << bufSize_ << " postDelay = " << postDelay_ << " : ";
        printVars(ss);
    }

    // private:
    size_t      bufSize_     = 64;
    size_t      postDelay_   = 8;
    size_t      postCount_   = 0;
    size_t      cur_         = 0;
    size_t      first_       = 0;
    size_t      numRecs_     = 0;
    bool        isOverrun_   = false;
    size_t      samplesLost_ = 0;
    bool        reset_       = false;
    BufferState state_       = CLEAR;

    std::vector<BufferState>                                                     tagBuffer_;
    std::vector<std::string>                                                     handlerBuffer_;
    // Vector of ObjTreeCont - organized as all the sampled values with the index of the last
    //   triggered value
    std::vector<std::tuple<std::vector<std::unique_ptr<ObjTreeCont>>, unsigned>> objBuffers_;
    std::vector<uint64_t>                                                        cycleBuffer_;
    uint64_t                                                                     triggerCycle;

}; // class TraceBuffer
} // namespace SST::Core::Serialization
#endif
