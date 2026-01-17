/**
 * Virtual Memory Visualization Tool - Address Translation Header
 * 
 * Functions for virtual-to-physical address translation and 
 * page table walk visualization.
 * 
 * Implements x86_64 4-level paging:
 *   PML4 -> PDPT -> Page Directory -> Page Table -> Physical Page
 */

#ifndef ADDR_TRANSLATE_H
#define ADDR_TRANSLATE_H

#include "vmem_types.h"

/* ============================================================================
 * Address Translation Functions
 * ============================================================================ */

/**
 * Translate virtual address to physical address
 * 
 * Uses /proc/[pid]/pagemap to get the actual physical address mapping.
 * 
 * @param pid     Process ID
 * @param vaddr   Virtual address to translate
 * @param result  Pointer to store translation result with full walk details
 * @return        0 on success, -1 on error
 */
int translate_address(int pid, uint64_t vaddr, PageWalkResult *result);

/**
 * Extract page table indices from virtual address
 * 
 * Breaks down a 64-bit virtual address into its components:
 *   - Bits 47-39: PML4 index    (9 bits, 512 entries)
 *   - Bits 38-30: PDPT index    (9 bits, 512 entries)
 *   - Bits 29-21: PD index      (9 bits, 512 entries)
 *   - Bits 20-12: PT index      (9 bits, 512 entries)
 *   - Bits 11-0:  Page offset   (12 bits, 4096 bytes per page)
 * 
 * @param vaddr       Virtual address
 * @param pml4_idx    Pointer to store PML4 index
 * @param pdpt_idx    Pointer to store PDPT index
 * @param pd_idx      Pointer to store Page Directory index
 * @param pt_idx      Pointer to store Page Table index
 * @param offset      Pointer to store page offset
 */
void extract_page_indices(uint64_t vaddr, 
                          int *pml4_idx, 
                          int *pdpt_idx, 
                          int *pd_idx, 
                          int *pt_idx, 
                          int *offset);

/**
 * Perform a simulated page table walk
 * 
 * This function simulates the multi-level page table walk that
 * the MMU performs, providing detailed information about each level.
 * 
 * @param pid     Process ID
 * @param vaddr   Virtual address to walk
 * @param result  Pointer to store walk result with all indices
 * @return        0 on success, -1 on error
 */
int walk_page_table(int pid, uint64_t vaddr, PageWalkResult *result);

/* ============================================================================
 * Address Manipulation Functions
 * ============================================================================ */

/**
 * Get Virtual Page Number from address
 * 
 * @param vaddr  Virtual address
 * @return       VPN (address >> 12)
 */
uint64_t get_vpn(uint64_t vaddr);

/**
 * Get page offset from address
 * 
 * @param vaddr  Virtual address
 * @return       Page offset (lower 12 bits)
 */
int get_page_offset(uint64_t vaddr);

/**
 * Construct physical address from PFN and offset
 * 
 * @param pfn     Physical Frame Number
 * @param offset  Page offset
 * @return        Full physical address
 */
uint64_t construct_physical_address(uint64_t pfn, int offset);

/* ============================================================================
 * Address Display Functions
 * ============================================================================ */

/**
 * Format address in binary showing bit groupings
 * 
 * @param vaddr  Virtual address
 * @param buf    Buffer to store formatted string
 * @param len    Buffer length
 */
void format_address_binary(uint64_t vaddr, char *buf, size_t len);

/**
 * Print page walk result to stdout (for CLI)
 * 
 * @param result  Page walk result to print
 */
void print_page_walk(const PageWalkResult *result);

/**
 * Print translation result summary
 * 
 * @param result  Page walk result to print
 */
void print_translation(const PageWalkResult *result);

#endif /* ADDR_TRANSLATE_H */
