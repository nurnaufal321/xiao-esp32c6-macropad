# Seeed Studio XIAO ESP32C6 — 9-Key DIY Macropad (No Diodes Required)

A complete **Direct-Pin (Zero-Diode)** 9-Key Macropad firmware, hardware wiring guide, and **Web Serial Configurator App** for the **Seeed Studio XIAO ESP32C6**.

- **🌐 Live Web Serial Configurator:** [nurnaufal321.github.io/xiao-esp32c6-macropad](https://nurnaufal321.github.io/xiao-esp32c6-macropad/)
- **Zero Diodes / Zero External Resistors:** Each switch wires directly to its own GPIO pin (`D0`–`D8`) with internal pull-ups enabled, and all 9 switches share a single daisy-chained `GND` wire.
- **True 9-Key Rollover (NKRO):** Because we don't multiplex rows and columns without diodes, you can press any combination of keys simultaneously with zero ghosting.
- **Mac & Windows Ready:** Built-in persistent **Mac (`Cmd`) / Windows (`Ctrl`/`Win`) Mode Toggle** (hold **Key 1 + Key 9** for 1.5 seconds, or toggle from the Web App).
- **Hybrid Bluetooth LE HID + USB-C Web Serial:**
  - Works wirelessly over **Bluetooth LE** (Keyboard Shortcuts + Media Play/Pause/Prev/Next/Stop + Volume/Mute + Text Macros).
  - Simultaneously connects to the Web Serial Configurator with real-time 3×3 keypress lighting and one-click key remapping saved directly to non-volatile flash memory (`Preferences` / NVS) **without reflashing**!

---

## 🌐 Web Serial Configurator

You can configure and customize your macropad directly in Chrome, Edge, or Opera without installing any software or reflashing:

👉 **[Launch the Web Configurator](https://nurnaufal321.github.io/xiao-esp32c6-macropad/)**

- **Live 3×3 Key Visualizer:** Keys light up in real time on screen when pressed on the physical macropad.
- **One-Click Remapping:** Choose from presets (`Copy`, `Paste`, `Screenshot`, `App Windows Cmd+\``, `Play/Pause`, `Vol+`, `Vol-`, `Mute`, etc.), customize shortcut keys and modifier keys, or define custom text macros.
- **OS Layer Management:** Toggle between Mac and Windows layer configurations and save them straight to flash memory.

---

## 1. DIY Hardware Wiring Guide (No Diodes!)

Every mechanical switch (Cherry MX, Kailh, Gateron, etc.) has **2 metal pins (legs)** on the bottom. Switches are non-polarized—it does not matter which leg you treat as **Leg A** and which as **Leg B**.

### Physical 3×3 Switch Grid (Looking Down at the Keys from the Top)

```text
+-------------------+-------------------+-------------------+
|       KEY 1       |       KEY 2       |       KEY 3       |
|   Copy (Cmd/Ctrl) |  Paste (Cmd/Ctrl) |   Undo (Cmd/Ctrl) |
|                   |                   |                   |
|  Leg A -> Pin D0  |  Leg A -> Pin D1  |  Leg A -> Pin D2  |
|  Leg B -> GND Bus |  Leg B -> GND Bus |  Leg B -> GND Bus |
+-------------------+-------------------+-------------------+
|       KEY 4       |       KEY 5       |       KEY 6       |
|     Screenshot    |   Mute Mic (Zoom) |    Lock Screen    |
|                   |                   |                   |
|  Leg A -> Pin D3  |  Leg A -> Pin D4  |  Leg A -> Pin D5  |
|  Leg B -> GND Bus |  Leg B -> GND Bus |  Leg B -> GND Bus |
+-------------------+-------------------+-------------------+
|       KEY 7       |       KEY 8       |       KEY 9       |
|    Volume Down    |    Play / Pause   |     Volume Up     |
|                   |                   |                   |
|  Leg A -> Pin D6  |  Leg A -> Pin D7  |  Leg A -> Pin D8  |
|  Leg B -> GND Bus |  Leg B -> GND Bus |  Leg B -> GND Bus |
+-------------------+-------------------+-------------------+
```

### XIAO ESP32C6 Pinout Diagram (Top View, USB-C Port Facing Up)

```text
                  +------[ USB-C ]------+
   Key 1  <---    | D0               5V |
   Key 2  <---    | D1              GND |  <--- Common GND Bus (Leg B of all 9 switches)
   Key 3  <---    | D2              3V3 |
   Key 4  <---    | D3              D10 |  (Spare pin)
   Key 5  <---    | D4               D9 |  (Spare pin)
   Key 6  <---    | D5               D8 |  ---> Key 9
   Key 7  <---    | D6               D7 |  ---> Key 8
                  +---------------------+
```

### Step-by-Step Soldering Checklist
1. **Solder the Common Ground Bus (Leg B of all 9 switches):**
   - Run a single bare or stripped wire hopping from **Leg B** of `Key 1` -> `Key 2` -> `Key 3` -> `Key 6` -> `Key 5` -> `Key 4` -> `Key 7` -> `Key 8` -> `Key 9`, and solder the end of that chain to the **`GND`** pin on the XIAO ESP32C6.
2. **Solder the 9 Signal Wires (Leg A of each switch):**
   - `Key 1` (Top-Left) Leg A $\rightarrow$ **`D0`** (`GPIO 0`)
   - `Key 2` (Top-Mid) Leg A $\rightarrow$ **`D1`** (`GPIO 1`)
   - `Key 3` (Top-Right) Leg A $\rightarrow$ **`D2`** (`GPIO 2`)
   - `Key 4` (Mid-Left) Leg A $\rightarrow$ **`D3`** (`GPIO 21`)
   - `Key 5` (Center) Leg A $\rightarrow$ **`D4`** (`GPIO 22`)
   - `Key 6` (Mid-Right) Leg A $\rightarrow$ **`D5`** (`GPIO 23`)
   - `Key 7` (Bot-Left) Leg A $\rightarrow$ **`D6`** (`GPIO 16`)
   - `Key 8` (Bot-Mid) Leg A $\rightarrow$ **`D7`** (`GPIO 17`)
   - `Key 9` (Bot-Right) Leg A $\rightarrow$ **`D8`** (`GPIO 19`)

---

## 2. How to Flash the Firmware (One-Time Setup)

> [!TIP]
> **Test Before You Solder!**
> You can flash the XIAO ESP32C6 right now with just a USB-C cable before soldering a single wire. Once flashed, touch a jumper wire or paperclip between **`D0`** and **`GND`** to test `Key 1`!

### Option A: Using Arduino IDE (Recommended for Beginners)

1. **Install Arduino IDE 2.x** from [arduino.cc](https://www.arduino.cc/en/software).
2. **Add the ESP32 Board Package (v3.0+ required for ESP32-C6):**
   - Open **Settings / Preferences** $\rightarrow$ **Additional Boards Manager URLs** and paste:
     ```text
     https://espressif.github.io/arduino-esp32/package_esp32_index.json
     ```
   - Open **Tools $\rightarrow$ Board $\rightarrow$ Boards Manager**, search for **`esp32`** by *Espressif Systems*, and install **version 3.0.x or newer**.
3. **No External Libraries Required:**
   - The firmware uses the official built-in ESP32 BLE HID and Preferences libraries included with the ESP32 Core 3.x package.
4. **Configure Board Settings:**
   - **Tools $\rightarrow$ Board $\rightarrow$ esp32** $\rightarrow$ select **`XIAO_ESP32C6`**
   - **Tools $\rightarrow$ USB CDC On Boot** $\rightarrow$ select **`Enabled`** *(Crucial so USB Serial works!)*
   - **Tools $\rightarrow$ Port** $\rightarrow$ select your XIAO USB port (`cu.usbmodem...` on Mac or `COMx` on Windows).
5. **Upload:**
   - Open `firmware/xiao_c6_macropad/xiao_c6_macropad.ino` and click **Upload ($\rightarrow$)**.
   - *Troubleshooting Upload:* If the board doesn't enter upload mode automatically, hold down the tiny **`BOOT`** button on the XIAO, tap the **`RESET`** button once, then release **`BOOT`** and click Upload.

### Option B: Using PlatformIO (VS Code / CLI)

From this project directory, run:
```bash
pio run --target upload --target monitor
```

---

## 3. Using Your Macropad (Mac & Windows)

### Pairing via Bluetooth (BLE HID)
1. Plug the XIAO ESP32C6 into any USB-C power source (or your computer).
2. Open **Bluetooth Settings** on your Mac or Windows PC and pair with **`XIAO-C6 Macropad`**.
3. Press any key!

### Toggling Between Mac Mode & Windows Mode
- **Via Hardware Chord:** Hold **Key 1 (Top-Left)** + **Key 9 (Bottom-Right)** together for **1.5 seconds**.
  - **2 Yellow LED Blinks** = **Mac Mode** (`Cmd`-based shortcuts)
  - **3 Yellow LED Blinks** = **Windows Mode** (`Ctrl` / `Win`-based shortcuts)
- Your chosen OS mode is saved in flash memory (`NVS`) and remembered even after unplugging power.

### Live Reprogramming Over USB Serial (Without Reflashing!)
Open the Arduino **Serial Monitor** at **`115200` baud** (with **Newline** enabled) and try these commands:

| Command | Description |
| :--- | :--- |
| `STATUS` | Prints current OS mode (`MAC`/`WIN`), Bluetooth status, and key count |
| `GETMAP` | Dumps the entire Mac & Windows 9-key configuration as JSON |
| `MODE:MAC` | Switches to Mac keymap mode and saves to flash |
| `MODE:WIN` | Switches to Windows keymap mode and saves to flash |
| `BLE:OFF` | Mutes Bluetooth HID output (so only USB Serial `KEY:n:DOWN` events fire) |
| `BLE:ON` | Re-enables Bluetooth HID output |
| `SETKEY:<OS>:<KEY>:<TYPE>:<MOD>:<CODE>[:<TEXT>]` | Reprograms any key in flash memory immediately! |
| `RESETMAP` | Restores all 9 keys (Mac & Win) to factory defaults |

#### Example `SETKEY` Commands:
- **Bind Key 1 on Mac to `Cmd + Space` (Spotlight Search):**
  - `TYPE = 1` (Keyboard), `MOD = 8` (`MOD_LGUI`), `CODE = 0x2C` (`SPACE`)
  ```text
  SETKEY:MAC:1:1:8:0x2C
  ```
- **Bind Key 5 on Windows to type a text macro `"gg wp"`:**
  - `TYPE = 3` (Text Macro)
  ```text
  SETKEY:WIN:5:3:0:0:gg wp
  ```
- **Bind Key 7 on Mac to Mute Audio (`0x00E2`):**
  - `TYPE = 2` (Media Key), `MOD = 0`, `CODE = 0x00E2`
  ```text
  SETKEY:MAC:7:2:0:0x00E2
  ```
