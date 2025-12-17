#!/usr/bin/env python3
# Copyright 2019 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""
Test script that creates a network packet and compares dissection results
between tshark's built-in dissectors and our custom Emboss Lua dissector.
"""

import struct
import subprocess
import sys
import os
import tempfile


def create_test_packet():
    """
    Create a test Ethernet/IP/UDP packet with payload.
    
    Returns bytes representing:
    - Ethernet header (14 bytes)
    - IPv4 header (20 bytes)
    - UDP header (8 bytes)
    - Payload data (variable)
    """
    
    # Ethernet header (14 bytes)
    dst_mac = bytes([0x00, 0x11, 0x22, 0x33, 0x44, 0x55])  # Destination MAC
    src_mac = bytes([0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF])  # Source MAC
    ethertype = struct.pack('>H', 0x0800)  # IPv4
    
    ethernet_header = dst_mac + src_mac + ethertype
    
    # Payload data (simple test message)
    payload = b"Hello, Wireshark! This is a test payload from Emboss."
    
    # UDP header (8 bytes)
    src_port = 12345
    dst_port = 54321
    udp_length = 8 + len(payload)  # UDP header + payload
    udp_checksum = 0  # We'll use 0 for simplicity (optional in IPv4)
    
    udp_header = struct.pack('>HHHH', src_port, dst_port, udp_length, udp_checksum)
    
    # IPv4 header (20 bytes, no options)
    version_ihl = (4 << 4) | 5  # Version 4, IHL 5 (20 bytes)
    tos = 0
    total_length = 20 + len(udp_header) + len(payload)
    identification = 12345
    flags_fragment = 0x4000  # Don't fragment flag set
    ttl = 64
    protocol = 17  # UDP
    header_checksum = 0  # Will calculate below
    src_ip = struct.pack('>I', 0xC0A80001)  # 192.168.0.1
    dst_ip = struct.pack('>I', 0xC0A800FF)  # 192.168.0.255
    
    # Build IP header without checksum
    ip_header_no_checksum = struct.pack(
        '>BBHHHBBH',
        version_ihl, tos, total_length, identification,
        flags_fragment, ttl, protocol, 0  # checksum placeholder
    ) + src_ip + dst_ip
    
    # Calculate IP header checksum
    checksum = 0
    for i in range(0, len(ip_header_no_checksum), 2):
        word = (ip_header_no_checksum[i] << 8) + ip_header_no_checksum[i + 1]
        checksum += word
        checksum = (checksum & 0xFFFF) + (checksum >> 16)
    checksum = ~checksum & 0xFFFF
    
    # Rebuild IP header with correct checksum
    ip_header = struct.pack(
        '>BBHHHBBH',
        version_ihl, tos, total_length, identification,
        flags_fragment, ttl, protocol, checksum
    ) + src_ip + dst_ip
    
    # Combine all parts
    packet = ethernet_header + ip_header + udp_header + payload
    
    return packet


def write_pcap_file(packet, filename):
    """Write packet to a PCAP file."""
    
    # PCAP global header
    magic_number = 0xa1b2c3d4
    version_major = 2
    version_minor = 4
    thiszone = 0
    sigfigs = 0
    snaplen = 65535
    network = 1  # Ethernet
    
    pcap_global_header = struct.pack(
        '<IHHIIII',
        magic_number, version_major, version_minor,
        thiszone, sigfigs, snaplen, network
    )
    
    # PCAP packet header
    ts_sec = 1234567890
    ts_usec = 0
    incl_len = len(packet)
    orig_len = len(packet)
    
    pcap_packet_header = struct.pack(
        '<IIII',
        ts_sec, ts_usec, incl_len, orig_len
    )
    
    # Write to file
    with open(filename, 'wb') as f:
        f.write(pcap_global_header)
        f.write(pcap_packet_header)
        f.write(packet)


def run_tshark_builtin(pcap_file):
    """Run tshark with built-in dissectors."""
    try:
        result = subprocess.run(
            ['tshark', '-r', pcap_file, '-V'],
            capture_output=True,
            text=True,
            timeout=10
        )
        return result.stdout
    except FileNotFoundError:
        return "ERROR: tshark not found. Please install Wireshark/tshark."
    except subprocess.TimeoutExpired:
        return "ERROR: tshark timed out."
    except Exception as e:
        return f"ERROR: {str(e)}"


def run_tshark_custom(pcap_file, lua_dissector):
    """Run tshark with custom Lua dissector."""
    try:
        result = subprocess.run(
            ['tshark', '-r', pcap_file, '-X', f'lua_script:{lua_dissector}', '-V'],
            capture_output=True,
            text=True,
            timeout=10
        )
        return result.stdout
    except FileNotFoundError:
        return "ERROR: tshark not found. Please install Wireshark/tshark."
    except subprocess.TimeoutExpired:
        return "ERROR: tshark timed out."
    except Exception as e:
        return f"ERROR: {str(e)}"


def main():
    print("=" * 80)
    print("Emboss Wireshark Lua Dissector - Network Protocol Test")
    print("=" * 80)
    print()
    
    # Create test packet
    print("Creating test packet (Ethernet + IPv4 + UDP + payload)...")
    packet = create_test_packet()
    print(f"  Packet size: {len(packet)} bytes")
    print(f"  Ethernet: 14 bytes")
    print(f"  IPv4:     20 bytes")
    print(f"  UDP:      8 bytes")
    print(f"  Payload:  {len(packet) - 42} bytes")
    print()
    
    # Write to temporary PCAP file
    with tempfile.NamedTemporaryFile(mode='wb', suffix='.pcap', delete=False) as pcap_f:
        pcap_file = pcap_f.name
        write_pcap_file(packet, pcap_file)
    
    print(f"Created PCAP file: {pcap_file}")
    print()
    
    # Check if Lua dissector exists
    lua_dissector = '/tmp/network_headers.lua'
    if not os.path.exists(lua_dissector):
        print(f"WARNING: Custom Lua dissector not found at {lua_dissector}")
        print("You need to generate it first using the Emboss Lua generator.")
        print()
    
    # Run with built-in dissectors
    print("=" * 80)
    print("TSHARK OUTPUT - BUILT-IN DISSECTORS")
    print("=" * 80)
    builtin_output = run_tshark_builtin(pcap_file)
    print(builtin_output)
    print()
    
    # Run with custom dissector if available
    if os.path.exists(lua_dissector):
        print("=" * 80)
        print("TSHARK OUTPUT - CUSTOM EMBOSS LUA DISSECTOR")
        print("=" * 80)
        custom_output = run_tshark_custom(pcap_file, lua_dissector)
        print(custom_output)
        print()
        
        print("=" * 80)
        print("COMPARISON")
        print("=" * 80)
        print("Both dissectors should show similar protocol structure.")
        print("The custom Emboss dissector uses the 'emboss' filter prefix.")
        print("Built-in dissectors use standard filters: eth, ip, udp")
        print()
    
    # Cleanup
    try:
        os.unlink(pcap_file)
        print(f"Cleaned up temporary file: {pcap_file}")
    except:
        pass
    
    return 0


if __name__ == '__main__':
    sys.exit(main())
