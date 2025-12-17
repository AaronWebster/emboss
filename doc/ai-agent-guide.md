# AI Agent Guide: Translating to Emboss Structs

This guide is designed for AI agents (such as GitHub Copilot, Gemini, Claude, etc.) to help users translate C/C++ packed structs, bitfields, and electronic datasheet specifications into expertly crafted Emboss struct definitions.

## Table of Contents

- [Overview](#overview)
- [Common Use Cases](#common-use-cases)
- [Quick Translation Rules](#quick-translation-rules)
- [Feature Reference](#feature-reference)
- [Translation Examples](#translation-examples)
- [Best Practices](#best-practices)
- [Common Patterns](#common-patterns)

## Overview

Emboss is a tool for generating code that reads and writes binary data structures. It is designed for communicating with hardware devices, parsing binary protocols, and working with packed data formats found in datasheets.

### When to Use Emboss

✅ **Use Emboss for:**
- Binary hardware protocols (GPS receivers, sensors, actuators)
- Electronic datasheet specifications
- Network protocols with fixed binary formats
- Memory-mapped hardware registers
- File formats with binary structures

❌ **Don't use Emboss for:**
- Text-based protocols (HTTP, SMTP, etc.)
- Self-describing formats you control (use Protocol Buffers instead)
- JSON/XML data

## Common Use Cases

### Use Case 1: C/C++ Packed Struct Translation

When a user provides a C/C++ packed struct, translate it to Emboss following these patterns.

### Use Case 2: Datasheet Table Translation

When a user provides a table from an electronic datasheet, extract the structure definition.

## Quick Translation Rules

### 1. Basic Struct Translation

**C/C++ Packed Struct:**
```c
struct __attribute__((packed)) Message {
    uint8_t sync;
    uint16_t length;
    uint32_t payload;
};
```

**Emboss Translation:**
```
[$default byte_order: "LittleEndian"]  # Adjust based on target platform

struct Message:
  0 [+1]  UInt  sync
  1 [+2]  UInt  length
  3 [+4]  UInt  payload
```

### 2. Bitfield Translation

**C/C++ Bitfield:**
```c
struct __attribute__((packed)) Flags {
    uint8_t enabled : 1;
    uint8_t mode : 3;
    uint8_t reserved : 4;
};
```

**Emboss Translation:**
```
[$default byte_order: "LittleEndian"]

struct Flags:
  0 [+1]  bits:
    0 [+1]  Flag  enabled
    1 [+3]  UInt  mode
    4 [+4]  UInt  reserved
```

### 3. Using `$next` for Sequential Fields

Instead of manually calculating offsets:

```
struct Message:
  0     [+1]  UInt  sync
  $next [+2]  UInt  length
  $next [+4]  UInt  payload
  $next [+1]  UInt  checksum
```

## Feature Reference

### File Structure

Every `.emb` file should have:

1. **Optional documentation block** (starts with `-- `)
2. **Optional imports** (`import "other.emb" as other`)
3. **Attributes block** (byte order, namespace)
4. **Type definitions** (structs, enums, bits)

**Template:**
```
-- Module documentation explaining the protocol or device.

[$default byte_order: "LittleEndian"]  # or "BigEndian"
[(cpp) namespace: "your::namespace"]

# Type definitions follow
```

### Byte Order

**Always specify byte order** at the module or struct level:

```
[$default byte_order: "LittleEndian"]   # For x86, ARM (typically)
[$default byte_order: "BigEndian"]      # For network protocols, some embedded
```

You can override on individual fields:
```
struct Mixed:
  0 [+4]  UInt  little_endian_field
    [byte_order: "LittleEndian"]
  4 [+4]  UInt  big_endian_field
    [byte_order: "BigEndian"]
```

### Field Types

#### Integer Types

- `UInt` - Unsigned integer (1-64 bits)
- `Int` - Signed two's complement integer (1-64 bits)
- `Bcd` - Binary-coded decimal
- `Flag` - Boolean flag (single bit, use in `bits` only)

#### Sized Types

When explicit sizing is needed:
```
UInt:8    # 8-bit unsigned
UInt:16   # 16-bit unsigned
UInt:32   # 32-bit unsigned
Int:8     # 8-bit signed
```

#### Arrays

```
0 [+4]   UInt:8[4]    # Fixed-size array of 4 bytes
0 [+n]   UInt:8[n]    # Variable-size array of n bytes
0 [+16]  UInt:32[4]   # Array of 4 32-bit integers (16 bytes total)
```

### Enumerations

**C/C++ Enum:**
```c
enum MessageType {
    TYPE_REQUEST = 0x01,
    TYPE_RESPONSE = 0x02,
    TYPE_ERROR = 0xFF
};
```

**Emboss Translation:**
```
enum MessageType:
  TYPE_REQUEST  = 0x01
  TYPE_RESPONSE = 0x02
  TYPE_ERROR    = 0xFF

struct Message:
  0 [+1]  MessageType  message_type
```

**Inline enum (for single-use enums):**
```
struct Message:
  0 [+1]  enum  message_type:
    TYPE_REQUEST  = 0x01
    TYPE_RESPONSE = 0x02
```

### Bitfields (`bits`)

Use `bits` for sub-byte fields, flags, and register layouts:

```
bits ControlRegister:
  0  [+3]   UInt  overscan_color
  3  [+1]   Flag  overscan_disable
  4  [+12]  UInt  horizontal_offset
  16 [+16]  UInt  vertical_offset

struct RegisterPage:
  0 [+4]  ControlRegister  control
    [byte_order: "LittleEndian"]
```

**Anonymous bitfield (fields accessible directly):**
```
struct Message:
  0 [+1]  bits:
    0 [+1]  Flag  enabled
    1 [+3]  UInt  mode
    4 [+4]  UInt  priority
```

### Variable-Length Fields

```
struct VariableMessage:
  0 [+1]       UInt      length (n)
  1 [+n]       UInt:8[]  payload
  1+n [+2]     UInt      checksum
```

Use abbreviations `(n)` for complex expressions.

### Conditional Fields

```
struct ConditionalMessage:
  0 [+1]  enum  message_type:
    TYPE_A = 1
    TYPE_B = 2

  if message_type == MessageType.TYPE_A:
    1 [+16]  TypeAPayload  payload_a

  if message_type == MessageType.TYPE_B:
    1 [+8]   TypeBPayload  payload_b
```

### Virtual Fields (Computed Values)

```
struct Message:
  0 [+4]  UInt  raw_timestamp
    -- Seconds since 2000-01-01

  let timestamp = raw_timestamp + 946684800
    -- Convert to Unix timestamp (seconds since 1970-01-01)

  0 [+2]  UInt  raw_length
  2 [+2]  UInt  header_size
  let payload_length = raw_length - header_size
```

### Field Validation with `requires`

Add constraints to ensure data validity:

```
struct Message:
  0 [+1]  UInt  sync
    [requires: this == 0x42]  # Must be magic value

  1 [+2]  UInt  length
    [requires: 10 <= this <= 1024]  # Valid range

  [requires: length >= 10]  # Struct-level constraint
```

### Parameters

Structs can take runtime parameters:

```
struct Payload(version: UInt:8, length: UInt:16):
  0 [+length]  UInt:8[]  data

struct Message:
  0 [+1]     UInt              version
  1 [+2]     UInt              payload_length
  3 [+payload_length]  Payload(version, payload_length)  payload
```

### Overlapping Fields (Union-like)

Emboss allows overlapping fields for C-like unions:

```
struct Union:
  0 [+4]  UInt     as_uint32
  0 [+4]  Int      as_int32
  0 [+4]  UInt:8[4] as_bytes
  0 [+2]  UInt     low_word
  2 [+2]  UInt     high_word
```

### Documentation

Add documentation with `-- ` prefix:

```
struct Message:
  -- Top-level message structure for the XYZ protocol.
  --
  -- This message format is specified in section 5.3 of the device manual.

  0 [+1]  UInt  sync
    -- Synchronization byte, must be 0x42.

  1 [+2]  UInt  length
    -- Total message length including header and checksum.
```

## Translation Examples

### Example 1: Comprehensive Feature Demonstration

This example demonstrates ALL major Emboss features in a realistic sensor telemetry packet.

**Input (C):**
```c
#pragma pack(push, 1)
struct SensorTelemetry {
    uint8_t version;        // Protocol version
    uint8_t sensor_id;      // Sensor identifier
    uint8_t flags;          // Status flags (bitfield)
    uint8_t sample_count;   // Number of samples
    uint16_t samples[8];    // Up to 8 temperature samples
    uint32_t timestamp;     // Unix timestamp
    uint16_t checksum;      // Optional checksum if enabled
};
#pragma pack(pop)
```

**Output (Emboss):**
```
-- Comprehensive sensor telemetry packet demonstrating all Emboss features.

[$default byte_order: "LittleEndian"]
[(cpp) namespace: "sensor::telemetry"]

enum SensorType:
  -- Type of sensor hardware.
  TEMPERATURE = 0x01
  PRESSURE    = 0x02
  HUMIDITY    = 0x03
  COMBO       = 0x04

enum DataQuality:
  -- Quality indicator for sensor readings.
  GOOD      = 0
  DEGRADED  = 1
  POOR      = 2
  INVALID   = 3

struct SensorTelemetry:
  -- Telemetry packet from environmental sensor array.
  --
  -- This structure demonstrates:
  -- * Virtual fields (let)
  -- * Conditional fields (if)
  -- * Anonymous bits (inline bitfield)
  -- * Enums
  -- * Arrays
  -- * Flags
  -- * Requires statements
  
  [requires: version >= 1 && version <= 3 && sample_count <= 8]

  0 [+1]  UInt  version
    -- Protocol version number (1-3).
    [requires: this >= 1 && this <= 3]

  1 [+1]  UInt  sensor_id
    -- Unique sensor identifier (0-255).

  2 [+1]  bits:
    0 [+1]  Flag  enabled
      -- Sensor is actively collecting data.
    
    1 [+1]  Flag  calibrated
      -- Sensor has been calibrated.
      [requires: this == true]
    
    2 [+1]  Flag  low_battery
      -- Battery level is low.
    
    3 [+1]  Flag  checksum_present
      -- Checksum field is present at end of packet.
    
    4 [+2]  UInt  sensor_type_bits
      -- Encoded sensor type (see SensorType enum).
    
    6 [+2]  UInt  data_quality_bits
      -- Data quality indicator (see DataQuality enum).

  let sensor_type = (sensor_type_bits == 1 ? SensorType.TEMPERATURE : (sensor_type_bits == 2 ? SensorType.PRESSURE : (sensor_type_bits == 3 ? SensorType.HUMIDITY : SensorType.COMBO)))

  let data_quality = (data_quality_bits == 0 ? DataQuality.GOOD : (data_quality_bits == 1 ? DataQuality.DEGRADED : (data_quality_bits == 2 ? DataQuality.POOR : DataQuality.INVALID)))

  let is_reliable = calibrated && data_quality == DataQuality.GOOD

  3 [+1]  UInt  sample_count (n)
    -- Number of valid samples in array (0-8).
    [requires: this <= 8]

  4 [+n*2]  UInt:16[n]  samples
    -- Temperature samples in 0.01°C units.

  let timestamp_offset = 4 + n * 2

  timestamp_offset [+4]  UInt  timestamp
    -- Unix timestamp (seconds since 1970-01-01 00:00:00 UTC).
    [requires: this > 1600000000]

  let has_samples = sample_count > 0

  if checksum_present:
    timestamp_offset+4 [+2]  UInt  checksum
      -- CRC-16 checksum of all preceding bytes.
```

**Features Demonstrated:**

1. **Virtual Fields (`let`)**: `sensor_type`, `data_quality`, `is_reliable`, `timestamp_offset`, `has_samples`
2. **Conditional Fields (`if`)**: `checksum` field only exists when `checksum_present` flag is true
3. **Anonymous bits**: The flags byte at offset 2 is broken down into individual `Flag` fields and bit-packed `UInt` fields
4. **Enums**: `SensorType` and `DataQuality` with meaningful named values
5. **Arrays**: `samples` is a variable-length array of `UInt:16` elements (based on `sample_count`)
6. **Flags**: `enabled`, `calibrated`, `low_battery`, `checksum_present` are boolean `Flag` fields within the anonymous bits
7. **Requires statements**: 
   - Struct-level: Combined validation for version range and sample_count limit
   - Field-level: version range check, calibrated must be true, sample_count ≤ 8, timestamp sanity check

### Example 2: Bitfield-Heavy Register Map

**Input (C):**
```c
struct __attribute__((packed)) ControlReg {
    uint32_t enable : 1;
    uint32_t mode : 2;
    uint32_t threshold : 8;
    uint32_t reserved : 21;
};
```

**Output (Emboss):**
```
[$default byte_order: "LittleEndian"]
[(cpp) namespace: "device"]

bits ControlReg:
  -- Control register for device configuration.

  0  [+1]   Flag  enable
    -- Master enable bit. 1 = enabled, 0 = disabled.

  1  [+2]   UInt  mode
    -- Operating mode: 0 = sleep, 1 = normal, 2 = high-performance, 3 = debug.

  3  [+8]   UInt  threshold
    -- Threshold value (0-255).

  11 [+21]  UInt  reserved
    -- Reserved for future use. Write as 0.

struct DeviceRegisters:
  0 [+4]  ControlReg  control
    [byte_order: "LittleEndian"]
```

### Example 3: Datasheet Table Translation

**Input (Datasheet Table):**
```
Offset | Size | Field Name    | Description
-------|------|---------------|------------------
0x00   | 1    | SYNC          | Sync byte (0xAA)
0x01   | 1    | CMD           | Command code
0x02   | 2    | LENGTH        | Payload length
0x04   | N    | PAYLOAD       | Variable payload
0x04+N | 2    | CRC16         | CRC-16 checksum
```

**Output (Emboss):**
```
-- Protocol definition based on device specification v2.1.

[$default byte_order: "BigEndian"]  # Common for network protocols
[(cpp) namespace: "device::protocol"]

struct Message:
  -- Frame structure for device communication protocol.

  0 [+1]  UInt  sync
    -- Synchronization byte, must be 0xAA.
    [requires: this == 0xAA]

  1 [+1]  UInt  command
    -- Command code (see CommandCode enum).

  2 [+2]  UInt  length (n)
    -- Payload length in bytes.

  4 [+n]  UInt:8[]  payload
    -- Variable-length payload data.

  4+n [+2]  UInt  crc16
    -- CRC-16 checksum of entire message.

enum CommandCode:
  -- Command codes for device protocol.
  READ_STATUS   = 0x01
  WRITE_CONFIG  = 0x02
  RESET         = 0x10
```

### Example 4: Complex Nested Structure

**Input (C):**
```c
struct __attribute__((packed)) Header {
    uint8_t version;
    uint8_t flags;
    uint16_t payload_size;
};

struct __attribute__((packed)) Packet {
    struct Header header;
    uint8_t payload[256];
    uint32_t checksum;
};
```

**Output (Emboss):**
```
[$default byte_order: "LittleEndian"]
[(cpp) namespace: "network"]

struct Header:
  -- Packet header structure.

  0 [+1]  UInt  version
    -- Protocol version number.

  1 [+1]  bits:
    0 [+1]  Flag  compressed
    1 [+1]  Flag  encrypted
    2 [+6]  UInt  reserved

  2 [+2]  UInt  payload_size
    -- Size of payload in bytes (max 256).
    [requires: this <= 256]

struct Packet:
  -- Complete packet structure with header and payload.

  0 [+4]           Header    header
  4 [+header.payload_size]  UInt:8[]  payload
    -- Variable-length payload data.
  4+header.payload_size [+4]  UInt  checksum
    -- CRC32 checksum of header and payload.
```

### Example 5: Version-Dependent Structure

**Input (Scenario):**
"We have a protocol where v1 has 2-byte header, v2 has 4-byte header with extra flags."

**Output (Emboss):**
```
[$default byte_order: "LittleEndian"]
[(cpp) namespace: "protocol"]

enum Version:
  V1 = 1
  V2 = 2

struct Message(version: Version):
  -- Version-dependent message structure.

  0 [+1]  UInt  message_id

  if version == Version.V1:
    1 [+1]  bits:
      0 [+8]  UInt  v1_flags

  if version == Version.V2:
    1 [+3]  bits:
      0  [+8]   UInt  v2_flags
      8  [+8]   UInt  priority
      16 [+8]   UInt  reserved

struct VersionedPacket:
  -- Packet with version-specific header.

  0 [+1]  Version  version
  1 [+n]  Message(version)  message
    where n is determined by version
```

## Best Practices

### 1. Always Specify Byte Order

```
# At module level (applies to all)
[$default byte_order: "LittleEndian"]

# Or per struct
struct MyStruct:
  [$default byte_order: "BigEndian"]
```

### 2. Use Meaningful Names

```
# Good
struct SensorReading:
  0 [+2]  UInt  temperature_celsius_times_100

# Less ideal
struct Data:
  0 [+2]  UInt  temp
```

### 3. Add Documentation

Document especially:
- Magic values and their meaning
- Units of measurement
- Valid ranges
- References to specification sections

```
struct Reading:
  0 [+2]  UInt  temperature
    -- Temperature in units of 0.01°C.
    -- Valid range: -4000 to 12500 (-40°C to 125°C).
    -- See section 3.2 of datasheet XYZ-123.
    [requires: -4000 <= this <= 12500]
```

### 4. Use `requires` for Validation

```
struct Message:
  0 [+1]  UInt  magic
    [requires: this == 0x42]

  1 [+2]  UInt  length
    [requires: 8 <= this <= 1024]

  [requires: length % 4 == 0]  # Length must be multiple of 4
```

### 5. Use Virtual Fields for Clarity

```
struct DateTime:
  0 [+2]  UInt  raw_year
    -- Years since 1900

  let year = raw_year + 1900
    -- Gregorian year

  2 [+1]  UInt  zero_based_month
  let month = zero_based_month + 1
    -- Month 1-12
```

### 6. Use `$next` for Sequential Layouts

```
struct Sequential:
  0     [+4]  UInt  field1
  $next [+2]  UInt  field2
  $next [+1]  UInt  field3
  $next [+8]  UInt  field4
```

### 7. Prefer Enums Over Magic Numbers

```
# Good
enum Status:
  OK    = 0
  ERROR = 1

struct Response:
  0 [+1]  Status  status

# Less ideal
struct Response:
  0 [+1]  UInt  status  # 0 = OK, 1 = ERROR
```

## Common Patterns

### Pattern: Message with Length Prefix

```
struct LengthPrefixedMessage:
  0 [+2]       UInt      length (n)
  2 [+n]       UInt:8[]  data
  2+n [+2]     UInt      checksum
```

### Pattern: Tagged Union / Discriminated Union

```
struct TaggedUnion:
  0 [+1]  enum  tag:
    TYPE_INT  = 1
    TYPE_FLOAT = 2
    TYPE_STRING = 3

  if tag == Tag.TYPE_INT:
    1 [+4]  Int  int_value

  if tag == Tag.TYPE_FLOAT:
    1 [+4]  UInt  float_value  # Reinterpret as float in application

  if tag == Tag.TYPE_STRING:
    1 [+1]   UInt      string_length (n)
    2 [+n]   UInt:8[]  string_data
```

### Pattern: Header + Payload + Footer

```
struct Packet:
  -- Standard packet with header, variable payload, footer.

  # Header
  0 [+1]  UInt  sync
    [requires: this == 0x7E]
  1 [+1]  UInt  message_type
  2 [+2]  UInt  payload_length (n)

  # Payload
  4 [+n]  UInt:8[]  payload

  # Footer
  let footer_offset = 4 + n
  footer_offset [+2]  UInt  crc16
```

### Pattern: Bit-Packed Flags

```
struct Flags:
  0 [+2]  bits:
    0  [+1]  Flag  flag_a
    1  [+1]  Flag  flag_b
    2  [+1]  Flag  flag_c
    3  [+1]  Flag  flag_d
    4  [+3]  UInt  mode
    7  [+9]  UInt  reserved
```

### Pattern: Array with Count

```
struct ItemList:
  0 [+1]      UInt        count (n)
  1 [+n*4]    Item[n]     items

struct Item:
  0 [+4]  UInt  value
```

### Pattern: Multiple Views of Same Data

```
struct Register:
  # Allow access as single 32-bit value or separate bytes
  0 [+4]  UInt      as_uint32
  0 [+4]  UInt:8[4] as_bytes
  0 [+2]  UInt      low_word
  2 [+2]  UInt      high_word
```

### Pattern: Alignment and Padding

```
struct AlignedData:
  0 [+1]  UInt  byte_field
  1 [+3]  UInt  padding  # Explicit padding to align next field
    [text_output: "Skip"]  # Don't show in text output
  4 [+4]  UInt  aligned_field
```

## Decision Tree for AI Agents

When translating to Emboss, follow this decision tree:

1. **Determine byte order**
   - User specified? Use that.
   - C struct? Usually little-endian (x86/ARM).
   - Network protocol? Usually big-endian.
   - Ask if unclear.

2. **Identify structure type**
   - Bitfields (sub-byte)? → Use `bits`
   - Regular struct? → Use `struct`
   - Constants only? → Use `enum`

3. **Handle field types**
   - Integer? → `UInt` or `Int`
   - Single bit? → `Flag` (in bits)
   - Enumerated? → Create `enum`
   - Array? → Use `Type[]` syntax
   - Nested struct? → Define separately or inline

4. **Add constraints**
   - Magic values? → Add `[requires: this == value]`
   - Valid ranges? → Add `[requires: min <= this <= max]`
   - Complex validation? → Add struct-level `[requires: ...]`

5. **Document thoroughly**
   - Add `-- ` comments for all non-obvious fields
   - Reference specification sections
   - Note units and ranges

6. **Use advanced features**
   - Variable length? → Use field references in sizes
   - Computed values? → Use virtual fields with `let`
   - Version-dependent? → Use parameters or conditionals
   - Sequential layout? → Use `$next`

## Quick Reference Card

```
# File structure
[$default byte_order: "LittleEndian|BigEndian"]
[(cpp) namespace: "name::space"]

# Basic field
offset [+size]  Type  field_name

# Field with abbreviation
offset [+size]  Type  field_name (abbrev)

# Documentation
-- Comment text

# Constraint
[requires: expression]

# Virtual field
let name = expression

# Conditional field
if condition:
  offset [+size]  Type  field_name

# Array
offset [+total_size]  Type:element_size[count]  array_name

# Bitfield
offset [+byte_size]  bits:
  bit_offset [+bit_size]  Type  field_name

# Enum
enum Name:
  VALUE = number

# $next for sequential fields
$next [+size]  Type  field_name
```

## Troubleshooting Common Issues

### Issue: "Size expression is not constant"
**Cause:** Using dynamic field in bits.
**Solution:** `bits` must have compile-time known size. Use `struct` for dynamic sizes.

### Issue: "Byte order not specified"
**Cause:** Multi-byte field without byte order.
**Solution:** Add `[$default byte_order: "LittleEndian"]` at module or struct level.

### Issue: "Field references unavailable field"
**Cause:** Using field before it's defined.
**Solution:** Reorder fields or use forward references carefully.

### Issue: "Overlapping fields"
**Cause:** Field offsets overlap (unless intentional for union).
**Solution:** Check offset calculations. Use `$next` to avoid errors.

## Advanced Topics

### Imports

```
import "common.emb" as common

struct MyStruct:
  0 [+4]  common.CommonType  field
```

### Nested Types

```
struct Outer:
  struct Inner:
    0 [+2]  UInt  value

  0 [+2]  Inner  inner_field
  2 [+2]  Inner  another_inner
```

### Size Queries

```
struct Foo:
  0 [+4]  UInt  x

struct Bar:
  0 [+Foo.$size_in_bytes]  Foo  foo_field
  # Foo.$min_size_in_bytes and Foo.$max_size_in_bytes also available
```

## Summary for AI Agents

When a user asks you to translate C/C++ structs or datasheet tables to Emboss:

1. **Start with the template** (byte order + namespace)
2. **Choose the right construct** (struct, bits, or enum)
3. **Use `$next`** for sequential fields to avoid offset calculation errors
4. **Add enums** for any magic numbers or status codes
5. **Add `requires`** for validation
6. **Document everything** with `-- ` comments
7. **Use virtual fields** (`let`) for computed values
8. **Consider conditional fields** for version-dependent or optional data
9. **Verify byte order** - ask if unclear
10. **Test your translation** against the specification

Remember: Emboss prioritizes **safety** (bounds checking, validation) and **clarity** (documentation, expressive types) over brevity. A well-crafted Emboss definition should be self-documenting and prevent common errors.
