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


def run_vmem_command(*args):
    """
    Run the vmem_shell with --json flag and parse the result.
    """
    try:
        cmd = ['sudo', VMEM_SHELL, '--json'] + list(args)
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if result.returncode != 0:
            return {'success': False, 'error': result.stderr or 'Command failed'}
        
        return json.loads(result.stdout)
    except subprocess.TimeoutExpired:
        return {'success': False, 'error': 'Command timed out'}
    except json.JSONDecodeError:
        return {'success': False, 'error': 'Invalid JSON response from backend'}
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
        
        # Find slot
        empty_slot = None
        for i, entry in enumerate(tlb_state['entries']):
            if entry is None or not entry['valid']:
                empty_slot = i
                break
        
        if empty_slot is None:
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
