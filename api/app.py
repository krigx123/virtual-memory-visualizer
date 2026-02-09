"""
Virtual Memory Visualization Tool - Flask API Server

This server acts as a bridge between the React frontend and the C backend.
It runs the compiled vmem_shell executable with --json flag and returns results.
"""

from flask import Flask, jsonify, request
from flask_cors import CORS
import subprocess
import json
import os

app = Flask(__name__)
CORS(app)  # Enable CORS for React frontend

# Path to the compiled C executable
BACKEND_DIR = os.path.join(os.path.dirname(__file__), '..', 'backend')
VMEM_SHELL = os.path.join(BACKEND_DIR, 'bin', 'vmem_shell')

# TLB state (simulated in Python for API since C shell is stateless per call)
tlb_state = {
    'initialized': False,
    'size': 16,
    'policy': 'LRU',
    'entries': [],
    'hits': 0,
    'misses': 0
}

# Demand Paging simulation state
paging_state = {
    'initialized': False,
    'num_frames': 4,           # Number of physical memory frames
    'policy': 'LRU',           # Replacement policy
    'frames': [],              # List of frames: [{'vpn': x, 'loaded_at': t, 'last_access': t}, ...]
    'page_table': {},          # VPN -> frame_index mapping
    'page_faults': 0,
    'page_hits': 0,
    'disk_reads': 0,
    'access_counter': 0,
    'access_history': [],      # Recent accesses for visualization
    'clock_hand': 0            # For Clock algorithm
}


def run_vmem_command(*args):
    """
    Run the vmem_shell with --json flag and parse the result.
    """
    try:
        cmd = ['sudo', VMEM_SHELL, '--json'] + list(args)
        result = subprocess.run(
            cmd,
            capture_output=True,
            timeout=30
        )
        
        # Decode with error replacement for non-UTF-8 bytes
        stdout = result.stdout.decode('utf-8', errors='replace')
        stderr = result.stderr.decode('utf-8', errors='replace')
        
        if result.returncode != 0:
            return {'success': False, 'error': stderr or 'Command failed'}
        
        return json.loads(stdout)
    except subprocess.TimeoutExpired:
        return {'success': False, 'error': 'Command timed out'}
    except json.JSONDecodeError as e:
        return {'success': False, 'error': f'Invalid JSON response from backend: {str(e)}'}
    except FileNotFoundError:
        return {'success': False, 'error': f'Backend not found at {VMEM_SHELL}. Please compile first.'}
    except Exception as e:
        return {'success': False, 'error': str(e)}


# =============================================================================
# Process Endpoints
# =============================================================================

@app.route('/api/processes', methods=['GET'])
def get_processes():
    """Get list of all running processes."""
    return jsonify(run_vmem_command('processes'))


@app.route('/api/process/<int:pid>', methods=['GET'])
def get_process(pid):
    """Get information about a specific process."""
    result = run_vmem_command('processes')
    if not result.get('success'):
        return jsonify(result)
    
    for proc in result.get('data', []):
        if proc['pid'] == pid:
            return jsonify({'success': True, 'data': proc})
    
    return jsonify({'success': False, 'error': f'Process {pid} not found'})


# =============================================================================
# Memory Endpoints
# =============================================================================

@app.route('/api/process/<int:pid>/maps', methods=['GET'])
def get_memory_maps(pid):
    """Get memory regions for a process."""
    return jsonify(run_vmem_command('maps', str(pid)))


@app.route('/api/process/<int:pid>/translate/<address>', methods=['GET'])
def translate_address(pid, address):
    """Translate virtual address to physical address."""
    return jsonify(run_vmem_command('translate', str(pid), address))


@app.route('/api/process/<int:pid>/stats', methods=['GET'])
def get_memory_stats(pid):
    """Get memory statistics for a process."""
    return jsonify(run_vmem_command('stats', str(pid)))


@app.route('/api/system/memory', methods=['GET'])
def get_system_memory():
    """Get system-wide memory information."""
    return jsonify(run_vmem_command('sysinfo'))


# =============================================================================
# TLB Simulation Endpoints (Stateful - maintained in Python)
# =============================================================================

