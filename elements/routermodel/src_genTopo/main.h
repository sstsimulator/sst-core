/*
** $Id: main.h,v 1.6 2010/04/28 21:19:51 rolf Exp $
**
** Rolf Riesen, March 2010, Sandia National Laboratories
**
*/
#ifndef _GENTOPO_H_
#define _GENTOPO_H_

#define FALSE		(0)
#define TRUE		(1)


/* Which topology to generate? */
typedef enum {	TopoNone= 0,	/* Nothing specified */
		Mesh2D,		/* Two dimensional mesh, no wraparounds */
		Torus2D,	/* Two dimensional mesh, wraparounds in X and Y dimension */
		Torus2Dx,	/* Two dimensional mesh, wraparounds in X dimension only */
		Torus2Dy,	/* Two dimensional mesh, wraparounds in Y dimension only */
		Mesh3D,		/* Three dimensional mesh, no wraparounds */
		Torus3D,	/* Three dimensional mesh, wraparounds in X, Y, and Z dimension */
		Torus3Dx,	/* Three dimensional mesh, wraparounds in X dimension only */
		Torus3Dy,	/* Three dimensional mesh, wraparounds in Y dimension only */
		Torus3Dz,	/* Three dimensional mesh, wraparounds in Z dimension only */
		Torus3Dxy,	/* Three dimensional mesh, wraparounds in X and Y dimension */
		Torus3Dxz,	/* Three dimensional mesh, wraparounds in X and Z dimension */
		Torus3Dyz,	/* Three dimensional mesh, wraparounds in Y and Z dimension */
		Flat2Dbutter,	/* Flattened, two-dimensional butterfly */
		Ring,		/* A ring */
		Full,		/* A fully connected graph */
		Tree,		/* A simple binary tree with nodes at the leaves */
		FatTree,	/* A binary fat tree with nodes at the leaves */
		Hypercube	/* A hypercube */
} topo_type_t;

char *topo_names[]= {
    "none",
    "2D Mesh",
    "2D Torus",
    "2D Mesh, with x dimension wrap around",
    "2D Mesh, with y dimension wrap around",
    "3D Mesh",
    "3D Torus",
    "3D Mesh, with x dimension wrap around",
    "3D Mesh, with y dimension wrap around",
    "3D Mesh, with z dimension wrap around",
    "3D Mesh, with x and y dimension wrap around",
    "3D Mesh, with x and z dimension wrap around",
    "3D Mesh, with y and z dimension wrap around",
    "Flattened, two-dimensional butterfly",
    "Ring",
    "Fully connected",
    "Binary tree",
    "Binary fat tree",
    "Hypercube"
};

#endif /* _GENTOPO_H_ */
