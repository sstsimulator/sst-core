#ifndef SSB_FETCH_REC_H
#define SSB_FETCH_REC_H

//: IFETCH -> DISPATCH instruction queue definition 
//!SEC:ssBack
struct fetch_rec {
  instruction *IR;				/* inst register */
  md_addr_t regs_PC, pred_PC;		/* current PC, predicted next PC */
  struct bpred_update_t dir_update;	/* bpred direction update info */
  int stack_recover_idx;		/* branch predictor RSB index */
  unsigned int ptrace_seq;		/* print trace sequence id */
};

#endif
