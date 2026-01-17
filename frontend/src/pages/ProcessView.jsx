import { useState, useEffect } from 'react'
import { motion } from 'framer-motion'
import { 
  Cpu, 
  Search, 
  MapPin, 
  RefreshCw,
  AlertCircle,
  ChevronRight
} from 'lucide-react'
import { getProcesses, getMemoryMaps, getMemoryStats, formatBytes } from '../utils/api'

function ProcessView() {
  const [processes, setProcesses] = useState([])
  const [selectedPid, setSelectedPid] = useState(null)
  const [regions, setRegions] = useState([])
  const [stats, setStats] = useState(null)
  const [searchTerm, setSearchTerm] = useState('')
  const [loading, setLoading] = useState(false)

  useEffect(() => {
    loadProcesses()
  }, [])

  async function loadProcesses() {
    const result = await getProcesses()
    if (result.success) {
      const sorted = (result.data || []).sort((a, b) => b.memory_kb - a.memory_kb)
      setProcesses(sorted)
    }
  }

  async function selectProcess(pid) {
    setSelectedPid(pid)
    setLoading(true)
    
    // Load memory regions
    const mapsResult = await getMemoryMaps(pid)
    if (mapsResult.success) {
      setRegions(mapsResult.data || [])
    }
    
    // Load memory stats
    const statsResult = await getMemoryStats(pid)
    if (statsResult.success) {
      setStats(statsResult.data)
    }
    
    setLoading(false)
  }

  const filteredProcesses = processes.filter(p => 
    p.name.toLowerCase().includes(searchTerm.toLowerCase()) ||
    p.pid.toString().includes(searchTerm)
  )

  const selectedProcess = processes.find(p => p.pid === selectedPid)

  return (
    <div className="animate-slide-in">
      {/* Header */}
      <div style={{ marginBottom: 'var(--spacing-xl)' }}>
        <h1 style={{ fontSize: '1.75rem', fontWeight: '700', marginBottom: 'var(--spacing-sm)' }}>
          Process Memory
        </h1>
        <p style={{ color: 'var(--text-secondary)' }}>
          Analyze memory regions and statistics for any running process
        </p>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '350px 1fr', gap: 'var(--spacing-xl)' }}>
        {/* Process List */}
        <div className="card">
          <div className="card-header">
            <h3 className="card-title">
              <Cpu size={18} />
              Processes
            </h3>
            <button 
              onClick={loadProcesses}
              className="btn btn-secondary"
              style={{ padding: '0.4rem 0.8rem' }}
            >
              <RefreshCw size={14} />
            </button>
          </div>
          
          {/* Search */}
          <div className="input-group" style={{ marginBottom: 'var(--spacing-md)' }}>
            <div style={{ position: 'relative' }}>
              <Search size={16} style={{ 
                position: 'absolute', 
                left: '12px', 
                top: '50%', 
                transform: 'translateY(-50%)',
                color: 'var(--text-muted)'
              }} />
              <input
                type="text"
                placeholder="Search by name or PID..."
                value={searchTerm}
                onChange={(e) => setSearchTerm(e.target.value)}
                style={{ paddingLeft: '36px', width: '100%' }}
              />
            </div>
          </div>

          <div className="process-list" style={{ maxHeight: '500px' }}>
            {filteredProcesses.map(proc => (
              <div
                key={proc.pid}
                className={`process-item ${selectedPid === proc.pid ? 'selected' : ''}`}
                onClick={() => selectProcess(proc.pid)}
              >
                <span className="pid">{proc.pid}</span>
                <span className="name">{proc.name}</span>
                <span className="memory">{formatBytes(proc.memory_kb * 1024)}</span>
                <span className={`state ${proc.state === 'S' ? 'sleeping' : 'running'}`}>
                  {proc.state}
                </span>
              </div>
            ))}
          </div>
        </div>

        {/* Memory Details */}
        <div>
          {selectedPid ? (
            <>
              {/* Process Info Header */}
              <motion.div
                initial={{ opacity: 0, y: -10 }}
                animate={{ opacity: 1, y: 0 }}
                className="card"
                style={{ marginBottom: 'var(--spacing-lg)' }}
              >
                <div style={{ display: 'flex', alignItems: 'center', gap: 'var(--spacing-lg)' }}>
                  <div style={{
                    width: 48,
                    height: 48,
                    borderRadius: 'var(--radius-md)',
                    background: 'var(--gradient-primary)',
                    display: 'flex',
                    alignItems: 'center',
                    justifyContent: 'center'
                  }}>
                    <Cpu size={24} color="white" />
                  </div>
                  <div>
                    <h2 style={{ fontSize: '1.25rem', fontWeight: '600' }}>
                      {selectedProcess?.name}
                    </h2>
                    <span style={{ color: 'var(--text-muted)', fontFamily: 'var(--font-mono)' }}>
                      PID: {selectedPid}
                    </span>
                  </div>
                  {stats && (
                    <div style={{ marginLeft: 'auto', display: 'flex', gap: 'var(--spacing-xl)' }}>
                      <div>
                        <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Virtual Size</div>
                        <div style={{ fontFamily: 'var(--font-mono)', color: 'var(--accent-blue)' }}>
                          {formatBytes(stats.vm_size)}
                        </div>
                      </div>
                      <div>
                        <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>RSS</div>
                        <div style={{ fontFamily: 'var(--font-mono)', color: 'var(--accent-green)' }}>
                          {formatBytes(stats.vm_rss)}
                        </div>
                      </div>
                      <div>
                        <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>Page Faults</div>
                        <div style={{ fontFamily: 'var(--font-mono)', color: 'var(--accent-orange)' }}>
                          {stats.faults?.total?.toLocaleString() || 0}
                        </div>
                      </div>
                    </div>
                  )}
                </div>
              </motion.div>

              {/* Memory Regions */}
              <motion.div
                initial={{ opacity: 0 }}
                animate={{ opacity: 1 }}
                transition={{ delay: 0.1 }}
                className="card"
              >
                <div className="card-header">
                  <h3 className="card-title">
                    <MapPin size={18} />
                    Memory Regions ({regions.length})
                  </h3>
                </div>

                {loading ? (
                  <div style={{ padding: 'var(--spacing-xl)', textAlign: 'center', color: 'var(--text-muted)' }}>
                    Loading memory regions...
                  </div>
                ) : (
                  <div className="memory-map">
                    {/* Header */}
                    <div className="memory-region" style={{ 
                      background: 'var(--bg-tertiary)', 
                      fontWeight: '600',
                      fontSize: '0.7rem',
                      color: 'var(--text-muted)',
                      textTransform: 'uppercase',
                      cursor: 'default'
                    }}>
                      <span>Start</span>
                      <span>End</span>
                      <span>Perm</span>
                      <span>Path</span>
                      <span style={{ textAlign: 'right' }}>Size</span>
                      <span>Type</span>
                    </div>
                    
                    {regions.map((region, index) => (
                      <motion.div
                        key={index}
                        initial={{ opacity: 0, x: -10 }}
                        animate={{ opacity: 1, x: 0 }}
                        transition={{ delay: index * 0.02 }}
                        className="memory-region"
                      >
                        <span className="address">{region.start_addr}</span>
                        <span className="address">{region.end_addr}</span>
                        <div className="permissions">
                          <span className={`perm ${region.permissions[0] === 'r' ? 'r' : 'inactive'}`}>r</span>
                          <span className={`perm ${region.permissions[1] === 'w' ? 'w' : 'inactive'}`}>w</span>
                          <span className={`perm ${region.permissions[2] === 'x' ? 'x' : 'inactive'}`}>x</span>
                          <span className={`perm ${region.permissions[3] === 'p' ? 'p' : 'inactive'}`}>
                            {region.permissions[3]}
                          </span>
                        </div>
                        <span className="path" title={region.pathname}>
                          {region.pathname || '(anonymous)'}
                        </span>
                        <span className="size">{formatBytes(region.size)}</span>
                        <span className={`type ${region.region_type}`}>
                          {region.region_type}
                        </span>
                      </motion.div>
                    ))}
                  </div>
                )}
              </motion.div>
            </>
          ) : (
            <div className="card" style={{ 
              display: 'flex', 
              flexDirection: 'column',
              alignItems: 'center', 
              justifyContent: 'center',
              minHeight: '400px',
              color: 'var(--text-muted)'
            }}>
              <AlertCircle size={48} style={{ marginBottom: 'var(--spacing-lg)', opacity: 0.5 }} />
              <p style={{ fontSize: '1.1rem', fontWeight: '500' }}>No process selected</p>
              <p style={{ fontSize: '0.9rem' }}>Select a process from the list to view its memory regions</p>
            </div>
          )}
        </div>
      </div>
    </div>
  )
}

export default ProcessView
