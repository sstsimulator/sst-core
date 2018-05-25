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

#ifndef SST_CORE_SERIALIZATION_STATICS_H
#define SST_CORE_SERIALIZATION_STATICS_H

#include <list>

namespace SST {
namespace Core {
namespace Serialization {

class statics {
 public:
  typedef void (*clear_fxn)(void);

  static void
  register_finish(clear_fxn fxn);

  static void
  finish();

 protected:
  static std::list<clear_fxn>* fxns_;

};

template <class T>
class need_delete_statics {
 public:
  need_delete_statics(){
    statics::register_finish(&T::delete_statics);
  }
};

#define free_static_ptr(x) \
 if (x) delete x; x = 0

}
}
}

#endif // STATICS_H
