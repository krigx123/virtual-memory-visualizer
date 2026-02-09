/**
 * Virtual Memory Visualization Tool - JSON Output Implementation
 * 
 * Converts C data structures to JSON format for API communication.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "json_output.h"
#include "tlb_sim.h"

#define INITIAL_CAPACITY 4096
#define GROWTH_FACTOR 2

/* ============================================================================
 * JSON Buffer Management
 * ============================================================================ */

JsonBuffer* json_buffer_init(size_t initial_capacity) {
    JsonBuffer *buf = (JsonBuffer*)malloc(sizeof(JsonBuffer));
    if (buf == NULL) return NULL;
    
    if (initial_capacity == 0) {
        initial_capacity = INITIAL_CAPACITY;
    }
    
    buf->buffer = (char*)malloc(initial_capacity);
    if (buf->buffer == NULL) {
        free(buf);
        return NULL;
    }
    
    buf->buffer[0] = '\0';
    buf->size = 0;
    buf->capacity = initial_capacity;
    
    return buf;
}

void json_buffer_free(JsonBuffer *buf) {
    if (buf != NULL) {
        if (buf->buffer != NULL) {
            free(buf->buffer);
        }
        free(buf);
    }
}

const char* json_buffer_str(const JsonBuffer *buf) {
    return buf ? buf->buffer : NULL;
}

void json_buffer_print(const JsonBuffer *buf) {
    if (buf != NULL && buf->buffer != NULL) {
        printf("%s\n", buf->buffer);
    }
}

/**
 * Ensure buffer has enough capacity
 */
static int json_buffer_ensure(JsonBuffer *buf, size_t needed) {
    if (buf->size + needed >= buf->capacity) {
        size_t new_cap = buf->capacity * GROWTH_FACTOR;
        while (new_cap < buf->size + needed) {
            new_cap *= GROWTH_FACTOR;
        }
        
        char *new_buf = (char*)realloc(buf->buffer, new_cap);
        if (new_buf == NULL) return -1;
        
        buf->buffer = new_buf;
        buf->capacity = new_cap;
    }
    return 0;
}

/**
 * Append string to buffer
 */
static int json_append(JsonBuffer *buf, const char *str) {
    size_t len = strlen(str);
    if (json_buffer_ensure(buf, len + 1) != 0) return -1;
    
    strcpy(buf->buffer + buf->size, str);
    buf->size += len;
    return 0;
}

/**
 * Append formatted string
 */
static int json_appendf(JsonBuffer *buf, const char *fmt, ...) {
    char temp[1024];
    va_list args;
    
    va_start(args, fmt);
    vsnprintf(temp, sizeof(temp), fmt, args);
    va_end(args);
    
    return json_append(buf, temp);
}

/* ============================================================================
 * JSON Utility Functions
 * ============================================================================ */

int json_escape_string(const char *src, char *dest, size_t len) {
    size_t j = 0;
    
    for (size_t i = 0; src[i] != '\0' && j < len - 1; i++) {
        unsigned char c = (unsigned char)src[i];
        
        switch (c) {
            case '"':
                if (j + 2 >= len) goto truncated;
                dest[j++] = '\\';
                dest[j++] = '"';
                break;
            case '\\':
                if (j + 2 >= len) goto truncated;
                dest[j++] = '\\';
                dest[j++] = '\\';
                break;
            case '\n':
                if (j + 2 >= len) goto truncated;
                dest[j++] = '\\';
                dest[j++] = 'n';
                break;
            case '\r':
                if (j + 2 >= len) goto truncated;
                dest[j++] = '\\';
                dest[j++] = 'r';
                break;
            case '\t':
                if (j + 2 >= len) goto truncated;
                dest[j++] = '\\';
                dest[j++] = 't';
                break;
            case '\b':
                if (j + 2 >= len) goto truncated;
                dest[j++] = '\\';
                dest[j++] = 'b';
                break;
            case '\f':
                if (j + 2 >= len) goto truncated;
                dest[j++] = '\\';
                dest[j++] = 'f';
                break;
            default:
                if (c >= 32 && c < 127) {
                    /* Printable ASCII */
                    dest[j++] = c;
                } else if (c < 32) {
                    /* Control character: escape as \u00XX */
                    if (j + 6 >= len) goto truncated;
                    j += snprintf(dest + j, len - j, "\\u%04x", c);
                } else {
                    /* UTF-8 byte: pass through (valid in JSON strings) */
                    dest[j++] = c;
                }
                break;
        }
    }
    
    dest[j] = '\0';
    return 0;

truncated:
    dest[j] = '\0';
    return -1;
}