@app.route('/api/tlb/init', methods=['POST'])
def init_tlb():
    """Initialize TLB simulator."""
    global tlb_state
    
    data = request.get_json() or {}
    size = data.get('size', 16)
    policy = data.get('policy', 'LRU')
    
    if size < 1 or size > 256:
        return jsonify({'success': False, 'error': 'Size must be between 1 and 256'})
    
    tlb_state = {
        'initialized': True,
        'size': size,
        'policy': policy,
        'entries': [None] * size,
        'hits': 0,
        'misses': 0,
        'access_counter': 0
    }
    
    return jsonify({'success': True, 'message': f'TLB initialized with {size} entries'})


@app.route('/api/tlb/lookup', methods=['POST'])
def tlb_lookup():
    """Lookup address in TLB."""
    global tlb_state
    
    if not tlb_state['initialized']:
        return jsonify({'success': False, 'error': 'TLB not initialized'})
    
    data = request.get_json() or {}
    vpn = data.get('vpn')
    
    if vpn is None:
        return jsonify({'success': False, 'error': 'VPN required'})
    
    # Convert to int if string
    if isinstance(vpn, str):
        vpn = int(vpn, 16) if vpn.startswith('0x') else int(vpn)
    
    # Look for entry
    for entry in tlb_state['entries']:
        if entry is not None and entry['vpn'] == vpn and entry['valid']:
            tlb_state['hits'] += 1
            entry['last_access'] = tlb_state['access_counter']
            tlb_state['access_counter'] += 1
            return jsonify({
                'success': True,
                'hit': True,
                'pfn': entry['pfn'],
                'vpn': vpn
            })
    
    tlb_state['misses'] += 1
    return jsonify({'success': True, 'hit': False, 'vpn': vpn})


@app.route('/api/tlb/insert', methods=['POST'])
def tlb_insert():
    """Insert entry into TLB."""
    global tlb_state
    
    if not tlb_state['initialized']:
        return jsonify({'success': False, 'error': 'TLB not initialized'})
    
    data = request.get_json() or {}
    vpn = data.get('vpn')
    pfn = data.get('pfn')
    
    if vpn is None or pfn is None:
        return jsonify({'success': False, 'error': 'VPN and PFN required'})
    
    # Convert to int if string
    if isinstance(vpn, str):
        vpn = int(vpn, 16) if vpn.startswith('0x') else int(vpn)
    if isinstance(pfn, str):
        pfn = int(pfn, 16) if pfn.startswith('0x') else int(pfn)
    
    # Find empty slot or victim
    empty_slot = None
    for i, entry in enumerate(tlb_state['entries']):
        if entry is None or not entry['valid']:
            empty_slot = i
            break
    
    if empty_slot is None:
        # Apply replacement policy (LRU)
        min_access = float('inf')
        victim = 0
        for i, entry in enumerate(tlb_state['entries']):
            if entry and entry['last_access'] < min_access:
                min_access = entry['last_access']
                victim = i
        empty_slot = victim
    
    tlb_state['entries'][empty_slot] = {
        'vpn': vpn,
        'pfn': pfn,
        'valid': True,
        'last_access': tlb_state['access_counter']
    }
    tlb_state['access_counter'] += 1
    
    return jsonify({'success': True, 'message': 'Entry inserted', 'slot': empty_slot})


