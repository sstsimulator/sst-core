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

#include <sst/core/sdl.h>
#include <sst/core/configGraph.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <tinyxml/tinyxml.h>

#include <sst/core/debug.h>

namespace SST {

#define _SDL_DBG( fmt, args...) __DBG( DBG_SDL, SDL, fmt, ## args )

static void parameters( Params &params, TiXmlNode* node )
{
    TiXmlNode* pChild;
    for ( pChild = node->FirstChild(); pChild != 0; 
                                        pChild = pChild->NextSibling()) 
    {
            if ( strcmp( pChild->Value(), "params" ) == 0 ) {
                return parameters( params, pChild );
            }
            if ( pChild->ToElement()->GetText() == NULL ) {
                _abort(SDL,"element \"%s\" has no text\n",pChild->Value());
            } 
            _SDL_DBG("name=%s value=%s\n", pChild->Value(),
                                    pChild->ToElement()->GetText() ); 

	    params[pChild->Value()] = pChild->ToElement()->GetText();
    }
}

static void parameters_link( ConfigLink* link, int which_comp, TiXmlNode* node )
{
//     TiXmlNode* pChild;
//     for ( pChild = node->FirstChild(); pChild != 0; 
//                                         pChild = pChild->NextSibling()) 
//     {
//             if ( pChild->ToElement()->GetText() == NULL ) {
//                 _abort(SDL,"element \"%s\" has no text\n",pChild->Value());
//             } 
//             _SDL_DBG("name=%s value=%s\n", pChild->Value(),
//                                     pChild->ToElement()->GetText() ); 
// 	    if ( strcmp( pChild->Value(), "name" ) == 0 ) {
// 		link->port[which_comp] = pChild->ToElement()->GetText();
// 	    }
// 	    if ( strcmp( pChild->Value(), "lat" ) == 0 ) {
// 		link->latency[which_comp] = pChild->ToElement()->GetText();
// 	    }
//     }
}

static void link( Params &params, TiXmlNode* node )
{
    TiXmlNode* pChild;
    _SDL_DBG("%s\n",node->Value());

    for ( pChild = node->FirstChild(); pChild != 0; 
                                        pChild = pChild->NextSibling()) 
    {
        if ( strcmp( pChild->Value(), "params" ) == 0 ) {
            parameters( params, pChild);
        }
    }
}

static void new_link( ConfigLink* link, int which_comp, TiXmlNode* node )
{
    TiXmlNode* pChild;
    _SDL_DBG("%s\n",node->Value());

    for ( pChild = node->FirstChild(); pChild != 0; 
                                        pChild = pChild->NextSibling()) 
    {
        if ( strcmp( pChild->Value(), "params" ) == 0 ) {
            parameters_link( link, which_comp, pChild);
        }
    }
}

static std::string link_id( TiXmlNode* node )
{
    _SDL_DBG("name=%s value=%s\n",
            node->ToElement()->FirstAttribute()->Name(),
            node->ToElement()->FirstAttribute()->Value());

    return node->ToElement()->FirstAttribute()->Value();
}

static void links( SDL_links_t &links, TiXmlNode* node )
{
    TiXmlNode* pChild;

    _SDL_DBG("%s\n",node->Value());

    for ( pChild = node->FirstChild(); pChild != 0; 
                                        pChild = pChild->NextSibling()) 
    {
        if ( strcmp( pChild->Value(), "link" ) == 0 ) {
            SDL_Link *l = links[link_id(pChild)] = new SDL_Link;
            link( l->params, pChild);
        }
    }
}

static void new_links( ConfigGraph &graph, ConfigComponent* comp, TiXmlNode* node )
{
    TiXmlNode* pChild;

    for ( pChild = node->FirstChild(); pChild != 0; 
                                        pChild = pChild->NextSibling()) 
    {
        if ( strcmp( pChild->Value(), "link" ) == 0 ) {
	    std::string name = link_id(pChild);
	    // See if the link already exists
	    ConfigLink* link;
	    int which_comp;
	    if ( graph.links.count(name) ) {
		link = graph.links[name];
		which_comp = 1;
	    }
	    else {
		link = new ConfigLink();
		graph.links[name] = link;
		which_comp = 0;
	    }
	    // Now, set all the parameters for the link
	    link->component[which_comp] = comp;
	    new_link(link, which_comp, pChild);
	    comp->links.push_back(link);

        }
    }
}


static SDL_Component * component( TiXmlNode* node, float weight, int rank, bool isIntrospector )
{
    SDL_Component *c = new SDL_Component( node->Value() );
    TiXmlNode* pChild;

    c->rank = rank;
    c->weight = weight;
    c->_isIntrospector = isIntrospector;

    _SDL_DBG("type=%s\n",node->Value());

    for ( pChild = node->FirstChild(); pChild != 0; 
                                        pChild = pChild->NextSibling()) 
    {
        if ( strcmp( pChild->Value(), "params" ) == 0 ) {
            parameters( c->params, pChild );
        }
        if ( strcmp( pChild->Value(), "links" ) == 0 ) {
            links( c->links, pChild );
        }
    }
    return c;
}


static void parse_component( TiXmlNode* pParent, SDL_CompMap_t &compMap )
{
    TiXmlElement* element = pParent->ToElement();
    TiXmlNode* pChild;
    float weight = 1.0;
    int rank = -1;
    bool isIntrospector = false;

    if ( element->Attribute("rank") ) {
        rank = atoi( element->Attribute("rank") );
    }
    
    
    if ( element->Attribute("weight") ) {
        weight =atof( element->Attribute("weight") );
    }

    _SDL_DBG("id=%s weight=%f\n", element->Attribute("id"), weight );

    for ( pChild = pParent->FirstChild(); pChild != 0; 
                                        pChild = pChild->NextSibling()) 
    {
        _SDL_DBG("child %s\n",pChild->Value());
        compMap[element->Attribute("id")] = component( pChild, weight, rank, isIntrospector );
        return;
    }
}


static void parse_introspector( TiXmlNode* pParent, SDL_CompMap_t &compMap )
{
    TiXmlElement* element = pParent->ToElement();
    TiXmlNode* pChild;
    float weight = 1.0;
    bool isIntrospector = true;

    if ( element->Attribute("weight") ) {
        weight =atof( element->Attribute("weight") );
    }

    for ( pChild = pParent->FirstChild(); pChild != 0; 
                                        pChild = pChild->NextSibling()) 
    {
        compMap[element->Attribute("id")] = component( pChild, weight, -1, isIntrospector );
        return;
    }
}


static std::string parse_config( TiXmlNode* pParent )
{
    std::string tmp = "";
    if ( pParent->FirstChild() ) {
        _SDL_DBG("%s\n", pParent->FirstChild()->ValueStr().c_str());
        tmp = pParent->FirstChild()->ValueStr() + " "; 
    }
    return tmp;
}

static void parse( TiXmlNode* pParent, SDL_CompMap_t &compMap )
{
    if ( !pParent ) return;

    TiXmlNode* pChild;

    for ( pChild = pParent->FirstChild(); pChild != 0;
                                pChild = pChild->NextSibling()) 
    {
	switch (  pParent->Type())
        {
	case TiXmlNode::ELEMENT:
	    _SDL_DBG( "Element [%s]\n", pParent->Value() );
	    if ( strcmp( pParent->Value(), "component") == 0 ) {
		parse_component( pParent, compMap ); 
	    }
	    else if ( strcmp( pParent->Value(), "introspector") == 0 ) {
		parse_introspector( pParent, compMap ); 
	    }
	    else {
		parse( pChild, compMap );
	    }
	    break;
	    
	case TiXmlNode::DECLARATION:
	    {
// 	    TiXmlDeclaration* dec = static_cast<TiXmlDeclaration*>(pParent);
// 	    printf("Version: %s\n",dec->Version());
	    }
	    break;
	default:
	    parse( pChild, compMap );
	    break;
	}
    }
}




typedef std::map<std::string, TiXmlNode* > referenceMap_t;


static TiXmlElement* reference( TiXmlElement* element, referenceMap_t& map )
{
    //_SDL_DBG("tag=%s\n",node->Value()); 

    TiXmlAttribute* pAttrib = element->FirstAttribute();
    while (pAttrib)
    {
        if ( ! strcmp( pAttrib->Name(), "reference" ) )
        {
            referenceMap_t::iterator iter = map.find( pAttrib->Value() );
            if ( iter == map.end() ) {
                _abort(SDL,"undefine reference %s\n",pAttrib->Value() );
            }
            //_SDL_DBG("%s => %s\n", node->Value(), pAttrib->Value()); 

            TiXmlElement* tmp = new TiXmlElement("");
            
            static_cast< TiXmlElement* >( iter->second )->CopyTo( tmp );

            tmp->SetValue( element->Value() );

            return tmp; 
        }
        pAttrib=pAttrib->Next();
    }
    return NULL;
}


static TiXmlNode* create_references( TiXmlNode* node, referenceMap_t& map )
{
    if ( ! node ) return NULL;

    switch (  node->Type() )
    {
        case TiXmlNode::ELEMENT:
            {
                TiXmlElement* ref; 
                if ( ( ref = 
                        reference( static_cast<TiXmlElement*>(node), map ) ) )
                {
                    return ref;
                }
            }
            break;

        default:
            break;
    }

    for ( TiXmlNode* child = node->FirstChild(); child != 0;
                                child = child->NextSibling()) 
    {
        TiXmlNode* ref = create_references( child, map );

        if ( ref ) {
            node->ReplaceChild( child, *ref );
        }
    }
    return NULL;
}

static bool include( TiXmlElement* element, referenceMap_t& map )
{
    TiXmlAttribute* pAttrib = element->FirstAttribute();
    while (pAttrib)
    {
        TiXmlNode* start = NULL;
        if ( ! strncmp( pAttrib->Name(), "include", 7  ) )
        {
            referenceMap_t::iterator iter = map.find( pAttrib->Value() );
            if ( iter == map.end() ) {
                _abort(SDL,"undefine reference %s\n",pAttrib->Value() );
            }
            _SDL_DBG("tag=%s %s=%s\n",element->Value(), 
                    pAttrib->Name(), pAttrib->Value() );
            
            for ( TiXmlNode* child = iter->second->FirstChild(); child != 0;
                                                child = child->NextSibling() ) 
            {
                if ( start == NULL ) {
                    start = element->FirstChild();
                    if ( start == 0  ) {
                        start = element->InsertEndChild( *child );
                    } else {
                        start = element->InsertBeforeChild( start ,*child );
                    }
                } else {
                    element->InsertAfterChild( start ,*child );
                }
            }
        }
        pAttrib=pAttrib->Next();
    }
    return false;
}

static void create_include( TiXmlNode* node, referenceMap_t& map )
{
    if ( ! node ) return;

    switch (  node->Type() )
    {
        case TiXmlNode::ELEMENT:
            if ( include( static_cast<TiXmlElement*>(node), map ) ) {
                return;
            }
            break;

        default:
            break;
    }

    for ( TiXmlNode* child = node->FirstChild(); child != 0;
                                child = child->NextSibling()) 
    {
        create_include( child, map );
    }
}


static void init_reference_map( TiXmlNode* pParent, referenceMap_t& map )
{
    if ( !pParent ) return;

    TiXmlNode* pChild;

    switch (  pParent->Type())
    {
    case TiXmlNode::ELEMENT:
        _SDL_DBG( "%p Element [%s]\n", pParent,pParent->Value() );
        map[ pParent->Value()] = pParent;
        break;

    default:
        break;
    }
    for ( pChild = pParent->FirstChild(); pChild != 0;
                                pChild = pChild->NextSibling()) 
    {
        init_reference_map( pChild, map );
    }
}

static void init_references( TiXmlNode* pParent )
{
    std::map<std::string, TiXmlNode* > map;
    init_reference_map( pParent, map );
    create_references( pParent, map );
    create_include( pParent, map );
}

static std::string get_config( TiXmlNode* pParent )
{
    if ( !pParent ) return "";

    TiXmlNode* pChild;

    switch (  pParent->Type())
    {
    case TiXmlNode::ELEMENT:
        if ( strcmp( pParent->Value(), "config") == 0 ) {
            return  parse_config( pParent ); 
        }
        break;

    default:
        break;
    }
    std::string config;
    for ( pChild = pParent->FirstChild(); pChild != 0;
                                pChild = pChild->NextSibling()) 
    {
        config += get_config( pChild );
    }
    return config;
}

#if 0
static void parameters_print( std::string prefix, Params &p, std::string postfix )
{
    if ( p.size() == 0) return;
    std::cout << prefix;
    for ( Params::iterator iter = p.begin(); iter != p.end(); ++iter ) {
        std::cout << (*iter).first << "=" << (*iter).second << " ";
    }
    std::cout << postfix;
}

static void links_print( std::string prefix, SDL_links_t &l, std::string postfix )
{
    if ( l.size() == 0) return;
    std::cout << prefix << "links:" <<std::endl;
    for ( SDL_links_t::iterator iter = l.begin(); iter != l.end(); ++iter ) {
        std::cout << prefix << "  " << "id=" << (*iter).first;
            parameters_print( ", parameters: ", (*iter).second->params, "\n" );
    }
    std::cout << postfix;
}

static void component_print( std::string id, SDL_Component *c )
{
    std::cout << "componenet id=" << id << " type=" << c->type() << std::endl;
    parameters_print( "  parameters: ", c->params, "\n" );
    links_print("    ", c->links, "\n"  );
}

static void print_map( SDL_CompMap_t &map )
{
    printf("\n");
    for( SDL_CompMap_t::iterator iter = map.begin(); iter != map.end(); ++iter ) {
        component_print(  (*iter).first, (*iter).second );
    }
}
#endif


int xml_parse( std::string fileName,  SDL_CompMap_t &map )
{
    TiXmlDocument doc(fileName.c_str());
    bool loadOkay = doc.LoadFile();
    if (loadOkay)
    {
        init_references( &doc );
	printf("Initialized references\n");
        _SDL_DBG("file=\"%s\"\n", fileName.c_str());
        parse( &doc, map );
    }
    else
    {
        _ABORT(,"Failed to load file \"%s\"\n", fileName.c_str());
    }

#if 0
    print_map( map );
#endif

    return 0;
}

std::string xmlGetVersion( std::string fileName )
{
    TiXmlDocument doc(fileName.c_str());
    bool loadOkay = doc.LoadFile();
    if (loadOkay)
    {
	TiXmlDeclaration* dec = static_cast<TiXmlDeclaration*>(doc.FirstChild());
	printf("Version: %s\n",dec->Version());
	return dec->Version();
    }
    return "UNKNOWN";
}
std::string xmlGetConfig( std::string fileName )
{
    _SDL_DBG("\n%s:\n", fileName.c_str());
    TiXmlDocument doc(fileName.c_str());
    bool loadOkay = doc.LoadFile();
    if (loadOkay)
    {
        std::string tmp = get_config( &doc );

	
        std::string::size_type pos = 0;
        while ( ( pos = tmp.find( ' ', pos ) ) != std::string::npos ) {
            tmp.replace( pos, 1, 1, '\n' );
        }
        pos = 0;
        while ( ( pos = tmp.find( '\t', pos ) ) != std::string::npos ) {
            tmp.replace( pos, 1, 1, '\n' );
        }

        _SDL_DBG("\"%s\"\n",tmp.c_str()); 
        return tmp;
    }
    else
    {
        throw doc.ErrorDesc();
    }
    return 0;
}


// Version 2.0 parsing

inline static std::string resolve_variable(VariableMap_t& var_map, const std::string& value, int line_number)
{
    // Check to see if this is a variable
    if ( value.find("$") != 0 ) return value;
    std::string var_name = value.substr(1);
    if ( var_map.find(var_name) == var_map.end() ) {
	printf("ERROR: Parsing SDL file: Unknown variable specified (%s) on or around line %d\n",value.c_str(), line_number);
	exit(1);
    }
    return var_map[var_name];
}

static void new_parse_param_include( TiXmlNode* pParent, ConfigGraph &graph )
{
    TiXmlNode* pChild;

    for ( pChild = pParent->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) 
    {
	// Each node at this level is a separate set of parameters
	std::string include_name = pChild->Value();
	Params* params = new Params();

	for ( TiXmlNode* param = pChild->FirstChild(); param != NULL; param = param->NextSibling()) {
// 	    printf("Parameter: %s,  Value: %s\n",param->Value(),param->ToElement()->GetText());
	    (*params)[param->Value()] = param->ToElement()->GetText();
	}
	graph.includes[include_name] = params;
    }    
}

static void new_parse_variables( TiXmlNode* pParent, ConfigGraph &graph )
{
    TiXmlNode* pChild;

    for ( pChild = pParent->FirstChild(); pChild != NULL; pChild = pChild->NextSibling()) 
    {
	// Each node at this level is a separate set of parameters
	std::string variable_name = pChild->Value();
	graph.variables[variable_name] = pChild->ToElement()->GetText();
    }    
}

static void new_parse_variable( TiXmlNode* pParent, ConfigGraph &graph )
{
    std::string var_name = pParent->ToElement()->FirstAttribute()->Name();
    std::string var_value = pParent->ToElement()->FirstAttribute()->Value();

    graph.variables[var_name] = var_value;
//     printf("Variable: %s,  Value: %s\n",var_name.c_str(),var_value.c_str());
}

static void new_parse_params( TiXmlNode* pParent, ConfigGraph &graph, ConfigComponent* comp ) {
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
	std::string includes = element->Attribute("include");
	size_t start = 0;
	size_t end = 0;
	do {
	    end = includes.find(',',start);
	    size_t length = end - start;
	    std::string sub = includes.substr(start,length);
	    // See if the include exists
	    if ( graph.includes.find(sub) == graph.includes.end() ) {
		printf("ERROR: Parsing sdl file: Unknown include (%s) on or near line %d\n",sub.c_str(),pParent->Row());
		exit(1);
	    }
	    else {
		// Merge the two params
	        Params* inc_params = graph.includes[sub];
 		comp->params.insert(inc_params->begin(),inc_params->end());
	    }
	    start = end + 1;
	} while (end != std::string::npos);
//  	comp->params.print_all_params(std::cout);
    }
}

