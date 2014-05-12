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

#include <sst_config.h>
#include "sst/core/serialization.h"
#include <sst/core/model/sdlmodel.h>

#include <iostream>
#include "sst/core/configGraph.h"
#include "sst/core/simulation.h"
#include "sst/core/timeLord.h"

#include <tinyxml/tinyxml.h>

using namespace std;

namespace SST {
	SSTSDLModelDefinition::SSTSDLModelDefinition( const string fileName ) :
		SSTModelDescription()
	{
		verbosity=0;		//Scoggin(Jan09,2013) Added for feedback
		doc = new TiXmlDocument(fileName.c_str());
		bool load_ok = doc->LoadFile();
		if ( !load_ok ) {
			cerr << "Error loading " << fileName <<": " << doc->ErrorDesc() << " on line " << doc->ErrorRow() << endl;
			exit(1);
		}

		// Get the SDL version number
		version = "NONE";
		TiXmlNode* root = doc;
		TiXmlNode* child;

		for ( child = root->FirstChild(); child != 0; child = child->NextSibling() ) {
			if ( child->Type() == TiXmlNode::TINYXML_ELEMENT ) {
				if ( !strcmp( child->Value(), "sdl" ) ) {
					version = child->ToElement()->Attribute("version");
				}
			}
		}

		if ( version == "NONE" ) {
			cerr << "ERROR: No SDL version number specified in file " << fileName << "." << endl;
			cerr << "  Please add a version number to SDL file: <sdl version=VERSION>." << endl;
			exit(1);
		}
		else if ( version == "2.0" ) {} // valid version
		else {
			cerr << "ERROR: Unsupported SDL version: " << version << " in file " << fileName << "." << endl;
			exit(1);
		}    
	}

	SSTSDLModelDefinition::~SSTSDLModelDefinition()
	{
		delete doc;
	}

	string SSTSDLModelDefinition::getSDLConfigString() {

		TiXmlNode* root = doc;
		TiXmlNode* child;

		string config;

		for ( child = root->FirstChild(); child != 0; child = child->NextSibling() ) {
			if ( child->Type() == child->TINYXML_ELEMENT ) {
				if ( !strcmp( child->Value(), "config" ) ) {
                    if ( child->FirstChild() ) {
                        config = resolveEnvVars(child->FirstChild()->Value());
                        break;
                    }
				}
			}
		}

		string::size_type pos = 0;
		while ( ( pos = config.find( ' ', pos ) ) != string::npos ) {
			config.replace( pos, 1, 1, '\n' );
		}
		pos = 0;
		while ( ( pos = config.find( '\t', pos ) ) != string::npos ) {
			config.replace( pos, 1, 1, '\n' );
		}
		return config;
	}


	ConfigGraph* SSTSDLModelDefinition::createConfigGraph(){ 
		graph = new ConfigGraph();

		TiXmlNode* pParent = doc;
		TiXmlNode* pChild;
		TiXmlNode* sst_section = 0; //Scoggin(Jan23,2013) Fix initialization warning on build.

		// First, we need to look for includes and variables and find the sst section
		for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
		{
			switch (  pChild->Type()) {
				case TiXmlNode::TINYXML_ELEMENT:
					if ( strcmp( pChild->Value(), "param_include") == 0 ) {
						parse_param_include( pChild ); 
					}
					else if ( strcmp( pChild->Value(), "variable") == 0 ) {
						//	parse_variable( pChild ); //Scoggin(Jan09,2013) hijacked for error and comment detection
						parse_variables( pChild ); 
					}
					else if ( strcmp( pChild->Value(), "variables") == 0 ) {
						parse_variables( pChild ); 
					}
					else if ( strcmp( pChild->Value(), "sst") == 0 ) {
						sst_section = pChild;
					}
					break;
				default:
					break;
			}
		}

		// Now, process the sst section, which contains a list of components in the sim
		for ( pChild = sst_section->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) {
			if(verbosity>=1)						//Scoggin(Jan09,2013) Added for feedback
				std::cout<<"Parsing Components and Introspectors"<<std::endl;
			switch (  pChild->Type()) {
				case TiXmlNode::TINYXML_ELEMENT:
					if ( strcmp( pChild->Value(), "component") == 0 ) {
						parse_component( pChild ); 
					}
					else if ( strcmp( pChild->Value(), "introspector") == 0 ) {
						parse_introspector( pChild ); 
					}
					break;
				default:
					break;
			}
		}

		return graph;
	}