@app.route('/api/tlb/access', methods=['POST'])
def tlb_access():
    """Access address (lookup + insert on miss)."""
    global tlb_state
    
    if not tlb_state['initialized']:
        return jsonify({'success': False, 'error': 'TLB not initialized'})
    
    data = request.get_json() or {}
    vpn = data.get('vpn')
    pfn = data.get('pfn')  # PFN to insert on miss
    
    if vpn is None:
        return jsonify({'success': False, 'error': 'VPN required'})
    
    # First try lookup
    if isinstance(vpn, str):
        vpn = int(vpn, 16) if vpn.startswith('0x') else int(vpn)
    
    for entry in tlb_state['entries']:
        if entry is not None and entry['vpn'] == vpn and entry['valid']:
            tlb_state['hits'] += 1
            entry['last_access'] = tlb_state['access_counter']
            tlb_state['access_counter'] += 1
            return jsonify({
                'success': True,
                'hit': True,
                'pfn': entry['pfn'],
                'vpn': vpn
            })
    
    # Miss - insert if PFN provided
    tlb_state['misses'] += 1
    
    if pfn is not None:
        if isinstance(pfn, str):
            pfn = int(pfn, 16) if pfn.startswith('0x') else int(pfn)
        
        # Find empty slot first
        empty_slot = None
        for i, entry in enumerate(tlb_state['entries']):
            if entry is None or not entry['valid']:
                empty_slot = i
                break
        
        if empty_slot is None:
            # Apply replacement policy based on selected algorithm
            policy = tlb_state.get('policy', 'LRU')
            
            if policy == 'LRU':
                # Least Recently Used - evict entry with oldest last_access
                min_access = float('inf')
                victim = 0
                for i, entry in enumerate(tlb_state['entries']):
                    if entry and entry['last_access'] < min_access:
                        min_access = entry['last_access']
                        victim = i
                empty_slot = victim
                
            elif policy == 'FIFO':
                # First In First Out - evict entry with oldest insert_time
                min_insert = float('inf')
                victim = 0
                for i, entry in enumerate(tlb_state['entries']):
                    if entry and entry.get('insert_time', 0) < min_insert:
                        min_insert = entry.get('insert_time', 0)
                        victim = i
                empty_slot = victim
                
            elif policy == 'RANDOM':
                # Random replacement
                import random
                empty_slot = random.randint(0, tlb_state['size'] - 1)
                
            elif policy == 'CLOCK':
                # Clock (Second Chance) algorithm
                clock_hand = tlb_state.get('clock_hand', 0)
                while True:
                    entry = tlb_state['entries'][clock_hand]
                    if entry and not entry.get('reference_bit', False):
                        empty_slot = clock_hand
                        tlb_state['clock_hand'] = (clock_hand + 1) % tlb_state['size']
                        break
                    if entry:
                        entry['reference_bit'] = False
                    clock_hand = (clock_hand + 1) % tlb_state['size']
                
            else:
                # Default to LRU
                min_access = float('inf')
                victim = 0
                for i, entry in enumerate(tlb_state['entries']):
                    if entry and entry['last_access'] < min_access:
                        min_access = entry['last_access']
                        victim = i
                empty_slot = victim
        
        tlb_state['entries'][empty_slot] = {
            'vpn': vpn,
            'pfn': pfn,
            'valid': True,
            'last_access': tlb_state['access_counter'],
            'insert_time': tlb_state['access_counter'],
            'reference_bit': True
        }
        tlb_state['access_counter'] += 1
    
    return jsonify({
        'success': True,
        'hit': False,
        'vpn': vpn,
        'inserted': pfn is not None
    })


@app.route('/api/tlb/status', methods=['GET'])
def tlb_status():
    """Get TLB status and statistics."""
    if not tlb_state['initialized']:
        return jsonify({'success': False, 'error': 'TLB not initialized'})
    
    total = tlb_state['hits'] + tlb_state['misses']
    hit_rate = (tlb_state['hits'] / total * 100) if total > 0 else 0
    
    entries = []
    for i, entry in enumerate(tlb_state['entries']):
        if entry is not None:
            entries.append({
                'index': i,
                'vpn': hex(entry['vpn']),
                'pfn': hex(entry['pfn']),
                'valid': entry['valid'],
                'last_access': entry['last_access']
            })
        else:
            entries.append({
                'index': i,
                'vpn': None,
                'pfn': None,
                'valid': False,
                'last_access': 0
            })
    
    return jsonify({
        'success': True,
        'data': {
            'size': tlb_state['size'],
            'policy': tlb_state['policy'],
            'hits': tlb_state['hits'],
            'misses': tlb_state['misses'],
            'hit_rate': round(hit_rate, 2),
            'entries': entries
        }
    })


@app.route('/api/tlb/flush', methods=['POST'])
def tlb_flush():
    """Flush TLB."""
    global tlb_state
    
    if not tlb_state['initialized']:
        return jsonify({'success': False, 'error': 'TLB not initialized'})
    
    tlb_state['entries'] = [None] * tlb_state['size']
    
    return jsonify({'success': True, 'message': 'TLB flushed'})


@app.route('/api/tlb/reset', methods=['POST'])
def tlb_reset():
    """Reset TLB statistics."""
    global tlb_state
    
    if not tlb_state['initialized']:
        return jsonify({'success': False, 'error': 'TLB not initialized'})
    
    tlb_state['hits'] = 0
    tlb_state['misses'] = 0
    
    return jsonify({'success': True, 'message': 'Statistics reset'})


