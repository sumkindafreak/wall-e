#pragma once

#include <Arduino.h>

// ============================================================
//  WALL-E Web UI — Drive + Connections tabs
//  Self-contained HTML/CSS/JS. No external dependencies.
// ============================================================

const char WALLE_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
  <title>WALL-E Drive</title>
  <style>
    *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

    :root {
      --bg:        #0d0f14;
      --surface:   #13151e;
      --surface2:  #1a1d28;
      --border:    #252836;
      --border2:   #2f3347;
      --accent:    #f5a623;
      --accent2:   #ffc55a;
      --btn:       #1e2130;
      --btn-h:     #252a40;
      --btn-act:   #f5a623;
      --txt:       #dde1f0;
      --txt-dim:   #555e80;
      --txt-mid:   #8890aa;
      --stop:      #e63946;
      --stop-h:    #ff4d5a;
      --ok:        #3ddc84;
      --warn:      #f5a623;
      --tab-h:     56px;
      --r:         12px;
    }

    html, body {
      height: 100%;
      background: var(--bg);
      color: var(--txt);
      font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
      -webkit-tap-highlight-color: transparent;
      touch-action: manipulation;
      overflow: hidden;
    }

    #app { display: flex; flex-direction: column; height: 100%; }

    /* TOP BAR */
    #topbar {
      flex-shrink: 0;
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 10px 18px;
      background: var(--surface);
      border-bottom: 1px solid var(--border);
    }
    #topbar h1 { font-size: 1.1rem; font-weight: 700; letter-spacing: 0.1em; color: var(--accent); }
    #topbar h1 span { color: var(--txt-mid); font-weight: 400; font-size: 0.75rem; margin-left: 6px; }

    #conn-badge {
      display: flex; align-items: center; gap: 6px;
      font-size: 0.72rem; color: var(--txt-dim);
      background: var(--surface2); border: 1px solid var(--border);
      border-radius: 20px; padding: 4px 10px;
      cursor: pointer; user-select: none;
      transition: border-color 0.2s;
    }
    #conn-badge:hover { border-color: var(--accent); }
    #conn-dot {
      width: 7px; height: 7px; border-radius: 50%;
      background: var(--txt-dim); flex-shrink: 0;
      transition: background 0.3s;
    }
    #conn-dot.ap   { background: var(--warn); }
    #conn-dot.sta  { background: var(--ok); }
    #conn-dot.spin { background: var(--accent); animation: pulse 0.8s infinite; }
    @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:0.3} }

    /* TAB BAR */
    #tabbar {
      flex-shrink: 0; display: flex;
      background: var(--surface); border-bottom: 1px solid var(--border);
    }
    .tab {
      flex: 1; height: var(--tab-h);
      display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 3px;
      font-size: 0.62rem; font-weight: 600; letter-spacing: 0.07em; text-transform: uppercase;
      color: var(--txt-dim); cursor: pointer; user-select: none;
      border-bottom: 2px solid transparent; transition: color 0.15s, border-color 0.15s;
    }
    .tab svg { width: 20px; height: 20px; stroke: currentColor; }
    .tab:hover { color: var(--txt-mid); }
    .tab.active { color: var(--accent); border-bottom-color: var(--accent); }

    /* PAGES */
    #pages { flex: 1; overflow: hidden; position: relative; }
    .page {
      position: absolute; inset: 0; overflow-y: auto;
      overscroll-behavior: contain; display: none;
      flex-direction: column; align-items: center;
      padding: 20px 16px 36px; gap: 20px;
    }
    .page.active { display: flex; }

    /* DRIVE PAGE */
    .joystick-wrap {
      display: flex; justify-content: center; align-items: center;
      margin: 16px 0; touch-action: none; user-select: none; -webkit-user-select: none;
    }
    .joystick-container {
      position: relative;
      width: 160px; height: 160px;
      border-radius: 50%;
      background: var(--surface2);
      border: 2px solid var(--border);
      box-shadow: inset 0 0 20px rgba(0,0,0,0.4);
      cursor: pointer;
    }
    .joystick-stick {
      position: absolute;
      width: 56px; height: 56px;
      left: 50%; top: 50%;
      margin-left: -28px; margin-top: -28px;
      border-radius: 50%;
      background: var(--accent);
      border: 2px solid var(--border2);
      box-shadow: 0 2px 10px rgba(0,0,0,0.4);
      pointer-events: none;
      transition: transform 0.06s ease-out;
    }
    .joystick-stick.held { transition: none; }

    .speed-readout {
      display: flex; justify-content: space-between; align-items: center;
      font-size: 0.72rem; color: var(--txt-dim);
      letter-spacing: 0.05em; text-transform: uppercase;
      margin-top: 4px;
    }
    #speed-val { color: var(--accent); font-weight: 700; }
    .btn-settings {
      width: 100%; padding: 10px 16px; margin-top: 8px;
      background: var(--accent); border: none; border-radius: var(--r);
      color: #000; font-size: 0.75rem; font-weight: 700;
      letter-spacing: 0.06em; text-transform: uppercase; cursor: pointer;
    }
    .btn-settings:hover { filter: brightness(1.1); }

    input[type=range] {
      -webkit-appearance: none; width: 100%; height: 5px;
      border-radius: 4px; background: var(--border2); outline: none; cursor: pointer;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none; width: 22px; height: 22px; border-radius: 50%;
      background: var(--accent); border: 2px solid var(--bg);
      box-shadow: 0 0 8px rgba(245,166,35,0.4);
    }
    input[type=range]::-moz-range-thumb {
      width: 22px; height: 22px; border-radius: 50%;
      background: var(--accent); border: 2px solid var(--bg);
    }
    #drive-status { font-size: 0.72rem; color: var(--txt-dim); letter-spacing: 0.04em; }

    /* CONNECTIONS PAGE */
    .card {
      width: 100%; max-width: 420px;
      background: var(--surface2); border: 1px solid var(--border); border-radius: 14px; overflow: hidden;
    }
    .card-header {
      padding: 12px 16px; background: var(--surface); border-bottom: 1px solid var(--border);
      font-size: 0.7rem; font-weight: 700; letter-spacing: 0.1em; text-transform: uppercase;
      color: var(--txt-mid); display: flex; align-items: center; justify-content: space-between;
    }
    .card-body { padding: 16px; }

    .status-row {
      display: flex; justify-content: space-between; align-items: center;
      padding: 8px 0; border-bottom: 1px solid var(--border); font-size: 0.8rem;
    }
    .status-row:last-child { border-bottom: none; }
    .status-row .label { color: var(--txt-dim); }
    .status-row .value { color: var(--txt); font-weight: 600; font-family: monospace; font-size: 0.85rem; }
    .status-row .value.green { color: var(--ok); }
    .status-row .value.amber { color: var(--warn); }
    .status-row .value.dim   { color: var(--txt-dim); }

    #btn-disconnect {
      width: auto; padding: 4px 10px; margin: 0;
      background: transparent; border: 1px solid var(--stop); border-radius: 6px;
      color: var(--stop); font-size: 0.65rem; font-weight: 600;
      letter-spacing: 0.06em; text-transform: uppercase; cursor: pointer;
      transition: background 0.15s;
    }
    #btn-disconnect:hover { background: rgba(230,57,70,0.12); }
    #btn-disconnect:disabled { opacity: 0.3; cursor: default; }

    #network-list { display: flex; flex-direction: column; gap: 4px; }
    .net-item {
      display: flex; align-items: center; gap: 10px;
      padding: 10px 12px; border-radius: 8px;
      background: var(--surface); border: 1px solid var(--border);
      cursor: pointer; transition: border-color 0.15s, background 0.15s;
    }
    .net-item:hover { border-color: var(--accent); background: var(--btn-h); }
    .net-item.selected { border-color: var(--accent); background: rgba(245,166,35,0.08); }
    .net-name { flex: 1; font-size: 0.85rem; font-weight: 500; color: var(--txt); overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
    .net-meta { display: flex; align-items: center; gap: 6px; flex-shrink: 0; }
    .lock-icon svg { width: 11px; height: 11px; display: block; }
    .signal-bars { display: flex; align-items: flex-end; gap: 2px; height: 14px; }
    .bar { width: 3px; border-radius: 1px; background: var(--border2); }
    .bar.lit { background: var(--accent); }

    .scan-placeholder { text-align: center; padding: 20px; color: var(--txt-dim); font-size: 0.8rem; }

    .form-group { display: flex; flex-direction: column; gap: 6px; margin-bottom: 12px; }
    .form-group label { font-size: 0.7rem; font-weight: 600; letter-spacing: 0.07em; text-transform: uppercase; color: var(--txt-dim); }
    .form-group input {
      padding: 10px 12px; background: var(--surface); border: 1px solid var(--border2);
      border-radius: 8px; color: var(--txt); font-size: 0.9rem; outline: none;
      transition: border-color 0.15s; width: 100%;
    }
    .form-group input:focus { border-color: var(--accent); }

    #btn-connect {
      width: 100%; padding: 12px; background: var(--accent); border: none;
      border-radius: 8px; color: #000; font-size: 0.85rem; font-weight: 700;
      letter-spacing: 0.06em; text-transform: uppercase; cursor: pointer;
      transition: background 0.15s, opacity 0.15s;
    }
    #btn-connect:hover { background: var(--accent2); }
    #btn-connect:disabled { opacity: 0.4; cursor: default; }

    #btn-scan {
      background: transparent; border: 1px solid var(--border2); border-radius: 6px;
      color: var(--txt-mid); font-size: 0.7rem; font-weight: 600;
      letter-spacing: 0.06em; text-transform: uppercase; padding: 5px 10px;
      cursor: pointer; transition: border-color 0.15s, color 0.15s;
    }
    #btn-scan:hover { border-color: var(--accent); color: var(--accent); }
    #btn-scan:disabled { opacity: 0.4; cursor: default; }

    #connect-feedback { margin-top: 10px; font-size: 0.78rem; min-height: 1.2em; text-align: center; }
    #connect-feedback.ok  { color: var(--ok); }
    #connect-feedback.err { color: var(--stop); }
    #connect-feedback.inf { color: var(--warn); }

    .page-footer { font-size: 0.65rem; color: var(--txt-dim); opacity: 0.4; padding-top: 4px; }
  </style>
