// MACROPAD C6 Web Serial Configurator (BUSY C6 RawBlock style)
'use strict';

const $ = (s) => document.querySelector(s);
const $$ = (s) => [...document.querySelectorAll(s)];

// Modifier bitmasks (matching firmware)
const MOD_LCTRL  = 1;
const MOD_LSHIFT = 2;
const MOD_LALT   = 4;
const MOD_LGUI   = 8;

// Physical pin & grid metadata for Keys 1..9
const KEY_META = [
  { key: 1, pos: 'Top-Left',  pin: 'D0', gpio: 'GPIO0'  },
  { key: 2, pos: 'Top-Mid',   pin: 'D1', gpio: 'GPIO1'  },
  { key: 3, pos: 'Top-Right', pin: 'D2', gpio: 'GPIO2'  },
  { key: 4, pos: 'Mid-Left',  pin: 'D3', gpio: 'GPIO21' },
  { key: 5, pos: 'Center',    pin: 'D4', gpio: 'GPIO22' },
  { key: 6, pos: 'Mid-Right', pin: 'D5', gpio: 'GPIO23' },
  { key: 7, pos: 'Bot-Left',  pin: 'D6', gpio: 'GPIO16' },
  { key: 8, pos: 'Bot-Mid',   pin: 'D7', gpio: 'GPIO17' },
  { key: 9, pos: 'Bot-Right', pin: 'D8', gpio: 'GPIO19' },
];

// USB HID Keyboard Usage IDs (Page 0x07)
const HID_KEYS = [
  { code: 4,  label: 'A', eventKey: 'KeyA' },
  { code: 5,  label: 'B', eventKey: 'KeyB' },
  { code: 6,  label: 'C', eventKey: 'KeyC' },
  { code: 7,  label: 'D', eventKey: 'KeyD' },
  { code: 8,  label: 'E', eventKey: 'KeyE' },
  { code: 9,  label: 'F', eventKey: 'KeyF' },
  { code: 10, label: 'G', eventKey: 'KeyG' },
  { code: 11, label: 'H', eventKey: 'KeyH' },
  { code: 12, label: 'I', eventKey: 'KeyI' },
  { code: 13, label: 'J', eventKey: 'KeyJ' },
  { code: 14, label: 'K', eventKey: 'KeyK' },
  { code: 15, label: 'L', eventKey: 'KeyL' },
  { code: 16, label: 'M', eventKey: 'KeyM' },
  { code: 17, label: 'N', eventKey: 'KeyN' },
  { code: 18, label: 'O', eventKey: 'KeyO' },
  { code: 19, label: 'P', eventKey: 'KeyP' },
  { code: 20, label: 'Q', eventKey: 'KeyQ' },
  { code: 21, label: 'R', eventKey: 'KeyR' },
  { code: 22, label: 'S', eventKey: 'KeyS' },
  { code: 23, label: 'T', eventKey: 'KeyT' },
  { code: 24, label: 'U', eventKey: 'KeyU' },
  { code: 25, label: 'V', eventKey: 'KeyV' },
  { code: 26, label: 'W', eventKey: 'KeyW' },
  { code: 27, label: 'X', eventKey: 'KeyX' },
  { code: 28, label: 'Y', eventKey: 'KeyY' },
  { code: 29, label: 'Z', eventKey: 'KeyZ' },
  { code: 30, label: '1', eventKey: 'Digit1' },
  { code: 31, label: '2', eventKey: 'Digit2' },
  { code: 32, label: '3', eventKey: 'Digit3' },
  { code: 33, label: '4', eventKey: 'Digit4' },
  { code: 34, label: '5', eventKey: 'Digit5' },
  { code: 35, label: '6', eventKey: 'Digit6' },
  { code: 36, label: '7', eventKey: 'Digit7' },
  { code: 37, label: '8', eventKey: 'Digit8' },
  { code: 38, label: '9', eventKey: 'Digit9' },
  { code: 39, label: '0', eventKey: 'Digit0' },
  { code: 40, label: 'Enter / Return', eventKey: 'Enter' },
  { code: 41, label: 'Escape (Esc)',   eventKey: 'Escape' },
  { code: 42, label: 'Backspace',      eventKey: 'Backspace' },
  { code: 43, label: 'Tab',            eventKey: 'Tab' },
  { code: 44, label: 'Spacebar',       eventKey: 'Space' },
  { code: 45, label: '- (Minus)',      eventKey: 'Minus' },
  { code: 46, label: '= (Equal)',      eventKey: 'Equal' },
  { code: 47, label: '[ (Left Bracket)',  eventKey: 'BracketLeft' },
  { code: 48, label: '] (Right Bracket)', eventKey: 'BracketRight' },
  { code: 51, label: '; (Semicolon)',  eventKey: 'Semicolon' },
  { code: 54, label: ', (Comma)',      eventKey: 'Comma' },
  { code: 55, label: '. (Period)',     eventKey: 'Period' },
  { code: 56, label: '/ (Slash)',      eventKey: 'Slash' },
  { code: 58, label: 'F1',  eventKey: 'F1' },
  { code: 59, label: 'F2',  eventKey: 'F2' },
  { code: 60, label: 'F3',  eventKey: 'F3' },
  { code: 61, label: 'F4',  eventKey: 'F4' },
  { code: 62, label: 'F5',  eventKey: 'F5' },
  { code: 63, label: 'F6',  eventKey: 'F6' },
  { code: 64, label: 'F7',  eventKey: 'F7' },
  { code: 65, label: 'F8',  eventKey: 'F8' },
  { code: 66, label: 'F9',  eventKey: 'F9' },
  { code: 67, label: 'F10', eventKey: 'F10' },
  { code: 68, label: 'F11', eventKey: 'F11' },
  { code: 69, label: 'F12', eventKey: 'F12' },
  { code: 79, label: 'Right Arrow', eventKey: 'ArrowRight' },
  { code: 80, label: 'Left Arrow',  eventKey: 'ArrowLeft' },
  { code: 81, label: 'Down Arrow',  eventKey: 'ArrowDown' },
  { code: 82, label: 'Up Arrow',    eventKey: 'ArrowUp' },
];

