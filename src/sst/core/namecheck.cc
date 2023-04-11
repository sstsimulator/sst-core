// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#include "sst/core/namecheck.h"

using namespace SST;

bool
NameCheck::isNameValid(const std::string& name, bool allow_wildcard, bool allow_dot)
{
    // Here is a five state state machine to check (states 3 and 4 can
    // only be reached if wildcards are allowed):

    // State 0: start of name, including immediately after a dot.
    // Will transition to state 1 on an underscore, to state 2
    // otherwise.

    // State 1: only used after an opening underscore to ensure that
    // the next character is either a number or letter.  Will
    // transition to state 2.

    // State 2: middle of word.  Underscores, numbers, letters and
    // dots are valid.  Will transition back to state 0 when it finds
    // a dot.

    // State 3: entered a potential number wildcard (found a %).  Will
    // transition to state 4 if it finds optional documentation
    // (i.e. open paren), or back to state 2 if it finds the 'd' from
    // %d.

    // State 4: optional documentation for %d.  Go until we find a
    // closing paren. Will transition back to state 3 on closing
    // paren.

    // Also, the valid flag is set to false when the parsing is in a
    // state where reaching the end of the name would constitute an
    // invalid name.

    int  state = 0;
    bool valid = false;

    for ( const auto& c : name ) {
        switch ( state ) {
        case 0:
            // Start of a name.  Must be underscore or letter
            if ( c == '_' ) {
                state = 1;
                valid = false; // Can't end after opening underscore
                break;
            }
            if ( !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') ) return false;
            state = 2;
            valid = true;
            break;
        case 1:
            // First character was an underscore, so now we can only
            // have letters or numbers

            // Look for a number wildcard
            if ( allow_wildcard && c == '%' ) {
                valid = false;
                state = 3;
                break;
            }

            if ( !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') ) return false;
            state = 2;
            valid = true;
            break;
        case 2:
            // Middle of word.  Letters, numbers, underscores and dot valid
            if ( allow_dot && c == '.' ) {
                state = 0;
                valid = false; // can't end after a dot
                break;
            }

            // Number wildcard
            if ( allow_wildcard && c == '%' ) {
                valid = false;
                state = 3;
                break;
            }

            if ( !(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') && !(c >= '0' && c <= '9') && c != '_' )
                return false;
            state = 2;
            valid = true;
            break;
        case 3:
            if ( c == 'd' ) {
                valid = true;
                state = 2;
                break;
            }

            if ( c == '(' ) {
                valid = false;
                state = 4;
                break;
            }

            // Didn't get a valid character
            return false;
            break;
        case 4:
            // Ignore everything until we find a closing paren.
            // Cannot generate a valid match from this state.
            valid = false;
            if ( c == ')' ) state = 3;
            break;
        }
    }
    return valid;
}
