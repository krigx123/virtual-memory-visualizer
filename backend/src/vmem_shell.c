/**
 * Virtual Memory Visualization Tool - Interactive Shell
 * 
 * Command-line interface for demonstrating virtual memory concepts.
 * Perfect for terminal demos during teacher presentations.
 * 
 * Commands:
 *   ps                    - List all processes
 *   select <pid>          - Select a process to analyze
 *   maps                  - Show memory regions of selected process
 *   translate <addr>      - Translate virtual to physical address
 *   pagewalk <addr>       - Show full page table walk
 *   stats                 - Show memory statistics
 *   faults                - Show page fault statistics
 *   tlb init <size>       - Initialize TLB simulator
 *   tlb lookup <addr>     - Lookup address in TLB
 *   tlb status            - Show TLB contents
 *   tlb flush             - Flush TLB
 *   sysinfo               - Show system memory info
 *   help                  - Show help
 *   exit                  - Exit shell
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "vmem_types.h"
#include "proc_reader.h"
#include "addr_translate.h"
#include "tlb_sim.h"
#include "json_output.h"

/* Global state */
static int selected_pid = -1;
static char selected_name[256] = "";
static TLB *global_tlb = NULL;
static MemoryRegion *cached_regions = NULL;
static int cached_region_count = 0;

/* ============================================================================
 * Helper Functions
 * ============================================================================ */

static void print_banner(void) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║        Virtual Memory Visualization Tool - CLI v1.0          ║\n");
    printf("║              OS Lab Project - Memory Management              ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Type 'help' for available commands.\n");
    printf("Note: Some commands require root privileges.\n");
    printf("\n");
}

static void print_help(void) {
    printf("\n");
    printf("AVAILABLE COMMANDS:\n");
    printf("===================\n");
    printf("\n");
    printf("Process Commands:\n");
    printf("  ps                     List all running processes\n");
    printf("  select <pid>           Select a process to analyze\n");
    printf("\n");
    printf("Memory Analysis (requires selected process):\n");
    printf("  maps                   Show memory regions (from /proc/[pid]/maps)\n");
    printf("  translate <addr>       Translate virtual address to physical\n");
    printf("  pagewalk <addr>        Show detailed page table walk\n");
    printf("  stats                  Show memory statistics\n");
    printf("  faults                 Show page fault statistics\n");
    printf("\n");
    printf("TLB Simulator:\n");
    printf("  tlb init <size> [policy]   Initialize TLB (policy: LRU, FIFO, RANDOM, CLOCK)\n");
    printf("  tlb lookup <addr>      Lookup address in TLB\n");
    printf("  tlb access <addr>      Access address (lookup + insert on miss)\n");
    printf("  tlb status             Show TLB contents and statistics\n");
    printf("  tlb flush              Flush all TLB entries\n");
    printf("\n");
    printf("Demand Paging Simulator:\n");
    printf("  paging init <frames> [policy]  Initialize paging (policy: LRU, FIFO, RANDOM, CLOCK)\n");
    printf("  paging access <addr>       Access a page (may cause page fault)\n");
    printf("  paging status              Show physical memory frames and statistics\n");
    printf("  paging flush               Clear all frames\n");
    printf("\n");
    printf("Memory Playground (Active OS Interaction):\n");
    printf("  mem alloc <size_mb>        Allocate memory using mmap()\n");
    printf("  mem lock <id>              Lock region with mlock() (prevent swapping)\n");
    printf("  mem unlock <id>            Unlock region with munlock()\n");
    printf("  mem advise <id> <hint>     Apply madvise() hint (WILLNEED, DONTNEED, etc.)\n");
    printf("  mem free <id>              Free allocated region\n");
    printf("  mem status                 Show all allocated regions\n");
    printf("  mem reset                  Free all regions\n");
    printf("\n");
    printf("System Information:\n");
    printf("  sysinfo                Show system memory information\n");
    printf("\n");
    printf("Other:\n");
    printf("  help                   Show this help message\n");
    printf("  clear                  Clear screen\n");
    printf("  exit                   Exit the shell\n");
    printf("\n");
    printf("Note: Address arguments can be in hex (0x...) or decimal.\n");
    printf("\n");
}

