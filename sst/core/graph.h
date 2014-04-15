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

#ifndef SST_CORE_GRAPH_H
#define SST_CORE_GRAPH_H

#include <map>
#include <list>

namespace SST {

const char GRAPH_COMP_NAME[] = "comp_name";
const char GRAPH_LINK_NAME[] = "link_name";
const char GRAPH_WEIGHT[]    = "weight";
const char GRAPH_RANK[]      = "rank";
const char GRAPH_ID[]        = "id";

/** Property list
 *  @brief Maps names to values
 */
class PropList {
public:
    /** Set a mapping
     * @param name - Name to map to a value
     * @param value - value to be mapped to
     */
    void set( std::string name, std::string value ) {
        map[name] = value;
    }
    /** Returns the value associated with a name */
    std::string &get( std::string name ) {
        return map[name];
    }
private:
    std::map<std::string,std::string> map;
};

/** Represent a vertex in a Graph */
class Vertex {
public:
    /** Create a new, empty Vertex */
    Vertex() : _id(++count) {};
    /** Property list of this Vertex */
    PropList prop_list;
    /** Rank of this vertex */
    int rank;
    /** Returns the ID of this vertex */
    int id() { return _id; }
    /** Adjacency list */
    std::list<int> adj_list;

private:
    static int count;
    int _id;
};


/** Represent an edge between Verticies in a Graph */
class Edge {

    static int count;
    int _id;

    unsigned int vertex[2];

public:
    /** Create an edge between two verticies */
    Edge( int v0, int v1 ) : _id(++count) {
        vertex[0] = v0;
        vertex[1] = v1;
    }
    /** Property list of this edge */
    PropList prop_list;
    /** Returns the ID of this edge */
    int id(void) { return _id; }

    /** Returns the associated Vertex IDs
     * @param index - 0 or 1 for which Vertex to return
     */
    unsigned int v( int index) { return vertex[index]; }
};

/** Map ids to Edges */
typedef std::map<int, Edge*> EdgeList_t;
/** Map ids to Verticies */
typedef std::map<int, Vertex*> VertexList_t;

/** Represent a generic graph */
class Graph {
public:
    /** List of Edges */
    EdgeList_t elist;
    /** List of Verticies */
    VertexList_t vlist;
    /** Create a new, empty Graph */
    Graph(int x) {;};
    /** Return the number of verticies in the graph */
    int num_vertices(void) { return vlist.size(); } 
};

} // namespace SST

#endif // SST_CORE_GRAPH_H