</head>
<body>
<div id="app">

  <!-- Top Bar -->
  <div id="topbar">
    <h1>&#x1F916; WALL-E <span>Drive</span></h1>
    <div id="conn-badge" onclick="switchTab('connections')" title="Network status">
      <div id="conn-dot" class="ap"></div>
      <span id="conn-label">AP Only</span>
    </div>
  </div>

  <!-- Tab Bar -->
  <div id="tabbar">
    <div class="tab active" data-tab="drive" onclick="switchTab('drive')">
      <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <circle cx="12" cy="12" r="3"/>
        <path d="M12 2v3M12 19v3M2 12h3M19 12h3m-15.8-7.8 2.1 2.1M17.7 17.7l2.1 2.1M4.2 19.8l2.1-2.1M17.7 6.3l2.1-2.1"/>
      </svg>
      Drive
    </div>
    <div class="tab" data-tab="connections" onclick="switchTab('connections')">
      <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <path d="M5 12.55a11 11 0 0 1 14.08 0"/>
        <path d="M1.42 9a16 16 0 0 1 21.16 0"/>
        <path d="M8.53 16.11a6 6 0 0 1 6.95 0"/>
        <circle cx="12" cy="20" r="1" fill="currentColor"/>
      </svg>
      Connections
    </div>
    <div class="tab" data-tab="settings" onclick="switchTab('settings')">
      <svg viewBox="0 0 24 24" fill="none" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
        <circle cx="12" cy="12" r="3"/>
        <path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"/>
      </svg>
      Settings
    </div>
  </div>

  <!-- Pages -->
  <div id="pages">

    <!-- DRIVE PAGE -->
    <div class="page active" id="page-drive">
      <div class="joystick-wrap">
        <div class="joystick-container" id="joystick" aria-label="Drive joystick">
          <div class="joystick-stick" id="joystick-stick"></div>
        </div>
      </div>

      <div class="speed-readout">
        <span>Speed</span>
        <span><span id="speed-val">0</span> / <span id="speed-max">255</span></span>
      </div>

      <div id="drive-status">Ready</div>
      <div class="page-footer">WALL-E Tank Drive &bull; ESP32-S3</div>
    </div>

    <!-- CONNECTIONS PAGE -->
    <div class="page" id="page-connections">

      <!-- Status Card -->
      <div class="card">
        <div class="card-header">
          Network Status
          <button id="btn-disconnect" onclick="doDisconnect()" disabled>Disconnect</button>
        </div>
        <div class="card-body">
          <div class="status-row">
            <span class="label">AP Network</span>
            <span class="value amber" id="st-ap-ssid">WALL-E-Control</span>
          </div>
          <div class="status-row">
            <span class="label">AP Address</span>
            <span class="value" id="st-ap-ip">192.168.4.1</span>
          </div>
          <div class="status-row">
            <span class="label">AP Clients</span>
            <span class="value" id="st-ap-clients">—</span>
          </div>
          <div class="status-row">
            <span class="label">Home WiFi</span>
            <span class="value dim" id="st-sta-ssid">Not connected</span>
          </div>
          <div class="status-row">
            <span class="label">Home IP</span>
            <span class="value dim" id="st-sta-ip">—</span>
          </div>
        </div>
      </div>

      <!-- Scan Card -->
      <div class="card">
        <div class="card-header">
          Available Networks
          <button id="btn-scan" onclick="doScan()">Scan</button>
        </div>
        <div class="card-body" style="padding:8px 12px">
          <div id="network-list">
            <div class="scan-placeholder">Tap Scan to find networks</div>
          </div>
        </div>
      </div>

      <!-- Connect Form Card -->
      <div class="card">
        <div class="card-header">Connect to Network</div>
        <div class="card-body">
          <div class="form-group">
            <label>Network (SSID)</label>
            <input type="text" id="inp-ssid" placeholder="Select from list or type manually"
              autocomplete="off" autocorrect="off" autocapitalize="none" spellcheck="false">
          </div>
          <div class="form-group">
            <label>Password</label>
            <input type="password" id="inp-pass" placeholder="Enter password" autocomplete="current-password">
          </div>
          <button id="btn-connect" onclick="doConnect()">Connect</button>
          <div id="connect-feedback"></div>
        </div>
      </div>

      <div class="page-footer">Credentials saved to device NVS &bull; AP always stays active</div>
    </div>

    <!-- SETTINGS PAGE -->
    <div class="page" id="page-settings">
      <div class="card">
        <div class="card-header">Drive</div>
        <div class="card-body">
          <div class="status-row">
            <span class="label">Max speed</span>
            <span class="value"><span id="settings-max-val">255</span> / 255</span>
          </div>
          <input type="range" id="settings-max-slider" min="1" max="255" value="255" step="5" style="width:100%; margin:8px 0">
          <button id="btn-save-settings" class="btn-settings">Save</button>
        </div>
      </div>
      <div class="page-footer">Max speed limits joystick and keyboard &bull; Saved to device</div>
    </div>

  </div>
