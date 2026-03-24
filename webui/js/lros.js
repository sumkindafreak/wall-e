/**
 * WALL-E LROS - Living Robot Operating System
 * Full Web Console JavaScript
 */
const BASE = '';
const TOAST_DURATION = 4000;
const TOAST_MAX_VISIBLE = 2;
const FAILSAFE_MS = 440;

let toastId = 0;
/** Only page-home shows toast popups; updated by switchTab */
let currentVisiblePage = 'home';
let tankLeft = 0, tankRight = 0, maxSpeed = 255;
let driveMode = 'joystick';
let hbTimer = null;
let stateCache = {};
let cydOverride = false;
var _lastToastState = { lowBattery: false, rth: false, interest: false };
var _docReleaseBound = false;

const JOY_DEAD = 0.12, JOY_MAX = 40;

// ─── Navigation ─────────────────────────────────────────────
function switchTab(name) {
  currentVisiblePage = name;
  const stack = document.getElementById('toast-stack');
  if (stack && name !== 'home') stack.innerHTML = '';
  document.querySelectorAll('.tab').forEach(t => t.classList.toggle('active', t.dataset.tab === name));
  document.querySelectorAll('.page').forEach(p => p.classList.toggle('active', p.id === 'page-' + name));
  if (name === 'network') { fetchStatus(); showNetworkForm(false); }
  if (name === 'vision') initVision();
  if (name === 'navigation') drawMap();
  if (name === 'telemetry' || name === 'power') fetchStatus();
  if (name === 'drive') {
    document.getElementById('drive-joystick').style.display = driveMode === 'joystick' ? 'flex' : 'none';
    document.getElementById('drive-tank').style.display = driveMode === 'tank' ? 'block' : 'none';
    initJoystick();
    initTankSliders();
  }
}

// ─── Toasts (emotional presence) ────────────────────────────
function showToast(emoji, text) {
  if (currentVisiblePage !== 'home') return;
  const stack = document.getElementById('toast-stack');
  if (!stack) return;
  const el = document.createElement('div');
  el.className = 'toast';
  el.innerHTML = '<span style="margin-right:8px">' + emoji + '</span>' + text;
  stack.appendChild(el);
  while (stack.children.length > TOAST_MAX_VISIBLE) stack.removeChild(stack.firstChild);
  setTimeout(() => el.remove(), TOAST_DURATION);
}

// 5V rail: low = below 4.3V. Only show each state-toast once per transition (no spam every poll).
function updateToastsFromState(s) {
  if (!s) return;
  if (s.battery && s.battery.voltage < 4.3) {
    if (!_lastToastState.lowBattery) { _lastToastState.lowBattery = true; showToast('\uD83D\uDD0B', "I'm getting tired"); }
  } else { _lastToastState.lowBattery = false; }
  if (s.auto && s.auto.rthActive) {
    if (!_lastToastState.rth) { _lastToastState.rth = true; showToast('\uD83D\uDCE1', 'Searching for my dock'); }
  } else { _lastToastState.rth = false; }
  if (s.auto && s.auto.enabled && s.auto.interest > 70) {
    if (!_lastToastState.interest) { _lastToastState.interest = true; showToast('\uD83D\uDE0A', 'I like being with you'); }
  } else { _lastToastState.interest = false; }
}

// ─── Face & theme ───────────────────────────────────────────
function setFaceMood(mood) {
  const face = document.getElementById('walle-face');
  if (face) face.dataset.mood = (mood || 'happy').toLowerCase();
}

function setTheme(theme) {
  document.documentElement.dataset.theme = theme || '';
}

function setOverrideBanner(visible) {
  document.getElementById('override-banner').classList.toggle('visible', !!visible);
}

