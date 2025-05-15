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

#ifndef SST_CORE_WARNMACROS_H
#define SST_CORE_WARNMACROS_H

#define UNUSED(x) x [[maybe_unused]]

#define DIAG_STR(s)              #s
#define DIAG_JOINSTR(x, y)       DIAG_STR(x##y)
#define DIAG_DO_PRAGMA(x)        _Pragma(#x)
#define DIAG_PRAGMA(compiler, x) DIAG_DO_PRAGMA(compiler diagnostic x)

#define DIAG_DISABLE(option)         \
    DIAG_PRAGMA(DIAG_COMPILER, push) \
    DIAG_PRAGMA(DIAG_COMPILER, ignored DIAG_JOINSTR(-W, option))

#define REENABLE_WARNING DIAG_PRAGMA(DIAG_COMPILER, pop)

#if defined(__clang__)
#define DIAG_COMPILER clang

#define DISABLE_WARN_DEPRECATED_REGISTER DIAG_DISABLE(deprecated-register)

#define DISABLE_WARN_FORMAT_SECURITY DIAG_DISABLE(format-security)

#define DISABLE_WARN_MAYBE_UNINITIALIZED DIAG_DISABLE(uninitialized)

#define DISABLE_WARN_STRICT_ALIASING DIAG_DISABLE(strict-aliasing)

#if ( __clang_major__ >= 12 )
#define DISABLE_WARN_MISSING_OVERRIDE DIAG_DISABLE(suggest-override)
#else
#define DISABLE_WARN_MISSING_OVERRIDE DIAG_DISABLE(inconsistent-missing-override)
#endif

#define DISABLE_WARN_DEPRECATED_DECLARATION DIAG_DISABLE(deprecated-declarations)

#define DISABLE_WARN_CAST_FUNCTION_TYPE DIAG_DISABLE(cast-function-type)

#define DISABLE_WARN_DANGLING_POINTER DIAG_PRAGMA(DIAG_COMPILER, push)

#define DISABLE_WARN_MISSING_NORETURN DIAG_PRAGMA(DIAG_COMPILER, push)

#elif defined(__GNUC__)

#define DIAG_COMPILER GCC

#define DISABLE_WARN_DEPRECATED_REGISTER DIAG_PRAGMA(DIAG_COMPILER, push)

#define DISABLE_WARN_FORMAT_SECURITY DIAG_DISABLE(format-security)

#define DISABLE_WARN_MAYBE_UNINITIALIZED DIAG_DISABLE(maybe-uninitialized)

#define DISABLE_WARN_STRICT_ALIASING DIAG_DISABLE(strict-aliasing)

#define DISABLE_WARN_MISSING_NORETURN DIAG_DISABLE(missing-noreturn)

#if ( __GNUC__ >= 5 )
#define DISABLE_WARN_MISSING_OVERRIDE DIAG_DISABLE(suggest-override)
#else
#define DISABLE_WARN_MISSING_OVERRIDE DIAG_PRAGMA(DIAG_COMPILER, push)
#endif

#define DISABLE_WARN_DEPRECATED_DECLARATION DIAG_DISABLE(deprecated-declarations)

#define DISABLE_WARN_CAST_FUNCTION_TYPE DIAG_DISABLE(cast-function-type)

#if ( __GNUC__ >= 12 )
#define DISABLE_WARN_DANGLING_POINTER DIAG_DISABLE(dangling-pointer)
#else
#define DISABLE_WARN_DANGLING_POINTER DIAG_PRAGMA(DIAG_COMPILER, push)
#endif

#else

#undef REENABLE_WARNING
#define REENABLE_WARNING
#define DISABLE_WARN_DEPRECATED_REGISTER
#define DISABLE_WARN_FORMAT_SECURITY
#define DISABLE_WARN_MAYBE_UNINITIALIZED
#define DISABLE_WARN_STRICT_ALIASING
#define DISABLE_WARN_MISSING_OVERRIDE
#define DISABLE_WARN_CAST_FUNCTION_TYPE
#define DISABLE_WARN_DANGLING_POINTER
#define DISABLE_WARN_MISSING_NORETURN

#endif

#endif