</div>

<script>
const BASE         = '';
const FAILSAFE_MS  = 440;
const STATUS_POLL  = 5000;

let activeCmd      = null;
let heartbeatTimer = null;
let pendingReq     = false;
let currentSpeed   = 0;
let maxSpeed       = 255;

// ===== TABS =====
function switchTab(name) {
  document.querySelectorAll('.tab').forEach(t =>
    t.classList.toggle('active', t.dataset.tab === name));
  document.querySelectorAll('.page').forEach(p =>
    p.classList.toggle('active', p.id === 'page-' + name));
  if (name === 'connections') refreshStatus();
  if (name === 'settings') refreshSettings();
}

// ===== DRIVE =====
function setDriveStatus(text) {
  document.getElementById('drive-status').textContent = text;
}

async function sendCmd(cmd) {
  if (pendingReq) return;
  pendingReq = true;
  try {
    const res = await fetch(BASE + '/' + cmd);
    setDriveStatus(res.ok ? cmd.toUpperCase() : 'Error ' + res.status);
  } catch(e) {
    setDriveStatus('No response');
  } finally {
    pendingReq = false;
  }
}

async function sendSpeed(val) {
  try { await fetch(BASE + '/speed?value=' + val); } catch(_) {}
}

async function sendDrive(left, right) {
  try { await fetch(BASE + '/drive?left=' + left + '&right=' + right); } catch(_) {}
}

