/**
 * Virtual Memory Visualization Tool - TLB Simulator Header
 * 
 * Translation Lookaside Buffer (TLB) simulation with support for
 * different replacement policies (LRU, FIFO, Random).
 * 
 * The TLB is a hardware cache that stores recent virtual-to-physical
 * address translations to avoid expensive page table walks.
 */

#ifndef TLB_SIM_H
#define TLB_SIM_H

#include "vmem_types.h"

/* ============================================================================
 * TLB Lifecycle Functions
 * ============================================================================ */

/**
 * Initialize a new TLB
 * 
 * @param size               Number of entries in TLB
 * @param replacement_policy Replacement policy (TLB_POLICY_LRU, etc.)
 * @return                   Pointer to new TLB, or NULL on error
 */
TLB* tlb_init(int size, int replacement_policy);

/**
 * Free TLB resources
 * 
 * @param tlb  TLB to free
 */
void tlb_free(TLB *tlb);

/**
 * Flush all TLB entries (invalidate)
 * 
 * @param tlb  TLB to flush
 */
void tlb_flush(TLB *tlb);

/**
 * Reset TLB statistics
 * 
 * @param tlb  TLB to reset stats for
 */
void tlb_reset_stats(TLB *tlb);

/* ============================================================================
 * TLB Access Functions
 * ============================================================================ */

/**
 * Lookup a virtual page number in the TLB
 * 
 * @param tlb   TLB to search
 * @param vpn   Virtual Page Number to lookup
 * @param pfn   Pointer to store Physical Frame Number (if hit)
 * @return      1 if hit (pfn is set), 0 if miss
 */
int tlb_lookup(TLB *tlb, uint64_t vpn, uint64_t *pfn);

/**
 * Insert a new mapping into the TLB
 * 
 * If TLB is full, uses configured replacement policy to evict an entry.
 * 
 * @param tlb    TLB to insert into
 * @param vpn    Virtual Page Number
 * @param pfn    Physical Frame Number
 * @param dirty  Dirty bit (1 if page has been modified)
 */
void tlb_insert(TLB *tlb, uint64_t vpn, uint64_t pfn, int dirty);

/**
 * Invalidate a specific TLB entry
 * 
 * Used when a page is unmapped or its permissions change.
 * 
 * @param tlb  TLB to modify
 * @param vpn  Virtual Page Number to invalidate
 * @return     1 if entry was found and invalidated, 0 if not found
 */
int tlb_invalidate(TLB *tlb, uint64_t vpn);

/**
 * Combined lookup + insert (simulates full TLB access)
 * 
 * Looks up VPN. If miss, simulates fetching from page table and inserts.
 * 
 * @param tlb    TLB to access
 * @param vpn    Virtual Page Number
 * @param pfn    Physical Frame Number (for insertion on miss)
 * @param dirty  Dirty bit
 * @return       1 if hit, 0 if miss
 */
int tlb_access(TLB *tlb, uint64_t vpn, uint64_t pfn, int dirty);

/* ============================================================================
 * TLB Statistics Functions
 * ============================================================================ */

/**
 * Get TLB hit count
 * 
 * @param tlb  TLB to query
 * @return     Number of hits
 */
uint64_t tlb_get_hits(const TLB *tlb);

/**
 * Get TLB miss count
 * 
 * @param tlb  TLB to query
 * @return     Number of misses
 */
uint64_t tlb_get_misses(const TLB *tlb);

/**
 * Get TLB hit rate
 * 
 * @param tlb  TLB to query
 * @return     Hit rate as percentage (0.0 to 100.0)
 */
double tlb_get_hit_rate(const TLB *tlb);

/**
 * Get total number of accesses
 * 
 * @param tlb  TLB to query
 * @return     Total accesses (hits + misses)
 */
uint64_t tlb_get_total_accesses(const TLB *tlb);

/* ============================================================================
 * TLB Display Functions
 * ============================================================================ */

/**
 * Print TLB contents to stdout
 * 
 * @param tlb  TLB to print
 */
void tlb_print(const TLB *tlb);

/**
 * Print TLB statistics to stdout
 * 
 * @param tlb  TLB to print stats for
 */
void tlb_print_stats(const TLB *tlb);

/**
 * Get TLB entry at specific index (for display)
 * 
 * @param tlb    TLB to query
 * @param index  Index of entry
 * @param entry  Pointer to store entry copy
 * @return       0 on success, -1 if index out of bounds
 */
int tlb_get_entry(const TLB *tlb, int index, TLBEntry *entry);

/* ============================================================================
 * TLB Replacement Policy Names
 * ============================================================================ */

/**
 * Get name of replacement policy
 * 
 * @param policy  Policy constant (TLB_POLICY_LRU, etc.)
 * @return        Policy name string
 */
const char* tlb_policy_name(int policy);

#endif /* TLB_SIM_H */
