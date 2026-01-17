/**
 * Virtual Memory Visualization Tool - Address Translation Implementation
 * 
 * Implements virtual-to-physical address translation and page table walk
 * visualization for x86_64 4-level paging.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "addr_translate.h"
#include "proc_reader.h"

/* ============================================================================
 * Address Manipulation Functions
 * ============================================================================ */

uint64_t get_vpn(uint64_t vaddr) {
    return vaddr >> PAGE_SHIFT;
}

int get_page_offset(uint64_t vaddr) {
    return (int)(vaddr & PAGE_OFFSET_MASK);
}

uint64_t construct_physical_address(uint64_t pfn, int offset) {
    return (pfn << PAGE_SHIFT) | (offset & PAGE_OFFSET_MASK);
}

/* ============================================================================
 * Index Extraction
 * ============================================================================ */

void extract_page_indices(uint64_t vaddr, 
                          int *pml4_idx, 
                          int *pdpt_idx, 
                          int *pd_idx, 
                          int *pt_idx, 
                          int *offset) {
    /*
     * x86_64 Virtual Address Format (48-bit canonical):
     * 
     * 63       48 47    39 38    30 29    21 20    12 11        0
     * +---------+---------+---------+---------+---------+---------+
     * |  sign   |  PML4   |  PDPT   |   PD    |   PT    | Offset  |
     * | extend  |  index  |  index  |  index  |  index  |         |
     * +---------+---------+---------+---------+---------+---------+
     *   16 bits   9 bits    9 bits    9 bits    9 bits   12 bits
     */
    
    *pml4_idx = (vaddr >> PML4_SHIFT) & PT_INDEX_MASK;   /* Bits 47-39 */
    *pdpt_idx = (vaddr >> PDPT_SHIFT) & PT_INDEX_MASK;   /* Bits 38-30 */
    *pd_idx   = (vaddr >> PD_SHIFT) & PT_INDEX_MASK;     /* Bits 29-21 */
    *pt_idx   = (vaddr >> PT_SHIFT) & PT_INDEX_MASK;     /* Bits 20-12 */
    *offset   = vaddr & PAGE_OFFSET_MASK;                 /* Bits 11-0  */
}

/* ============================================================================
 * Page Table Walk
 * ============================================================================ */

int walk_page_table(int pid, uint64_t vaddr, PageWalkResult *result) {
    memset(result, 0, sizeof(PageWalkResult));
    result->virtual_addr = vaddr;
    
    /* Extract indices */
    extract_page_indices(vaddr,
                         &result->pml4_index,
                         &result->pdpt_index,
                         &result->pd_index,
                         &result->pt_index,
                         &result->page_offset);
    
    /* Read the actual pagemap entry */
    if (read_pagemap_entry(pid, vaddr, &result->pte) != 0) {
        result->success = 0;
        snprintf(result->error_msg, sizeof(result->error_msg),
                "Failed to read pagemap for PID %d (need root?)", pid);
        return -1;
    }
    
    /* Check if page is present */
    if (!result->pte.present) {
        result->success = 0;
        if (result->pte.swapped) {
            snprintf(result->error_msg, sizeof(result->error_msg),
                    "Page is swapped out (swap offset: 0x%lx)", result->pte.swap_offset);
        } else {
            snprintf(result->error_msg, sizeof(result->error_msg),
                    "Page not present (not yet accessed or demand paging)");
        }
        return -1;
    }
    
    /* Calculate physical address */
    result->physical_addr = construct_physical_address(result->pte.pfn, 
                                                        result->page_offset);
    result->success = 1;
    
    return 0;
}

int translate_address(int pid, uint64_t vaddr, PageWalkResult *result) {
    return walk_page_table(pid, vaddr, result);
}

/* ============================================================================
 * Display Functions
 * ============================================================================ */

void format_address_binary(uint64_t vaddr, char *buf, size_t len) {
    /* Format with spaces between bit groups */
    char temp[80];
    int pos = 0;
    
    /* Only show 48 bits (canonical address) */
    for (int i = 47; i >= 0; i--) {
        temp[pos++] = (vaddr & (1ULL << i)) ? '1' : '0';
        
        /* Add space after bit groups */
        if (i == 39 || i == 30 || i == 21 || i == 12) {
            temp[pos++] = ' ';
        }
    }
    temp[pos] = '\0';
    
    snprintf(buf, len, "%s", temp);
}