let tankLeft = 0, tankRight = 0;
function startTankHB() {
  clearInterval(heartbeatTimer);
  heartbeatTimer = setInterval(() => sendDrive(tankLeft, tankRight), FAILSAFE_MS);
}
function stopHB() {
  clearInterval(heartbeatTimer);
  heartbeatTimer = null;
}

function onPress(cmd) {
  if (activeCmd === cmd) return;
  activeCmd = cmd;
  sendCmd(cmd);
  if (cmd !== 'stop') startHB(cmd);
}
function onRelease() {
  if (!activeCmd) return;
  activeCmd = null;
  stopHB();
  sendCmd('stop');
}

function startHB(cmd) {
  clearInterval(heartbeatTimer);
  heartbeatTimer = setInterval(() => sendCmd(cmd), FAILSAFE_MS);
}

// Tank joystick: smaller dead zone, continuous left/right mixing for smooth diagonals and curves
const JOY_DEAD = 0.12;
const JOY_MAX = 42;
function tankMix(dx, dy) {
  const r = Math.sqrt(dx*dx + dy*dy);
  if (r < JOY_DEAD) return { left: 0, right: 0 };
  const scale = Math.min(1, (r - JOY_DEAD) / (1 - JOY_DEAD)) / r;
  const throttle = -dy * scale;
  const steer = dx * scale;
  let left = throttle - steer;
  let right = throttle + steer;
  const m = Math.max(Math.abs(left), Math.abs(right), 1);
  left = Math.round((left / m) * maxSpeed);
  right = Math.round((right / m) * maxSpeed);
  left = Math.max(-maxSpeed, Math.min(maxSpeed, left));
  right = Math.max(-maxSpeed, Math.min(maxSpeed, right));
  return { left, right };
}
function updateStickVisual(px, py) {
  const stick = document.getElementById('joystick-stick');
  const r = Math.min(1, Math.sqrt(px*px + py*py));
  if (r < 0.05) { stick.style.transform = 'translate(0, 0)'; return; }
  const a = Math.atan2(py, px);
  const x = Math.cos(a) * r * JOY_MAX;
  const y = Math.sin(a) * r * JOY_MAX;
  stick.style.transform = 'translate(' + x + 'px,' + y + 'px)';
}