# =============================================================================
# Demand Paging Simulation Endpoints
# =============================================================================

@app.route('/api/paging/init', methods=['POST'])
def init_paging():
    """Initialize demand paging simulator."""
    global paging_state
    
    data = request.get_json() or {}
    num_frames = data.get('frames', 4)
    policy = data.get('policy', 'LRU')
    
    if num_frames < 1 or num_frames > 64:
        return jsonify({'success': False, 'error': 'Frames must be between 1 and 64'})
    
    if policy not in ['LRU', 'FIFO', 'RANDOM', 'CLOCK']:
        return jsonify({'success': False, 'error': 'Invalid policy. Use: LRU, FIFO, RANDOM, CLOCK'})
    
    paging_state = {
        'initialized': True,
        'num_frames': num_frames,
        'policy': policy,
        'frames': [None] * num_frames,  # Each frame: None or {'vpn': x, 'loaded_at': t, 'last_access': t, 'reference_bit': True}
        'page_table': {},  # VPN -> frame_index
        'page_faults': 0,
        'page_hits': 0,
        'disk_reads': 0,
        'access_counter': 0,
        'access_history': [],
        'clock_hand': 0
    }
    
    return jsonify({
        'success': True, 
        'message': f'Paging simulator initialized with {num_frames} frames ({policy})'
    })


@app.route('/api/paging/access', methods=['POST'])
def paging_access():
    """Access a page (may trigger page fault)."""
    global paging_state
    import random
    
    if not paging_state['initialized']:
        return jsonify({'success': False, 'error': 'Paging not initialized'})
    
    data = request.get_json() or {}
    vpn = data.get('vpn')
    
    if vpn is None:
        return jsonify({'success': False, 'error': 'VPN required'})
    
    # Convert string VPN to int
    if isinstance(vpn, str):
        vpn = int(vpn, 16) if vpn.startswith('0x') else int(vpn)
    
    result = {
        'vpn': vpn,
        'vpn_hex': hex(vpn),
        'page_fault': False,
        'evicted_vpn': None,
        'frame_index': None
    }
    
    # Check if page is already in memory
    if vpn in paging_state['page_table']:
        # PAGE HIT
        frame_idx = paging_state['page_table'][vpn]
        paging_state['page_hits'] += 1
        paging_state['frames'][frame_idx]['last_access'] = paging_state['access_counter']
        paging_state['frames'][frame_idx]['reference_bit'] = True
        result['hit'] = True
        result['frame_index'] = frame_idx
    else:
        # PAGE FAULT
        paging_state['page_faults'] += 1
        paging_state['disk_reads'] += 1
        result['page_fault'] = True
        result['hit'] = False
        
        # Find a free frame or evict one
        free_frame = None
        for i, frame in enumerate(paging_state['frames']):
            if frame is None:
                free_frame = i
                break
        
        if free_frame is not None:
            # Use free frame
            frame_idx = free_frame
        else:
            # Need to evict - apply replacement policy
            policy = paging_state['policy']
            
            if policy == 'LRU':
                # Find least recently used
                min_access = float('inf')
                victim = 0
                for i, frame in enumerate(paging_state['frames']):
                    if frame['last_access'] < min_access:
                        min_access = frame['last_access']
                        victim = i
                frame_idx = victim
                
            elif policy == 'FIFO':
                # Find oldest loaded page
                min_loaded = float('inf')
                victim = 0
                for i, frame in enumerate(paging_state['frames']):
                    if frame['loaded_at'] < min_loaded:
                        min_loaded = frame['loaded_at']
                        victim = i
                frame_idx = victim
                
            elif policy == 'RANDOM':
                frame_idx = random.randint(0, paging_state['num_frames'] - 1)
                
            elif policy == 'CLOCK':
                # Clock (Second Chance) algorithm
                while True:
                    frame = paging_state['frames'][paging_state['clock_hand']]
                    if not frame['reference_bit']:
                        frame_idx = paging_state['clock_hand']
                        paging_state['clock_hand'] = (paging_state['clock_hand'] + 1) % paging_state['num_frames']
                        break
                    # Give second chance
                    frame['reference_bit'] = False
                    paging_state['clock_hand'] = (paging_state['clock_hand'] + 1) % paging_state['num_frames']
            
            # Record evicted page
            evicted = paging_state['frames'][frame_idx]
            result['evicted_vpn'] = evicted['vpn']
            result['evicted_vpn_hex'] = hex(evicted['vpn'])
            # Remove from page table
            del paging_state['page_table'][evicted['vpn']]
        
        # Load new page into frame
        paging_state['frames'][frame_idx] = {
            'vpn': vpn,
            'loaded_at': paging_state['access_counter'],
            'last_access': paging_state['access_counter'],
            'reference_bit': True
        }
        paging_state['page_table'][vpn] = frame_idx
        result['frame_index'] = frame_idx
    
    # Record access history (keep last 50)
    paging_state['access_history'].append({
        'vpn': vpn,
        'vpn_hex': hex(vpn),
        'hit': result['hit'],
        'frame': result['frame_index'],
        'evicted': result.get('evicted_vpn_hex')
    })
    if len(paging_state['access_history']) > 50:
        paging_state['access_history'] = paging_state['access_history'][-50:]
    
    paging_state['access_counter'] += 1
    
    return jsonify({'success': True, **result})


