import { motion } from 'framer-motion'
import { BookOpen, Cpu, Database, ArrowRightLeft, AlertTriangle, Layers, HardDrive, RefreshCcw } from 'lucide-react'

function Learn() {
  const concepts = [
    {
      icon: <Layers size={24} />,
      title: 'Virtual Memory',
      color: 'var(--accent-blue)',
      description: 'Virtual memory is an abstraction that gives each process the illusion of having its own private, contiguous address space.',
      details: [
        'Each process has its own virtual address space (e.g., 0 to 2⁴⁸-1 on x86_64)',
        'Virtual addresses are mapped to physical addresses by the MMU',
        'Allows processes to be isolated from each other',
        'Enables memory larger than physical RAM through swapping',
        'Simplifies memory allocation for programs'
      ]
    },
    {
      icon: <ArrowRightLeft size={24} />,
      title: 'Address Translation',
      color: 'var(--accent-cyan)',
      description: 'The Memory Management Unit (MMU) translates virtual addresses to physical addresses using page tables.',
      details: [
        'Virtual Address = VPN (Virtual Page Number) + Page Offset',
        'Physical Address = PFN (Physical Frame Number) + Page Offset',
        'The page offset remains the same during translation',
        'Page size is typically 4KB (2¹² bytes) on x86',
        'The MMU performs this translation on every memory access'
      ]
    },
    {
      icon: <Cpu size={24} />,
      title: '4-Level Page Table (x86_64)',
      color: 'var(--accent-purple)',
      description: 'x86_64 uses a 4-level hierarchical page table structure to map 48-bit virtual addresses.',
      details: [
        'PML4 (Page Map Level 4): Bits 47-39, 512 entries',
        'PDPT (Page Directory Pointer Table): Bits 38-30, 512 entries',
        'PD (Page Directory): Bits 29-21, 512 entries',
        'PT (Page Table): Bits 20-12, 512 entries → PFN',
        'Page Offset: Bits 11-0, 4096 bytes per page'
      ]
    },
    {
      icon: <Database size={24} />,
      title: 'Translation Lookaside Buffer (TLB)',
      color: 'var(--accent-green)',
      description: 'The TLB is a hardware cache that stores recent virtual-to-physical address translations.',
      details: [
        'Avoids expensive page table walks for cached translations',
        'Typical size: 32-1024 entries',
        'TLB Hit: Translation found, fast path (~1 cycle)',
        'TLB Miss: Must walk page table, slow path (~10-100 cycles)',
        'TLB entries are invalidated on context switch (ASID helps)',
        'Replacement policies: LRU, FIFO, Random, Clock'
      ]
    },
    {
      icon: <HardDrive size={24} />,
      title: 'Demand Paging',
      color: 'var(--accent-yellow)',
      description: 'Pages are loaded into physical memory only when they are first accessed, not when the program starts.',
      details: [
        'Lazy loading: Only load pages that are actually needed',
        'Page Fault triggers loading: OS loads page from disk when accessed',
        'Benefits: Faster startup, lower memory usage',
        'Working Set: The set of pages actively used by a process',
        'Thrashing: When working set exceeds available memory',
        'Try it: Use the Demand Paging Simulator!'
      ]
    },
    {
      icon: <RefreshCcw size={24} />,
      title: 'Page Replacement Algorithms',
      color: 'var(--accent-pink)',
      description: 'When physical memory is full and a new page is needed, the OS must choose which page to evict.',
      details: [
        'LRU (Least Recently Used): Evict the page not used for the longest time',
        'FIFO (First In First Out): Evict the oldest loaded page',
        'Clock (Second Chance): LRU approximation using reference bit',
        'Random: Simple but unpredictable, rarely used in practice',
        'Optimal (Bélády\'s): Theoretical best - evict page used furthest in future',
        'Compare them: Try different policies in the simulators!'
      ]
    },
    {
      icon: <AlertTriangle size={24} />,
      title: 'Page Faults',
      color: 'var(--accent-orange)',
      description: 'A page fault occurs when a process accesses a page that is not currently in physical memory.',
      details: [
        'Minor Fault: Page is in memory but not mapped (quick fix)',
        'Major Fault: Page must be loaded from disk (slow, ~10ms)',
        'Invalid Fault: Segmentation fault, process is terminated',
        'Handled by the operating system kernel',
        'Demand paging: Pages loaded only when first accessed',
        'Copy-on-Write: Shared pages copied only when modified'
      ]
    },
    {
      icon: <BookOpen size={24} />,
      title: 'Memory Regions',
      color: 'var(--accent-red)',
      description: 'A process\'s virtual address space is divided into different regions with specific purposes.',
      details: [
        'Code (.text): Read-only executable code',
        'Data (.data, .bss): Global/static variables',
        'Heap: Dynamic memory (malloc, grows upward)',
        'Stack: Local variables, function calls (grows downward)',
        'Shared Libraries: Dynamically linked code',
        'Memory-mapped files: Files mapped directly into memory'
      ]
    }
  ]

  return (
    <div className="animate-slide-in">
      {/* Header */}
      <div style={{ marginBottom: 'var(--spacing-xl)' }}>
        <h1 style={{ fontSize: '1.75rem', fontWeight: '700', marginBottom: 'var(--spacing-sm)' }}>
          Learn OS Concepts
        </h1>
        <p style={{ color: 'var(--text-secondary)' }}>
          Understand the key concepts behind virtual memory and address translation
        </p>
      </div>

      {/* Concepts Grid */}
      <div style={{ display: 'grid', gap: 'var(--spacing-lg)' }}>
        {concepts.map((concept, index) => (
          <motion.div
            key={concept.title}
            initial={{ opacity: 0, y: 20 }}
            animate={{ opacity: 1, y: 0 }}
            transition={{ delay: index * 0.1 }}
            className="card"
          >
            <div style={{ display: 'flex', gap: 'var(--spacing-lg)' }}>
              {/* Icon */}
              <div style={{
                width: 56,
                height: 56,
                borderRadius: 'var(--radius-md)',
                background: `${concept.color}20`,
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'center',
                color: concept.color,
                flexShrink: 0
              }}>
                {concept.icon}
              </div>

              {/* Content */}
              <div style={{ flex: 1 }}>
                <h3 style={{ 
                  fontSize: '1.1rem', 
                  fontWeight: '600', 
                  marginBottom: 'var(--spacing-sm)',
                  color: concept.color
                }}>
                  {concept.title}
                </h3>
                
                <p style={{ 
                  color: 'var(--text-secondary)', 
                  marginBottom: 'var(--spacing-md)',
                  lineHeight: '1.6'
                }}>
                  {concept.description}
                </p>

                <ul style={{ 
                  listStyle: 'none',
                  display: 'grid',
                  gap: 'var(--spacing-xs)'
                }}>
                  {concept.details.map((detail, i) => (
                    <li 
                      key={i}
                      style={{
                        display: 'flex',
                        alignItems: 'flex-start',
                        gap: 'var(--spacing-sm)',
                        fontSize: '0.9rem',
                        color: 'var(--text-secondary)'
                      }}
                    >
                      <span style={{ 
                        color: concept.color, 
                        marginTop: '2px',
                        fontSize: '0.6rem'
                      }}>●</span>
                      <span style={{ fontFamily: detail.includes(':') ? 'var(--font-mono)' : 'inherit' }}>
                        {detail}
                      </span>
                    </li>
                  ))}
                </ul>
              </div>
            </div>
          </motion.div>
        ))}
      </div>

      {/* Quick Reference */}
      <motion.div
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        transition={{ delay: 0.6 }}
        className="card"
        style={{ marginTop: 'var(--spacing-xl)' }}
      >
        <h3 style={{ 
          fontSize: '1.1rem', 
          fontWeight: '600', 
          marginBottom: 'var(--spacing-lg)' 
        }}>
          Quick Reference: x86_64 Virtual Address
        </h3>
        
        <div style={{
          fontFamily: 'var(--font-mono)',
          background: 'var(--bg-tertiary)',
          padding: 'var(--spacing-lg)',
          borderRadius: 'var(--radius-md)',
          overflowX: 'auto'
        }}>
          <pre style={{ margin: 0, lineHeight: '1.8' }}>
{`┌─────────────────────────────────────────────────────────────────────────────┐
│                         48-bit Virtual Address                              │
├─────────────────────────────────────────────────────────────────────────────┤
│  63    48 │ 47    39 │ 38    30 │ 29    21 │ 20    12 │ 11           0 │
│  (unused) │   PML4   │   PDPT   │    PD    │    PT    │   Page Offset  │
│  16 bits  │  9 bits  │  9 bits  │  9 bits  │  9 bits  │    12 bits     │
└───────────┴──────────┴──────────┴──────────┴──────────┴────────────────┘

Translation Process:
  1. CR3 register points to PML4 table base address
  2. PML4[bits 47-39] → PDPT base address
  3. PDPT[bits 38-30] → PD base address
  4. PD[bits 29-21] → PT base address
  5. PT[bits 20-12] → Physical Frame Number (PFN)
  6. Physical Address = (PFN << 12) | Page Offset
`}
          </pre>
        </div>
      </motion.div>
    </div>
  )
}

export default Learn
