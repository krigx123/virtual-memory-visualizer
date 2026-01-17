/**
 * API utility functions for communicating with the Flask backend
 */

const API_BASE = '/api'

/**
 * Fetch wrapper with error handling
 */
async function fetchAPI(endpoint, options = {}) {
  try {
    const response = await fetch(`${API_BASE}${endpoint}`, {
      headers: {
        'Content-Type': 'application/json',
        ...options.headers
      },
      ...options
    })
    
    const data = await response.json()
    return data
  } catch (error) {
    console.error('API Error:', error)
    return { success: false, error: error.message }
  }
}

// =============================================================================
// Process APIs
// =============================================================================

export async function getProcesses() {
  return fetchAPI('/processes')
}

export async function getProcess(pid) {
  return fetchAPI(`/process/${pid}`)
}

export async function getMemoryMaps(pid) {
  return fetchAPI(`/process/${pid}/maps`)
}

export async function translateAddress(pid, address) {
  return fetchAPI(`/process/${pid}/translate/${address}`)
}

export async function getMemoryStats(pid) {
  return fetchAPI(`/process/${pid}/stats`)
}

// =============================================================================
// System APIs
// =============================================================================

export async function getSystemMemory() {
  return fetchAPI('/system/memory')
}

export async function healthCheck() {
  return fetchAPI('/health')
}

// =============================================================================
// TLB APIs
// =============================================================================

export async function initTLB(size = 16, policy = 'LRU') {
  return fetchAPI('/tlb/init', {
    method: 'POST',
    body: JSON.stringify({ size, policy })
  })
}

export async function tlbLookup(vpn) {
  return fetchAPI('/tlb/lookup', {
    method: 'POST',
    body: JSON.stringify({ vpn })
  })
}

export async function tlbInsert(vpn, pfn) {
  return fetchAPI('/tlb/insert', {
    method: 'POST',
    body: JSON.stringify({ vpn, pfn })
  })
}

export async function tlbAccess(vpn, pfn = null) {
  return fetchAPI('/tlb/access', {
    method: 'POST',
    body: JSON.stringify({ vpn, pfn })
  })
}

export async function getTLBStatus() {
  return fetchAPI('/tlb/status')
}

export async function flushTLB() {
  return fetchAPI('/tlb/flush', { method: 'POST' })
}

export async function resetTLBStats() {
  return fetchAPI('/tlb/reset', { method: 'POST' })
}

// =============================================================================
// Utility Functions
// =============================================================================

export function formatBytes(bytes) {
  if (bytes === 0) return '0 B'
  const k = 1024
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB']
  const i = Math.floor(Math.log(bytes) / Math.log(k))
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]
}

export function formatAddress(addr) {
  if (typeof addr === 'string') {
    return addr.startsWith('0x') ? addr : `0x${addr}`
  }
  return `0x${addr.toString(16).padStart(16, '0')}`
}

export function parseAddress(str) {
  if (!str) return 0
  const clean = str.trim()
  if (clean.startsWith('0x') || clean.startsWith('0X')) {
    return parseInt(clean, 16)
  }
  return parseInt(clean, 10)
}