static void trim(char *str) {
    /* Remove leading whitespace */
    char *start = str;
    while (isspace(*start)) start++;
    if (start != str) memmove(str, start, strlen(start) + 1);
    
    /* Remove trailing whitespace */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) *end-- = '\0';
}

static uint64_t parse_address(const char *str) {
    if (str == NULL) return 0;
    
    /* Skip leading whitespace */
    while (isspace(*str)) str++;
    
    if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) {
        return strtoull(str, NULL, 16);
    } else {
        return strtoull(str, NULL, 10);
    }
}

/* ============================================================================
 * Command Handlers
 * ============================================================================ */

static void cmd_ps(void) {
    ProcessInfo processes[100];
    int count = get_process_list(processes, 100);
    
    if (count < 0) {
        printf("Failed to read process list\n");
        return;
    }
    
    printf("\n");
    printf("PID     NAME                            MEMORY      STATE\n");
    printf("------------------------------------------------------------\n");
    
    for (int i = 0; i < count && i < 50; i++) {
        char size_buf[32];
        format_size(processes[i].memory_kb * 1024, size_buf, sizeof(size_buf));
        printf("%-7d %-30s %-10s %c\n",
               processes[i].pid,
               processes[i].name,
               size_buf,
               processes[i].state);
    }
    
    if (count > 50) {
        printf("... and %d more processes\n", count - 50);
    }
    printf("\n");
    printf("Total: %d processes\n", count);
    printf("\n");
}

static void cmd_select(const char *arg) {
    if (arg == NULL || *arg == '\0') {
        printf("Usage: select <pid>\n");
        return;
    }
    
    int pid = atoi(arg);
    if (pid <= 0) {
        printf("Invalid PID: %s\n", arg);
        return;
    }
    
    ProcessInfo info;
    if (get_process_info(pid, &info) != 0) {
        printf("Process %d not found\n", pid);
        return;
    }
    
    selected_pid = pid;
    strncpy(selected_name, info.name, sizeof(selected_name) - 1);
    
    /* Clear cached regions */
    if (cached_regions != NULL) {
        free(cached_regions);
        cached_regions = NULL;
        cached_region_count = 0;
    }
    
    printf("[OK] Selected process: %s (PID %d)\n", selected_name, selected_pid);
}

static void cmd_maps(void) {
    if (selected_pid < 0) {
        printf("No process selected. Use 'select <pid>' first.\n");
        return;
    }
    
    /* Allocate and cache regions */
    if (cached_regions == NULL) {
        cached_regions = (MemoryRegion*)malloc(MAX_REGIONS * sizeof(MemoryRegion));
    }
    
    cached_region_count = get_memory_regions(selected_pid, cached_regions, MAX_REGIONS);
    
    if (cached_region_count < 0) {
        printf("Failed to read memory regions for PID %d\n", selected_pid);
        return;
    }
    
    printf("\n");
    printf("MEMORY REGIONS FOR PID %d (%s)\n", selected_pid, selected_name);
    printf("================================================================================\n");
    printf("START ADDR       END ADDR         PERM  SIZE       TYPE        PATH\n");
    printf("--------------------------------------------------------------------------------\n");
    
    uint64_t total_size = 0;
    for (int i = 0; i < cached_region_count; i++) {
        MemoryRegion *r = &cached_regions[i];
        char size_buf[32];
        format_size(r->size, size_buf, sizeof(size_buf));
        
        printf("0x%012lx   0x%012lx   %s   %-10s %-10s  %s\n",
               r->start_addr,
               r->end_addr,
               r->permissions,
               size_buf,
               r->region_type,
               r->pathname);
        
        total_size += r->size;
    }
    
    char total_buf[32];
    format_size(total_size, total_buf, sizeof(total_buf));
    printf("================================================================================\n");
    printf("Total: %d regions, %s mapped\n", cached_region_count, total_buf);
    printf("\n");
}

