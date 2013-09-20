// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2013, Sandia Corporation
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

        typedef std::map<std::string,Params*> ParamsMap_t;
        typedef std::map<std::string,std::string> VariableMap_t;

        class ConfigComponent;
        class ConfigGraph;

        class SSTSDLModelDefinition : public SSTModelDescription {

                public:
                       	SSTSDLModelDefinition(std::string filename);
                        ~SSTSDLModelDefinition();

                        std::string getSDLConfigString();
                        ConfigGraph* createConfigGraph();
                        std::string getVersion() {return version;}


                        int verbosity;//Scoggin(Jan09,2013) 0=quiet, 1=coarse, 2=parameters, 3=comments
                private:

                        ConfigGraph* graph;

                        TiXmlDocument*   doc;
                        std::string	 version;

                        std::map<std::string,Params*> includes;
                        /* ParamsMap_t      includes; */
                        VariableMap_t    variables;

                        // Parsing methods
                        void parse_parameter(TiXmlNode* pParent, Params* params);//Scoggin(Jan09,2013)
                        void parse_param_include(TiXmlNode* pParent);
                        void parse_variable(TiXmlNode* pParent);
                        void parse_variables(TiXmlNode* pParent);
                        void parse_component(TiXmlNode* pParent);
                       	void parse_introspector(TiXmlNode* pParent);
                       	void parse_params(TiXmlNode* pParent, ConfigComponent* comp);
                       	void parse_link(TiXmlNode* pParent, ConfigComponent* comp);
                       	std::string resolve_variable(const std::string value, int line_number);
                       	std::string resolveEnvVars(const char* input);
                       	std::string resolveEnvVars(std::string input);
                       	const char* get_node_text(TiXmlNode* pParent);//Scoggin(Jan09,2013)
       	};


} // namespace SST

#endif //SST_CORE_SDL_H