const MEDIA_NAMES = {
  233: 'VOL +',
  234: 'VOL −',
  226: 'MUTE',
  205: 'PLAY / PAUSE',
  181: 'NEXT TRACK',
  182: 'PREV TRACK',
  183: 'STOP',
};

// Quick one-click presets (adapts to active OS layer)
const QUICK_PRESETS = [
  { name: 'Copy',        mac: { type: 1, mod: 8,  code: 6  }, win: { type: 1, mod: 1,  code: 6  } },
  { name: 'Paste',       mac: { type: 1, mod: 8,  code: 25 }, win: { type: 1, mod: 1,  code: 25 } },
  { name: 'Cut',         mac: { type: 1, mod: 8,  code: 27 }, win: { type: 1, mod: 1,  code: 27 } },
  { name: 'Undo',        mac: { type: 1, mod: 8,  code: 29 }, win: { type: 1, mod: 1,  code: 29 } },
  { name: 'Redo',        mac: { type: 1, mod: 10, code: 29 }, win: { type: 1, mod: 1,  code: 28 } },
  { name: 'Screenshot',  mac: { type: 1, mod: 10, code: 33 }, win: { type: 1, mod: 10, code: 22 } },
  { name: 'Mute Mic',    mac: { type: 1, mod: 10, code: 4  }, win: { type: 1, mod: 3,  code: 16 } },
  { name: 'Lock Screen', mac: { type: 1, mod: 9,  code: 20 }, win: { type: 1, mod: 8,  code: 15 } },
  { name: 'Search / Run',mac: { type: 1, mod: 8,  code: 44 }, win: { type: 1, mod: 8,  code: 21 } },
  { name: 'Vol −',       mac: { type: 2, mod: 0,  code: 234}, win: { type: 2, mod: 0,  code: 234} },
  { name: 'Play / Pause',mac: { type: 2, mod: 0,  code: 205}, win: { type: 2, mod: 0,  code: 205} },
  { name: 'Vol +',       mac: { type: 2, mod: 0,  code: 233}, win: { type: 2, mod: 0,  code: 233} },
];