let _pillState = {};
function pollNodeHealth() {
  fetch(BASE + '/api/system/health').then(r => r.json()).then(s => {
    if (!s || !s.nodes) return;
    const map = {};
    s.nodes.forEach(function(n) { map[n.id] = n; });
    ['base','master','audio','dock','vision'].forEach(function(id) {
      const el = document.querySelector('.node-pill[data-pill="' + id + '"]');
      if (!el) return;
      const n = map[id];
      if (!n) return;
      const on = !!n.online;
      const prev = _pillState[id];
      _pillState[id] = on;
      el.classList.remove('ok','warn','off');
      if (on) el.classList.add('ok');
      else el.classList.add('off');
      if (prev !== undefined && prev !== on) {
        el.classList.add('edge');
        setTimeout(function() { el.classList.remove('edge'); }, 500);
      }
    });
    const bfill = document.getElementById('status-batt-fill');
    const base = map.base;
    if (bfill && base && base.battery_pct != null && base.battery_pct >= 0) {
      bfill.style.width = Math.max(0, Math.min(100, base.battery_pct)) + '%';
      bfill.className = 'status-batt-fill' + (base.battery_pct < 20 ? ' low' : '');
    }
    const dockIc = document.getElementById('status-dock-ic');
    if (dockIc && map.dock) {
      var chg = (map.dock.flags & 2) !== 0;
      dockIc.classList.toggle('charging', chg);
    }
  }).catch(function() {});
}

// ─── Drive ──────────────────────────────────────────────────
function setDriveMode(mode) {
  driveMode = mode;
  document.getElementById('drive-joystick').style.display = mode === 'joystick' ? 'flex' : 'none';
  document.getElementById('drive-tank').style.display = mode === 'tank' ? 'block' : 'none';
  document.querySelectorAll('#page-drive [id^="mode-"]').forEach(b => b.classList.remove('btn-ghost'));
  document.querySelectorAll('#page-drive [id^="mode-"]').forEach(b => b.classList.add('btn-ghost'));
  const m = document.getElementById('mode-' + mode);
  if (m) { m.classList.remove('btn-ghost'); m.classList.add('btn'); }
}

function tankMix(dx, dy) {
  const r = Math.sqrt(dx*dx + dy*dy);
  if (r < JOY_DEAD) return { left: 0, right: 0 };
  const scale = Math.min(1, (r - JOY_DEAD) / (1 - JOY_DEAD)) / r;
  const throttle = -dy * scale, steer = dx * scale;
  let left = throttle - steer, right = throttle + steer;
  const m = Math.max(Math.abs(left), Math.abs(right), 1);
  left = Math.round((left/m)*maxSpeed); right = Math.round((right/m)*maxSpeed);
  return { left: Math.max(-maxSpeed, Math.min(maxSpeed, left)), right: Math.max(-maxSpeed, Math.min(maxSpeed, right)) };
}

function updateStick(px, py) {
  const stick = document.getElementById('joystick-stick');
  if (!stick) return;
  const r = Math.min(1, Math.sqrt(px*px + py*py));
  if (r < 0.05) { stick.style.transform = 'translate(0,0)'; return; }
  const x = (px/r)*JOY_MAX, y = (py/r)*JOY_MAX;
  stick.style.transform = 'translate('+x+'px,'+y+'px)';
}

async function sendDrive(l, r) {
  try { await fetch(BASE + '/drive?left='+l+'&right='+r); } catch(_) {}
  const sp = document.getElementById('drive-speed');
  if (sp) sp.textContent = Math.round((Math.abs(l)+Math.abs(r))/2) + ' / ' + maxSpeed;
}

function onJoyMove(cx, cy) {
  const joy = document.getElementById('joystick');
  if (!joy) return;
  const rect = joy.getBoundingClientRect();
  const dx = (cx - rect.left - rect.width/2) / (rect.width/2);
  const dy = (cy - rect.top - rect.height/2) / (rect.height/2);
  const { left, right } = tankMix(dx, dy);
  updateStick(dx, dy);
  tankLeft = left; tankRight = right;
  sendDrive(left, right);
  if (left === 0 && right === 0) {
    clearInterval(hbTimer);
    hbTimer = null;
    try { fetch(BASE + '/stop'); } catch(_) {}
  } else {
    if (!hbTimer) hbTimer = setInterval(() => sendDrive(tankLeft, tankRight), FAILSAFE_MS);
  }
}

function onJoyEnd() {
  const stick = document.getElementById('joystick-stick');
  if (stick) stick.classList.remove('held');
  updateStick(0,0);
  tankLeft = tankRight = 0;
  sendDrive(0,0);
  clearInterval(hbTimer);
  hbTimer = null;
  try { fetch(BASE + '/stop'); } catch(_) {}
}

