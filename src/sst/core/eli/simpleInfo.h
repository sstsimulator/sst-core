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

#ifndef SST_CORE_ELI_SIMPLE_INFO_H
#define SST_CORE_ELI_SIMPLE_INFO_H

#include "sst/core/eli/elibase.h"

namespace SST {
namespace ELI {

// ProvidesSimpleInfo is a class to quickly add ELI info to an ELI
// Base API.  This class should only be used for APIs that aren't
// reported in sst-info, since you can't override the plain text and
// XML printing functions.


// Class used to differentiate the different versions of ELI_getSimpleInfo
template <int num, typename InfoType>
struct SimpleInfoPlaceHolder
{};

//  Class to check for an ELI_getSimpleInfo Function.  The
//  MethodDetect way to detect a member function doesn't seem to work
//  for functions with specific type signatures and we need to
//  differentiate between the various versions using the first
//  parameter to the function (index + type).
template <class T, int index, class InfoType>
class checkForELI_getSimpleInfoFunction
{
    template <typename F, F>
    struct check;

    typedef char Match;
    typedef long NotMatch;

    typedef const InfoType& (*functionsig)(SimpleInfoPlaceHolder<index, InfoType>);

    template <typename F>
    static Match HasFunction(check<functionsig, &F::ELI_getSimpleInfo>*);

    template <typename F>
    static NotMatch HasFunction(...);

public:
    static bool const value = (sizeof(HasFunction<T>(0)) == sizeof(Match));
};

// Actual functions that use checkForELI_getSimpleInfoFunction class
// to create functions to get the information from the class
template <class T, int index, class InfoType>
typename std::enable_if<checkForELI_getSimpleInfoFunction<T, index, InfoType>::value, const InfoType&>::type
ELI_templatedGetSimpleInfo()
{
    return T::ELI_getSimpleInfo(SimpleInfoPlaceHolder<index, InfoType>());
}

template <class T, int index, class InfoType>
typename std::enable_if<not checkForELI_getSimpleInfoFunction<T, index, InfoType>::value, const InfoType&>::type
ELI_templatedGetSimpleInfo()
{
    static InfoType var;
    return var;
}

// Class that lets you add ELI information to an ELI API.  This class
// can be used with any type/class that can be initialized using list
// initialization (x = {}).  You have to provide an index as well as
// the type to be store so that the templating system can
// differentiate between different items of the same type.  It would
// be cool if we could template on a string instead so they could be
// named, but that doesn't seem to work in c++ 11.
template <int num, typename InfoType>
class ProvidesSimpleInfo
{
public:
    const InfoType& getSimpleInfo() const { return info_; }

protected:
    template <class T>
    ProvidesSimpleInfo(T* UNUSED(t)) : info_(ELI_templatedGetSimpleInfo<T, num, InfoType>())
    {}

private:
    InfoType info_;
};

} // namespace ELI
} // namespace SST

// Macro used by the API to create macros to populate the added ELI
// info
#define SST_ELI_DOCUMENT_SIMPLE_INFO(type, index, ...)                                           \
    static const type& ELI_getSimpleInfo(SST::ELI::SimpleInfoPlaceHolder<index, type> UNUSED(a)) \
    {                                                                                            \
        static type my_info = { __VA_ARGS__ };                                                   \
        return my_info;                                                                          \
    }

#endif // SST_CORE_ELI_SIMPLE_INFO_H
