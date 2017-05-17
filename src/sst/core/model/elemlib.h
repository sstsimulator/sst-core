#include <string>
#include <vector>
#include <map>

namespace SST {

class LibraryInfo;
class ComponentElementInfo;
class SubComponentElementInfo;
class ModuleElementInfo;

namespace Core {
namespace PyModel {


class ElementDef {
protected:
    virtual PyTypeObject* getTypeObject() = 0;

public:
    ElementDef() { }
    virtual ~ElementDef() {}
    bool addToModule(PyObject *module);
    virtual const std::string& getTypeName() const = 0;
};




class LibraryDef {
    std::string modName;
    std::vector<ElementDef*> items;
    static std::map<std::string, ElementDef*> typeMap;
    static std::map<std::string, LibraryDef> foundLibraries;

public:
    LibraryDef(const std::string &modName);
    static LibraryDef& findLibraryDefinition(const std::string &modName);
    static ElementDef* findTypeDefinition(const std::string &fullTypeName);
    bool loadModule(PyObject *module);
};

}
}
}
