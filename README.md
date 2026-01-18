# Virtual Memory Visualization Tool

An interactive visualization tool for understanding virtual memory concepts in operating systems. Built with a C backend for core OS logic and a React frontend for beautiful visualizations.

## 🎯 Features

### Core Functionality
- **Process Memory Map Viewer** - Visualize memory regions (heap, stack, code, libraries)
- **Address Translation** - Translate virtual addresses to physical addresses
- **4-Level Page Table Walk** - Step-by-step visualization of x86_64 paging
- **TLB Simulator** - Interactive Translation Lookaside Buffer with LRU replacement
- **Memory Statistics** - Real-time system and process memory info
- **Learn Mode** - Educational content explaining OS concepts

### Technical Highlights
- **C Backend** - Reads directly from Linux `/proc` filesystem
- **Interactive Shell** - Full CLI for terminal demos
- **React Frontend** - Modern UI with animations and dark theme
- **REST API** - Flask server bridging frontend and backend

## 📁 Project Structure

```
virtual-memory-visualizer/
├── backend/                    # C Backend
│   ├── src/
│   │   ├── vmem_types.h       # Data structures
│   │   ├── proc_reader.c/h    # /proc filesystem reader
│   │   ├── addr_translate.c/h # Address translation
│   │   ├── tlb_sim.c/h        # TLB simulation
│   │   ├── json_output.c/h    # JSON serialization
│   │   └── vmem_shell.c       # Interactive CLI
│   └── Makefile
│
├── api/                        # Python API
│   ├── app.py                 # Flask server
│   └── requirements.txt
│
└── frontend/                   # React Frontend
    ├── src/
    │   ├── pages/
    │   │   ├── Dashboard.jsx
    │   │   ├── ProcessView.jsx
    │   │   ├── AddressTranslator.jsx
    │   │   ├── TLBSimulator.jsx
    │   │   └── Learn.jsx
    │   ├── utils/api.js
    │   ├── App.jsx
    │   └── index.css
    └── package.json
```

## 🚀 Quick Start

### Prerequisites
- **Windows with WSL** (Ubuntu/Fedora) OR **Native Linux**
- **GCC** - C compiler (in WSL/Linux)
- **Python 3.8+** with Flask (in WSL/Linux)
- **Node.js 18+** - For React frontend (Windows or WSL)

---

## 🖥️ Running on Windows with WSL

### 1. Build C Backend (in WSL)

```powershell
# From PowerShell - build the C backend in WSL
wsl -e bash -c "cd '/mnt/c/path/to/virtual-memory-visualizer/backend' && make"
```

Or open WSL terminal directly:
```bash
cd /mnt/c/path/to/virtual-memory-visualizer/backend
make
```

### 2. Test CLI in WSL

```bash
# In WSL terminal
cd /mnt/c/path/to/virtual-memory-visualizer/backend
./bin/vmem_shell

# Or with sudo for full pagemap access
sudo ./bin/vmem_shell
```

Example session:
```
vmem> ps
vmem> select 1234
vmem> maps
vmem> translate 0x7fff00010000
vmem> pagewalk 0x7fff00010000
vmem> tlb init 16
vmem> tlb access 0x7fff00010000
vmem> tlb status
```

### 3. Install Python Dependencies (WSL)

For **Fedora WSL**:
```bash
sudo dnf install -y python3-pip python3-flask python3-flask-cors
```

For **Ubuntu WSL**:
```bash
sudo apt update
sudo apt install -y python3-pip python3-flask
pip3 install flask-cors
```

### 4. Start API Server (WSL)

```bash
# In WSL terminal
cd /mnt/c/path/to/virtual-memory-visualizer/api
python3 app.py
```

Server runs on `http://localhost:5000`

### 5. Start React Frontend (Windows PowerShell)

```powershell
# In a new PowerShell window
cd C:\path\to\virtual-memory-visualizer\frontend
npm install
npm run dev
```

Frontend runs on `http://localhost:3000`

---

## 🐧 Running on Native Linux

### 1. Build C Backend

```bash
cd backend
make
```

This creates `bin/vmem_shell` - the main executable.

### 2. Test CLI (Terminal Demo)

```bash
# Run with sudo for full pagemap access
sudo ./bin/vmem_shell
```

### 3. Start API Server

```bash
cd api
pip install -r requirements.txt
python app.py
```

Server runs on `http://localhost:5000`

### 4. Start React Frontend

```bash
cd frontend
npm install
npm run dev
```

Frontend runs on `http://localhost:3000`

## 📖 Usage Guide

### Dashboard
- View system memory statistics
- See top processes by memory usage
- Check API connection status

### Process Memory
- Select any running process
- View all memory regions (stack, heap, code, libs)
- See permissions (rwx) and sizes

### Address Translator
1. Select a process
2. Enter a virtual address (e.g., `0x7fff00010000`)
3. Click "Translate Address"
4. See the full 4-level page table walk
5. View binary address breakdown

### TLB Simulator
1. Initialize TLB with desired size (e.g., 16)
2. Enter addresses to access
3. Watch hit/miss indicators
4. View hit rate statistics
5. See LRU replacement in action

### Learn Mode
- Study virtual memory concepts
- Understand address translation
- Learn about page tables, TLB, page faults

## 🎓 Teacher Demo Script

1. **Show C code structure:**
   ```bash
   cat backend/src/addr_translate.c
   cat backend/src/tlb_sim.c
   ```

2. **Run terminal demo:**
   ```bash
   sudo ./backend/bin/vmem_shell
   vmem> ps
   vmem> select <firefox_pid>
   vmem> maps
   vmem> pagewalk 0x7fff00010000
   ```

3. **Show web interface:**
   - Open http://localhost:3000
   - Navigate through pages
   - Demonstrate address translation
   - Show TLB simulator

## 🔧 API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/processes` | List all processes |
| GET | `/api/process/<pid>/maps` | Get memory regions |
| GET | `/api/process/<pid>/translate/<addr>` | Translate address |
| GET | `/api/process/<pid>/stats` | Get memory stats |
| GET | `/api/system/memory` | System memory info |
| POST | `/api/tlb/init` | Initialize TLB |
| POST | `/api/tlb/access` | Access TLB |
| GET | `/api/tlb/status` | Get TLB state |

## 🧪 OS Concepts Demonstrated

1. **Virtual Memory** - Process isolation, address abstraction
2. **Page Tables** - Hierarchical 4-level structure (x86_64)
3. **Address Translation** - VPN → PFN mapping
4. **TLB** - Translation cache with replacement policies
5. **Page Faults** - Minor vs Major faults
6. **Memory Regions** - Stack, heap, code, shared libraries
7. **Memory Protection** - Read/Write/Execute permissions

## 📜 License

MIT License - Free for educational use

## 👥 Team

OS Lab Project - Virtual Memory Visualization Tool
