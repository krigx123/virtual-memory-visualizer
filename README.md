# Virtual Memory Visualization Tool

An interactive visualization tool for understanding virtual memory concepts in operating systems. Built with a C backend for core OS logic and a React frontend for beautiful visualizations.

![OS Lab Project](https://img.shields.io/badge/OS-Lab%20Project-blue)
![C Backend](https://img.shields.io/badge/Backend-C-green)
![React Frontend](https://img.shields.io/badge/Frontend-React-61DAFB)

## 🎯 Features

### Core Functionality
- **Process Memory Map Viewer** - Visualize memory regions (heap, stack, code, libraries)
- **Address Translation** - Translate virtual addresses to physical addresses
- **4-Level Page Table Walk** - Step-by-step visualization of x86_64 paging
- **TLB Simulator** - Interactive TLB with LRU, FIFO, Random, and Clock replacement
- **Demand Paging Simulator** - Physical memory frames, page faults, replacement policies
- **Memory Playground** - Active OS interaction with mmap(), mlock(), madvise()
- **Memory Statistics** - Real-time system and process memory info
- **Learn Mode** - Educational content on 8 OS concepts

### Technical Highlights
- **C Backend** - Reads directly from Linux `/proc` filesystem
- **Interactive Shell** - Full CLI for terminal demos with TLB and Paging simulation
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
│   │   ├── tlb_sim.c/h        # TLB simulation (LRU/FIFO/Random/Clock)
│   │   ├── json_output.c/h    # JSON serialization
│   │   └── vmem_shell.c       # Interactive CLI + Paging simulator
│   └── Makefile
│
├── api/                        # Python API
│   ├── app.py                 # Flask server (TLB + Paging state)
│   └── requirements.txt
│
└── frontend/                   # React Frontend
    ├── src/
    │   ├── pages/
    │   │   ├── Dashboard.jsx       # System overview
    │   │   ├── ProcessView.jsx     # Memory regions
    │   │   ├── AddressTranslator.jsx # Page table walk
    │   │   ├── TLBSimulator.jsx    # TLB simulation
    │   │   ├── DemandPaging.jsx    # Demand paging simulator
    │   │   ├── MemoryPlayground.jsx # Active OS interaction
    │   │   └── Learn.jsx           # OS concepts (8 topics)
    │   ├── utils/api.js
    │   ├── App.jsx
    │   └── index.css
    └── package.json
```

---

## 🚀 Quick Start

### Prerequisites
- **Windows with WSL** (Ubuntu/Fedora) OR **Native Linux**
- **GCC** - C compiler (in WSL/Linux)
- **Python 3.8+** with Flask (in WSL/Linux)
- **Node.js 18+** - For React frontend (Windows or WSL)

---

## 🖥️ Running on Windows with WSL

### Step 1: Build C Backend (in WSL)

**Option A - From PowerShell:**
```powershell
wsl -e bash -c "cd '/mnt/c/Users/YourName/path/to/virtual-memory-visualizer/backend' && make"
```

**Option B - In WSL Terminal:**
```bash
cd /mnt/c/Users/YourName/path/to/virtual-memory-visualizer/backend
make
```

Expected output:
```
gcc -Wall -Wextra -g -O2 -c src/addr_translate.c -o obj/addr_translate.o
...
Build complete: bin/vmem_shell
```

### Step 2: Test CLI in WSL

```bash
cd /mnt/c/Users/YourName/path/to/virtual-memory-visualizer/backend
./bin/vmem_shell    # Basic mode
sudo ./bin/vmem_shell   # Full pagemap access
```

### Step 3: Install Python Dependencies (WSL)

**For Fedora WSL:**
```bash
sudo dnf install -y python3-pip python3-flask python3-flask-cors
```

**For Ubuntu WSL:**
```bash
sudo apt update
sudo apt install -y python3-pip python3-flask
pip3 install flask-cors
```

### Step 4: Start API Server (WSL Terminal)

```bash
cd /mnt/c/Users/YourName/path/to/virtual-memory-visualizer/api
python3 app.py
```
Server runs on `http://localhost:5000`

### Step 5: Start React Frontend (Windows PowerShell)

```powershell
cd C:\path\to\virtual-memory-visualizer\frontend
npm install
npm run dev
```
Frontend runs on `http://localhost:3000`

---

## 🐧 Running on Native Linux

### Step 1: Build C Backend

```bash
cd backend
make
```

### Step 2: Test CLI

```bash
sudo ./bin/vmem_shell
```

### Step 3: Install Python Dependencies

```bash
cd api
pip install -r requirements.txt
```

### Step 4: Start API Server

```bash
python app.py
```
Server runs on `http://localhost:5000`

### Step 5: Start React Frontend

```bash
cd frontend
npm install
npm run dev
```
Frontend runs on `http://localhost:3000`

---

## 💻 Example CLI Sessions

### Process and Memory Analysis

```
╔══════════════════════════════════════════════════════════════╗
║        Virtual Memory Visualization Tool - CLI v1.0          ║
╚══════════════════════════════════════════════════════════════╝

vmem> ps
  PID   NAME              STATE  MEMORY       UID
  1     systemd           S      13.0 MB      0
  234   NetworkManager    S      15.7 MB      0
  1892  firefox           S      524.3 MB     1000
  2541  python3           S      42.1 MB      1000

vmem> select 1892
[OK] Selected process 1892 (firefox)

vmem> maps
MEMORY REGIONS FOR PID 1892 (firefox)
==================================================
  START ADDRESS     END ADDRESS       SIZE     PERM  TYPE           NAME
  0x55a4b2c00000    0x55a4b2d40000    1.3 MB   r-xp  Code           /usr/lib/firefox/firefox
  0x55a4b3000000    0x55a4b5000000    32.0 MB  rw-p  Heap           [heap]
  0x7f8a40000000    0x7f8a40021000    132 KB   rw-p  Data           [anon]
  0x7ffca8700000    0x7ffca8721000    132 KB   rw-p  Stack          [stack]

vmem> translate 0x55a4b2c00000
ADDRESS TRANSLATION
====================
Virtual Address:  0x55a4b2c00000
Virtual Page Num: 0x55a4b2c00
Physical Frame:   0x1a4b2c00
Physical Address: 0x1a4b2c00000
Page Offset:      0x000

vmem> pagewalk 0x55a4b2c00000
4-LEVEL PAGE TABLE WALK
========================
Virtual Address: 0x55a4b2c00000 (binary breakdown below)

PML4 Index:    0x0AB (bits 47-39)  → PML4[171] 
PDPT Index:    0x125 (bits 38-30)  → PDPT[293]
PD Index:      0x016 (bits 29-21)  → PD[22]
PT Index:      0x000 (bits 20-12)  → PT[0]
Page Offset:   0x000 (bits 11-0)

Result: ✓ Valid mapping found
Physical Frame: 0x1a4b2c00
```

### TLB Simulator Session

```
vmem> tlb init 8 LRU
[OK] TLB initialized with 8 entries (LRU replacement)

vmem> tlb access 0x1000
[TLB MISS] VPN 0x1 not found
[TLB INSERT] VPN 0x1 -> PFN 0x1 (simulated)

vmem> tlb access 0x2000
[TLB MISS] VPN 0x2 not found
[TLB INSERT] VPN 0x2 -> PFN 0x2 (simulated)

vmem> tlb access 0x1000
[TLB HIT] VPN 0x1 -> PFN 0x1

vmem> tlb status
TLB STATUS
==========
Size: 8 entries | Policy: LRU

┌───────┬─────────┬─────────┬───────┬─────────────┐
│ Index │   VPN   │   PFN   │ Valid │ Last Access │
├───────┼─────────┼─────────┼───────┼─────────────┤
│   0   │  0x1    │  0x1    │   ✓   │     2       │
│   1   │  0x2    │  0x2    │   ✓   │     1       │
│   2   │   -     │   -     │   ✗   │     -       │
│   3   │   -     │   -     │   ✗   │     -       │
│   4   │   -     │   -     │   ✗   │     -       │
│   5   │   -     │   -     │   ✗   │     -       │
│   6   │   -     │   -     │   ✗   │     -       │
│   7   │   -     │   -     │   ✗   │     -       │
└───────┴─────────┴─────────┴───────┴─────────────┘

Statistics:
  Hits:     1
  Misses:   2
  Hit Rate: 33.3%
```

### Demand Paging Simulator Session

```
vmem> paging init 4 FIFO
[OK] Paging simulator initialized with 4 frames (FIFO replacement)

vmem> paging access 0x1000
[PAGE FAULT] VPN 0x1 not in memory, loaded into Frame 0

vmem> paging access 0x2000
[PAGE FAULT] VPN 0x2 not in memory, loaded into Frame 1

vmem> paging access 0x3000
[PAGE FAULT] VPN 0x3 not in memory, loaded into Frame 2

vmem> paging access 0x4000
[PAGE FAULT] VPN 0x4 not in memory, loaded into Frame 3

vmem> paging access 0x5000
[PAGE FAULT] VPN 0x5 not in memory, evicted VPN 0x1 from Frame 0

vmem> paging access 0x1000
[PAGE FAULT] VPN 0x1 not in memory, evicted VPN 0x2 from Frame 1

vmem> paging status

PAGING SIMULATOR STATUS
=======================
Frames: 4 | Policy: FIFO

Physical Memory Frames:
┌───────┬─────────┬─────────┬─────────────┐
│ Frame │   VPN   │ Loaded  │ Last Access │
├───────┼─────────┼─────────┼─────────────┤
│    0  │  0x5    │      4  │       4     │
│    1  │  0x1    │      5  │       5     │
│    2  │  0x3    │      2  │       2     │
│    3  │  0x4    │      3  │       3     │
└───────┴─────────┴─────────┴─────────────┘

Statistics:
  Page Hits:   0
  Page Faults: 6
  Hit Rate:    0.0%
```

### Memory Playground Session (Active OS Interaction)

```
vmem> mem alloc 50
[OK] Allocated 50 MB (Region #0, 12800 pages touched)
     Address: 0x7f8a40000000

vmem> mem alloc 25
[OK] Allocated 25 MB (Region #1, 6400 pages touched)
     Address: 0x7f8a10000000

vmem> mem lock 0
[OK] Locked Region #0 (50 MB) - cannot be swapped out

vmem> mem advise 1 WILLNEED
[OK] Applied WILLNEED hint to Region #1

vmem> mem status

MEMORY PLAYGROUND STATUS
========================

Active Regions: 2 / 32
Total Allocated: 75 MB
Total Locked: 50 MB

┌────┬────────────────────┬──────────┬────────┬────────────┐
│ ID │      Address       │   Size   │ Locked │   Advice   │
├────┼────────────────────┼──────────┼────────┼────────────┤
│  0 │   0x7f8a40000000   │   50 MB  │   ✓   │ NORMAL     │
│  1 │   0x7f8a10000000   │   25 MB  │   ✗   │ WILLNEED   │
└────┴────────────────────┴──────────┴────────┴────────────┘

vmem> mem free 1
[OK] Freed Region #1 (25 MB)

vmem> mem reset
[OK] Freed 1 regions (50 MB total)
```

---

## 📖 Usage Guide

### Dashboard
- View system memory statistics (total, free, cached)
- See top processes by memory usage
- Visual memory distribution bar

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
1. Select replacement policy (LRU, FIFO, Random, Clock)
2. Initialize TLB with desired size
3. Use quick access buttons or enter addresses
4. Watch hit/miss indicators with animations
5. View statistics and reconfigure as needed

### Demand Paging Simulator
1. Configure number of frames (2-16)
2. Select replacement policy
3. Access pages using quick buttons or sequence input
4. Watch page faults and frame allocation
5. See eviction when memory is full

### Learn Mode
Study 8 OS concepts:
- Virtual Memory
- Address Translation (VPN/PFN explained)
- 4-Level Page Table (x86_64)
- Translation Lookaside Buffer (TLB)
- Demand Paging
- Page Replacement Algorithms
- Page Faults
- Memory Regions

---

## 🎓 Teacher Demo Script

### 1. Show C code structure
```bash
cat backend/src/addr_translate.c   # Page table walk logic
cat backend/src/tlb_sim.c          # TLB with 4 policies
cat backend/src/vmem_shell.c       # Paging simulator
```

### 2. Run terminal demo
```bash
sudo ./backend/bin/vmem_shell

# Process analysis
vmem> ps
vmem> select <firefox_pid>
vmem> maps
vmem> pagewalk 0x7fff00010000

# TLB demo
vmem> tlb init 8 LRU
vmem> tlb access 0x1000
vmem> tlb access 0x2000
vmem> tlb access 0x1000   # HIT!
vmem> tlb status

# Paging demo
vmem> paging init 4 FIFO
vmem> paging access 0x1000
vmem> paging access 0x2000
vmem> paging access 0x3000
vmem> paging access 0x4000
vmem> paging access 0x5000  # Eviction!
vmem> paging status
```

### 3. Show web interface
- Open http://localhost:3000
- Navigate through all pages
- Demonstrate TLB with different policies
- Show Demand Paging with eviction

---

## 🔧 API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/health` | API health check |
| GET | `/api/processes` | List all processes |
| GET | `/api/process/<pid>/maps` | Get memory regions |
| GET | `/api/process/<pid>/translate/<addr>` | Translate address |
| GET | `/api/process/<pid>/stats` | Get memory stats |
| GET | `/api/system/memory` | System memory info |
| POST | `/api/tlb/init` | Initialize TLB `{size, policy}` |
| POST | `/api/tlb/access` | Access TLB `{vpn, pfn}` |
| GET | `/api/tlb/status` | Get TLB state |
| POST | `/api/tlb/flush` | Flush TLB |
| POST | `/api/paging/init` | Initialize paging `{frames, policy}` |
| POST | `/api/paging/access` | Access page `{vpn}` |
| GET | `/api/paging/status` | Get paging state |
| POST | `/api/paging/sequence` | Run sequence `{addresses: [...]}` |

---

## 🧪 OS Concepts Demonstrated

| Concept | Where | Algorithms |
|---------|-------|------------|
| Virtual Memory | Dashboard, Process View | - |
| Page Tables | Address Translator | 4-level walk |
| Address Translation | Address Translator | VPN → PFN |
| TLB | TLB Simulator | LRU, FIFO, Random, Clock |
| Demand Paging | Demand Paging Sim | On-demand loading |
| Page Replacement | Both Simulators | LRU, FIFO, Random, Clock |
| Page Faults | Stats, Simulators | Major/Minor |
| Memory Regions | Process View | Stack, Heap, Code |
| Memory Protection | Process View | rwx permissions |

---

## 📜 License

MIT License - Free for educational use

## 👥 Team

OS Lab Project - Virtual Memory Visualization Tool
