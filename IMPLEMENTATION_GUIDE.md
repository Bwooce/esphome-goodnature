# Goodnature BLE Implementation Guide

## Overview

This guide provides concrete implementation approaches for integrating Goodnature A24 Chirp traps with ESPHome.

## Option 1: Advertisement-Based Parsing (Recommended)

This approach parses manufacturer data from BLE advertisements without establishing a GATT connection. This allows the official app to continue working while ESPHome monitors the device.

### Advantages
- No interference with official mobile app
- Multiple ESPHome devices can monitor simultaneously
- Lower memory usage
- No connection timeout issues

### Implementation in `goodnature_ble_listener.cpp`

```cpp
bool GoodnatureBleListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  // Check device name
  if (strcmp(device.get_name().c_str(), "GN") != 0) {
    return false;
  }

  ESP_LOGD(TAG, "Found Goodnature device: %s", device.address_str().c_str());

  // Parse manufacturer data
  auto mfg_datas = device.get_manufacturer_datas();
  for (auto &mfg_data : mfg_datas) {
    ESP_LOGD(TAG, "Manufacturer data UUID: %s, length: %i",
             mfg_data.uuid.to_string().c_str(), mfg_data.data.size());

    // Check if we have enough data
    if (mfg_data.data.size() >= 17) {
      // Kill count is at byte offset 16 according to community research
      uint8_t kill_count = mfg_data.data[16];

      // Validate kill count is reasonable (0-99 for example)
      if (kill_count >= 0 && kill_count <= 99) {
        // If MAC address matches or not configured, process this device
        if (device.address_uint64() == this->mac_address_ || this->mac_address_ == 0) {
          this->kill_count_ = kill_count;

          ESP_LOGI(TAG, "Goodnature device: %s, Kill count: %d",
                   device.address_str().c_str(), this->kill_count_);

          if (kill_count_sensor_ != nullptr) {
            kill_count_sensor_->publish_state(this->kill_count_);
          }

          return true;
        }
      }
    }
  }

  // Alternative: Check service data
  auto service_datas = device.get_service_datas();
  for (auto &service_data : service_datas) {
    ESP_LOGD(TAG, "Service data UUID: %s, length: %i",
             service_data.uuid.to_string().c_str(), service_data.data.size());

    // Check for kill info service UUID: 0000D00D-1212-EFDE-1523-785FEF13D123
    // Note: May need to check if service_data.uuid matches this
    if (service_data.data.size() >= 21) {
      // Parse the format: AAbbbbbbbbCdddEEEEEEEfGG
      // Kill count is at position 20
      char kill_char = service_data.data[20];
      if (kill_char >= '0' && kill_char <= '9') {
        this->kill_count_ = kill_char - '0';

        // Extract serial number (positions 2-9, reversed)
        if (service_data.data.size() >= 10) {
          std::string serial;
          // Reverse byte pairs
          for (int i = 8; i >= 2; i -= 2) {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02X", service_data.data[i]);
            serial += hex;
            if (i > 2) {
              snprintf(hex, sizeof(hex), "%02X", service_data.data[i-1]);
              serial += hex;
            }
          }
          this->serial_ = serial;

          ESP_LOGI(TAG, "Goodnature device: %s (Serial: %s), Kill count: %d",
                   device.address_str().c_str(), this->serial_.c_str(), this->kill_count_);
        }

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

### Step 2: Analyze Raw Data

Enable hex buffer printing in the code (already present in current implementation):

```cpp
ESP_LOGW(TAG, "Manufacturer data - %s: (length %i)",
         data.uuid.to_string().c_str(), data.data.size());
print_buffer(&data.data[0], data.data.size());
```

### Step 3: Identify Kill Count Location

Trigger the trap and observe which bytes change in the manufacturer/service data.

### Step 4: Validate Parsing

Compare parsed kill count with the official mobile app.

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
1. Verify byte offset 16 contains kill count
2. Try parsing service data instead of manufacturer data
3. Check data format matches expected structure
4. Enable hex buffer printing to inspect raw data

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

1. **Immediate:** Test advertisement parsing with real device
2. **Short-term:** Validate kill count byte offset and data format
3. **Medium-term:** Implement full GATT client for battery/serial
4. **Long-term:** Add Home Assistant automations for notifications

## References

- [GitHub Gist: Goodnature A24 Chirp BLE Protocol](https://gist.github.com/codyc1515/a6b93850ad81db06cde5e76244cf96f5)
- [ESPHome BLE Client Documentation](https://esphome.io/components/ble_client/)
- [ESPHome BLE Tracker Documentation](https://esphome.io/components/esp32_ble_tracker/)