static void new_parse_link( TiXmlNode* pParent, ConfigGraph &graph, ConfigComponent* comp ) {

    TiXmlElement* element = pParent->ToElement();

    std::string name;
    if ( element->Attribute("name") == NULL ) {
	printf("ERROR: Parsing SDL file: Unspecified link name on or near line %d\n",pParent->Row());
	exit(1);
    }
    else {
	name = element->Attribute("name");
    }

    // Need to see if someone else has referenced this link;
    ConfigLink* link;
    if ( graph.links.find(name) == graph.links.end() ) {
	link = new ConfigLink();
	link->name = name;
	graph.links[name] = link;
    }
    else {
	link = graph.links[name];
	if ( link->current_ref >= 2 ) {
	    printf("ERROR: Parsing SDL file: Link %s referenced more than two times\n",name.c_str());
	    exit(1);
	}
    }

    std::string port;
    if ( element->Attribute("port") == NULL ) {
	printf("ERROR: Parsing SDL file: Unspecified link port on or near line %d\n",pParent->Row());
	exit(1);
    }
    else {
	port = element->Attribute("port");
    }
    
    SimTime_t latency;
    if ( element->Attribute("latency") == NULL ) {
	printf("ERROR: Parsing SDL file: Unspecified link latency on or near line %d\n",pParent->Row());
	exit(1);
    }
    else {
	std::string lat_str = element->Attribute("latency");
	lat_str = resolve_variable(graph.variables,lat_str,pParent->Row());
	latency = Simulation::getSimulation()->getTimeLord()->getSimCycles(lat_str, "Parsing sdl");
    }
    
    int index = link->current_ref++;
    link->component[index] = comp;
    link->port[index] = port;
    link->latency[index] = latency;

    comp->links.push_back(link);
}