function initJoystick() {
  var joy = document.getElementById('joystick');
  var stick = document.getElementById('joystick-stick');
  if (!joy || !stick) return;
  joy.onmousedown = function(e) { e.preventDefault(); stick.classList.add('held'); onJoyMove(e.clientX, e.clientY); };
  joy.onmousemove = function(e) { if (stick.classList.contains('held')) onJoyMove(e.clientX, e.clientY); };
  joy.onmouseup = onJoyEnd;
  joy.onmouseleave = onJoyEnd;
  joy.ontouchstart = function(e) { e.preventDefault(); stick.classList.add('held'); onJoyMove(e.touches[0].clientX, e.touches[0].clientY); };
  joy.ontouchmove = function(e) { e.preventDefault(); if (e.touches.length) onJoyMove(e.touches[0].clientX, e.touches[0].clientY); };
  joy.ontouchend = function(e) { if (!e.touches.length) onJoyEnd(); };
  if (!_docReleaseBound) {
    _docReleaseBound = true;
    document.addEventListener('mouseup', function() { if (stick.classList.contains('held')) onJoyEnd(); });
  }
}

function initTankSliders() {
  var tl = document.getElementById('tank-left'), tr = document.getElementById('tank-right');
  if (!tl || !tr) return;
  var send = function() {
    var l = parseInt(tl.value, 10), r = parseInt(tr.value, 10);
    sendDrive(l, r);
    if (l === 0 && r === 0) {
      clearInterval(hbTimer);
      hbTimer = null;
      try { fetch(BASE + '/stop'); } catch(_) {}
    } else {
      startTankHB();
    }
  };
  tl.addEventListener('input', send);
  tr.addEventListener('input', send);
}

function startTankHB() {
  if (hbTimer) return;
  hbTimer = setInterval(() => {
    const tl = document.getElementById('tank-left'), tr = document.getElementById('tank-right');
    if (tl && tr) sendDrive(parseInt(tl.value,10), parseInt(tr.value,10));
  }, FAILSAFE_MS);
}

function applySpeedProfile() {
  const s = document.getElementById('speed-profile');
  if (!s) return;
  const v = s.value;
  maxSpeed = v === 'low' ? 128 : v === 'high' ? 255 : 200;
  try { fetch(BASE + '/settings/set?max_speed=' + maxSpeed); } catch(_) {}
}

// ─── E-Stop ─────────────────────────────────────────────────
async function doEStop() {
  if (navigator.vibrate) navigator.vibrate(100);
  try { await fetch(BASE + '/stop'); } catch(_) {}
  showToast('\u26D4', 'Stopped');
}

