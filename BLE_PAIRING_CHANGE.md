# BLE Pairing Change - Firmware → Game Integration Notes

**Date:** 2026-04-20
**Breaking:** YES - game connection flow must change
**Firmware commit:** (after 8531fca)

## What changed

NUS characteristics on PipBoy now require **MITM-authenticated encryption**
(PIN-based pairing). Previously they were open - any BLE central could connect
and send commands without pairing.

### Before

```cpp
pTxChar = createCharacteristic(NUS_TX_UUID, NIMBLE_PROPERTY::NOTIFY);
pRxChar = createCharacteristic(NUS_RX_UUID, WRITE | WRITE_NR);
```

Connection flow:
1. Game scans and finds `PipBoy-xxxxx`
2. `BleakClient.connect()` → connects without pairing
3. Game subscribes to TX notify and writes RX directly
4. `onPassKeyDisplay()` **never called** → PIN never shown on watch
5. Commands work without auth (security hole)

### Now

```cpp
pTxChar = createCharacteristic(NUS_TX_UUID,
    NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_AUTHEN);
pRxChar = createCharacteristic(NUS_RX_UUID,
    WRITE | WRITE_NR | NIMBLE_PROPERTY::WRITE_AUTHEN);
```

Connection flow:
1. Game scans and finds `PipBoy-xxxxx`
2. `BleakClient.connect()` → connects (plain LE link)
3. Game subscribes to TX notify or writes RX →
   **central receives Insufficient Authentication (0x05)**
4. OS triggers pairing request
5. Watch fires `onPassKeyDisplay()` → `PAIRING` overlay with 6-digit PIN
6. User reads PIN from watch screen, enters it on uConsole
7. `onAuthenticationComplete` fires with `isEncrypted()=true`
8. Subscribe/write retry succeeds → `WATCH_DOGS` skull overlay shown
9. Bond is stored in NVS — next connection auto-authenticates (no PIN prompt)

## What the game must implement (bleak on Linux/uConsole)

Bleak on Linux uses BlueZ. Pairing has to go through **BlueZ Agent API** because
bleak itself has no PIN input. Two options:

### Option A: Pre-pair via `bluetoothctl` (simplest)

User-level workflow on uConsole:

```bash
# Once per watch, before running game:
bluetoothctl
> agent KeyboardDisplay
> default-agent
> scan on           # wait for PipBoy-xxxxx
> pair AA:BB:CC:DD:EE:FF
  [watch shows PIN]
> [enter 6-digit PIN when prompted]
> trust AA:BB:CC:DD:EE:FF
> quit
```

Then game just uses `BleakClient(address).connect()` - already bonded, no
PIN prompt on subsequent connects. This is the path of least resistance.

### Option B: In-game BLE pairing agent

Register a BlueZ agent from Python so PIN prompt appears inside the game UI:

```python
import dbus, dbus.service, dbus.mainloop.glib
from gi.repository import GLib

AGENT_PATH = "/org/bluez/pipboy_agent"

class PipBoyAgent(dbus.service.Object):
    def __init__(self, bus, pin_getter):
        self.pin_getter = pin_getter   # callback -> str (6 digits)
        super().__init__(bus, AGENT_PATH)

    @dbus.service.method("org.bluez.Agent1", in_signature="o",
                         out_signature="s")
    def RequestPinCode(self, device):
        # Called when watch requests legacy PIN pairing
        return self.pin_getter()

    @dbus.service.method("org.bluez.Agent1", in_signature="o",
                         out_signature="u")
    def RequestPasskey(self, device):
        # Called when watch requests SSP numeric comparison (our case)
        return int(self.pin_getter())

    @dbus.service.method("org.bluez.Agent1", in_signature="ouq")
    def DisplayPasskey(self, device, passkey, entered):
        pass

    @dbus.service.method("org.bluez.Agent1", in_signature="os")
    def DisplayPinCode(self, device, pincode):
        pass

    @dbus.service.method("org.bluez.Agent1", in_signature="ou")
    def RequestConfirmation(self, device, passkey):
        return  # auto-confirm

    @dbus.service.method("org.bluez.Agent1")
    def Cancel(self):
        pass

# During game start:
dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
bus = dbus.SystemBus()
agent = PipBoyAgent(bus, lambda: game.prompt_user_for_pin())
mgr = dbus.Interface(bus.get_object("org.bluez", "/org/bluez"),
                     "org.bluez.AgentManager1")
mgr.RegisterAgent(AGENT_PATH, "KeyboardDisplay")
mgr.RequestDefaultAgent(AGENT_PATH)
```

Then `BleakClient.connect()` triggers `RequestPasskey` - game shows modal
"Enter PIN from watch screen", user types it, returns int → bond created.

### Which authentication method the firmware uses

Firmware sets:
```
SecurityAuth = BOND | MITM | SC     (Secure Connections + MITM)
SecurityIOCap = DISPLAY_ONLY         (watch shows PIN, doesn't receive)
```

Central (uConsole) must advertise `KeyboardDisplay` or `KeyboardOnly` IO
capability so BlueZ will prompt for a passkey. With `NoInputNoOutput` the
stack falls back to Just Works which our MITM requirement will refuse.

## First-connect vs subsequent connects

- **First connection** (no bond stored on either side):
  watch displays PIN, uConsole prompts for it, user reads and types
- **Subsequent connections** (both sides have bond keys in NVS / `/var/lib/bluetooth`):
  fully silent - pair phase skipped, characters accessible immediately,
  `WATCH_DOGS` overlay appears right after connect

If watch is reflashed with erased NVS (`pio run -t erase`) or user manually
removes the bond on uConsole (`bluetoothctl remove AA:BB:CC:DD:EE:FF`) the
pairing flow runs again on next connect.

## What did NOT change

- UUIDs (`6E400001-B5A3-F393-E0A9-E50E24DCCA9E` etc) same
- Advertisement format same (name + NUS UUID in scan response)
- Unified JSON command API same (`{"cmd":"status"}` → `{"type":"status",...}` etc)
- Chunked 20-byte framing on TX (watch → game), newline-terminated
- Auto-push events (`{"event":"scan_done",...}`, `{"event":"lora_msg",...}`,
  `{"event":"nfc_tag",...}`, `{"event":"deauth_detected",...}`)
- Heartbeat watchdog: game should write something at least every 60s to keep
  the link from disconnecting on the watch's end

## Suggested test order for the game agent

1. `bluetoothctl remove` the watch, flash firmware fresh, reboot watch
2. Enable BLE on watch via WiFi app → watch shows name in serial:
   `[BLE] Started: PipBoy-mb7a3 PIN: 123456`
3. From game, scan/connect → expect pairing prompt, read PIN from watch,
   enter it
4. Confirm `[BLE] Auth OK - encrypted` in watch serial
5. Confirm watch shows `WATCH_DOGS` skull overlay with dimmed brightness
6. Send `{"cmd":"status"}` → receive `{"type":"status",...}`
7. Disconnect, reconnect → should NOT prompt for PIN (bonded), overlay appears
   again instantly

## Fallback plan (if pairing is too painful to integrate now)

Revert the authentication requirement to only `READ_ENC | WRITE_ENC` (bonded
encryption, Just Works allowed) by changing `READ_AUTHEN`/`WRITE_AUTHEN` back
in `src/hal/ble_uart_service.cpp`. Loses MITM protection but keeps the bond;
BlueZ can pair without a PIN prompt in that mode.
