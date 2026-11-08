/**
 * ============================================================================
 * Seeed Studio XIAO ESP32C6 — 9-Key DIY Macropad Firmware
 * ============================================================================
 *
 * HARDWARE ARCHITECTURE:
 *   - Board: Seeed Studio XIAO ESP32C6 (RISC-V, BLE 5.0 + USB-C JTAG/Serial)
 *   - Wiring: Direct-Pin (1 GPIO per switch, NO DIODES REQUIRED, True 9-Key NKRO)
 *   - Switch Pins: D0 through D8 (Active LOW using internal pull-up resistors)
 *   - Common Bus: 1 leg of all 9 switches daisy-chained to GND
 *
 * CONNECTIVITY (HYBRID DUAL-MODE):
 *   1. Wireless Bluetooth LE HID (Keyboard + Media Control):
 *      Pairs natively with macOS, Windows, Linux, iOS, and Android.
 *      Uses NimBLE-Arduino (compatible with both v1.4.x and v2.x on ESP32 Core 3.x).
 *   2. Wired USB-C Serial Event & Live Config Stream (115200 baud):
 *      Emits structured events (`KEY:1:DOWN`, `KEY:1:UP`) for a companion script
 *      AND accepts live configuration commands so you can remap any key in flash
 *      memory (NVS) without ever reflashing the board!
 *
 * CROSS-PLATFORM MAC / WINDOWS MODE:
 *   - Hold Key 1 (Top-Left) + Key 9 (Bottom-Right) for 1.5 seconds to toggle
 *     between MAC mode (Cmd-based shortcuts) and WIN mode (Ctrl/Win shortcuts).
 *   - Your selected OS mode and all custom keybindings are saved in non-volatile
 *     flash memory (Preferences / NVS) and persist across power cycles.
 *
 * ============================================================================
 */

#include <Arduino.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLEHIDDevice.h>
#include <BLE2902.h>
#include <BLESecurity.h>
#include <HIDTypes.h>
#include "esp_mac.h"

// ============================================================================
// 1. HARDWARE PIN DEFINITIONS (Seeed Studio XIAO ESP32C6)
// ============================================================================

static const uint8_t NUM_KEYS = 9;

// Direct-pin mapping: Key 1..9 -> XIAO pins D0..D8
// Each switch connects between its D-pin and GND (no diodes required!)
static const uint8_t KEY_PINS[NUM_KEYS] = {
    D0, // Key 1 (Row 1, Col 1 - Top-Left)     -> GPIO 0
    D1, // Key 2 (Row 1, Col 2 - Top-Mid)      -> GPIO 1
    D2, // Key 3 (Row 1, Col 3 - Top-Right)    -> GPIO 2
    D3, // Key 4 (Row 2, Col 1 - Mid-Left)     -> GPIO 21
    D4, // Key 5 (Row 2, Col 2 - Center)       -> GPIO 22
    D5, // Key 6 (Row 2, Col 3 - Mid-Right)    -> GPIO 23
    D6, // Key 7 (Row 3, Col 1 - Bot-Left)     -> GPIO 16
    D7, // Key 8 (Row 3, Col 2 - Bot-Mid)      -> GPIO 17
    D8  // Key 9 (Row 3, Col 3 - Bot-Right)    -> GPIO 19
};

// XIAO ESP32C6 Onboard RF Antenna Switch Control Pins
// GPIO 3 enables the RF switch (Active LOW).
// GPIO 14 selects the antenna: LOW = onboard ceramic antenna, HIGH = external U.FL.
static const uint8_t PIN_RF_SWITCH_EN  = 3;
static const uint8_t PIN_RF_ANT_SELECT = 14;

// XIAO ESP32C6 Onboard User LED (Yellow LED on GPIO 15)
#ifndef LED_BUILTIN
#define LED_BUILTIN 15
#endif
static const uint8_t PIN_STATUS_LED = LED_BUILTIN;

// Debounce & Chord Timing Constants
static const uint32_t DEBOUNCE_MS          = 10;
static const uint32_t MODE_TOGGLE_HOLD_MS  = 1500; // Hold Key 1 + Key 9 for 1.5s

// ============================================================================
// 2. USB HID KEYCODES, MODIFIERS & CONSUMER MEDIA CODES
// ============================================================================

// Standard USB HID Modifier Bitmask (Byte 0 of Keyboard Report)
static const uint8_t MOD_NONE   = 0x00;
static const uint8_t MOD_LCTRL  = 0x01;
static const uint8_t MOD_LSHIFT = 0x02;
static const uint8_t MOD_LALT   = 0x04; // Option on Mac, Alt on Windows
static const uint8_t MOD_LGUI   = 0x08; // Command (Cmd) on Mac, Win key on Windows
static const uint8_t MOD_RCTRL  = 0x10;
static const uint8_t MOD_RSHIFT = 0x20;
static const uint8_t MOD_RALT   = 0x40;
static const uint8_t MOD_RGUI   = 0x80;

