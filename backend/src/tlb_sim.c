/**
 * Virtual Memory Visualization Tool - TLB Simulator Implementation
 * 
 * Simulates Translation Lookaside Buffer behavior with support for
 * LRU, FIFO, and Random replacement policies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "tlb_sim.h"

/* ============================================================================
 * TLB Lifecycle Functions
 * ============================================================================ */

TLB* tlb_init(int size, int replacement_policy) {
    if (size <= 0 || size > 1024) {
        return NULL;
    }
    
    TLB *tlb = (TLB*)malloc(sizeof(TLB));
    if (tlb == NULL) {
        return NULL;
    }
    
    tlb->entries = (TLBEntry*)calloc(size, sizeof(TLBEntry));
    if (tlb->entries == NULL) {
        free(tlb);
        return NULL;
    }
    
    tlb->size = size;
    tlb->replacement_policy = replacement_policy;
    tlb->hits = 0;
    tlb->misses = 0;
    tlb->access_counter = 0;
    
    /* Initialize all entries as invalid */
    for (int i = 0; i < size; i++) {
        tlb->entries[i].valid = 0;
        tlb->entries[i].last_access = 0;
    }
    
    /* Seed random number generator for random replacement */
    srand(time(NULL));
    
    return tlb;
}

void tlb_free(TLB *tlb) {
    if (tlb != NULL) {
        if (tlb->entries != NULL) {
            free(tlb->entries);
        }
        free(tlb);
    }
}

void tlb_flush(TLB *tlb) {
    if (tlb == NULL) return;
    
    for (int i = 0; i < tlb->size; i++) {
        tlb->entries[i].valid = 0;
        tlb->entries[i].vpn = 0;
        tlb->entries[i].pfn = 0;
        tlb->entries[i].last_access = 0;
    }
}

void tlb_reset_stats(TLB *tlb) {
    if (tlb == NULL) return;
    
    tlb->hits = 0;
    tlb->misses = 0;
    tlb->access_counter = 0;
}

/* ============================================================================
 * TLB Access Functions
 * ============================================================================ */

int tlb_lookup(TLB *tlb, uint64_t vpn, uint64_t *pfn) {
    if (tlb == NULL) return 0;
    
    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].vpn == vpn) {
            /* TLB Hit */
            *pfn = tlb->entries[i].pfn;
            tlb->entries[i].accessed = 1;
            tlb->entries[i].last_access = tlb->access_counter++;
            tlb->hits++;
            return 1;
        }
    }
    
    /* TLB Miss */
    tlb->misses++;
    return 0;
}

/**
 * Find victim entry for replacement based on policy
 */
static int find_victim(TLB *tlb) {
    int victim = 0;
    
    /* First, try to find an empty slot */
    for (int i = 0; i < tlb->size; i++) {
        if (!tlb->entries[i].valid) {
            return i;
        }
    }
    
    /* No empty slot, apply replacement policy */
    switch (tlb->replacement_policy) {
        case TLB_POLICY_LRU:
            /* Find entry with smallest last_access (least recently used) */
            {
                uint64_t min_access = tlb->entries[0].last_access;
                victim = 0;
                for (int i = 1; i < tlb->size; i++) {
                    if (tlb->entries[i].last_access < min_access) {
                        min_access = tlb->entries[i].last_access;
                        victim = i;
                    }
                }
            }
            break;
            
        case TLB_POLICY_FIFO:
            /* Find entry with smallest last_access (first inserted) */
            /* For FIFO, we don't update last_access on hits, only on insert */
            {
                uint64_t min_access = tlb->entries[0].last_access;
                victim = 0;
                for (int i = 1; i < tlb->size; i++) {
                    if (tlb->entries[i].last_access < min_access) {
                        min_access = tlb->entries[i].last_access;
                        victim = i;
                    }
                }
            }
            break;
            
        case TLB_POLICY_RANDOM:
            victim = rand() % tlb->size;
            break;
            
        default:
            victim = 0;
    }
    
    return victim;
}

