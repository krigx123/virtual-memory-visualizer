/**
 * Virtual Memory Visualization Tool - Proc Reader Header
 * 
 * Functions for reading process information from the Linux /proc filesystem.
 * This module interfaces with:
 *   - /proc/[pid]/maps    : Memory region mappings
 *   - /proc/[pid]/smaps   : Detailed memory statistics
 *   - /proc/[pid]/pagemap : Virtual to physical address mappings
 *   - /proc/[pid]/status  : Process status and memory usage
 *   - /proc/meminfo       : System-wide memory information
 */

#ifndef PROC_READER_H
#define PROC_READER_H

#include "vmem_types.h"

/* ============================================================================
 * Process List Functions
 * ============================================================================ */

/**
 * Get list of all running processes
 * 
 * @param processes  Array to store process info (caller allocates)
 * @param max_count  Maximum number of processes to return
 * @return          Number of processes found, or -1 on error
 */
int get_process_list(ProcessInfo *processes, int max_count);

/**
 * Get information about a specific process
 * 
 * @param pid   Process ID
 * @param info  Pointer to store process info
 * @return      0 on success, -1 on error
 */
int get_process_info(int pid, ProcessInfo *info);

/**
 * Check if a process exists
 * 
 * @param pid  Process ID
 * @return     1 if exists, 0 if not
 */
int process_exists(int pid);

/* ============================================================================
 * Memory Region Functions (from /proc/[pid]/maps)
 * ============================================================================ */

/**
 * Get memory regions for a process
 * 
 * Parses /proc/[pid]/maps to get all virtual memory regions.
 * 
 * @param pid        Process ID
 * @param regions    Array to store regions (caller allocates)
 * @param max_count  Maximum number of regions to return
 * @return          Number of regions found, or -1 on error
 */
int get_memory_regions(int pid, MemoryRegion *regions, int max_count);

/**
 * Interpret region type from pathname
 * 
 * Determines if region is [stack], [heap], [vdso], code, library, etc.
 * 
 * @param region  Memory region to interpret (modifies region_type field)
 */
void interpret_region_type(MemoryRegion *region);

/**
 * Find region containing a specific address
 * 
 * @param regions      Array of memory regions
 * @param region_count Number of regions
 * @param addr         Virtual address to find
 * @return            Pointer to region, or NULL if not found
 */
MemoryRegion* find_region_for_address(MemoryRegion *regions, int region_count, 
                                       uint64_t addr);

/* ============================================================================
 * Page Mapping Functions (from /proc/[pid]/pagemap)
 * ============================================================================ */

/**
 * Read pagemap entry for a virtual address
 * 
 * Reads /proc/[pid]/pagemap to get the physical frame number (PFN)
 * for a given virtual address.
 * 
 * NOTE: Requires root privileges to read physical addresses.
 * 
 * @param pid    Process ID
 * @param vaddr  Virtual address
 * @param pte    Pointer to store page table entry info
 * @return       0 on success, -1 on error
 */
int read_pagemap_entry(int pid, uint64_t vaddr, PageTableEntry *pte);

/**
 * Get physical address for a virtual address
 * 
 * @param pid    Process ID  
 * @param vaddr  Virtual address
 * @param paddr  Pointer to store physical address
 * @return       0 on success, -1 on error (page not present or no permission)
 */
int get_physical_address(int pid, uint64_t vaddr, uint64_t *paddr);

/* ============================================================================
 * Memory Statistics Functions
 * ============================================================================ */

/**
 * Get memory statistics for a process
 * 
 * Reads from /proc/[pid]/status and /proc/[pid]/smaps
 * 
 * @param pid    Process ID
 * @param stats  Pointer to store statistics
 * @return       0 on success, -1 on error
 */
int get_memory_stats(int pid, MemoryStats *stats);

/**
 * Get page fault statistics for a process
 * 
 * @param pid    Process ID
 * @param stats  Pointer to store fault stats
 * @return       0 on success, -1 on error
 */
int get_page_fault_stats(int pid, PageFaultStats *stats);

/**
 * Get system-wide memory information
 * 
 * Reads from /proc/meminfo
 * 
 * @param info  Pointer to store system memory info
 * @return      0 on success, -1 on error
 */
int get_system_memory_info(SystemMemInfo *info);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

/**
 * Parse a size string with K/M/G suffix
 * 
 * @param str   String like "1234 kB"
 * @return      Size in bytes
 */
uint64_t parse_size_string(const char *str);

/**
 * Format size in human-readable format
 * 
 * @param bytes  Size in bytes
 * @param buf    Buffer to store result
 * @param len    Buffer length
 */
void format_size(uint64_t bytes, char *buf, size_t len);

#endif /* PROC_READER_H */
