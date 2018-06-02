// -*- c++ -*-

// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_WARNMACROS_H
#define SST_CORE_WARNMACROS_H


#define UNUSED(x) x __attribute__((unused))


#define DIAG_STR(s) #s
#define DIAG_JOINSTR(x,y) DIAG_STR(x ## y)
#define DIAG_DO_PRAGMA(x) _Pragma(#x)
#define DIAG_PRAGMA(compiler,x) DIAG_DO_PRAGMA(compiler diagnostic x)


#define DIAG_DISABLE(option) \
    DIAG_PRAGMA(DIAG_COMPILER,push) \
    DIAG_PRAGMA(DIAG_COMPILER,ignored DIAG_JOINSTR(-W,option))

#define REENABLE_WARNING \
    DIAG_PRAGMA(DIAG_COMPILER,pop)

#if defined(__clang__)
#define DIAG_COMPILER clang


#define DISABLE_WARN_DEPRECATED_REGISTER \
    DIAG_DISABLE(deprecated-register)

#define DISABLE_WARN_STRICT_ALIASING \
    DIAG_DISABLE(strict-aliasing)

#define DISABLE_WARN_MISSING_OVERRIDE \
    DIAG_DISABLE(inconsistent-missing-override)


#elif defined(__GNUC__)
#define DIAG_COMPILER GCC

#define DISABLE_WARN_DEPRECATED_REGISTER

#define DISABLE_WARN_STRICT_ALIASING \
    DIAG_DISABLE(strict-aliasing)

#if (__GNUC__ >= 5)
#define DISABLE_WARN_MISSING_OVERRIDE \
    DIAG_DISABLE(suggest-override)
#else
#define DISABLE_WARN_MISSING_OVERRIDE
#endif

#else

#undef REENABLE_WARNING
#define REENABLE_WARNING
#define DISABLE_WARN_DEPRECATED_REGISTER
#define DISABLE_WARN_STRICT_ALIASING
#define DISABLE_WARN_MISSING_OVERRIDE
#endif

#endif
