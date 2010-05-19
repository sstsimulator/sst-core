// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_BOOST_H
#define SST_BOOST_H

#warning "File boost.h is deprecated.  Do not include."

#if 0

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/split_member.hpp>

#define BOOST_SERIALIZE \
            friend class boost::serialization::access;\
            template<class Archive> \
            void serialize(Archive & ar, const unsigned int version )
            
#define BOOST_SERIALIZE_SAVE \
        template<class Archive> \
        void save(Archive & ar, const unsigned int version) const

#define BOOST_SERIALIZE_LOAD \
        template<class Archive> \
        void load(Archive & ar, const unsigned int version)

#define SAVE_CONSTRUCT_DATA(type) \
        template<class Archive> \
        friend void save_construct_data \
                (Archive & ar, const type * t, const unsigned int file_version)


#define LOAD_CONSTRUCT_DATA(type) \
        template<class Archive> \
        friend void load_construct_data \
                (Archive & ar, type * t, const unsigned int file_version)

#define BOOST_VOID_CAST_REGISTER(X,Y) \
            boost::serialization::void_cast_register(\
                static_cast<X>(NULL), static_cast<Y>(NULL)\
            )

#if BOOST_VERSION < 103900

#define BOOST_CLASS_EXPORT_TEMPLATE3( name, arg1, arg2, arg3 )          \
    namespace boost {                                                   \
    namespace archive {                                                 \
    namespace detail {                                                  \
    template<>                                                          \
    struct init_guid< name<arg1,arg2,arg3> > {                          \
        static ::boost::archive::detail::guid_initializer<              \
                                        name<arg1,arg2,arg3> > const    \
            & guid_initializer;                                         \
    };                                                                  \
    }}}                                                                 \
    ::boost::archive::detail::guid_initializer<                         \
                             name<arg1,arg2,arg3> > const &             \
        ::boost::archive::detail::init_guid<                            \
                        name<arg1,arg2,arg3> >::guid_initializer =      \
           ::boost::serialization::singleton<                           \
               ::boost::archive::detail::guid_initializer<              \
                                        name<arg1,arg2,arg3> >          \
                >::get_mutable_instance().export_guid                   \
                     (#name"<"#arg1","#arg2","#arg3">");                \
/**/

#define BOOST_CLASS_EXPORT_TEMPLATE4( name, arg1, arg2, arg3, arg4 )    \
    namespace boost {                                                   \
    namespace archive {                                                 \
    namespace detail {                                                  \
    template<>                                                          \
    struct init_guid< name<arg1,arg2,arg3,arg4> > {                     \
        static ::boost::archive::detail::guid_initializer<              \
                                        name<arg1,arg2,arg3,arg4> > const\
            & guid_initializer;                                         \
    };                                                                  \
    }}}                                                                 \
    ::boost::archive::detail::guid_initializer<                         \
                             name<arg1,arg2,arg3,arg4> > const &        \
        ::boost::archive::detail::init_guid<                            \
                        name<arg1,arg2,arg3,arg4> >::guid_initializer = \
           ::boost::serialization::singleton<                           \
               ::boost::archive::detail::guid_initializer<              \
                                        name<arg1,arg2,arg3,arg4> >     \
                >::get_mutable_instance().export_guid                   \
                     (#name"<"#arg1","#arg2","#arg3","#arg4">");        \
/**/

#else

#define BOOST_CLASS_EXPORT_TEMPLATE3( name, arg1, arg2, arg3 )          \
    namespace boost {                                                   \
    namespace archive {                                                 \
    namespace detail {                                                  \
    namespace {                                                         \
    template<>                                                          \
    struct init_guid< name<arg1,arg2,arg3> > {                          \
        static ::boost::archive::detail::guid_initializer<              \
                                        name<arg1,arg2,arg3> > const    \
            & guid_initializer;                                         \
    };                                                                  \
    }}}}                                                                \
    ::boost::archive::detail::guid_initializer<                         \
                             name<arg1,arg2,arg3> > const &             \
        ::boost::archive::detail::init_guid<                            \
                        name<arg1,arg2,arg3> >::guid_initializer =      \
           ::boost::serialization::singleton<                           \
               ::boost::archive::detail::guid_initializer<              \
                                        name<arg1,arg2,arg3> >          \
                >::get_mutable_instance().export_guid                   \
                     (#name"<"#arg1","#arg2","#arg3">");                \
/**/

#define BOOST_CLASS_EXPORT_TEMPLATE4( name, arg1, arg2, arg3, arg4 )    \
    namespace boost {                                                   \
    namespace archive {                                                 \
    namespace detail {                                                  \
    namespace {                                                         \
    template<>                                                          \
    struct init_guid< name<arg1,arg2,arg3,arg4> > {                     \
        static ::boost::archive::detail::guid_initializer<              \
                                        name<arg1,arg2,arg3,arg4> > const\
            & guid_initializer;                                         \
    };                                                                  \
    }}}}                                                                \
    ::boost::archive::detail::guid_initializer<                         \
                             name<arg1,arg2,arg3,arg4> > const &        \
        ::boost::archive::detail::init_guid<                            \
                        name<arg1,arg2,arg3,arg4> >::guid_initializer = \
           ::boost::serialization::singleton<                           \
               ::boost::archive::detail::guid_initializer<              \
                                        name<arg1,arg2,arg3,arg4> >     \
                >::get_mutable_instance().export_guid                   \
                     (#name"<"#arg1","#arg2","#arg3","#arg4">");        \
/**/

#endif // BOOST_VERSION < 103900

#endif

#endif // SST_BOOST_H
