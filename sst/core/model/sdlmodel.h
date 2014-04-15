// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//
// History:
// Unkown - Created [?????2009]
// Michael Scoggin(org1422) - Comment and Error Handling [Jan09,2012]
//              modified:       parse_params() parse_variables() parse_variable()
//              created:        parse_parameter() get_node_text()
//              added:          verbosity


#ifndef SST_CORE_SDLMODEL_H
#define SST_CORE_SDLMODEL_H

#include <sst/core/params.h>
#include <sst/core/model/sstmodel.h>

#include <map>
#include <string>


class TiXmlDocument;
class TiXmlNode;

namespace SST {


/** Map a name to a Value */
typedef std::map<std::string,std::string> VariableMap_t;

class ConfigComponent;
class ConfigGraph;

/** XML Parser/Model generator */
class SSTSDLModelDefinition : public SSTModelDescription {

public:
    /** Create a new ModelDefinition object using the XML found in filename */
    SSTSDLModelDefinition(std::string filename);
    ~SSTSDLModelDefinition();

    /** Returns a string suitable for parsing by SST::Config */
    std::string getSDLConfigString();

    ConfigGraph* createConfigGraph();

    /** Returns the SDL version */
    std::string getVersion() {return version;}

    /** 0=quiet, 1=coarse, 2=parameters, 3=comments */
    int verbosity;
private:

    ConfigGraph* graph;

    TiXmlDocument*   doc;
    std::string	 version;

    std::map<std::string,Params*> includes;
    VariableMap_t    variables;

    // Parsing methods
    void parse_parameter(TiXmlNode* pParent, Params* params);//Scoggin(Jan09,2013)
    void parse_param_include(TiXmlNode* pParent);
    void parse_variable(TiXmlNode* pParent);
    void parse_variables(TiXmlNode* pParent);
    void parse_component(TiXmlNode* pParent);
    void parse_introspector(TiXmlNode* pParent);
    void parse_params(TiXmlNode* pParent, ComponentId_t comp);
    void parse_link(TiXmlNode* pParent, ComponentId_t comp);
    std::string resolve_variable(const std::string value, int line_number);
    std::string resolveEnvVars(const char* input);
    std::string resolveEnvVars(std::string input);
    const char* get_node_text(TiXmlNode* pParent);//Scoggin(Jan09,2013)
};


} // namespace SST

#endif //SST_CORE_SDL_H
