# Goodnature BLE Implementation Guide

## Overview

This guide provides concrete implementation approaches for integrating Goodnature A24 Chirp traps with ESPHome.

## Option 1: iBeacon Advertisement Parsing (Recommended)

⚡ **CRITICAL:** The Goodnature Chirp uses Apple's iBeacon format, not standard BLE manufacturer data!

This approach parses iBeacon advertisement data without establishing a GATT connection. This allows the official app to continue working while ESPHome monitors the device.

### Advantages
- No interference with official mobile app
- Multiple ESPHome devices can monitor simultaneously
- Lower memory usage
- No connection timeout issues
- Can extract both kill count and serial number

### iBeacon Packet Structure (Verified from Local Testing)

```
Manufacturer Data (Company ID 0x004C - Apple):
02 15 B0 B0 EE E7 B9 B0 4B C5 B5 E4 F5 CB 61 0E B7 [01] 8F C9 3B E3 37
│  │  └─────────────────── UUID (16 bytes) ──────────────────┘  │  └─Major─┘ └─Minor─┘ └TX
│  └─ iBeacon prefix                                              │
└─ Length                                                         └─ KILL COUNT (byte 18, index 17)

Serial Number E33BC98F is encoded in:
- Major: 8F C9 (0xC98F)
- Minor: 3B E3 (0xE33B)
```

### Implementation in `goodnature_ble_listener.cpp`

```cpp
bool GoodnatureBleListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  // Check device name first (quick filter)
  if (strcmp(device.get_name().c_str(), "GN") != 0) {
    return false;
  }

  ESP_LOGD(TAG, "Found Goodnature device: %s", device.address_str().c_str());

  // Parse manufacturer data for iBeacon format
  auto mfg_datas = device.get_manufacturer_datas();
  for (auto &mfg_data : mfg_datas) {
    ESP_LOGD(TAG, "Manufacturer data UUID: %s, length: %i",
             mfg_data.uuid.to_string().c_str(), mfg_data.data.size());

    // Check for Apple company ID (0x004C) and correct iBeacon length (23 bytes)
    if (mfg_data.uuid.to_string() == "0x004C" && mfg_data.data.size() == 23) {

      // Verify iBeacon prefix (0x02 0x15)
      if (mfg_data.data[0] != 0x02 || mfg_data.data[1] != 0x15) {
        ESP_LOGD(TAG, "Not iBeacon format");
        continue;
      }

      // Verify Goodnature UUID: B0B0EEE7-B9B0-4BC5-B5E4-F5CB610EB700
      const uint8_t goodnature_uuid[16] = {
        0xB0, 0xB0, 0xEE, 0xE7, 0xB9, 0xB0, 0x4B, 0xC5,
        0xB5, 0xE4, 0xF5, 0xCB, 0x61, 0x0E, 0xB7, 0x00
      };

      if (memcmp(&mfg_data.data[2], goodnature_uuid, 16) != 0) {
        ESP_LOGD(TAG, "Not Goodnature UUID");
        continue;
      }

      // Extract kill count from byte 18 (0-based index 17)
      uint8_t kill_count = mfg_data.data[17];

      // Extract serial number from iBeacon Major and Minor
      // Major = bytes 18-19, Minor = bytes 20-21
      uint16_t major = (mfg_data.data[18] << 8) | mfg_data.data[19];
      uint16_t minor = (mfg_data.data[20] << 8) | mfg_data.data[21];

      // Construct serial: Minor (high) + Major (low)
      // Example: minor=0xE33B, major=0xC98F → serial=E33BC98F
      char serial_str[9];
      snprintf(serial_str, sizeof(serial_str), "%04X%04X", minor, major);
      this->serial_ = serial_str;

      // TX Power at byte 22 (optional, for RSSI calculations)
      int8_t tx_power = (int8_t)mfg_data.data[22];

      ESP_LOGI(TAG, "Goodnature iBeacon: %s (Serial: %s), Kill count: %d, TX Power: %d",
               device.address_str().c_str(), this->serial_.c_str(), kill_count, tx_power);

      // If MAC address matches or not configured, process this device
      if (device.address_uint64() == this->mac_address_ || this->mac_address_ == 0) {
        this->kill_count_ = kill_count;

        if (kill_count_sensor_ != nullptr) {
          kill_count_sensor_->publish_state(this->kill_count_);
        }

        return true;
      }
    }
  }

  return false;
}
```

### ESPHome YAML Configuration

