# Calming the Goodnature Trap

## The Problem

The Goodnature Chirp continuously broadcasts BLE packets after a kill event until something "calms" it. This:
- Drains the battery faster
- Causes repeated notifications if multiple devices are listening
- Is the intended behavior to ensure you see the notification

## What We Know

From the research and community findings:
1. The official app connects via BLE GATT
2. The app sends "a value" to calm the Chirp
3. After calming, the device stops transmitting until the next kill
4. Only one device can connect at a time

## What We Don't Know (Yet)

🔍 **We need to discover:**
1. Which characteristic to write to
2. What value to write
3. Whether we need to read before writing

## Candidate Characteristics

Based on the GATT service map:

### Service: 0000D00D-1212-EFDE-1523-785FEF13D123 (Kill Information)

**Option 1: 0000D20D (Test Strike Data)**
- Properties: Notify, Writable
- Purpose: Unknown, but "test strike" suggests it might accept commands
- **Most likely candidate for calming**

**Option 2: 0000D30D (Main Kill Data)**
- Properties: Notify capable
- Purpose: Contains the kill count and serial data
- Probably read-only despite not being explicitly marked

### Other Services

**Service: 0000FADE**
- 0000FAD1: Usually value 218 or 234
- 0000FAD2: Usually value 0 or 1
- Could be status/control registers

## Methods to Discover the Calm Command

### Method 1: Bluetooth Sniffing (Recommended)

Use a Bluetooth sniffer to capture what the official app sends:

**Tools:**
- **Nordic nRF Sniffer** with Wireshark (most detailed)
- **Android HCI Snoop** (if you have Android phone)
- **iOS PacketLogger** (if you have iOS with Xcode)

**Steps:**
1. Set up BLE sniffer
2. Open official Goodnature app
3. Trigger trap or view a trap with kills
4. Capture the BLE traffic
5. Look for GATT Write operations
6. Note the characteristic UUID and value written

### Method 2: nRF Connect Exploration

Use nRF Connect app to manually test:

**Steps:**
1. Trigger your trap so it's broadcasting
2. Open nRF Connect app on your phone
3. Connect to device "GN"
4. Navigate to service 0000D00D-1212-EFDE-1523-785FEF13D123
5. Try writing to characteristic 0000D20D:
   - Try: `0x00` (null/acknowledge)
   - Try: `0x01` (confirm)
   - Try: Empty write
   - Try: `0xFF` (reset)
6. After each write, check if device stops broadcasting
7. Disconnect and scan to see if "GN" still appears

### Method 3: Trial and Error with ESPHome

Test different write values programmatically:

```yaml
ble_client:
  - mac_address: D3:58:7F:15:99:0B
    id: goodnature_trap_client

button:
  - platform: template
    name: "Test Calm - Write 0x00"
    on_press:
      - ble_client.connect: goodnature_trap_client
      - delay: 1s
      - ble_client.ble_write:
          id: goodnature_trap_client
          service_uuid: '0000D00D-1212-EFDE-1523-785FEF13D123'
          characteristic_uuid: '0000D20D-1212-EFDE-1523-785FEF13D123'
          value: [0x00]
      - delay: 500ms
      - ble_client.disconnect: goodnature_trap_client

  - platform: template
    name: "Test Calm - Write 0x01"
    on_press:
      - ble_client.connect: goodnature_trap_client
      - delay: 1s
      - ble_client.ble_write:
          id: goodnature_trap_client
          service_uuid: '0000D00D-1212-EFDE-1523-785FEF13D123'
          characteristic_uuid: '0000D20D-1212-EFDE-1523-785FEF13D123'
          value: [0x01]
      - delay: 500ms
      - ble_client.disconnect: goodnature_trap_client
```

After each test, check if the device stops appearing in BLE scans.

## Expected Behavior After Calming

✅ Success indicators:
- Device "GN" stops appearing in BLE scanner
- ESPHome stops seeing iBeacon advertisements
- Battery life improves
- Next trap trigger causes device to broadcast again

❌ If it doesn't work:
- Device continues broadcasting
- Try different characteristic
- Try different write value
- Check if reading is required first

## Testing Procedure

1. **Baseline:** Trigger trap, note continuous broadcasting
2. **Connect:** Use ESPHome or nRF Connect to connect
3. **Read:** Optionally read the kill data characteristic
4. **Write:** Send calm command to suspected characteristic
5. **Verify:** Disconnect and check if device stops broadcasting
6. **Reset:** Wait for next trap trigger to verify it broadcasts again

## Possible Write Values to Test

Based on common BLE patterns:

```
0x00        - Acknowledge/Clear
0x01        - Confirm/OK
0xFF        - Reset/Stop
[]          - Empty write
[0x00, 0x00] - Double byte clear
```

## Implementation After Discovery

Once we know the correct characteristic and value:

```cpp
// In goodnature_ble_listener.cpp
void GoodnatureBleListener::calm_device() {
  // Connect to device
  // Write to characteristic 0x0000D20D (or discovered characteristic)
  // value: [discovered_value]
  // Disconnect
}
```

Or in YAML:
```yaml
on_value:
  then:
    - ble_client.connect: trap_client
    - ble_client.ble_write:
        id: trap_client
        service_uuid: '0000D00D-1212-EFDE-1523-785FEF13D123'
        characteristic_uuid: '0000D20D-1212-EFDE-1523-785FEF13D123'
        value: [0x??]  # Discovered value
    - ble_client.disconnect: trap_client
```

## Alternative: Don't Calm (Current Implementation)

**Pros of NOT calming:**
- Simpler implementation (what we have now)
- No interference with mobile app
- Multiple ESPHome devices can all see the event
- No risk of preventing legitimate app notifications

**Cons of NOT calming:**
- Battery drain on the Chirp
- Continuous BLE traffic
- Could miss next event if broadcast never stops (unlikely, probably has timeout)

## Recommendations

### For Now (Testing Phase):
1. Use current advertisement-only implementation
2. Let trap broadcast continuously for testing
3. Verify kill count detection works correctly
4. Battery impact likely minimal for testing

### For Production:
1. Discover the calm command using Method 1 or 2
2. Implement conditional calming:
   ```yaml
   # Only calm if enabled
   - if:
       condition:
         switch.is_on: auto_calm_trap
       then:
         - ble_client.connect: trap_client
         # ... write calm command
   ```

### Collaborative Approach:
1. Test with nRF Connect app first
2. Document findings in GitHub issue
3. Share with community (codyc1515's gist)
4. Update implementation based on findings

## Community Research Needed

To complete this implementation, we need someone to:
- Use Bluetooth sniffer with official app
- Or manually test characteristics with nRF Connect
- Document the working calm command
- Share findings for everyone's benefit

This would be a valuable contribution to the reverse engineering effort!

## Files to Update After Discovery

1. `RESEARCH.md` - Add calm command documentation
2. `IMPLEMENTATION_GUIDE.md` - Add calm implementation examples
3. `goodnature_ble_listener.cpp` - Add calm_device() method
4. `goodnature_ble.yaml` - Add auto-calm configuration
5. `README.md` - Document calm feature

## Next Steps

1. **Test current implementation** without calming (works fine)
2. **Optionally investigate** calm command using guides above
3. **Share findings** with community
4. **Update code** once calm command is discovered

For now, the implementation works perfectly for monitoring kill counts, even without the calming feature!