// Default fallback state (matches firmware defaults so editor works offline too)
function defaultState() {
  return {
    osMode: 'MAC',
    bleEnabled: true,
    bleConnected: false,
    mac: [
      { key: 1, type: 1, mod: 8,  code: 6,   text: '' },
      { key: 2, type: 1, mod: 8,  code: 25,  text: '' },
      { key: 3, type: 1, mod: 8,  code: 29,  text: '' },
      { key: 4, type: 1, mod: 10, code: 33,  text: '' },
      { key: 5, type: 1, mod: 10, code: 4,   text: '' },
      { key: 6, type: 1, mod: 9,  code: 20,  text: '' },
      { key: 7, type: 2, mod: 0,  code: 234, text: '' },
      { key: 8, type: 2, mod: 0,  code: 205, text: '' },
      { key: 9, type: 2, mod: 0,  code: 233, text: '' },
    ],
    win: [
      { key: 1, type: 1, mod: 1,  code: 6,   text: '' },
      { key: 2, type: 1, mod: 1,  code: 25,  text: '' },
      { key: 3, type: 1, mod: 1,  code: 29,  text: '' },
      { key: 4, type: 1, mod: 10, code: 22,  text: '' },
      { key: 5, type: 1, mod: 3,  code: 16,  text: '' },
      { key: 6, type: 1, mod: 8,  code: 15,  text: '' },
      { key: 7, type: 2, mod: 0,  code: 234, text: '' },
      { key: 8, type: 2, mod: 0,  code: 205, text: '' },
      { key: 9, type: 2, mod: 0,  code: 233, text: '' },
    ],
  };
}

let state = defaultState();
let selectedKey = 1; // 1..9
let pressedKeys = new Set();
let recentKeys = new Set();
let recentTimers = {};
let recordingKey = false;

function log(msg) {
  const el = $('#log');
  el.textContent += `${new Date().toLocaleTimeString()}  ${msg}\n`;
  el.scrollTop = el.scrollHeight;
}

function setStatus(msg, kind = '') {
  const el = $('#status');
  el.textContent = msg;
  el.classList.toggle('ok', kind === 'ok');
  el.classList.toggle('err', kind === 'err');
}

// Format a keybinding into a crisp human-readable label for the 3x3 keycap
function formatBindingLabel(b, osMode) {
  if (!b || b.type === 0) return 'SERIAL ONLY';
  if (b.type === 2) return MEDIA_NAMES[b.code] || `MEDIA ${b.code}`;
  if (b.type === 3) return `"${b.text || '...'}"`;

  const mods = [];
  if (b.mod & MOD_LCTRL)  mods.push('CTRL');
  if (b.mod & MOD_LALT)   mods.push(osMode === 'MAC' ? 'OPT' : 'ALT');
  if (b.mod & MOD_LSHIFT) mods.push('SHIFT');
  if (b.mod & MOD_LGUI)   mods.push(osMode === 'MAC' ? 'CMD' : 'WIN');

  const kObj = HID_KEYS.find((k) => k.code === b.code);
  const kName = kObj ? kObj.label.split(' ')[0].toUpperCase() : `0x${Number(b.code).toString(16).toUpperCase()}`;
  return [...mods, kName].join(' + ');
}

// ---------- Board Connection ----------

