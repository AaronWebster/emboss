# Network Protocol Dissector Test

This directory contains a comprehensive test demonstrating the Emboss Wireshark Lua dissector generator with real network protocol headers.

## Files

- **`network_headers.emb`** - Emboss definitions for Ethernet, IPv4, and UDP headers
- **`generate_and_test.py`** - Script to generate Lua dissector and run comparison test
- **`test_network_dissector.py`** - Creates test PCAP file and compares built-in vs custom dissectors

## Network Protocols Defined

### Ethernet Header (14 bytes)
- Destination MAC address (6 bytes)
- Source MAC address (6 bytes)
- EtherType (2 bytes) - enum with IPV4, ARP, IPV6 values

### IPv4 Header (20 bytes, no options)
- Version and IHL (4 bits each)
- Type of Service
- Total Length
- Identification
- Flags and Fragment Offset
- TTL
- Protocol (1 byte) - enum with ICMP, TCP, UDP values
- Header Checksum
- Source IP Address
- Destination IP Address

### UDP Header (8 bytes)
- Source Port
- Destination Port
- Length
- Checksum

## Usage

### Generate and Test

Run the complete workflow:

```bash
cd /home/runner/work/emboss/emboss
python3 testdata/generate_and_test.py
```

This will:
1. Generate IR from `network_headers.emb`
2. Generate Lua dissector from the IR
3. Create a test PCAP file with sample packet data
4. Show the generated Lua dissector code
5. (If tshark is installed) Compare built-in vs custom dissector output

### Manual Steps

You can also run the steps manually:

```bash
# Step 1: Generate IR
python3 compiler/front_end/emboss_front_end.py \
    testdata/network_headers.emb \
    --output-file=/tmp/network_headers.emb.ir

# Step 2: Generate Lua dissector
python3 compiler/back_end/lua/emboss_codegen_lua.py \
    --input-file=/tmp/network_headers.emb.ir \
    --output-file=/tmp/network_headers.lua

# Step 3: View the generated dissector
cat /tmp/network_headers.lua

# Step 4: Test with tshark (requires Wireshark/tshark installed)
python3 testdata/test_network_dissector.py
```

## Test Packet Structure

The test creates a packet with the following structure:

```
+----------------+
| Ethernet (14B) |
|  Dst: 00:11:.. |
|  Src: AA:BB:.. |
|  Type: 0x0800  |
+----------------+
| IPv4 (20B)     |
|  Src: 192...1  |
|  Dst: 192..255 |
|  Proto: UDP(17)|
+----------------+
| UDP (8B)       |
|  Src: 12345    |
|  Dst: 54321    |
+----------------+
| Payload (53B)  |
|  "Hello, ..."  |
+----------------+
```

## Expected Output

### Generated Lua Dissector Features

The generated dissector includes:
- **Enum value tables**: EtherType and IpProtocol enums show text names instead of numbers
- **Protocol fields**: All header fields are defined as ProtoFields
- **Dissector function**: Parses packet and displays fields in Wireshark tree
- **Registration placeholder**: Template code to register on specific port

### Comparison with Built-in Dissectors

When tshark is available, the test shows:
- **Built-in dissectors**: Use standard filters `eth.*`, `ip.*`, `udp.*`
- **Custom dissector**: Uses `network.*` filter prefix
- **Both should show** similar protocol hierarchy and field values

## Using the Dissector in Wireshark

1. Copy the generated `.lua` file to Wireshark plugins directory:
   - Linux: `~/.local/lib/wireshark/plugins/`
   - macOS: `~/.wireshark/plugins/`
   - Windows: `%APPDATA%\Wireshark\plugins\`

2. Edit the Lua file to register on Ethernet type or specific port

3. Reload Lua plugins in Wireshark (Ctrl+Shift+L)

## Notes

- The test demonstrates the complete workflow from `.emb` to working dissector
- Enum values (EtherType, IpProtocol) display as readable text in Wireshark
- The dissector handles proper byte ordering (BigEndian for network protocols)
- Bit fields in IPv4 header show how to handle sub-byte fields

## Requirements

- Python 3
- Emboss compiler (front end and Lua backend)
- Optional: Wireshark/tshark for live testing
