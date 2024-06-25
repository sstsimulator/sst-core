// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SST_NAMECHECK_H
#define SST_CORE_SST_NAMECHECK_H

#include <string>

namespace SST {

class NameCheck
{

    /**
       Checks to see if a name is valid according to the SST naming
       convention.

       Names can start with a letter or an underscore, but not a
       double underscore.  Names also cannot consist of only an
       underscore.  There can also be a dot ('.') in the name, but
       each segment on either side of the dot must be a valid name in
       and of itself.  Name cannot end with a dot.  Anywhere a number
       can go, you can also have a number wildcard with one of the
       following formats: %d, %(some documentation)d.  The use of dots
       and wildcards can be turned on and off with the proper flags.

       @param name Name to check for validity

       @param allow_wildcard If true, the %d format is considered
       valid, if false, using %d will result in a failed check

       @param allow_dot If true, a dot ('.') can be used in the name,
       if false, using a dot will result in a failed check

       @return true if name is valid, false otherwise
    */
    static bool isNameValid(const std::string& name, bool allow_wildcard, bool allow_dot);

public:
    static inline bool isComponentNameValid(const std::string& name) { return isNameValid(name, false, true); }

    static inline bool isLinkNameValid(const std::string& name) { return isNameValid(name, false, true); }

    static inline bool isParamNameValid(const std::string& name) { return isNameValid(name, true, true); }

    static inline bool isPortNameValid(const std::string& name) { return isNameValid(name, true, true); }

    static inline bool isSlotNameValid(const std::string& name) { return isNameValid(name, false, false); }
};

} // namespace SST

#endif // SST_CORE_SST_NAMECHECK_H