const board = new MacropadBoard({
  onMap: (mapData) => {
    state = mapData;
    log(`Loaded 9-key map from board (Active OS: ${state.osMode}).`);
    setStatus(`Ready · Loaded 9-key NVS map (${state.osMode} layer active).`, 'ok');
    renderAll();
    loadKeyIntoEditor(selectedKey);
  },
  onStatus: (st) => {
    if (st.OS) state.osMode = st.OS;
    if (st.BLE_ENABLED !== undefined) state.bleEnabled = !!st.BLE_ENABLED;
    if (st.BLE_CONNECTED !== undefined) state.bleConnected = !!st.BLE_CONNECTED;
    setStatus(`Connected · OS: ${state.osMode} · BLE: ${state.bleEnabled ? 'Enabled' : 'Muted'}`, 'ok');
    renderAll();
  },
  onKeyEvent: ({ key, pressed }) => {
    if (pressed) {
      pressedKeys.add(key);
      recentKeys.add(key);
      clearTimeout(recentTimers[key]);
      selectedKey = key;
      loadKeyIntoEditor(selectedKey);
      const b = getActiveMapArray()[key - 1];
      setStatus(`Key ${key} (${KEY_META[key - 1]?.pin}) PRESSED → ${formatBindingLabel(b, state.osMode)}`, 'ok');
    } else {
      pressedKeys.delete(key);
      clearTimeout(recentTimers[key]);
      recentTimers[key] = setTimeout(() => {
        recentKeys.delete(key);
        renderGrid();
      }, 350);
    }
    renderGrid();
    $('#live-press-badge').hidden = !pressedKeys.has(selectedKey);
  },
  onEvent: async (line) => {
    if (line.startsWith('EVENT:MODE_CHANGED:')) {
      state.osMode = line.split(':')[2];
      renderAll();
      loadKeyIntoEditor(selectedKey);
      setStatus(`Switched board to ${state.osMode} mode.`, 'ok');
    } else if (line === 'EVENT:BLE:CONNECTED') {
      state.bleConnected = true;
      renderAll();
    } else if (line === 'EVENT:BLE:DISCONNECTED') {
      state.bleConnected = false;
      renderAll();
    } else if (line.startsWith('OK:SETKEY:')) {
      setStatus(`Saved Key ${selectedKey} (${state.osMode}) to board NVS flash.`, 'ok');
      await board.getMap();
    } else if (line === 'OK:RESETMAP') {
      setStatus('Restored factory default keymaps in NVS flash.', 'ok');
      await board.getMap();
    } else if (line.startsWith('OK:MODE:')) {
      state.osMode = line.split(':')[2];
      renderAll();
      loadKeyIntoEditor(selectedKey);
      setStatus(`Active OS layer set to ${state.osMode}.`, 'ok');
    } else if (line.startsWith('OK:BLE:')) {
      state.bleEnabled = line.endsWith('ON');
      renderAll();
      setStatus(`Bluetooth HID output ${state.bleEnabled ? 'enabled' : 'muted'}.`, 'ok');
    }
  },
  onLog: log,
  onClose: () => {
    log('Board disconnected.');
    pressedKeys.clear();
    renderAll();
  },
});

async function connectPort(port) {
  try {
    await board.open(port);
  } catch (e) {
    setStatus(`Couldn't open the board: ${e.message}. Close any open Serial Monitor first.`, 'err');
    log(`Open failed: ${e.message}`);
    return;
  }
  log('Connected to XIAO ESP32C6.');
  setStatus('Connected · Syncing NVS keymap…', 'ok');
  renderAll();

  // Send STATUS first so ESP32-C6 HWCDC marks USB CDC connected before GETMAP runs
  await board.getStatus();
  await new Promise((r) => setTimeout(r, 90));
  await board.getMap();
}

$('#connect').addEventListener('click', async () => {
  if (board.connected) {
    await board.close();
    log('Disconnected.');
    renderAll();
    return;
  }
  try {
    const port = await navigator.serial.requestPort({
      filters: [{ usbVendorId: ESPRESSIF_USB_VENDOR }],
    });
    await connectPort(port);
  } catch (e) {
    if (e.name !== 'NotFoundError') setStatus(e.message, 'err');
  }
});