void tlb_insert(TLB *tlb, uint64_t vpn, uint64_t pfn, int dirty) {
    if (tlb == NULL) return;
    
    /* Check if entry already exists - if so, update it */
    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].vpn == vpn) {
            tlb->entries[i].pfn = pfn;
            tlb->entries[i].dirty = dirty;
            tlb->entries[i].accessed = 1;
            tlb->entries[i].last_access = tlb->access_counter++;
            return;
        }
    }
    
    /* Find slot for new entry */
    int slot = find_victim(tlb);
    
    tlb->entries[slot].vpn = vpn;
    tlb->entries[slot].pfn = pfn;
    tlb->entries[slot].valid = 1;
    tlb->entries[slot].dirty = dirty;
    tlb->entries[slot].accessed = 1;
    tlb->entries[slot].last_access = tlb->access_counter++;
}

int tlb_invalidate(TLB *tlb, uint64_t vpn) {
    if (tlb == NULL) return 0;
    
    for (int i = 0; i < tlb->size; i++) {
        if (tlb->entries[i].valid && tlb->entries[i].vpn == vpn) {
            tlb->entries[i].valid = 0;
            return 1;
        }
    }
    return 0;
}

int tlb_access(TLB *tlb, uint64_t vpn, uint64_t pfn, int dirty) {
    uint64_t found_pfn;
    
    if (tlb_lookup(tlb, vpn, &found_pfn)) {
        /* Hit - already counted in lookup */
        return 1;
    } else {
        /* Miss - insert the mapping */
        tlb_insert(tlb, vpn, pfn, dirty);
        return 0;
    }
}

/* ============================================================================
 * TLB Statistics Functions
 * ============================================================================ */

uint64_t tlb_get_hits(const TLB *tlb) {
    return tlb ? tlb->hits : 0;
}

uint64_t tlb_get_misses(const TLB *tlb) {
    return tlb ? tlb->misses : 0;
}

double tlb_get_hit_rate(const TLB *tlb) {
    if (tlb == NULL) return 0.0;
    
    uint64_t total = tlb->hits + tlb->misses;
    if (total == 0) return 0.0;
    
    return (double)tlb->hits / total * 100.0;
}

uint64_t tlb_get_total_accesses(const TLB *tlb) {
    return tlb ? (tlb->hits + tlb->misses) : 0;
}

/* ============================================================================
 * TLB Display Functions
 * ============================================================================ */

void tlb_print(const TLB *tlb) {
    if (tlb == NULL) {
        printf("TLB is NULL\n");
        return;
    }
    
    printf("\nTLB STATUS (%d entries, %s replacement)\n", 
           tlb->size, tlb_policy_name(tlb->replacement_policy));
    printf("========================================\n");
    printf("INDEX   VPN             PFN             VALID   LAST ACCESS\n");
    printf("----------------------------------------------------------------\n");
    
    for (int i = 0; i < tlb->size; i++) {
        TLBEntry *e = &tlb->entries[i];
        if (e->valid) {
            printf("%-7d 0x%-14lx 0x%-14lx YES     %lu\n",
                   i, e->vpn, e->pfn, e->last_access);
        } else {
            printf("%-7d (empty)         -               NO      -\n", i);
        }
    }
    printf("----------------------------------------------------------------\n");
}

void tlb_print_stats(const TLB *tlb) {
    if (tlb == NULL) return;
    
    printf("\nTLB STATISTICS\n");
    printf("==============\n");
    printf("Replacement Policy: %s\n", tlb_policy_name(tlb->replacement_policy));
    printf("Size: %d entries\n", tlb->size);
    printf("Hits: %lu\n", tlb->hits);
    printf("Misses: %lu\n", tlb->misses);
    printf("Total Accesses: %lu\n", tlb->hits + tlb->misses);
    printf("Hit Rate: %.2f%%\n", tlb_get_hit_rate(tlb));
    printf("\n");
}

int tlb_get_entry(const TLB *tlb, int index, TLBEntry *entry) {
    if (tlb == NULL || index < 0 || index >= tlb->size) {
        return -1;
    }
    
    memcpy(entry, &tlb->entries[index], sizeof(TLBEntry));
    return 0;
}

const char* tlb_policy_name(int policy) {
    switch (policy) {
        case TLB_POLICY_LRU:    return "LRU";
        case TLB_POLICY_FIFO:   return "FIFO";
        case TLB_POLICY_RANDOM: return "Random";
        default:               return "Unknown";
    }
}