// Common USB HID Usage IDs (Keyboard/Keypad Page 0x07)
static const uint8_t HID_KEY_NONE       = 0x00;
static const uint8_t HID_KEY_A          = 0x04;
static const uint8_t HID_KEY_B          = 0x05;
static const uint8_t HID_KEY_C          = 0x06;
static const uint8_t HID_KEY_D          = 0x07;
static const uint8_t HID_KEY_E          = 0x08;
static const uint8_t HID_KEY_F          = 0x09;
static const uint8_t HID_KEY_G          = 0x0A;
static const uint8_t HID_KEY_H          = 0x0B;
static const uint8_t HID_KEY_I          = 0x0C;
static const uint8_t HID_KEY_J          = 0x0D;
static const uint8_t HID_KEY_K          = 0x0E;
static const uint8_t HID_KEY_L          = 0x0F;
static const uint8_t HID_KEY_M          = 0x10;
static const uint8_t HID_KEY_N          = 0x11;
static const uint8_t HID_KEY_O          = 0x12;
static const uint8_t HID_KEY_P          = 0x13;
static const uint8_t HID_KEY_Q          = 0x14;
static const uint8_t HID_KEY_R          = 0x15;
static const uint8_t HID_KEY_S          = 0x16;
static const uint8_t HID_KEY_T          = 0x17;
static const uint8_t HID_KEY_U          = 0x18;
static const uint8_t HID_KEY_V          = 0x19;
static const uint8_t HID_KEY_W          = 0x1A;
static const uint8_t HID_KEY_X          = 0x1B;
static const uint8_t HID_KEY_Y          = 0x1C;
static const uint8_t HID_KEY_Z          = 0x1D;
static const uint8_t HID_KEY_1          = 0x1E;
static const uint8_t HID_KEY_2          = 0x1F;
static const uint8_t HID_KEY_3          = 0x20;
static const uint8_t HID_KEY_4          = 0x21;
static const uint8_t HID_KEY_5          = 0x22;
static const uint8_t HID_KEY_6          = 0x23;
static const uint8_t HID_KEY_7          = 0x24;
static const uint8_t HID_KEY_8          = 0x25;
static const uint8_t HID_KEY_9          = 0x26;
static const uint8_t HID_KEY_0          = 0x27;
static const uint8_t HID_KEY_ENTER      = 0x28;
static const uint8_t HID_KEY_ESC        = 0x29;
static const uint8_t HID_KEY_BACKSPACE  = 0x2A;
static const uint8_t HID_KEY_TAB        = 0x2B;
static const uint8_t HID_KEY_SPACE      = 0x2C;
static const uint8_t HID_KEY_F1         = 0x3A;
static const uint8_t HID_KEY_F2         = 0x3B;
static const uint8_t HID_KEY_F3         = 0x3C;
static const uint8_t HID_KEY_F4         = 0x3D;
static const uint8_t HID_KEY_F5         = 0x3E;
static const uint8_t HID_KEY_F6         = 0x3F;
static const uint8_t HID_KEY_F7         = 0x40;
static const uint8_t HID_KEY_F8         = 0x41;
static const uint8_t HID_KEY_F9         = 0x42;
static const uint8_t HID_KEY_F10        = 0x43;
static const uint8_t HID_KEY_F11        = 0x44;
static const uint8_t HID_KEY_F12        = 0x45;

// Consumer Control Media Usage IDs (16-bit Consumer Page 0x0C)
static const uint16_t MEDIA_PLAY_PAUSE  = 0x00CD;
static const uint16_t MEDIA_NEXT_TRACK  = 0x00B5;
static const uint16_t MEDIA_PREV_TRACK  = 0x00B6;
static const uint16_t MEDIA_STOP        = 0x00B7;
static const uint16_t MEDIA_MUTE        = 0x00E2;
static const uint16_t MEDIA_VOL_UP      = 0x00E9;
static const uint16_t MEDIA_VOL_DOWN    = 0x00EA;

// ============================================================================
// 3. STANDARD 8-BYTE BOOT/NKRO BLE HID REPORT DESCRIPTOR (No Report ID Prefix)
// ============================================================================
// Using Report ID 0 (no 0x85 Report ID tag) guarantees an exact 8-byte report
// ([modifier, 0x00, key1..key6]) across macOS, Windows, iOS, and Android with
// zero possibility of an 8-byte vs 9-byte Report ID header mismatch.

static const uint8_t REPORT_ID_KEYBOARD = 0x00;

static const uint8_t HID_REPORT_MAP[] = {
    0x05, 0x01,       // Usage Page (Generic Desktop)
    0x09, 0x06,       // Usage (Keyboard)
    0xA1, 0x01,       // Collection (Application)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0xE0,       //   Usage Minimum (224 - Left Control)
    0x29, 0xE7,       //   Usage Maximum (231 - Right GUI)
    0x15, 0x00,       //   Logical Minimum (0)
    0x25, 0x01,       //   Logical Maximum (1)
    0x75, 0x01,       //   Report Size (1 bit)
    0x95, 0x08,       //   Report Count (8 bits)
    0x81, 0x02,       //   Input (Data, Variable, Absolute) -> Modifier byte
    0x95, 0x01,       //   Report Count (1)
    0x75, 0x08,       //   Report Size (8)
    0x81, 0x01,       //   Input (Constant) -> Reserved byte
    0x95, 0x06,       //   Report Count (6)
    0x75, 0x08,       //   Report Size (8)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255 - includes Volume/Mute 0x7F..0x81)
    0x05, 0x07,       //   Usage Page (Key Codes)
    0x19, 0x00,       //   Usage Minimum (0)
    0x2A, 0xFF, 0x00, //   Usage Maximum (255)
    0x81, 0x00,       //   Input (Data, Array) -> 6 Keycodes
    0xC0              // End Collection
};