if ('serial' in navigator) {
  const reconnect = async () => {
    if (board.connected) return;
    const [port] = await navigator.serial.getPorts();
    if (port) connectPort(port);
  };
  navigator.serial.addEventListener('connect', reconnect);
  reconnect();
} else {
  $('#noserial').hidden = false;
  $('#connect').disabled = true;
}

// ---------- UI Initialization & Rendering ----------

function initSelectors() {
  const keySelect = $('#key-code');
  keySelect.replaceChildren(
    ...HID_KEYS.map((k) => {
      const opt = document.createElement('option');
      opt.value = k.code;
      opt.textContent = k.label;
      return opt;
    })
  );

  const presetsBox = $('#presets');
  presetsBox.replaceChildren(
    ...QUICK_PRESETS.map((p) => {
      const btn = document.createElement('button');
      btn.type = 'button';
      btn.className = 'small';
      btn.textContent = p.name;
      btn.addEventListener('click', () => {
        const preset = state.osMode === 'MAC' ? p.mac : p.win;
        applyBindingToEditor({ ...preset, text: '' });
      });
      return btn;
    })
  );
}

function getActiveMapArray() {
  return state.osMode === 'MAC' ? state.mac : state.win;
}

function renderGrid() {
  const grid = $('#key-grid');
  const activeMap = getActiveMapArray();

  if (grid.children.length !== 9) {
    grid.replaceChildren(
      ...KEY_META.map((meta) => {
        const btn = document.createElement('button');
        btn.type = 'button';
        btn.className = 'keycap';
        btn.dataset.key = meta.key;
        btn.innerHTML = `
          <div class="keycap-top">
            <span class="keycap-num">KEY ${meta.key}</span>
            <span class="keycap-pin">${meta.pin}</span>
          </div>
          <div class="keycap-action"></div>
          <div class="keycap-pos">${meta.pos}</div>
        `;
        btn.addEventListener('click', () => {
          selectedKey = meta.key;
          loadKeyIntoEditor(selectedKey);
          renderGrid();
        });
        return btn;
      })
    );
  }

  [...grid.children].forEach((btn, idx) => {
    const keyNum = idx + 1;
    const meta = KEY_META[idx];
    const binding = activeMap[idx];
    const isPressed = pressedKeys.has(keyNum);
    const isRecent = recentKeys.has(keyNum);
    btn.classList.toggle('selected', keyNum === selectedKey);
    btn.classList.toggle('pressed', isPressed);
    btn.classList.toggle('just-pressed', !isPressed && isRecent);
    btn.querySelector('.keycap-action').textContent = formatBindingLabel(binding, state.osMode);
    btn.querySelector('.keycap-pos').textContent = isPressed ? '● PRESSED' : meta.pos;
  });
}

function loadKeyIntoEditor(keyNum) {
  const meta = KEY_META[keyNum - 1];
  const activeMap = getActiveMapArray();
  const binding = activeMap[keyNum - 1] || { type: 1, mod: 0, code: 4, text: '' };

  $('#editor-key-title').textContent = `KEY ${meta.key} (${meta.pos.toUpperCase()} · PIN ${meta.pin})`;
  $('#editor-key-sub').textContent = `Configuring for ${state.osMode === 'MAC' ? 'MAC (Command)' : 'WINDOWS (Ctrl / Win)'} layer`;
  $('#mod-gui-label').textContent = state.osMode === 'MAC' ? 'Cmd (GUI)' : 'Win (GUI)';
  $('#mod-alt-label').textContent = state.osMode === 'MAC' ? 'Option (Alt)' : 'Alt';

  applyBindingToEditor(binding);
}

