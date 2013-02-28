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

}
