#ifndef SST_CORE_OLD_ELI_H
#define SST_CORE_OLD_ELI_H

namespace SST {

class OldELITag {
 public:
  OldELITag(const std::string& elemlib) :
   lib(elemlib)
  {
  }

  std::string lib;
};

}

#endif

