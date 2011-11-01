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


#include <sst_config.h>
#include "sst/core/serialization/core.h"

#include <iostream>

#include "sst/core/configGraph.h"
#include "sst/core/sdl.h"
#include "sst/core/simulation.h"
#include "sst/core/timeLord.h"

#include <tinyxml/tinyxml.h>

using namespace std;

namespace SST {

sdl_parser::sdl_parser( const string fileName )
{
    doc = new TiXmlDocument(fileName.c_str());
    bool load_ok = doc->LoadFile();
    if ( !load_ok ) {
	cout << "Error loading " << fileName <<": " << doc->ErrorDesc() << endl;
	exit(1);
    }

    // Get the SDL version number
    version = "NONE";
    TiXmlNode* root = doc;
    TiXmlNode* child;
    
    for ( child = root->FirstChild(); child != 0; child = child->NextSibling() ) {
	if ( child->Type() == TiXmlNode::ELEMENT ) {
	    if ( !strcmp( child->Value(), "sdl" ) ) {
		version = child->ToElement()->Attribute("version");
	    }
	}
    }

    if ( version == "NONE" ) {
	cout << "ERROR: No SDL version number specified in file " << fileName << "." << endl;
	cout << "  Please add a version number to SDL file: <sdl version=VERSION>." << endl;
	exit(1);
    }
    else if ( version == "2.0" ) {} // valid version
    else {
	cout << "ERROR: Unsupported SDL version: " << version << " in file " << fileName << "." << endl;
	exit(1);
    }    
}

sdl_parser::~sdl_parser()
{
    delete doc;
}

string
sdl_parser::getSDLConfigString() {

    TiXmlNode* root = doc;
    TiXmlNode* child;

    string config;
    
    for ( child = root->FirstChild(); child != 0; child = child->NextSibling() ) {
	if ( child->Type() == TiXmlNode::ELEMENT ) {
	    if ( !strcmp( child->Value(), "config" ) ) {
		config = child->FirstChild()->Value();
		break;
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
    

ConfigGraph*
sdl_parser::createConfigGraph()
{
    graph = new ConfigGraph();
    
    TiXmlNode* pParent = doc;
    TiXmlNode* pChild;
    TiXmlNode* sst_section;

    // First, we need to look for includes and variables and find the sst section
    for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
    {
	switch (  pChild->Type()) {
	case TiXmlNode::ELEMENT:
	    if ( strcmp( pChild->Value(), "param_include") == 0 ) {
		parse_param_include( pChild ); 
	    }
	    else if ( strcmp( pChild->Value(), "variable") == 0 ) {
 		parse_variable( pChild ); 
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
    for ( pChild = sst_section->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
    {
	switch (  pChild->Type()) {
	case TiXmlNode::ELEMENT:
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
    
void
sdl_parser::parse_param_include( TiXmlNode* pParent )
{
    TiXmlNode* pChild;

    for ( pChild = pParent->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) 
    {
	// Each node at this level is a separate set of parameters
	std::string include_name = pChild->Value();
	Params* params = new Params();

	for ( TiXmlNode* param = pChild->FirstChild(); param != NULL; param = param->NextSibling()) {
	    (*params)[param->Value()] = param->ToElement()->GetText();
	}
	includes[include_name] = params;
    }        
}

void
sdl_parser::parse_variable( TiXmlNode* pParent )
{
    string var_name = pParent->ToElement()->FirstAttribute()->Name();
    variables[var_name] = pParent->ToElement()->FirstAttribute()->Value();    
}
    
void
sdl_parser::parse_variables( TiXmlNode* pParent )
{
    TiXmlNode* pChild;

    for ( pChild = pParent->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) 
    {
	// Each node at this level is a separate set of parameters
	std::string variable_name = pChild->Value();
	variables[variable_name] = pChild->ToElement()->GetText();
    }    
}

void
sdl_parser::parse_component(TiXmlNode* pParent)
{
    ConfigComponent* comp = new ConfigComponent();
    comp->isIntrospector = false;

    // Get the attributes from the component
    TiXmlElement* element = pParent->ToElement();
    if ( element->Attribute("name") == NULL ) {
	cout << "ERROR: Parsing SDL file: Unspecified component name on or near line " << pParent->Row() << endl;
	exit(1);
    }
    else {
	comp->name = element->Attribute("name");
    }
    if ( element->Attribute("type") == NULL ) {
	cout << "ERROR: Parsing SDL file: Unspecified component type on or near line " << pParent->Row() << endl;
	exit(1);
    }
    else {
	comp->type = element->Attribute("type");
    }

    int status;
    // Get the rank
    status = element->QueryIntAttribute("rank",&comp->rank);
    if ( status == TIXML_WRONG_TYPE ) {
	cout << "ERROR: Parsing SDL file: Bad rank specified (" << element->Attribute("rank") <<
	    ") on or near line " << pParent->Row();
	exit(1);
    }
    else if ( status == TIXML_NO_ATTRIBUTE ) {
	comp->rank = -1;
    }

    // Get the weight
    status = element->QueryFloatAttribute("weight",&comp->weight);
    if ( status == TIXML_WRONG_TYPE ) {
	cout << "ERROR: Parsing SDL file: Bad weight specified (" << element->Attribute("weight") <<
	    ") on or near line " << pParent->Row();
	exit(1);
    }
    else if ( status == TIXML_NO_ATTRIBUTE ) {
	comp->weight = 0;
    }

    // Now get the rest of the data (params and links)
    TiXmlNode* pChild;
    for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) {
	switch (  pChild->Type()) {
	case TiXmlNode::ELEMENT:
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

    graph->comps[comp->id] = comp;    
}

void
sdl_parser::parse_introspector(TiXmlNode* pParent)
{
    ConfigComponent* comp = new ConfigComponent();
    comp->isIntrospector = true;

    // Get the attributes from the component
    TiXmlElement* element = pParent->ToElement();
    if ( element->Attribute("name") == NULL ) {
	cout << "ERROR: Parsing SDL file: Unspecified introspector name on or near line " << pParent->Row() << endl;
	exit(1);
    }
    else {
	comp->name = element->Attribute("name");
    }
    if ( element->Attribute("type") == NULL ) {
	cout << "ERROR: Parsing SDL file: Unspecified introspector type on or near line " << pParent->Row() << endl;
	exit(1);
    }
    else {
	comp->type = element->Attribute("type");
    }

    // Now get the rest of the data (params and links)
    TiXmlNode* pChild;
    for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) {
	switch (  pChild->Type()) {
	case TiXmlNode::ELEMENT:
	    if ( strcmp( pChild->Value(), "params") == 0 ) {
		parse_params( pChild, comp );
	    }
	    break;
	default:
	    break;
	}	
    }

    graph->comps[comp->id] = comp;    
}

void
sdl_parser::parse_params(TiXmlNode* pParent, ConfigComponent* comp) {
    TiXmlElement* element = pParent->ToElement();

    // First, put all the params specified here in the Params object
    // of the component.  The includes will be merged after, and the
    // net result will be parameters specified in the component will
    // take priority over those specified in includes
    for ( TiXmlNode* param = pParent->FirstChild(); param != NULL; param = param->NextSibling()) {
	comp->params[param->Value()] = param->ToElement()->GetText();
    }

    // Now, see if there are any includes
    if ( element->Attribute("include") != NULL ) {
	// Include could be a comma separated list
	std::string includes_str = element->Attribute("include");
	size_t start = 0;
	size_t end = 0;
	do {
	    end = includes_str.find(',',start);
	    size_t length = end - start;
	    std::string sub = includes_str.substr(start,length);
	    // See if the include exists
	    if ( includes.find(sub) == includes.end() ) {
		cout << "ERROR: Parsing sdl file: Unknown include (" << sub.c_str() << ") on or near line "
		     << pParent->Row() << endl;
		exit(1);
	    }
	    else {
		// Merge the two params
	        Params* inc_params = includes[sub];
 		comp->params.insert(inc_params->begin(),inc_params->end());
	    }
	    start = end + 1;
	} while (end != std::string::npos);
    } 
}

void
sdl_parser::parse_link(TiXmlNode* pParent, ConfigComponent* comp)
{
    TiXmlElement* element = pParent->ToElement();

    std::string name;
    if ( element->Attribute("name") == NULL ) {
	cout << "ERROR: Parsing SDL file: Unspecified link name on or near line " << pParent->Row() << endl;
	exit(1);
    }
    else {
	name = element->Attribute("name");
    }

    // Need to see if someone else has referenced this link;
    ConfigLink* link;
    if ( graph->links.find(name) == graph->links.end() ) {
	link = new ConfigLink();
	link->name = name;
	graph->links[name] = link;
    }
    else {
	link = graph->links[name];
	if ( link->current_ref >= 2 ) {
	    cout << "ERROR: Parsing SDL file: Link " << name << " referenced more than two times" << endl;
	    exit(1);
	}
    }

    std::string port;
    if ( element->Attribute("port") == NULL ) {
	cout << "ERROR: Parsing SDL file: Unspecified link port on or near line " << pParent->Row() << endl;
	exit(1);
    }
    else {
	port = element->Attribute("port");
    }
    
    SimTime_t latency;
    if ( element->Attribute("latency") == NULL ) {
	cout << "ERROR: Parsing SDL file: Unspecified link latency on or near line " << pParent->Row() << endl;
	exit(1);
    }
    else {
	std::string lat_str = element->Attribute("latency");
	lat_str = resolve_variable(lat_str,pParent->Row());
	latency = Simulation::getSimulation()->getTimeLord()->getSimCycles(lat_str, "Parsing sdl");
    }
    
    int index = link->current_ref++;
    link->component[index] = comp->id;
    link->port[index] = port;
    link->latency[index] = latency;

    comp->links.push_back(link);
    
}

string
sdl_parser::resolve_variable(const string value, int line_number)
{
    // Check to see if this is a variable
    if ( value.find("$") != 0 ) return value;
    std::string var_name = value.substr(1);
    if ( variables.find(var_name) == variables.end() ) {
	printf("ERROR: Parsing SDL file: Unknown variable specified (%s) on or around line %d\n",value.c_str(), line_number);
	exit(1);
    }
    return variables[var_name];
}

    
} // namespace SST