static void cmd_translate(const char *arg) {
    if (selected_pid < 0) {
        printf("No process selected. Use 'select <pid>' first.\n");
        return;
    }
    
    if (arg == NULL || *arg == '\0') {
        printf("Usage: translate <virtual_address>\n");
        printf("Example: translate 0x7fff00010000\n");
        return;
    }
    
    uint64_t vaddr = parse_address(arg);
    PageWalkResult result;
    
    translate_address(selected_pid, vaddr, &result);
    print_translation(&result);
}

static void cmd_pagewalk(const char *arg) {
    if (selected_pid < 0) {
        printf("No process selected. Use 'select <pid>' first.\n");
        return;
    }
    
    if (arg == NULL || *arg == '\0') {
        printf("Usage: pagewalk <virtual_address>\n");
        printf("Example: pagewalk 0x7fff00010000\n");
        return;
    }
    
    uint64_t vaddr = parse_address(arg);
    PageWalkResult result;
    
    walk_page_table(selected_pid, vaddr, &result);
    print_page_walk(&result);
}

static void cmd_stats(void) {
    if (selected_pid < 0) {
        printf("No process selected. Use 'select <pid>' first.\n");
        return;
    }
    
    MemoryStats stats;
    if (get_memory_stats(selected_pid, &stats) != 0) {
        printf("Failed to get memory stats for PID %d\n", selected_pid);
        return;
    }
    
    char buf[32];
    
    printf("\n");
    printf("MEMORY STATISTICS FOR PID %d (%s)\n", selected_pid, selected_name);
    printf("==========================================\n");
    
    format_size(stats.vm_size, buf, sizeof(buf));
    printf("Virtual Size:    %s\n", buf);
    
    format_size(stats.vm_rss, buf, sizeof(buf));
    printf("Resident (RSS):  %s\n", buf);
    
    format_size(stats.vm_data, buf, sizeof(buf));
    printf("Data Segment:    %s\n", buf);
    
    format_size(stats.vm_stack, buf, sizeof(buf));
    printf("Stack:           %s\n", buf);
    
    format_size(stats.vm_exe, buf, sizeof(buf));
    printf("Executable:      %s\n", buf);
    
    format_size(stats.vm_lib, buf, sizeof(buf));
    printf("Shared Libs:     %s\n", buf);
    
    format_size(stats.vm_swap, buf, sizeof(buf));
    printf("Swapped:         %s\n", buf);
    
    printf("\n");
}

static void cmd_faults(void) {
    if (selected_pid < 0) {
        printf("No process selected. Use 'select <pid>' first.\n");
        return;
    }
    
    PageFaultStats stats;
    if (get_page_fault_stats(selected_pid, &stats) != 0) {
        printf("Failed to get page fault stats for PID %d\n", selected_pid);
        return;
    }
    
    printf("\n");
    printf("PAGE FAULT STATISTICS FOR PID %d (%s)\n", selected_pid, selected_name);
    printf("==========================================\n");
    printf("Minor Faults: %lu  (page in memory, just not mapped)\n", stats.minor_faults);
    printf("Major Faults: %lu  (page had to be read from disk)\n", stats.major_faults);
    printf("Total Faults: %lu\n", stats.total_faults);
    printf("\n");
}