// ─── Status & API ───────────────────────────────────────────
async function fetchStatus() {
  try {
    const [wifi, battery, auto, imu, settings] = await Promise.all([
      fetch(BASE + '/wifi/status').then(r => r.json()).catch(() => ({})),
      fetch(BASE + '/battery/status').then(r => r.json()).catch(() => ({})),
      fetch(BASE + '/api/autonomy').then(r => r.json()).catch(() => ({})),
      fetch(BASE + '/imu/status').then(r => r.json()).catch(() => ({})),
      fetch(BASE + '/settings').then(r => r.json()).catch(() => ({}))
    ]);
    stateCache = { wifi, battery, auto, imu, settings };

    // Connection badge
    const dot = document.getElementById('conn-dot'), lbl = document.getElementById('conn-label');
    if (dot) dot.className = wifi.state === 2 ? 'sta' : wifi.state === 1 ? 'pulse' : 'ap';
    if (lbl) lbl.textContent = wifi.state === 2 ? (wifi.sta_ip || 'Connected') : wifi.state === 1 ? 'Connecting…' : 'AP';

    // Home
    setById('home-battery', battery.voltage != null ? battery.voltage + ' V' : '-');
    setById('home-state', auto.state || '-');
    setById('home-emotion', auto.emotion || '-');
    const pct = battery.percent != null ? battery.percent : (battery.voltage != null ? Math.min(100, Math.max(0, (battery.voltage - 10.5) / 2.1 * 100)) : 80);
    const bar = document.getElementById('home-battery-bar');
    if (bar) { bar.style.width = pct + '%'; bar.className = 'progress-fill' + (pct < 20 ? ' critical' : pct < 40 ? ' low' : ''); }

    setFaceMood(auto.emotion || (auto.enabled ? 'curious' : 'happy'));
    if (pct < 25) setTheme('low-battery');
    else if (auto.rthActive) setTheme('docking');
    else setTheme('');

    // Network
    setById('net-ap', wifi.ap_ssid || 'WALL-E-Control');
    setById('net-sta', wifi.sta_ssid || 'Not connected');

    // Docking (placeholder - no dock API yet)
    setById('dock-stage', auto.rthActive ? 'Homing' : '-');
    setById('dock-beam', '-');
    setById('dock-current', '-');

    // Telemetry
    setById('tel-heading', imu.heading != null ? imu.heading + '\u00B0' : '-');
    setById('tel-orient', (imu.pitch != null && imu.roll != null) ? imu.pitch + '\u00B0 / ' + imu.roll + '\u00B0' : '-');
    setById('tel-sonar', auto.sonar != null ? auto.sonar + ' cm' : '-');
    const sonarBar = document.getElementById('tel-sonar-bar');
    if (sonarBar && auto.sonar != null) sonarBar.style.width = Math.min(100, 100 - auto.sonar / 2) + '%';
    setById('tel-gps', auto.gpsValid ? 'Valid' : 'No fix');
    setById('tel-sats', '-');

    // Power
    setById('pwr-voltage', battery.voltage != null ? battery.voltage + ' V' : '-');
    setById('pwr-runtime', '-');
    const pwrBar = document.getElementById('pwr-bar');
    if (pwrBar) { pwrBar.style.width = pct + '%'; pwrBar.className = 'progress-fill' + (pct < 20 ? ' critical' : pct < 40 ? ' low' : ''); }

    maxSpeed = settings.max_speed || 255;
    const sp = document.getElementById('speed-profile');
    if (sp) sp.value = maxSpeed <= 128 ? 'low' : maxSpeed >= 255 ? 'high' : 'normal';

    // Navigation
    setById('nav-home', auto.gpsValid ? 'Set' : 'Not set');
    setById('nav-current', auto.gpsValid ? (auto.lat + ', ' + auto.lon) : '-');

    updateToastsFromState(stateCache);
  } catch (_) {}
}

function setById(id, v) {
  const el = document.getElementById(id);
  if (el && el.textContent !== undefined) el.textContent = v;
}

async function apiCall(path) {
  try {
    const r = await fetch(BASE + path);
    const t = await r.text();
    showToast('\u2713', r.ok ? 'OK' : t);
  } catch (e) { showToast('\u26A0', 'Error'); }
}

// ─── Network wizard ─────────────────────────────────────────
function showNetworkForm(show) {
  const card = document.getElementById('network-form-card');
  if (card) card.style.display = show !== false ? 'block' : 'none';
  if (show) doScan();
}

