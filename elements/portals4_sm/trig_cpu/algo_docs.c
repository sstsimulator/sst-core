/****** BARRIER (tree) ******/

for (i = 0 ; i < num_children ; ++i) {
    recv(0, children[i]);
 }
if (num_children != 0) waitall();
if (my_root != my_id) {
    send(0, my_root);
    recv(0, my_root);
    waitall();
 }
for (i = 0 ; i < num_children ; ++i) {
    send(0, children[i]);
 }

/****** BARRIER (recdbl) ******/

for (level = 0x1 ; level < num_nodes ; level <<= 1) {
     remote = my_id ^ level;
     recv(remote, 0, NULL);
     send(remote, 0, NULL);
     waitall();
 }

/****** BARRIER (dissem) ******/

for (level = 0x1 ; level < num_nodes ; level <<= log2(radix)) {
    for (i = 0 ; i < (radix - 1) ; ++i) {
        isend((my_id + level + i) % num_nodes, NULL, 0);
        irecv((my_id + num_nodes - (level + i)) % num_nodes, NULL, handle);
        waitall();
    }
 }

/****** Triggered BARRIER (recdbl) ******/

PtlPut(my_md_h, 0, 0, 0, my_id, 0, 0, 0, NULL, 0);
PtlPut(my_md_h, 0, 0, 0, my_id ^ 0x1, 0, 0, 0, NULL, 0);
for (i = 1, level = 0x10 ; level < num_nodes ; level <<=1, ++i) {
    PtlTriggeredPut(my_md_h, 0, 0, 0, my_id, 0, i, 0, NULL, 0, my_level_ct_hs[i - 1], 2);
    PtlTriggeredPut(my_md_h, 0, 0, 0, remote, 0, i, 0, NULL, 0, my_level_ct_hs[i - 1], 2);
    PtlTriggeredCTInc(my_level_ct_hs[i - 1], -2, my_level_ct_hs[i - 1], 2);
 }
PtlCTWait(my_lvel_ct_hs[my_levels - 1], 2);
PtlTriggeredCTInc(my_level_ct_hs[my_levels - 1], -2, my_level_ct_hs[my_levels - 1], 2);

/****** Triggered BARRIER (dissem) ******/
for (j = 1 ; j < radix ; ++j) {
    PtlPut(my_md_h, 0, 0, 0, (my_id + j) % num_nodes, 0, 0, 0, NULL, 0);
 }

for (i = 1, level = 0x2 ; level < num_nodes ; level << log2(radix), ++i) {
    for (j = 0 ; j < (radix - 1) ;++j) {
        remote = (id + level + i) % num_nodes;
        PtlTriggeredPut(my_md_h, 0, 0, 0, remote, 0, i, 0, NULL, 0, 
                        level_ct_hs[i - 1], radix - 1);
    }

    PtlTriggeredCTInc(level_ct_hs[i - 1], -(radix - 1), 
                      level_ct_hs[i - 1], (radix - 1));
 }

// wait for completion
PtlCTWait(level_ct_h[levels], (radix - 1));
PtlTriggeredCTInc(level_ct_hs[levels - 1], -(radix - 1), 
                  level_ct_hs[levels], (radix - 1));

/****** Triggered BARRIER (tree) ******/    

if (num_children == 0) {
    PtlPut(my_md_h, 0, 0, 0, my_root, PT_UP, 0, 0, NULL, 0);
 } else {
    if (my_id != my_root) {
        PtlTriggeredPut(my_md_h, 0, 0, 0, my_root, PT_UP, 0, 0, NULL, 0, 
                        up_tree_ct_h, num_children);
    } else {
        PtlTriggeredCTInc(down_tree_ct_h, 1, up_tree_ct_h, num_children);
    }
    PtlTriggeredCTInc(up_tree_ct_h, -num_children, up_tree_ct_h, num_children);
 
    for (i = 0 ; i < num_children ; ++i) {
        PtlTriggeredPut(my_md_h, 0, 0, 0, my_children[i], PT_DOWN, 0, 0, NULL, 0,
                        down_tree_ct_h, 1);
    }
 }
