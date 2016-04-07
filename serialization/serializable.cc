/*
 *  This file is part of SST/macroscale:
 *               The macroscale architecture simulator from the SST suite.
 *  Copyright (c) 2009 Sandia Corporation.
 *  This software is distributed under the BSD License.
 *  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
 *  the U.S. Government retains certain rights in this software.
 *  For more information, see the LICENSE file in the top
 *  SST/macroscale directory.
 */

#include <sst/core/serialization/serializable.h>
#include <sst/core/serialization/statics.h>
#include <cstring>
#include <iostream>

namespace SST {
namespace Core {
namespace Serialization {

static need_delete_statics<serializable_factory> del_statics;
serializable_factory::builder_map* serializable_factory::builders_ = 0;

uint32_t
// serializable_factory::add_builder(serializable_builder* builder, uint32_t cls_id)
serializable_factory::add_builder(serializable_builder* builder)
{
  if (builders_ == 0) {
    builders_ = new builder_map;
  }

  // const char* key = builder->name();
  // int len = ::strlen(key);
  // uint32_t hash = 0;
  // for(int i = 0; i < len; ++i)
  // {
  //   hash += key[i];
  //   hash += (hash << 10);
  //   hash ^= (hash >> 6);
  // }
  // hash += (hash << 3);
  // hash ^= (hash >> 11);
  // hash += (hash << 15);

  // std::cout << "Using cls_id " << cls_id << " for class " << builder->name() << std::endl;
  
  uint32_t hash = builder->cls_id();
  
  builder_map& bmap = *builders_;
  serializable_builder*& current = bmap[hash];
  if (current != 0){
    // std::cerr << sprockit::printf(
    //   "amazingly %s and %s both hash to same serializable id %u",
    //   current->name(), builder->name(), hash) << std::endl;
      std::cerr << "amazingly " << current->name() << " and " << builder->name() <<
          " both hash to same serializable id " << hash << std::endl;
    abort();
  }
  current = builder;
  return hash;
}

void
serializable_factory::delete_statics()
{
//  delete_vals(*builders_);
  delete builders_;
}

serializable*
serializable_factory::get_serializable(uint32_t cls_id)
{
  builder_map::const_iterator it
    = builders_->find(cls_id);
  if (it == builders_->end()) {
      std::cerr << "class id " << cls_id << " is not a valid serializable id" << std::endl;
      // spkt_throw_printf(value_error,
    //                  "class id %ld is not a valid serializable id",
    //                  cls_id);
  }
  serializable_builder* builder = it->second;
  return builder->build();
}

}  // namespace Serialization
}  // namespace Core
}  // namespace SST