async function doScan() {
  const list = document.getElementById('network-list');
  if (!list) return;
  list.innerHTML = '<div class="log-item value dim">Scanning...</div>';
  try {
    const nets = await fetch(BASE + '/wifi/scan').then(r => r.json());
    if (!nets.length) { list.innerHTML = '<div class="log-item value dim">No networks</div>'; return; }
    const sorted = nets.slice().sort((a,b) => (b.rssi||-100) - (a.rssi||-100));
    list.innerHTML = sorted.slice(0,10).map(n => 
      '<div class="net-item" data-ssid="' + (n.ssid||'').replace(/"/g,'&quot;') + '" onclick="selectNet(\'' + (n.ssid||'').replace(/'/g,"\\'") + '\')"><span class="net-name">' + (n.ssid||'') + '</span><span class="net-rssi">' + (n.rssi||'') + ' dBm</span></div>'
    ).join('');
  } catch (e) { list.innerHTML = '<div class="log-item value dim">Scan failed</div>'; }
}

function selectNet(ssid) {
  document.getElementById('net-ssid').value = ssid;
  document.querySelectorAll('.net-item').forEach(el => el.classList.toggle('selected', (el.dataset.ssid||el.querySelector('.net-name')?.textContent) === ssid));
}

async function doConnect() {
  const ssid = document.getElementById('net-ssid').value.trim();
  const pass = document.getElementById('net-pass').value;
  const btn = document.querySelector('#network-form-card .btn');
  if (!ssid) { showToast('\u26A0', 'Enter SSID'); return; }
  if (btn) btn.disabled = true;
  try {
    await fetch(BASE + '/wifi/connect?ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass));
    showToast('\uD83D\uDCE1', 'Connecting...');
    setTimeout(fetchStatus, 2000);
  } catch (e) { showToast('\u26A0', 'Failed'); }
  if (btn) btn.disabled = false;
}

// ─── Auto dock, Set home ────────────────────────────────────
function triggerAutoDock() {
  showToast('\uD83D\uDCE1', 'Docking...');
  apiCall('/api/autonomy/enable?enable=1');
  switchTab('docking');
}

function setHome() {
  apiCall('/api/autonomy/set_home');
  showToast('\uD83D\uCCCD', 'Home set');
}

// ─── Map ───────────────────────────────────────────────────
function drawMap() {
  const c = document.getElementById('map-canvas');
  if (!c) return;
  const ctx = c.getContext('2d');
  const w = c.width, h = c.height;
  ctx.fillStyle = '#0f1219';
  ctx.fillRect(0,0,w,h);
  ctx.strokeStyle = '#252d3a';
  ctx.strokeRect(0,0,w,h);
  ctx.fillStyle = '#f5a623';
  ctx.beginPath();
  ctx.arc(w/2, h/2, 8, 0, Math.PI*2);
  ctx.fill();
  ctx.fillStyle = '#fff';
  ctx.font = '10px sans-serif';
  ctx.fillText('WALL-E', w/2 - 20, h/2 - 12);
}

// ─── Vision ─────────────────────────────────────────────────
function initVision() {
  const img = document.getElementById('fpv-img');
  const ph = document.getElementById('fpv-placeholder');
  if (img && ph) {
    img.src = BASE + '/api/vision/stream?' + Date.now();
    img.onload = () => { img.style.display = 'block'; ph.style.display = 'none'; };
    img.onerror = () => { img.style.display = 'none'; ph.style.display = 'flex'; };
  }
}

function snapshot() { showToast('\uD83D\uDCF7', 'Snapshot (API not implemented)'); }
function toggleNightMode() { showToast('\uD83C\uDF19', 'Night mode (API not implemented)'); }

// ─── Audio ──────────────────────────────────────────────────
function playSound(id) {
  showToast('\uD83D\uDD0A', id);
  apiCall('/api/audio/play?id=' + id);
}

function setVolume(v) {
  document.getElementById('audio-vol-val').textContent = v;
  apiCall('/api/audio/volume?value=' + v);
}

// ─── AI ─────────────────────────────────────────────────────
function setBehaviourMode(mode) {
  apiCall('/api/personality/mode?mode=' + mode);
  setFaceMood(mode);
}

// ─── Missions ───────────────────────────────────────────────
function runMission(id) {
  if (id === 'rth') triggerAutoDock();
  else showToast('\uD83C\uDFAF', 'Mission ' + id);
}

function addWaypoint() { showToast('\uD83D\uCCCD', 'Add waypoint (API not implemented)'); }

// ─── Files ──────────────────────────────────────────────────
function refreshFiles() {
  fetch(BASE + '/api/files/list').then(r => r.json()).then(d => {
    const list = document.getElementById('file-list');
    if (list) list.innerHTML = (d.files || []).length ? d.files.map(f => '<div class="log-item">' + f + '</div>').join('') : '<div class="log-item value dim">No files</div>';
  }).catch(() => {});
}

// ─── Developer ──────────────────────────────────────────────
function devFetch() {
  const inp = document.getElementById('dev-cmd');
  if (!inp) return;
  fetch(BASE + inp.value).then(r => r.text()).then(t => showToast('\u2713', t.slice(0,50))).catch(e => showToast('\u26A0', String(e)));
}

// ─── Init ───────────────────────────────────────────────────
function initAll() {
  initJoystick();
  initTankSliders();
  fetchStatus();
  pollNodeHealth();
  setInterval(fetchStatus, 5000);
  setInterval(pollNodeHealth, 1500);
  try { fetch(BASE + '/stop'); } catch(_) {}
  setTimeout(() => showToast('\uD83D\uDE0A', "Hi! I'm WALL-E"), 3000);
}
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', initAll);
} else {
  initAll();
}
