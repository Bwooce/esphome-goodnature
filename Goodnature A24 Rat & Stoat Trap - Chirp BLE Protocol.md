## From codyc1515@ at https://gist.github.com/codyc1515/a6b93850ad81db06cde5e76244cf96f5 as of 2024-10-08

# Introduction
Goodnature is a New Zealand based manufacturer of humane traps. Their model A24 trap targets Rat & Stoats. The device is triggered when a pest brushes past an activation pin. This causes the striker to activate instantly using the power of a small, replacable CO2 gas canister.

The device is also available with an optional visual counter or a Bluetooth Low Energy (BLE) enabled device and companion app, named Chirp.  This gist aims to document some of the more technical details of the Goodnature A24 Chirp device (such as the BLE Services and Characteristics) which are not immediately clear from the support website.

# How it works
* The app requires "Always"-On location permission to detect when the user is near the trap. When near the trap, the app will then listen for BLE beacons from the Chirp.
* Only one device using the app can connect to the Chirp at any time. When the device disconnects from the Chirp, it goes back to "sleep" again and must be "woken".
* Because the Chirp does not have Wi-Fi or Cellular, this can make notifications unreliable as you need to be within proximity of the Chirp (Bluetooth range) to receive them.

## Hardware
The Chirp device seems to have an unregistered MAC Address prefix of D7:AC:E9. (or D3:58:7F)

The Bluetooth chip in the Chirp appears to be from Nordic, as BLE services for Nordic firmware updates are present.

# Triggering the Chirp
There appears to be a 23 second cooldown between activations (persumably to save battery life).

## Automatically - when the strike activates and App is in-range
Pest enters the A24 chamber and brushes past the activation pin:

1. A24 strike activates
2. Chirp is shaken by the strike activation
3. Chirp transmits BLE advertisement with name "GN"
4. App listens for BLE advertisements from Chirp
5. App receives BLE advertisement from Chirp
6. App interrogates Chirp BLE Services for kill information

## Periodically - when the strike previously activated but the App was out of range
I suspect that the Chirp transmits periodically following a strike as a yellow light flashes, but have not seen evidence of the BLE activating afterwards.

From checking, it appears that the slide to refresh in the app does not do anything, as the Chirp does not appear to have a way to "wake" manually as it is a "sleepy" device.

## Manually - by hand
Tilt the Chirp device so it is facing upside down:

1. A red light begins to flash.
2. If the red light does not show, shake it.
3. If the red light still does not show, wait 30 seconds & shake again.

# Once Chirp has activated
Chirp transmits BLE advertisement with name "GN":

1. App listens for BLE advertisements from Chirp
2. App receives BLE advertisement from Chirp
3. App interrogates Chirp BLE Services for kill information

## BLE Services & Characteristics
There are quite a number of Services & Characteristics, however I have only managed to work out a few of these.

### 0000D00D-1212-EFDE-1523-785FEF13D123
Kill information (Service)
#### 0000D20D-1212-EFDE-1523-785FEF13D123
Unknown - usually empty
May be test strike related, but is writable.
#### 0000D30D-1212-EFDE-1523-785FEF13D123
Kill information (Characteristic) - string

##### String format
AAbbbbbbbbCdddEEEEEEEfGG

* A	Unknown value (61 77 92 b3) - changes
* b	Serial number (XXYYZZ00) - values are reversed from right to left in groups of two, i.e. a serial number of ABCDEF00 would show as 00EFCDAB
* C	Unknown value (1) - static
* d	Unknown value (a87 bb4 cd1 c1f) - changes
* E	Unknown value (bda3010) - static
* f	Kill count (1 3 4 6) - does not reset in BLE when reset in the app, i.e. you need to deduct from the kill count when it is reset. I am not sure what happens when the kill count goes over 10, as I have not activated it this many times. It is possible that the value moves left or right.
* G	Unknown value (00) - static (may be used for Kill count but have not confirmed this)

#### 0000D50D-1212-EFDE-1523-785FEF13D123
Unknown - usually 0
#### 0000D60D-1212-EFDE-1523-785FEF13D123
Unknown - usually empty. Seems to be serial number related.

### 0000DE11-1212-EFDE-1523-785FEF13D123
Device information service
#### 0000DE12-1212-EFDE-1523-785FEF13D123
Serial number characteristic - string
#### 0000DE13-1212-EFDE-1523-785FEF13D123
Unknown - usually 2
Seems to be Test strike related in that the value may be able to be 0 or 2. It's usually 2. Perhaps you write to value 0, then it goes back to 2 once struck? Or maybe 5?
#### 0000DE14-1212-EFDE-1523-785FEF13D123
Unknown - usually 2
#### 0000DE15-1212-EFDE-1523-785FEF13D123
Unknown - usually empty. May be related to (value) / 255 * 100
#### 0000DE16-1212-EFDE-1523-785FEF13D123
Unknown - usually 0

### 0000E770-1212-EFDE-1523-785FEF13D123
Unknown service
#### 0000E771-1212-EFDE-1523-785FEF13D123
Unknown - usually empty (writeable)
#### 0000E772-1212-EFDE-1523-785FEF13D123
Unknown - usually empty
#### 0000E773-1212-EFDE-1523-785FEF13D123
Unknown - usually empty

### 0000F1AE-1212-EFDE-1523-785FEF13D123
Time service
#### 0000F1AF-1212-EFDE-1523-785FEF13D123
Time characteristic (writable) - string in hex with Little Endian encoding

Current time in milliseconds is 1650500007224 as at 21-Apr-2022 00:13:27 UTC (source: https://currentmillis.com).
 Therefore 1650500007224 / 60000 is 27508333.4537 rounded to 27508333 which would be 6dbea301 in hex (source: https://www.save-editor.com/tools/wse_hex.html).

##### Examples

* a2bda301  27508130
* babda301  27508154
* f1bda301  27508209
* f6bda301  27508214
* 00bea301  27508224

### 0000FADE-1212-EFDE-1523-785FEF13D123
Unknown service
#### 0000FAD1-1212-EFDE-1523-785FEF13D123
Unknown - usually 234
#### 0000FAD2-1212-EFDE-1523-785FEF13D123
Unknown - usuallly 0. Seems it may have been serial number related.