@app.route('/api/paging/status', methods=['GET'])
def paging_status():
    """Get paging simulator status."""
    if not paging_state['initialized']:
        return jsonify({'success': False, 'error': 'Paging not initialized'})
    
    total = paging_state['page_hits'] + paging_state['page_faults']
    hit_rate = (paging_state['page_hits'] / total * 100) if total > 0 else 0
    
    # Format frames for display
    frames = []
    for i, frame in enumerate(paging_state['frames']):
        if frame is not None:
            frames.append({
                'index': i,
                'vpn': frame['vpn'],
                'vpn_hex': hex(frame['vpn']),
                'loaded_at': frame['loaded_at'],
                'last_access': frame['last_access'],
                'occupied': True
            })
        else:
            frames.append({
                'index': i,
                'vpn': None,
                'vpn_hex': None,
                'loaded_at': None,
                'last_access': None,
                'occupied': False
            })
    
    return jsonify({
        'success': True,
        'data': {
            'num_frames': paging_state['num_frames'],
            'policy': paging_state['policy'],
            'frames': frames,
            'page_faults': paging_state['page_faults'],
            'page_hits': paging_state['page_hits'],
            'hit_rate': round(hit_rate, 2),
            'disk_reads': paging_state['disk_reads'],
            'access_history': paging_state['access_history'][-20:]  # Last 20 accesses
        }
    })


@app.route('/api/paging/reset', methods=['POST'])
def paging_reset():
    """Reset paging statistics (keep frames)."""
    global paging_state
    
    if not paging_state['initialized']:
        return jsonify({'success': False, 'error': 'Paging not initialized'})
    
    paging_state['page_faults'] = 0
    paging_state['page_hits'] = 0
    paging_state['disk_reads'] = 0
    paging_state['access_history'] = []
    
    return jsonify({'success': True, 'message': 'Statistics reset'})


