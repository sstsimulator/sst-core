
#ifndef SST_CORE_PART_ZOLT
#define SST_CORE_PART_ZOLT

#include <sst/core/part/sstpart.h>
#include <sst/core/output.h>

#include <zoltan.h>
#include <mpi.h>

using namespace SST;
using namespace SST::Partition;

namespace SST {
namespace Partition {

class SSTZoltanPartition : public SST::Partition::SSTPartitioner {

	public:
		SSTZoltanPartition(int verbosity);
		~SSTZoltanPartition();
		void performPartition(ConfigGraph* graph);

	protected:
		void initZoltan();
		int rankcount;
		Output* partOutput;
		struct Zoltan_Struct * zolt_config;
		int rank;

};

}
}

/*extern "C" {
static int  sst_zoltan_count_vertices(void* data, int* ierr);
static void sst_zoltan_get_vertex_list(void* data, int sizeGID, int sizeLID,
	ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
	int wgt_dim, float* obj_wgts, int* ierr);
static void sst_zoltan_get_num_edges_list(void *data, int sizeGID, int sizeLID, int num_obj,
             ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
             int *numEdges, int *ierr);
static void sst_zoltan_get_edge_list(void *data, int sizeGID, int sizeLID,
        int num_obj, ZOLTAN_ID_PTR globalID, ZOLTAN_ID_PTR localID,
        int *num_edges,
        ZOLTAN_ID_PTR nborGID, int *nborProc,
        int wgt_dim, float *ewgts, int *ierr);
}*/

#endif