	/* parse_paramter
	 * TiXmlNode* pParent	The node to parse, should be an element
	 * Params* params		The params container to add to
	 * This does error checking and comment skipping to parse what we expect to be parameter
	 *
	 * Scoggin(Jan09,2013)
	 */
	void SSTSDLModelDefinition::parse_parameter(TiXmlNode* pParent, Params* params){
		switch(pParent->Type()){
			case TiXmlNode::TINYXML_COMMENT:
				if(verbosity>=3)						//Scoggin(Jan09,2013) Added for feedback
					std::cout<<"COMMENT:"<<pParent->Value()<<std::endl;
				break;
			case TiXmlNode::TINYXML_ELEMENT:
				if(get_node_text(pParent)){
                    params->insert(std::make_pair(pParent->Value(), resolveEnvVars(get_node_text(pParent))));
				}
				break;
			case TiXmlNode::TINYXML_TEXT:
				std::cerr<<"ERROR:(SDL line:"<<pParent->Row()<<") unexpeted text: "<<pParent->Value()<<std::endl;
				exit(1);
				break;
			case TiXmlNode::TINYXML_DOCUMENT:
				std::cerr<<"ERROR:(SDL line:"<<pParent->Row()<<") unexpeted document: "<<pParent->Value()<<std::endl;
				exit(1);
				break;
			case TiXmlNode::TINYXML_DECLARATION:
				std::cerr<<"ERROR:(SDL line:"<<pParent->Row()<<") unexpected declaration: "<<pParent->Value()<<std::endl;
				exit(1);
				break;
			case TiXmlNode::TINYXML_UNKNOWN:
				std::cerr<<"ERROR:(SDL line:"<<pParent->Row()<<") unknown node: "<<pParent->Value()<<std::endl;
				exit(1);
				break;
			default:
				std::cerr<<"ERROR:(SDL line:"<<pParent->Row()<<") default case"<<std::endl;
				exit(1);
				break;
		}
	}

	void SSTSDLModelDefinition::parse_param_include( TiXmlNode* pParent ){
		TiXmlNode* pChild;
		if(verbosity>=1)						//Scoggin(Jan09,2013) Added for feedback
			std::cout<<std::endl<<"Parsing Includes"<<std::endl;
		for ( pChild = pParent->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) 
		{
			// Each node at this level is a separate set of parameters
			std::string include_name = pChild->Value();
			Params* params = new Params();

			if(verbosity>=2)						//Scoggin(Jan09,2013) Added for feedback
				std::cout<<" "<<include_name<<std::endl;

			for ( TiXmlNode* param = pChild->FirstChild(); param != NULL; param = param->NextSibling()) {
				parse_parameter(param,params); //Scoggin(Jan09,2013) modified for error and comment handling
				if(verbosity>=2)						//Scoggin(Jan09,2013) Added for feedback
					if((*params).find(pChild->Value() ) != (*params).end() )
						std::cout<<"  "<<pChild->Value()<<"="<<(*params)[pChild->Value()]<<std::endl;
			}
			includes[include_name] = params;
		}        
	}

