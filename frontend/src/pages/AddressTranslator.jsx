import { useState, useEffect } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { 
  ArrowRightLeft, 
  Search,
  ChevronDown,
  CheckCircle,
  XCircle,
  Cpu
} from 'lucide-react'
import { getProcesses, translateAddress, formatBytes, parseAddress } from '../utils/api'

function AddressTranslator() {
  const [processes, setProcesses] = useState([])
  const [selectedPid, setSelectedPid] = useState('')
  const [virtualAddr, setVirtualAddr] = useState('')
  const [result, setResult] = useState(null)
  const [loading, setLoading] = useState(false)
  const [error, setError] = useState(null)

  useEffect(() => {
    loadProcesses()
  }, [])

  async function loadProcesses() {
    const result = await getProcesses()
    if (result.success) {
      setProcesses(result.data || [])
    }
  }

  async function handleTranslate() {
    if (!selectedPid || !virtualAddr) {
      setError('Please select a process and enter a virtual address')
      return
    }

    setLoading(true)
    setError(null)
    
    const res = await translateAddress(selectedPid, virtualAddr)
    
    if (res.success) {
      setResult(res.data)
    } else {
      setError(res.error || 'Translation failed')
      setResult(null)
    }
    
    setLoading(false)
  }

  // Convert virtual address to binary representation
  function addressToBinary(addr) {
    if (!addr) return null
    
    let numAddr
    if (typeof addr === 'string') {
      numAddr = addr.startsWith('0x') ? parseInt(addr, 16) : parseInt(addr)
    } else {
      numAddr = addr
    }
    
    if (isNaN(numAddr)) return null
    
    // Get 48-bit binary representation
    const binary = numAddr.toString(2).padStart(48, '0')
    
    return {
      pml4: binary.slice(0, 9),    // bits 47-39
      pdpt: binary.slice(9, 18),   // bits 38-30
      pd: binary.slice(18, 27),    // bits 29-21
      pt: binary.slice(27, 36),    // bits 20-12
      offset: binary.slice(36, 48) // bits 11-0
    }
  }

  const binaryAddr = result ? addressToBinary(result.virtual_addr) : null

  return (
    <div className="animate-slide-in">
      {/* Header */}
      <div style={{ marginBottom: 'var(--spacing-xl)' }}>
        <h1 style={{ fontSize: '1.75rem', fontWeight: '700', marginBottom: 'var(--spacing-sm)' }}>
          Address Translator
        </h1>
        <p style={{ color: 'var(--text-secondary)' }}>
          Translate virtual addresses to physical addresses with page table walk visualization
        </p>
      </div>

      <div className="translator-container">
        {/* Input Section */}
        <div className="card">
          <div className="card-header">
            <h3 className="card-title">
              <Search size={18} />
              Input
            </h3>
          </div>

          <div className="translator-input">
            <div className="input-group">
              <label>Select Process</label>
              <select 
                value={selectedPid} 
                onChange={(e) => setSelectedPid(e.target.value)}
              >
                <option value="">Choose a process...</option>
                {processes.slice(0, 50).map(proc => (
                  <option key={proc.pid} value={proc.pid}>
                    {proc.pid} - {proc.name} ({formatBytes(proc.memory_kb * 1024)})
                  </option>
                ))}
              </select>
            </div>

            <div className="input-group">
              <label>Virtual Address</label>
              <input
                type="text"
                placeholder="e.g., 0x7fff00010000"
                value={virtualAddr}
                onChange={(e) => setVirtualAddr(e.target.value)}
              />
              <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>
                Enter address in hex (0x...) or decimal
              </span>
            </div>

            <button 
              className="btn btn-primary"
              onClick={handleTranslate}
              disabled={loading}
              style={{ marginTop: 'var(--spacing-md)' }}
            >
              <ArrowRightLeft size={18} />
              {loading ? 'Translating...' : 'Translate Address'}
            </button>

            {error && (
              <div style={{ 
                padding: 'var(--spacing-md)', 
                background: 'rgba(239, 68, 68, 0.1)',
                border: '1px solid rgba(239, 68, 68, 0.3)',
                borderRadius: 'var(--radius-md)',
                color: 'var(--accent-red)',
                fontSize: '0.9rem'
              }}>
                {error}
              </div>
            )}
          </div>
        </div>

        {/* Result Section */}
        <div className="card">
          <div className="card-header">
            <h3 className="card-title">
              <ArrowRightLeft size={18} />
              Translation Result
            </h3>
          </div>

          <AnimatePresence mode="wait">
            {result ? (
              <motion.div
                key="result"
                initial={{ opacity: 0, y: 10 }}
                animate={{ opacity: 1, y: 0 }}
                exit={{ opacity: 0 }}
              >
                {/* Status */}
                <div style={{ 
                  display: 'flex', 
                  alignItems: 'center', 
                  gap: 'var(--spacing-sm)',
                  marginBottom: 'var(--spacing-lg)'
                }}>
                  {result.translation_success ? (
                    <>
                      <CheckCircle size={20} style={{ color: 'var(--accent-green)' }} />
                      <span style={{ color: 'var(--accent-green)', fontWeight: '600' }}>
                        Translation Successful
                      </span>
                    </>
                  ) : (
                    <>
                      <XCircle size={20} style={{ color: 'var(--accent-red)' }} />
                      <span style={{ color: 'var(--accent-red)', fontWeight: '600' }}>
                        Translation Failed: {result.error || 'Page not present'}
                      </span>
                    </>
                  )}
                </div>

                {/* Address Display */}
                <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 'var(--spacing-md)', marginBottom: 'var(--spacing-xl)' }}>
                  <div className="address-display">
                    <span className="label">Virtual Address</span>
                    <span className="value">{result.virtual_addr}</span>
                  </div>
                  <div className="address-display">
                    <span className="label">Physical Address</span>
                    <span className={`value ${result.translation_success ? 'success' : 'error'}`}>
                      {result.translation_success ? result.physical_addr : 'N/A'}
                    </span>
                  </div>
                </div>

                {/* Binary Address Breakdown */}
                {binaryAddr && (
                  <div style={{ marginBottom: 'var(--spacing-xl)' }}>
                    <h4 style={{ 
                      fontSize: '0.9rem', 
                      fontWeight: '600', 
                      marginBottom: 'var(--spacing-md)',
                      color: 'var(--text-secondary)'
                    }}>
                      Address Bit Breakdown (48-bit)
                    </h4>
                    <div className="binary-address">
                      <div className="bit-group pml4">
                        <div className="bits">{binaryAddr.pml4}</div>
                        <div className="label">PML4 [47:39]</div>
                      </div>
                      <div className="bit-group pdpt">
                        <div className="bits">{binaryAddr.pdpt}</div>
                        <div className="label">PDPT [38:30]</div>
                      </div>
                      <div className="bit-group pd">
                        <div className="bits">{binaryAddr.pd}</div>
                        <div className="label">PD [29:21]</div>
                      </div>
                      <div className="bit-group pt">
                        <div className="bits">{binaryAddr.pt}</div>
                        <div className="label">PT [20:12]</div>
                      </div>
                      <div className="bit-group offset">
                        <div className="bits">{binaryAddr.offset}</div>
                        <div className="label">Offset [11:0]</div>
                      </div>
                    </div>
                  </div>
                )}

                {/* Page Table Walk */}
                <div>
                  <h4 style={{ 
                    fontSize: '0.9rem', 
                    fontWeight: '600', 
                    marginBottom: 'var(--spacing-md)',
                    color: 'var(--text-secondary)'
                  }}>
                    4-Level Page Table Walk (x86_64)
                  </h4>
                  
                  <div className="page-walk">
                    <motion.div 
                      initial={{ opacity: 0, x: -20 }}
                      animate={{ opacity: 1, x: 0 }}
                      transition={{ delay: 0.1 }}
                      className="page-walk-level"
                    >
                      <span className="level-name">PML4</span>
                      <span className="level-index">Index: {result.pml4_index}</span>
                      <span className="level-bits">Bits 47-39 → Page Directory Pointer Table</span>
                    </motion.div>
                    
                    <div className="page-walk-arrow">
                      <ChevronDown size={20} />
                    </div>
                    
                    <motion.div 
                      initial={{ opacity: 0, x: -20 }}
                      animate={{ opacity: 1, x: 0 }}
                      transition={{ delay: 0.2 }}
                      className="page-walk-level"
                    >
                      <span className="level-name">PDPT</span>
                      <span className="level-index">Index: {result.pdpt_index}</span>
                      <span className="level-bits">Bits 38-30 → Page Directory</span>
                    </motion.div>
                    
                    <div className="page-walk-arrow">
                      <ChevronDown size={20} />
                    </div>
                    
                    <motion.div 
                      initial={{ opacity: 0, x: -20 }}
                      animate={{ opacity: 1, x: 0 }}
                      transition={{ delay: 0.3 }}
                      className="page-walk-level"
                    >
                      <span className="level-name">Page Dir</span>
                      <span className="level-index">Index: {result.pd_index}</span>
                      <span className="level-bits">Bits 29-21 → Page Table</span>
                    </motion.div>
                    
                    <div className="page-walk-arrow">
                      <ChevronDown size={20} />
                    </div>
                    
                    <motion.div 
                      initial={{ opacity: 0, x: -20 }}
                      animate={{ opacity: 1, x: 0 }}
                      transition={{ delay: 0.4 }}
                      className="page-walk-level"
                    >
                      <span className="level-name">Page Table</span>
                      <span className="level-index">Index: {result.pt_index}</span>
                      <span className="level-bits">Bits 20-12 → Physical Frame</span>
                    </motion.div>
                    
                    <div className="page-walk-arrow">
                      <ChevronDown size={20} />
                    </div>
                    
                    <motion.div 
                      initial={{ opacity: 0, x: -20 }}
                      animate={{ opacity: 1, x: 0 }}
                      transition={{ delay: 0.5 }}
                      className={`page-walk-level ${result.translation_success ? 'active' : ''}`}
                    >
                      <span className="level-name" style={{ color: result.translation_success ? 'var(--accent-green)' : 'var(--accent-red)' }}>
                        {result.translation_success ? 'Physical Frame' : 'Page Fault'}
                      </span>
                      {result.translation_success ? (
                        <>
                          <span className="level-index">PFN: {result.pfn}</span>
                          <span className="level-bits">+ Offset {result.page_offset} = Physical Address</span>
                        </>
                      ) : (
                        <span className="level-bits" style={{ color: 'var(--accent-red)' }}>
                          {result.error || 'Page not in memory'}
                        </span>
                      )}
                    </motion.div>
                  </div>
                </div>
              </motion.div>
            ) : (
              <motion.div
                key="empty"
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                style={{ 
                  display: 'flex', 
                  flexDirection: 'column',
                  alignItems: 'center', 
                  justifyContent: 'center',
                  minHeight: '300px',
                  color: 'var(--text-muted)'
                }}
              >
                <ArrowRightLeft size={48} style={{ marginBottom: 'var(--spacing-lg)', opacity: 0.3 }} />
                <p style={{ fontSize: '1rem' }}>Enter an address to translate</p>
                <p style={{ fontSize: '0.85rem', marginTop: 'var(--spacing-sm)' }}>
                  The result will show the full page table walk
                </p>
              </motion.div>
            )}
          </AnimatePresence>
        </div>
      </div>
    </div>
  )
}

export default AddressTranslator