static void cmd_tlb(const char *subcmd, const char *arg, const char *arg3) {
    if (subcmd == NULL || *subcmd == '\0') {
        printf("Usage: tlb <init|lookup|access|status|flush> [args]\n");
        return;
    }
    
    if (strcmp(subcmd, "init") == 0) {
        int size = 16;
        int policy = TLB_POLICY_LRU;
        
        if (arg != NULL && *arg != '\0') {
            size = atoi(arg);
            if (size <= 0 || size > 256) {
                printf("Invalid TLB size. Must be between 1 and 256.\n");
                return;
            }
        }
        
        /* Parse policy from arg3 */
        if (arg3 != NULL && *arg3 != '\0') {
            if (strcasecmp(arg3, "LRU") == 0) {
                policy = TLB_POLICY_LRU;
            } else if (strcasecmp(arg3, "FIFO") == 0) {
                policy = TLB_POLICY_FIFO;
            } else if (strcasecmp(arg3, "RANDOM") == 0) {
                policy = TLB_POLICY_RANDOM;
            } else if (strcasecmp(arg3, "CLOCK") == 0) {
                policy = TLB_POLICY_CLOCK;
            } else {
                printf("Unknown policy: %s. Using LRU.\n", arg3);
                printf("Available policies: LRU, FIFO, RANDOM, CLOCK\n");
            }
        }
        
        if (global_tlb != NULL) {
            tlb_free(global_tlb);
        }
        
        global_tlb = tlb_init(size, policy);
        if (global_tlb == NULL) {
            printf("Failed to initialize TLB\n");
            return;
        }
        
        printf("[OK] TLB initialized with %d entries (%s replacement)\n", size, tlb_policy_name(policy));
        
    } else if (strcmp(subcmd, "lookup") == 0 || strcmp(subcmd, "access") == 0) {
        if (global_tlb == NULL) {
            printf("TLB not initialized. Use 'tlb init' first.\n");
            return;
        }
        
        if (arg == NULL || *arg == '\0') {
            printf("Usage: tlb %s <address>\n", subcmd);
            return;
        }
        
        uint64_t vaddr = parse_address(arg);
        uint64_t vpn = get_vpn(vaddr);
        uint64_t found_pfn;
        
        int hit = tlb_lookup(global_tlb, vpn, &found_pfn);
        
        if (hit) {
            printf("[TLB HIT] VPN 0x%lx -> PFN 0x%lx\n", vpn, found_pfn);
        } else {
            printf("[TLB MISS] VPN 0x%lx not found\n", vpn);
            
            if (strcmp(subcmd, "access") == 0) {
                /* Simulate fetching from page table and inserting */
                PageWalkResult result;
                if (selected_pid > 0 && translate_address(selected_pid, vaddr, &result) == 0) {
                    tlb_insert(global_tlb, vpn, result.pte.pfn, 0);
                    printf("[TLB INSERT] VPN 0x%lx -> PFN 0x%lx\n", vpn, result.pte.pfn);
                } else {
                    /* Fake PFN for demo purposes */
                    uint64_t fake_pfn = vpn & 0xFFFFF;
                    tlb_insert(global_tlb, vpn, fake_pfn, 0);
                    printf("[TLB INSERT] VPN 0x%lx -> PFN 0x%lx (simulated)\n", vpn, fake_pfn);
                }
            }
        }
        
    } else if (strcmp(subcmd, "status") == 0) {
        if (global_tlb == NULL) {
            printf("TLB not initialized. Use 'tlb init' first.\n");
            return;
        }
        tlb_print(global_tlb);
        tlb_print_stats(global_tlb);
        
    } else if (strcmp(subcmd, "flush") == 0) {
        if (global_tlb == NULL) {
            printf("TLB not initialized.\n");
            return;
        }
        tlb_flush(global_tlb);
        printf("[OK] TLB flushed\n");
        
    } else {
        printf("Unknown TLB command: %s\n", subcmd);
        printf("Usage: tlb <init|lookup|access|status|flush> [args]\n");
    }
}

/* ============================================================================
 * Paging Simulation (Demand Paging)
 * ============================================================================ */

#define MAX_PAGING_FRAMES 64

typedef struct {
    int vpn;              /* Virtual page number stored in this frame (-1 = empty) */
    int loaded_at;        /* Access counter when page was loaded */
    int last_access;      /* Access counter when last accessed */
    int reference_bit;    /* For clock algorithm */
} PageFrame;

typedef struct {
    PageFrame frames[MAX_PAGING_FRAMES];
    int num_frames;
    int policy;           /* Reuse TLB_POLICY_* constants */
    int page_faults;
    int page_hits;
    int access_counter;
    int clock_hand;
} PagingSimulator;

