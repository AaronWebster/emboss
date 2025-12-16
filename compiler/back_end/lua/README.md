# Emboss Wireshark Lua Dissector Generator

This is an Emboss backend that generates Wireshark Lua dissectors from `.emb` files.

## Overview

The Lua dissector generator creates Wireshark protocol dissectors that can parse and display binary protocol data based on Emboss structure definitions. This allows you to:

- Automatically generate Wireshark dissectors from your Emboss protocol specifications
- Display protocol fields with proper names and descriptions
- Show enum values as human-readable text
- Maintain hierarchical relationships between nested structures
- Add custom filter names for Wireshark display filters

## Usage

### Using Bazel

Add a `emboss_lua_library` target to your BUILD file:

```python
load("//:build_defs.bzl", "emboss_lua_library")

emboss_lua_library(
    name = "my_protocol_dissector",
    srcs = ["my_protocol.emb"],
)
```

This will generate a `my_protocol.emb.lua` file that can be loaded into Wireshark.

### Using the Command Line

You can also generate dissectors directly from the command line:

```bash
# First, generate the IR file
bazel run //compiler/front_end:emboss_front_end -- \
  /path/to/protocol.emb \
  --output-file=/tmp/protocol.emb.ir

# Then, generate the Lua dissector
bazel run //compiler/back_end/lua:emboss_codegen_lua -- \
  --input-file=/tmp/protocol.emb.ir \
  --output-file=/tmp/protocol.lua \
  --protocol-name=myproto
```

### Loading into Wireshark

1. Copy the generated `.lua` file to your Wireshark plugins directory:
   - Linux: `~/.local/lib/wireshark/plugins/`
   - macOS: `~/.wireshark/plugins/` or `/Applications/Wireshark.app/Contents/PlugIns/wireshark/`
   - Windows: `%APPDATA%\Wireshark\plugins\`

2. Edit the dissector file to uncomment and configure the registration code at the bottom:
   ```lua
   local udp_table = DissectorTable.get("udp.port")
   udp_table:add(12345, my_protocol_proto)  -- Replace 12345 with your port
   ```

3. Restart Wireshark or reload Lua plugins (Ctrl+Shift+L)

## Emboss Annotations

### Wireshark Filter Names

You can customize the Wireshark filter names using the `wireshark_filter` attribute:

```emboss
-- At module level
[(wireshark_filter): "myproto"]

-- At struct level
struct MyHeader:
  [(wireshark_filter): "myproto.header"]
  0 [+2]  UInt  field1
  
-- At field level
struct MyData:
  0 [+1]  UInt  status
    [(wireshark_filter): "myproto.custom_status"]
```

Without this annotation, filter names are automatically generated from the structure hierarchy.

### Documentation Comments

Use double-dash (`--`) comments to add descriptions that will appear in Wireshark:

```emboss
-- This is a protocol header
struct ProtocolHeader:
  -- Message type identifier
  0 [+1]  MessageType  msg_type
  -- Current protocol version
  1 [+1]  UInt         version
```

Note: Hash (`#`) comments are for Emboss license headers and are not included in the dissector.

## Example

Here's a complete example:

```emboss
# Copyright notice here

-- Network protocol definition
[(wireshark_filter): "netproto"]

-- Message types
enum MessageType:
  -- Data packet
  DATA     = 0x01
  -- Acknowledgment packet
  ACK      = 0x02
  -- Error packet
  ERROR    = 0x03

-- Protocol header
struct Header:
  -- Message type
  0 [+1]  MessageType  type
  -- Sequence number
  1 [+2]  UInt         seq_num
  -- Payload length in bytes
  3 [+2]  UInt         length
```

This generates a dissector that:
- Shows `MessageType` values as "DATA", "ACK", or "ERROR" instead of hex values
- Displays field descriptions in Wireshark
- Uses filter names like `netproto.type`, `netproto.seq_num`, etc.

## Features

- **Automatic Type Mapping**: Emboss UInt/Int types are mapped to appropriate Wireshark field types (uint8, uint16, uint32, uint64, int8, etc.) based on field size
- **Enum Support**: Enum fields display symbolic names instead of numeric values using Wireshark value strings
- **Documentation**: Double-dash comments from your `.emb` file appear as field descriptions in Wireshark
- **Hierarchical Filters**: Filter names preserve the structure hierarchy for easy filtering
- **Byte Order Support**: Respects the byte order specified in your Emboss definitions

## Limitations

- Currently supports only fixed-size fields (dynamic-size fields are not yet supported)
- Arrays and complex nested structures need manual adjustments
- Conditional fields (if statements in Emboss) are not yet fully supported
- The generated dissector provides a starting point that may need customization for complex protocols

## Future Enhancements

Planned improvements include:
- Support for nested structures and arrays
- Conditional field handling
- Dynamic size calculation
- Automatic port registration based on attributes
- Better handling of bit fields
- Support for multiple packet types in one dissector