// ============================================================================
// 4. KEYMAP DATA STRUCTURES & DEFAULT MAC / WINDOWS BINDINGS
// ============================================================================

enum ActionType : uint8_t {
    ACTION_NONE  = 0, // Do nothing over BLE (Serial event still fires)
    ACTION_KEY   = 1, // Standard Keyboard Shortcut (modifiers + HID keycode)
    ACTION_MEDIA = 2, // Consumer Media Key (16-bit usage ID, e.g. 0x00E9 Vol+)
    ACTION_TEXT  = 3  // Type a short ASCII string over BLE
};

enum OsMode : uint8_t {
    OS_MAC = 0,
    OS_WIN = 1
};

struct KeyBinding {
    uint8_t  type;       // ActionType (ACTION_KEY, ACTION_MEDIA, ACTION_TEXT, ACTION_NONE)
    uint8_t  modifiers;  // Modifier bitmask (MOD_LGUI, MOD_LCTRL, MOD_LSHIFT, MOD_LALT)
    uint16_t code;       // HID keycode (for ACTION_KEY) or 16-bit Media usage (for ACTION_MEDIA)
    char     text[24];   // Optional text macro for ACTION_TEXT
};

// Factory Default Mac Keymap (Keys 1..9)
static const KeyBinding DEFAULT_MAC_KEYMAP[NUM_KEYS] = {
    { ACTION_KEY,   MOD_LGUI,              HID_KEY_C,        "" }, // Key 1: Copy (Cmd + C)
    { ACTION_KEY,   MOD_LGUI,              HID_KEY_V,        "" }, // Key 2: Paste (Cmd + V)
    { ACTION_KEY,   MOD_LGUI,              HID_KEY_Z,        "" }, // Key 3: Undo (Cmd + Z)
    { ACTION_KEY,   MOD_LGUI | MOD_LSHIFT, HID_KEY_4,        "" }, // Key 4: Screenshot (Cmd + Shift + 4)
    { ACTION_KEY,   MOD_LGUI | MOD_LSHIFT, HID_KEY_A,        "" }, // Key 5: Mute Mic Zoom/Discord (Cmd + Shift + A)
    { ACTION_KEY,   MOD_LGUI | MOD_LCTRL,  HID_KEY_Q,        "" }, // Key 6: Lock Screen (Ctrl + Cmd + Q)
    { ACTION_MEDIA, MOD_NONE,              MEDIA_VOL_DOWN,   "" }, // Key 7: Volume Down
    { ACTION_MEDIA, MOD_NONE,              MEDIA_PLAY_PAUSE, "" }, // Key 8: Play / Pause
    { ACTION_MEDIA, MOD_NONE,              MEDIA_VOL_UP,     "" }  // Key 9: Volume Up
};

// Factory Default Windows Keymap (Keys 1..9)
static const KeyBinding DEFAULT_WIN_KEYMAP[NUM_KEYS] = {
    { ACTION_KEY,   MOD_LCTRL,              HID_KEY_C,        "" }, // Key 1: Copy (Ctrl + C)
    { ACTION_KEY,   MOD_LCTRL,              HID_KEY_V,        "" }, // Key 2: Paste (Ctrl + V)
    { ACTION_KEY,   MOD_LCTRL,              HID_KEY_Z,        "" }, // Key 3: Undo (Ctrl + Z)
    { ACTION_KEY,   MOD_LGUI  | MOD_LSHIFT, HID_KEY_S,        "" }, // Key 4: Snipping Tool (Win + Shift + S)
    { ACTION_KEY,   MOD_LCTRL | MOD_LSHIFT, HID_KEY_M,        "" }, // Key 5: Mute Mic Teams/Discord (Ctrl + Shift + M)
    { ACTION_KEY,   MOD_LGUI,               HID_KEY_L,        "" }, // Key 6: Lock Screen (Win + L)
    { ACTION_MEDIA, MOD_NONE,               MEDIA_VOL_DOWN,   "" }, // Key 7: Volume Down
    { ACTION_MEDIA, MOD_NONE,               MEDIA_PLAY_PAUSE, "" }, // Key 8: Play / Pause
    { ACTION_MEDIA, MOD_NONE,               MEDIA_VOL_UP,     "" }  // Key 9: Volume Up
};

// Active runtime keymaps (loaded from NVS flash on boot)
static KeyBinding g_keymapMac[NUM_KEYS];
static KeyBinding g_keymapWin[NUM_KEYS];
static OsMode     g_osMode     = OS_MAC;
static bool       g_bleEnabled = true; // Can be muted over Serial via BLE:OFF when using Companion App

