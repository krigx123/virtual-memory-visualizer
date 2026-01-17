/**
 * Virtual Memory Visualization Tool - JSON Output Header
 * 
 * Functions for generating JSON output from C data structures.
 * Used by the CLI tool to output data that can be parsed by the Python API.
 */

#ifndef JSON_OUTPUT_H
#define JSON_OUTPUT_H

#include "vmem_types.h"

/* ============================================================================
 * JSON Buffer Management
 * ============================================================================ */

/**
 * Initialize a JSON output buffer
 * 
 * @param initial_capacity  Initial buffer size
 * @return                  New JsonBuffer, or NULL on error
 */
JsonBuffer* json_buffer_init(size_t initial_capacity);

/**
 * Free JSON buffer
 * 
 * @param buf  Buffer to free
 */
void json_buffer_free(JsonBuffer *buf);

/**
 * Get the JSON string from buffer
 * 
 * @param buf  Buffer to read
 * @return     Null-terminated JSON string
 */
const char* json_buffer_str(const JsonBuffer *buf);

/**
 * Print buffer contents to stdout
 * 
 * @param buf  Buffer to print
 */
void json_buffer_print(const JsonBuffer *buf);

/* ============================================================================
 * JSON Conversion Functions
 * ============================================================================ */

/**
 * Convert process list to JSON array
 * 
 * @param processes  Array of processes
 * @param count      Number of processes
 * @param buf        Output buffer
 * @return           0 on success, -1 on error
 */
int json_process_list(const ProcessInfo *processes, int count, JsonBuffer *buf);

/**
 * Convert memory regions to JSON array
 * 
 * @param regions  Array of memory regions
 * @param count    Number of regions
 * @param buf      Output buffer
 * @return         0 on success, -1 on error
 */
int json_memory_regions(const MemoryRegion *regions, int count, JsonBuffer *buf);

/**
 * Convert page walk result to JSON object
 * 
 * @param result  Page walk result
 * @param buf     Output buffer
 * @return        0 on success, -1 on error
 */
int json_page_walk(const PageWalkResult *result, JsonBuffer *buf);

/**
 * Convert memory stats to JSON object
 * 
 * @param stats  Memory statistics
 * @param buf    Output buffer
 * @return       0 on success, -1 on error
 */
int json_memory_stats(const MemoryStats *stats, JsonBuffer *buf);

/**
 * Convert TLB state to JSON object
 * 
 * @param tlb  TLB to convert
 * @param buf  Output buffer
 * @return     0 on success, -1 on error
 */
int json_tlb_state(const TLB *tlb, JsonBuffer *buf);

/**
 * Convert system memory info to JSON object
 * 
 * @param info  System memory info
 * @param buf   Output buffer
 * @return      0 on success, -1 on error
 */
int json_system_memory(const SystemMemInfo *info, JsonBuffer *buf);

/**
 * Convert page fault stats to JSON object
 * 
 * @param stats  Page fault statistics
 * @param buf    Output buffer
 * @return       0 on success, -1 on error
 */
int json_page_fault_stats(const PageFaultStats *stats, JsonBuffer *buf);

/* ============================================================================
 * JSON Utility Functions
 * ============================================================================ */

/**
 * Escape a string for JSON output
 * 
 * @param src   Source string
 * @param dest  Destination buffer
 * @param len   Destination buffer length
 * @return      0 on success, -1 if truncated
 */
int json_escape_string(const char *src, char *dest, size_t len);

/**
 * Create a simple error JSON response
 * 
 * @param error_msg  Error message
 * @param buf        Output buffer
 * @return           0 on success, -1 on error
 */
int json_error(const char *error_msg, JsonBuffer *buf);

/**
 * Create a simple success JSON response
 * 
 * @param message  Success message
 * @param buf      Output buffer
 * @return         0 on success, -1 on error
 */
int json_success(const char *message, JsonBuffer *buf);

#endif /* JSON_OUTPUT_H */