```yaml
esphome:
  name: goodnature_monitor
  platform: ESP32
  board: esp32dev

wifi:
  ssid: "Your_SSID"
  password: "Your_Password"

api:
ota:

logger:
  level: DEBUG

# Enable BLE tracking
esp32_ble_tracker:
  scan_parameters:
    interval: 1100ms
    window: 1100ms
    active: true

# Custom component for Goodnature
sensor:
  - platform: goodnature_ble
    mac_address: "D3:58:7F:15:99:0B"
    kill_count:
      name: "Trap Kill Count"
      id: trap_kill_count
```

### Alternative: Filter by Service UUID (Simpler Approach)

Instead of parsing the iBeacon data, you can identify specific traps by their advertised service UUIDs which encode the serial number:

```cpp
bool GoodnatureBleListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  // Check device name
  if (strcmp(device.get_name().c_str(), "GN") != 0) {
    return false;
  }

  // Extract serial number parts from advertised service UUIDs
  // For device with serial E33BC98F, look for services 0xE33B and 0xC98F
  auto service_uuids = device.get_service_uuids();

  uint16_t serial_minor = 0;
  uint16_t serial_major = 0;
  uint16_t kill_count_service = 0;

  for (auto &uuid : service_uuids) {
    uint16_t uuid_short = uuid.get_uuid16();

    // Check for serial number parts (these will be unique to each device)
    // First part usually around 0xE33B range
    // Second part usually around 0xC98F range
    if (uuid_short >= 0xE000 && uuid_short <= 0xFFFF) {
      serial_minor = uuid_short;
    } else if (uuid_short >= 0xC000 && uuid_short <= 0xD000) {
      serial_major = uuid_short;
    }

    // Kill count service changes: 0xD801 (no kills), 0xD802 (1+ kills)
    if (uuid_short == 0xD801 || uuid_short == 0xD802) {
      kill_count_service = uuid_short;
    }
  }

  // If we found serial number services, this is our device
  if (serial_minor != 0 && serial_major != 0) {
    char serial_str[9];
    snprintf(serial_str, sizeof(serial_str), "%04X%04X", serial_minor, serial_major);

    ESP_LOGI(TAG, "Goodnature device with serial: %s", serial_str);

    // Infer kill count from service UUID (rough indicator)
    if (kill_count_service == 0xD801) {
      ESP_LOGD(TAG, "Kill count service indicates 0 kills");
    } else if (kill_count_service == 0xD802) {
      ESP_LOGD(TAG, "Kill count service indicates 1+ kills");
    }

    return true;
  }

  return false;
}
```

**Note:** This approach is simpler but less precise for kill count. Use the iBeacon parsing method above for accurate kill count values.

## Option 2: BLE Client with GATT Connection

This approach establishes a full GATT connection to read characteristics directly. Note that this may prevent the mobile app from connecting simultaneously.

### ESPHome Configuration

```yaml
esphome:
  name: goodnature_monitor
  platform: ESP32
  board: esp32dev

wifi:
  ssid: "Your_SSID"
  password: "Your_Password"

api:
ota:

logger:
  level: DEBUG

# Enable BLE tracking
esp32_ble_tracker:

# BLE Client configuration
ble_client:
  - mac_address: D3:58:7F:15:99:0B
    id: goodnature_trap
    auto_connect: true
    on_connect:
      then:
        - logger.log: "Connected to Goodnature trap"
    on_disconnect:
      then:
        - logger.log: "Disconnected from Goodnature trap"

# Sensors reading from BLE characteristics
sensor:
  # Kill count from kill data characteristic
  - platform: ble_client
    type: characteristic
    ble_client_id: goodnature_trap
    name: "Kill Count"
    service_uuid: '0000D00D-1212-EFDE-1523-785FEF13D123'
    characteristic_uuid: '0000D30D-1212-EFDE-1523-785FEF13D123'
    notify: true
    icon: "mdi:counter"
    accuracy_decimals: 0
    lambda: |-
      // Parse kill data string format: AAbbbbbbbbCdddEEEEEEEfGG
      // Kill count is at position 20 (byte index 20)
      if (x.size() >= 21) {
        char kill_char = x[20];
        if (kill_char >= '0' && kill_char <= '9') {
          return (float)(kill_char - '0');
        }
      }
      return NAN;
    on_value:
      then:
        - logger.log:
            format: "Kill count updated: %.0f"
            args: ['x']

  # Time since last activation (from time service)
  - platform: ble_client
    type: characteristic
    ble_client_id: goodnature_trap
    name: "Last Activation Time"
    service_uuid: '0000F1AE-1212-EFDE-1523-785FEF13D123'
    characteristic_uuid: '0000F1AF-1212-EFDE-1523-785FEF13D123'
    icon: "mdi:clock"
    unit_of_measurement: "min"
    accuracy_decimals: 0
    lambda: |-
      // Time is in milliseconds / 60000, encoded as Little Endian
      if (x.size() >= 4) {
        uint32_t time_minutes = 0;
        time_minutes |= (uint32_t)x[0];
        time_minutes |= (uint32_t)x[1] << 8;
        time_minutes |= (uint32_t)x[2] << 16;
        time_minutes |= (uint32_t)x[3] << 24;
        return (float)time_minutes;
      }
      return NAN;

# Text sensor for serial number
text_sensor:
  - platform: ble_client
    ble_client_id: goodnature_trap
    name: "Trap Serial Number"
    service_uuid: '0000DE11-1212-EFDE-1523-785FEF13D123'
    characteristic_uuid: '0000DE12-1212-EFDE-1523-785FEF13D123'
    icon: "mdi:identifier"
```