function applyBindingToEditor(binding) {
  const type = Number(binding.type);
  $('#action-type').value = String(type);

  $('#pane-key').hidden   = type !== 1;
  $('#pane-media').hidden = type !== 2;
  $('#pane-text').hidden  = type !== 3;

  if (type === 1) {
    $('#mod-ctrl').checked  = !!(binding.mod & MOD_LCTRL);
    $('#mod-shift').checked = !!(binding.mod & MOD_LSHIFT);
    $('#mod-alt').checked   = !!(binding.mod & MOD_LALT);
    $('#mod-gui').checked   = !!(binding.mod & MOD_LGUI);
    $('#key-code').value    = String(binding.code || 4);
  } else if (type === 2) {
    $('#media-code').value  = String(binding.code || 233);
  } else if (type === 3) {
    $('#macro-text').value  = binding.text || '';
  }
}

function readEditorBinding() {
  const type = Number($('#action-type').value);
  if (type === 1) {
    let mod = 0;
    if ($('#mod-ctrl').checked)  mod |= MOD_LCTRL;
    if ($('#mod-shift').checked) mod |= MOD_LSHIFT;
    if ($('#mod-alt').checked)   mod |= MOD_LALT;
    if ($('#mod-gui').checked)   mod |= MOD_LGUI;
    const code = Number($('#key-code').value);
    return { type: 1, mod, code, text: '' };
  }
  if (type === 2) {
    return { type: 2, mod: 0, code: Number($('#media-code').value), text: '' };
  }
  if (type === 3) {
    return { type: 3, mod: 0, code: 0, text: $('#macro-text').value.trim() };
  }
  return { type: 0, mod: 0, code: 0, text: '' };
}

function renderAll() {
  const on = board.connected;
  $('#board-name').textContent = on ? 'Seeed Studio XIAO ESP32C6' : 'No board connected';

  const statusChip = $('#board-status');
  statusChip.textContent = on ? 'USB Connected' : 'Not connected';
  statusChip.classList.toggle('ok', on);

  const bleChip = $('#ble-status');
  bleChip.hidden = !on;
  if (on) {
    bleChip.textContent = !state.bleEnabled
      ? 'BLE: Muted'
      : state.bleConnected
      ? 'BLE: Paired & Active'
      : 'BLE: Advertising';
    bleChip.classList.toggle('ok', state.bleEnabled && state.bleConnected);
    bleChip.classList.toggle('warn', !state.bleEnabled);
  }

  const modeChip = $('#mode-status');
  modeChip.hidden = !on;
  modeChip.textContent = `OS: ${state.osMode}`;

  $('#connect').textContent = on ? 'Disconnect' : 'Connect';
  $('#summary').textContent = on
    ? `Connected to XIAO ESP32C6 · Active Layer: ${state.osMode}`
    : 'Connect the board to read & edit its NVS flash keymap.';

  $('#editing-meta').textContent = `Editing: ${state.osMode} layer · Press any switch on the board to select it`;

  // OS toggle buttons
  $$('#os-toggle button').forEach((b) => {
    b.classList.toggle('on', b.dataset.os === state.osMode);
    b.disabled = false; // Allow switching preview layer even offline
  });

  // BLE toggle buttons
  $$('#ble-toggle button').forEach((b) => {
    const isBleOnBtn = b.dataset.ble === '1';
    b.classList.toggle('on', isBleOnBtn === !!state.bleEnabled);
    b.disabled = !on;
  });

  $('#save-key').disabled = !on;
  $('#reset-map').disabled = !on;

  renderGrid();
}

// ---------- Event Listeners ----------

$('#action-type').addEventListener('change', () => {
  const current = readEditorBinding();
  applyBindingToEditor(current);
});

$$('#os-toggle button').forEach((btn) => {
  btn.addEventListener('click', async () => {
    const targetOs = btn.dataset.os;
    state.osMode = targetOs;
    renderAll();
    loadKeyIntoEditor(selectedKey);
    if (board.connected) {
      await board.setOsMode(targetOs);
    }
  });
});

$$('#ble-toggle button').forEach((btn) => {
  btn.addEventListener('click', async () => {
    const enable = btn.dataset.ble === '1';
    state.bleEnabled = enable;
    renderAll();
    if (board.connected) {
      await board.setBleEnabled(enable);
    }
  });
});

