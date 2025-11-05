# Goodnature A24 Chirp BLE Protocol Research

## Overview

The Goodnature A24 trap uses a BLE-enabled device called "Chirp" (also called Smart Shroud Cap) to track trap activations. The device uses a Nordic Semiconductor NRF52 SoC with Bluetooth 5.4 capability.

## ⚡ CRITICAL DISCOVERY: iBeacon Format

**The Goodnature Chirp uses Apple's iBeacon format for BLE advertisements!** This is why it appeared "weird" - it's not standard BLE manufacturer data, but rather a modified iBeacon packet.

### Verified iBeacon Structure (from local testing)

**Manufacturer Data (Company ID 0x004C - Apple):**
```
02 15 B0 B0 EE E7 B9 B0 4B C5 B5 E4 F5 CB 61 0E B7 [01] 8F C9 3B E3 37
│  │  └─────────────────── UUID (16 bytes) ──────────────────┘  │  └─Major─┘ └─Minor─┘ └TX
│  └─ iBeacon prefix                                              │
└─ Length                                                         └─ KILL COUNT (changes 00/01)
```

**Breakdown:**
- **Bytes 0-1:** `02 15` - iBeacon prefix (standard)
- **Bytes 2-17:** `B0B0EEE7-B9B0-4BC5-B5E4-F5CB610EB700` - UUID (constant for all Goodnature devices)
- **Byte 18:** `00` or `01` - **KILL COUNT** (this is the key data!)
- **Bytes 19-20:** `8F C9` - Major (end of serial number: 0xC98F)
- **Bytes 21-22:** `3B E3` - Minor (start of serial number: 0xE33B)
- **Byte 23:** TX Power / RSSI reference (varies)

### Serial Number Encoding

Serial number `E33BC98F` is embedded in multiple places:

1. **iBeacon Major/Minor fields:**
   - Major: `8F C9` (0xC98F - last 2 bytes of serial, reversed)
   - Minor: `3B E3` (0xE33B - first 2 bytes of serial, reversed)

2. **Advertised Service UUIDs:**
   - `0xC98F` and `0xE33B` appear as service UUIDs
   - Allows filtering by specific device without parsing manufacturer data

3. **Additional advertised services (constant):**
   - `0x0036`, `0x1234`, `0x1500` - Purpose unknown
   - `0xD801` or `0xD802` - Changes with kill count (D801=0, D802=1+)

### Local Testing Results

**Device Information:**
- **Serial:** E33BC98F
- **Firmware:** 1.1.3
- **MAC Address:** D3:58:7F:15:99:0B

**Observed iBeacon packets:**
```
Before strike: 02 15 B0 B0 EE E7 B9 B0 4B C5 B5 E4 F5 CB 61 0E B7 00 8F C9 3B E3 36
After strike:  02 15 B0 B0 EE E7 B9 B0 4B C5 B5 E4 F5 CB 61 0E B7 01 8F C9 3B E3 37
                                                                    ^^             ^^
                                                                Kill count      TX Power
```

**Key observation:** Byte 18 (index 17 when 0-based) increments from `00` to `01` after a strike!

## Key Findings

### Device Identification
- **Advertised Name:** "GN" (very short, 2 characters)
- **MAC Address Range:** Primarily D7:AC:E9:XX:XX:XX, also D3:58:7F:XX:XX:XX observed
- **Bluetooth Chip:** Nordic NRF52 SoC
- **Advertisement Format:** iBeacon (Apple company ID 0x004C)

### Behavioral Characteristics

1. **Advertisement Behavior:**
   - The Chirp continuously broadcasts BLE packets until the companion app connects and "calms" it
   - This prevents repeated notifications for the same kill event
   - 23-second cooldown between activations to conserve battery

2. **Connection Limitations:**
   - Only one device can connect to the Chirp at any time
   - Requires proximity (Bluetooth range)

## BLE Protocol Details

### Primary Service UUIDs

#### 1. Kill Information Service
**UUID:** `0000D00D-1212-EFDE-1523-785FEF13D123`

**Main Characteristic - Kill Data (0000D30D-1212-EFDE-1523-785FEF13D123):**
- **Format:** String pattern `AAbbbbbbbbCdddEEEEEEEfGG`
- **Structure:**
  - `AA` - Unknown dynamic value
  - `bbbbbbbb` - Serial number (reversed byte pairs)
  - `C` - Static "1"
  - `ddd` - Unknown changing value
  - `EEEEEE` - Static identifier
  - `f` - **Kill count** (single digit, observed range: 1-6)
  - `GG` - Static "00"