## Option 3: Hybrid Approach (Best of Both Worlds)

Use advertisement parsing for kill count detection (non-intrusive), but optionally connect to read additional details when needed.

### C++ Component Structure

```cpp
// goodnature_ble_listener.h
class GoodnatureBleListener : public esp32_ble_tracker::ESPBTDeviceListener,
                                public Component {
 public:
  void set_kill_count_sensor(sensor::Sensor *sensor) { kill_count_sensor_ = sensor; }
  void set_battery_level_sensor(sensor::Sensor *sensor) { battery_level_sensor_ = sensor; }
  void set_serial_sensor(text_sensor::TextSensor *sensor) { serial_sensor_ = sensor; }
  void set_mac_address(uint64_t address) { this->mac_address_ = address; }

  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  void dump_config() override;

  // Optional: methods to trigger GATT connection for detailed data
  void request_detailed_info();

 protected:
  uint64_t mac_address_{0};
  sensor::Sensor *kill_count_sensor_{nullptr};
  sensor::Sensor *battery_level_sensor_{nullptr};
  text_sensor::TextSensor *serial_sensor_{nullptr};

  int last_kill_count_{-1};
  std::string serial_;

  // Parse methods
  bool parse_manufacturer_data(const std::vector<uint8_t> &data);
  bool parse_service_data(const esp_ble_gap_cb_param_t::ble_scan_result_evt_param::ble_adv_data_t &adv_data);
};
```

## Testing and Debugging

### Step 1: Verify Device Discovery

```yaml
logger:
  level: DEBUG
  logs:
    esp32_ble_tracker: DEBUG
    goodnature_ble: DEBUG
```

Upload and monitor logs to see:
- Device name "GN" being detected
- MAC address
- Manufacturer data contents
- Service data contents

### Step 2: Analyze Raw iBeacon Data

Enable hex buffer printing in the code (already present in current implementation):

```cpp
ESP_LOGW(TAG, "Manufacturer data - %s: (length %i)",
         data.uuid.to_string().c_str(), data.data.size());
print_buffer(&data.data[0], data.data.size());
```

Expected output for Goodnature device:
```
Manufacturer data - 0x004C: (length 23)
02 15 B0 B0 EE E7 B9 B0 4B C5 B5 E4 F5 CB 61 0E B7 [01] 8F C9 3B E3 37
```

Key indicators:
- Company ID: `0x004C` (Apple)
- Length: 23 bytes
- Prefix: `02 15` (iBeacon)
- UUID: `B0B0EEE7-B9B0-4BC5-B5E4-F5CB610EB700` (Goodnature)
- Byte 18 (index 17): Kill count (changes after strike)

### Step 3: Verify iBeacon Parsing

Trigger the trap and observe:
1. Byte 18 (index 17) should increment
2. Before strike: `00`, After strike: `01`
3. Serial number in Major/Minor stays constant
4. TX Power (last byte) may vary

Example from local testing:
```
Before: 02 15 B0 B0 EE E7 B9 B0 4B C5 B5 E4 F5 CB 61 0E B7 00 8F C9 3B E3 36
After:  02 15 B0 B0 EE E7 B9 B0 4B C5 B5 E4 F5 CB 61 0E B7 01 8F C9 3B E3 37
                                                            ^^             ^^
                                                        Kill count      TX Power
```

### Step 4: Verify Service UUIDs

