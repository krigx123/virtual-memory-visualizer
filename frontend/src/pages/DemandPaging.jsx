import { useState, useEffect } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { 
  HardDrive, 
  Play, 
  RotateCcw, 
  Zap,
  AlertTriangle,
  CheckCircle,
  XCircle,
  TrendingUp,
  List,
  Settings
} from 'lucide-react'
import { 
  initPaging, 
  pagingAccess, 
  getPagingStatus, 
  resetPagingStats,
  runPagingSequence,
  parseAddress 
} from '../utils/api'

function DemandPaging() {
  const [numFrames, setNumFrames] = useState(4)
  const [policy, setPolicy] = useState('LRU')
  const [initialized, setInitialized] = useState(false)
  const [pagingData, setPagingData] = useState(null)
  const [addressInput, setAddressInput] = useState('')
  const [sequenceInput, setSequenceInput] = useState('')
  const [lastAccess, setLastAccess] = useState(null)
  const [loading, setLoading] = useState(false)
  const [animatingFrame, setAnimatingFrame] = useState(null)

  const policies = [
    { value: 'LRU', label: 'LRU (Least Recently Used)', desc: 'Evicts the page that was accessed longest ago' },
    { value: 'FIFO', label: 'FIFO (First In First Out)', desc: 'Evicts the oldest loaded page' },
    { value: 'RANDOM', label: 'Random', desc: 'Randomly selects a victim page' },
    { value: 'CLOCK', label: 'Clock (Second Chance)', desc: 'Uses reference bit to give pages a second chance' }
  ]

  useEffect(() => {
    checkStatus()
  }, [])

  async function checkStatus() {
    const result = await getPagingStatus()
    if (result.success) {
      setInitialized(true)
      setPagingData(result.data)
    }
  }

  async function handleInit() {
    setLoading(true)
    const result = await initPaging(numFrames, policy)
    if (result.success) {
      setInitialized(true)
      setLastAccess(null)
      await checkStatus()
    }
    setLoading(false)
  }

  async function handleAccess() {
    if (!addressInput) return
    
    setLoading(true)
    const vpn = parseAddress(addressInput) >> 12
    
    const result = await pagingAccess(vpn)
    
    if (result.success) {
      setLastAccess(result)
      setAnimatingFrame(result.frame_index)
      setTimeout(() => setAnimatingFrame(null), 1000)
      await checkStatus()
    }
    
    setAddressInput('')
    setLoading(false)
  }

  async function handleSequence() {
    if (!sequenceInput.trim()) return
    
    setLoading(true)
    
    // Parse addresses from input (comma or space separated)
    const addresses = sequenceInput
      .split(/[\s,]+/)
      .filter(a => a.trim())
      .map(a => parseAddress(a) >> 12)
    
    const result = await runPagingSequence(addresses)
    
    if (result.success) {
      await checkStatus()
    }
    
    setSequenceInput('')
    setLoading(false)
  }

  async function handleReset() {
    await resetPagingStats()
    setLastAccess(null)
    await checkStatus()
  }

  async function handleReinit() {
    setInitialized(false)
    setPagingData(null)
    setLastAccess(null)
  }

  return (
    <div className="animate-slide-in">
      {/* Header */}
      <div style={{ marginBottom: 'var(--spacing-xl)' }}>
        <h1 style={{ fontSize: '1.75rem', fontWeight: '700', marginBottom: 'var(--spacing-sm)' }}>
          Demand Paging Simulator
        </h1>
        <p style={{ color: 'var(--text-secondary)' }}>
          Visualize page faults, page replacement, and memory management
        </p>
      </div>

      {!initialized ? (
        /* Init Card */
        <motion.div 
          initial={{ opacity: 0, scale: 0.95 }}
          animate={{ opacity: 1, scale: 1 }}
          className="card"
          style={{ maxWidth: '600px' }}
        >
          <div className="card-header">
            <h3 className="card-title">
              <Settings size={18} />
              Configure Physical Memory
            </h3>
          </div>
          
          <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: 'var(--spacing-lg)', marginBottom: 'var(--spacing-lg)' }}>
            <div className="input-group">
              <label>Number of Frames</label>
              <input
                type="number"
                min="2"
                max="16"
                value={numFrames}
                onChange={(e) => setNumFrames(parseInt(e.target.value) || 4)}
              />
              <span style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>
                Physical memory size (2-16 frames)
              </span>
            </div>
            
            <div className="input-group">
              <label>Replacement Policy</label>
              <select
                value={policy}
                onChange={(e) => setPolicy(e.target.value)}
                style={{ padding: 'var(--spacing-md)', background: 'var(--bg-tertiary)', border: '1px solid var(--border-primary)', borderRadius: 'var(--radius-md)', color: 'var(--text-primary)' }}
              >
                {policies.map(p => (
                  <option key={p.value} value={p.value}>{p.label}</option>
                ))}
              </select>
            </div>
          </div>
          
          {/* Info Box - Dynamic based on policy */}
          <div style={{ 
            padding: 'var(--spacing-md)', 
            background: 'rgba(59, 130, 246, 0.1)', 
            borderRadius: 'var(--radius-md)',
            borderLeft: '3px solid var(--accent-blue)',
            marginBottom: 'var(--spacing-lg)',
            fontSize: '0.85rem'
          }}>
            <strong style={{ color: 'var(--accent-blue)' }}>{policies.find(p => p.value === policy)?.label}:</strong>
            <span style={{ color: 'var(--text-secondary)', marginLeft: 'var(--spacing-sm)' }}>
              {policies.find(p => p.value === policy)?.desc}
            </span>
          </div>
          
          <button 
            className="btn btn-primary"
            onClick={handleInit}
            disabled={loading}
            style={{ width: '100%' }}
          >
            <Play size={18} />
            Initialize with {numFrames} Frames ({policy})
          </button>
        </motion.div>
      ) : (
        <div style={{ display: 'grid', gridTemplateColumns: '1fr 350px', gap: 'var(--spacing-xl)' }}>
          {/* Main Content */}
          <div>
            {/* Physical Memory Frames */}
            <div className="card" style={{ marginBottom: 'var(--spacing-lg)' }}>
              <div className="card-header">
                <h3 className="card-title">
                  <HardDrive size={18} />
                  Physical Memory ({pagingData?.num_frames} Frames)
                </h3>
                <div style={{ display: 'flex', gap: 'var(--spacing-sm)' }}>
                  <button className="btn btn-secondary" onClick={handleReset} style={{ padding: '0.4rem 0.8rem' }}>
                    <RotateCcw size={14} />
                    Reset Stats
                  </button>
                  <button className="btn btn-secondary" onClick={handleReinit} style={{ padding: '0.4rem 0.8rem' }}>
                    <Settings size={14} />
                    Reconfigure
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
                        PAGE HIT: VPN {lastAccess.vpn_hex} found in Frame {lastAccess.frame_index}
                      </>
                    ) : (
                      <>
                        <AlertTriangle size={16} />
                        PAGE FAULT: VPN {lastAccess.vpn_hex} loaded into Frame {lastAccess.frame_index}
                        {lastAccess.evicted_vpn_hex && (
                          <span style={{ marginLeft: 'var(--spacing-sm)', opacity: 0.8 }}>
                            (evicted {lastAccess.evicted_vpn_hex})
                          </span>
                        )}
                      </>
                    )}
                  </motion.div>
                )}
              </AnimatePresence>

              {/* Frame Grid */}
              <div style={{ 
                display: 'grid', 
                gridTemplateColumns: `repeat(${Math.min(pagingData?.num_frames || 4, 8)}, 1fr)`,
                gap: 'var(--spacing-md)'
              }}>
                {pagingData?.frames?.map((frame, index) => (
                  <motion.div
                    key={index}
                    animate={{
                      scale: animatingFrame === index ? [1, 1.1, 1] : 1,
                      boxShadow: animatingFrame === index 
                        ? '0 0 20px rgba(59, 130, 246, 0.5)' 
                        : 'none'
                    }}
                    transition={{ duration: 0.5 }}
                    style={{
                      padding: 'var(--spacing-md)',
                      background: frame.occupied 
                        ? 'linear-gradient(135deg, rgba(59, 130, 246, 0.2), rgba(139, 92, 246, 0.2))'
                        : 'var(--bg-tertiary)',
                      borderRadius: 'var(--radius-md)',
                      border: `2px solid ${frame.occupied ? 'var(--accent-blue)' : 'var(--border-primary)'}`,
                      textAlign: 'center',
                      minHeight: '80px',
                      display: 'flex',
                      flexDirection: 'column',
                      justifyContent: 'center'
                    }}
                  >
                    <div style={{ fontSize: '0.7rem', color: 'var(--text-muted)', marginBottom: '4px' }}>
                      Frame {index}
                    </div>
                    {frame.occupied ? (
                      <>
                        <div style={{ 
                          fontSize: '0.95rem', 
                          fontWeight: '600', 
                          color: 'var(--accent-cyan)',
                          fontFamily: 'var(--font-mono)'
                        }}>
                          {frame.vpn_hex}
                        </div>
                        <div style={{ fontSize: '0.65rem', color: 'var(--text-muted)', marginTop: '4px' }}>
                          Last: {frame.last_access}
                        </div>
                      </>
                    ) : (
                      <div style={{ color: 'var(--text-muted)', fontSize: '0.85rem' }}>
                        Empty
                      </div>
                    )}
                  </motion.div>
                ))}
              </div>
            </div>

            {/* Access History */}
            <div className="card">
              <div className="card-header">
                <h3 className="card-title">
                  <List size={18} />
                  Access History
                </h3>
              </div>
              
              <div style={{ maxHeight: '200px', overflowY: 'auto' }}>
                {pagingData?.access_history?.length > 0 ? (
                  <div style={{ display: 'flex', flexDirection: 'column', gap: '4px' }}>
                    {[...pagingData.access_history].reverse().map((access, i) => (
                      <div
                        key={i}
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
                          color: access.hit ? 'var(--accent-green)' : 'var(--accent-orange)',
                          fontWeight: '600',
                          width: '50px'
                        }}>
                          {access.hit ? 'HIT' : 'FAULT'}
                        </span>
                        <span style={{ color: 'var(--text-secondary)' }}>{access.vpn_hex}</span>
                        <span style={{ color: 'var(--text-muted)' }}>→ F{access.frame}</span>
                        {access.evicted && (
                          <span style={{ color: 'var(--accent-red)', marginLeft: 'auto', fontSize: '0.7rem' }}>
                            evicted {access.evicted}
                          </span>
                        )}
                      </div>
                    ))}
                  </div>
                ) : (
                  <div style={{ color: 'var(--text-muted)', textAlign: 'center', padding: 'var(--spacing-lg)' }}>
                    No accesses yet
                  </div>
                )}
              </div>
            </div>
          </div>

          {/* Sidebar */}
          <div style={{ display: 'flex', flexDirection: 'column', gap: 'var(--spacing-lg)' }}>
            {/* Access Page */}
            <div className="card">
              <div className="card-header">
                <h3 className="card-title">
                  <Zap size={18} />
                  Access Page
                </h3>
                <span style={{ 
                  fontSize: '0.85rem', 
                  fontWeight: '600',
                  color: 'var(--accent-cyan)', 
                  background: 'rgba(6, 182, 212, 0.15)', 
                  padding: '4px 12px', 
                  borderRadius: '6px',
                  border: '1px solid rgba(6, 182, 212, 0.3)'
                }}>
                  {pagingData?.policy || 'LRU'}
                </span>
              </div>
              
              <div className="input-group" style={{ marginBottom: 'var(--spacing-md)' }}>
                <label>Virtual Address</label>
                <input
                  type="text"
                  placeholder="e.g., 0x1000, 0x2000"
                  value={addressInput}
                  onChange={(e) => setAddressInput(e.target.value)}
                  onKeyDown={(e) => e.key === 'Enter' && handleAccess()}
                />
              </div>
              
              <button 
                className="btn btn-primary"
                onClick={handleAccess}
                disabled={loading || !addressInput}
                style={{ width: '100%' }}
              >
                <Play size={18} />
                Access Page
              </button>

              {/* Quick Access Buttons */}
              <div style={{ marginTop: 'var(--spacing-md)' }}>
                <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)', marginBottom: 'var(--spacing-sm)' }}>
                  Quick Access:
                </div>
                <div style={{ display: 'flex', flexWrap: 'wrap', gap: '6px' }}>
                  {[1, 2, 3, 4, 5, 6, 7, 8].map(n => (
                    <button
                      key={n}
                      onClick={() => setAddressInput(`0x${n}000`)}
                      style={{
                        padding: '4px 8px',
                        background: 'var(--bg-tertiary)',
                        border: '1px solid var(--border-primary)',
                        borderRadius: 'var(--radius-sm)',
                        color: 'var(--text-secondary)',
                        fontSize: '0.75rem',
                        cursor: 'pointer',
                        fontFamily: 'var(--font-mono)'
                      }}
                    >
                      0x{n}000
                    </button>
                  ))}
                </div>
              </div>
            </div>

            {/* Run Sequence */}
            <div className="card">
              <div className="card-header">
                <h3 className="card-title">
                  <List size={18} />
                  Access Sequence
                </h3>
              </div>
              
              <div className="input-group" style={{ marginBottom: 'var(--spacing-md)' }}>
                <label>Addresses (comma/space separated)</label>
                <input
                  type="text"
                  placeholder="0x1000, 0x2000, 0x3000, 0x1000"
                  value={sequenceInput}
                  onChange={(e) => setSequenceInput(e.target.value)}
                  onKeyDown={(e) => e.key === 'Enter' && handleSequence()}
                />
              </div>

              {/* Quick Sequence Buttons */}
              <div style={{ marginBottom: 'var(--spacing-md)' }}>
                <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)', marginBottom: 'var(--spacing-sm)' }}>
                  Demo Sequences:
                </div>
                <div style={{ display: 'flex', flexDirection: 'column', gap: '6px' }}>
                  <button
                    onClick={() => setSequenceInput('0x1000, 0x2000, 0x3000, 0x4000, 0x1000')}
                    style={{
                      padding: '6px 10px',
                      background: 'var(--bg-tertiary)',
                      border: '1px solid var(--border-primary)',
                      borderRadius: 'var(--radius-sm)',
                      color: 'var(--text-secondary)',
                      fontSize: '0.75rem',
                      cursor: 'pointer',
                      textAlign: 'left'
                    }}
                  >
                    Sequential (shows hit)
                  </button>
                  <button
                    onClick={() => setSequenceInput('0x1000, 0x2000, 0x3000, 0x4000, 0x5000')}
                    style={{
                      padding: '6px 10px',
                      background: 'var(--bg-tertiary)',
                      border: '1px solid var(--border-primary)',
                      borderRadius: 'var(--radius-sm)',
                      color: 'var(--text-secondary)',
                      fontSize: '0.75rem',
                      cursor: 'pointer',
                      textAlign: 'left'
                    }}
                  >
                    All faults (causes eviction)
                  </button>
                </div>
              </div>
              
              <button 
                className="btn btn-secondary"
                onClick={handleSequence}
                disabled={loading || !sequenceInput.trim()}
                style={{ width: '100%' }}
              >
                <Play size={18} />
                Run Sequence
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
                    {pagingData?.hit_rate?.toFixed(1) || 0}%
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
                    <div style={{ fontSize: '1.2rem', fontWeight: '700', color: 'var(--accent-green)' }}>
                      {pagingData?.page_hits || 0}
                    </div>
                  </div>
                  
                  <div style={{ 
                    padding: 'var(--spacing-md)', 
                    background: 'rgba(245, 158, 11, 0.1)',
                    borderRadius: 'var(--radius-md)',
                    textAlign: 'center'
                  }}>
                    <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Faults</div>
                    <div style={{ fontSize: '1.2rem', fontWeight: '700', color: 'var(--accent-orange)' }}>
                      {pagingData?.page_faults || 0}
                    </div>
                  </div>
                </div>
                
                <div style={{ 
                  padding: 'var(--spacing-sm)',
                  background: 'var(--bg-tertiary)',
                  borderRadius: 'var(--radius-sm)',
                  fontSize: '0.8rem',
                  color: 'var(--text-muted)',
                  textAlign: 'center'
                }}>
                  Disk Reads: {pagingData?.disk_reads || 0} | Policy: {pagingData?.policy}
                </div>
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  )
}

export default DemandPaging
