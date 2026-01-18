/**
 * Memory Playground
 * 
 * Active OS interaction with mmap, mlock, madvise.
 * Allows users to allocate real memory, lock it, and apply hints.
 */

#ifndef PLAYGROUND_H
#define PLAYGROUND_H

#include <stddef.h>

/* ============================================================================
 * Constants
 * ============================================================================ */

#define MAX_PLAYGROUND_REGIONS 32

/* ============================================================================
 * Data Structures
 * ============================================================================ */

/**
 * A single memory region allocated by the playground
 */
typedef struct {
    void *addr;           /* Mapped address */
    size_t size;          /* Size in bytes */
    int locked;           /* Is mlock'd? */
    int advice;           /* Current madvise hint */
    int active;           /* Still allocated? */
} PlaygroundRegion;

/* ============================================================================
 * Functions
 * ============================================================================ */

/**
 * Get the name of an madvise hint
 */
const char* madvise_name(int advice);

/**
 * CLI command handler for memory playground
 */
void cmd_playground(const char *subcmd, char *arg);

#endif /* PLAYGROUND_H */
