/**
 * Virtual Memory Visualization Tool - Proc Reader Implementation
 * 
 * Reads process and memory information from Linux /proc filesystem.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "proc_reader.h"

/* ============================================================================
 * Process List Functions
 * ============================================================================ */

int get_process_list(ProcessInfo *processes, int max_count) {
    DIR *proc_dir;
    struct dirent *entry;
    int count = 0;
    
    proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("Failed to open /proc");
        return -1;
    }
    
    while ((entry = readdir(proc_dir)) != NULL && count < max_count) {
        /* Check if directory name is a number (PID) */
        int is_pid = 1;
        for (int i = 0; entry->d_name[i] != '\0'; i++) {
            if (!isdigit(entry->d_name[i])) {
                is_pid = 0;
                break;
            }
        }
        
        if (!is_pid) continue;
        
        int pid = atoi(entry->d_name);
        if (get_process_info(pid, &processes[count]) == 0) {
            count++;
        }
    }
    
    closedir(proc_dir);
    return count;
}

int get_process_info(int pid, ProcessInfo *info) {
    char path[64];
    char buffer[1024];
    FILE *fp;
    
    info->pid = pid;
    info->memory_kb = 0;
    info->state = '?';
    info->uid = -1;
    strcpy(info->name, "unknown");
    
    /* Read /proc/[pid]/status */
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strncmp(buffer, "Name:", 5) == 0) {
            sscanf(buffer + 6, "%255s", info->name);
        } else if (strncmp(buffer, "State:", 6) == 0) {
            sscanf(buffer + 7, " %c", &info->state);
        } else if (strncmp(buffer, "Uid:", 4) == 0) {
            sscanf(buffer + 5, "%d", &info->uid);
        } else if (strncmp(buffer, "VmRSS:", 6) == 0) {
            sscanf(buffer + 7, "%lu", &info->memory_kb);
        }
    }
    
    fclose(fp);
    return 0;
}

int process_exists(int pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d", pid);
    return access(path, F_OK) == 0;
}

/* ============================================================================
 * Memory Region Functions
 * ============================================================================ */

int get_memory_regions(int pid, MemoryRegion *regions, int max_count) {
    char path[64];
    char line[512];
    FILE *fp;
    int count = 0;
    
    snprintf(path, sizeof(path), "/proc/%d/maps", pid);
    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }
    
    while (fgets(line, sizeof(line), fp) != NULL && count < max_count) {
        MemoryRegion *region = &regions[count];
        char perms[5] = {0};
        char dev[12] = {0};
        char pathname[MAX_PATH_LEN] = {0};
        
        /* Parse line format:
         * start-end perms offset dev inode pathname
         * Example:
         * 00400000-00452000 r-xp 00000000 08:01 123456 /usr/bin/program
         */
        int matched = sscanf(line, "%lx-%lx %4s %lx %11s %lu %255[^\n]",
                            &region->start_addr,
                            &region->end_addr,
                            perms,
                            &region->offset,
                            dev,
                            &region->inode,
                            pathname);
        
        if (matched < 5) continue;
        
        strncpy(region->permissions, perms, sizeof(region->permissions) - 1);
        strncpy(region->device, dev, sizeof(region->device) - 1);
        
        /* Clean pathname (remove leading whitespace) */
        char *p = pathname;
        while (*p == ' ' || *p == '\t') p++;
        strncpy(region->pathname, p, MAX_PATH_LEN - 1);
        
        region->size = region->end_addr - region->start_addr;
        
        /* Interpret region type */
        interpret_region_type(region);
        
        count++;
    }
    
    fclose(fp);
    return count;
}

void interpret_region_type(MemoryRegion *region) {
    const char *path = region->pathname;
    
    if (strlen(path) == 0) {
        strcpy(region->region_type, "anonymous");
    } else if (strcmp(path, "[stack]") == 0) {
        strcpy(region->region_type, "stack");
    } else if (strcmp(path, "[heap]") == 0) {
        strcpy(region->region_type, "heap");
    } else if (strcmp(path, "[vdso]") == 0) {
        strcpy(region->region_type, "vdso");
    } else if (strcmp(path, "[vvar]") == 0) {
        strcpy(region->region_type, "vvar");
    } else if (strcmp(path, "[vsyscall]") == 0) {
        strcpy(region->region_type, "vsyscall");
    } else if (strncmp(path, "[stack:", 7) == 0) {
        strcpy(region->region_type, "thread_stack");
    } else if (strstr(path, ".so") != NULL) {
        /* Shared library */
        if (region->permissions[2] == 'x') {
            strcpy(region->region_type, "lib_code");
        } else if (region->permissions[1] == 'w') {
            strcpy(region->region_type, "lib_data");
        } else {
            strcpy(region->region_type, "lib_rodata");
        }
    } else if (path[0] == '/') {
        /* File-backed mapping */
        if (region->permissions[2] == 'x') {
            strcpy(region->region_type, "code");
        } else if (region->permissions[1] == 'w') {
            strcpy(region->region_type, "data");
        } else {
            strcpy(region->region_type, "rodata");
        }
    } else {
        strcpy(region->region_type, "other");
    }
}

