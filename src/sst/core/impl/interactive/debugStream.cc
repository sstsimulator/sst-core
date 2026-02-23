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

#include "sst/core/impl/interactive/debugStream.h"

#include <iostream>

namespace SST::IMPL::Interactive {

std::streambuf::int_type
DebuggerStreamBuf::overflow(std::streambuf::int_type c)
{
    if ( quit_ ) {
        return EOF;
    }

    curChars_++;
    if ( '\n' == c ) {
        curLines_++;
        curChars_ = 0;
        if ( paginate_ && (curLines_ % linesPerScreen_ == 0) ) {
            dest_->sputc('\n');
            dest_->pubsync();
            std::cout << "--Type <RET> for more, q to quit, c to continue without paging--" << std::endl;
            char cmd = std::cin.get();
            if ( ('q' == cmd) || ('Q' == cmd) ) {
                quit_ = true;
                std::cin.ignore(1000, '\n');
                return EOF;
            }
            else if ( ('c' == cmd) || ('C' == cmd) ) {
                paginate_ = false;
                std::cin.ignore(1000, '\n');
            }
            else {
                if ( '\n' != cmd ) std::cin.ignore(1000, '\n');
            }
            return c;
        }
    }
    else if ( curChars_ == charsPerLine_ ) {
        dest_->sputc('.');
        dest_->sputc('.');
        return dest_->sputc('.');
    }
    else if ( curChars_ > charsPerLine_ ) {
        return c;
    }
    return dest_->sputc(c);
}

int
DebuggerStreamBuf::sync()
{
    return dest_->pubsync();
}

} // namespace SST::IMPL::Interactive
