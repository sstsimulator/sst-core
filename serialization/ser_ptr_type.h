#ifndef SER_PTR_TYPE_H
#define SER_PTR_TYPE_H

#include <sprockit/ptr_type.h>
#include <sprockit/serializable.h>

namespace sprockit {

class serializable_ptr_type :
  public ptr_type,
  public serializable
{
 public:
  typedef sprockit::refcount_ptr<serializable_ptr_type> ptr;

};

}

#endif // SER_PTR_TYPE_H
