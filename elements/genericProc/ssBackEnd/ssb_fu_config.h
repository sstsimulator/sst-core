#ifndef SSB_FU_CONFIG_H
#define SSB_FU_CONFIG_H

/*
 * functional unit resource configuration
 */

#include "ssb_fu_config-defs.h"

/* resource pool definition, NOTE: update FU_*_INDEX defs if you change this */
res_desc fu_config[] = {
  {
    "integer-ALU",
    4,
    0,
    {
      { IntALU, 1, 1 }
    }
  },
  {
    "integer-MULT/DIV",
    1,
    0,
    {
      { IntMULT, 3, 1 },
      { IntDIV, 20, 19 }
    }
  },
  {
    "memory-port",
    2,
    0,
    {
      { RdPort, 1, 1 },
      { WrPort, 1, 1 }
    }
  },
  {
    "FP-adder",
    4,
    0,
    {
      { FloatADD, 2, 1 },
      { FloatCMP, 2, 1 },
      { FloatCVT, 2, 1 }
    }
  },
  {
    "FP-MULT/DIV",
    1,
    0,
    {
      { FloatMULT, 4, 1 },
      { FloatDIV, 12, 12 },
      { FloatSQRT, 24, 24 }
    }
  },
};


#endif