@app.route('/api/paging/sequence', methods=['POST'])
def paging_sequence():
    """Run a sequence of page accesses."""
    global paging_state
    import random
    
    if not paging_state['initialized']:
        return jsonify({'success': False, 'error': 'Paging not initialized'})
    
    data = request.get_json() or {}
    addresses = data.get('addresses', [])
    
    if not addresses:
        return jsonify({'success': False, 'error': 'Addresses list required'})
    
    results = []
    for addr in addresses:
        # Parse address
        if isinstance(addr, str):
            vpn = int(addr, 16) if addr.startswith('0x') else int(addr)
        else:
            vpn = int(addr)
        
        # Simulate access (reuse logic from paging_access)
        hit = vpn in paging_state['page_table']
        evicted = None  # Initialize evicted for access_history tracking
        
        if hit:
            frame_idx = paging_state['page_table'][vpn]
            paging_state['page_hits'] += 1
            paging_state['frames'][frame_idx]['last_access'] = paging_state['access_counter']
            paging_state['frames'][frame_idx]['reference_bit'] = True
            results.append({'vpn': hex(vpn), 'hit': True, 'frame': frame_idx, 'evicted': None})
        else:
            paging_state['page_faults'] += 1
            paging_state['disk_reads'] += 1
            
            # Find free or evict
            free_frame = None
            for i, frame in enumerate(paging_state['frames']):
                if frame is None:
                    free_frame = i
                    break
            
            evicted = None
            if free_frame is not None:
                frame_idx = free_frame
            else:
                policy = paging_state['policy']
                if policy == 'LRU':
                    victim = min(range(len(paging_state['frames'])), 
                                key=lambda i: paging_state['frames'][i]['last_access'])
                elif policy == 'FIFO':
                    victim = min(range(len(paging_state['frames'])), 
                                key=lambda i: paging_state['frames'][i]['loaded_at'])
                elif policy == 'RANDOM':
                    victim = random.randint(0, paging_state['num_frames'] - 1)
                else:  # CLOCK
                    while True:
                        frame = paging_state['frames'][paging_state['clock_hand']]
                        if not frame['reference_bit']:
                            victim = paging_state['clock_hand']
                            paging_state['clock_hand'] = (paging_state['clock_hand'] + 1) % paging_state['num_frames']
                            break
                        frame['reference_bit'] = False
                        paging_state['clock_hand'] = (paging_state['clock_hand'] + 1) % paging_state['num_frames']
                
                frame_idx = victim
                evicted = paging_state['frames'][frame_idx]['vpn']
                del paging_state['page_table'][evicted]
            
            paging_state['frames'][frame_idx] = {
                'vpn': vpn,
                'loaded_at': paging_state['access_counter'],
                'last_access': paging_state['access_counter'],
                'reference_bit': True
            }
            paging_state['page_table'][vpn] = frame_idx
            results.append({'vpn': hex(vpn), 'hit': False, 'frame': frame_idx, 'evicted': hex(evicted) if evicted else None})
        
        # Record access history (keep last 50)
        paging_state['access_history'].append({
            'vpn': vpn,
            'vpn_hex': hex(vpn),
            'hit': hit,
            'frame': frame_idx,
            'evicted': hex(evicted) if evicted else None
        })
        if len(paging_state['access_history']) > 50:
            paging_state['access_history'] = paging_state['access_history'][-50:]
        
        paging_state['access_counter'] += 1
    
    total = paging_state['page_hits'] + paging_state['page_faults']
    hit_rate = (paging_state['page_hits'] / total * 100) if total > 0 else 0
    
    return jsonify({
        'success': True,
        'results': results,
        'stats': {
            'page_faults': paging_state['page_faults'],
            'page_hits': paging_state['page_hits'],
            'hit_rate': round(hit_rate, 2)
        }
    })


# =============================================================================
# Health Check
# =============================================================================

@app.route('/api/health', methods=['GET'])
def health_check():
    """API health check."""
    backend_exists = os.path.exists(VMEM_SHELL)
    return jsonify({
        'success': True,
        'api': 'running',
        'backend': 'available' if backend_exists else 'not compiled'
    })


@app.route('/api/reset-all', methods=['POST'])
def reset_all():
    """Reset all simulators (TLB and Paging) to initial state."""
    global tlb_state, paging_state
    
    # Reset TLB
    tlb_state = {
        'initialized': False,
        'size': 16,
        'policy': 'LRU',
        'entries': [],
        'hits': 0,
        'misses': 0
    }
    
    # Reset Paging
    paging_state = {
        'initialized': False,
        'num_frames': 4,
        'policy': 'LRU',
        'frames': [],
        'page_table': {},
        'page_faults': 0,
        'page_hits': 0,
        'disk_reads': 0,
        'access_counter': 0,
        'access_history': [],
        'clock_hand': 0
    }
    
    return jsonify({
        'success': True,
    })


# =============================================================================
# Memory Playground (Active OS Interaction)
# =============================================================================

import mmap
import ctypes

# Playground state - tracks allocated memory regions
playground_state = {
    'allocations': [],      # List of allocated regions
    'total_allocated': 0,   # Total bytes allocated
    'total_locked': 0,      # Total bytes locked
    'page_faults_start': 0, # Page faults when started
}

# Load libc for mlock/munlock/madvise
try:
    libc = ctypes.CDLL('libc.so.6', use_errno=True)
    MLOCK_AVAILABLE = True
except:
    MLOCK_AVAILABLE = False