int json_error(const char *error_msg, JsonBuffer *buf) {
    char escaped[512];
    json_escape_string(error_msg, escaped, sizeof(escaped));
    
    json_append(buf, "{\"success\":false,\"error\":\"");
    json_append(buf, escaped);
    json_append(buf, "\"}");
    
    return 0;
}

int json_success(const char *message, JsonBuffer *buf) {
    char escaped[512];
    json_escape_string(message, escaped, sizeof(escaped));
    
    json_append(buf, "{\"success\":true,\"message\":\"");
    json_append(buf, escaped);
    json_append(buf, "\"}");
    
    return 0;
}

/* ============================================================================
 * JSON Conversion Functions
 * ============================================================================ */

int json_process_list(const ProcessInfo *processes, int count, JsonBuffer *buf) {
    json_append(buf, "{\"success\":true,\"data\":[");
    
    for (int i = 0; i < count; i++) {
        if (i > 0) json_append(buf, ",");
        
        char escaped_name[512];
        json_escape_string(processes[i].name, escaped_name, sizeof(escaped_name));
        
        json_appendf(buf, 
            "{\"pid\":%d,\"name\":\"%s\",\"state\":\"%c\",\"memory_kb\":%lu,\"uid\":%d}",
            processes[i].pid,
            escaped_name,
            processes[i].state,
            processes[i].memory_kb,
            processes[i].uid
        );
    }
    
    json_append(buf, "]}");
    return 0;
}

int json_memory_regions(const MemoryRegion *regions, int count, JsonBuffer *buf) {
    json_append(buf, "{\"success\":true,\"data\":[");
    
    for (int i = 0; i < count; i++) {
        if (i > 0) json_append(buf, ",");
        
        char escaped_path[512];
        char escaped_type[64];
        char escaped_perms[16];
        char escaped_device[32];
        json_escape_string(regions[i].pathname, escaped_path, sizeof(escaped_path));
        json_escape_string(regions[i].region_type, escaped_type, sizeof(escaped_type));
        json_escape_string(regions[i].permissions, escaped_perms, sizeof(escaped_perms));
        json_escape_string(regions[i].device, escaped_device, sizeof(escaped_device));
        
        json_appendf(buf,
            "{\"start_addr\":\"0x%lx\",\"end_addr\":\"0x%lx\","
            "\"permissions\":\"%s\",\"offset\":\"0x%lx\","
            "\"device\":\"%s\",\"inode\":%lu,"
            "\"pathname\":\"%s\",\"region_type\":\"%s\",\"size\":%lu}",
            regions[i].start_addr,
            regions[i].end_addr,
            escaped_perms,
            regions[i].offset,
            escaped_device,
            regions[i].inode,
            escaped_path,
            escaped_type,
            regions[i].size
        );
    }
    
    json_append(buf, "]}");
    return 0;
}

int json_page_walk(const PageWalkResult *result, JsonBuffer *buf) {
    json_append(buf, "{\"success\":true,\"data\":{");
    
    json_appendf(buf, "\"virtual_addr\":\"0x%lx\",", result->virtual_addr);
    json_appendf(buf, "\"pml4_index\":%d,", result->pml4_index);
    json_appendf(buf, "\"pdpt_index\":%d,", result->pdpt_index);
    json_appendf(buf, "\"pd_index\":%d,", result->pd_index);
    json_appendf(buf, "\"pt_index\":%d,", result->pt_index);
    json_appendf(buf, "\"page_offset\":%d,", result->page_offset);
    
    if (result->success) {
        json_appendf(buf, "\"physical_addr\":\"0x%lx\",", result->physical_addr);
        json_appendf(buf, "\"pfn\":\"0x%lx\",", result->pte.pfn);
        json_appendf(buf, "\"vpn\":\"0x%lx\",", result->pte.vpn);
        json_append(buf, "\"present\":true,");
        json_appendf(buf, "\"swapped\":%s,", result->pte.swapped ? "true" : "false");
        json_append(buf, "\"translation_success\":true");
    } else {
        json_append(buf, "\"physical_addr\":null,");
        json_append(buf, "\"pfn\":null,");
        json_append(buf, "\"present\":false,");
        json_append(buf, "\"translation_success\":false,");
        
        char escaped_error[256];
        json_escape_string(result->error_msg, escaped_error, sizeof(escaped_error));
        json_appendf(buf, "\"error\":\"%s\"", escaped_error);
    }
    
    json_append(buf, "}}");
    return 0;
}