**Related Characteristics:**
- `0000D20D` - Test strike data (writable)
- `0000D50D` - Usually "0"
- `0000D60D` - Serial number related

#### 2. Device Information Service
**UUID:** `0000DE11-1212-EFDE-1523-785FEF13D123`

Contains characteristics for:
- `0000DE12` - Serial number
- `0000DE13` through `0000DE16` - Status/configuration fields

#### 3. Time Service
**UUID:** `0000F1AE-1212-EFDE-1523-785FEF13D123`

**Characteristic (0000F1AF):**
- Current time in milliseconds divided by 60000
- Encoded in hex with Little Endian format

#### 4. Additional Services
- `0000E770` - Unknown service
- `0000FADE` - Unknown service (FAD1 usually 234, FAD2 usually 0)

### Complete GATT Services (from connection)

When connected via GATT, the following services and characteristics are available:

#### 1. Service 0000E770-1212-EFDE-1523-785FEF13D123
**Characteristics:**
- `0000E771-1212-EFDE-1523-785FEF13D123` (notify capable)
- `0000E772-1212-EFDE-1523-785FEF13D123` (notify capable)
- `0000E773-1212-EFDE-1523-785FEF13D123` (notify capable)

#### 2. Service 0000FADE-1212-EFDE-1523-785FEF13D123
**Characteristics:**
- `0000FAD1-1212-EFDE-1523-785FEF13D123` (notify capable) - Usually value: 218 or 234
- `0000FAD2-1212-EFDE-1523-785FEF13D123` (notify capable) - Usually value: 0 or 1

#### 3. Service 0000F1AE-1212-EFDE-1523-785FEF13D123 (Time Service)
**Characteristics:**
- `0000F1AF-1212-EFDE-1523-785FEF13D123` (notify capable)
  - Example value: `f28ab701`
  - Current time in milliseconds / 60000, Little Endian format

#### 4. Service 0000DE11-1212-EFDE-1523-785FEF13D123 (Device Information)
**Characteristics:**
- `0000DE12-1212-EFDE-1523-785FEF13D123` (notify capable) - Serial number
- `0000DE13-1212-EFDE-1523-785FEF13D123` (notify capable) - Status field
- `0000DE14-1212-EFDE-1523-785FEF13D123` (notify capable) - Status field
- `0000DE15-1212-EFDE-1523-785FEF13D123` (notify capable) - Status field
- `0000DE16-1212-EFDE-1523-785FEF13D123` (notify capable) - Status field

#### 5. Service 0000D00D-1212-EFDE-1523-785FEF13D123 (Kill Information)
**Characteristics:**
- `0000D20D-1212-EFDE-1523-785FEF13D123` (notify, writable) - Test strike data
- `0000D30D-1212-EFDE-1523-785FEF13D123` (notify capable) - **MAIN KILL DATA**
  - Format: `928fc93be308cba6b5011400` (after strike)
  - Structure: `[A][Serial][C][ddd][EEEE][f][GG]`
  - Serial: `8fc93be3` → reversed: `E33BC98F`
  - Kill count at position [20]: `1`
- `0000D50D-1212-EFDE-1523-785FEF13D123` (notify capable) - Usually "0"
- `0000D60D-1212-EFDE-1523-785FEF13D123` (notify capable) - Serial number related

#### 6. Service 0000DEAD-1212-EFDE-1523-785FEF13D123
**Characteristics:**
- `0000DEED-1212-EFDE-1523-785FEF13D123` (notify capable)
- `0000D2ED-1212-EFDE-1523-785FEF13D123` (notify capable)
- `0000D3ED-1212-EFDE-1523-785FEF13D123` (notify capable)

#### 7. Secure DFU Service (Nordic firmware updates)
**Characteristics:**
- Buttonless DFU Without Bonds

**Note:** All characteristics support Client Characteristic Configuration Descriptors (CCCD), enabling notifications.

## Implementation Approaches

### Approach 1: Simple BLE Presence Detection (Current Workaround)

**Pros:**
- Simple to implement
- Low memory usage
- Detects trap activations

**Cons:**
- Cannot read kill count directly
- Cannot read battery level
- Cannot read serial number
- Triggers on any advertisement

**ESPHome Config:**
```yaml
esp32_ble_tracker:

binary_sensor:
  - platform: ble_presence
    mac_address: D3:58:7F:15:99:0B
    name: "Goodnature Trap"
```

### Approach 2: BLE Client with GATT Connection (Recommended)

**Pros:**
- Can read all sensor data
- Can parse kill count, battery, serial number
- More reliable data extraction

**Cons:**
- Higher memory usage
- More complex implementation
- Only one device can connect at a time (may interfere with mobile app)