$('#save-key').addEventListener('click', async () => {
  const binding = readEditorBinding();
  const activeMap = getActiveMapArray();
  activeMap[selectedKey - 1] = { key: selectedKey, ...binding };
  renderGrid();

  if (board.connected) {
    setStatus(`Saving Key ${selectedKey} (${state.osMode}) to board…`);
    await board.setKey(state.osMode, selectedKey, binding);
  }
});

$('#reset-map').addEventListener('click', async () => {
  if (!board.connected) return;
  if (!confirm('Reset all 9 keys (both Mac and Windows layers) to factory defaults?')) return;
  await board.resetFactoryMap();
});

// Keystroke Recorder button
$('#record-btn').addEventListener('click', () => {
  recordingKey = !recordingKey;
  const btn = $('#record-btn');
  btn.textContent = recordingKey ? 'Press any key combo…' : 'Record Keystroke';
  btn.classList.toggle('on', recordingKey);
});

window.addEventListener('keydown', (e) => {
  if (!recordingKey) return;
  e.preventDefault();
  e.stopPropagation();

  // Ignore standalone modifier keydowns while waiting for the main key
  if (['Meta', 'Control', 'Shift', 'Alt'].includes(e.key)) return;

  const found = HID_KEYS.find((k) => k.eventKey === e.code);
  if (found) {
    $('#mod-ctrl').checked  = e.ctrlKey;
    $('#mod-shift').checked = e.shiftKey;
    $('#mod-alt').checked   = e.altKey;
    $('#mod-gui').checked   = e.metaKey;
    $('#key-code').value    = String(found.code);
    setStatus(`Recorded: ${formatBindingLabel(readEditorBinding(), state.osMode)}`, 'ok');
  } else {
    setStatus(`Key "${e.code}" not in standard list; pick from dropdown.`, 'err');
  }

  recordingKey = false;
  $('#record-btn').textContent = 'Record Keystroke';
  $('#record-btn').classList.remove('on');
});

// Export / Import JSON Profile
$('#export-json').addEventListener('click', () => {
  const blob = new Blob([JSON.stringify({ mac: state.mac, win: state.win }, null, 2)], {
    type: 'application/json',
  });
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'xiao-c6-macropad-keymap.json';
  a.click();
  URL.revokeObjectURL(url);
  log('Exported keymap JSON.');
});

$('#import-json-btn').addEventListener('click', () => $('#import-json-file').click());

$('#import-json-file').addEventListener('change', async (e) => {
  const file = e.target.files?.[0];
  if (!file) return;
  try {
    const text = await file.text();
    const data = JSON.parse(text);
    if (!Array.isArray(data.mac) || !Array.isArray(data.win) || data.mac.length !== 9 || data.win.length !== 9) {
      throw new Error('JSON must contain "mac" and "win" arrays of 9 keys each.');
    }
    state.mac = data.mac;
    state.win = data.win;
    renderAll();
    loadKeyIntoEditor(selectedKey);

    if (board.connected) {
      setStatus('Pushing imported Mac & Windows keymaps to board NVS…');
      for (let i = 0; i < 9; i++) {
        await board.setKey('MAC', i + 1, state.mac[i]);
        await new Promise((r) => setTimeout(r, 35));
        await board.setKey('WIN', i + 1, state.win[i]);
        await new Promise((r) => setTimeout(r, 35));
      }
      await board.getMap();
      setStatus('Imported and saved all 18 bindings (Mac + Win) to board!', 'ok');
    } else {
      setStatus('Loaded JSON into preview. Connect board to flash it.', 'ok');
    }
  } catch (err) {
    setStatus(`Import failed: ${err.message}`, 'err');
  } finally {
    e.target.value = '';
  }
});

initSelectors();
renderAll();
loadKeyIntoEditor(selectedKey);
