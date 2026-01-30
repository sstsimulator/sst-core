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

#ifndef SST_CORE_IMPL_INTERACTIVE_DEBUGSTREAM_H
#define SST_CORE_IMPL_INTERACTIVE_DEBUGSTREAM_H

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace SST::IMPL::Interactive {

class DebuggerStreamBuf : public std::streambuf {
    private:
    std::streambuf* dest_;
    unsigned        linesPerScreen_;
    unsigned        curLines_;
    bool            paginate_;
    bool            quit_;
    unsigned        charsPerLine_;
    unsigned        curChars_;

    protected:
    virtual int_type overflow(int_type c) override;
    virtual int sync() override;


    public:
    DebuggerStreamBuf(std::streambuf* dest, unsigned linesPerScreen, unsigned charsPerLine) 
        : dest_(dest), linesPerScreen_(linesPerScreen), curLines_(0),
          paginate_(true), quit_(false), charsPerLine_(charsPerLine), curChars_(0) {}

    void reset(){
            paginate_ = true;
            quit_     = false;
            curLines_ = 0;
            curChars_ = 0;
    }
    void setLineWidth(const unsigned w){charsPerLine_ = w;}

};

class DebuggerStream : public std::ostream {
    private:
    DebuggerStreamBuf buf_;

    public:
    DebuggerStream(std::ostream& dest, unsigned linesPerScreen, unsigned charsPerLine)
        : std::ostream(&buf_), buf_(dest.rdbuf(), linesPerScreen, charsPerLine) {}

    void reset(){
        buf_.reset();
        clear();
    }

    void setLineWidth(const unsigned w){ buf_.setLineWidth(w);}

};

inline DebuggerStream& operator<<(DebuggerStream& stream, DebuggerStream& (*func)(DebuggerStream&)) {
    return func(stream);
}

inline DebuggerStream& dreset(DebuggerStream& stream) {
    stream.reset();
    stream.flush();
    return stream;
}


}

#endif