static void new_parse_component( TiXmlNode* pParent, ConfigGraph& graph ) {
    ConfigComponent* comp = new ConfigComponent();
    comp->isIntrospector = false;

    TiXmlElement* element = pParent->ToElement();
    if ( element->Attribute("name") == NULL ) {
	printf("ERROR: Parsing SDL file: Unspecified component name on or near line %d\n",pParent->Row());
	exit(1);
    }
    else {
	comp->name = element->Attribute("name");
    }
    if ( element->Attribute("type") == NULL ) {
	printf("ERROR: Parsing SDL file: Unspecified component type on or near line %d\n",pParent->Row());
	exit(1);
    }
    else {
	comp->type = element->Attribute("type");
    }

    int status;
    // Get the rank
    status = element->QueryIntAttribute("rank",&comp->rank);
    if ( status == TIXML_WRONG_TYPE ) {
	printf("ERROR: Parsing SDL file: Bad rank specified (%s) on or near line %d\n",
	       element->Attribute("rank"),pParent->Row());
	exit(1);
    }
    else if ( status == TIXML_NO_ATTRIBUTE ) {
	comp->rank = -1;
    }

    // Get the weight
    status = element->QueryFloatAttribute("weight",&comp->weight);
    if ( status == TIXML_WRONG_TYPE ) {
	printf("ERROR: Parsing SDL file: Bad weight specified (%s) on or near line %d\n",
	       element->Attribute("weight"),pParent->Row());
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
		new_parse_params( pChild, graph, comp );
	    }
	    else if ( strcmp( pChild->Value(), "link") == 0 ) {
 		new_parse_link( pChild, graph, comp ); 
	    }
	    break;
	default:
	    break;
	}	
    }

    graph.comps[comp->id] = comp;
    
}