@app.route('/api/playground/allocate', methods=['POST'])
def playground_allocate():
    """Allocate a new memory region."""
    data = request.get_json() or {}
    size_mb = data.get('size_mb', 10)
    touch = data.get('touch', True)  # Touch pages to cause real allocation
    
    size_bytes = size_mb * 1024 * 1024
    
    try:
        # Create anonymous mmap (real memory allocation)
        mm = mmap.mmap(-1, size_bytes, mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS)
        
        region_id = len(playground_state['allocations'])
        
        # Touch pages if requested (causes actual page faults)
        pages_touched = 0
        if touch:
            page_size = 4096
            for offset in range(0, size_bytes, page_size):
                mm[offset] = 0x42  # Write to each page
                pages_touched += 1
        
        region = {
            'id': region_id,
            'size_mb': size_mb,
            'size_bytes': size_bytes,
            'locked': False,
            'advice': 'NORMAL',
            'pages': size_bytes // 4096,
            'pages_touched': pages_touched,
            'mmap_obj': mm,
            'address': ctypes.addressof(ctypes.c_char.from_buffer(mm))
        }
        
        playground_state['allocations'].append(region)
        playground_state['total_allocated'] += size_bytes
        
        return jsonify({
            'success': True,
            'region_id': region_id,
            'size_mb': size_mb,
            'pages_touched': pages_touched,
            'message': f'Allocated {size_mb}MB ({pages_touched} pages touched)'
        })
        
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)})


@app.route('/api/playground/lock', methods=['POST'])
def playground_lock():
    """Lock a memory region (mlock - prevent swapping)."""
    data = request.get_json() or {}
    region_id = data.get('region_id', 0)
    
    if not MLOCK_AVAILABLE:
        return jsonify({'success': False, 'error': 'mlock not available on this system'})
    
    if region_id >= len(playground_state['allocations']):
        return jsonify({'success': False, 'error': 'Invalid region ID'})
    
    region = playground_state['allocations'][region_id]
    
    if region['locked']:
        return jsonify({'success': False, 'error': 'Region already locked'})
    
    try:
        addr = region['address']
        size = region['size_bytes']
        
        result = libc.mlock(ctypes.c_void_p(addr), ctypes.c_size_t(size))
        if result != 0:
            return jsonify({'success': False, 'error': f'mlock failed: errno {ctypes.get_errno()}'})
        
        region['locked'] = True
        playground_state['total_locked'] += size
        
        return jsonify({
            'success': True,
            'message': f'Locked {region["size_mb"]}MB (cannot be swapped out)'
        })
        
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)})


@app.route('/api/playground/unlock', methods=['POST'])
def playground_unlock():
    """Unlock a memory region (munlock)."""
    data = request.get_json() or {}
    region_id = data.get('region_id', 0)
    
    if not MLOCK_AVAILABLE:
        return jsonify({'success': False, 'error': 'munlock not available'})
    
    if region_id >= len(playground_state['allocations']):
        return jsonify({'success': False, 'error': 'Invalid region ID'})
    
    region = playground_state['allocations'][region_id]
    
    if not region['locked']:
        return jsonify({'success': False, 'error': 'Region not locked'})
    
    try:
        addr = region['address']
        size = region['size_bytes']
        
        result = libc.munlock(ctypes.c_void_p(addr), ctypes.c_size_t(size))
        if result != 0:
            return jsonify({'success': False, 'error': f'munlock failed'})
        
        region['locked'] = False
        playground_state['total_locked'] -= size
        
        return jsonify({
            'success': True,
            'message': f'Unlocked {region["size_mb"]}MB'
        })
        
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)})


@app.route('/api/playground/advise', methods=['POST'])
def playground_advise():
    """Apply madvise hint to a memory region."""
    data = request.get_json() or {}
    region_id = data.get('region_id', 0)
    advice = data.get('advice', 'NORMAL')
    
    if region_id >= len(playground_state['allocations']):
        return jsonify({'success': False, 'error': 'Invalid region ID'})
    
    region = playground_state['allocations'][region_id]
    
    # Map advice strings to madvise constants
    advice_map = {
        'NORMAL': 0,      # MADV_NORMAL
        'RANDOM': 1,      # MADV_RANDOM
        'SEQUENTIAL': 2,  # MADV_SEQUENTIAL
        'WILLNEED': 3,    # MADV_WILLNEED (prefetch)
        'DONTNEED': 4,    # MADV_DONTNEED (can discard)
    }
    
    if advice not in advice_map:
        return jsonify({'success': False, 'error': f'Unknown advice: {advice}'})
    
    try:
        mm = region['mmap_obj']
        mm.madvise(advice_map[advice])
        region['advice'] = advice
        
        descriptions = {
            'NORMAL': 'Default access pattern',
            'RANDOM': 'Expect random access (disable readahead)',
            'SEQUENTIAL': 'Expect sequential access (aggressive readahead)',
            'WILLNEED': 'Will need soon (prefetch into memory)',
            'DONTNEED': 'Won\'t need soon (can be swapped out)'
        }
        
        return jsonify({
            'success': True,
            'advice': advice,
            'message': descriptions.get(advice, advice)
        })
        
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)})


