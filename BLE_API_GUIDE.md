# PipBoy Watch — BLE API Guide for WatchDogs Game

## Quick Start

1. User enables BLE on watch: WiFi app → BLE ON/OFF
2. Game scans for `PipBoy-*` devices
3. Game connects → watch shows 6-digit PIN → user enters in game
4. Send JSON commands via Nordic UART Service (NUS)

## BLE Details

| Field | Value |
|-------|-------|
| Device name | `PipBoy-xxxxx` (unique per device) |
| Service UUID | `6E400001-B5A3-F393-E0A9-E50E24DCCA9E` |
| RX (game→watch) | `6E400002-...` (write) |
| TX (watch→game) | `6E400003-...` (notify) |
| Pairing | 6-digit PIN displayed on watch |
| IO Capability | Display Only |
| Bonding | Yes (auto-reconnect after first pair) |
| MTU | 20 bytes (chunked, reassemble on \n) |

## Protocol

JSON lines, \n terminated. Both directions.

### Commands (game → watch)

```json
{"cmd":"status"}
{"cmd":"version"}
{"cmd":"haptic"}
{"cmd":"brightness","params":{"v":128}}
{"cmd":"gps_on"}
{"cmd":"gps_off"}
{"cmd":"recon_wifi"}
{"cmd":"recon_ble","params":{"duration":10}}
{"cmd":"recon_deauth","params":{"bssid":"AA:BB:CC:DD:EE:FF","ch":6}}
{"cmd":"recon_stop"}
{"cmd":"recon_results"}
{"cmd":"nfc_scan"}
{"cmd":"nfc_stop"}
{"cmd":"nfc_save"}
{"cmd":"nfc_list"}
{"cmd":"nfc_delete","params":{"idx":0}}
{"cmd":"nfc_download","params":{"idx":0}}
{"cmd":"nfc_export"}
{"cmd":"lora_start"}
{"cmd":"lora_stop"}
{"cmd":"lora_send","params":{"text":"hello"}}
{"cmd":"lora_advert"}
{"cmd":"lora_history"}
{"cmd":"watchface","params":{"style":"next"}}
{"cmd":"reboot"}
```

### Responses (watch → game)

```json
{"ok":true,"msg":"wifi scanning"}
{"version":"PipBoy-3000 v0.3","hw":"T-Watch Ultra ESP32-S3","features":["nfc","lora","gps","recon","compass"]}
{"bat":85,"bat_v":4.05,"charging":false,"time":"14:30:22","ntp":true,"lora":true,"nfc":false,"heap":180}
{"tags":["Tag-A1B2C3","Tag-D4E5F6"]}
{"type":"nfc_file","name":"/nfc/tag_0.nfc","data":"<base64>"}
{"messages":[{"ch":"public","text":"hello","hops":1,"rssi":-65,"ts":1711500000}]}
{"wifi":[{"ssid":"MyNet","bssid":"AA:BB:CC:DD:EE:FF","rssi":-45,"ch":6,"auth":"WPA2"}],"ble":[{"mac":"11:22:33:44:55:66","name":"AirPods","rssi":-60,"airtag":false}]}
```

### Events (watch → game, pushed asynchronously)

```json
{"event":"nfc_tag","uid":"A1:B2:C3:D4","ndef":"hello"}
{"event":"lora_msg","channel":"public","text":"PipBoy: hello","hops":1,"rssi":-65,"ts":1711500000}
```

## Pairing Flow

1. Watch: BLE OFF by default (battery saving)
2. User: enters WiFi app on watch → taps "BLE ON/OFF"
3. Watch: starts NimBLE advertising as "PipBoy-xxxxx"
4. Game: BLE scan → finds PipBoy-xxxxx
5. Game: initiates connection
6. Watch: NimBLE triggers onPassKeyDisplay()
7. Watch: displays overlay "PAIRING / BLE PIN: 847291"
8. User: reads PIN from watch screen, enters in game
9. Game: provides passkey to BlueZ/BLE stack
10. Watch: verifies PIN → encrypts connection → hides overlay
11. Bonded: future connections auto-reconnect (no PIN)

## RX Buffer Pattern (Python)

```python
rx_buffer = ""

def on_notify(sender, data):
    global rx_buffer
    rx_buffer += data.decode("utf-8", errors="replace")
    while "\n" in rx_buffer:
        line, rx_buffer = rx_buffer.split("\n", 1)
        if line.strip():
            handle_message(json.loads(line))
```

## NFC File Transfer

Response to `nfc_download` contains base64-encoded Flipper .nfc file:
```python
msg = await send_and_wait({"cmd":"nfc_download","params":{"idx":0}})
content = base64.b64decode(msg["data"]).decode("utf-8")
# content = "Filetype: Flipper NFC device\nVersion: 4\n..."
```

## Notes

- BLE and WiFi AP can run simultaneously
- Deauth temporarily stops WiFi AP (restarts after)
- Events are pushed without request (subscribe to NUS_TX notify)
- All commands are non-blocking (flag-based execution in main loop)
- Watch screen shows "IN ACTION" overlay during remote operations
