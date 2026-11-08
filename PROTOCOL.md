# Protocol

The page and the firmware talk in text lines (ending in `\n` at `115200` baud) over the XIAO ESP32C6's built-in USB-C port, which shows up to the browser as a serial port with Espressif USB vendor ID `0x303A`.

## Page to Board

| Line | Meaning |
|---|---|
| `STATUS` | Ask for the current OS mode (`MAC` or `WIN`), BLE output state, and BLE connection status. |
| `GETMAP` | Ask for the full 9-key Mac & Windows keymap JSON stored in NVS flash. |
| `MODE:MAC` | Switch active layer to macOS (`Cmd` shortcuts) and save to NVS flash. |
| `MODE:WIN` | Switch active layer to Windows (`Ctrl`/`Win` shortcuts) and save to NVS flash. |
| `BLE:ON` | Enable Bluetooth LE HID keyboard/media output. |
| `BLE:OFF` | Mute Bluetooth LE HID output (so only USB Serial `KEY:<n>:DOWN` events fire). |
| `SETKEY:<MAC\|WIN>:<1-9>:<type>:<mod>:<code>[:<text>]` | Save a new keybinding for key `1..9` on the `MAC` or `WIN` layer directly into NVS flash. |
| `RESETMAP` | Restore all 9 keys across both Mac & Windows layers to factory defaults in NVS flash. |

## Board to Page

| Line | Meaning |
|---|---|
| `STATUS:OS=<MAC\|WIN>,BLE_ENABLED=<0\|1>,BLE_CONNECTED=<0\|1>,KEYS=9` | Sent in reply to `STATUS`. |
| `MAP:{"osMode":"MAC","bleEnabled":true,...}` | Sent in reply to `GETMAP`. Contains `mac` and `win` arrays of 9 key objects (`{key, type, mod, code, text}`). |
| `KEY:<1-9>:<DOWN\|UP>:<MAC\|WIN>` | Sent immediately whenever a physical key (`D0`–`D8`) is pressed or released. |
| `EVENT:MODE_CHANGED:<MAC\|WIN>` | Sent when the user toggles OS mode on the hardware via the 1.5s `Key 1 + Key 9` chord. |
| `EVENT:BLE:CONNECTED` / `EVENT:BLE:DISCONNECTED` | Sent when a Bluetooth host connects or disconnects. |
| `OK:<CMD>` / `ERR:<REASON>` | Confirmation or error reply to a configuration command. |