function updateSpeedDisplay(val) {
  currentSpeed = val;
  const el = document.getElementById('speed-val');
  if (el) el.textContent = val;
}

const joystickEl = document.getElementById('joystick');
const stickEl = document.getElementById('joystick-stick');
let joystickActive = false;
let lastTankLeft = -999, lastTankRight = -999;

function handleJoystickMove(clientX, clientY) {
  const rect = joystickEl.getBoundingClientRect();
  const cx = rect.left + rect.width/2, cy = rect.top + rect.height/2;
  const dx = (clientX - cx) / (rect.width/2);
  const dy = (clientY - cy) / (rect.height/2);
  const { left, right } = tankMix(dx, dy);
  updateStickVisual(dx, dy);
  tankLeft = left;
  tankRight = right;
  if (left !== lastTankLeft || right !== lastTankRight) {
    lastTankLeft = left;
    lastTankRight = right;
    sendDrive(left, right);
    updateSpeedDisplay(Math.round((Math.abs(left) + Math.abs(right)) / 2));
  }
  if (left !== 0 || right !== 0) {
    if (!heartbeatTimer) startTankHB();
  } else {
    stopHB();
  }
}
function handleJoystickEnd() {
  joystickActive = false;
  stickEl.classList.remove('held');
  updateStickVisual(0, 0);
  tankLeft = tankRight = 0;
  lastTankLeft = lastTankRight = -999;
  sendDrive(0, 0);
  updateSpeedDisplay(0);
  stopHB();
  sendCmd('stop');
}

joystickEl.addEventListener('mousedown', e => {
  e.preventDefault();
  joystickActive = true;
  stickEl.classList.add('held');
  handleJoystickMove(e.clientX, e.clientY);
});
joystickEl.addEventListener('mousemove', e => { if (joystickActive) handleJoystickMove(e.clientX, e.clientY); });
joystickEl.addEventListener('mouseup', () => { if (joystickActive) handleJoystickEnd(); });
joystickEl.addEventListener('mouseleave', () => { if (joystickActive) handleJoystickEnd(); });

