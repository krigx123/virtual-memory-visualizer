import { useState, useEffect } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { 
  Database, 
  Play, 
  RotateCcw, 
  Trash2, 
  CheckCircle,
  XCircle,
  TrendingUp,
  Activity
} from 'lucide-react'
import { 
  initTLB, 
  tlbAccess, 
  getTLBStatus, 
  flushTLB, 
  resetTLBStats,
  parseAddress 
} from '../utils/api'

function TLBSimulator() {
  const [tlbSize, setTlbSize] = useState(16)
  const [tlbInitialized, setTlbInitialized] = useState(false)
  const [tlbData, setTlbData] = useState(null)
  const [addressInput, setAddressInput] = useState('')
  const [pfnInput, setPfnInput] = useState('')
  const [lastAccess, setLastAccess] = useState(null)
  const [accessHistory, setAccessHistory] = useState([])
  const [loading, setLoading] = useState(false)

  useEffect(() => {
    checkTLBStatus()
  }, [])

  async function checkTLBStatus() {
    const result = await getTLBStatus()
    if (result.success) {
      setTlbInitialized(true)
      setTlbData(result.data)
    }
  }

  async function handleInit() {
    setLoading(true)
    const result = await initTLB(tlbSize, 'LRU')
    if (result.success) {
      setTlbInitialized(true)
      setAccessHistory([])
      await checkTLBStatus()
    }
    setLoading(false)
  }

  async function handleAccess() {
    if (!addressInput) return
    
    setLoading(true)
    
    const vpn = parseAddress(addressInput) >> 12 // Get VPN from address
    const pfn = pfnInput ? parseAddress(pfnInput) : vpn & 0xFFFFF // Use input PFN or generate fake one
    
    const result = await tlbAccess(vpn, pfn)
    
    if (result.success) {
      setLastAccess({
        vpn: `0x${vpn.toString(16)}`,
        hit: result.hit,
        pfn: result.hit ? result.pfn : `0x${pfn.toString(16)}`,
        timestamp: new Date().toLocaleTimeString()
      })
      
      setAccessHistory(prev => [{
        vpn: `0x${vpn.toString(16)}`,
        hit: result.hit,
        timestamp: new Date().toLocaleTimeString()
      }, ...prev].slice(0, 20))
      
      await checkTLBStatus()
    }
    
    setAddressInput('')
    setLoading(false)
  }

  async function handleFlush() {
    const result = await flushTLB()
    if (result.success) {
      await checkTLBStatus()
      setLastAccess(null)
    }
  }

  async function handleResetStats() {
    const result = await resetTLBStats()
    if (result.success) {
      await checkTLBStatus()
      setAccessHistory([])
    }
  }

  return (
    <div className="animate-slide-in">
      {/* Header */}
      <div style={{ marginBottom: 'var(--spacing-xl)' }}>
        <h1 style={{ fontSize: '1.75rem', fontWeight: '700', marginBottom: 'var(--spacing-sm)' }}>
          TLB Simulator
        </h1>
        <p style={{ color: 'var(--text-secondary)' }}>
          Simulate Translation Lookaside Buffer behavior with LRU replacement policy
        </p>
      </div>

      {!tlbInitialized ? (
        /* Init Card */
        <motion.div 
          initial={{ opacity: 0, scale: 0.95 }}
          animate={{ opacity: 1, scale: 1 }}
          className="card"
          style={{ maxWidth: '500px' }}
        >
          <div className="card-header">
            <h3 className="card-title">
              <Database size={18} />
              Initialize TLB
            </h3>
          </div>
          
          <div className="input-group" style={{ marginBottom: 'var(--spacing-lg)' }}>
            <label>TLB Size (entries)</label>
            <input
              type="number"
              min="4"
              max="64"
              value={tlbSize}
              onChange={(e) => setTlbSize(parseInt(e.target.value) || 16)}
            />
            <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>
              Recommended: 8, 16, or 32 entries
            </span>
          </div>
          
          <button 
            className="btn btn-primary"
            onClick={handleInit}
            disabled={loading}
          >
            <Play size={18} />
            Initialize TLB
          </button>
        </motion.div>
      ) : (
        <div className="tlb-container">
          {/* TLB Table */}
          <div className="card">
            <div className="card-header">
              <h3 className="card-title">
                <Database size={18} />
                TLB Entries
              </h3>
              <div style={{ display: 'flex', gap: 'var(--spacing-sm)' }}>
                <button className="btn btn-secondary" onClick={handleFlush} style={{ padding: '0.4rem 0.8rem' }}>
                  <Trash2 size={14} />
                  Flush
                </button>
                <button className="btn btn-secondary" onClick={handleResetStats} style={{ padding: '0.4rem 0.8rem' }}>
                  <RotateCcw size={14} />
                  Reset Stats
                </button>
              </div>
            </div>

            {/* Last Access Indicator */}
            <AnimatePresence>
              {lastAccess && (
                <motion.div
                  initial={{ opacity: 0, height: 0 }}
                  animate={{ opacity: 1, height: 'auto' }}
                  exit={{ opacity: 0, height: 0 }}
                  className={`hit-indicator ${lastAccess.hit ? 'hit' : 'miss'}`}
                  style={{ marginBottom: 'var(--spacing-md)' }}
                >
                  {lastAccess.hit ? (
                    <>
                      <CheckCircle size={16} />
                      TLB HIT: VPN {lastAccess.vpn} → PFN {lastAccess.pfn}
                    </>
                  ) : (
                    <>
                      <XCircle size={16} />
                      TLB MISS: VPN {lastAccess.vpn} (inserted PFN {lastAccess.pfn})
                    </>
                  )}
                </motion.div>
              )}
            </AnimatePresence>

            {/* TLB Table */}
            <div style={{ overflowX: 'auto' }}>
              <table className="tlb-table">
                <thead>
                  <tr>
                    <th>Index</th>
                    <th>VPN</th>
                    <th>PFN</th>
                    <th>Valid</th>
                    <th>Last Access</th>
                  </tr>
                </thead>
                <tbody>
                  {tlbData?.entries?.map((entry, index) => (
                    <motion.tr
                      key={index}
                      initial={false}
                      animate={{
                        backgroundColor: lastAccess && entry.vpn === lastAccess.vpn 
                          ? 'rgba(59, 130, 246, 0.15)' 
                          : 'var(--bg-card)'
                      }}
                    >
                      <td>{entry.index}</td>
                      <td style={{ color: entry.valid ? 'var(--accent-cyan)' : 'var(--text-muted)' }}>
                        {entry.valid ? entry.vpn : '-'}
                      </td>
                      <td style={{ color: entry.valid ? 'var(--accent-green)' : 'var(--text-muted)' }}>
                        {entry.valid ? entry.pfn : '-'}
                      </td>
                      <td className={entry.valid ? 'valid' : 'invalid'}>
                        {entry.valid ? '✓' : '✗'}
                      </td>
                      <td style={{ color: 'var(--text-muted)' }}>
                        {entry.valid ? entry.last_access : '-'}
                      </td>
                    </motion.tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>

          {/* Controls and Stats */}
          <div className="tlb-stats">
            {/* Access Form */}
            <div className="card">
              <div className="card-header">
                <h3 className="card-title">
                  <Activity size={18} />
                  Access TLB
                </h3>
              </div>
              
              <div className="input-group" style={{ marginBottom: 'var(--spacing-md)' }}>
                <label>Virtual Address</label>
                <input
                  type="text"
                  placeholder="e.g., 0x7fff0000"
                  value={addressInput}
                  onChange={(e) => setAddressInput(e.target.value)}
                  onKeyDown={(e) => e.key === 'Enter' && handleAccess()}
                />
              </div>
              
              <div className="input-group" style={{ marginBottom: 'var(--spacing-md)' }}>
                <label>PFN (optional, for miss insertion)</label>
                <input
                  type="text"
                  placeholder="Auto-generated if empty"
                  value={pfnInput}
                  onChange={(e) => setPfnInput(e.target.value)}
                />
              </div>
              
              <button 
                className="btn btn-primary w-full"
                onClick={handleAccess}
                disabled={loading || !addressInput}
              >
                <Play size={18} />
                Access Address
              </button>
            </div>

            {/* Statistics */}
            <div className="card">
              <div className="card-header">
                <h3 className="card-title">
                  <TrendingUp size={18} />
                  Statistics
                </h3>
              </div>
              
              <div style={{ display: 'grid', gap: 'var(--spacing-md)' }}>
                <div className="stat-card" style={{ padding: 'var(--spacing-md)' }}>
                  <div className="stat-label">Hit Rate</div>
                  <div className="stat-value" style={{ fontSize: '1.5rem' }}>
                    {tlbData?.hit_rate?.toFixed(1) || 0}%
                  </div>
                </div>
                
                <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 'var(--spacing-md)' }}>
                  <div style={{ 
                    padding: 'var(--spacing-md)', 
                    background: 'rgba(16, 185, 129, 0.1)',
                    borderRadius: 'var(--radius-md)',
                    textAlign: 'center'
                  }}>
                    <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Hits</div>
                    <div style={{ 
                      fontSize: '1.2rem', 
                      fontWeight: '700', 
                      color: 'var(--accent-green)',
                      fontFamily: 'var(--font-mono)'
                    }}>
                      {tlbData?.hits || 0}
                    </div>
                  </div>
                  
                  <div style={{ 
                    padding: 'var(--spacing-md)', 
                    background: 'rgba(239, 68, 68, 0.1)',
                    borderRadius: 'var(--radius-md)',
                    textAlign: 'center'
                  }}>
                    <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Misses</div>
                    <div style={{ 
                      fontSize: '1.2rem', 
                      fontWeight: '700', 
                      color: 'var(--accent-red)',
                      fontFamily: 'var(--font-mono)'
                    }}>
                      {tlbData?.misses || 0}
                    </div>
                  </div>
                </div>
                
                <div style={{ fontSize: '0.8rem', color: 'var(--text-muted)' }}>
                  Policy: {tlbData?.policy || 'LRU'} | Size: {tlbData?.size || 0} entries
                </div>
              </div>
            </div>

            {/* Access History */}
            <div className="card" style={{ flex: 1 }}>
              <div className="card-header">
                <h3 className="card-title">Access History</h3>
              </div>
              
              <div style={{ 
                maxHeight: '200px', 
                overflowY: 'auto',
                display: 'flex',
                flexDirection: 'column',
                gap: '4px'
              }}>
                {accessHistory.length > 0 ? (
                  accessHistory.map((access, index) => (
                    <div
                      key={index}
                      style={{
                        display: 'flex',
                        alignItems: 'center',
                        gap: 'var(--spacing-sm)',
                        padding: 'var(--spacing-xs) var(--spacing-sm)',
                        background: 'var(--bg-tertiary)',
                        borderRadius: 'var(--radius-sm)',
                        fontSize: '0.8rem',
                        fontFamily: 'var(--font-mono)'
                      }}
                    >
                      <span style={{ 
                        color: access.hit ? 'var(--accent-green)' : 'var(--accent-red)',
                        fontWeight: '600'
                      }}>
                        {access.hit ? 'HIT' : 'MISS'}
                      </span>
                      <span style={{ color: 'var(--text-secondary)' }}>{access.vpn}</span>
                      <span style={{ marginLeft: 'auto', color: 'var(--text-muted)', fontSize: '0.7rem' }}>
                        {access.timestamp}
                      </span>
                    </div>
                  ))
                ) : (
                  <div style={{ color: 'var(--text-muted)', fontSize: '0.85rem', textAlign: 'center', padding: 'var(--spacing-md)' }}>
                    No accesses yet
                  </div>
                )}
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  )
}

export default TLBSimulator