MemoryRegion* find_region_for_address(MemoryRegion *regions, int region_count, 
                                       uint64_t addr) {
    for (int i = 0; i < region_count; i++) {
        if (addr >= regions[i].start_addr && addr < regions[i].end_addr) {
            return &regions[i];
        }
    }
    return NULL;
}

/* ============================================================================
 * Page Mapping Functions (from /proc/[pid]/pagemap)
 * ============================================================================ */

/* Pagemap entry format (64 bits):
 * Bits 0-54:  Page Frame Number (if present) or swap info
 * Bit 55:     Soft-dirty
 * Bit 56:     Exclusively mapped
 * Bits 57-60: Zero
 * Bit 61:     File/shared page
 * Bit 62:     Swapped
 * Bit 63:     Present
 */
#define PM_PRESENT     (1ULL << 63)
#define PM_SWAPPED     (1ULL << 62)
#define PM_FILE        (1ULL << 61)
#define PM_SOFT_DIRTY  (1ULL << 55)
#define PM_PFN_MASK    ((1ULL << 55) - 1)

int read_pagemap_entry(int pid, uint64_t vaddr, PageTableEntry *pte) {
    char path[64];
    int fd;
    uint64_t pagemap_entry;
    off_t offset;
    
    snprintf(path, sizeof(path), "/proc/%d/pagemap", pid);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    /* Calculate offset in pagemap file */
    /* Each entry is 8 bytes, index is virtual page number */
    offset = (vaddr / PAGE_SIZE) * sizeof(uint64_t);
    
    if (lseek(fd, offset, SEEK_SET) == -1) {
        close(fd);
        return -1;
    }
    
    if (read(fd, &pagemap_entry, sizeof(pagemap_entry)) != sizeof(pagemap_entry)) {
        close(fd);
        return -1;
    }
    
    close(fd);
    
    /* Parse pagemap entry */
    pte->vpn = vaddr >> PAGE_SHIFT;
    pte->present = (pagemap_entry & PM_PRESENT) ? 1 : 0;
    pte->swapped = (pagemap_entry & PM_SWAPPED) ? 1 : 0;
    
    if (pte->present) {
        pte->pfn = pagemap_entry & PM_PFN_MASK;
    } else if (pte->swapped) {
        pte->swap_offset = pagemap_entry & PM_PFN_MASK;
        pte->pfn = 0;
    } else {
        pte->pfn = 0;
    }
    
    /* Note: dirty/accessed bits are not available in pagemap */
    pte->dirty = 0;
    pte->accessed = 0;
    
    /* Get permissions from memory map (would need to cross-reference) */
    pte->writeable = 0;
    pte->executable = 0;
    pte->user = 1;
    
    return 0;
}

int get_physical_address(int pid, uint64_t vaddr, uint64_t *paddr) {
    PageTableEntry pte;
    
    if (read_pagemap_entry(pid, vaddr, &pte) != 0) {
        return -1;
    }
    
    if (!pte.present) {
        return -1;  /* Page not present */
    }
    
    /* Physical address = PFN * PAGE_SIZE + page offset */
    *paddr = (pte.pfn << PAGE_SHIFT) | (vaddr & PAGE_OFFSET_MASK);
    return 0;
}

/* ============================================================================
 * Memory Statistics Functions
 * ============================================================================ */

