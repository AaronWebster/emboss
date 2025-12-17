#!/usr/bin/env python3
"""
Generate Lua dissector from network_headers.emb and run comparison test.
This script demonstrates the complete workflow.
"""

import sys
import os
import subprocess

# Add project root to path
sys.path.insert(0, '/home/runner/work/emboss/emboss')

from compiler.back_end.lua import dissector_generator
from compiler.util import ir_data
from compiler.util import ir_data_utils


def generate_ir_from_emb(emb_file, ir_file):
    """Generate IR file from .emb file using embossc."""
    try:
        # Use Python to run the front end
        cmd = [
            sys.executable,
            '/home/runner/work/emboss/emboss/compiler/front_end/emboss_front_end.py',
            emb_file,
            f'--output-file={ir_file}'
        ]
        
        # Set PYTHONPATH
        env = os.environ.copy()
        env['PYTHONPATH'] = '/home/runner/work/emboss/emboss'
        
        result = subprocess.run(cmd, capture_output=True, text=True, env=env)
        
        if result.returncode != 0:
            print(f"Error generating IR: {result.stderr}")
            return False
        
        return True
    except Exception as e:
        print(f"Exception generating IR: {e}")
        return False


def generate_lua_from_ir(ir_file, lua_file):
    """Generate Lua dissector from IR file."""
    try:
        with open(ir_file, 'r') as f:
            ir = ir_data_utils.IrDataSerializer.from_json(ir_data.EmbossIr, f.read())
        
        code, errors = dissector_generator.generate_dissector(ir, "network")
        
        if errors:
            print(f"Errors generating dissector: {errors}")
            return False
        
        with open(lua_file, 'w') as f:
            f.write(code)
        
        return True
    except Exception as e:
        print(f"Exception generating Lua: {e}")
        import traceback
        traceback.print_exc()
        return False


def main():
    print("Generating Lua dissector from network_headers.emb...")
    print()
    
    emb_file = '/home/runner/work/emboss/emboss/testdata/network_headers.emb'
    ir_file = '/tmp/network_headers.emb.ir'
    lua_file = '/tmp/network_headers.lua'
    
    # Step 1: Generate IR
    print(f"Step 1: Generating IR from {emb_file}...")
    if not generate_ir_from_emb(emb_file, ir_file):
        print("Failed to generate IR")
        return 1
    print(f"  Created: {ir_file}")
    print()
    
    # Step 2: Generate Lua dissector
    print(f"Step 2: Generating Lua dissector from IR...")
    if not generate_lua_from_ir(ir_file, lua_file):
        print("Failed to generate Lua dissector")
        return 1
    print(f"  Created: {lua_file}")
    print()
    
    # Show the generated dissector
    print("=" * 80)
    print("GENERATED LUA DISSECTOR")
    print("=" * 80)
    with open(lua_file, 'r') as f:
        print(f.read())
    print()
    
    # Step 3: Run the comparison test
    print("=" * 80)
    print("Step 3: Running dissector comparison test...")
    print("=" * 80)
    print()
    
    test_script = '/home/runner/work/emboss/emboss/testdata/test_network_dissector.py'
    result = subprocess.run([sys.executable, test_script])
    
    return result.returncode


if __name__ == '__main__':
    sys.exit(main())