joystickEl.addEventListener('touchstart', e => {
  e.preventDefault();
  joystickActive = true;
  stickEl.classList.add('held');
  const t = e.touches[0];
  handleJoystickMove(t.clientX, t.clientY);
}, { passive: false });
joystickEl.addEventListener('touchmove', e => {
  e.preventDefault();
  if (e.touches.length) handleJoystickMove(e.touches[0].clientX, e.touches[0].clientY);
}, { passive: false });
joystickEl.addEventListener('touchend', e => {
  if (!e.touches.length) handleJoystickEnd();
}, { passive: false });
joystickEl.addEventListener('touchcancel', () => handleJoystickEnd());

document.addEventListener('mouseup', () => { if (joystickActive) handleJoystickEnd(); });

// Settings: load on start and when opening Settings tab
async function refreshSettings() {
  try {
    const d = await fetch(BASE + '/settings').then(r => r.json());
    maxSpeed = Math.max(1, Math.min(255, d.max_speed || 255));
    const maxEl = document.getElementById('speed-max');
    if (maxEl) maxEl.textContent = maxSpeed;
    const slider = document.getElementById('settings-max-slider');
    const valEl = document.getElementById('settings-max-val');
    if (slider) { slider.value = maxSpeed; slider.max = 255; }
    if (valEl) valEl.textContent = maxSpeed;
  } catch (_) {}
}
async function saveSettings() {
  const slider = document.getElementById('settings-max-slider');
  if (!slider) return;
  const v = parseInt(slider.value, 10);
  try {
    await fetch(BASE + '/settings/set?max_speed=' + v);
    maxSpeed = Math.max(1, Math.min(255, v));
    document.getElementById('speed-max').textContent = maxSpeed;
    document.getElementById('settings-max-val').textContent = maxSpeed;
  } catch (_) {}
}
document.getElementById('btn-save-settings')?.addEventListener('click', saveSettings);
document.getElementById('settings-max-slider')?.addEventListener('input', function() {
  document.getElementById('settings-max-val').textContent = this.value;
});
refreshSettings();

const keyMap = { ArrowUp:'forward', ArrowDown:'reverse', ArrowLeft:'left', ArrowRight:'right', ' ':'stop' };
const heldKeys = new Set();
document.addEventListener('keydown', e => {
  const cmd = keyMap[e.key]; if (!cmd) return;
  e.preventDefault();
  if (heldKeys.has(e.key)) return;
  heldKeys.add(e.key);
  if (cmd !== 'stop') sendSpeed(maxSpeed);
  updateSpeedDisplay(cmd === 'stop' ? 0 : maxSpeed);
  onPress(cmd);
});
document.addEventListener('keyup', e => {
  const cmd = keyMap[e.key]; if (!cmd) return;
  heldKeys.delete(e.key);
  if (cmd !== 'stop') onRelease();
  if (!activeCmd) { updateSpeedDisplay(0); sendSpeed(0); }
});

// ===== CONNECTIONS =====
function updateBadge(state, staIp) {
  const dot   = document.getElementById('conn-dot');
  const label = document.getElementById('conn-label');
  dot.className = '';
  if      (state === 0) { dot.classList.add('ap');   label.textContent = 'AP Only'; }
  else if (state === 1) { dot.classList.add('spin'); label.textContent = 'Connecting\u2026'; }
  else if (state === 2) { dot.classList.add('sta');  label.textContent = staIp || 'Connected'; }
  else                  { dot.classList.add('ap');   label.textContent = 'WiFi Failed'; }
}

async function refreshStatus() {
  try {
    const data = await fetch(BASE + '/wifi/status').then(r => r.json());
    updateBadge(data.state, data.sta_ip);
    document.getElementById('st-ap-ssid').textContent    = data.ap_ssid  || 'WALL-E-Control';
    document.getElementById('st-ap-ip').textContent      = data.ap_ip    || '192.168.4.1';
    document.getElementById('st-ap-clients').textContent = data.ap_clients != null ? data.ap_clients : '0';
    const connected = data.state === 2;
    const connecting = data.state === 1;
    const ssiEl = document.getElementById('st-sta-ssid');
    ssiEl.textContent = data.sta_ssid || (connecting ? 'Connecting\u2026' : 'Not connected');
    ssiEl.className   = 'value ' + (connected ? 'green' : connecting ? 'amber' : 'dim');
    const ipEl = document.getElementById('st-sta-ip');
    ipEl.textContent  = data.sta_ip || '\u2014';
    ipEl.className    = 'value ' + (connected ? '' : 'dim');
    document.getElementById('btn-disconnect').disabled = !connected && !connecting;
  } catch(e) {
    updateBadge(0, null);
  }
}

