import { useState, useEffect } from 'react'
import { motion, AnimatePresence } from 'framer-motion'
import { 
  HardDrive, 
  Lock, 
  Unlock, 
  Trash2, 
  Plus, 
  RefreshCcw,
  Zap,
  Info,
  AlertTriangle
} from 'lucide-react'
import { 
  playgroundAllocate, 
  playgroundLock, 
  playgroundUnlock, 
  playgroundAdvise, 
  playgroundFree, 
  getPlaygroundStatus, 
  playgroundReset,
  getSystemMemory
} from '../utils/api'

function MemoryPlayground() {
  const [status, setStatus] = useState(null)
  const [systemMem, setSystemMem] = useState(null)
  const [allocSize, setAllocSize] = useState(10)
  const [loading, setLoading] = useState(false)
  const [message, setMessage] = useState(null)
  const [selectedRegion, setSelectedRegion] = useState(null)

  const adviceOptions = [
    { value: 'NORMAL', label: 'Normal', desc: 'Default access pattern' },
    { value: 'SEQUENTIAL', label: 'Sequential', desc: 'Expect sequential reads' },
    { value: 'RANDOM', label: 'Random', desc: 'Expect random access' },
    { value: 'WILLNEED', label: 'Will Need', desc: 'Prefetch into memory' },
    { value: 'DONTNEED', label: 'Don\'t Need', desc: 'Can be swapped out' }
  ]

  useEffect(() => {
    loadStatus()
    loadSystemMem()
    const interval = setInterval(loadSystemMem, 3000)
    return () => clearInterval(interval)
  }, [])

  async function loadStatus() {
    const result = await getPlaygroundStatus()
    if (result.success) {
      setStatus(result.data || result)
    }
  }

  async function loadSystemMem() {
    const result = await getSystemMemory()
    if (result.success) {
      setSystemMem(result.data)
    }
  }

  async function handleAllocate() {
    setLoading(true)
    const result = await playgroundAllocate(allocSize, true)
    if (result.success) {
      showMessage(`✓ Allocated ${allocSize}MB (${result.data?.pages_touched || allocSize * 256} pages touched)`, 'success')
      await loadStatus()
      await loadSystemMem()
    } else {
      showMessage(`✗ ${result.error}`, 'error')
    }
    setLoading(false)
  }

  async function handleLock(regionId) {
    setLoading(true)
    const result = await playgroundLock(regionId)
    if (result.success) {
      showMessage(`🔒 ${result.data?.message || 'Region locked'}`, 'success')
      await loadStatus()
    } else {
      showMessage(`✗ ${result.error}`, 'error')
    }
    setLoading(false)
  }

  async function handleUnlock(regionId) {
    setLoading(true)
    const result = await playgroundUnlock(regionId)
    if (result.success) {
      showMessage(`🔓 ${result.data?.message || 'Region unlocked'}`, 'success')
      await loadStatus()
    } else {
      showMessage(`✗ ${result.error}`, 'error')
    }
    setLoading(false)
  }

  async function handleAdvise(regionId, advice) {
    setLoading(true)
    const result = await playgroundAdvise(regionId, advice)
    if (result.success) {
      showMessage(`📋 ${result.data?.message || `Applied ${advice}`}`, 'success')
      await loadStatus()
    } else {
      showMessage(`✗ ${result.error}`, 'error')
    }
    setLoading(false)
  }

  async function handleFree(regionId) {
    setLoading(true)
    const result = await playgroundFree(regionId)
    if (result.success) {
      showMessage(`🗑️ ${result.data?.message || 'Region freed'}`, 'success')
      await loadStatus()
      await loadSystemMem()
    } else {
      showMessage(`✗ ${result.error}`, 'error')
    }
    setLoading(false)
  }

  async function handleReset() {
    setLoading(true)
    const result = await playgroundReset()
    if (result.success) {
      showMessage(`♻️ ${result.data?.message || 'All regions freed'}`, 'success')
      await loadStatus()
      await loadSystemMem()
    } else {
      showMessage(`✗ ${result.error}`, 'error')
    }
    setLoading(false)
  }

  function showMessage(text, type) {
    setMessage({ text, type })
    setTimeout(() => setMessage(null), 4000)
  }

  const totalAllocated = status?.total_allocated_mb || 0
  const totalLocked = status?.total_locked_mb || 0
  const regions = status?.regions || []
  const mlockAvailable = status?.mlock_available ?? true

  return (
    <div className="page-content">
      <motion.div
        initial={{ opacity: 0, y: -20 }}
        animate={{ opacity: 1, y: 0 }}
        className="page-header"
      >
        <h1 className="page-title">
          <Zap size={28} style={{ color: 'var(--accent-orange)' }} />
          Memory Playground
        </h1>
        <p className="page-subtitle">
          Actively interact with OS memory: allocate, lock, and advise
        </p>
      </motion.div>

      {/* Message Toast */}
      <AnimatePresence>
        {message && (
          <motion.div
            initial={{ opacity: 0, y: -20 }}
            animate={{ opacity: 1, y: 0 }}
            exit={{ opacity: 0, y: -20 }}
            style={{
              padding: 'var(--spacing-md) var(--spacing-lg)',
              borderRadius: 'var(--radius-md)',
              marginBottom: 'var(--spacing-lg)',
              background: message.type === 'success' 
                ? 'rgba(16, 185, 129, 0.2)' 
                : 'rgba(239, 68, 68, 0.2)',
              border: `1px solid ${message.type === 'success' 
                ? 'var(--accent-green)' 
                : 'var(--accent-red)'}`,
              color: message.type === 'success' 
                ? 'var(--accent-green)' 
                : 'var(--accent-red)'
            }}
          >
            {message.text}
          </motion.div>
        )}
      </AnimatePresence>

      {/* Info Banner */}
      <motion.div
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        className="card"
        style={{ 
          marginTop: 'var(--spacing-lg)',
          marginBottom: 'var(--spacing-xl)',
          background: 'rgba(245, 158, 11, 0.1)',
          borderLeft: '4px solid var(--accent-orange)'
        }}
      >
        <div style={{ display: 'flex', gap: 'var(--spacing-md)', alignItems: 'flex-start' }}>
          <Info size={20} style={{ color: 'var(--accent-orange)', flexShrink: 0, marginTop: 2 }} />
          <div>
            <strong style={{ color: 'var(--accent-orange)' }}>What this does:</strong>
            <p style={{ color: 'var(--text-secondary)', margin: '4px 0 0 0', fontSize: '0.9rem' }}>
              This page allocates <strong>real memory</strong> using <code>mmap()</code>, locks it with <code>mlock()</code>,
              and applies kernel hints with <code>madvise()</code>. Watch the system memory stats change in real-time!
            </p>
          </div>
        </div>
      </motion.div>

      <div style={{ display: 'grid', gridTemplateColumns: '350px 1fr', gap: 'var(--spacing-xl)' }}>
        {/* Left Panel - Controls */}
        <div>
          {/* Allocation Card */}
          <motion.div
            initial={{ opacity: 0, x: -20 }}
            animate={{ opacity: 1, x: 0 }}
            className="card"
            style={{ marginBottom: 'var(--spacing-lg)' }}
          >
            <h3 style={{ marginBottom: 'var(--spacing-lg)', display: 'flex', alignItems: 'center', gap: 'var(--spacing-sm)' }}>
              <Plus size={18} style={{ color: 'var(--accent-green)' }} />
              Allocate Memory
            </h3>

            <div style={{ marginBottom: 'var(--spacing-lg)' }}>
              <label style={{ display: 'block', marginBottom: 'var(--spacing-sm)', fontSize: '0.9rem' }}>
                Size: <strong style={{ color: 'var(--accent-cyan)' }}>{allocSize} MB</strong>
                <span style={{ color: 'var(--text-muted)', marginLeft: 'var(--spacing-sm)' }}>
                  ({allocSize * 256} pages)
                </span>
              </label>
              <input
                type="range"
                min="1"
                max="100"
                value={allocSize}
                onChange={(e) => setAllocSize(parseInt(e.target.value))}
                style={{ width: '100%' }}
              />
              <div style={{ display: 'flex', justifyContent: 'space-between', fontSize: '0.75rem', color: 'var(--text-muted)' }}>
                <span>1 MB</span>
                <span>100 MB</span>
              </div>
            </div>

            <button 
              className="btn btn-primary"
              onClick={handleAllocate}
              disabled={loading}
              style={{ width: '100%' }}
            >
              <HardDrive size={16} />
              Allocate {allocSize} MB
            </button>
          </motion.div>

          {/* Stats Card */}
          <motion.div
            initial={{ opacity: 0, x: -20 }}
            animate={{ opacity: 1, x: 0 }}
            transition={{ delay: 0.1 }}
            className="card"
            style={{ marginBottom: 'var(--spacing-lg)' }}
          >
            <h3 style={{ marginBottom: 'var(--spacing-lg)' }}>Playground Stats</h3>
            
            <div style={{ display: 'grid', gap: 'var(--spacing-md)' }}>
              <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                <span style={{ color: 'var(--text-secondary)' }}>Total Allocated</span>
                <span style={{ color: 'var(--accent-cyan)', fontWeight: '600' }}>{totalAllocated.toFixed(1)} MB</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                <span style={{ color: 'var(--text-secondary)' }}>Total Locked</span>
                <span style={{ color: 'var(--accent-orange)', fontWeight: '600' }}>{totalLocked.toFixed(1)} MB</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                <span style={{ color: 'var(--text-secondary)' }}>Active Regions</span>
                <span style={{ color: 'var(--text-primary)', fontWeight: '600' }}>{regions.length}</span>
              </div>
              <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                <span style={{ color: 'var(--text-secondary)' }}>mlock Available</span>
                <span style={{ color: mlockAvailable ? 'var(--accent-green)' : 'var(--accent-red)', fontWeight: '600' }}>
                  {mlockAvailable ? 'Yes' : 'No'}
                </span>
              </div>
            </div>

            {regions.length > 0 && (
              <button 
                className="btn btn-secondary"
                onClick={handleReset}
                disabled={loading}
                style={{ width: '100%', marginTop: 'var(--spacing-lg)' }}
              >
                <RefreshCcw size={14} />
                Free All Regions
              </button>
            )}
          </motion.div>

          {/* System Memory Card */}
          {systemMem && (
            <motion.div
              initial={{ opacity: 0, x: -20 }}
              animate={{ opacity: 1, x: 0 }}
              transition={{ delay: 0.2 }}
              className="card"
            >
              <h3 style={{ marginBottom: 'var(--spacing-lg)' }}>System Memory (Live)</h3>
              
              <div style={{ display: 'grid', gap: 'var(--spacing-md)' }}>
                <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                  <span style={{ color: 'var(--text-secondary)' }}>Total</span>
                  <span style={{ color: 'var(--text-primary)' }}>{(systemMem.total / 1024 / 1024 / 1024).toFixed(1)} GB</span>
                </div>
                <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                  <span style={{ color: 'var(--text-secondary)' }}>Used</span>
                  <span style={{ color: 'var(--accent-purple)' }}>{((systemMem.total - systemMem.available) / 1024 / 1024 / 1024).toFixed(2)} GB</span>
                </div>
                <div style={{ display: 'flex', justifyContent: 'space-between' }}>
                  <span style={{ color: 'var(--text-secondary)' }}>Available</span>
                  <span style={{ color: 'var(--accent-green)' }}>{(systemMem.available / 1024 / 1024 / 1024).toFixed(2)} GB</span>
                </div>
                
                {/* Memory bar */}
                <div style={{ 
                  height: 8, 
                  borderRadius: 4, 
                  background: 'var(--bg-tertiary)',
                  overflow: 'hidden',
                  marginTop: 'var(--spacing-sm)'
                }}>
                  <div style={{
                    height: '100%',
                    width: `${((systemMem.total - systemMem.available) / systemMem.total * 100).toFixed(1)}%`,
                    background: 'linear-gradient(90deg, var(--accent-purple), var(--accent-blue))',
                    transition: 'width 0.3s ease'
                  }} />
                </div>
              </div>
            </motion.div>
          )}
        </div>

        {/* Right Panel - Regions */}
        <motion.div
          initial={{ opacity: 0, x: 20 }}
          animate={{ opacity: 1, x: 0 }}
          className="card"
        >
          <h3 style={{ marginBottom: 'var(--spacing-lg)', display: 'flex', alignItems: 'center', gap: 'var(--spacing-sm)' }}>
            <HardDrive size={18} style={{ color: 'var(--accent-blue)' }} />
            Allocated Regions
          </h3>

          {regions.length === 0 ? (
            <div style={{ 
              textAlign: 'center', 
              padding: 'var(--spacing-xxl)',
              color: 'var(--text-muted)'
            }}>
              <HardDrive size={48} style={{ opacity: 0.3, marginBottom: 'var(--spacing-md)' }} />
              <p>No memory allocated yet</p>
              <p style={{ fontSize: '0.85rem' }}>Use the panel on the left to allocate memory</p>
            </div>
          ) : (
            <div style={{ display: 'grid', gap: 'var(--spacing-md)' }}>
              <AnimatePresence>
                {regions.map((region) => (
                  <motion.div
                    key={region.id}
                    initial={{ opacity: 0, scale: 0.9 }}
                    animate={{ opacity: 1, scale: 1 }}
                    exit={{ opacity: 0, scale: 0.9 }}
                    style={{
                      padding: 'var(--spacing-lg)',
                      background: region.locked 
                        ? 'rgba(245, 158, 11, 0.1)' 
                        : 'var(--bg-tertiary)',
                      borderRadius: 'var(--radius-md)',
                      border: region.locked 
                        ? '1px solid var(--accent-orange)' 
                        : '1px solid var(--border-primary)'
                    }}
                  >
                    <div style={{ display: 'flex', justifyContent: 'space-between', alignItems: 'center', marginBottom: 'var(--spacing-md)' }}>
                      <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--spacing-md)' }}>
                        <div style={{
                          width: 40,
                          height: 40,
                          borderRadius: 'var(--radius-sm)',
                          background: region.locked ? 'var(--accent-orange)' : 'var(--accent-blue)',
                          display: 'flex',
                          alignItems: 'center',
                          justifyContent: 'center',
                          color: 'white'
                        }}>
                          {region.locked ? <Lock size={20} /> : <HardDrive size={20} />}
                        </div>
                        <div>
                          <div style={{ fontWeight: '600' }}>
                            Region #{region.id}
                            {region.locked && (
                              <span style={{ 
                                marginLeft: 'var(--spacing-sm)', 
                                fontSize: '0.75rem', 
                                color: 'var(--accent-orange)',
                                background: 'rgba(245, 158, 11, 0.2)',
                                padding: '2px 8px',
                                borderRadius: 'var(--radius-sm)'
                              }}>
                                LOCKED
                              </span>
                            )}
                          </div>
                          <div style={{ fontSize: '0.85rem', color: 'var(--text-muted)' }}>
                            {region.size_mb} MB • {region.pages} pages • {region.advice}
                          </div>
                        </div>
                      </div>
                      
                      <div style={{ display: 'flex', gap: 'var(--spacing-sm)' }}>
                        {region.locked ? (
                          <button
                            className="btn btn-secondary"
                            onClick={() => handleUnlock(region.id)}
                            disabled={loading}
                            title="Unlock (munlock)"
                          >
                            <Unlock size={14} />
                          </button>
                        ) : (
                          <button
                            className="btn btn-secondary"
                            onClick={() => handleLock(region.id)}
                            disabled={loading || !mlockAvailable}
                            title="Lock (mlock)"
                          >
                            <Lock size={14} />
                          </button>
                        )}
                        <button
                          className="btn btn-secondary"
                          onClick={() => handleFree(region.id)}
                          disabled={loading}
                          title="Free memory"
                          style={{ color: 'var(--accent-red)' }}
                        >
                          <Trash2 size={14} />
                        </button>
                      </div>
                    </div>

                    {/* Advice selector */}
                    <div style={{ display: 'flex', gap: 'var(--spacing-sm)', flexWrap: 'wrap' }}>
                      {adviceOptions.map(opt => (
                        <button
                          key={opt.value}
                          className="btn btn-secondary"
                          onClick={() => handleAdvise(region.id, opt.value)}
                          disabled={loading}
                          title={opt.desc}
                          style={{
                            fontSize: '0.75rem',
                            padding: '4px 10px',
                            background: region.advice === opt.value 
                              ? 'var(--accent-purple)' 
                              : undefined,
                            color: region.advice === opt.value 
                              ? 'white' 
                              : undefined
                          }}
                        >
                          {opt.label}
                        </button>
                      ))}
                    </div>
                  </motion.div>
                ))}
              </AnimatePresence>
            </div>
          )}
        </motion.div>
      </div>

      {/* Warning for WSL */}
      {!mlockAvailable && (
        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          className="card"
          style={{ 
            marginTop: 'var(--spacing-xl)',
            background: 'rgba(239, 68, 68, 0.1)',
            borderLeft: '4px solid var(--accent-red)'
          }}
        >
          <div style={{ display: 'flex', gap: 'var(--spacing-md)', alignItems: 'center' }}>
            <AlertTriangle size={20} style={{ color: 'var(--accent-red)' }} />
            <span style={{ color: 'var(--accent-red)' }}>
              <strong>mlock not available</strong> — Memory locking requires root privileges or <code>CAP_IPC_LOCK</code> capability.
            </span>
          </div>
        </motion.div>
      )}
    </div>
  )
}

export default MemoryPlayground