int json_memory_stats(const MemoryStats *stats, JsonBuffer *buf) {
    json_append(buf, "{\"success\":true,\"data\":{");
    
    json_appendf(buf, "\"vm_size\":%lu,", stats->vm_size);
    json_appendf(buf, "\"vm_rss\":%lu,", stats->vm_rss);
    json_appendf(buf, "\"vm_data\":%lu,", stats->vm_data);
    json_appendf(buf, "\"vm_stack\":%lu,", stats->vm_stack);
    json_appendf(buf, "\"vm_exe\":%lu,", stats->vm_exe);
    json_appendf(buf, "\"vm_lib\":%lu,", stats->vm_lib);
    json_appendf(buf, "\"vm_swap\":%lu,", stats->vm_swap);
    json_appendf(buf, "\"shared_clean\":%lu,", stats->shared_clean);
    json_appendf(buf, "\"shared_dirty\":%lu,", stats->shared_dirty);
    json_appendf(buf, "\"private_clean\":%lu,", stats->private_clean);
    json_appendf(buf, "\"private_dirty\":%lu,", stats->private_dirty);
    json_appendf(buf, "\"referenced\":%lu,", stats->referenced);
    json_appendf(buf, "\"anonymous\":%lu,", stats->anonymous);
    
    json_append(buf, "\"faults\":{");
    json_appendf(buf, "\"minor\":%lu,", stats->fault_stats.minor_faults);
    json_appendf(buf, "\"major\":%lu,", stats->fault_stats.major_faults);
    json_appendf(buf, "\"total\":%lu", stats->fault_stats.total_faults);
    json_append(buf, "}");
    
    json_append(buf, "}}");
    return 0;
}

int json_tlb_state(const TLB *tlb, JsonBuffer *buf) {
    if (tlb == NULL) {
        json_error("TLB not initialized", buf);
        return -1;
    }
    
    json_append(buf, "{\"success\":true,\"data\":{");
    
    json_appendf(buf, "\"size\":%d,", tlb->size);
    json_appendf(buf, "\"policy\":\"%s\",", tlb_policy_name(tlb->replacement_policy));
    json_appendf(buf, "\"hits\":%lu,", tlb->hits);
    json_appendf(buf, "\"misses\":%lu,", tlb->misses);
    json_appendf(buf, "\"hit_rate\":%.2f,", tlb_get_hit_rate(tlb));
    
    json_append(buf, "\"entries\":[");
    
    for (int i = 0; i < tlb->size; i++) {
        if (i > 0) json_append(buf, ",");
        
        TLBEntry *e = &tlb->entries[i];
        json_appendf(buf,
            "{\"index\":%d,\"vpn\":\"0x%lx\",\"pfn\":\"0x%lx\","
            "\"valid\":%s,\"dirty\":%s,\"last_access\":%lu}",
            i, e->vpn, e->pfn,
            e->valid ? "true" : "false",
            e->dirty ? "true" : "false",
            e->last_access
        );
    }
    
    json_append(buf, "]}}");
    return 0;
}

int json_system_memory(const SystemMemInfo *info, JsonBuffer *buf) {
    json_append(buf, "{\"success\":true,\"data\":{");
    
    json_appendf(buf, "\"total\":%lu,", info->total);
    json_appendf(buf, "\"free\":%lu,", info->free);
    json_appendf(buf, "\"available\":%lu,", info->available);
    json_appendf(buf, "\"buffers\":%lu,", info->buffers);
    json_appendf(buf, "\"cached\":%lu,", info->cached);
    json_appendf(buf, "\"swap_total\":%lu,", info->swap_total);
    json_appendf(buf, "\"swap_free\":%lu,", info->swap_free);
    json_appendf(buf, "\"active\":%lu,", info->active);
    json_appendf(buf, "\"inactive\":%lu", info->inactive);
    
    json_append(buf, "}}");
    return 0;
}

int json_page_fault_stats(const PageFaultStats *stats, JsonBuffer *buf) {
    json_append(buf, "{\"success\":true,\"data\":{");
    
    json_appendf(buf, "\"minor\":%lu,", stats->minor_faults);
    json_appendf(buf, "\"major\":%lu,", stats->major_faults);
    json_appendf(buf, "\"total\":%lu", stats->total_faults);
    
    json_append(buf, "}}");
    return 0;
}
