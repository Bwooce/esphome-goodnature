# ESPHome Goodnature BLE Monitor

ESPHome custom component to monitor Goodnature A24 pest trap kill counts via Bluetooth Low Energy (BLE).

## ⚡ Key Discovery

The Goodnature Chirp uses **Apple's iBeacon protocol** (not standard BLE), which is why it appeared unusual. This component properly parses the iBeacon format to extract kill counts.

## Quick Start

### 1. Hardware Requirements
- ESP32 board (ESP8266 doesn't support BLE)
- Goodnature A24 trap with Chirp counter

### 2. Find Your Trap's MAC Address

**Option A: Use a phone app**
- Download "nRF Connect" (Android/iOS) or "BLE Scanner" app
- Activate your trap (trigger it)
- Look for a device named "GN"
- Note its MAC address (e.g., D3:58:7F:15:99:0B)

**Option B: Use ESPHome logs**
- Flash the ESP32 with logging enabled
- Check logs for devices named "GN"
- The MAC address will be shown

### 3. Configuration

Edit `goodnature_ble.yaml`:

```yaml
sensor:
  - platform: goodnature_ble
    mac_address: "D3:58:7F:15:99:0B"  # Replace with your trap's MAC
    kill_count:
      name: "Trap Kill Count"
```

### 4. Flash and Monitor

```bash
# Compile and upload
esphome run goodnature_ble.yaml

# Monitor logs
esphome logs goodnature_ble.yaml
```

### 5. What You Should See

When the trap activates, you'll see logs like:
```
[goodnature_ble] Found Goodnature device: D3:58:7F:15:99:0B
[goodnature_ble] Found Apple manufacturer data (iBeacon):
[goodnature_ble]    02 15 B0 B0 EE E7 B9 B0 4B C5 B5 E4 F5 CB 61 0E B7 01 8F C9 3B E3 37
[goodnature_ble] Goodnature iBeacon: D3:58:7F:15:99:0B (Serial: E33BC98F), Kill count: 1, TX Power: 55 dBm
```

## Multiple Traps

To monitor multiple traps, add more sensor entries:

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

## How It Works

### iBeacon Format

The Goodnature Chirp broadcasts iBeacon advertisements:

```
02 15 B0 B0 EE E7 B9 B0 4B C5 B5 E4 F5 CB 61 0E B7 [01] 8F C9 3B E3 37
│  │  └─────────────────── UUID (16 bytes) ──────────────────┘  │  └─Major─┘ └─Minor─┘
│  └─ iBeacon prefix                                              │
└─ Length                                                         └─ Kill count
```

- **Company ID:** 0x004C (Apple)
- **UUID:** B0B0EEE7-B9B0-4BC5-B5E4-F5CB610EB700 (Goodnature constant)
- **Kill Count:** Byte 18 (increments with each kill)
- **Serial Number:** Encoded in Major/Minor fields

### Non-Intrusive Monitoring

This implementation:
- Parses advertisement data only (no GATT connection)
- Doesn't interfere with the official mobile app
- Allows multiple ESP32 devices to monitor the same trap
- Low power consumption

### ⚠️ Note: Trap Continues Broadcasting

Currently, this implementation **does not "calm" the trap** after reading the kill count. The trap will continue broadcasting until:
- The official mobile app connects and calms it
- It times out (battery saving feature)

**Why not calm it?**
- We haven't yet discovered the correct GATT command to calm the device
- See [CALMING_GUIDE.md](CALMING_GUIDE.md) for how to help discover this
- Current implementation works perfectly for monitoring without calming

## Troubleshooting

### "Not Goodnature device"
- Device must advertise with name "GN"
- Activate the trap to ensure it's broadcasting
- Check you're within BLE range (10-30 feet)

### Kill count not updating
- Verify the MAC address is correct
- Check DEBUG logs show iBeacon data being received
- Ensure byte 17 (kill count) is incrementing in hex dump
- Compare with official app to verify trap is working

### No devices detected
- Verify ESP32 BLE is working (see other BLE devices in logs)
- Move ESP32 closer to trap
- Activate trap manually to trigger broadcast

## Technical Details

See detailed documentation:
- [RESEARCH.md](RESEARCH.md) - Complete protocol reverse engineering
- [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md) - Implementation approaches

## File Structure

```
components/goodnature_ble/
├── __init__.py                      # Python integration
├── sensor.py                        # Sensor platform definition
├── goodnature_ble_listener.h       # C++ header
└── goodnature_ble_listener.cpp     # iBeacon parser implementation

goodnature_ble.yaml                  # Example configuration
```

## Home Assistant Integration

Once the ESP32 is connected to Home Assistant via the API, the kill count sensor will automatically appear:

```yaml
# Example automation
automation:
  - alias: "Trap Activated"
    trigger:
      platform: state
      entity_id: sensor.front_yard_trap_kill_count
    action:
      - service: notify.mobile_app
        data:
          message: "Trap caught something! Count: {{ states('sensor.front_yard_trap_kill_count') }}"
```

## Credits

- Reverse engineering by [codyc1515](https://gist.github.com/codyc1515/a6b93850ad81db06cde5e76244cf96f5)
- iBeacon format discovery through local testing
- ESPHome community

## License

MIT License - See LICENSE file for details
