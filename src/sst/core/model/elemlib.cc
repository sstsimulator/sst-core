#include "sst_config.h"
#include <Python.h>
#include <sst/core/model/elemlib.h>
#include <sst/core/model/pymodel_comp.h>
#include <sst/core/elementinfo.h>

/* Things to load an element library
 * Components
 * SubComponents
 * Modules
 */

using namespace SST;
using namespace SST::Core;
using namespace SST::Core::PyModel;



class ElementDefImpl : public ElementDef {
protected:
    std::string fullName;
    std::string typeName;
    PyTypeObject typeObject;

    PyTypeObject* getTypeObject() override { return &typeObject; }
    const std::string& getTypeName() const override { return typeName; }

public:
    ElementDefImpl(const std::string &modName, const std::string &name) : ElementDef(),
        fullName(modName + "." + name), typeName(name)
    {
        struct {
            PyObject_HEAD
        } dummy = { PyObject_HEAD_INIT(NULL) };
        memset(&typeObject, '\0', sizeof(typeObject));
        memcpy(&typeObject, &dummy, sizeof(dummy));
        typeObject.tp_name = fullName.c_str();
        typeObject.tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE;
    }
    virtual ~ElementDefImpl() { }
};

static int moduleCompInit(PyObject *self, PyObject *args, PyObject *UNUSED(kwds));
class ComponentDef;

struct ComponentDefType {
    struct ComponentPy_t base;
    ComponentDef *typePtr;
};

class ComponentDef : public ElementDefImpl {
private:
    std::string doc;
    ComponentElementInfo *ei;

    static PyMethodDef methodDefs[];
public:
    ComponentDef(const std::string &modName, const std::string &name, ComponentElementInfo* ei) :
        ElementDefImpl(modName, name),
        doc(ei->getDescription()),
        ei(ei)
    {
        typeObject.tp_basicsize = sizeof(struct ComponentPy_t) + sizeof(ComponentDef*);
        typeObject.tp_base = &PyModel_ComponentType;
        typeObject.tp_doc = doc.c_str();
        typeObject.tp_methods = methodDefs;
        typeObject.tp_init = moduleCompInit;
    }
    ~ComponentDef() { }
};
PyMethodDef ComponentDef::methodDefs[] = {
    { NULL, NULL, 0, NULL }
};

static int moduleCompInit(PyObject *self, PyObject *args, PyObject *UNUSED(kwds))
{
    PyTypeObject* type = Py_TYPE(self);

    char *name = NULL;
    if ( !PyArg_ParseTuple(args, "s", &name) ) {
        return -1;
    }
    PyObject *newArgs = Py_BuildValue("ss", name, type->tp_name + 4);

    int res = type->tp_base->tp_init((PyObject *)self, newArgs, kwds);
    Py_XDECREF(newArgs);
    if ( res >= 0 ) {
        ((ComponentDefType*)self)->typePtr = dynamic_cast<ComponentDef*>(LibraryDef::findTypeDefinition(type->tp_name));
    }
    return res;
}









class SubComponentDef : public ElementDefImpl {
private:
    std::string doc;
    static PyMethodDef methodDefs[];
public:
    SubComponentDef(const std::string &modName, const std::string &name, SubComponentElementInfo* ei) :
        ElementDefImpl(modName, name),
        doc(ei->getDescription())
    {
        typeObject.tp_basicsize = sizeof(struct ComponentPy_t) + 4; /* SubTypes should be larger than their super Types */
        typeObject.tp_base = &PyModel_SubComponentType;
        typeObject.tp_doc = doc.c_str();
        typeObject.tp_methods = methodDefs;
        typeObject.tp_init = PyModel_SubComponentType.tp_init;
    }
    ~SubComponentDef() { }
};

PyMethodDef SubComponentDef::methodDefs[] = {
    { NULL, NULL, 0, NULL }
};





bool ElementDef::addToModule(PyObject *module)
{
    PyTypeObject *typeObj = getTypeObject();
    if ( !typeObj )
        return false;
    if ( PyType_Ready(typeObj) < 0 ) {
        /* TODO:  Print error properly */
        return false;
    }

    Py_INCREF(typeObj);
    return (0 == PyModule_AddObject(module, getTypeName().c_str(), (PyObject*)typeObj));
}



LibraryDef::LibraryDef(const std::string &modName) :
    modName(modName)
{
    LibraryInfo *libInfo = ElementLibraryDatabase::getLibraryInfo(modName.substr(4));
    /* For now, only support new ELI style */
    if ( libInfo == NULL )
        return;

    for ( auto &i : libInfo->components ) {
        items.push_back(new ComponentDef(modName, i.first, i.second));
    }
    for ( auto &i : libInfo->subcomponents ) {
        items.push_back(new SubComponentDef(modName, i.first, i.second));
    }
}


std::map<std::string, LibraryDef> LibraryDef::foundLibraries;
std::map<std::string, ElementDef*> LibraryDef::typeMap;

LibraryDef& LibraryDef::findLibraryDefinition(const std::string &modName)
{
    return foundLibraries.emplace(modName, modName).first->second;
}

ElementDef* LibraryDef::findTypeDefinition(const std::string &modName)
{
    auto i = typeMap.find(modName);
    return ( i == typeMap.end() ) ? NULL : i->second;
}


bool LibraryDef::loadModule(PyObject *module)
{
    bool ok = true;
    for ( ElementDef* ed : items ) {
        ok &= ed->addToModule(module);
    }
    return ok;
}