// Switch Debounce State
struct SwitchState {
    bool     rawPressed;
    bool     stablePressed;
    uint32_t lastChangeMs;
};

static SwitchState g_switches[NUM_KEYS];
static bool        g_chordTriggered = false;
static uint32_t    g_chordStartMs   = 0;

// Preferences (NVS Flash Storage)
static Preferences g_prefs;

// ============================================================================
// 5. BLE HID SERVER STATE (Built-in ESP32 Core 3.x BLEHIDDevice)
// ============================================================================

static BLEServer*         g_pServer      = nullptr;
static BLEHIDDevice*      g_pHid         = nullptr;
static BLECharacteristic* g_pKbInputChar = nullptr;
static BLECharacteristic* g_pKbOutputChar = nullptr;
static volatile bool      g_bleConnected = false;

static void forceEnableCccdNotifications(BLECharacteristic* pChar) {
    if (pChar == nullptr) return;
    BLE2902* p2902 = (BLE2902*)pChar->getDescriptorByUUID((uint16_t)0x2902);
    if (p2902 != nullptr) {
        p2902->setNotifications(true);
    }
}

class MacropadReportCallbacks : public BLECharacteristicCallbacks {
    void onStatus(BLECharacteristic* pCharacteristic, Status s, uint32_t code) override {
        if (s != Status::SUCCESS_NOTIFY && s != Status::SUCCESS_INDICATE) {
            Serial.printf("EVENT:BLE:NOTIFY_ERR:%d:%lu\n", (int)s, (unsigned long)code);
        }
    }
};

// Backports ESP32 Core 3.3.7 HOGP fix to Core 3.1.3:
// Report Reference Descriptor (0x2908) and CCCD (0x2902) must be readable/writable
// without encryption so macOS/Windows can enumerate Report IDs before link encryption finishes.
static void fixHidReportPermissions(BLECharacteristic* pChar, BLECharacteristicCallbacks* pCb) {
    if (pChar == nullptr) return;
    pChar->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
    if (pCb != nullptr) {
        pChar->setCallbacks(pCb);
    }
    BLEDescriptor* p2908 = pChar->getDescriptorByUUID(BLEUUID((uint16_t)0x2908));
    if (p2908 != nullptr) {
        p2908->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
    }
    BLE2902* p2902 = (BLE2902*)pChar->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
    if (p2902 != nullptr) {
        p2902->setAccessPermissions(ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE);
        p2902->setNotifications(true);
    }
}

class MacropadServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) override {
        g_bleConnected = true;
        forceEnableCccdNotifications(g_pKbInputChar);
        Serial.println("EVENT:BLE:CONNECTED");
    }

    void onDisconnect(BLEServer* pServer) override {
        g_bleConnected = false;
        Serial.println("EVENT:BLE:DISCONNECTED");
        BLEDevice::startAdvertising();
    }
};

class MacropadSecurityCallbacks : public BLESecurityCallbacks {
    bool onSecurityRequest() override {
        return true;
    }

#if defined(CONFIG_BLUEDROID_ENABLED)
    void onAuthenticationComplete(esp_ble_auth_cmpl_t auth_cmpl) override {
        if (auth_cmpl.success) {
            Serial.println("EVENT:BLE:PAIRED");
        } else {
            Serial.printf("EVENT:BLE:PAIR_FAIL:%d\n", auth_cmpl.fail_reason);
        }
    }
#elif defined(CONFIG_NIMBLE_ENABLED)
    void onAuthenticationComplete(ble_gap_conn_desc* desc) override {
        if (desc->sec_state.encrypted) {
            Serial.println("EVENT:BLE:PAIRED");
        }
    }
#endif

    uint32_t onPassKeyRequest() override { return 0; }
    void onPassKeyNotify(uint32_t pass_key) override {}
    bool onConfirmPIN(uint32_t pass_key) override { return true; }
};

// ============================================================================
// 6. HELPER FUNCTIONS: LED FEEDBACK & RF ANTENNA INIT
// ============================================================================

void configureXiaoAntenna() {
    // Enable the onboard RF switch and route to the internal ceramic antenna
    pinMode(PIN_RF_SWITCH_EN, OUTPUT);
    digitalWrite(PIN_RF_SWITCH_EN, LOW);  // Active LOW: turns on RF switch

    pinMode(PIN_RF_ANT_SELECT, OUTPUT);
    digitalWrite(PIN_RF_ANT_SELECT, LOW); // LOW = Onboard Ceramic Antenna (HIGH = U.FL)
}

void blinkStatusLed(uint8_t count, uint16_t onMs = 120, uint16_t offMs = 120) {
    for (uint8_t i = 0; i < count; i++) {
        digitalWrite(PIN_STATUS_LED, LOW);  // XIAO user LED is active LOW on GPIO 15
        delay(onMs);
        digitalWrite(PIN_STATUS_LED, HIGH);
        if (i + 1 < count) {
            delay(offMs);
        }
    }
}

