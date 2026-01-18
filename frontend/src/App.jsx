import { useState } from 'react'
import { BrowserRouter, Routes, Route, NavLink } from 'react-router-dom'
import { 
  LayoutDashboard, 
  Cpu, 
  Map, 
  ArrowRightLeft, 
  Database, 
  Activity,
  BookOpen,
  Zap,
  HardDrive
} from 'lucide-react'

import Dashboard from './pages/Dashboard'
import ProcessView from './pages/ProcessView'
import AddressTranslator from './pages/AddressTranslator'
import TLBSimulator from './pages/TLBSimulator'
import DemandPaging from './pages/DemandPaging'
import Learn from './pages/Learn'

function App() {
  return (
    <BrowserRouter>
      <div className="app-container">
        {/* Sidebar */}
        <aside className="sidebar">
          <div className="sidebar-logo">
            <div className="logo-icon">
              <Zap size={20} />
            </div>
            <h1>VMem Visualizer</h1>
          </div>

          <nav>
            <div className="nav-section">
              <div className="nav-section-title">Overview</div>
              <NavLink to="/" className={({ isActive }) => `nav-link ${isActive ? 'active' : ''}`}>
                <LayoutDashboard size={18} />
                Dashboard
              </NavLink>
            </div>

            <div className="nav-section">
              <div className="nav-section-title">Analysis</div>
              <NavLink to="/process" className={({ isActive }) => `nav-link ${isActive ? 'active' : ''}`}>
                <Cpu size={18} />
                Process Memory
              </NavLink>
              <NavLink to="/translate" className={({ isActive }) => `nav-link ${isActive ? 'active' : ''}`}>
                <ArrowRightLeft size={18} />
                Address Translator
              </NavLink>
            </div>

            <div className="nav-section">
              <div className="nav-section-title">Simulation</div>
              <NavLink to="/tlb" className={({ isActive }) => `nav-link ${isActive ? 'active' : ''}`}>
                <Database size={18} />
                TLB Simulator
              </NavLink>
              <NavLink to="/paging" className={({ isActive }) => `nav-link ${isActive ? 'active' : ''}`}>
                <HardDrive size={18} />
                Demand Paging
              </NavLink>
            </div>

            <div className="nav-section">
              <div className="nav-section-title">Resources</div>
              <NavLink to="/learn" className={({ isActive }) => `nav-link ${isActive ? 'active' : ''}`}>
                <BookOpen size={18} />
                Learn Concepts
              </NavLink>
            </div>
          </nav>

          <div style={{ marginTop: 'auto', padding: '1rem', borderTop: '1px solid var(--border-primary)' }}>
            <div style={{ fontSize: '0.75rem', color: 'var(--text-muted)' }}>
              OS Lab Project
              <br />
              Virtual Memory Visualization
            </div>
          </div>
        </aside>

        {/* Main content */}
        <main className="main-content">
          <Routes>
            <Route path="/" element={<Dashboard />} />
            <Route path="/process" element={<ProcessView />} />
            <Route path="/translate" element={<AddressTranslator />} />
            <Route path="/tlb" element={<TLBSimulator />} />
            <Route path="/paging" element={<DemandPaging />} />
            <Route path="/learn" element={<Learn />} />
          </Routes>
        </main>
      </div>
    </BrowserRouter>
  )
}

export default App