Check that the device advertises service UUIDs with serial number parts:
```
Advertised service UUIDs:
  - 0xC98F  (serial part 1)
  - 0xE33B  (serial part 2)
  - 0x0036  (constant)
  - 0x1234  (constant)
  - 0xD802  (kill count indicator: 0xD801=0, 0xD802=1+)
  - 0x1500  (constant)
```

### Step 5: Validate Parsing

Compare parsed kill count with the official mobile app to ensure accuracy.

## Multiple Trap Support

```yaml
sensor:
  - platform: goodnature_ble
    mac_address: "D3:58:7F:15:99:0B"
    kill_count:
      name: "Front Yard Trap Kill Count"

  - platform: goodnature_ble
    mac_address: "D7:AC:E9:AA:BB:CC"
    kill_count:
      name: "Back Yard Trap Kill Count"
```

## MAC Address Discovery

Since the trap doesn't advertise until it's activated, you need to:

1. Activate the trap (trigger it manually)
2. Use a BLE scanner app on your phone (e.g., nRF Connect, BLE Scanner)
3. Look for devices named "GN"
4. Note the MAC address

Alternatively, configure without MAC address to scan all "GN" devices:

```cpp
// In parse_device(), if mac_address_ is 0, accept any GN device
if (device.address_uint64() == this->mac_address_ || this->mac_address_ == 0) {
  // Process device
}
```

## Performance Considerations

### Memory Usage
- BLE Client uses significant RAM (~40-50KB per connection)
- Limit to 3 simultaneous BLE connections maximum
- Advertisement parsing uses minimal memory

### Scan Parameters
```yaml
esp32_ble_tracker:
  scan_parameters:
    interval: 1100ms  # Balance between responsiveness and power
    window: 1100ms    # 100% duty cycle for best detection
    active: true      # Active scanning for service data
```

### Battery Impact on ESP32
- Continuous BLE scanning increases power consumption
- Consider implementing sleep modes if battery powered
- Or use `active: false` for passive scanning (lower power)

## Troubleshooting

### Device Not Detected
1. Verify trap is within BLE range (typically 10-30 feet)
2. Activate trap to ensure it's broadcasting
3. Check MAC address is correct
4. Enable DEBUG logging to see all discovered devices

### Kill Count Not Updating
1. **Verify iBeacon format:** Check manufacturer data is from 0x004C (Apple) with 23 bytes
2. **Verify Goodnature UUID:** Bytes 2-17 must match B0B0EEE7-B9B0-4BC5-B5E4-F5CB610EB700
3. **Check kill count location:** Byte 18 (0-based index 17) should increment after strike
4. **Enable hex buffer printing:** Inspect raw iBeacon data to see if byte 17 changes
5. **Compare with service UUIDs:** Check if 0xD801 changes to 0xD802 after strike

### Connection Failures (BLE Client)
1. Ensure only one device is connected at a time
2. Check if mobile app is connected (disconnect it)
3. Verify service/characteristic UUIDs are correct
4. Increase connection timeout values

### False Triggers
1. Add device name verification (must be exactly "GN")
2. Add MAC address filtering
3. Implement kill count change detection (only trigger on increment)
4. Add debouncing logic

## Next Steps

1. **Immediate:** Implement iBeacon parsing in `parse_device()` function
2. **Test:** Verify kill count extraction from byte 18 (index 17) of iBeacon data
3. **Validate:** Compare parsed values with official mobile app
4. **Enhance:** Add serial number extraction from Major/Minor fields
5. **Optional:** Implement GATT client for additional data (battery, timestamp)
6. **Automate:** Add Home Assistant automations for notifications

## Key Findings Summary

⚡ **Critical Discovery:** Goodnature Chirp uses **Apple's iBeacon protocol** (Company ID 0x004C), NOT standard BLE manufacturer data!

**iBeacon Structure:**
- UUID: `B0B0EEE7-B9B0-4BC5-B5E4-F5CB610EB700` (constant for all Goodnature devices)
- Kill count at byte 18 (0-based index 17)
- Serial number encoded in Major/Minor fields
- Also advertised as service UUIDs for easy filtering

**Implementation Priority:**
1. Parse iBeacon manufacturer data (recommended)
2. Alternative: Filter by service UUIDs (simpler)
3. Optional: GATT connection for advanced features

## References

- [GitHub Gist: Goodnature A24 Chirp BLE Protocol](https://gist.github.com/codyc1515/a6b93850ad81db06cde5e76244cf96f5)
- [ESPHome BLE Client Documentation](https://esphome.io/components/ble_client/)
- [ESPHome BLE Tracker Documentation](https://esphome.io/components/esp32_ble_tracker/)
