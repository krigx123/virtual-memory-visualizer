/**
 * Demand Paging Simulator Implementation
 * 
 * Simulates demand paging with configurable replacement policies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "paging_sim.h"
#include "tlb_sim.h"  /* For tlb_policy_name, parse_address */

/* Global paging simulator instance */
static PagingSimulator *global_paging = NULL;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

/* Forward declaration - defined in vmem_shell.c */
extern uint64_t parse_address(const char *str);

/* ============================================================================
 * Core Functions
 * ============================================================================ */

void paging_init(PagingSimulator *sim, int num_frames, int policy) {
    if (num_frames < 1) num_frames = 4;
    if (num_frames > MAX_PAGING_FRAMES) num_frames = MAX_PAGING_FRAMES;
    
    sim->num_frames = num_frames;
    sim->policy = policy;
    sim->page_faults = 0;
    sim->page_hits = 0;
    sim->access_counter = 0;
    sim->clock_hand = 0;
    sim->initialized = 1;
    
    for (int i = 0; i < num_frames; i++) {
        sim->frames[i].vpn = -1;
        sim->frames[i].loaded_at = 0;
        sim->frames[i].last_access = 0;
        sim->frames[i].reference_bit = 0;
    }
}

int paging_access(PagingSimulator *sim, int vpn) {
    /* Check if page is in memory */
    int frame_idx = -1;
    for (int i = 0; i < sim->num_frames; i++) {
        if (sim->frames[i].vpn == vpn) {
            frame_idx = i;
            break;
        }
    }
    
    if (frame_idx >= 0) {
        /* PAGE HIT */
        sim->page_hits++;
        sim->frames[frame_idx].last_access = sim->access_counter;
        sim->frames[frame_idx].reference_bit = 1;
        sim->access_counter++;
        return 1;  /* Hit */
    }
    
    /* PAGE FAULT */
    sim->page_faults++;
    
    int target_frame = -1;
    
    /* Look for empty frame first */
    for (int i = 0; i < sim->num_frames; i++) {
        if (sim->frames[i].vpn == -1) {
            target_frame = i;
            break;
        }
    }
    
    if (target_frame < 0) {
        /* Need to evict - apply policy */
        int policy = sim->policy;
        
        if (policy == PAGING_POLICY_LRU) {
            int min_access = INT_MAX;
            for (int i = 0; i < sim->num_frames; i++) {
                if (sim->frames[i].last_access < min_access) {
                    min_access = sim->frames[i].last_access;
                    target_frame = i;
                }
            }
        } else if (policy == PAGING_POLICY_FIFO) {
            int min_loaded = INT_MAX;
            for (int i = 0; i < sim->num_frames; i++) {
                if (sim->frames[i].loaded_at < min_loaded) {
                    min_loaded = sim->frames[i].loaded_at;
                    target_frame = i;
                }
            }
        } else if (policy == PAGING_POLICY_RANDOM) {
            target_frame = rand() % sim->num_frames;
        } else if (policy == PAGING_POLICY_CLOCK) {
            while (1) {
                if (!sim->frames[sim->clock_hand].reference_bit) {
                    target_frame = sim->clock_hand;
                    sim->clock_hand = (sim->clock_hand + 1) % sim->num_frames;
                    break;
                }
                sim->frames[sim->clock_hand].reference_bit = 0;
                sim->clock_hand = (sim->clock_hand + 1) % sim->num_frames;
            }
        }
    }
    
    /* Load page into frame */
    sim->frames[target_frame].vpn = vpn;
    sim->frames[target_frame].loaded_at = sim->access_counter;
    sim->frames[target_frame].last_access = sim->access_counter;
    sim->frames[target_frame].reference_bit = 1;
    
    sim->access_counter++;
    return 0;  /* Fault */
}

const char* paging_policy_name(int policy) {
    return tlb_policy_name(policy);  /* Reuse TLB policy names */
}

void paging_print_status(PagingSimulator *sim) {
    printf("\n");
    printf("PAGING SIMULATOR STATUS\n");
    printf("=======================\n");
    printf("Frames: %d | Policy: %s\n", sim->num_frames, paging_policy_name(sim->policy));
    printf("\n");
    printf("Physical Memory Frames:\n");
    printf("┌───────┬─────────┬─────────┬─────────────┐\n");
    printf("│ Frame │   VPN   │ Loaded  │ Last Access │\n");
    printf("├───────┼─────────┼─────────┼─────────────┤\n");
    
    for (int i = 0; i < sim->num_frames; i++) {
        if (sim->frames[i].vpn >= 0) {
            printf("│   %2d  │  0x%-4x │   %4d  │    %4d     │\n",
                   i, sim->frames[i].vpn,
                   sim->frames[i].loaded_at,
                   sim->frames[i].last_access);
        } else {
            printf("│   %2d  │  (empty) │    -    │      -      │\n", i);
        }
    }
    printf("└───────┴─────────┴─────────┴─────────────┘\n");
    printf("\n");
    
    int total = sim->page_hits + sim->page_faults;
    double hit_rate = total > 0 ? (double)sim->page_hits / total * 100 : 0;
    printf("Statistics:\n");
    printf("  Page Hits:   %d\n", sim->page_hits);
    printf("  Page Faults: %d\n", sim->page_faults);
    printf("  Hit Rate:    %.1f%%\n", hit_rate);
    printf("\n");
}

