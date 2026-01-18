import { useState, useEffect } from 'react'
import { motion } from 'framer-motion'
import { 
  Activity, 
  Cpu, 
  HardDrive, 
  MemoryStick, 
  TrendingUp,
  AlertCircle,
  CheckCircle,
  Server,
  RotateCcw
} from 'lucide-react'
import { getSystemMemory, getProcesses, healthCheck, formatBytes, resetAll } from '../utils/api'

function Dashboard() {
  const [systemMem, setSystemMem] = useState(null)
  const [processes, setProcesses] = useState([])
  const [apiStatus, setApiStatus] = useState('checking')
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState(null)

  useEffect(() => {
    loadData()
  }, [])

  async function loadData() {
    setLoading(true)
    
    // Check API health
    const health = await healthCheck()
    setApiStatus(health.success ? 'connected' : 'disconnected')
    
    // Load system memory
    const memResult = await getSystemMemory()
    if (memResult.success) {
      setSystemMem(memResult.data)
    }
    
    // Load processes
    const procResult = await getProcesses()
    if (procResult.success) {
      // Sort by memory usage descending
      const sorted = (procResult.data || []).sort((a, b) => b.memory_kb - a.memory_kb)
      setProcesses(sorted.slice(0, 10))
    }
    
    setLoading(false)
  }

  const memUsagePercent = systemMem 
    ? ((systemMem.total - systemMem.available) / systemMem.total * 100).toFixed(1)
    : 0

  return (
    <div className="animate-slide-in">
      {/* Header */}
      <div style={{ marginBottom: 'var(--spacing-xl)' }}>
        <h1 style={{ fontSize: '1.75rem', fontWeight: '700', marginBottom: 'var(--spacing-sm)' }}>
          Dashboard
        </h1>
        <p style={{ color: 'var(--text-secondary)' }}>
          System overview and memory statistics
        </p>
      </div>

      {/* API Status Banner */}
      <motion.div
        initial={{ opacity: 0, y: -10 }}
        animate={{ opacity: 1, y: 0 }}
        style={{
          display: 'flex',
          alignItems: 'center',
          gap: 'var(--spacing-sm)',
          padding: 'var(--spacing-md)',
          background: apiStatus === 'connected' 
            ? 'rgba(16, 185, 129, 0.1)' 
            : apiStatus === 'disconnected'
            ? 'rgba(239, 68, 68, 0.1)'
            : 'rgba(59, 130, 246, 0.1)',
          borderRadius: 'var(--radius-md)',
          marginBottom: 'var(--spacing-xl)',
          border: `1px solid ${apiStatus === 'connected' 
            ? 'rgba(16, 185, 129, 0.3)' 
            : apiStatus === 'disconnected'
            ? 'rgba(239, 68, 68, 0.3)'
            : 'rgba(59, 130, 246, 0.3)'}`
        }}
      >
        {apiStatus === 'connected' ? (
          <>
            <CheckCircle size={18} style={{ color: 'var(--accent-green)' }} />
            <span style={{ color: 'var(--accent-green)', fontWeight: '500' }}>
              API Connected - Backend is running
            </span>
          </>
        ) : apiStatus === 'disconnected' ? (
          <>
            <AlertCircle size={18} style={{ color: 'var(--accent-red)' }} />
            <span style={{ color: 'var(--accent-red)', fontWeight: '500' }}>
              API Disconnected - Start the Flask server and C backend
            </span>
          </>
        ) : (
          <>
            <Activity size={18} className="animate-pulse" style={{ color: 'var(--accent-blue)' }} />
            <span style={{ color: 'var(--accent-blue)', fontWeight: '500' }}>
              Checking connection...
            </span>
          </>
        )}
        <div style={{ display: 'flex', gap: 'var(--spacing-sm)', marginLeft: 'auto' }}>
          <button 
            onClick={() => resetAll()}
            className="btn btn-secondary"
            style={{ padding: '0.5rem 1rem' }}
            title="Reset TLB and Paging simulators"
          >
            <RotateCcw size={14} />
            Reset Simulators
          </button>
          <button 
            onClick={loadData}
            className="btn btn-secondary"
            style={{ padding: '0.5rem 1rem' }}
          >
            Refresh
          </button>
        </div>
      </motion.div>

      {/* Stats Grid */}
      <div className="card-grid" style={{ marginBottom: 'var(--spacing-xl)' }}>
        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: 0.1 }}
          className="stat-card"
        >
          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--spacing-sm)', marginBottom: 'var(--spacing-md)' }}>
            <MemoryStick size={20} style={{ color: 'var(--accent-blue)' }} />
            <span className="stat-label">Total Memory</span>
          </div>
          <div className="stat-value">
            {systemMem ? formatBytes(systemMem.total) : '--'}
          </div>
        </motion.div>

        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: 0.2 }}
          className="stat-card green"
        >
          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--spacing-sm)', marginBottom: 'var(--spacing-md)' }}>
            <HardDrive size={20} style={{ color: 'var(--accent-green)' }} />
            <span className="stat-label">Available Memory</span>
          </div>
          <div className="stat-value">
            {systemMem ? formatBytes(systemMem.available) : '--'}
          </div>
        </motion.div>

        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: 0.3 }}
          className="stat-card orange"
        >
          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--spacing-sm)', marginBottom: 'var(--spacing-md)' }}>
            <TrendingUp size={20} style={{ color: 'var(--accent-orange)' }} />
            <span className="stat-label">Memory Usage</span>
          </div>
          <div className="stat-value">
            {memUsagePercent}%
          </div>
        </motion.div>

        <motion.div
          initial={{ opacity: 0, y: 20 }}
          animate={{ opacity: 1, y: 0 }}
          transition={{ delay: 0.4 }}
          className="stat-card"
        >
          <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--spacing-sm)', marginBottom: 'var(--spacing-md)' }}>
            <Server size={20} style={{ color: 'var(--accent-purple)' }} />
            <span className="stat-label">Cached</span>
          </div>
          <div className="stat-value">
            {systemMem ? formatBytes(systemMem.cached) : '--'}
          </div>
        </motion.div>
      </div>

      {/* Memory Bar */}
      {systemMem && (
        <motion.div
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ delay: 0.5 }}
          className="card"
          style={{ marginBottom: 'var(--spacing-xl)' }}
        >
          <div className="card-header">
            <h3 className="card-title">
              <Activity size={18} />
              Memory Distribution
            </h3>
          </div>
          
          {/* Clean segmented bar without text */}
          <div style={{ 
            height: '32px', 
            borderRadius: 'var(--radius-md)', 
            overflow: 'hidden',
            display: 'flex',
            background: 'var(--bg-tertiary)',
            border: '1px solid var(--border-primary)'
          }}>
            <div 
              style={{ 
                width: `${(systemMem.active / systemMem.total) * 100}%`,
                background: 'linear-gradient(135deg, #3b82f6 0%, #2563eb 100%)',
                transition: 'width 0.5s ease'
              }}
              title={`Active: ${formatBytes(systemMem.active)}`}
            />
            <div 
              style={{ 
                width: `${(systemMem.cached / systemMem.total) * 100}%`,
                background: 'linear-gradient(135deg, #10b981 0%, #059669 100%)',
                transition: 'width 0.5s ease'
              }}
              title={`Cached: ${formatBytes(systemMem.cached)}`}
            />
            <div 
              style={{ 
                width: `${(systemMem.buffers / systemMem.total) * 100}%`,
                background: 'linear-gradient(135deg, #8b5cf6 0%, #7c3aed 100%)',
                transition: 'width 0.5s ease'
              }}
              title={`Buffers: ${formatBytes(systemMem.buffers)}`}
            />
          </div>
          
          {/* Detailed legend with percentages */}
          <div style={{ 
            display: 'grid', 
            gridTemplateColumns: 'repeat(4, 1fr)',
            gap: 'var(--spacing-md)', 
            marginTop: 'var(--spacing-lg)',
          }}>
            <div style={{ 
              padding: 'var(--spacing-sm) var(--spacing-md)',
              background: 'rgba(59, 130, 246, 0.1)',
              borderRadius: 'var(--radius-sm)',
              borderLeft: '3px solid #3b82f6'
            }}>
              <div style={{ fontSize: '0.7rem', color: 'var(--text-muted)', marginBottom: '2px' }}>ACTIVE</div>
              <div style={{ fontSize: '0.95rem', fontWeight: '600', color: '#3b82f6' }}>{formatBytes(systemMem.active)}</div>
              <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>{((systemMem.active / systemMem.total) * 100).toFixed(1)}%</div>
            </div>
            <div style={{ 
              padding: 'var(--spacing-sm) var(--spacing-md)',
              background: 'rgba(16, 185, 129, 0.1)',
              borderRadius: 'var(--radius-sm)',
              borderLeft: '3px solid #10b981'
            }}>
              <div style={{ fontSize: '0.7rem', color: 'var(--text-muted)', marginBottom: '2px' }}>CACHED</div>
              <div style={{ fontSize: '0.95rem', fontWeight: '600', color: '#10b981' }}>{formatBytes(systemMem.cached)}</div>
              <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>{((systemMem.cached / systemMem.total) * 100).toFixed(1)}%</div>
            </div>
            <div style={{ 
              padding: 'var(--spacing-sm) var(--spacing-md)',
              background: 'rgba(139, 92, 246, 0.1)',
              borderRadius: 'var(--radius-sm)',
              borderLeft: '3px solid #8b5cf6'
            }}>
              <div style={{ fontSize: '0.7rem', color: 'var(--text-muted)', marginBottom: '2px' }}>BUFFERS</div>
              <div style={{ fontSize: '0.95rem', fontWeight: '600', color: '#8b5cf6' }}>{formatBytes(systemMem.buffers)}</div>
              <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>{((systemMem.buffers / systemMem.total) * 100).toFixed(1)}%</div>
            </div>
            <div style={{ 
              padding: 'var(--spacing-sm) var(--spacing-md)',
              background: 'rgba(100, 116, 139, 0.1)',
              borderRadius: 'var(--radius-sm)',
              borderLeft: '3px solid #64748b'
            }}>
              <div style={{ fontSize: '0.7rem', color: 'var(--text-muted)', marginBottom: '2px' }}>FREE</div>
              <div style={{ fontSize: '0.95rem', fontWeight: '600', color: '#64748b' }}>{formatBytes(systemMem.free)}</div>
              <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>{((systemMem.free / systemMem.total) * 100).toFixed(1)}%</div>
            </div>
          </div>
        </motion.div>
      )}

      {/* Top Processes */}
      <motion.div
        initial={{ opacity: 0, y: 20 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ delay: 0.6 }}
        className="card"
      >
        <div className="card-header">
          <h3 className="card-title">
            <Cpu size={18} />
            Top Processes by Memory
          </h3>
        </div>
        
        <div className="process-list">
          {processes.map((proc, index) => (
            <motion.div
              key={proc.pid}
              initial={{ opacity: 0, x: -20 }}
              animate={{ opacity: 1, x: 0 }}
              transition={{ delay: 0.7 + index * 0.05 }}
              className="process-item"
            >
              <span className="pid">{proc.pid}</span>
              <span className="name">{proc.name}</span>
              <span className="memory">{formatBytes(proc.memory_kb * 1024)}</span>
              <span className={`state ${proc.state === 'S' ? 'sleeping' : 'running'}`}>
                {proc.state}
              </span>
            </motion.div>
          ))}
          
          {processes.length === 0 && !loading && (
            <div style={{ 
              padding: 'var(--spacing-xl)', 
              textAlign: 'center', 
              color: 'var(--text-muted)' 
            }}>
              {apiStatus === 'disconnected' 
                ? 'Start the backend to view processes'
                : 'No processes found'
              }
            </div>
          )}
        </div>
      </motion.div>
    </div>
  )
}

export default Dashboard
