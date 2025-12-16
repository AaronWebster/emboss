# Quick Start Guide: Wireshark Lua Dissector Generator

This guide will help you quickly generate and use Wireshark dissectors from your Emboss protocol definitions.

## Step 1: Define Your Protocol in Emboss

Create a `.emb` file with your protocol definition:

```emboss
# myprotocol.emb

-- My custom network protocol
[(wireshark_filter): "myproto"]

[$default byte_order: "LittleEndian"]

-- Packet types
enum PacketType:
  -- Data packet
  DATA = 0x01
  -- Acknowledgment
  ACK  = 0x02

-- Protocol header
struct Header:
  -- Packet type identifier
  0 [+1]  PacketType  type
  -- Packet sequence number
  1 [+4]  UInt        seq_num
  -- Payload length
  5 [+2]  UInt        length
```

## Step 2: Add to Your BUILD File

```python
load("//:build_defs.bzl", "emboss_lua_library")

emboss_lua_library(
    name = "myprotocol_dissector",
    srcs = ["myprotocol.emb"],
)
```

## Step 3: Build the Dissector

```bash
bazel build :myprotocol_dissector
```

The generated `.lua` file will be in `bazel-bin/myprotocol.emb.lua`

## Step 4: Install in Wireshark

Copy the generated `.lua` file to your Wireshark plugins directory:

**Linux:**
```bash
cp bazel-bin/myprotocol.emb.lua ~/.local/lib/wireshark/plugins/
```

**macOS:**
```bash
cp bazel-bin/myprotocol.emb.lua ~/.wireshark/plugins/
```

**Windows:**
```powershell
copy bazel-bin\myprotocol.emb.lua %APPDATA%\Wireshark\plugins\
```

## Step 5: Configure Port Registration

Edit the generated `.lua` file and uncomment the registration code at the bottom:

```lua
-- Register the dissector
local udp_table = DissectorTable.get("udp.port")
udp_table:add(12345, myproto_proto)  -- Replace 12345 with your port
```

For TCP:
```lua
local tcp_table = DissectorTable.get("tcp.port")
tcp_table:add(12345, myproto_proto)
```

## Step 6: Load in Wireshark

1. Open Wireshark
2. Reload Lua plugins: Analyze → Reload Lua Plugins (or Ctrl+Shift+L)
3. Your dissector is now active!

## Testing Your Dissector

1. Capture some traffic on your configured port
2. Wireshark should automatically use your dissector
3. You should see your fields displayed with their names and descriptions
4. Enum values will show as text (e.g., "DATA" instead of "0x01")

## Troubleshooting

**Dissector not loading?**
- Check Wireshark's Lua console (Tools → Lua → Console) for errors
- Verify the .lua file is in the correct plugins directory
- Make sure you reloaded Lua plugins

**Fields not displaying?**
- Verify your port registration matches your traffic
- Check that byte order matches your data
- Ensure field sizes are correct in your .emb file

**Want to see all available filters?**
- In Wireshark, go to Edit → Preferences → Protocols
- Find your protocol in the list
- Or use the filter expression builder (Analyze → Display Filter Expression)

## Advanced Features

### Custom Filter Names

```emboss
struct MyStruct:
  [(wireshark_filter): "myproto.custom"]
  0 [+1]  UInt  field1
    [(wireshark_filter): "myproto.special_field"]
```

### Nested Structures

```emboss
struct Inner:
  0 [+2]  UInt  value

struct Outer:
  0 [+2]  Inner  inner_data
  2 [+4]  UInt   other_field
```

The dissector will maintain the hierarchy.

## Next Steps

- See the full README.md for all features
- Check out example_protocol.emb for a comprehensive example
- Read about Emboss language features in the main Emboss documentation