	/* parse_variable
	 * TiXmlNode* pParent	The node to parse, should be an element
	 * This does error checking and comment skipping to parse what we expect to be a variable
	 *
	 * Scoggin(Jan09,2012) Retrofited for error and comment handling
	 */
	void SSTSDLModelDefinition::parse_variable( TiXmlNode* pParent ){
		//string var_name = pParent->ToElement()->FirstAttribute()->Name();
		//variables[var_name] = pParent->ToElement()->FirstAttribute()->Value();    

		switch(pParent->Type()){
			case TiXmlNode::TINYXML_COMMENT:						//Scoggin(Jan09,2013) Added for feedback
				if(verbosity>=3)
					std::cout<<"COMMENT:"<<pParent->Value()<<std::endl;
				break;
			case TiXmlNode::TINYXML_ELEMENT:
				if(get_node_text(pParent)){
					variables[pParent->Value()]=resolveEnvVars(get_node_text(pParent));
				}
				break;
			case TiXmlNode::TINYXML_TEXT:
				std::cerr<<"ERROR:(SDL line:"<<pParent->Row()<<") unexpeted text: "<<pParent->Value()<<std::endl;
				exit(1);
				break;
			case TiXmlNode::TINYXML_DOCUMENT:
				std::cerr<<"ERROR:(SDL line:"<<pParent->Row()<<") unexpeted document: "<<pParent->Value()<<std::endl;
				exit(1);
				break;
			case TiXmlNode::TINYXML_DECLARATION:
				std::cerr<<"ERROR:(SDL line:"<<pParent->Row()<<") unexpected declaration: "<<pParent->Value()<<std::endl;
				exit(1);
				break;
			case TiXmlNode::TINYXML_UNKNOWN:
				std::cerr<<"ERROR:(SDL line:"<<pParent->Row()<<") unknown node: "<<pParent->Value()<<std::endl;
				exit(1);
				break;
			default:
				std::cerr<<"ERROR:(SDL line:"<<pParent->Row()<<") default case"<<std::endl;
				break;
		}
	}

	void SSTSDLModelDefinition::parse_variables( TiXmlNode* pParent ){
		TiXmlNode* pChild;
		if(verbosity>=1)						//Scoggin(Jan09,2013) Added for feedback
			std::cout<<std::endl<<"Parsing Variables"<<std::endl;
		for ( pChild = pParent->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) {
			parse_variable(pChild); //Scoggin(Jan09,2013) modified for error and comment handling
			if(verbosity>=2)						//Scoggin(Jan09,2013) Added for feedback
				if(variables.find(pChild->Value() ) != variables.end() )
					std::cout<<"  "<<pChild->Value()<<"="<<variables[pChild->Value()]<<std::endl;
			// Each node at this level is a separate set of parameters
			//std::string variable_name = pChild->Value();
			//			if(pChild->ToElement()->GetText())	
			//				variables[variable_name] = pChild->ToElement()->GetText();
		}    
	}