static PagingSimulator *global_paging = NULL;

static void cmd_paging(const char *subcmd, char *arg) {
    if (subcmd == NULL || *subcmd == '\0') {
        printf("Usage: paging <init|access|status|flush> [args]\n");
        return;
    }
    
    if (strcmp(subcmd, "init") == 0) {
        int num_frames = 4;
        int policy = TLB_POLICY_LRU;
        
        if (arg != NULL && *arg != '\0') {
            /* Parse: <frames> [policy] */
            char policy_str[32] = "";
            sscanf(arg, "%d %31s", &num_frames, policy_str);
            
            if (num_frames < 1) num_frames = 4;
            if (num_frames > MAX_PAGING_FRAMES) num_frames = MAX_PAGING_FRAMES;
            
            if (strlen(policy_str) > 0) {
                if (strcasecmp(policy_str, "FIFO") == 0) policy = TLB_POLICY_FIFO;
                else if (strcasecmp(policy_str, "RANDOM") == 0) policy = TLB_POLICY_RANDOM;
                else if (strcasecmp(policy_str, "CLOCK") == 0) policy = TLB_POLICY_CLOCK;
            }
        }
        
        if (global_paging == NULL) {
            global_paging = (PagingSimulator *)malloc(sizeof(PagingSimulator));
        }
        
        global_paging->num_frames = num_frames;
        global_paging->policy = policy;
        global_paging->page_faults = 0;
        global_paging->page_hits = 0;
        global_paging->access_counter = 0;
        global_paging->clock_hand = 0;
        
        for (int i = 0; i < num_frames; i++) {
            global_paging->frames[i].vpn = -1;  /* Empty */
            global_paging->frames[i].loaded_at = 0;
            global_paging->frames[i].last_access = 0;
            global_paging->frames[i].reference_bit = 0;
        }
        
        printf("[OK] Paging simulator initialized with %d frames (%s replacement)\n", 
               num_frames, tlb_policy_name(policy));
        
    } else if (strcmp(subcmd, "access") == 0) {
        if (global_paging == NULL) {
            printf("Paging not initialized. Use 'paging init' first.\n");
            return;
        }
        
        if (arg == NULL || *arg == '\0') {
            printf("Usage: paging access <address>\n");
            return;
        }
        
        uint64_t vaddr = parse_address(arg);
        int vpn = (int)(vaddr >> 12);  /* Convert address to page number */
        
        /* Check if page is in memory */
        int frame_idx = -1;
        for (int i = 0; i < global_paging->num_frames; i++) {
            if (global_paging->frames[i].vpn == vpn) {
                frame_idx = i;
                break;
            }
        }
        
        if (frame_idx >= 0) {
            /* PAGE HIT */
            global_paging->page_hits++;
            global_paging->frames[frame_idx].last_access = global_paging->access_counter;
            global_paging->frames[frame_idx].reference_bit = 1;
            printf("[PAGE HIT] VPN 0x%x found in Frame %d\n", vpn, frame_idx);
        } else {
            /* PAGE FAULT */
            global_paging->page_faults++;
            
            /* Find free frame or evict */
            int target_frame = -1;
            
            /* First, look for empty frame */
            for (int i = 0; i < global_paging->num_frames; i++) {
                if (global_paging->frames[i].vpn == -1) {
                    target_frame = i;
                    break;
                }
            }
            
            if (target_frame < 0) {
                /* Need to evict - apply policy */
                int policy = global_paging->policy;
                
                if (policy == TLB_POLICY_LRU) {
                    int min_access = INT_MAX;
                    for (int i = 0; i < global_paging->num_frames; i++) {
                        if (global_paging->frames[i].last_access < min_access) {
                            min_access = global_paging->frames[i].last_access;
                            target_frame = i;
                        }
                    }
                } else if (policy == TLB_POLICY_FIFO) {
                    int min_loaded = INT_MAX;
                    for (int i = 0; i < global_paging->num_frames; i++) {
                        if (global_paging->frames[i].loaded_at < min_loaded) {
                            min_loaded = global_paging->frames[i].loaded_at;
                            target_frame = i;
                        }
                    }
                } else if (policy == TLB_POLICY_RANDOM) {
                    target_frame = rand() % global_paging->num_frames;
                } else if (policy == TLB_POLICY_CLOCK) {
                    while (1) {
                        if (!global_paging->frames[global_paging->clock_hand].reference_bit) {
                            target_frame = global_paging->clock_hand;
                            global_paging->clock_hand = (global_paging->clock_hand + 1) % global_paging->num_frames;
                            break;
                        }
                        global_paging->frames[global_paging->clock_hand].reference_bit = 0;
                        global_paging->clock_hand = (global_paging->clock_hand + 1) % global_paging->num_frames;
                    }
                }
                
                int evicted_vpn = global_paging->frames[target_frame].vpn;
                printf("[PAGE FAULT] VPN 0x%x not in memory, evicted VPN 0x%x from Frame %d\n", 
                       vpn, evicted_vpn, target_frame);
            } else {
                printf("[PAGE FAULT] VPN 0x%x not in memory, loaded into Frame %d\n", vpn, target_frame);
            }
            
            /* Load page into frame */
            global_paging->frames[target_frame].vpn = vpn;
            global_paging->frames[target_frame].loaded_at = global_paging->access_counter;
            global_paging->frames[target_frame].last_access = global_paging->access_counter;
            global_paging->frames[target_frame].reference_bit = 1;
        }
        
        global_paging->access_counter++;
        
    } else if (strcmp(subcmd, "status") == 0) {
        if (global_paging == NULL) {
            printf("Paging not initialized. Use 'paging init' first.\n");
            return;
        }
        
        printf("\n");
        printf("PAGING SIMULATOR STATUS\n");
        printf("=======================\n");
        printf("Frames: %d | Policy: %s\n", global_paging->num_frames, tlb_policy_name(global_paging->policy));
        printf("\n");
        printf("Physical Memory Frames:\n");
        printf("┌───────┬─────────┬─────────┬─────────────┐\n");
        printf("│ Frame │   VPN   │ Loaded  │ Last Access │\n");
        printf("├───────┼─────────┼─────────┼─────────────┤\n");
        
        for (int i = 0; i < global_paging->num_frames; i++) {
            if (global_paging->frames[i].vpn >= 0) {
                printf("│   %2d  │  0x%-4x │   %4d  │    %4d     │\n",
                       i, global_paging->frames[i].vpn,
                       global_paging->frames[i].loaded_at,
                       global_paging->frames[i].last_access);
            } else {
                printf("│   %2d  │  (empty) │    -    │      -      │\n", i);
            }
        }
        printf("└───────┴─────────┴─────────┴─────────────┘\n");
        printf("\n");
        
        int total = global_paging->page_hits + global_paging->page_faults;
        double hit_rate = total > 0 ? (double)global_paging->page_hits / total * 100 : 0;
        printf("Statistics:\n");
        printf("  Page Hits:   %d\n", global_paging->page_hits);
        printf("  Page Faults: %d\n", global_paging->page_faults);
        printf("  Hit Rate:    %.1f%%\n", hit_rate);
        printf("\n");
        
    } else if (strcmp(subcmd, "flush") == 0) {
        if (global_paging == NULL) {
            printf("Paging not initialized.\n");
            return;
        }
        
        for (int i = 0; i < global_paging->num_frames; i++) {
            global_paging->frames[i].vpn = -1;
        }
        global_paging->page_hits = 0;
        global_paging->page_faults = 0;
        global_paging->access_counter = 0;
        printf("[OK] Paging simulator flushed\n");
        
    } else {
        printf("Unknown paging command: %s\n", subcmd);
        printf("Usage: paging <init|access|status|flush> [args]\n");
    }
}

