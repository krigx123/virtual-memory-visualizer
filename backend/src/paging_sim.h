/**
 * Demand Paging Simulator
 * 
 * Simulates demand paging with configurable number of frames
 * and replacement policies (LRU, FIFO, Random, Clock).
 */

#ifndef PAGING_SIM_H
#define PAGING_SIM_H

#include <stdint.h>
#include "vmem_types.h"

/* ============================================================================
 * Constants
 * ============================================================================ */

#define MAX_PAGING_FRAMES 64

/* Reuse TLB_POLICY_* from vmem_types.h */
#define PAGING_POLICY_LRU    TLB_POLICY_LRU
#define PAGING_POLICY_FIFO   TLB_POLICY_FIFO
#define PAGING_POLICY_RANDOM TLB_POLICY_RANDOM
#define PAGING_POLICY_CLOCK  TLB_POLICY_CLOCK

/* ============================================================================
 * Data Structures
 * ============================================================================ */

/**
 * A single physical memory frame
 */
typedef struct {
    int vpn;              /* Virtual page number stored in this frame (-1 = empty) */
    int loaded_at;        /* Access counter when page was loaded */
    int last_access;      /* Access counter when last accessed */
    int reference_bit;    /* For clock algorithm */
} PageFrame;

/**
 * Paging simulator state
 */
typedef struct {
    PageFrame frames[MAX_PAGING_FRAMES];
    int num_frames;
    int policy;           /* PAGING_POLICY_* */
    int page_faults;
    int page_hits;
    int access_counter;
    int clock_hand;
    int initialized;
} PagingSimulator;

/* ============================================================================
 * Functions
 * ============================================================================ */

/**
 * Initialize the paging simulator
 * 
 * @param sim        Simulator instance
 * @param num_frames Number of physical frames (1-64)
 * @param policy     Replacement policy (PAGING_POLICY_*)
 */
void paging_init(PagingSimulator *sim, int num_frames, int policy);

/**
 * Access a page (may trigger page fault)
 * 
 * @param sim  Simulator instance
 * @param vpn  Virtual page number
 * @return     1 if hit, 0 if fault
 */
int paging_access(PagingSimulator *sim, int vpn);

/**
 * Get the name of a policy
 */
const char* paging_policy_name(int policy);

/**
 * Print current paging status
 */
void paging_print_status(PagingSimulator *sim);

/**
 * Flush all frames (reset to empty)
 */
void paging_flush(PagingSimulator *sim);

/**
 * CLI command handler
 */
void cmd_paging(const char *subcmd, char *arg);

#endif /* PAGING_SIM_H */
