#include "ssb_rs_link.h"

/* initialize the free RS_LINK pool */
RS_link_list::RS_link_list(int nlinks)
{
  int i;
  RS_link *link;

  head = NULL;
  for (i=0; i<nlinks; i++)
    {
      link = (RS_link*)calloc(1, sizeof(struct RS_link));
      if (!link)
	fatal("out of virtual memory");
      link->next = head;
      head = link;
    }

  RSLINK_NULL.next = NULL;
  RSLINK_NULL.rs = NULL;
  RSLINK_NULL.tag = 0;
 }