@app.route('/api/playground/free', methods=['POST'])
def playground_free():
    """Free a memory region."""
    data = request.get_json() or {}
    region_id = data.get('region_id', 0)
    
    if region_id >= len(playground_state['allocations']):
        return jsonify({'success': False, 'error': 'Invalid region ID'})
    
    region = playground_state['allocations'][region_id]
    
    if region.get('freed'):
        return jsonify({'success': False, 'error': 'Region already freed'})
    
    try:
        # Unlock if locked
        if region['locked'] and MLOCK_AVAILABLE:
            libc.munlock(ctypes.c_void_p(region['address']), 
                        ctypes.c_size_t(region['size_bytes']))
            playground_state['total_locked'] -= region['size_bytes']
        
        # Close mmap
        region['mmap_obj'].close()
        region['freed'] = True
        playground_state['total_allocated'] -= region['size_bytes']
        
        return jsonify({
            'success': True,
            'message': f'Freed region {region_id} ({region["size_mb"]}MB)'
        })
        
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)})


@app.route('/api/playground/status', methods=['GET'])
def playground_status():
    """Get current playground status."""
    regions = []
    for r in playground_state['allocations']:
        if not r.get('freed'):
            regions.append({
                'id': r['id'],
                'size_mb': r['size_mb'],
                'pages': r['pages'],
                'locked': r['locked'],
                'advice': r['advice']
            })
    
    return jsonify({
        'success': True,
        'regions': regions,
        'total_allocated_mb': playground_state['total_allocated'] / (1024 * 1024),
        'total_locked_mb': playground_state['total_locked'] / (1024 * 1024),
        'mlock_available': MLOCK_AVAILABLE
    })


@app.route('/api/playground/reset', methods=['POST'])
def playground_reset():
    """Free all allocations and reset playground."""
    freed_count = 0
    freed_mb = 0
    
    for region in playground_state['allocations']:
        if not region.get('freed'):
            try:
                if region['locked'] and MLOCK_AVAILABLE:
                    libc.munlock(ctypes.c_void_p(region['address']),
                                ctypes.c_size_t(region['size_bytes']))
                region['mmap_obj'].close()
                freed_count += 1
                freed_mb += region['size_mb']
            except:
                pass
    
    playground_state['allocations'] = []
    playground_state['total_allocated'] = 0
    playground_state['total_locked'] = 0
    
    return jsonify({
        'success': True,
        'message': f'Freed {freed_count} regions ({freed_mb}MB total)'
    })


# =============================================================================
# Main
# =============================================================================

if __name__ == '__main__':
    print("Virtual Memory Visualization Tool - API Server")
    print(f"Backend path: {VMEM_SHELL}")
    print("Starting server on http://localhost:5000")
    print("\nEndpoints:")
    print("  GET  /api/health              - Health check")
    print("  GET  /api/processes           - List processes")
    print("  GET  /api/process/<pid>/maps  - Memory regions")
    print("  GET  /api/process/<pid>/translate/<addr> - Translate address")
    print("  GET  /api/process/<pid>/stats - Memory statistics")
    print("  GET  /api/system/memory       - System memory info")
    print("  POST /api/tlb/init            - Initialize TLB")
    print("  POST /api/tlb/lookup          - Lookup in TLB")
    print("  POST /api/tlb/access          - TLB access (lookup + insert)")
    print("  GET  /api/tlb/status          - TLB status")
    print("  POST /api/tlb/flush           - Flush TLB")
    print()
    
    app.run(debug=True, host='0.0.0.0', port=5000)
