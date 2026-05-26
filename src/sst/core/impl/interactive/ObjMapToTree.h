// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_CORE_SERIALIZATION_OBJECTMAPTOTREE_DEBUGGER_H
#define SST_CORE_SERIALIZATION_OBJECTMAPTOTREE_DEBUGGER_H

#include "sst/core/serialization/ObjTree.h"
#include "sst/core/serialization/objectMapTreeBuilder.h"

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace SST::Core::Serialization {

class ObjectMapToTree
{
    using IntVariant   = IntegerObj::IntVariant;
    using FloatVariant = FloatObj::FloatVariant;

    // Known integer type strings
    static bool isIntegerType(const std::string& type)
    {
        static const std::unordered_set<std::string> intTypes = { "signed char", "int8_t", "char", "short", "int16_t",
            "int", "int32_t", "long", "long long", "int64_t", "unsigned char", "uint8_t", "unsigned short", "uint16_t",
            "unsigned int", "unsigned", "uint32_t", "unsigned long", "unsigned long long", "uint64_t" };
        return intTypes.count(type) > 0;
    }

    static bool isFloatType(const std::string& type)
    {
        return type == "float" || type == "double" || type == "long double";
    }

    static bool isStringType(const std::string& type)
    {
        // Demangled std::string can appear in various forms
        return type.find("std::string") != std::string::npos ||
               type.find("std::__cxx11::basic_string") != std::string::npos ||
               type.find("basic_string") != std::string::npos;
    }

    static bool isContainerType(const std::string& type)
    {
        return type.find("std::vector") != std::string::npos || type.find("std::map") != std::string::npos ||
               type.find("std::unordered_map") != std::string::npos || type.find("std::set") != std::string::npos ||
               type.find("std::unordered_set") != std::string::npos || type.find("std::list") != std::string::npos ||
               type.find("std::deque") != std::string::npos || type.find("std::multimap") != std::string::npos ||
               type.find("std::array") != std::string::npos;
    }

    static std::unique_ptr<IntegerObj> makeIntegerObj(const std::string& type, void* addr)
    {
        if ( !addr ) return nullptr;
        if ( type == "signed char" || type == "int8_t" || type == "char" )
            return std::make_unique<IntegerObj>(*static_cast<int8_t*>(addr), addr);
        if ( type == "short" || type == "int16_t" )
            return std::make_unique<IntegerObj>(*static_cast<int16_t*>(addr), addr);
        if ( type == "int" || type == "int32_t" )
            return std::make_unique<IntegerObj>(*static_cast<int32_t*>(addr), addr);
        if ( type == "long" || type == "long long" || type == "int64_t" )
            return std::make_unique<IntegerObj>(*static_cast<int64_t*>(addr), addr);
        if ( type == "unsigned char" || type == "uint8_t" )
            return std::make_unique<IntegerObj>(*static_cast<uint8_t*>(addr), addr);
        if ( type == "unsigned short" || type == "uint16_t" )
            return std::make_unique<IntegerObj>(*static_cast<uint16_t*>(addr), addr);
        if ( type == "unsigned int" || type == "unsigned" || type == "uint32_t" )
            return std::make_unique<IntegerObj>(*static_cast<uint32_t*>(addr), addr);
        if ( type == "unsigned long" || type == "unsigned long long" || type == "uint64_t" )
            return std::make_unique<IntegerObj>(*static_cast<uint64_t*>(addr), addr);
        return nullptr;
    }

    static std::unique_ptr<FloatObj> makeFloatObj(const std::string& type, void* addr)
    {
        if ( !addr ) return nullptr;
        if ( type == "float" ) return std::make_unique<FloatObj>(*static_cast<float*>(addr), addr);
        if ( type == "double" ) return std::make_unique<FloatObj>(*static_cast<double*>(addr), addr);
        if ( type == "long double" ) return std::make_unique<FloatObj>(*static_cast<long double*>(addr), addr);
        return nullptr;
    }

public:
    // Convert a single ObjectMap* into an ObjTreeCont*
    // Caller takes ownership of the returned pointer
    static ObjTreeCont* convert(const std::string& name, ObjectMap* objMap, bool recursive = true)
    {
        if ( !objMap ) return nullptr;

        const std::string type = objMap->getType();
        void*             addr = objMap->getAddr();

        if ( objMap->isFundamental() ) {
            if ( type == "bool" && addr ) {
                // handle bitset and vector<bool> case
                auto* ref = objMap->buildTreeNode(name);
                if ( ref ) {
                    return ref;
                }
                // standard case
                auto* o = new BoolObj(*static_cast<bool*>(addr), addr);
                o->setName(name);
                o->setType(type);
                return o;
            }
            if ( isIntegerType(type) ) {
                // handle fundamental ref case
                auto* ref = objMap->buildTreeNode(name);
                if ( ref ) {
                    return ref;
                }
                // standard case
                if ( auto o = makeIntegerObj(type, addr) ) {
                    o->setName(name);
                    o->setType(type);
                    if ( objMap->isReadOnly() ) {
                        o->makeReadOnly();
                    }
                    return o.release();
                }
            }
            if ( isFloatType(type) ) {
                // handle fundamental ref case
                auto* ref = objMap->buildTreeNode(name);
                if ( ref ) {
                    return ref;
                }
                // standard case
                if ( auto o = makeFloatObj(type, addr) ) {
                    o->setName(name);
                    o->setType(type);
                    if ( objMap->isReadOnly() ) {
                        o->makeReadOnly();
                    }
                    return o.release();
                }
            }
            if ( isStringType(type) && addr ) {
                auto* o = new StringObj(*static_cast<std::string*>(addr), addr);
                o->setName(name);
                o->setType(type);
                if ( objMap->isReadOnly() ) {
                    o->makeReadOnly();
                }
                return o;
            }
            // Unknown fundamental — wrap in GenericObj with the value as string
            // Note: we pass a nullptr in for objMap as the objMap used here is destroyed by the caller
            //      If it is necessary to set one of these generic values we will need to preserve the
            //      value of objMap. For now, we just pass in a nullptr rather than carry the (potentially)
            //      heavy ObjMap around in this container
            auto* g = new GenericValObj(objMap->get(), addr, nullptr);
            g->setName(name);
            g->setType(type);
            if ( objMap->isReadOnly() ) {
                g->makeReadOnly();
            }
            return g;
        }

        if ( objMap->isContainer() || isContainerType(type) ) {
            const auto& vars = objMap->getVariables();
            auto*       c    = new ContainerObj(name, type, vars.size(), nullptr);
            for ( const auto& [n, m] : vars )
                if ( auto* child = convert(n, m, recursive) ) c->addChildObj(child);
            return c;
        }

        if ( objMap->getCategory() == ObjectMap::ObjectCategory::Component ) {
            if ( auto* comp = static_cast<BaseComponent*>(addr) ) {
                auto* co = new ComponentObj(comp, nullptr);
                co->setName(name);
                co->setType(type);
                if ( objMap->isReadOnly() ) {
                    co->makeReadOnly();
                }
                return co;
            }
        }

        auto* node = new ObjTreeCont(name, type);
        if ( recursive ) {
            for ( const auto& [n, m] : objMap->getVariables() )
                if ( auto* child = convert(n, m, false) ) node->addChildObj(child);
        }
        return node;
    }

    static void addChildrenFromMap(ObjTreeCont* parent, const ObjectMultimap& variables)
    {
        if ( !parent ) return;
        for ( const auto& [name, objMap] : variables ) {
            ObjTreeCont* child = convert(name, objMap, false);
            if ( child ) parent->addChildObj(child);
        }
    }

    static void addChildrenFromMapRecursive(ObjTreeCont* parent, const ObjectMultimap& variables)
    {
        if ( !parent ) return;
        for ( const auto& [name, objMap] : variables ) {
            ObjTreeCont* child = convert(name, objMap, true);
            if ( child ) parent->addChildObj(child);
        }
    }

    // Convert an entire ObjectMap's variables into an ObjTreeCont tree
    static std::unique_ptr<ObjTreeCont> convertTree(const std::string& rootName, ObjectMap* objMap)
    {
        auto        root      = std::make_unique<ObjTreeCont>(rootName, objMap->getType());
        const auto& variables = objMap->getVariables();
        for ( const auto& [name, childMap] : variables ) {
            ObjTreeCont* child = convert(name, childMap, true);
            if ( child ) {
                root->addChildObj(child);
            }
        }
        return root;
    }

    /**
     * Takes a ComponentObj, serializes its BaseComponent via
     * ObjectMapDeferred, and populates the ComponentObj's children
     * with the serialized variables.
     *
     * @param compNode  The ComponentObj node to expand
     * @param recursive If true, recursively convert all children
     * @return true if serialization succeeded
     */
    static bool serializeComponent(ComponentObj* compNode, bool recursive = false)
    {
        if ( !compNode ) return false;

        BaseComponent* comp = compNode->getVal();
        if ( !comp ) return false;

        ComponentInfo* compInfo = compNode->getInfo();


        // Create a temporary deferred map and trigger serialization
        ComponentSerializer serializer(comp);
        serializer.serialize();

        if ( serializer.hasSerialized() ) {
            const auto& variables = serializer.getVariables();

            // Collect sub-component addresses so we skip them during conversion
            std::vector<void*> subCompAddrs;
            if ( compInfo ) {
                collectSubComponentAddrs(compInfo, subCompAddrs);
            }

            for ( const auto& [name, objMap] : variables ) {
                if ( !objMap ) continue;

                // Skip variables that are sub-components
                if ( std::find(subCompAddrs.begin(), subCompAddrs.end(), objMap->getAddr()) != subCompAddrs.end() )
                    continue;

                // ObjTreeCont* child = recursive
                //     ? convert(name, objMap)
                //     : convertNode(name, objMap);
                ObjTreeCont* child = convert(name, objMap, recursive);
                if ( child ) compNode->addChildObj(child);
            }
        }

        // Serialize sub-components from ComponentInfo
        if ( compInfo ) {
            serializeSubComponents(compNode, compInfo, recursive);
        }

        return true;
    }

    /**
     * Serialize all ComponentObj children of a given node.
     *
     * @param parent    The parent node whose ComponentObj children to expand
     * @param recursive If true, recursively convert all grandchildren
     */
    static void serializeAllComponents(ObjTreeCont* parent, bool recursive = false)
    {
        if ( !parent ) return;

        parent->applyRecursiveByType<ComponentObj>(
            [recursive](ComponentObj* comp) { serializeComponent(comp, recursive); });
    }

    /**
     * Full pipeline: convert an ObjectMap tree, then serialize
     * any ComponentObj nodes found in the result.
     *
     * @param rootName  Name for the root node
     * @param objMap    Source ObjectMap to convert
     * @param recursive If true, fully expand all levels
     * @return Root of the converted and serialized tree
     */
    static std::unique_ptr<ObjTreeCont> convertAndSerialize(
        const std::string& rootName, ObjectMap* objMap, bool recursive = true)
    {
        auto root = convertTree(rootName, objMap);
        if ( !root ) return nullptr;

        // Serialize all component nodes in the tree
        serializeAllComponents(root.get(), recursive);

        return root;
    }

private:
    static void collectSubComponentAddrs(ComponentInfo* compInfo, std::vector<void*>& addrs)
    {
        auto& subComps = compInfo->getSubComponents();

        for ( auto it = subComps.begin(); it != subComps.end(); ++it ) {

            BaseComponent* sub = it->second.getComponent();
            if ( sub ) {
                addrs.push_back(static_cast<void*>(sub));
            }
        }
    }

    static void serializeSubComponents(ObjTreeCont* parent, ComponentInfo* compInfo, bool recursive)
    {
        auto& subComps = compInfo->getSubComponents();
        for ( auto& [compId, subInfo] : subComps ) {
            BaseComponent* sub = subInfo.getComponent();
            if ( !sub ) continue;

            auto* subObj = new ComponentObj(sub, &subInfo);
            subObj->setName(subInfo.getName());
            subObj->setType(subInfo.getType());

            // Serialize this sub-component's own variables
            ComponentSerializer subSerializer(sub);
            subSerializer.serialize();

            if ( subSerializer.hasSerialized() ) {
                const auto& subVars = subSerializer.getVariables();

                // Collect nested sub-component addresses
                std::vector<void*> nestedAddrs;
                collectSubComponentAddrs(&subInfo, nestedAddrs);

                for ( const auto& [name, objMap] : subVars ) {
                    if ( !objMap ) continue;
                    if ( std::find(nestedAddrs.begin(), nestedAddrs.end(), objMap->getAddr()) != nestedAddrs.end() )
                        continue;

                    // ObjTreeCont* child = recursive
                    //     ? convert(name, objMap)
                    //     : convertNode(name, objMap);
                    ObjTreeCont* child = convert(name, objMap, recursive);
                    if ( child ) subObj->addChildObj(child);
                }
            }

            // Recurse into this sub-component's own sub-components
            if ( recursive ) {
                serializeSubComponents(subObj, &subInfo, recursive);
            }

            parent->addChildObj(subObj);
        }
    }
};

} // namespace SST::Core::Serialization

#endif
