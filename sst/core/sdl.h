// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.



#ifndef _SST_SDL_H
#define _SST_SDL_H

#include <map>
#include <string>

#include "sst/core/params.h"


class TiXmlDocument;
class TiXmlNode;

namespace SST {

typedef std::map<std::string,Params*> ParamsMap_t;
typedef std::map<std::string,std::string> VariableMap_t;

class ConfigComponent;
class ConfigGraph;
 
class sdl_parser {

public:
    sdl_parser(std::string filename);
    ~sdl_parser();

    std::string getSDLConfigString();
    ConfigGraph* createConfigGraph();
    std::string getVersion() {return version;}
    
private:

    ConfigGraph* graph;
    
    TiXmlDocument*   doc;
    std::string      version;

    std::map<std::string,Params*> includes;
    /* ParamsMap_t      includes; */
    VariableMap_t    variables;

    // Parsing methods
    void parse_param_include(TiXmlNode* pParent);
    void parse_variable(TiXmlNode* pParent);
    void parse_variables(TiXmlNode* pParent);
    void parse_component(TiXmlNode* pParent);
    void parse_introspector(TiXmlNode* pParent);
    void parse_params(TiXmlNode* pParent, ConfigComponent* comp);
    void parse_link(TiXmlNode* pParent, ConfigComponent* comp);
    std::string resolve_variable(const std::string value, int line_number);
};

 
} // namespace SST

#endif
