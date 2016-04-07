#ifndef SERIALIZE_SIZER_H
#define SERIALIZE_SIZER_H

namespace SST {
namespace Core {
namespace Serialization {
namespace pvt {

class ser_sizer
{
 public:
  ser_sizer() :
    size_(0)
  {
  }

  template <class T>
  void
  size(T& t){
    size_ += sizeof(T);
  }

  void
  size_string(std::string& str);

  void
  add(size_t s) {
    size_ += s;
  }

  size_t
  size() const {
    return size_;
  }

  void
  reset() {
    size_ = 0;
  }

 protected:
  size_t size_;

};

} }
}
}

#endif // SERIALIZE_SIZER_H
