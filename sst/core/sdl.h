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

// Legacy support
    
class SDL_Link {
    public:
    float        weight;
    Params params;
};

typedef std::map<std::string,SDL_Link *> SDL_links_t;

class SDL_Component {
    std::string _type;
public:
    SDL_Component( std::string type) : _type(type), _isIntrospector(0) {};
    std::string &type( void ) { return _type; };
    bool isIntrospector( void ) { return _isIntrospector; };
    float        weight;
    int          rank;
    Params params;
    SDL_links_t  links;
    bool _isIntrospector;
};

typedef std::map < std::string, SDL_Component * > SDL_CompMap_t;
 class ConfigGraph;
 
extern int xml_parse( std::string file, SDL_CompMap_t& map);
extern int xml_parse( std::string fileName, ConfigGraph& graph );
extern int xml_parse( TiXmlDocument* doc, ConfigGraph& graph );
extern std::string xmlGetConfig( std::string file );
extern std::string xmlGetVersion( std::string file );

// End legacy support

 
typedef std::map<std::string,Params*> ParamsMap_t;
typedef std::map<std::string,std::string> VariableMap_t;

class ConfigComponent;
 
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