/* ============================================================================
 * Memory Playground (Active OS Interaction)
 * ============================================================================ */

#include <sys/mman.h>

#define MAX_PLAYGROUND_REGIONS 32

typedef struct {
    void *addr;           /* Mapped address */
    size_t size;          /* Size in bytes */
    int locked;           /* Is mlock'd? */
    int advice;           /* Current madvise hint */
    int active;           /* Still allocated? */
} PlaygroundRegion;

static PlaygroundRegion playground_regions[MAX_PLAYGROUND_REGIONS];
static int playground_count = 0;

static const char* madvise_name(int advice) {
    switch(advice) {
        case MADV_NORMAL: return "NORMAL";
        case MADV_RANDOM: return "RANDOM";
        case MADV_SEQUENTIAL: return "SEQUENTIAL";
        case MADV_WILLNEED: return "WILLNEED";
        case MADV_DONTNEED: return "DONTNEED";
        default: return "UNKNOWN";
    }
}

static void cmd_playground(const char *subcmd, char *arg) {
    if (subcmd == NULL || *subcmd == '\0') {
        printf("Usage: mem <alloc|lock|unlock|advise|free|status|reset> [args]\n");
        return;
    }
    
    if (strcmp(subcmd, "alloc") == 0) {
        int size_mb = 10;
        if (arg != NULL && *arg != '\0') {
            size_mb = atoi(arg);
            if (size_mb < 1) size_mb = 1;
            if (size_mb > 500) size_mb = 500;
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

static void cmd_sysinfo(void) {
    SystemMemInfo info;
    
    if (get_system_memory_info(&info) != 0) {
        printf("Failed to get system memory info\n");
        return;
    }
    
    char buf[32];
    
    printf("\n");
    printf("SYSTEM MEMORY INFORMATION\n");
    printf("=========================\n");
    
    format_size(info.total, buf, sizeof(buf));
    printf("Total Memory:     %s\n", buf);
    
    format_size(info.free, buf, sizeof(buf));
    printf("Free Memory:      %s\n", buf);
    
    format_size(info.available, buf, sizeof(buf));
    printf("Available:        %s\n", buf);
    
    format_size(info.buffers, buf, sizeof(buf));
    printf("Buffers:          %s\n", buf);
    
    format_size(info.cached, buf, sizeof(buf));
    printf("Cached:           %s\n", buf);
    
    format_size(info.active, buf, sizeof(buf));
    printf("Active:           %s\n", buf);
    
    format_size(info.inactive, buf, sizeof(buf));
    printf("Inactive:         %s\n", buf);
    
    printf("\n");
    
    format_size(info.swap_total, buf, sizeof(buf));
    printf("Swap Total:       %s\n", buf);
    
    format_size(info.swap_free, buf, sizeof(buf));
    printf("Swap Free:        %s\n", buf);
    
    printf("\n");
}

/* ============================================================================
 * Main Shell Loop
 * ============================================================================ */

static void run_shell(void) {
    char input[512];
    char cmd[64];
    char arg1[256];
    char arg2[256];
    
    print_banner();
    
    while (1) {
        /* Print prompt */
        if (selected_pid > 0) {
            printf("vmem[%d]> ", selected_pid);
        } else {
            printf("vmem> ");
        }
        fflush(stdout);
        
        /* Read input */
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\nGoodbye!\n");
            break;
        }
        
        trim(input);
        if (input[0] == '\0') continue;
        
        /* Parse command */
        char arg3[256];
        cmd[0] = '\0';
        arg1[0] = '\0';
        arg2[0] = '\0';
        arg3[0] = '\0';
        sscanf(input, "%63s %255s %255s %255s", cmd, arg1, arg2, arg3);
        
        /* Execute command */
        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0 || strcmp(cmd, "q") == 0) {
            printf("Goodbye!\n");
            break;
            
        } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
            print_help();
            
        } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
            printf("\033[2J\033[H");  /* ANSI escape to clear screen */
            
        } else if (strcmp(cmd, "ps") == 0) {
            cmd_ps();
            
        } else if (strcmp(cmd, "select") == 0) {
            cmd_select(arg1);
            
        } else if (strcmp(cmd, "maps") == 0) {
            cmd_maps();
            
        } else if (strcmp(cmd, "translate") == 0) {
            cmd_translate(arg1);
            
        } else if (strcmp(cmd, "pagewalk") == 0) {
            cmd_pagewalk(arg1);
            
        } else if (strcmp(cmd, "stats") == 0) {
            cmd_stats();
            
        } else if (strcmp(cmd, "faults") == 0) {
            cmd_faults();
            
        } else if (strcmp(cmd, "tlb") == 0) {
            cmd_tlb(arg1, arg2, arg3);
            
        } else if (strcmp(cmd, "paging") == 0) {
            cmd_paging(arg1, arg2);
            
        } else if (strcmp(cmd, "mem") == 0) {
            cmd_playground(arg1, arg2);
            
        } else if (strcmp(cmd, "sysinfo") == 0) {
            cmd_sysinfo();
            
        } else {
            printf("Unknown command: %s\n", cmd);
            printf("Type 'help' for available commands.\n");
        }
    }
}