void paging_flush(PagingSimulator *sim) {
    for (int i = 0; i < sim->num_frames; i++) {
        sim->frames[i].vpn = -1;
    }
    sim->page_hits = 0;
    sim->page_faults = 0;
    sim->access_counter = 0;
}

/* ============================================================================
 * CLI Command Handler
 * ============================================================================ */

void cmd_paging(const char *subcmd, char *arg) {
    if (subcmd == NULL || *subcmd == '\0') {
        printf("Usage: paging <init|access|status|flush> [args]\n");
        return;
    }
    
    if (strcmp(subcmd, "init") == 0) {
        int num_frames = 4;
        int policy = PAGING_POLICY_LRU;
        
        if (arg != NULL && *arg != '\0') {
            char policy_str[32] = "";
            sscanf(arg, "%d %31s", &num_frames, policy_str);
            
            if (strlen(policy_str) > 0) {
                if (strcasecmp(policy_str, "FIFO") == 0) policy = PAGING_POLICY_FIFO;
                else if (strcasecmp(policy_str, "RANDOM") == 0) policy = PAGING_POLICY_RANDOM;
                else if (strcasecmp(policy_str, "CLOCK") == 0) policy = PAGING_POLICY_CLOCK;
            }
        }
        
        if (global_paging == NULL) {
            global_paging = (PagingSimulator *)malloc(sizeof(PagingSimulator));
        }
        
        paging_init(global_paging, num_frames, policy);
        printf("[OK] Paging simulator initialized with %d frames (%s replacement)\n", 
               num_frames, paging_policy_name(policy));
        
    } else if (strcmp(subcmd, "access") == 0) {
        if (global_paging == NULL || !global_paging->initialized) {
            printf("Paging not initialized. Use 'paging init' first.\n");
            return;
        }
        
        if (arg == NULL || *arg == '\0') {
            printf("Usage: paging access <address>\n");
            return;
        }
        
        uint64_t vaddr = parse_address(arg);
        int vpn = (int)(vaddr >> 12);
        
        /* Find target frame for output message */
        int frame_idx = -1;
        for (int i = 0; i < global_paging->num_frames; i++) {
            if (global_paging->frames[i].vpn == vpn) {
                frame_idx = i;
                break;
            }
        }
        
        if (frame_idx >= 0) {
            paging_access(global_paging, vpn);
            printf("[PAGE HIT] VPN 0x%x found in Frame %d\n", vpn, frame_idx);
        } else {
            /* Find where it will be loaded */
            int target = -1;
            for (int i = 0; i < global_paging->num_frames; i++) {
                if (global_paging->frames[i].vpn == -1) {
                    target = i;
                    break;
                }
            }
            
            int evicted_vpn = -1;
            if (target < 0) {
                /* Will evict - find victim based on current state */
                if (global_paging->policy == PAGING_POLICY_LRU) {
                    int min_access = INT_MAX;
                    for (int i = 0; i < global_paging->num_frames; i++) {
                        if (global_paging->frames[i].last_access < min_access) {
                            min_access = global_paging->frames[i].last_access;
                            target = i;
                        }
                    }
                } else if (global_paging->policy == PAGING_POLICY_FIFO) {
                    int min_loaded = INT_MAX;
                    for (int i = 0; i < global_paging->num_frames; i++) {
                        if (global_paging->frames[i].loaded_at < min_loaded) {
                            min_loaded = global_paging->frames[i].loaded_at;
                            target = i;
                        }
                    }
                } else {
                    target = 0;
                }
                evicted_vpn = global_paging->frames[target].vpn;
            }
            
            paging_access(global_paging, vpn);
            
            if (evicted_vpn >= 0) {
                printf("[PAGE FAULT] VPN 0x%x not in memory, evicted VPN 0x%x from Frame %d\n", 
                       vpn, evicted_vpn, target);
            } else {
                printf("[PAGE FAULT] VPN 0x%x not in memory, loaded into Frame %d\n", vpn, target);
            }
        }
        
    } else if (strcmp(subcmd, "status") == 0) {
        if (global_paging == NULL || !global_paging->initialized) {
            printf("Paging not initialized. Use 'paging init' first.\n");
            return;
        }
        paging_print_status(global_paging);
        
    } else if (strcmp(subcmd, "flush") == 0) {
        if (global_paging == NULL) {
            printf("Paging not initialized.\n");
            return;
        }
        paging_flush(global_paging);
        printf("[OK] Paging simulator flushed\n");
        
    } else {
        printf("Unknown paging command: %s\n", subcmd);
        printf("Usage: paging <init|access|status|flush> [args]\n");
    }
}
