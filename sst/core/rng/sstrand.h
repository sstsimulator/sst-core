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


#include <iostream>
#include <fstream>

using namespace std;

/*
	Implements basic random number generation for SST core or 
	components.
*/
class SSTRandom {

    public:
  	SSTRandom(unsigned int initial_z,
		unsigned int initial_w);
	SSTRandom();
	double nextUniform();

    private:
	unsigned int generateNext();
	unsigned int m_z;
	unsigned int m_w;

};
