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


#ifndef _SST_GRAPH_H_
#define _SST_GRAPH_H_

#include <map>
#include <list>


namespace SST {

const char GRAPH_COMP_NAME[] = "comp_name";
const char GRAPH_LINK_NAME[] = "link_name";
const char GRAPH_WEIGHT[]    = "weight";
const char GRAPH_RANK[]      = "rank";
const char GRAPH_ID[]        = "id";

class PropList {
  public:
	void set( std::string name, std::string value ) {
		map[name] = value; 
	}
	std::string &get( std::string name ) {
		return map[name];
	}
  private:
	std::map<std::string,std::string> map;
};

class Vertex {
public:
    Vertex() : _id(++count) {};
	PropList prop_list;
	int id() { return _id; }  
	std::list<int> adj_list;

private:
	static int count;
	int _id;
};

class Edge {

	static int count;
	int _id;

    unsigned int vertex[2];

public:
    Edge( int v0, int v1 ) : _id(++count) {
        vertex[0] = v0;
        vertex[1] = v1;
    }
	PropList prop_list;
	int id(void) { return _id; }  

	unsigned int v( int index) { return vertex[index]; }
};

typedef std::map<int, Edge*> EdgeList_t;
typedef std::map<int, Vertex*> VertexList_t;

class Graph {
  public:
    EdgeList_t elist;
    VertexList_t vlist;
	Graph(int x) {;};	
	int num_vertices(void) { return vlist.size(); } 
};

} // namespace SST

#endif