static void parse( TiXmlNode* pParent, ConfigGraph &graph )
{
    if ( !pParent ) return;

    TiXmlNode* pChild;
    TiXmlNode* sst_section;

    // First, we need to look for includes and variables and find the sst section
    for ( pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling()) 
    {
	switch (  pChild->Type()) {
	case TiXmlNode::ELEMENT:
	    if ( strcmp( pChild->Value(), "param_include") == 0 ) {
		new_parse_param_include( pChild, graph ); 
	    }
	    else if ( strcmp( pChild->Value(), "variable") == 0 ) {
 		new_parse_variable( pChild, graph ); 
	    }
	    else if ( strcmp( pChild->Value(), "variables") == 0 ) {
 		new_parse_variables( pChild, graph ); 
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
		new_parse_component( pChild, graph ); 
	    }
// 	    else if ( strcmp( pChild->Value(), "introspector") == 0 ) {
//  		new_parse_introspector( pChild, graph ); 
// 	    }
// 	    else if ( strcmp( pChild->Value(), "sst") == 0 ) {
// 		sst_section = pChild;
// 	    }
	    break;
	default:
	    break;
	}
    }
}


int xml_parse( std::string fileName, ConfigGraph& graph )
{
    TiXmlDocument doc(fileName.c_str());
    bool loadOkay = doc.LoadFile();
    if (loadOkay)
    {
        parse( &doc, graph );
    }
    else
    {
        _ABORT(,"Failed to load file \"%s\"\n", fileName.c_str());
    }

    return 0;
}

} // namespace SST