**ESPHome Config Structure:**
```yaml
esp32_ble_tracker:

ble_client:
  - mac_address: D3:58:7F:15:99:0B
    id: goodnature_trap
    auto_connect: true

sensor:
  - platform: ble_client
    type: characteristic
    ble_client_id: goodnature_trap
    name: "Kill Count"
    service_uuid: '0000D00D-1212-EFDE-1523-785FEF13D123'
    characteristic_uuid: '0000D30D-1212-EFDE-1523-785FEF13D123'
    notify: true
    lambda: |-
      // Parse the kill data string format: AAbbbbbbbbCdddEEEEEEEfGG
      // Kill count is at position 20 (index 20)
      if (x.size() >= 21) {
        // Extract kill count character and convert to number
        char kill_char = x[20];
        return (float)(kill_char - '0');
      }
      return NAN;
```

### Approach 3: iBeacon Advertisement Parsing (RECOMMENDED - Most Efficient)

**Pros:**
- No GATT connection needed (won't interfere with app)
- Low memory usage
- Can extract kill count from iBeacon advertisements
- Multiple devices can monitor simultaneously
- Can filter by serial number using advertised service UUIDs

**Cons:**
- Requires custom C++ component
- iBeacon parsing logic needed
- Limited to data in advertisements (kill count only)

**Implementation Details:**

The kill count is at **byte 18** (0-based index 17) of the iBeacon manufacturer data:

```cpp
// Look for manufacturer data with Apple company ID (0x004C)
if (mfg_data.uuid.to_string() == "0x004C" && mfg_data.data.size() == 23) {
  // Verify iBeacon prefix
  if (mfg_data.data[0] == 0x02 && mfg_data.data[1] == 0x15) {
    // Verify Goodnature UUID: B0B0EEE7-B9B0-4BC5-B5E4-F5CB610EB700
    const uint8_t expected_uuid[16] = {
      0xB0, 0xB0, 0xEE, 0xE7, 0xB9, 0xB0, 0x4B, 0xC5,
      0xB5, 0xE4, 0xF5, 0xCB, 0x61, 0x0E, 0xB7, 0x00
    };
    if (memcmp(&mfg_data.data[2], expected_uuid, 16) == 0) {
      // Extract kill count from byte 18 (index 17)
      uint8_t kill_count = mfg_data.data[17];

      // Extract serial from iBeacon Major/Minor
      uint16_t major = (mfg_data.data[18] << 8) | mfg_data.data[19];  // 0xC98F
      uint16_t minor = (mfg_data.data[20] << 8) | mfg_data.data[21];  // 0xE33B
      // Serial: E33B + C98F = E33BC98F

      this->kill_count_ = kill_count;
      if (kill_count_sensor_ != nullptr) {
        kill_count_sensor_->publish_state(kill_count);
      }
    }
  }
}
```

**Alternative: Filter by Serial Number Service UUIDs**

Instead of parsing iBeacon data, you can filter devices by the advertised service UUIDs that encode the serial number:

```cpp
// Check if device advertises the expected serial number service UUIDs
// For serial E33BC98F: look for services 0xE33B and 0xC98F
auto services = device.get_service_uuids();
bool has_serial_part1 = false;
bool has_serial_part2 = false;

for (auto &service : services) {
  if (service == 0xE33B) has_serial_part1 = true;
  if (service == 0xC98F) has_serial_part2 = true;
}

if (has_serial_part1 && has_serial_part2) {
  // This is our device
  // Kill count can also be inferred from 0xD801 vs 0xD802 service
  for (auto &service : services) {
    if (service == 0xD801) kill_count = 0;
    else if (service == 0xD802) kill_count = 1+;
  }
}
```

## Recommended Implementation Path

### Phase 1: iBeacon Advertisement Parsing (PRIORITY)
1. Update `parse_device()` to detect Apple manufacturer data (0x004C)
2. Verify iBeacon prefix (0x02 0x15)
3. Verify Goodnature UUID (B0B0EEE7-B9B0-4BC5-B5E4-F5CB610EB700)
4. Extract kill count from byte 18 (index 17)
5. Extract serial number from Major/Minor fields
6. Filter by MAC address or serial number service UUIDs

**Why this first:** Non-intrusive, works with mobile app, immediate value

### Phase 2: Enhanced Filtering and Multi-Device Support
1. Implement serial number extraction from iBeacon Major/Minor
2. Add filtering by advertised service UUIDs (0xE33B, 0xC98F for serial)
3. Support multiple traps by serial number
4. Add kill count change detection (only notify on increment)

**Why this second:** Better reliability, multiple trap support

### Phase 3: Optional GATT Reading for Advanced Data
1. Implement BLE client connection to service `0000D00D-1212-EFDE-1523-785FEF13D123`
2. Read characteristic `0000D30D-1212-EFDE-1523-785FEF13D123`
3. Parse the full string format to extract:
   - Serial number (bytes 2-9, reversed)
   - Kill count (byte 20)
4. Add time service reading (0000F1AF) for last activation timestamp
5. Add notification support for real-time updates

**Why this third:** Provides additional data, but requires connection that may block app

## Technical Challenges Identified

1. **iBeacon Format:** Device uses Apple's iBeacon protocol, not standard BLE manufacturer data
   - **Solution:** Parse iBeacon structure with 0x02 0x15 prefix and Goodnature UUID
2. **Short Device Name:** Device advertises as "GN" which is very short and may match other devices
   - **Solution:** Filter by Goodnature iBeacon UUID or advertised service UUIDs containing serial
3. **Non-Standard iBeacon Usage:** Kill count stored in non-standard location (byte 18 instead of typical Major/Minor)
   - **Solution:** Custom parsing at byte 18 of manufacturer data
4. **Serial Number Encoding:** Serial embedded in both iBeacon Major/Minor AND advertised service UUIDs
   - **Solution:** Can extract from either source for device identification
5. **Limited Documentation:** No official API documentation from manufacturer
   - **Solution:** Use reverse-engineered protocol from community research
6. **Connection Exclusivity:** Only one BLE connection allowed at a time (GATT)
   - **Solution:** Use advertisement parsing instead of GATT connection

## Resources

### Primary Documentation
- **GitHub Gist by codyc1515:** https://gist.github.com/codyc1515/a6b93850ad81db06cde5e76244cf96f5
  - Most comprehensive reverse-engineered protocol documentation
  - Includes all service and characteristic UUIDs
  - Community contributions and findings

### ESPHome Documentation
- **BLE Client Component:** https://esphome.io/components/ble_client/
- **BLE Client Sensor:** https://esphome.io/components/sensor/ble_client/
- **ESP32 BLE Tracker:** https://esphome.io/components/esp32_ble_tracker/

### Community Implementations
- Home Assistant Community discussions with working examples
- Shelly BLE receiver script showing byte offset 16 parsing
- Various presence-based implementations

### Nordic Semiconductor Documentation
- NRF52 advertisement packet structure
- Manufacturer data format (0xFF type)
- BLE advertising fundamentals

## Next Steps

1. Modify `goodnature_ble_listener.cpp` to parse manufacturer data correctly
2. Test kill count extraction from advertisement data
3. Optionally implement BLE client for full GATT access
4. Document MAC address discovery process for users
5. Add configuration examples for multiple traps

## Summary: Key Implementation Points

### For Advertisement-Based Implementation (Recommended):
1. **Filter devices by name:** `"GN"`
2. **Look for manufacturer data:** Company ID `0x004C` (Apple)
3. **Verify iBeacon format:** First 2 bytes = `0x02 0x15`
4. **Verify Goodnature UUID:** Bytes 2-17 = `B0B0EEE7-B9B0-4BC5-B5E4-F5CB610EB700`
5. **Extract kill count:** Byte 18 (0-based index 17)
6. **Extract serial from Major/Minor:** Bytes 18-21 encode serial number
7. **Alternative filtering:** Check for advertised service UUIDs matching serial parts

### For GATT-Based Implementation (Optional):
1. **Connect to service:** `0000D00D-1212-EFDE-1523-785FEF13D123`
2. **Read characteristic:** `0000D30D-1212-EFDE-1523-785FEF13D123`
3. **Parse hex string format:** Extract kill count at position 20
4. **Enable notifications:** Subscribe to CCCD for real-time updates
5. **Read time service:** `0000F1AF-1212-EFDE-1523-785FEF13D123` for timestamp

### Device Information Summary:
- **Name:** "GN"
- **MAC:** D3:58:7F:15:99:0B (example)
- **Serial:** E33BC98F (example)
- **Firmware:** 1.1.3
- **iBeacon UUID:** B0B0EEE7-B9B0-4BC5-B5E4-F5CB610EB700 (constant for all devices)

## Notes

- The current code has commented-out sections suggesting previous attempts to parse service data
- The `parse_kill_info()` function exists but is never called in current implementation
- **IMPORTANT:** Device uses iBeacon format, not standard BLE manufacturer data
- Advertisement parsing is recommended over GATT to avoid blocking the mobile app
- Multiple ESPHome devices can monitor the same trap simultaneously using advertisement parsing
