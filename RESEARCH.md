# Goodnature A24 Chirp BLE Protocol Research

## Overview

The Goodnature A24 trap uses a BLE-enabled device called "Chirp" (also called Smart Shroud Cap) to track trap activations. The device uses a Nordic Semiconductor NRF52 SoC with Bluetooth 5.4 capability.

## Key Findings

### Device Identification
- **Advertised Name:** "GN" (very short, 2 characters)
- **MAC Address Range:** Primarily D7:AC:E9:XX:XX:XX, though other ranges observed (e.g., CC:53:AE:FD:AA:94)
- **Bluetooth Chip:** Nordic NRF52 SoC

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

### Manufacturer Data Format

The manufacturer data in BLE advertisements follows Nordic NRF52 standard format:
- **Type Byte:** 0xFF (manufacturer specific data)
- **Company ID:** First 2 bytes (Company identifier)
- **Custom Data:** Up to 27 bytes remaining

**Kill Count Location:** Based on community findings, the kill count appears at **byte offset 16** of the advertisement payload.

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

### Approach 3: Advertisement Data Parsing (Most Efficient)

**Pros:**
- No GATT connection needed (won't interfere with app)
- Low memory usage
- Can extract kill count from advertisements
- Multiple devices can monitor simultaneously

**Cons:**
- Requires custom C++ component
- More complex parsing logic
- Limited to data in advertisements

**Implementation Note:** This requires extending the current `goodnature_ble_listener.cpp` to properly parse manufacturer data at byte offset 16.

## Recommended Implementation Path

### Phase 1: Fix Current Advertisement Parsing
1. Update `parse_device()` to properly extract manufacturer data
2. Parse kill count from byte offset 16 of manufacturer data
3. Test with simple presence detection

### Phase 2: Enhanced GATT Reading
1. Implement BLE client connection to service `0000D00D-1212-EFDE-1523-785FEF13D123`
2. Read characteristic `0000D30D-1212-EFDE-1523-785FEF13D123`
3. Parse the full string format to extract:
   - Serial number (bytes 2-9, reversed)
   - Kill count (byte 20)
4. Add notification support for real-time updates

### Phase 3: Complete Data Extraction
1. Add time service reading for last activation timestamp
2. Add device info service for battery level and status
3. Implement proper error handling and reconnection logic

## Technical Challenges Identified

1. **Short Device Name:** Device advertises as "GN" which is very short and may match other devices
2. **Custom Data Format:** Non-standard string format for kill data requires custom parsing
3. **Reversed Serial Numbers:** Serial number bytes need to be reversed in pairs
4. **Limited Documentation:** No official API documentation from manufacturer
5. **Connection Exclusivity:** Only one BLE connection allowed at a time

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

## Notes

- The current code has commented-out sections suggesting previous attempts to parse service data
- The `parse_kill_info()` function exists but is never called in current implementation
- Need to determine whether to use advertisement parsing or GATT connection based on use case
- Consider whether simultaneous app usage is required (affects implementation choice)
