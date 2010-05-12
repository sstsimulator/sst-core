/*
// Mesh routing
inline bool SS_router::findRoute( int destNid, int inVC, int inDir,
                    int& outVC, int& outDir )
{
  int xDim = network->xDimSize();
  int yDim = network->yDimSize();
  int zDim = network->zDimSize();

    int my_xPos = calcXPosition( routerID, xDim, yDim, zDim );
    int my_yPos = calcYPosition( routerID, xDim, yDim, zDim );
    int my_zPos = calcZPosition( routerID, xDim, yDim, zDim );

    int dst_xPos = calcXPosition( destNid, xDim, yDim, zDim );
    int dst_yPos = calcYPosition( destNid, xDim, yDim, zDim );
    int dst_zPos = calcZPosition( destNid, xDim, yDim, zDim );

    outVC = 0;
    
    if ( my_xPos != dst_xPos ) {
      if ( my_xPos < dst_xPos ) outDir = 0;
      else outDir = 1;
    }
    else if ( my_yPos != dst_yPos ) {
      if ( my_yPos < dst_yPos ) outDir = 2;
      else outDir = 3;
    }
    else if ( my_zPos != dst_zPos ) {
      if ( my_zPos < dst_zPos ) outDir = 4;
      else outDir = 5;
    }
    else outDir = 6;

//     printf("Routing (%d,%d,%d) to (%d,%d,%d) out port %d\n",my_xPos,my_yPos,my_zPos,dst_xPos,dst_yPos,dst_zPos,outDir);
    
    return true;
}
*/

 /*
// Unidirectional torus routing
inline bool SS_router::findRoute( int destNid, int inVC, int inDir,
                    int& outVC, int& outDir )
{
  int xDim = network->xDimSize();
  int yDim = network->yDimSize();
  int zDim = network->zDimSize();

    int my_xPos = calcXPosition( routerID, xDim, yDim, zDim );
    int my_yPos = calcYPosition( routerID, xDim, yDim, zDim );
    int my_zPos = calcZPosition( routerID, xDim, yDim, zDim );

    int dst_xPos = calcXPosition( destNid, xDim, yDim, zDim );
    int dst_yPos = calcYPosition( destNid, xDim, yDim, zDim );
    int dst_zPos = calcZPosition( destNid, xDim, yDim, zDim );

    if ( my_xPos != dst_xPos ) {
      outDir = LINK_POS_X;
      if ( inDir == LINK_NEG_X ) {
        if ( iAmDateline(0) ) {
	  //           printf("Hit x dateline\n");
          outVC = 2;
        }
        else outVC = inVC;
      }
      else outVC = 0;
    }
    else if ( my_yPos != dst_yPos ) {
      outDir = LINK_POS_Y;
      if ( inDir == LINK_NEG_Y ) {
        if ( iAmDateline(1) ) {
	  //           printf("Hit y dateline\n");
          outVC = 2;
        }
        else outVC = inVC;
      }
      else outVC = 0;
    }
    else if ( my_zPos != dst_zPos ) {
      outDir = LINK_POS_Z;
      if ( inDir == LINK_NEG_Z ) {
        if ( iAmDateline(2) ) {
	  //           printf("Hit z dateline\n");
          outVC = 2;
        }
        else outVC = inVC;
      }
      else outVC = 0;
    }
    else {
      outDir = 6;
      outVC = 0;
    }
    
    return true;
}
 */
  /*
// Full torus routing
inline bool SS_router::findRoute( int destNid, int inVC, int inDir,
                    int& outVC, int& outDir )
{
  int xDim = network->xDimSize();
  int yDim = network->yDimSize();
  int zDim = network->zDimSize();

    int my_xPos = calcXPosition( routerID, xDim, yDim, zDim );
    int my_yPos = calcYPosition( routerID, xDim, yDim, zDim );
    int my_zPos = calcZPosition( routerID, xDim, yDim, zDim );

    int dst_xPos = calcXPosition( destNid, xDim, yDim, zDim );
    int dst_yPos = calcYPosition( destNid, xDim, yDim, zDim );
    int dst_zPos = calcZPosition( destNid, xDim, yDim, zDim );

    if ( my_xPos != dst_xPos ) {
      outDir = LINK_POS_X;
      if ( inDir == LINK_NEG_X ) {
        if ( iAmDateline(0) ) {
	  //           printf("Hit x dateline\n");
          outVC = 2;
        }
        else outVC = inVC;
      }
      else outVC = 0;
    }
    else if ( my_yPos != dst_yPos ) {
      outDir = LINK_POS_Y;
      if ( inDir == LINK_NEG_Y ) {
        if ( iAmDateline(1) ) {
	  //           printf("Hit y dateline\n");
          outVC = 2;
        }
        else outVC = inVC;
      }
      else outVC = 0;
    }
    else if ( my_zPos != dst_zPos ) {
      outDir = LINK_POS_Z;
      if ( inDir == LINK_NEG_Z ) {
        if ( iAmDateline(2) ) {
	  //           printf("Hit z dateline\n");
          outVC = 2;
        }
        else outVC = inVC;
      }
      else outVC = 0;
    }
    else {
      outDir = 6;
      outVC = 0;
    }
    
    return true;
}
*/