void print_translation(const PageWalkResult *result) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════════════════════════╗\n");
    printf("║                    ADDRESS TRANSLATION                            ║\n");
    printf("╠═══════════════════════════════════════════════════════════════════╣\n");
    printf("║  Virtual Address:  0x%016lx                       ║\n", result->virtual_addr);
    printf("║  ├─ VPN (Virtual Page Number): 0x%lx                       \n", 
           get_vpn(result->virtual_addr));
    printf("║  └─ Offset: 0x%03x                                                \n", 
           result->page_offset);
    printf("║                                                                   ║\n");
    
    if (result->success) {
        printf("║  Physical Address: 0x%016lx                       ║\n", 
               result->physical_addr);
        printf("║  ├─ PFN (Physical Frame Number): 0x%lx                     \n", 
               result->pte.pfn);
        printf("║  └─ Offset: 0x%03x                                                \n", 
               result->page_offset);
        printf("║                                                                   ║\n");
        printf("║  Page Properties:                                                 ║\n");
        printf("║  ├─ Present: YES                                                  ║\n");
        printf("║  ├─ Swapped: %s                                                   \n",
               result->pte.swapped ? "YES" : "NO");
    } else {
        printf("║  Physical Address: UNAVAILABLE                                    ║\n");
        printf("║  Error: %-55s  ║\n", result->error_msg);
    }
    printf("╚═══════════════════════════════════════════════════════════════════╝\n");
    printf("\n");
}

void print_page_walk(const PageWalkResult *result) {
    char binary_addr[80];
    format_address_binary(result->virtual_addr, binary_addr, sizeof(binary_addr));
    
    printf("\n");
    printf("PAGE TABLE WALK (4-Level Paging - x86_64)\n");
    printf("=========================================\n\n");
    
    printf("Virtual Address: 0x%016lx\n", result->virtual_addr);
    printf("Binary (48-bit): %s\n", binary_addr);
    printf("                 PML4      PDPT       PD        PT       Offset\n");
    printf("\n");
    
    printf("┌─────────────────────────────────────────────────────────────────────┐\n");
    printf("│ Bits 47-39: PML4 Index = %-3d                                       │\n", 
           result->pml4_index);
    printf("│     └──> PML4[%d] → PDPT                                            \n", 
           result->pml4_index);
    printf("├─────────────────────────────────────────────────────────────────────┤\n");
    printf("│ Bits 38-30: PDPT Index = %-3d                                       │\n", 
           result->pdpt_index);
    printf("│     └──> PDPT[%d] → Page Directory                                  \n", 
           result->pdpt_index);
    printf("├─────────────────────────────────────────────────────────────────────┤\n");
    printf("│ Bits 29-21: PD Index = %-3d                                         │\n", 
           result->pd_index);
    printf("│     └──> PD[%d] → Page Table                                        \n", 
           result->pd_index);
    printf("├─────────────────────────────────────────────────────────────────────┤\n");
    printf("│ Bits 20-12: PT Index = %-3d                                         │\n", 
           result->pt_index);
    
    if (result->success) {
        printf("│     └──> PT[%d] → Physical Frame 0x%lx                          \n", 
               result->pt_index, result->pte.pfn);
        printf("├─────────────────────────────────────────────────────────────────────┤\n");
        printf("│ Bits 11-0: Page Offset = 0x%03x (%d bytes)                         \n", 
               result->page_offset, result->page_offset);
        printf("└─────────────────────────────────────────────────────────────────────┘\n");
        printf("\n");
        printf("Physical Address = (PFN << 12) | Offset\n");
        printf("                = (0x%lx << 12) | 0x%03x\n", 
               result->pte.pfn, result->page_offset);
        printf("                = 0x%016lx ✓\n", result->physical_addr);
    } else {
        printf("│     └──> %s\n", result->error_msg);
        printf("└─────────────────────────────────────────────────────────────────────┘\n");
    }
    printf("\n");
}