	void SSTSDLModelDefinition::parse_component(TiXmlNode* pParent){
        std::string name;
        std::string type;
        float weight = 0.0f;
        int rank = 0;

		// Get the attributes from the component
		TiXmlElement* element = pParent->ToElement();
		if ( element->Attribute("name") == NULL ) {
			cerr << "ERROR: Parsing SDL file: Unspecified component name on or near line " << pParent->Row() << endl;
			exit(1);
		}
		else {
			name = resolveEnvVars(element->Attribute("name"));
		}
		if ( element->Attribute("type") == NULL ) {
			cerr << "ERROR: Parsing SDL file: Unspecified component type on or near line " << pParent->Row() << endl;
			exit(1);
		}
		else {
			type = resolveEnvVars(element->Attribute("type"));
		}
		if(verbosity>=1)						//Scoggin(Jan09,2013) Added for feedback
			cout<<" "<<name<<endl;

		int status;
		// Get the rank
		status = element->QueryIntAttribute("rank",&rank);
		if ( status == TIXML_WRONG_TYPE ) {
			cerr << "ERROR: Parsing SDL file: Bad rank specified (" << element->Attribute("rank") <<
				") on or near line " << pParent->Row();
			exit(1);
		}
		else if ( status == TIXML_NO_ATTRIBUTE ) {
			rank = -1;
		}

		// Get the weight
		status = element->QueryFloatAttribute("weight",&weight);
		if ( status == TIXML_WRONG_TYPE ) {
			cerr << "ERROR: Parsing SDL file: Bad weight specified (" << element->Attribute("weight") <<
				") on or near line " << pParent->Row();
			exit(1);
		}
		else if ( status == TIXML_NO_ATTRIBUTE ) {
			weight = 0;
		}



        ComponentId_t comp = graph->addComponent(name, type, weight, rank);

		// Now get the rest of the data (params and links)
		TiXmlNode* pChild;
		for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) {
			switch (  pChild->Type()) {
				case TiXmlNode::TINYXML_ELEMENT:
					if ( strcmp( pChild->Value(), "params") == 0 ) {
						parse_params( pChild, comp );
					}
					else if ( strcmp( pChild->Value(), "link") == 0 ) {
						parse_link( pChild, comp ); 
					}
					break;
				default:
					break;
			}
		}

	}

	void SSTSDLModelDefinition::parse_introspector(TiXmlNode* pParent){
        std::string name;
        std::string type;

		// Get the attributes from the component
		TiXmlElement* element = pParent->ToElement();
		if ( element->Attribute("name") == NULL ) {
			cerr << "ERROR: Parsing SDL file: Unspecified introspector name on or near line " << pParent->Row() << endl;
			exit(1);
		}
		else {
			name = resolveEnvVars(element->Attribute("name"));
		}
		if ( element->Attribute("type") == NULL ) {
			cerr << "ERROR: Parsing SDL file: Unspecified introspector type on or near line " << pParent->Row() << endl;
			exit(1);
		}
		else {
			type = resolveEnvVars(element->Attribute("type"));
		}
		if(verbosity>=2)						//Scoggin(Jan09,2013) Added for feedback
			std::cout<<" "<<name<<std::endl;
		// Now get the rest of the data (params and links)

        ComponentId_t comp = graph->addIntrospector(name, type);


		TiXmlNode* pChild;
		for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) {
			switch (  pChild->Type()) {
				case TiXmlNode::TINYXML_ELEMENT:
					if ( strcmp( pChild->Value(), "params") == 0 ) {
						parse_params( pChild, comp );
					}
					break;
				default:
					break;
			}	
		}

	}

	void SSTSDLModelDefinition::parse_params(TiXmlNode* pParent, ComponentId_t comp) {
		TiXmlElement* element = pParent->ToElement();
		if(verbosity>=2)						//Scoggin(Jan09,2013) Added for feedback
			std::cout<<"  Parameters"<<std::endl;
		// First, put all the params specified here in the Params object
		// of the component.  The includes will be merged after, and the
		// net result will be parameters specified in the component will
		// take priority over those specified in includes
        Params params;
		for ( TiXmlNode* param = pParent->FirstChild(); param != NULL; param = param->NextSibling()) {
			parse_parameter(param,&params);		//Scoggin(Jan09,2013) Modified for error and comment handling
			if(verbosity>=2)						//Scoggin(Jan09,2013) Added for feedback
				if(params.find(param->Value() ) != params.end() )
					std::cout<<"   "<<param->Value()<<" = '"<<params[param->Value()]<<"'"<<std::endl;
		}

		// Now, see if there are any includes
		if ( element->Attribute("include") != NULL ) {
			// Include could be a comma separated list
			std::string includes_str = resolveEnvVars(element->Attribute("include"));
			size_t start = 0;
			size_t end = 0;
			do {
				end = includes_str.find(',',start);
				size_t length = end - start;
				std::string sub = includes_str.substr(start,length);
				// See if the include exists
				if ( includes.find(sub) == includes.end() ) {
					cerr << "ERROR: Parsing sdl file: Unknown include (" << sub.c_str() << ") on or near line "
						<< pParent->Row() << endl;
					exit(1);
				}
				else {
					// Merge the two params
					Params* inc_params = includes[sub];
					params.insert(inc_params->begin(),inc_params->end());
				}
				start = end + 1;
			} while (end != std::string::npos);
		}
        graph->addParams(comp, params);
	}

	void SSTSDLModelDefinition::parse_link(TiXmlNode* pParent, ComponentId_t comp){
		TiXmlElement* element = pParent->ToElement();

		std::string name;
		if ( element->Attribute("name") == NULL ) {
			cerr << "ERROR: Parsing SDL file: Unspecified link name on or near line " << pParent->Row() << endl;
			exit(1);
		}
		else {
			name = resolveEnvVars(element->Attribute("name"));
		}
		if(verbosity>=2)						//Scoggin(Jan09,2013) Added for feedback
			std::cout<<"  "<<name<<std::endl;

		std::string port;
		if ( element->Attribute("port") == NULL ) {
			cerr << "ERROR: Parsing SDL file: Unspecified link port on or near line " << pParent->Row() << endl;
			exit(1);
		}
		else {
			port = resolveEnvVars(element->Attribute("port"));
		}

        std::string lat_str;
		if ( element->Attribute("latency") == NULL ) {
			cerr << "ERROR: Parsing SDL file: Unspecified link latency on or near line " << pParent->Row() << endl;
			exit(1);
		}
		else {
			lat_str = resolveEnvVars(element->Attribute("latency"));
			lat_str = resolve_variable(lat_str,pParent->Row());
		}

        graph->addLink(comp, name, port, lat_str);

	}

	string SSTSDLModelDefinition::resolve_variable(const string value, int line_number){
		// Check to see if this is a variable
		if ( value.find("$") != 0 ) return value;
		std::string var_name = value.substr(1);
		if ( variables.find(var_name) == variables.end() ) {
			printf("ERROR: Parsing SDL file: Unknown variable specified (%s) on or around line %d\n",value.c_str(), line_number);
			exit(1);
		}
		return variables[var_name];
	}


	std::string SSTSDLModelDefinition::resolveEnvVars(const char *input) {
		std::string res = input;
		return resolveEnvVars(res);
	}

	std::string SSTSDLModelDefinition::resolveEnvVars(std::string input) {
		std::string res = input;
		size_t envStart = 0;
		while ( (envStart = res.find("${", envStart)) != std::string::npos ) {
			size_t envEnd = res.find("}", envStart);
			if ( envEnd == std::string::npos ) {
				// No end tag.  Must be malformed
				break;
			}
			size_t len = envEnd - envStart -2;
			std::string envname = res.substr(envStart+2, len);
			const char *envval = getenv(envname.c_str());
			if ( envval )
				res.replace(envStart, len+3, envval);
			else
				envStart = envEnd;
		}
		return res;
	}

	/* get_node_text
	 * TiXmlNode* pParent	The node we want to get the text from
	 * This fetches the text value of the element, it skips non text entries and return null if no text exist
	 *
	 * Scoggin(Jan09,2013)
	 */
	const char* SSTSDLModelDefinition::get_node_text(TiXmlNode* pParent){
		const char* nodeText=0;
		for(TiXmlNode* child=pParent->FirstChild();child;child=child->NextSibling()){
			//		std::cout<<".";
			switch(child->Type()){
				case TiXmlNode::TINYXML_COMMENT:
					if(verbosity>=3)						//Scoggin(Jan09,2013) Added for feedback
						std::cout<<"COMMENT:"<<child->Value()<<std::endl;
					break;
				case TiXmlNode::TINYXML_ELEMENT:
					std::cerr<<"ERROR: Parsing SDL file, unexpected element: "<<child->Value()<<std::endl;
					exit(1);
					break;
				case TiXmlNode::TINYXML_TEXT:
					nodeText=child->Value();
					child=pParent->LastChild();
					break;
				case TiXmlNode::TINYXML_DOCUMENT:
					std::cerr<<"ERROR: Parsing SDL file, unexpected document: "<<child->Value()<<std::endl;
					exit(1);
					break;
				case TiXmlNode::TINYXML_DECLARATION:
					std::cerr<<"ERROR: Parsing SDL file, unexpected declaration: "<<child->Value()<<std::endl;
					exit(1);
					break;
				case TiXmlNode::TINYXML_UNKNOWN:
					std::cerr<<"ERROR: Parsing SDL file, unknown node: "<<child->Value()<<std::endl;
					exit(1);
					break;
				default:
					std::cerr<<"ERROR: Parsing SDL file, default case"<<std::endl;
					exit(1);
					break;
			}
		}
		return nodeText;
	}

} // namespace SST
