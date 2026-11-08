// The MACROPAD C6 serial protocol: text lines over the XIAO ESP32C6's built-in USB-C port.
'use strict';

const ESPRESSIF_USB_VENDOR = 0x303a;

class MacropadBoard {
  constructor({ onMap, onStatus, onKeyEvent, onEvent, onLog, onClose }) {
    this.onMap = onMap;
    this.onStatus = onStatus;
    this.onKeyEvent = onKeyEvent;
    this.onEvent = onEvent;
    this.onLog = onLog;
    this.onClose = onClose;
    this.port = null;
    this.writer = null;
    this.reader = null;
  }

  get connected() {
    return !!this.port;
  }

  async open(port) {
    await port.open({ baudRate: 115200 });
    // Clear DTR/RTS so opening the port does not reset the ESP32-C6 chip.
    await port.setSignals({ dataTerminalReady: false, requestToSend: false }).catch(() => {});
    this.port = port;
    this.writer = port.writable.getWriter();
    this.readLoop();
    // Give the USB-Serial/JTAG FIFO a brief moment after opening before querying state
    await new Promise((r) => setTimeout(r, 80));
  }

  async close() {
    const port = this.port;
    this.port = null;
    try { await this.reader?.cancel(); } catch {}
    try { this.writer?.releaseLock(); } catch {}
    try { await port?.close(); } catch {}
    this.reader = this.writer = null;
  }

  async readLoop() {
    const decoder = new TextDecoder();
    let pending = '';
    try {
      this.reader = this.port.readable.getReader();
      while (true) {
        const { value, done } = await this.reader.read();
        if (done) break;
        pending += decoder.decode(value, { stream: true });
        let nl;
        while ((nl = pending.indexOf('\n')) >= 0) {
          const line = pending.slice(0, nl).trim();
          pending = pending.slice(nl + 1);
          if (line) this.onLine(line);
        }
      }
    } catch (e) {
      this.onLog(`Read stopped: ${e.message}`);
    } finally {
      try { this.reader?.releaseLock(); } catch {}
    }
    if (this.port) {
      await this.close();
      this.onClose();
    }
  }

  onLine(line) {
    // 1. Full JSON keymap dump: MAP:{"osMode":"MAC",...}
    if (line.startsWith('MAP:')) {
      try {
        const data = JSON.parse(line.slice(4));
        this.onMap?.(data);
      } catch (e) {
        this.onLog(`Failed to parse MAP JSON: ${e.message}`);
      }
      return;
    }

    // 2. Status line: STATUS:OS=MAC,BLE_ENABLED=1,BLE_CONNECTED=1,KEYS=9
    if (line.startsWith('STATUS:')) {
      const info = {};
      for (const part of line.slice(7).split(',')) {
        const [k, v] = part.split('=');
        info[k] = isNaN(Number(v)) ? v : Number(v);
      }
      this.onStatus?.(info);
      return;
    }

    // 3. Live physical key press/release: KEY:1:DOWN:MAC or KEY:1:UP:MAC
    if (line.startsWith('KEY:')) {
      const parts = line.split(':');
      const keyNum = Number(parts[1]);
      const dir = parts[2]; // 'DOWN' or 'UP'
      const os = parts[3] || '';
      this.onKeyEvent?.({ key: keyNum, pressed: dir === 'DOWN', dir, os });
      this.onLog(`Key ${keyNum} ${dir} (${os})`);
      return;
    }

    // 4. Asynchronous board events: EVENT:MODE_CHANGED:WIN, EVENT:BLE:CONNECTED, OK:...
    if (line.startsWith('EVENT:') || line.startsWith('OK:') || line.startsWith('ERR:')) {
      this.onLog(line);
      this.onEvent?.(line);
      return;
    }

    this.onLog(line);
  }

  send(line) {
    if (!this.writer) return Promise.resolve();
    return this.writer.write(new TextEncoder().encode(line + '\n'));
  }

  getStatus() {
    return this.send('STATUS');
  }

  getMap() {
    return this.send('GETMAP');
  }

  setOsMode(os) {
    const target = os.toUpperCase() === 'WIN' ? 'WIN' : 'MAC';
    return this.send(`MODE:${target}`);
  }

  setBleEnabled(enabled) {
    return this.send(enabled ? 'BLE:ON' : 'BLE:OFF');
  }

  /**
   * Sends SETKEY:<MAC|WIN>:<1-9>:<type>:<mod>:<code>[:<text>]
   */
  setKey(os, keyNum, { type, mod, code, text = '' }) {
    const osTag = os.toUpperCase() === 'WIN' ? 'WIN' : 'MAC';
    const cleanText = String(text || '').replace(/[\r\n]/g, '').slice(0, 23);
    if (Number(type) === 3) {
      return this.send(`SETKEY:${osTag}:${keyNum}:${type}:${mod}:${code}:${cleanText}`);
    }
    return this.send(`SETKEY:${osTag}:${keyNum}:${type}:${mod}:${code}`);
  }

  resetFactoryMap() {
    return this.send('RESETMAP');
  }
}
