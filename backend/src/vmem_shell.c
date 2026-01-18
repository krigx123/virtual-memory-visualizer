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
#include "paging_sim.h"
#include "playground.h"

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
    printf("  unselect               Deselect current process\n");
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

uint64_t parse_address(const char *str) {
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

/* Comparison function for sorting processes by memory (descending) */
static int compare_proc_by_memory(const void *a, const void *b) {
    const ProcessInfo *pa = (const ProcessInfo *)a;
    const ProcessInfo *pb = (const ProcessInfo *)b;
    /* Sort descending by memory */
    if (pb->memory_kb > pa->memory_kb) return 1;
    if (pb->memory_kb < pa->memory_kb) return -1;
    return 0;
}

static void cmd_ps(void) {
    ProcessInfo processes[1000];
    int count = get_process_list(processes, 1000);
    
    if (count < 0) {
        printf("Failed to read process list\n");
        return;
    }
    
    /* Sort by memory usage (highest first) */
    qsort(processes, count, sizeof(ProcessInfo), compare_proc_by_memory);
    
    printf("\n");
    printf("PID     NAME                            MEMORY      STATE\n");
    printf("------------------------------------------------------------\n");
    
    for (int i = 0; i < count; i++) {
        char size_buf[32];
        format_size(processes[i].memory_kb * 1024, size_buf, sizeof(size_buf));
        printf("%-7d %-30s %-10s %c\n",
               processes[i].pid,
               processes[i].name,
               size_buf,
               processes[i].state);
    }
    
    printf("\n");
    printf("Total: %d processes (sorted by memory, high to low)\n", count);
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

static void cmd_unselect(void) {
    if (selected_pid < 0) {
        printf("No process selected\n");
        return;
    }
    
    printf("[OK] Deselected process %d (%s)\n", selected_pid, selected_name);
    selected_pid = -1;
    selected_name[0] = '\0';
    
    /* Clear cached regions */
    if (cached_regions != NULL) {
        free(cached_regions);
        cached_regions = NULL;
        cached_region_count = 0;
    }
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
        
        /* Educational hint for users confusing Address with VPN */
        if (vpn == 0 && vaddr > 0 && vaddr < 4096) {
            printf("[INFO] Note: Address 0x%lx maps to VPN 0x0 (Offset 0x%lx)\n", vaddr, vaddr);
            printf("       To access VPN 1, use address 0x1000 (4096)\n");
        }

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

/* NOTE: Paging simulator implementation moved to paging_sim.c/h */

/* ============================================================================
 * Memory Playground (Active OS Interaction)
 * ============================================================================ */

/* NOTE: Memory Playground implementation moved to playground.c/h */

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
            
        } else if (strcmp(cmd, "unselect") == 0) {
            cmd_unselect();
            
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
            /* Combine arg2 and arg3 for paging commands that need both (e.g., "4 FIFO") */
            char paging_arg[512];
            if (arg3[0] != '\0') {
                snprintf(paging_arg, sizeof(paging_arg), "%s %s", arg2, arg3);
            } else {
                snprintf(paging_arg, sizeof(paging_arg), "%s", arg2);
            }
            cmd_paging(arg1, paging_arg);
            
        } else if (strcmp(cmd, "mem") == 0) {
            /* Combine arg2 and arg3 for commands that need hints like "advise" */
            char mem_arg[512];
            if (arg3[0] != '\0') {
                snprintf(mem_arg, sizeof(mem_arg), "%s %s", arg2, arg3);
            } else {
                snprintf(mem_arg, sizeof(mem_arg), "%s", arg2);
            }
            cmd_playground(arg1, mem_arg);
            
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
                ProcessInfo processes[1000];
                int count = get_process_list(processes, 1000);
                if (count >= 0) {
                    /* Sort by memory usage (highest first) */
                    qsort(processes, count, sizeof(ProcessInfo), compare_proc_by_memory);
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
