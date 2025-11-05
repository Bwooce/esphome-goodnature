# Testing Guide: Discovering the Calm Command

This guide will help you discover the correct command to "calm" the Goodnature trap.

## What You'll Need

- ESP32 flashed with `goodnature_test_calm.yaml`
- Goodnature trap
- Something to trigger the trap (manual activation works fine)
- Home Assistant (or ESPHome web interface) to press test buttons
- Patience for systematic testing

## Understanding the Test Setup

### No Pairing Required!

Good news: The Goodnature trap **doesn't require pairing/bonding**. You can connect directly without any authentication. This makes testing easy!

### Test Configuration

The `goodnature_test_calm.yaml` includes:
- ✅ BLE client configured for your trap
- ✅ 8 test buttons for different calm commands
- ✅ Status sensor showing if trap is broadcasting
- ✅ Manual connect/disconnect buttons
- ✅ Detailed logging

## Testing Procedure

### Step 1: Flash the Test Configuration

```bash
esphome run goodnature_test_calm.yaml
```

Watch the logs:
```bash
esphome logs goodnature_test_calm.yaml
```

### Step 2: Trigger Your Trap

Manually activate the trap so it starts broadcasting. You should see in the logs:
```
[esp32_ble_tracker] Found device: GN (D3:58:7F:15:99:0B)
[ble_presence] Trap Is Broadcasting: ON
```

### Step 3: Run Tests Systematically

In Home Assistant (or ESPHome web UI), you'll see test buttons. **Test them one at a time:**

#### Test 1: Write 0x00 to D30D (Main Kill Data)
1. Press button "Test 1: Write 0x00 to D30D"
2. Watch logs for:
   ```
   ✅ Connected to trap!
   TEST 1: Writing 0x00 to characteristic D30D...
   ❌ Disconnected from trap
   ```
3. Wait 10 seconds
4. **Check:** Does "GN" device still appear in BLE scans?
   - Look at "Trap Is Broadcasting" sensor
   - Or check logs for continued device discoveries

#### Test 2: Write 0x01 to D30D
Same process, different value

#### Test 3: Write 0x00 to D20D (Test Strike) ⭐ **Most Likely**
This characteristic is **writable** and named "test strike" - prime candidate!

#### Test 4: Write 0x01 to D20D
Same characteristic, different value

#### Test 5: Just Connect & Read
Maybe connecting and reading is enough to calm it?

#### Test 6: Write 0xFF to D30D
Try a reset/clear value

#### Test 7: Write 0x00 to FAD2
This service characteristic changes value (0 or 1), might be control

#### Test 8: Empty Write to D20D
Some BLE devices need empty writes as ACKs

### Step 4: Identify Success

**Success looks like:**
- ✅ After pressing a test button
- ✅ "Trap Is Broadcasting" sensor turns **OFF**
- ✅ Logs stop showing "GN" device discoveries
- ✅ Trap stays quiet until next activation

**Failure looks like:**
- ❌ "Trap Is Broadcasting" stays **ON**
- ❌ Logs continue showing "GN" device
- ❌ Try next test

## What If None Work?

### Try Combinations

Some devices need multiple writes:

**Edit the YAML and add a custom test:**
```yaml
button:
  - platform: template
    name: "Test 9: Read Then Write"
    on_press:
      - logger.log: "TEST 9: Connecting..."
      - ble_client.connect: test_trap
      - delay: 2s
      # Maybe we need to read first?
      - logger.log: "TEST 9: Reading D30D first..."
      - delay: 1s
      - logger.log: "TEST 9: Now writing 0x00 to D20D..."
      - ble_client.ble_write:
          id: test_trap
          service_uuid: '0000D00D-1212-EFDE-1523-785FEF13D123'
          characteristic_uuid: '0000D20D-1212-EFDE-1523-785FEF13D123'
          value: [0x00]
      - delay: 1s
      - ble_client.disconnect: test_trap
```

### Try Different Timing

Maybe the device needs the connection held longer:
```yaml
- delay: 5s  # Instead of 2s
```

### Try Multi-byte Values

```yaml
value: [0x00, 0x00]  # Two bytes
value: [0x01, 0x00]  # Little endian 1
```

## Advanced: If You Have nRF Connect App

This is actually **easier** than ESPHome for discovery:

1. **Trigger your trap** (activate it manually)
2. **Open nRF Connect** on your phone
3. **Scan** and find device "GN"
4. **Connect** (no pairing needed!)
5. **Navigate** to service `0xD00D`
6. **Tap** on characteristic `0xD20D`
7. **Try writing:**
   - Single arrow up: Write `0x00`
   - Try `01`, `FF`, etc.
8. **Disconnect**
9. **Re-scan** - Does "GN" disappear?

This is faster than flashing ESPHome repeatedly!

## Logging What Works

When you find the working command, **document it:**

```
WORKING CALM COMMAND:
- Service: 0000D00D-1212-EFDE-1523-785FEF13D123
- Characteristic: 0000D20D-1212-EFDE-1523-785FEF13D123
- Value: [0x00]
- Notes: Device stopped broadcasting immediately
```

## Next Steps After Discovery

### 1. Report to GitHub Issue

Share your findings so others benefit!

### 2. Update the Implementation

I'll help you add it to the C++ code:

```cpp
// In goodnature_ble_listener.cpp
void GoodnatureBleListener::calm_device() {
  // Connect, write discovered value, disconnect
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
        value: [0x??]  # Your discovered value
    - ble_client.disconnect: trap_client
```

### 3. Make It Optional

Add a switch to enable/disable auto-calming:
```yaml
switch:
  - platform: template
    name: "Auto-Calm Trap"
    id: auto_calm
    optimistic: true

sensor:
  - platform: goodnature_ble
    on_value:
      then:
        - if:
            condition:
              switch.is_on: auto_calm
            then:
              # Calm the trap
```

## Expected Results

Based on typical BLE patterns, my **prediction**:

🎯 **Most likely to work:**
- Characteristic: `0x0000D20D` (Test Strike - it's writable!)
- Value: `0x00` or `0x01`

🤔 **Second most likely:**
- Just connecting and reading might be enough
- The device knows someone acknowledged it

🔮 **Dark horse:**
- Service `0xFADE`, characteristic `0xFAD2`
- This one changes between 0 and 1, could be status/control

## Troubleshooting

### "Failed to connect"
- Move ESP32 closer to trap
- Ensure trap is broadcasting (trigger it)
- Check MAC address matches

### "Write failed"
- Characteristic might not be writable
- Try a different characteristic
- Check service UUID is correct

### Connection drops immediately
- This is normal, some devices do this
- Still check if it stopped broadcasting

### Device never stops broadcasting
- Try all 8 tests systematically
- Test with nRF Connect app
- It's possible the command is more complex

## Battery Impact Note

If we **can't** discover the calm command, it's okay!
- The implementation works fine without it
- Trap has long battery life anyway
- Official app will calm it when you check it
- Or it times out automatically

## Time Estimate

- **Quick test (8 buttons):** 15 minutes
- **Thorough testing:** 1 hour
- **nRF Connect method:** 10 minutes

Good luck! 🍀 Report back what you find!
