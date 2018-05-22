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

#include <sst/core/serialization/statics.h>

namespace SST {
namespace Core {
namespace Serialization {

std::list<statics::clear_fxn>* statics::fxns_ = 0;

void
statics::register_finish(clear_fxn fxn)
{
  if (fxns_ == 0){
    fxns_ = new std::list<statics::clear_fxn>;
  }
  fxns_->push_back(fxn);
}

void
statics::finish()
{
  if (fxns_ == 0)
    return;

  std::list<clear_fxn>::iterator it, end = fxns_->end();
  for (it=fxns_->begin(); it != end; ++it){
    clear_fxn fxn = *it;
    fxn();
  }
  fxns_->clear();
  delete fxns_;
  fxns_ = 0;
}

}
}
}