// ============================================================================
// 7. NVS FLASH STORAGE (Load / Save Keymaps & OS Mode)
// ============================================================================

void loadKeymapsFromNvs() {
    if (!g_prefs.begin("macropad", false)) {
        memcpy(g_keymapMac, DEFAULT_MAC_KEYMAP, sizeof(DEFAULT_MAC_KEYMAP));
        memcpy(g_keymapWin, DEFAULT_WIN_KEYMAP, sizeof(DEFAULT_WIN_KEYMAP));
        return;
    }

    g_osMode = (OsMode)g_prefs.getUChar("os_mode", (uint8_t)OS_MAC);

    size_t macLen = g_prefs.getBytes("map_mac", g_keymapMac, sizeof(g_keymapMac));
    if (macLen != sizeof(g_keymapMac)) {
        memcpy(g_keymapMac, DEFAULT_MAC_KEYMAP, sizeof(DEFAULT_MAC_KEYMAP));
        g_prefs.putBytes("map_mac", g_keymapMac, sizeof(g_keymapMac));
    }

    size_t winLen = g_prefs.getBytes("map_win", g_keymapWin, sizeof(g_keymapWin));
    if (winLen != sizeof(g_keymapWin)) {
        memcpy(g_keymapWin, DEFAULT_WIN_KEYMAP, sizeof(DEFAULT_WIN_KEYMAP));
        g_prefs.putBytes("map_win", g_keymapWin, sizeof(g_keymapWin));
    }
    g_prefs.end();
}

void saveKeymapsToNvs() {
    g_prefs.begin("macropad", false); // Read-write mode
    g_prefs.putUChar("os_mode", (uint8_t)g_osMode);
    g_prefs.putBytes("map_mac", g_keymapMac, sizeof(g_keymapMac));
    g_prefs.putBytes("map_win", g_keymapWin, sizeof(g_keymapWin));
    g_prefs.end();
}

void saveOsModeToNvs() {
    g_prefs.begin("macropad", false);
    g_prefs.putUChar("os_mode", (uint8_t)g_osMode);
    g_prefs.end();
}

void resetFactoryKeymaps() {
    memcpy(g_keymapMac, DEFAULT_MAC_KEYMAP, sizeof(DEFAULT_MAC_KEYMAP));
    memcpy(g_keymapWin, DEFAULT_WIN_KEYMAP, sizeof(DEFAULT_WIN_KEYMAP));
    saveKeymapsToNvs();
}

// ============================================================================
// 8. BLE HID INITIALIZATION & REPORT SENDING
// ============================================================================

void initBleHid() {
    BLEDevice::init("XIAO-C6 Macropad");
    BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT_NO_MITM);

    // Configure "Just Works" BLE Security with Bonding (compatible with Core 3.1.x & 3.3.x)
    BLESecurity* pSecurity = new BLESecurity();
    pSecurity->setCapability(ESP_IO_CAP_NONE);
    pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
    pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    pSecurity->setRespEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
    BLEDevice::setSecurityCallbacks(new MacropadSecurityCallbacks());

    g_pServer = BLEDevice::createServer();
    g_pServer->setCallbacks(new MacropadServerCallbacks());

    g_pHid = new BLEHIDDevice(g_pServer);
    g_pHid->manufacturer()->setValue("Seeed Studio DIY");
    g_pHid->pnp(0x02, 0x2886, 0x0046, 0x0110);
    g_pHid->hidInfo(0x00, 0x01);
    g_pHid->reportMap((uint8_t*)HID_REPORT_MAP, sizeof(HID_REPORT_MAP));

    MacropadReportCallbacks* pReportCb = new MacropadReportCallbacks();
    g_pKbInputChar = g_pHid->inputReport(REPORT_ID_KEYBOARD);

    // Unlock 0x2908 (Report Reference) & 0x2902 (CCCD) permissions before startServices()
    fixHidReportPermissions(g_pKbInputChar, pReportCb);

    // IMPORTANT: startServices() MUST be called before setBatteryLevel(100), because
    // BLECharacteristic::m_pService is only initialized inside BLEService::start()!
    g_pHid->startServices();
    g_pHid->setBatteryLevel(100);

    // Set initial 8-byte zero report AFTER startServices() (once GATT attribute handle is created)
    uint8_t zeroKb[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    g_pKbInputChar->setValue(zeroKb, sizeof(zeroKb));

    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setAppearance(HID_KEYBOARD);
    pAdvertising->addServiceUUID(g_pHid->hidService()->getUUID());
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
}

bool isBleReady() {
    return g_bleEnabled && g_bleConnected && (g_pServer != nullptr) && (g_pServer->getConnectedCount() > 0);
}

