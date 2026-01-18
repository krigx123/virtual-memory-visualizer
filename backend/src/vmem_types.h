/**
 * Virtual Memory Visualization Tool - Type Definitions
 * 
 * Core data structures for representing virtual memory concepts:
 * - Memory regions (from /proc/[pid]/maps)
 * - Page table entries
 * - TLB entries
 * - Page fault information
 * - Process information
 */

#ifndef VMEM_TYPES_H
#define VMEM_TYPES_H

#include <stdint.h>
#include <time.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define MAX_PATH_LEN 256
#define MAX_REGIONS 1024
#define MAX_PROCESSES 4096
#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

/* x86_64 4-level paging constants */
#define PML4_SHIFT 39
#define PDPT_SHIFT 30
#define PD_SHIFT 21
#define PT_SHIFT 12
#define PAGE_OFFSET_MASK 0xFFF
#define PT_INDEX_MASK 0x1FF

/* ============================================================================
 * Memory Region Structure (from /proc/[pid]/maps)
 * ============================================================================ */

typedef struct {
    uint64_t start_addr;        /* Start virtual address */
    uint64_t end_addr;          /* End virtual address */
    char permissions[5];        /* rwxp (read, write, execute, private/shared) */
    uint64_t offset;            /* Offset in file */
    char device[12];            /* Device (major:minor) */
    uint64_t inode;             /* Inode number */
    char pathname[MAX_PATH_LEN]; /* File path or special region name */
    char region_type[32];       /* Interpreted type: [stack], [heap], code, etc. */
    uint64_t size;              /* Size in bytes */
} MemoryRegion;

/* ============================================================================
 * Page Table Entry
 * ============================================================================ */

typedef struct {
    uint64_t vpn;               /* Virtual Page Number */
    uint64_t pfn;               /* Physical Frame Number */
    int present;                /* Page present in memory (1/0) */
    int dirty;                  /* Page has been modified (1/0) */
    int accessed;               /* Page has been accessed (1/0) */
    int writeable;              /* Page is writeable (1/0) */
    int executable;             /* Page is executable (1/0) */
    int user;                   /* User-mode accessible (1/0) */
    int swapped;                /* Page is swapped out (1/0) */
    uint64_t swap_offset;       /* Swap offset if swapped */
} PageTableEntry;

/* ============================================================================
 * Page Table Walk Result
 * ============================================================================ */

typedef struct {
    /* Input */
    uint64_t virtual_addr;
    
    /* Extracted indices */
    int pml4_index;             /* PML4 table index (bits 47-39) */
    int pdpt_index;             /* PDPT table index (bits 38-30) */
    int pd_index;               /* Page Directory index (bits 29-21) */
    int pt_index;               /* Page Table index (bits 20-12) */
    int page_offset;            /* Page offset (bits 11-0) */
    
    /* Result */
    uint64_t physical_addr;
    PageTableEntry pte;
    int success;                /* 1 if translation succeeded, 0 otherwise */
    char error_msg[128];        /* Error message if failed */
} PageWalkResult;

/* ============================================================================
 * TLB Entry and TLB Structure
 * ============================================================================ */

typedef struct {
    uint64_t vpn;               /* Virtual Page Number */
    uint64_t pfn;               /* Physical Frame Number */
    int valid;                  /* Entry is valid (1/0) */
    int dirty;                  /* Dirty bit */
    int accessed;               /* Accessed bit */
    uint64_t last_access;       /* Last access timestamp (for LRU) */
} TLBEntry;

typedef struct {
    TLBEntry *entries;          /* Array of TLB entries */
    int size;                   /* Number of entries */
    int replacement_policy;     /* 0=LRU, 1=FIFO, 2=Random, 3=Clock */
    int clock_hand;             /* Current position for Clock algorithm */
    
    /* Statistics */
    uint64_t hits;
    uint64_t misses;
    uint64_t access_counter;    /* For LRU tracking */
} TLB;

/* Replacement policies */
#define TLB_POLICY_LRU 0
#define TLB_POLICY_FIFO 1
#define TLB_POLICY_RANDOM 2
#define TLB_POLICY_CLOCK 3

/* ============================================================================
 * Page Fault Information
 * ============================================================================ */

typedef struct {
    uint64_t address;           /* Faulting address */
    char fault_type[16];        /* "minor" or "major" */
    time_t timestamp;           /* When fault occurred */
    int pid;                    /* Process ID */
    char region[MAX_PATH_LEN];  /* Region where fault occurred */
} PageFault;

typedef struct {
    uint64_t minor_faults;      /* Page in memory, just not mapped */
    uint64_t major_faults;      /* Page had to be read from disk */
    uint64_t total_faults;
} PageFaultStats;

/* ============================================================================
 * Memory Statistics
 * ============================================================================ */

typedef struct {
    /* From /proc/[pid]/status */
    uint64_t vm_size;           /* Total virtual memory size */
    uint64_t vm_rss;            /* Resident Set Size */
    uint64_t vm_data;           /* Data segment size */
    uint64_t vm_stack;          /* Stack size */
    uint64_t vm_exe;            /* Executable code size */
    uint64_t vm_lib;            /* Shared library size */
    uint64_t vm_swap;           /* Swapped-out memory */
    
    /* From /proc/[pid]/smaps (summary) */
    uint64_t shared_clean;
    uint64_t shared_dirty;
    uint64_t private_clean;
    uint64_t private_dirty;
    uint64_t referenced;
    uint64_t anonymous;
    
    /* Page fault stats */
    PageFaultStats fault_stats;
} MemoryStats;

/* ============================================================================
 * Process Information
 * ============================================================================ */

typedef struct {
    int pid;                    /* Process ID */
    char name[256];             /* Process name */
    char state;                 /* R=running, S=sleeping, etc. */
    uint64_t memory_kb;         /* Memory usage in KB */
    int uid;                    /* User ID */
} ProcessInfo;

/* ============================================================================
 * System Memory Information (from /proc/meminfo)
 * ============================================================================ */

typedef struct {
    uint64_t total;
    uint64_t free;
    uint64_t available;
    uint64_t buffers;
    uint64_t cached;
    uint64_t swap_total;
    uint64_t swap_free;
    uint64_t active;
    uint64_t inactive;
} SystemMemInfo;

/* ============================================================================
 * JSON Output Buffer
 * ============================================================================ */

typedef struct {
    char *buffer;
    size_t size;
    size_t capacity;
} JsonBuffer;

#endif /* VMEM_TYPES_H */