inline bool SS_router::findRoute( int destNid, int inVC, int inDir,
                    int& outVC, int& outDir )
{

    if ( destNid >= network->size() ) {
        return false;
    }

    outDir = findOutputDir( destNid );
    outVC = findOutputVC( inVC, inDir, outDir );
    DBprintf("XXX destNid=%d inVC=%d inDir=%d outVC=%d outDir=%d\n",
             destNid, inVC, inDir, outVC, outDir);
//    exit(0);
//    if ( inDir != ROUTER_HOST_PORT && inDir == outDir ) printf("Ping pong routing\n");
//    if ( inDir != ROUTER_HOST_PORT && dimension(inDir) == dimension(outDir) && inVC != outVC ) printf("Crossing dateline\n");
//    if ( inVC != outVC ) printf("Input/output VCs different\n");
//    printf("oVC: %d\n",outVC);
    return true;
}


inline int SS_router::findOutputDir( int destNid )
{
//    DBprintf( "XXX destNid=%d dir=%d\n", destNid, m_routingTableV[destNid] );
    return m_routingTableV[destNid];
}

inline int SS_router::dimension( int dir )
{
    // don't want to a modulo
    static int foo[] = { 0,0,1,1,2,2 };
    return foo[dir];
}

inline bool SS_router::iAmDateline( int dimension )
{
    return m_datelineV[ dimension ];
}

inline int SS_router::changeVC( int vc )
{
/*     static int foo[] = { 1, 0, 3, 2 }; */
    static int foo[] = { 2, 0, 0, 2 };
    return foo[vc];
}

inline int SS_router::findOutputVC( int inVC, int inDir, int outDir )
{
    int outVC = inVC;
    int inDim = dimension( inDir );
    int outDim = dimension( outDir );

/*     printf("---> %d %d %d %d\n",inDir,dimension(inDir),outDir,dimension(outDir)); */
    
    if ( inDir == ROUTER_HOST_PORT || outDir == ROUTER_HOST_PORT ) return 0;

/*     if ( inDim == outDim ) printf("same dimension: %d %d %d\n",inDir,outDir,inDim); */
    
/*     printf("-----> %d: %d %d %d\n",routerID,inDim,outDim,iAmDateline( inDim )); */
    if ( ( inDim == outDim ) && iAmDateline( inDim ) )
    {
        outVC = changeVC( inVC );
/* 	printf("--> %d %d %d %d\n",inDim,outDim,inVC,outVC); */
    }
 //   DBprintf( "XXX inVC=%d inDir=%d outDir=%d outVC=%d\n", 
  //                                          inVC, inDir, outDir, outVC );
    return outVC;
}

inline int SS_router::calcXPosition( int nodeNumber, int x, int y, int z )
{
    return nodeNumber % x; 
}

inline int SS_router::calcYPosition( int nodeNumber, int x, int y, int z )
{
    return ( nodeNumber / x ) % y; 
}

inline int SS_router::calcZPosition( int nodeNumber, int x, int y, int z )
{
    return nodeNumber / ( x * y ); 
}

inline int SS_router::calcDirection( int srcPos, int dstPos, int size )
{
    int pos; 
    int neg;
    if ( srcPos < dstPos ) {
        pos = ( dstPos - srcPos ); 
        neg = srcPos + ( size - dstPos )  + 1;
    } else {
        neg = ( srcPos - dstPos ); 
        pos = dstPos + ( size - srcPos )  + 1;
    }
    return pos > neg ? -1 : + 1; 
}
