// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef SST_CORE_HANDLER_H
#define SST_CORE_HANDLER_H

namespace SST {

    /// Functor classes for Event handling
    template <typename paramT, typename returnT>
    class HandlerBase {
    public:
        /** Handler function */
        virtual returnT operator()(paramT) = 0;
        virtual ~HandlerBase() {}
    };
    
    
    /**
     * Event Handler class with user-data argument
     */
    template <typename paramT, typename returnT, typename classT, typename argT = void>
    class Handler : public HandlerBase<paramT, returnT> {
    private:
        typedef returnT (classT::*PtrMember)(paramT, argT);
        classT* object;
        const PtrMember member;
        argT data;

    public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         * @param data - Additional argument to pass to handler
         */
        Handler( classT* const object, PtrMember member, argT data ) :
            object(object),
            member(member),
            data(data)
        {}

        returnT operator()(paramT input) {
            return (object->*member)(input,data);
        }
    };


    /**
     * Event Handler class with no user-data.
     */
    template <typename paramT, typename returnT, typename classT>
    class Handler<paramT, returnT, classT, void> : public HandlerBase<paramT, returnT> {
    private:
        typedef returnT (classT::*PtrMember)(paramT);
        const PtrMember member;
        classT* object;
        
    public:
        /** Constructor
         * @param object - Pointer to Object upon which to call the handler
         * @param member - Member function to call as the handler
         */
        Handler( classT* const object, PtrMember member ) :
            member(member),
            object(object)
        {}
        
        returnT operator()(paramT input) {
            return (object->*member)(input);
        }
    };
    
} //namespace SST

// BOOST_CLASS_EXPORT_KEY(SST::Activity)

#endif // SST_CORE_HANDLER_H