int get_memory_stats(int pid, MemoryStats *stats) {
    char path[64];
    char buffer[256];
    FILE *fp;
    
    memset(stats, 0, sizeof(MemoryStats));
    
    /* Read from /proc/[pid]/status */
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strncmp(buffer, "VmSize:", 7) == 0) {
            stats->vm_size = parse_size_string(buffer + 8);
        } else if (strncmp(buffer, "VmRSS:", 6) == 0) {
            stats->vm_rss = parse_size_string(buffer + 7);
        } else if (strncmp(buffer, "VmData:", 7) == 0) {
            stats->vm_data = parse_size_string(buffer + 8);
        } else if (strncmp(buffer, "VmStk:", 6) == 0) {
            stats->vm_stack = parse_size_string(buffer + 7);
        } else if (strncmp(buffer, "VmExe:", 6) == 0) {
            stats->vm_exe = parse_size_string(buffer + 7);
        } else if (strncmp(buffer, "VmLib:", 6) == 0) {
            stats->vm_lib = parse_size_string(buffer + 7);
        } else if (strncmp(buffer, "VmSwap:", 7) == 0) {
            stats->vm_swap = parse_size_string(buffer + 8);
        }
    }
    
    fclose(fp);
    
    /* Get page fault stats */
    get_page_fault_stats(pid, &stats->fault_stats);
    
    return 0;
}

int get_page_fault_stats(int pid, PageFaultStats *stats) {
    char path[64];
    char buffer[1024];
    FILE *fp;
    
    memset(stats, 0, sizeof(PageFaultStats));
    
    /* Read from /proc/[pid]/stat */
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (fp == NULL) {
        return -1;
    }
    
    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        /* Fields in /proc/[pid]/stat:
         * 10: minflt  - minor faults
         * 12: majflt  - major faults
         * We need to skip past the comm field which may contain spaces
         */
        char *p = strchr(buffer, ')');
        if (p != NULL) {
            unsigned long minflt, cminflt, majflt, cmajflt;
            /* Skip fields 3-9, read 10-13 */
            int matched = sscanf(p + 2, "%*c %*d %*d %*d %*d %*d %*u %lu %lu %lu %lu",
                                &minflt, &cminflt, &majflt, &cmajflt);
            if (matched >= 4) {
                stats->minor_faults = minflt;
                stats->major_faults = majflt;
                stats->total_faults = minflt + majflt;
            }
        }
    }
    
    fclose(fp);
    return 0;
}

int get_system_memory_info(SystemMemInfo *info) {
    FILE *fp;
    char buffer[256];
    char name[64];
    uint64_t value;
    
    memset(info, 0, sizeof(SystemMemInfo));
    
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        return -1;
    }
    
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (sscanf(buffer, "%63s %lu", name, &value) >= 2) {
            /* Convert from kB to bytes */
            value *= 1024;
            
            if (strcmp(name, "MemTotal:") == 0) {
                info->total = value;
            } else if (strcmp(name, "MemFree:") == 0) {
                info->free = value;
            } else if (strcmp(name, "MemAvailable:") == 0) {
                info->available = value;
            } else if (strcmp(name, "Buffers:") == 0) {
                info->buffers = value;
            } else if (strcmp(name, "Cached:") == 0) {
                info->cached = value;
            } else if (strcmp(name, "SwapTotal:") == 0) {
                info->swap_total = value;
            } else if (strcmp(name, "SwapFree:") == 0) {
                info->swap_free = value;
            } else if (strcmp(name, "Active:") == 0) {
                info->active = value;
            } else if (strcmp(name, "Inactive:") == 0) {
                info->inactive = value;
            }
        }
    }
    
    fclose(fp);
    return 0;
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

uint64_t parse_size_string(const char *str) {
    uint64_t value = 0;
    char unit[8] = {0};
    
    sscanf(str, "%lu %7s", &value, unit);
    
    /* Convert to bytes based on unit */
    if (strcasecmp(unit, "kB") == 0 || strcasecmp(unit, "KB") == 0) {
        value *= 1024;
    } else if (strcasecmp(unit, "MB") == 0) {
        value *= 1024 * 1024;
    } else if (strcasecmp(unit, "GB") == 0) {
        value *= 1024 * 1024 * 1024;
    }
    
    return value;
}

void format_size(uint64_t bytes, char *buf, size_t len) {
    if (bytes >= 1024ULL * 1024 * 1024) {
        snprintf(buf, len, "%.2f GB", (double)bytes / (1024 * 1024 * 1024));
    } else if (bytes >= 1024 * 1024) {
        snprintf(buf, len, "%.2f MB", (double)bytes / (1024 * 1024));
    } else if (bytes >= 1024) {
        snprintf(buf, len, "%.2f KB", (double)bytes / 1024);
    } else {
        snprintf(buf, len, "%lu B", bytes);
    }
}