function signalBars(quality) {
  const lit = quality > 75 ? 4 : quality > 50 ? 3 : quality > 25 ? 2 : 1;
  return ['4px','7px','10px','14px']
    .map((h, i) => `<div class="bar${i < lit ? ' lit' : ''}" style="height:${h}"></div>`)
    .join('');
}

async function doScan() {
  const btn  = document.getElementById('btn-scan');
  const list = document.getElementById('network-list');
  btn.disabled     = true;
  btn.textContent  = 'Scanning\u2026';
  list.innerHTML   = '<div class="scan-placeholder">Scanning, please wait\u2026</div>';
  try {
    const nets = await fetch(BASE + '/wifi/scan').then(r => r.json());
    if (!nets.length) { list.innerHTML = '<div class="scan-placeholder">No networks found</div>'; return; }
    list.innerHTML = nets.map(n => {
      const ssidEsc = n.ssid.replace(/\\/g,'\\\\').replace(/'/g,"\\'");
      return `<div class="net-item" onclick="selectNet('${ssidEsc}')">
        <span class="net-name">${n.ssid.replace(/</g,'&lt;')}</span>
        <span class="net-meta">
          ${n.enc ? `<span class="lock-icon"><svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg></span>` : ''}
          <div class="signal-bars">${signalBars(n.quality)}</div>
        </span>
      </div>`;
    }).join('');
  } catch(e) {
    list.innerHTML = '<div class="scan-placeholder" style="color:var(--stop)">Scan failed</div>';
  } finally {
    btn.disabled    = false;
    btn.textContent = 'Scan';
  }
}

function selectNet(ssid) {
  document.getElementById('inp-ssid').value = ssid;
  document.querySelectorAll('.net-item').forEach(el =>
    el.classList.toggle('selected', el.querySelector('.net-name').textContent === ssid));
  document.getElementById('inp-pass').focus();
}

async function doConnect() {
  const ssid = document.getElementById('inp-ssid').value.trim();
  const pass = document.getElementById('inp-pass').value;
  const fb   = document.getElementById('connect-feedback');
  const btn  = document.getElementById('btn-connect');
  if (!ssid) { fb.className = 'err'; fb.textContent = 'Please enter a network name'; return; }
  btn.disabled    = true;
  fb.className    = 'inf';
  fb.textContent  = 'Sending credentials\u2026';
  try {
    const url = BASE + '/wifi/connect?ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass);
    const res  = await fetch(url);
    const data = await res.json();
    if (!res.ok) { fb.className = 'err'; fb.textContent = data.error || 'Error'; btn.disabled = false; return; }
    fb.className  = 'inf';
    fb.textContent = 'Connecting to ' + ssid + '\u2026 (up to 12s)';
    let attempts = 0;
    const poll = setInterval(async () => {
      attempts++;
      try {
        const s = await fetch(BASE + '/wifi/status').then(r => r.json());
        refreshStatus();
        if (s.state === 2) {
          clearInterval(poll);
          fb.className   = 'ok';
          fb.textContent = 'Connected! IP: ' + s.sta_ip;
          btn.disabled   = false;
        } else if (s.state === 3 || attempts > 14) {
          clearInterval(poll);
          fb.className   = 'err';
          fb.textContent = s.state === 3 ? 'Connection failed \u2014 check password' : 'Timed out';
          btn.disabled   = false;
        }
      } catch(_) {}
    }, 1000);
  } catch(e) {
    fb.className   = 'err';
    fb.textContent = 'Could not reach device';
    btn.disabled   = false;
  }
}

async function doDisconnect() {
  document.getElementById('btn-disconnect').disabled = true;
  try {
    await fetch(BASE + '/wifi/disconnect');
    document.getElementById('connect-feedback').className = '';
    document.getElementById('connect-feedback').textContent = '';
    setTimeout(refreshStatus, 500);
  } catch(_) {}
}

// INIT
(async () => {
  try { await fetch(BASE + '/stop'); setDriveStatus('Ready'); }
  catch(_) { setDriveStatus('Offline'); }
  refreshStatus();
  setInterval(refreshStatus, STATUS_POLL);
})();
</script>
</body>
</html>
)rawliteral";