void sendBleKeyboardReport(uint8_t modifiers, uint8_t keycode) {
    if (!isBleReady() || g_pKbInputChar == nullptr) return;
    forceEnableCccdNotifications(g_pKbInputChar);
    if (modifiers != 0 && keycode != 0) {
        // macOS IOHIDEventSystem requires the modifier byte (e.g. Command) to be
        // sent first before the main keycode arrives in the next report.
        uint8_t modOnly[8] = { modifiers, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
        g_pKbInputChar->setValue(modOnly, sizeof(modOnly));
        g_pKbInputChar->notify();
        delay(12);
    }
    uint8_t report[8] = { modifiers, 0x00, keycode, 0x00, 0x00, 0x00, 0x00, 0x00 };
    g_pKbInputChar->setValue(report, sizeof(report));
    g_pKbInputChar->notify();
    Serial.printf("EVENT:BLE:REPORT_SENT:KB:%u:%u\n", modifiers, keycode);
}

void releaseBleKeyboardReport() {
    if (!isBleReady() || g_pKbInputChar == nullptr) return;
    forceEnableCccdNotifications(g_pKbInputChar);
    delay(8);
    uint8_t report[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    g_pKbInputChar->setValue(report, sizeof(report));
    g_pKbInputChar->notify();
}

void sendBleMediaReport(uint16_t mediaUsage) {
    if (mediaUsage == 0) {
        releaseBleKeyboardReport();
        return;
    }
    uint8_t hidKey = 0;
    switch (mediaUsage) {
        case MEDIA_VOL_UP:     hidKey = 0x80; break; // Keyboard Volume Up
        case MEDIA_VOL_DOWN:   hidKey = 0x81; break; // Keyboard Volume Down
        case MEDIA_MUTE:       hidKey = 0x7F; break; // Keyboard Mute
        case MEDIA_PLAY_PAUSE: hidKey = HID_KEY_F8; break;
        case MEDIA_PREV_TRACK: hidKey = HID_KEY_F7; break;
        case MEDIA_NEXT_TRACK: hidKey = HID_KEY_F9; break;
        default:               hidKey = 0x80; break;
    }
    sendBleKeyboardReport(0x00, hidKey);
    Serial.printf("EVENT:BLE:REPORT_SENT:MEDIA:%u\n", mediaUsage);
}

void releaseBleMediaReport() {
    releaseBleKeyboardReport();
}

// Helper to convert an ASCII character to a USB HID Keycode + Shift modifier
bool asciiToHid(char c, uint8_t& outMod, uint8_t& outKey) {
    outMod = 0x00;
    if (c >= 'a' && c <= 'z') { outKey = HID_KEY_A + (c - 'a'); return true; }
    if (c >= 'A' && c <= 'Z') { outMod = MOD_LSHIFT; outKey = HID_KEY_A + (c - 'A'); return true; }
    if (c >= '1' && c <= '9') { outKey = HID_KEY_1 + (c - '1'); return true; }
    if (c == '0')             { outKey = HID_KEY_0; return true; }
    if (c == ' ')             { outKey = HID_KEY_SPACE; return true; }
    if (c == '\n')            { outKey = HID_KEY_ENTER; return true; }
    return false;
}

void sendBleTextString(const char* text) {
    if (!isBleReady() || text == nullptr) return;
    for (size_t i = 0; text[i] != '\0'; i++) {
        uint8_t mod = 0, key = 0;
        if (asciiToHid(text[i], mod, key)) {
            sendBleKeyboardReport(mod, key);
            delay(12);
            releaseBleKeyboardReport();
            delay(12);
        }
    }
}

// ============================================================================
// 9. KEY PRESS / RELEASE EXECUTION
// ============================================================================

const KeyBinding& getActiveBinding(uint8_t keyIndex) {
    return (g_osMode == OS_MAC) ? g_keymapMac[keyIndex] : g_keymapWin[keyIndex];
}

void handleKeyActionDown(uint8_t keyIndex) {
    const uint8_t keyNum = keyIndex + 1;

    // 1. Always emit structured event over USB-C Serial for Companion App / Debug
    Serial.printf("KEY:%u:DOWN:%s\n", keyNum, (g_osMode == OS_MAC) ? "MAC" : "WIN");

    // 2. Brief visual flash on onboard LED (active LOW)
    digitalWrite(PIN_STATUS_LED, LOW);

    // 3. Fire BLE HID action if Bluetooth is connected and enabled
    const KeyBinding& binding = getActiveBinding(keyIndex);
    switch (binding.type) {
        case ACTION_KEY:
            sendBleKeyboardReport(binding.modifiers, (uint8_t)(binding.code & 0xFF));
            break;
        case ACTION_MEDIA:
            sendBleMediaReport(binding.code);
            break;
        case ACTION_TEXT:
            sendBleTextString(binding.text);
            break;
        case ACTION_NONE:
        default:
            break;
    }
}

void handleKeyActionUp(uint8_t keyIndex) {
    const uint8_t keyNum = keyIndex + 1;

    // 1. Emit structured release event over USB-C Serial
    Serial.printf("KEY:%u:UP:%s\n", keyNum, (g_osMode == OS_MAC) ? "MAC" : "WIN");

    // 2. Turn off onboard LED
    digitalWrite(PIN_STATUS_LED, HIGH);

    // 3. Release BLE HID report
    const KeyBinding& binding = getActiveBinding(keyIndex);
    switch (binding.type) {
        case ACTION_KEY:
            releaseBleKeyboardReport();
            break;
        case ACTION_MEDIA:
            releaseBleMediaReport();
            break;
        default:
            break;
    }
}

// ============================================================================
// 10. USB SERIAL LIVE CONFIGURATION PROTOCOL (Flash Once, Configure via App!)
// ============================================================================
// Supported Serial Commands (send with newline '\n' at 115200 baud):
//   STATUS                                    -> Prints current OS mode, BLE status, firmware version
//   GETMAP                                    -> Prints JSON of all 9 Mac & Win keybindings
//   MODE:MAC  or  MODE:WIN                    -> Switches OS mode & saves to NVS flash
//   BLE:ON    or  BLE:OFF                     -> Enables/Mutes BLE HID output (useful when wired)
//   SETKEY:<MAC|WIN>:<1-9>:<type>:<mod>:<code>[:<text>] -> Updates a key in RAM & saves to NVS
//   RESETMAP                                  -> Restores factory default keymaps in NVS
// ============================================================================

void printKeymapJson() {
    String out = "MAP:{";
    out += "\"osMode\":\"";
    out += (g_osMode == OS_MAC) ? "MAC" : "WIN";
    out += "\",\"bleEnabled\":";
    out += g_bleEnabled ? "true" : "false";
    out += ",\"bleConnected\":";
    out += g_bleConnected ? "true" : "false";
    out += ",";

    const char* osLabels[2] = { "mac", "win" };
    const KeyBinding* maps[2] = { g_keymapMac, g_keymapWin };

    for (int m = 0; m < 2; m++) {
        out += "\"";
        out += osLabels[m];
        out += "\":[";
        for (uint8_t k = 0; k < NUM_KEYS; k++) {
            const KeyBinding& b = maps[m][k];
            char item[96];
            snprintf(
                item, sizeof(item),
                "{\"key\":%u,\"type\":%u,\"mod\":%u,\"code\":%u,\"text\":\"%s\"}%s",
                k + 1, b.type, b.modifiers, b.code, b.text,
                (k + 1 < NUM_KEYS) ? "," : ""
            );
            out += item;
        }
        out += (m == 0) ? "]," : "]";
    }
    out += "}\n";
    Serial.print(out);
}

void processSerialCommand(String line) {
    line.trim();
    if (line.length() == 0) return;

    if (line.equalsIgnoreCase("STATUS")) {
        Serial.printf(
            "STATUS:OS=%s,BLE_ENABLED=%u,BLE_CONNECTED=%u,KEYS=%u\n",
            (g_osMode == OS_MAC) ? "MAC" : "WIN",
            g_bleEnabled ? 1 : 0,
            g_bleConnected ? 1 : 0,
            NUM_KEYS
        );
        return;
    }

    if (line.equalsIgnoreCase("GETMAP")) {
        printKeymapJson();
        return;
    }

    if (line.equalsIgnoreCase("MODE:MAC")) {
        g_osMode = OS_MAC;
        saveOsModeToNvs();
        Serial.println("OK:MODE:MAC");
        blinkStatusLed(2);
        return;
    }

    if (line.equalsIgnoreCase("MODE:WIN")) {
        g_osMode = OS_WIN;
        saveOsModeToNvs();
        Serial.println("OK:MODE:WIN");
        blinkStatusLed(3);
        return;
    }

    if (line.equalsIgnoreCase("BLE:ON")) {
        g_bleEnabled = true;
        Serial.println("OK:BLE:ON");
        return;
    }

    if (line.equalsIgnoreCase("BLE:OFF") || line.equalsIgnoreCase("BLE:MUTE")) {
        g_bleEnabled = false;
        releaseBleKeyboardReport();
        releaseBleMediaReport();
        Serial.println("OK:BLE:OFF");
        return;
    }

    if (line.equalsIgnoreCase("RESETMAP")) {
        resetFactoryKeymaps();
        Serial.println("OK:RESETMAP");
        blinkStatusLed(4, 60, 60);
        return;
    }

    // Format: SETKEY:<MAC|WIN>:<1-9>:<type>:<mod>:<code>[:<text>]
    if (line.startsWith("SETKEY:")) {
        // Split tokens by ':'
        int idx1 = line.indexOf(':');
        int idx2 = line.indexOf(':', idx1 + 1);
        int idx3 = line.indexOf(':', idx2 + 1);
        int idx4 = line.indexOf(':', idx3 + 1);
        int idx5 = line.indexOf(':', idx4 + 1);

        if (idx1 < 0 || idx2 < 0 || idx3 < 0 || idx4 < 0 || idx5 < 0) {
            Serial.println("ERR:SETKEY_FORMAT");
            return;
        }

        String osStr   = line.substring(idx1 + 1, idx2);
        int keyNum     = line.substring(idx2 + 1, idx3).toInt();
        int typeVal    = line.substring(idx3 + 1, idx4).toInt();
        int modVal     = line.substring(idx4 + 1, idx5).toInt();

        int idx6       = line.indexOf(':', idx5 + 1);
        int codeVal    = 0;
        String textStr = "";
        if (idx6 >= 0) {
            codeVal = (int)strtol(line.substring(idx5 + 1, idx6).c_str(), nullptr, 0);
            textStr = line.substring(idx6 + 1);
        } else {
            codeVal = (int)strtol(line.substring(idx5 + 1).c_str(), nullptr, 0);
        }

        if (keyNum < 1 || keyNum > NUM_KEYS) {
            Serial.println("ERR:KEY_RANGE");
            return;
        }

        KeyBinding* targetMap = osStr.equalsIgnoreCase("WIN") ? g_keymapWin : g_keymapMac;
        KeyBinding& b = targetMap[keyNum - 1];
        b.type      = (uint8_t)typeVal;
        b.modifiers = (uint8_t)modVal;
        b.code      = (uint16_t)codeVal;
        memset(b.text, 0, sizeof(b.text));
        textStr.toCharArray(b.text, sizeof(b.text));

        saveKeymapsToNvs();
        Serial.printf("OK:SETKEY:%s:%d\n", osStr.c_str(), keyNum);
        blinkStatusLed(1, 50, 50);
        return;
    }

    Serial.printf("ERR:UNKNOWN_CMD:%s\n", line.c_str());
}

void pollSerialCommands() {
    static String rxBuffer = "";
    while (Serial.available() > 0) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            if (rxBuffer.length() > 0) {
                processSerialCommand(rxBuffer);
                rxBuffer = "";
            }
        } else if (rxBuffer.length() < 256) {
            rxBuffer += c;
        }
    }
}