PtlCTWait(down_tree_ct_h, 1);
PtlTriggeredCTInc(down_tree_ct_h, -1, down_tree_ct_h, 1);


/****** BCAST ******/

for (i = 0 ; i < size ; i += chunksize) {
    if (my_root != my_id) {
        recv(size, my_root);
    }
    for (j = 0 ; j < num_children ; ++j) {
        send(size, children[i]);
    }
    waitall();
 }

/****** Triggered BCAST ******/

if (msg_size < chunk_size) {
    /* short message protocol */

    if (my_root == my_id) {
        /* copy to self */
        memcpy(out_buf, in_buf, msg_size);
        /* send to children */
        for (i = 0 ; i < num_children ; ++i) {
            PtlPut(out_md_h, 0, msg_size, 0, children[i], PTL_BOUNCE, 0x0, 
                   0, NULL, 0);
        }
    } else {
        /* copy into user's in buffer and reset bounce count */
        PtlTriggeredPut(bounce_md_h, 0, msg_size, 0, my_id,
                        PT_OUT, 0x0, 0, NULL, 0, bounce_ct_h, 1);
        PtlTriggeredCTInc(bounce_ct_h, -1, bounce_ct_h, 1);
        /* ack that we are done with the bounce buffer */
        PtlTriggeredPut(ack_md_h, 0, 0, 0, my_root, PTL_ACK, 0x0, 0, 
                        NULL, 0, out_me_ct_h, 1);
        /* send down tree to all children */
        for (i = 0 ; i < num_children ; ++i) {
            PtlTriggeredPut(out_md_h, 0, msg_size, 0, children[i], 
                            PTL_BOUNCE, 0x0, 0, NULL, 0, out_me_ct_h, 1);
        }
    }
    /* If leaf, wait for copy into user's in buffer to complete,
       otherwise wait for the requisite number of acks to prevent
       reuse of bounce buffer before we're ready */
    if (num_children > 0) {
        PtlCTWait(ack_ct_h, num_children);
    } else {
        PtlCTWait(out_me_ct_h, 1);
    }

 } else {
    /* long message protocol */

    if (my_root == my_id) {
        /* copy to self */
        memcpy(out_buf, in_buf, msg_size);
        /* send to children */
        for (j = 0 ; j < msg_size ; j += chunk_size) {
            for (i = 0 ; i < num_children ; ++i) {
                PtlPut(bounce_md_h, 0, 0, 0, children[i], PTL_BOUNCE, 0x0, 
                       0, NULL, 0);
            }
        }

    } else {
        /* iterate over chunks */
        for (j = 0 ; j < msg_size ; j += chunk_size) {
            /* when a chunk is ready, issue get. */
            PtlTriggeredGet(out_md_h, j, comm_length, my_root, PTL_OUT, 0x0,
                            NULL, j, bounce_ct_h, j / chunk_size);
            /* then when the get is completed, send ready acks to children */
            for (i = 0 ; i < num_children ; ++i) {
                PtlTriggeredPut(bounce_md_h, 0, 0, 0, children[i], PTL_BOUNCE, 0x0, 
                                0, NULL, 0,
                                out_md_ct_h, j / chunk_size);
            }
        }
        /* reset 0-byte put received counter */
        count = (msg_size + chunk_size - 1) / chunk_size;
        PtlTriggeredCTInc(bounce_ct_h, -count, bounce_ct_h, count);
    }
    if (num_children > 0) {
        /* wait for children gets */
        count = num_children * ((msg_size + chunk_size - 1) / chunk_size);
        PtlCTWait(out_me_ct_h, count);
    } else {
        /* wait for local gets to complete */
        count = (msg_size + chunk_size - 1) / chunk_size;
        PtlCTWait(out_md_ct_h, count);
    }
 }
