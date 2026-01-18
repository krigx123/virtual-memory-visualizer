/**
 * Memory Playground Implementation
 * 
 * Active OS interaction with mmap, mlock, madvise.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include "playground.h"

/* ============================================================================
 * Static State
 * ============================================================================ */

static PlaygroundRegion playground_regions[MAX_PLAYGROUND_REGIONS];
static int playground_count = 0;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

const char* madvise_name(int advice) {
    switch(advice) {
        case MADV_NORMAL: return "NORMAL";
        case MADV_RANDOM: return "RANDOM";
        case MADV_SEQUENTIAL: return "SEQUENTIAL";
        case MADV_WILLNEED: return "WILLNEED";
        case MADV_DONTNEED: return "DONTNEED";
        default: return "UNKNOWN";
    }
}

/* ============================================================================
 * CLI Command Handler
 * ============================================================================ */

void cmd_playground(const char *subcmd, char *arg) {
    if (subcmd == NULL || *subcmd == '\0') {
        printf("Usage: mem <alloc|lock|unlock|advise|free|status|reset> [args]\n");
        return;
    }
    
    if (strcmp(subcmd, "alloc") == 0) {
        int size_mb = 10;
        if (arg != NULL && *arg != '\0') {
            size_mb = atoi(arg);
            if (size_mb < 1) size_mb = 1;
            if (size_mb > 1000) size_mb = 1000;
        }
        
        if (playground_count >= MAX_PLAYGROUND_REGIONS) {
            printf("[ERROR] Maximum regions (%d) reached. Use 'mem reset' first.\n", MAX_PLAYGROUND_REGIONS);
            return;
        }
        
        size_t size_bytes = (size_t)size_mb * 1024 * 1024;
        
        void *addr = mmap(NULL, size_bytes, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        
        if (addr == MAP_FAILED) {
            printf("[ERROR] mmap failed: %s\n", strerror(errno));
            return;
        }
        
        /* Touch pages to actually allocate them */
        int pages_touched = 0;
        for (size_t offset = 0; offset < size_bytes; offset += 4096) {
            *((char*)addr + offset) = 0x42;
            pages_touched++;
        }
        
        int id = playground_count;
        playground_regions[id].addr = addr;
        playground_regions[id].size = size_bytes;
        playground_regions[id].locked = 0;
        playground_regions[id].advice = MADV_NORMAL;
        playground_regions[id].active = 1;
        playground_count++;
        
        printf("[OK] Allocated %d MB (Region #%d, %d pages touched)\n", size_mb, id, pages_touched);
        printf("     Address: %p\n", addr);
        
    } else if (strcmp(subcmd, "lock") == 0) {
        int id = 0;
        if (arg != NULL && *arg != '\0') {
            id = atoi(arg);
        }
        
        if (id < 0 || id >= playground_count || !playground_regions[id].active) {
            printf("[ERROR] Invalid region ID: %d\n", id);
            return;
        }
        
        if (playground_regions[id].locked) {
            printf("[WARN] Region #%d is already locked\n", id);
            return;
        }
        
        if (mlock(playground_regions[id].addr, playground_regions[id].size) != 0) {
            printf("[ERROR] mlock failed: %s\n", strerror(errno));
            printf("        (Requires root or CAP_IPC_LOCK capability)\n");
            return;
        }
        
        playground_regions[id].locked = 1;
        printf("[OK] Locked Region #%d (%zu MB) - cannot be swapped out\n", 
               id, playground_regions[id].size / (1024 * 1024));
        
    } else if (strcmp(subcmd, "unlock") == 0) {
        int id = 0;
        if (arg != NULL && *arg != '\0') {
            id = atoi(arg);
        }
        
        if (id < 0 || id >= playground_count || !playground_regions[id].active) {
            printf("[ERROR] Invalid region ID: %d\n", id);
            return;
        }
        
        if (!playground_regions[id].locked) {
            printf("[WARN] Region #%d is not locked\n", id);
            return;
        }
        
        if (munlock(playground_regions[id].addr, playground_regions[id].size) != 0) {
            printf("[ERROR] munlock failed: %s\n", strerror(errno));
            return;
        }
        
        playground_regions[id].locked = 0;
        printf("[OK] Unlocked Region #%d\n", id);
        
    } else if (strcmp(subcmd, "advise") == 0) {
        int id = 0;
        char hint_str[32] = "";
        
        if (arg != NULL && *arg != '\0') {
            sscanf(arg, "%d %31s", &id, hint_str);
        }
        
        if (id < 0 || id >= playground_count || !playground_regions[id].active) {
            printf("[ERROR] Invalid region ID: %d\n", id);
            return;
        }
        
        int advice = MADV_NORMAL;
        if (strcasecmp(hint_str, "RANDOM") == 0) advice = MADV_RANDOM;
        else if (strcasecmp(hint_str, "SEQUENTIAL") == 0) advice = MADV_SEQUENTIAL;
        else if (strcasecmp(hint_str, "WILLNEED") == 0) advice = MADV_WILLNEED;
        else if (strcasecmp(hint_str, "DONTNEED") == 0) advice = MADV_DONTNEED;
        else if (strcasecmp(hint_str, "NORMAL") == 0) advice = MADV_NORMAL;
        else if (strlen(hint_str) > 0) {
            printf("[ERROR] Unknown hint: %s\n", hint_str);
            printf("        Valid: NORMAL, RANDOM, SEQUENTIAL, WILLNEED, DONTNEED\n");
            return;
        }
        
        if (madvise(playground_regions[id].addr, playground_regions[id].size, advice) != 0) {
            printf("[ERROR] madvise failed: %s\n", strerror(errno));
            return;
        }
        
        playground_regions[id].advice = advice;
        printf("[OK] Applied %s hint to Region #%d\n", madvise_name(advice), id);
        
    } else if (strcmp(subcmd, "free") == 0) {
        int id = 0;
        if (arg != NULL && *arg != '\0') {
            id = atoi(arg);
        }
        
        if (id < 0 || id >= playground_count || !playground_regions[id].active) {
            printf("[ERROR] Invalid region ID: %d\n", id);
            return;
        }
        
        /* Unlock first if locked */
        if (playground_regions[id].locked) {
            munlock(playground_regions[id].addr, playground_regions[id].size);
        }
        
        if (munmap(playground_regions[id].addr, playground_regions[id].size) != 0) {
            printf("[ERROR] munmap failed: %s\n", strerror(errno));
            return;
        }
        
        playground_regions[id].active = 0;
        printf("[OK] Freed Region #%d (%zu MB)\n", id, playground_regions[id].size / (1024 * 1024));
        
    } else if (strcmp(subcmd, "status") == 0) {
        printf("\n");
        printf("MEMORY PLAYGROUND STATUS\n");
        printf("========================\n\n");
        
        int active_count = 0;
        size_t total_size = 0;
        size_t locked_size = 0;
        
        for (int i = 0; i < playground_count; i++) {
            if (playground_regions[i].active) {
                active_count++;
                total_size += playground_regions[i].size;
                if (playground_regions[i].locked) {
                    locked_size += playground_regions[i].size;
                }
            }
        }
        
        printf("Active Regions: %d / %d\n", active_count, MAX_PLAYGROUND_REGIONS);
        printf("Total Allocated: %zu MB\n", total_size / (1024 * 1024));
        printf("Total Locked: %zu MB\n\n", locked_size / (1024 * 1024));
        
        if (active_count > 0) {
            printf("┌────┬────────────────────┬──────────┬────────┬────────────┐\n");
            printf("│ ID │      Address       │   Size   │ Locked │   Advice   │\n");
            printf("├────┼────────────────────┼──────────┼────────┼────────────┤\n");
            
            for (int i = 0; i < playground_count; i++) {
                if (playground_regions[i].active) {
                    printf("│ %2d │ %18p │ %5zu MB │   %s   │ %-10s │\n",
                           i,
                           playground_regions[i].addr,
                           playground_regions[i].size / (1024 * 1024),
                           playground_regions[i].locked ? "✓" : "✗",
                           madvise_name(playground_regions[i].advice));
                }
            }
            printf("└────┴────────────────────┴──────────┴────────┴────────────┘\n");
        }
        printf("\n");
        
    } else if (strcmp(subcmd, "reset") == 0) {
        int freed_count = 0;
        size_t freed_size = 0;
        
        for (int i = 0; i < playground_count; i++) {
            if (playground_regions[i].active) {
                if (playground_regions[i].locked) {
                    munlock(playground_regions[i].addr, playground_regions[i].size);
                }
                munmap(playground_regions[i].addr, playground_regions[i].size);
                freed_size += playground_regions[i].size;
                freed_count++;
            }
        }
        
        playground_count = 0;
        memset(playground_regions, 0, sizeof(playground_regions));
        
        printf("[OK] Freed %d regions (%zu MB total)\n", freed_count, freed_size / (1024 * 1024));
        
    } else {
        printf("Unknown playground command: %s\n", subcmd);
        printf("Usage: mem <alloc|lock|unlock|advise|free|status|reset> [args]\n");
    }
}