// ============================================================================
// 11. SWITCH SCANNING, DEBOUNCING & OS MODE CHORD DETECTION
// ============================================================================

void toggleOsModeViaChord() {
    releaseBleKeyboardReport();
    releaseBleMediaReport();

    g_osMode = (g_osMode == OS_MAC) ? OS_WIN : OS_MAC;
    saveOsModeToNvs();

    Serial.printf("EVENT:MODE_CHANGED:%s\n", (g_osMode == OS_MAC) ? "MAC" : "WIN");

    // 2 blinks = MAC Mode, 3 blinks = WIN Mode
    blinkStatusLed((g_osMode == OS_MAC) ? 2 : 3, 150, 150);
}

void scanDirectSwitches() {
    const uint32_t now = millis();

    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        // Active LOW: switch pressed when pin reads LOW (grounded)
        const bool pressedNow = (digitalRead(KEY_PINS[i]) == LOW);

        if (pressedNow != g_switches[i].rawPressed) {
            g_switches[i].rawPressed   = pressedNow;
            g_switches[i].lastChangeMs = now;
        }

        if ((now - g_switches[i].lastChangeMs) >= DEBOUNCE_MS) {
            if (pressedNow != g_switches[i].stablePressed) {
                g_switches[i].stablePressed = pressedNow;
                if (pressedNow) {
                    handleKeyActionDown(i);
                } else {
                    handleKeyActionUp(i);
                }
            }
        }
    }

    // Check if Key 1 (index 0) + Key 9 (index 8) are held together for 1.5s to switch Mac/Win mode
    const bool chordHeld = g_switches[0].stablePressed && g_switches[8].stablePressed;
    if (chordHeld) {
        if (g_chordStartMs == 0) {
            g_chordStartMs   = now;
            g_chordTriggered = false;
        } else if (!g_chordTriggered && (now - g_chordStartMs >= MODE_TOGGLE_HOLD_MS)) {
            g_chordTriggered = true;
            toggleOsModeViaChord();
        }
    } else {
        g_chordStartMs   = 0;
        g_chordTriggered = false;
    }
}