/* ============================================================================
 * Main Entry Point
 * ============================================================================ */

int main(int argc, char *argv[]) {
    /* Check if running with specific command (for API use) */
    if (argc > 1) {
        /* Non-interactive mode for API */
        if (strcmp(argv[1], "--json") == 0 && argc > 2) {
            JsonBuffer *buf = json_buffer_init(0);
            
            if (strcmp(argv[2], "processes") == 0) {
                ProcessInfo processes[500];
                int count = get_process_list(processes, 500);
                if (count >= 0) {
                    json_process_list(processes, count, buf);
                } else {
                    json_error("Failed to read process list", buf);
                }
                
            } else if (strcmp(argv[2], "maps") == 0 && argc > 3) {
                int pid = atoi(argv[3]);
                MemoryRegion regions[MAX_REGIONS];
                int count = get_memory_regions(pid, regions, MAX_REGIONS);
                if (count >= 0) {
                    json_memory_regions(regions, count, buf);
                } else {
                    json_error("Failed to read memory regions", buf);
                }
                
            } else if (strcmp(argv[2], "translate") == 0 && argc > 4) {
                int pid = atoi(argv[3]);
                uint64_t vaddr = parse_address(argv[4]);
                PageWalkResult result;
                translate_address(pid, vaddr, &result);
                json_page_walk(&result, buf);
                
            } else if (strcmp(argv[2], "stats") == 0 && argc > 3) {
                int pid = atoi(argv[3]);
                MemoryStats stats;
                if (get_memory_stats(pid, &stats) == 0) {
                    json_memory_stats(&stats, buf);
                } else {
                    json_error("Failed to read memory stats", buf);
                }
                
            } else if (strcmp(argv[2], "sysinfo") == 0) {
                SystemMemInfo info;
                if (get_system_memory_info(&info) == 0) {
                    json_system_memory(&info, buf);
                } else {
                    json_error("Failed to read system memory info", buf);
                }
                
            } else {
                json_error("Unknown command", buf);
            }
            
            json_buffer_print(buf);
            json_buffer_free(buf);
            return 0;
        }
        
        /* Show help */
        printf("Usage: %s [--json <command> [args...]]\n", argv[0]);
        printf("\nInteractive mode: Run without arguments\n");
        printf("API mode: --json <command> [args...]\n");
        printf("\nAPI commands:\n");
        printf("  processes              List all processes\n");
        printf("  maps <pid>             Get memory regions for PID\n");
        printf("  translate <pid> <addr> Translate virtual address\n");
        printf("  stats <pid>            Get memory stats for PID\n");
        printf("  sysinfo                Get system memory info\n");
        return 0;
    }
    
    /* Interactive shell mode */
    run_shell();
    
    /* Cleanup */
    if (global_tlb != NULL) {
        tlb_free(global_tlb);
    }
    if (cached_regions != NULL) {
        free(cached_regions);
    }
    
    return 0;
}
