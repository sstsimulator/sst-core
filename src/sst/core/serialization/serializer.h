/*
 *  This file is part of SST/macroscale:
 *               The macroscale architecture simulator from the SST suite.
 *  Copyright (c) 2009-2018 NTESS.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-NA0003525 with NTESS,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top
 *  SST/macroscale directory.
 */

#ifndef SPROCKIT_COMMON_MESSAGES_spkt_serializer_H_INCLUDED
#define SPROCKIT_COMMON_MESSAGES_spkt_serializer_H_INCLUDED

//#include <sprockit/spkt_config.h>
#include <sst/core/serialization/serialize_packer.h>
#include <sst/core/serialization/serialize_sizer.h>
#include <sst/core/serialization/serialize_unpacker.h>
#include <typeinfo>

#include <cstring>
#include <list>
#include <vector>
#include <map>
#include <set>

namespace SST {
namespace Core {
namespace Serialization {

/**
  * This class is basically a wrapper for objects to declare the order in
  * which their members should be ser/des
  */
class serializer
{
public:
    typedef enum {
        SIZER, PACK, UNPACK
    } SERIALIZE_MODE;
    
public:
    serializer() :
        mode_(SIZER) //just sizing by default
        {
        }
    
    pvt::ser_packer&
    packer() {
        return packer_;
    }
    
    pvt::ser_unpacker&
    unpacker() {
        return unpacker_;
    }
    
    pvt::ser_sizer&
    sizer() {
        return sizer_;
    }
    
    template <class T>
    void
    size(T& t){
        sizer_.size<T>(t);
    }
    
    template <class T>
    void
    pack(T& t){
        packer_.pack<T>(t);
    }
  
    template <class T>
    void
    unpack(T& t){
        unpacker_.unpack<T>(t);
    }

    virtual
    ~serializer() {
    }

    SERIALIZE_MODE
    mode() const {
        return mode_;
    }

    void
    set_mode(SERIALIZE_MODE mode) {
        mode_ = mode;
    }

    void
    reset(){
        sizer_.reset();
        packer_.reset();
        unpacker_.reset();
    }

    template<typename T>
    void
    primitive(T &t) {
        switch(mode_) {
        case SIZER:
            sizer_.size(t);
            break;
        case PACK:
            packer_.pack(t);
            break;
        case UNPACK:
            unpacker_.unpack(t);
            break;
        }
    }
  
    template <class T, int N>
    void
    array(T arr[N]){
        switch (mode_) {
        case SIZER: {
            sizer_.add(sizeof(T) * N);
            break;
        }
        case PACK: {
            char* charstr = packer_.next_str(N*sizeof(T));
            ::memcpy(charstr, arr, N*sizeof(T));
            break;
        }
        case UNPACK: {
            char* charstr = unpacker_.next_str(N*sizeof(T));
            ::memcpy(arr, charstr, N*sizeof(T));
            break;
        }   
        }
    }

    template <typename T, typename Int>
    void
    binary(T*& buffer, Int& size){
        switch (mode_) {
        case SIZER: {
            sizer_.add(sizeof(Int));
            sizer_.add(size);
            break;
        }
        case PACK: {
            if (buffer){
              packer_.pack(size);
              packer_.pack_buffer(buffer, size*sizeof(T));
            } else {
              Int nullsize = 0;
              packer_.pack(nullsize);
            }
            break;
        }
        case UNPACK: {
            unpacker_.unpack(size);
            if (size != 0){
              unpacker_.unpack_buffer(&buffer, size*sizeof(T));
            } else {
              buffer = nullptr;
            }
            break;
        }
        }
    }

    // For void*, we get sizeof(void), which errors.
    // Create a wrapper that casts to char* and uses above
    template <typename Int>
    void
    binary(void*& buffer, Int& size){
      char* tmp = (char*) buffer;
      binary<char>(tmp, size);
      buffer = tmp;
    }
  
    void
    string(std::string& str);
    
    void
    start_packing(char* buffer, size_t size){
        packer_.init(buffer, size);
        mode_ = PACK;
    }

    void
    start_sizing(){
        sizer_.reset();
        mode_ = SIZER;
    }

    void
    start_unpacking(char* buffer, size_t size){
        unpacker_.init(buffer, size);
        mode_ = UNPACK;
    }
    
    size_t
    size() const {
        switch (mode_){
        case SIZER: return sizer_.size();
        case PACK: return packer_.size();
        case UNPACK: return unpacker_.size();
        }
        return 0;
    }

protected:
    //only one of these is going to be valid for this spkt_serializer
    //not very good class design, but a little more convenient
    pvt::ser_packer packer_;
    pvt::ser_unpacker unpacker_;
    pvt::ser_sizer sizer_;
    SERIALIZE_MODE mode_;

};

}
}
}

#endif