// ============================================================================
// 12. ARDUINO SETUP & MAIN LOOP
// ============================================================================

void setup() {
    // 1. Configure onboard ceramic RF antenna switch for reliable Bluetooth range
    configureXiaoAntenna();

    // 2. Configure status LED (active LOW on GPIO 15)
    pinMode(PIN_STATUS_LED, OUTPUT);
    digitalWrite(PIN_STATUS_LED, HIGH); // Off initially

    // 3. Configure all 9 switch pins (D0..D8) with internal pull-up resistors
    for (uint8_t i = 0; i < NUM_KEYS; i++) {
        pinMode(KEY_PINS[i], INPUT_PULLUP);
        g_switches[i] = { false, false, 0 };
    }

    // 4. Start USB-C Serial (CDC/JTAG) with 2KB TX/RX buffers so JSON maps never truncate
    Serial.setRxBufferSize(1024);
    Serial.setTxBufferSize(2048);
    Serial.begin(115200);

    // 5. Load saved Mac & Windows keymaps + OS mode from NVS flash
    loadKeymapsFromNvs();

    // 6. Initialize NimBLE HID Server (Keyboard + Media Control)
    initBleHid();

    // 7. Startup LED indication: 2 blinks = Mac Mode, 3 blinks = Windows Mode
    blinkStatusLed((g_osMode == OS_MAC) ? 2 : 3, 100, 100);

    Serial.println("READY:XIAO_ESP32C6_MACROPAD");
}

void loop() {
    scanDirectSwitches();
    pollSerialCommands();
    delay(1); // ~1000 Hz scan rate with low CPU power
}
