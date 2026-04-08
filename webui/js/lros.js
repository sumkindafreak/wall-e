/**
 * WALL-E LROS - Living Robot Operating System
 * Full Web Console JavaScript
 */
function getBaseUrl() {
  try {
    var u = localStorage.getItem('walle_base_url');
    if (u) return u.replace(/\/?$/, '');
  } catch (e) {}
  return '';
}
/** Prefix-relative API paths with optional origin from Settings */
function api(path) {
  var p = path || '';
  if (p.charAt(0) !== '/') p = '/' + p;
  var b = getBaseUrl();
  return (b || '') + p;
}
/** Attach X-Wall-E-Token from localStorage when set (optional API token on base). */
function apiAuthHeaders() {
  var h = {};
  try {
    var t = localStorage.getItem('walle_api_token');
    if (t) h['X-Wall-E-Token'] = t;
  } catch (e) {}
  return h;
}
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
var missionQueue = [];
var _imuHist = [];
let cydOverride = false;
var _operatorLastOk = 0;
var _operatorPayload = null;
var _lastToastState = { lowBattery: false, rth: false, interest: false };
var _docReleaseBound = false;

const JOY_DEAD = 0.12, JOY_MAX = 40;
const HEAD_JOY_MAX = 32;
const HEAD_SERVO_CH_PAN = 0;
const HEAD_SERVO_CH_TILT = 1;
var _headLastServoSend = 0;
const TANK_DEADZONE = 15;
const DRIVE_SMOOTH = 0.32;
var _smoothL = 0;
var _smoothR = 0;
var _cmdInflight = 0;
var _battHist = [];
var _sonarHist = [];
var _TELE_CAP = 64;
var LS_DISMISSED_CARDS = 'walle_dismissed_cards';
var LS_WIDGET_ORDER = 'walle_widget_order';
var LS_HOME_CARD_ORDER = 'walle_home_card_order';
var LS_GEOFENCE = 'walle_geofence_v1';
var _gfPrevInside = null;
var _gfEstopLatched = false;
var _gfPanelBound = false;

// ─── Navigation ─────────────────────────────────────────────
var _dockPollTimer = null;

function switchTab(name) {
  currentVisiblePage = name;
  const stack = document.getElementById('toast-stack');
  if (stack && name !== 'home') stack.innerHTML = '';
  document.querySelectorAll('.tab').forEach(t => t.classList.toggle('active', t.dataset.tab === name));
  document.querySelectorAll('.page').forEach(p => p.classList.toggle('active', p.id === 'page-' + name));
  if (_dockPollTimer) {
    clearInterval(_dockPollTimer);
    _dockPollTimer = null;
  }
  if (name === 'network') { fetchStatus(); showNetworkForm(false); }
  if (name === 'home') {
    bindLaserControls();
    fetch(api('/api/laser/status')).then(function (r) { return r.json(); }).then(refreshLaserPanel).catch(function () {});
  }
  if (name === 'settings') loadConnectionSettings();
  if (name === 'safety' && typeof refreshGeofenceDisplay === 'function') refreshGeofenceDisplay();
  if (name === 'docking') {
    refreshDockPanel(true);
    _dockPollTimer = setInterval(function () { refreshDockPanel(false); }, 2500);
  }
  if (name === 'vision') initVision();
  if (name === 'navigation' && window.LrosNavigation) {
    setTimeout(function () {
      LrosNavigation.init();
      if (window.LrosMapNav && typeof LrosMapNav.onTabShow === 'function') LrosMapNav.onTabShow();
      if (window.NavWorldContext) {
        NavWorldContext.bindGeoButton();
        NavWorldContext.refresh({}, { force: true });
      }
    }, 0);
  }
  if (name === 'sequence' && window.LrosSequences) {
    setTimeout(function () { LrosSequences.onTabShow(); }, 0);
  }
  if (name === 'telemetry' || name === 'power') fetchStatus();
  if (name === 'drive') {
    var aiPanel = document.getElementById('drive-ai-panel');
    document.getElementById('drive-joystick').style.display = driveMode === 'joystick' ? 'flex' : 'none';
    document.getElementById('drive-tank').style.display = driveMode === 'tank' ? 'block' : 'none';
    if (aiPanel) aiPanel.style.display = driveMode === 'ai' ? 'block' : 'none';
    var deck = document.getElementById('drive-deck');
    if (deck) deck.dataset.activeMode = driveMode;
    var pd = document.getElementById('page-drive');
    if (pd) pd.dataset.mode = driveMode;
    var man = document.getElementById('drive-manual-zone');
    if (man) man.style.display = driveMode === 'ai' ? 'none' : '';
    syncDriveCockpitHint();
    initJoystick();
    initHeadPad();
    initTankSliders();
    applyDriveLockUI(_operatorPayload);
  }
  refreshAutonomyPolling();
}

function shouldPollAutonomy() {
  return (currentVisiblePage === 'drive' && driveMode === 'ai') || currentVisiblePage === 'ai';
}

function refreshAutonomyPolling() {
  if (shouldPollAutonomy()) startAiAssistPolling();
  else stopAiAssistPolling();
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
  const m = (mood || 'happy').toLowerCase();
  const face = document.getElementById('walle-face');
  if (face) face.dataset.mood = m;
  document.body.dataset.mood = m;
}

function setTheme(theme) {
  document.documentElement.dataset.theme = theme || '';
}

/** Optional opts: { text, severity: 'info'|'warn'|'danger' } */
function setOverrideBanner(visible, opts) {
  var el = document.getElementById('override-banner');
  if (!el) return;
  el.classList.toggle('visible', !!visible);
  el.classList.remove('severity--info', 'severity--warn', 'severity--danger');
  if (visible && opts && opts.severity) el.classList.add('severity--' + opts.severity);
  if (opts && opts.text) el.textContent = opts.text;
  else if (!visible) el.textContent = 'Local Control Active - CYD touchscreen has control';
}

function computeOperatorLinkState(j) {
  if (!navigator.onLine) return 'OFFLINE';
  if (!j) {
    if (_operatorLastOk <= 0) return 'TIMEOUT';
    return Date.now() - _operatorLastOk > 12000 ? 'OFFLINE' : 'TIMEOUT';
  }
  if (j._linkDegraded) return 'TIMEOUT';
  if (j.command_stale) return 'STALE';
  var ws = window.WalleConnection && WalleConnection.getState();
  if (ws === 'ws') return 'LIVE';
  return 'HTTP FALLBACK';
}

function deriveBannerFromOperator(j, linkState) {
  if (!j) {
    if (linkState === 'OFFLINE' || linkState === 'TIMEOUT') {
      return { visible: true, text: 'Connection lost — operator state unavailable', severity: 'danger' };
    }
    return { visible: false };
  }
  if (linkState === 'OFFLINE') {
    return { visible: true, text: 'Browser offline — commands will not reach the base', severity: 'danger' };
  }
  if (j.unifiedSafety || j.authority === 'SAFETY') {
    return { visible: true, text: j.lock_reason || 'Safety stop latched — drive disabled', severity: 'danger' };
  }
  if (j.authority === 'POLICY') {
    return { visible: true, text: j.lock_reason || 'Motion policy limits CYD vs browser', severity: 'warn' };
  }
  if (j.authority === 'CYD') {
    return { visible: true, text: j.lock_reason || 'CYD touchscreen has control', severity: 'warn' };
  }
  if (j.authority === 'DOCKING') {
    return { visible: true, text: j.lock_reason || 'Docking controller owns drive', severity: 'info' };
  }
  if (j.drive_locked && j.authority === 'AI') {
    return { visible: true, text: j.lock_reason || 'AI assist active — manual override available', severity: 'info' };
  }
  if (j.command_stale) {
    return { visible: true, text: j.lock_reason || 'Connection stale — commands may be ignored', severity: 'warn' };
  }
  if (linkState === 'TIMEOUT') {
    return { visible: true, text: 'Operator link interrupted — retrying', severity: 'warn' };
  }
  return { visible: false };
}

function applyDriveLockUI(j) {
  var pd = document.getElementById('page-drive');
  var msg = document.getElementById('drive-lock-msg');
  if (!pd) return;
  var locked = !!(j && j.drive_locked);
  pd.classList.toggle('drive-console-locked', locked);
  if (j && j.authority) pd.dataset.authority = String(j.authority).toLowerCase();
  else pd.removeAttribute('data-authority');
  if (msg) {
    if (locked && j && j.lock_reason) {
      msg.hidden = false;
      msg.textContent = j.lock_reason;
    } else if (locked) {
      msg.hidden = false;
      msg.textContent = 'Drive controls are locked by the base.';
    } else {
      msg.hidden = true;
      msg.textContent = '';
    }
  }
}

function updateOperatorConsoleOffline() {
  var strip = document.getElementById('operator-strip');
  if (strip) {
    strip.classList.add('operator-strip--offline', 'operator-strip--stale');
    strip.classList.remove('operator-strip--locked');
  }
  setById('op-authority-val', 'UNKNOWN');
  setById('op-policy-val', '—');
  setById('op-motion-val', 'OFFLINE');
  setById('op-profile-val', '—');
  setById('op-link-val', 'OFFLINE');
  setById('op-fresh-val', '—');
  setById('op-lock-val', 'Robot offline or link lost');
  document.body.classList.add('lros-operator-offline');
  try {
    window.__lrosOperatorSnapshot = null;
  } catch (e) {}
  applyDriveLockUI(null);
  var b = deriveBannerFromOperator(null, 'OFFLINE');
  setOverrideBanner(b.visible, b.visible ? { text: b.text, severity: b.severity } : {});
}

function updateOperatorConsole(j, linkStateOverride) {
  if (!j) return;
  var strip = document.getElementById('operator-strip');
  if (strip) {
    strip.classList.remove('operator-strip--offline');
    strip.classList.toggle('operator-strip--stale', !!j.command_stale || !!j._linkDegraded);
    strip.classList.toggle('operator-strip--locked', !!j.drive_locked);
  }
  document.body.classList.remove('lros-operator-offline');

  var linkState = linkStateOverride !== undefined && linkStateOverride !== null
    ? linkStateOverride
    : computeOperatorLinkState(j);
  setById('op-authority-val', j.authority || 'UNKNOWN');
  setById('op-policy-val', j.motion_policy || '—');
  setById('op-motion-val', j.motion || '—');
  setById('op-profile-val', j.drive_profile || '—');
  setById('op-link-val', linkState);
  var age = j.last_command_age_ms != null ? Number(j.last_command_age_ms) : null;
  var fs = j.failsafe_timeout_ms != null ? Number(j.failsafe_timeout_ms) : null;
  var freshStr = age != null ? age + ' ms' : '—';
  if (fs != null) freshStr += ' / ' + fs + ' ms failsafe';
  setById('op-fresh-val', freshStr);
  var lockShow = j.lock_reason || (!j.drive_locked ? '—' : 'locked');
  setById('op-lock-val', lockShow);

  applyDriveLockUI(j);

  var b = deriveBannerFromOperator(j, linkState);
  setOverrideBanner(b.visible, b.visible ? { text: b.text, severity: b.severity } : {});

  if (typeof console !== 'undefined' && console.debug) {
    console.debug('[LROS operator]', j.authority, j.motion, j.drive_profile, linkState, 'drive_locked=', j.drive_locked);
  }
  try {
    window.__lrosOperatorSnapshot = j;
  } catch (e) {}
  if (typeof updateNavMapHud === 'function') updateNavMapHud();
}

var _lastVisionEvtMs = 0;
var _visionEventsInited = false;

function pollVisionEvents() {
  fetch(api('/api/vision/events'), { cache: 'no-store', headers: apiAuthHeaders() })
    .then(function (r) {
      return r.json();
    })
    .then(function (j) {
      if (!j || !j.events || !j.events.length) return;
      var maxT = 0;
      j.events.forEach(function (ev) {
        if (ev && ev.t_ms > maxT) maxT = ev.t_ms;
      });
      if (!_visionEventsInited) {
        _lastVisionEvtMs = maxT;
        _visionEventsInited = true;
        return;
      }
      j.events.forEach(function (ev) {
        if (ev && ev.t_ms > _lastVisionEvtMs && ev.code) {
          if (typeof showToast === 'function') showToast('\uD83D\uDCF9', 'Vision event ' + ev.code);
        }
      });
      if (maxT > _lastVisionEvtMs) _lastVisionEvtMs = maxT;
    })
    .catch(function () {});
}

function pollMotionOperator() {
  fetch(api('/api/motion/operator'), { cache: 'no-store' })
    .then(function (r) {
      if (!r.ok) throw new Error('bad status');
      return r.json();
    })
    .then(function (j) {
      _operatorLastOk = Date.now();
      delete j._linkDegraded;
      _operatorPayload = j;
      if (window.WalleConnection) WalleConnection.markHttpOk();
      updateOperatorConsole(j);
    })
    .catch(function (e) {
      if (typeof console !== 'undefined' && console.debug) console.debug('[LROS operator] poll failed', e);
      if (_operatorPayload) {
        var degraded = Object.assign({}, _operatorPayload, { _linkDegraded: true });
        if (!degraded.lock_reason) degraded.lock_reason = 'Connection stale — commands may be ignored';
        updateOperatorConsole(degraded, 'TIMEOUT');
      } else {
        updateOperatorConsoleOffline();
      }
    });
}

/** Navigation page — mission HUD chips (MapLibre deck). */
function updateNavMapHud() {
  try {
    var page = document.getElementById('page-navigation');
    if (!page || !page.classList.contains('active')) return;
    var auto = stateCache.auto || {};
    var imu = stateCache.imu || {};
    var op = window.__lrosOperatorSnapshot;
    var set = function (id, t) {
      var el = document.getElementById(id);
      if (el) el.textContent = t;
    };
    set('nav-hud-autonomy', auto.state != null ? String(auto.state) : '—');
    set('nav-hud-motion', op && op.motion ? String(op.motion) : '—');
    set('nav-hud-profile', op && op.drive_profile ? String(op.drive_profile) : '—');
    if (op && op.last_command_age_ms != null) set('nav-hud-tel-age', String(op.last_command_age_ms) + ' ms');
    else set('nav-hud-tel-age', '—');
    set('nav-hud-gps', auto.gpsValid ? 'Fix' : 'No fix');
    set('nav-hud-hdg', imu.heading != null ? imu.heading + '\u00B0' : '—');
    var ri = window.LrosNavigation && LrosNavigation.getRouteInfo ? LrosNavigation.getRouteInfo() : {};
    set('nav-hud-route-m', ri.pathLength != null ? '~' + ri.pathLength.toFixed(1) + ' u' : '—');
    set('nav-hud-eta', ri.etaSeconds != null && ri.etaSeconds ? '~' + ri.etaSeconds + ' s' : '—');
    if (window.LrosNavigation && typeof LrosNavigation.getMapSnapshot === 'function') {
      var s = LrosNavigation.getMapSnapshot();
      if (s.waypoints && s.waypoints.length && auto.gpsValid && auto.lat != null && auto.lon != null) {
        var w = s.waypoints[0];
        var d = haversineMeters(Number(auto.lat), Number(auto.lon), w.lat, w.lng);
        set('nav-hud-next-wp', Math.round(d) + ' m');
      } else {
        set('nav-hud-next-wp', '—');
      }
    }
  } catch (e) {
    if (typeof console !== 'undefined' && console.debug) console.debug('[LROS] nav HUD', e);
  }
}

let _pillState = {};
function pollNodeHealth() {
  fetch(api('/api/system/health')).then(r => r.json()).then(s => {
    if (!s || !s.nodes) return;
    const map = {};
    s.nodes.forEach(function(n) { map[n.id] = n; });
    ['base','master','audio','dock','vision'].forEach(function(id) {
      const nodes = document.querySelectorAll('.node-pill[data-pill="' + id + '"]');
      if (!nodes.length) return;
      const n = map[id];
      if (!n) return;
      const on = !!n.online;
      const prev = _pillState[id];
      _pillState[id] = on;
      nodes.forEach(function (el) {
        el.classList.remove('ok', 'warn', 'off');
        if (on) el.classList.add('ok');
        else el.classList.add('off');
        if (prev !== undefined && prev !== on) {
          el.classList.add('edge');
          setTimeout(function () { el.classList.remove('edge'); }, 500);
        }
      });
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
    var beamEl = document.getElementById('dock-beam');
    if (beamEl && map.dock) {
      var chg2 = (map.dock.flags & 2) !== 0;
      beamEl.textContent = chg2 ? 'Charge flag active (fleet)' : 'No charge flag · IR/beam not detailed in API';
      beamEl.className = 'value dim' + (chg2 ? ' ok' : '');
    }
    var snu = document.getElementById('settings-node-updated');
    if (snu) {
      snu.innerHTML = 'Last sync <span class="mono">' + new Date().toLocaleTimeString() + '</span> · <span class="mono">/api/system/health</span>';
    }
  }).catch(function() {});
}

// ─── Drive ──────────────────────────────────────────────────
function syncDriveCockpitHint() {
  var hint = document.getElementById('drive-cockpit-hint');
  if (!hint) return;
  if (driveMode === 'tank') {
    hint.textContent = 'Drag the vertical rollers — top forward, bottom reverse. Two fingers for independent treads.';
  } else {
    hint.textContent = 'Head: left pad — pan & tilt the face (servo 0/1). Drive: right stick — same tank mix as CYD.';
  }
}

function setDriveMode(mode) {
  var prev = driveMode;
  driveMode = mode;
  var aiPanel = document.getElementById('drive-ai-panel');
  var joy = document.getElementById('drive-joystick');
  var tank = document.getElementById('drive-tank');
  if (joy) joy.style.display = mode === 'joystick' ? 'flex' : 'none';
  if (tank) tank.style.display = mode === 'tank' ? 'block' : 'none';
  if (aiPanel) aiPanel.style.display = mode === 'ai' ? 'block' : 'none';
  document.querySelectorAll('#page-drive [id^="mode-"]').forEach(b => b.classList.remove('btn-ghost'));
  document.querySelectorAll('#page-drive [id^="mode-"]').forEach(b => b.classList.add('btn-ghost'));
  const m = document.getElementById('mode-' + mode);
  if (m) { m.classList.remove('btn-ghost'); m.classList.add('btn'); }

  var deck = document.getElementById('drive-deck');
  if (deck) deck.dataset.activeMode = mode;
  var pd = document.getElementById('page-drive');
  if (pd) pd.dataset.mode = mode;
  var man = document.getElementById('drive-manual-zone');
  if (man) man.style.display = mode === 'ai' ? 'none' : '';

  syncDriveCockpitHint();
  document.querySelectorAll('#page-drive [id^="mode-"]').forEach(function (b) {
    b.setAttribute('aria-selected', b.id === 'mode-' + mode ? 'true' : 'false');
  });

  if (mode === 'ai') {
    try { fetch(api('/api/autonomy/enable?enable=1')); } catch (e) {}
  } else if (prev === 'ai') {
    try { fetch(api('/api/autonomy/enable?enable=0')); } catch (e) {}
    try { fetch(api('/api/autonomy/manual?active=0')); } catch (e) {}
  }
  refreshAutonomyPolling();
}

function startAiAssistPolling() {
  stopAiAssistPolling();
  function tick() {
    fetch(api('/api/autonomy'))
      .then(function (r) { return r.json(); })
      .then(function (d) {
        updateDriveAiPanel(d);
        updateAiAutonomyPanel(d);
      })
      .catch(function () {});
  }
  tick();
  window._aiAssistPoll = setInterval(tick, 1500);
}

function stopAiAssistPolling() {
  if (window._aiAssistPoll) {
    clearInterval(window._aiAssistPoll);
    window._aiAssistPoll = null;
  }
}

function updateDriveAiPanel(auto) {
  if (!auto) return;
  var en = document.getElementById('drive-ai-enabled');
  var st = document.getElementById('drive-ai-state');
  var un = document.getElementById('drive-ai-unified');
  var sf = document.getElementById('drive-ai-safety');
  var em = document.getElementById('drive-ai-emotion');
  var mo = document.getElementById('drive-ai-manual');
  if (en) en.textContent = auto.enabled ? 'On' : 'Off';
  if (st) st.textContent = auto.state != null ? String(auto.state) : '';
  if (un) un.textContent = auto.unifiedState != null ? String(auto.unifiedState) : '';
  if (sf) {
    sf.textContent = auto.unifiedSafety ? 'STOP' : 'OK';
    sf.className = 'value' + (auto.unifiedSafety ? ' safety-stop' : '');
  }
  if (em) em.textContent = auto.emotion != null ? String(auto.emotion) : '';
  if (mo) mo.textContent = auto.manualOverride ? 'Manual' : 'AI';
}

/** AI & Autonomy page — live /api/autonomy (unified brain + engine + sensors) */
function updateAiAutonomyPanel(auto) {
  if (!auto) return;
  function set(id, text) {
    var el = document.getElementById(id);
    if (el && el.textContent !== undefined) el.textContent = text;
  }
  set('ai-autonomy-enabled', auto.enabled ? 'On' : 'Off');
  set('ai-autonomy-state', auto.state != null ? String(auto.state) : '\u2014');
  set('ai-unified', auto.unifiedState != null ? String(auto.unifiedState) : '\u2014');
  var sf = document.getElementById('ai-safety');
  if (sf) {
    sf.textContent = auto.unifiedSafety ? 'STOP' : 'OK';
    sf.className = 'value' + (auto.unifiedSafety ? ' safety-stop' : '');
  }
  set('ai-manual', auto.manualOverride ? 'Manual' : 'AI');
  set('ai-sonar', auto.sonar != null ? Number(auto.sonar).toFixed(1) + ' cm' : '\u2014');
  set('ai-interest', auto.interest != null ? Number(auto.interest).toFixed(0) : '\u2014');
  set('ai-heading', auto.heading != null ? Number(auto.heading).toFixed(0) + '\u00B0' : '\u2014');
  set('ai-emotion-live', auto.emotion != null ? String(auto.emotion) : '\u2014');
  set('ai-pose-emotion', auto.poseEmotion != null ? String(auto.poseEmotion) : '\u2014');
  var rth = document.getElementById('ai-rth');
  if (rth) {
    rth.textContent = auto.rthActive
      ? (auto.rthState ? String(auto.rthState) : 'Active') +
        (auto.rthDistance != null ? ' \u00B7 ' + Number(auto.rthDistance).toFixed(1) + ' m' : '')
      : 'Off';
  }
  set('ai-gps', auto.gpsValid ? 'Fix' : 'No fix');
  set('ai-gps-detail', auto.gpsValid && auto.lat != null && auto.lon != null
    ? Number(auto.lat).toFixed(5) + ', ' + Number(auto.lon).toFixed(5)
    : '\u2014');
  set('ai-object', auto.objectDetected ? 'Yes' : 'No');
  var pill = document.getElementById('ai-pill-unified');
  if (pill) {
    pill.textContent = auto.unifiedState != null ? String(auto.unifiedState) : '\u2014';
    pill.classList.toggle('brain-error', !!auto.unifiedSafety);
  }
  if (auto.personality) {
    var p = auto.personality;
    set('ai-p-cur', p.curiosity != null ? Number(p.curiosity).toFixed(2) : '\u2014');
    set('ai-p-bra', p.bravery != null ? Number(p.bravery).toFixed(2) : '\u2014');
    set('ai-p-nrg', p.energy != null ? Number(p.energy).toFixed(2) : '\u2014');
    set('ai-p-rnd', p.randomness != null ? Number(p.randomness).toFixed(2) : '\u2014');
    var c = document.getElementById('ai-curiosity');
    if (c && p.curiosity != null) c.value = String(Math.round(Number(p.curiosity) * 100));
    var e = document.getElementById('ai-energy');
    if (e && p.energy != null) e.value = String(Math.round(Number(p.energy) * 100));
  }
}

function aiAssistStart() {
  apiCall('/api/autonomy/enable?enable=1');
  fetch(api('/api/autonomy')).then(r => r.json()).then(function (d) {
    updateDriveAiPanel(d);
    updateAiAutonomyPanel(d);
  }).catch(function () {});
}

function aiAssistStop() {
  apiCall('/api/autonomy/enable?enable=0');
  try { fetch(api('/api/autonomy/manual?active=0')); } catch (e) {}
  fetch(api('/api/autonomy')).then(r => r.json()).then(function (d) {
    updateDriveAiPanel(d);
    updateAiAutonomyPanel(d);
  }).catch(function () {});
}

function aiAssistTakeOver() {
  try { fetch(api('/api/autonomy/manual?active=1')); } catch (e) {}
  fetch(api('/api/autonomy')).then(r => r.json()).then(function (d) {
    updateDriveAiPanel(d);
    updateAiAutonomyPanel(d);
  }).catch(function () {});
}

function aiAssistResume() {
  try { fetch(api('/api/autonomy/manual?active=0')); } catch (e) {}
  fetch(api('/api/autonomy')).then(r => r.json()).then(function (d) {
    updateDriveAiPanel(d);
    updateAiAutonomyPanel(d);
  }).catch(function () {});
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
  const joy = document.getElementById('joystick');
  const vec = document.getElementById('joystick-vector');
  if (!stick) return;
  const r = Math.min(1, Math.sqrt(px * px + py * py));
  if (joy) joy.classList.toggle('joystick-active', r > 0.08);
  if (r < 0.05) {
    stick.style.transform = 'translate(0,0)';
    if (vec) vec.style.transform = 'rotate(0deg)';
    return;
  }
  const x = (px / r) * JOY_MAX;
  const y = (py / r) * JOY_MAX;
  stick.style.transform = 'translate(' + x + 'px,' + y + 'px)';
  if (vec) {
    const deg = (Math.atan2(px, -py) * 180) / Math.PI;
    vec.style.transform = 'rotate(' + deg + 'deg)';
  }
}

function updateHeadReadout(pan, tilt) {
  var el = document.getElementById('head-servo-readout');
  if (el) el.textContent = 'Pan ' + pan + ' · Tilt ' + tilt;
}

function sendHeadServos(pan, tilt) {
  pan = Math.max(0, Math.min(100, Math.round(pan)));
  tilt = Math.max(0, Math.min(100, Math.round(tilt)));
  var now = Date.now();
  if (now - _headLastServoSend < 72) return;
  _headLastServoSend = now;
  updateHeadReadout(pan, tilt);
  try {
    fetch(api('/servo/set?ch=' + HEAD_SERVO_CH_PAN + '&pos=' + pan + '&speed=85'));
    fetch(api('/servo/set?ch=' + HEAD_SERVO_CH_TILT + '&pos=' + tilt + '&speed=85'));
  } catch (_) {}
}

function updateHeadStick(px, py) {
  const stick = document.getElementById('head-stick');
  const joy = document.getElementById('head-joystick');
  const vec = document.getElementById('head-vector');
  if (!stick) return;
  const r = Math.min(1, Math.sqrt(px * px + py * py));
  if (joy) joy.classList.toggle('joystick-active', r > 0.08);
  if (r < 0.05) {
    stick.style.transform = 'translate(0,0)';
    if (vec) vec.style.transform = 'rotate(0deg)';
    return;
  }
  const x = (px / r) * HEAD_JOY_MAX;
  const y = (py / r) * HEAD_JOY_MAX;
  stick.style.transform = 'translate(' + x + 'px,' + y + 'px)';
  if (vec) {
    const deg = (Math.atan2(px, -py) * 180) / Math.PI;
    vec.style.transform = 'rotate(' + deg + 'deg)';
  }
}

function onHeadMove(cx, cy) {
  const joy = document.getElementById('head-joystick');
  if (!joy) return;
  const rect = joy.getBoundingClientRect();
  let dx = (cx - rect.left - rect.width / 2) / (rect.width / 2);
  let dy = (cy - rect.top - rect.height / 2) / (rect.height / 2);
  const mag = Math.sqrt(dx * dx + dy * dy);
  if (mag > 1) {
    dx /= mag;
    dy /= mag;
  }
  const pan = 50 + dx * 48;
  const tilt = 50 - dy * 48;
  updateHeadStick(dx, dy);
  sendHeadServos(pan, tilt);
}

function onHeadEnd() {
  const stick = document.getElementById('head-stick');
  const joy = document.getElementById('head-joystick');
  if (stick) stick.classList.remove('held');
  if (joy) joy.classList.remove('joystick-active');
  updateHeadStick(0, 0);
}

function bindGlobalPointerRelease() {
  if (_docReleaseBound) return;
  _docReleaseBound = true;
  document.addEventListener('mouseup', function () {
    var s = document.getElementById('joystick-stick');
    if (s && s.classList.contains('held')) onJoyEnd();
    var hs = document.getElementById('head-stick');
    if (hs && hs.classList.contains('held')) onHeadEnd();
  });
}

function applyTankDeadzone(v) {
  return Math.abs(v) < TANK_DEADZONE ? 0 : v;
}

async function sendDrive(l, r) {
  var sl = l;
  var sr = r;
  if (driveMode === 'joystick') {
    if (l === 0 && r === 0) {
      _smoothL = 0;
      _smoothR = 0;
    } else {
      _smoothL += (l - _smoothL) * DRIVE_SMOOTH;
      _smoothR += (r - _smoothR) * DRIVE_SMOOTH;
      sl = Math.round(_smoothL);
      sr = Math.round(_smoothR);
    }
  }
  try {
    await fetch(api('/drive?left=' + sl + '&right=' + sr));
  } catch (_) {}
  var avg = Math.round((Math.abs(sl) + Math.abs(sr)) / 2);
  const sp = document.getElementById('drive-speed');
  if (sp) sp.textContent = avg + ' / ' + maxSpeed;
  var ml = document.getElementById('drive-motor-l');
  var mr = document.getElementById('drive-motor-r');
  if (ml) ml.textContent = String(sl);
  if (mr) mr.textContent = String(sr);
  var bar = document.getElementById('drive-speed-bar');
  if (bar && maxSpeed > 0) {
    bar.style.width = Math.min(100, (avg / maxSpeed) * 100) + '%';
  }
  var tlb = document.getElementById('drive-tank-l-bar');
  var trb = document.getElementById('drive-tank-r-bar');
  if (tlb && maxSpeed > 0) tlb.style.width = Math.min(100, (Math.abs(sl) / maxSpeed) * 100) + '%';
  if (trb && maxSpeed > 0) trb.style.width = Math.min(100, (Math.abs(sr) / maxSpeed) * 100) + '%';
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
    try { fetch(api('/stop')); } catch(_) {}
  } else {
    if (!hbTimer) hbTimer = setInterval(() => sendDrive(tankLeft, tankRight), FAILSAFE_MS);
  }
}

function onJoyEnd() {
  const stick = document.getElementById('joystick-stick');
  const joy = document.getElementById('joystick');
  if (stick) stick.classList.remove('held');
  if (joy) joy.classList.remove('joystick-active');
  updateStick(0, 0);
  tankLeft = tankRight = 0;
  sendDrive(0, 0);
  clearInterval(hbTimer);
  hbTimer = null;
  try { fetch(api('/stop')); } catch(_) {}
}

function initJoystick() {
  var joy = document.getElementById('joystick');
  var stick = document.getElementById('joystick-stick');
  if (!joy || !stick) return;
  bindGlobalPointerRelease();
  joy.onmousedown = function(e) { e.preventDefault(); stick.classList.add('held'); onJoyMove(e.clientX, e.clientY); };
  joy.onmousemove = function(e) { if (stick.classList.contains('held')) onJoyMove(e.clientX, e.clientY); };
  joy.onmouseup = onJoyEnd;
  joy.onmouseleave = onJoyEnd;
  joy.ontouchstart = function(e) { e.preventDefault(); stick.classList.add('held'); onJoyMove(e.touches[0].clientX, e.touches[0].clientY); };
  joy.ontouchmove = function(e) { e.preventDefault(); if (e.touches.length) onJoyMove(e.touches[0].clientX, e.touches[0].clientY); };
  joy.ontouchend = function(e) { if (!e.touches.length) onJoyEnd(); };
}

function initHeadPad() {
  var joy = document.getElementById('head-joystick');
  var stick = document.getElementById('head-stick');
  if (!joy || !stick) return;
  bindGlobalPointerRelease();
  joy.onmousedown = function(e) { e.preventDefault(); stick.classList.add('held'); onHeadMove(e.clientX, e.clientY); };
  joy.onmousemove = function(e) { if (stick.classList.contains('held')) onHeadMove(e.clientX, e.clientY); };
  joy.onmouseup = onHeadEnd;
  joy.onmouseleave = onHeadEnd;
  joy.ontouchstart = function(e) { e.preventDefault(); stick.classList.add('held'); onHeadMove(e.touches[0].clientX, e.touches[0].clientY); };
  joy.ontouchmove = function(e) { e.preventDefault(); if (e.touches.length) onHeadMove(e.touches[0].clientX, e.touches[0].clientY); };
  joy.ontouchend = function(e) { if (!e.touches.length) onHeadEnd(); };
}

function syncTankRollerVisual(side, v) {
  var thumb = document.getElementById(side === 'left' ? 'tank-thumb-left' : 'tank-thumb-right');
  if (!thumb) return;
  var t = (v + 255) / 510;
  thumb.style.bottom = t * 100 + '%';
}

function applyTankRollersFromInputs() {
  var tl = document.getElementById('tank-left');
  var tr = document.getElementById('tank-right');
  if (!tl || !tr) return;
  var l = applyTankDeadzone(parseInt(tl.value, 10));
  var r = applyTankDeadzone(parseInt(tr.value, 10));
  tankLeft = l;
  tankRight = r;
  syncTankRollerVisual('left', l);
  syncTankRollerVisual('right', r);
  sendDrive(l, r);
  if (l === 0 && r === 0) {
    clearInterval(hbTimer);
    hbTimer = null;
    try { fetch(api('/stop')); } catch (_) {}
  } else {
    startTankHB();
  }
}

function initTankRollers() {
  var leftTrack = document.getElementById('tank-roller-left');
  var rightTrack = document.getElementById('tank-roller-right');
  if (!leftTrack || !rightTrack) return false;
  function bindTrack(track, side) {
    var hidden = document.getElementById(side === 'left' ? 'tank-left' : 'tank-right');
    function applyY(clientY) {
      var rect = track.getBoundingClientRect();
      var t = (clientY - rect.top) / rect.height;
      t = Math.max(0, Math.min(1, t));
      var v = Math.round((1 - t) * 510 - 255);
      if (hidden) hidden.value = String(v);
      syncTankRollerVisual(side, v);
      applyTankRollersFromInputs();
      try {
        track.setAttribute('aria-valuenow', String(v));
      } catch (_) {}
    }
    function release(e) {
      try {
        if (track.hasPointerCapture(e.pointerId)) track.releasePointerCapture(e.pointerId);
      } catch (_) {}
    }
    track.onpointerdown = function (e) {
      e.preventDefault();
      try {
        track.setPointerCapture(e.pointerId);
      } catch (_) {}
      applyY(e.clientY);
    };
    track.onpointermove = function (e) {
      if (!track.hasPointerCapture(e.pointerId)) return;
      applyY(e.clientY);
    };
    track.onpointerup = release;
    track.onpointercancel = release;
  }
  bindTrack(leftTrack, 'left');
  bindTrack(rightTrack, 'right');
  var tl = document.getElementById('tank-left');
  var tr = document.getElementById('tank-right');
  if (tl) syncTankRollerVisual('left', parseInt(tl.value, 10) || 0);
  if (tr) syncTankRollerVisual('right', parseInt(tr.value, 10) || 0);
  return true;
}

function initTankSliders() {
  if (initTankRollers()) return;
  var tl = document.getElementById('tank-left');
  var tr = document.getElementById('tank-right');
  if (!tl || !tr) return;
  if (tl.type === 'hidden') return;
  var send = function() {
    var l = applyTankDeadzone(parseInt(tl.value, 10));
    var r = applyTankDeadzone(parseInt(tr.value, 10));
    sendDrive(l, r);
    if (l === 0 && r === 0) {
      clearInterval(hbTimer);
      hbTimer = null;
      try { fetch(api('/stop')); } catch(_) {}
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
    if (tl && tr) {
      sendDrive(applyTankDeadzone(parseInt(tl.value, 10)), applyTankDeadzone(parseInt(tr.value, 10)));
    }
  }, FAILSAFE_MS);
}

function applySpeedProfile() {
  const s = document.getElementById('speed-profile');
  if (!s) return;
  const v = s.value;
  maxSpeed = v === 'low' ? 128 : v === 'high' ? 255 : 200;
  try { fetch(api('/settings/set?max_speed=' + maxSpeed)); } catch(_) {}
  var s2 = document.getElementById('set-speed-profile');
  if (s2) s2.value = v;
  var ro = document.getElementById('set-max-speed-readout');
  if (ro) ro.textContent = String(maxSpeed);
}

function applySpeedFromSettings() {
  var s = document.getElementById('set-speed-profile');
  if (!s) return;
  var v = s.value;
  maxSpeed = v === 'low' ? 128 : v === 'high' ? 255 : 200;
  try { fetch(api('/settings/set?max_speed=' + maxSpeed)); } catch(_) {}
  var sp = document.getElementById('speed-profile');
  if (sp) sp.value = v;
  var ro = document.getElementById('set-max-speed-readout');
  if (ro) ro.textContent = String(maxSpeed);
}

var UI_LS_DENSE = 'walle_ui_dense';
var UI_LS_MOTION = 'walle_ui_reduced_motion';
var UI_LS_SCALE = 'walle_ui_scale';

function applyUiPreferencesFromStorage() {
  try {
    var dense = localStorage.getItem(UI_LS_DENSE) === '1';
    var motion = localStorage.getItem(UI_LS_MOTION) === '1';
    var scale = parseFloat(localStorage.getItem(UI_LS_SCALE) || '1');
    if (isNaN(scale) || scale < 0.85 || scale > 1.25) scale = 1;
    document.documentElement.classList.toggle('ui-dense', dense);
    document.documentElement.setAttribute('data-reduced-motion', motion ? '1' : '0');
    document.documentElement.style.fontSize = scale * 100 + '%';
  } catch (e) {}
}

function loadUiPreferencesToForm() {
  try {
    var d = document.getElementById('set-dense');
    var m = document.getElementById('set-reduced-motion');
    var sc = document.getElementById('set-ui-scale');
    if (d) d.checked = localStorage.getItem(UI_LS_DENSE) === '1';
    if (m) m.checked = localStorage.getItem(UI_LS_MOTION) === '1';
    if (sc) sc.value = localStorage.getItem(UI_LS_SCALE) || '1';
  } catch (e) {}
}

function saveUiPreferences() {
  var d = document.getElementById('set-dense');
  var m = document.getElementById('set-reduced-motion');
  var sc = document.getElementById('set-ui-scale');
  try {
    localStorage.setItem(UI_LS_DENSE, d && d.checked ? '1' : '0');
    localStorage.setItem(UI_LS_MOTION, m && m.checked ? '1' : '0');
    localStorage.setItem(UI_LS_SCALE, sc ? sc.value : '1');
  } catch (e) {}
  applyUiPreferencesFromStorage();
  var el = document.getElementById('settings-ping');
  if (el) el.textContent = 'Display preferences saved.';
  setTimeout(function () {
    if (el && el.textContent === 'Display preferences saved.') el.textContent = '';
  }, 2800);
}

function clearLocalConsolePrefs() {
  try {
    localStorage.removeItem(UI_LS_DENSE);
    localStorage.removeItem(UI_LS_MOTION);
    localStorage.removeItem(UI_LS_SCALE);
    localStorage.removeItem(LS_DISMISSED_CARDS);
    localStorage.removeItem(LS_WIDGET_ORDER);
    localStorage.removeItem(LS_HOME_CARD_ORDER);
    localStorage.removeItem(LS_GEOFENCE);
  } catch (e) {}
  loadUiPreferencesToForm();
  applyUiPreferencesFromStorage();
  restoreDismissedPanels();
  if (typeof applyWidgetOrder === 'function') applyWidgetOrder();
  if (typeof applyHomeCardOrder === 'function') applyHomeCardOrder();
  var hint = document.getElementById('settings-ls-hint');
  if (hint) hint.textContent = 'UI prefs & panel layout cleared (URLs unchanged).';
}

function readDismissedCards() {
  try {
    var j = localStorage.getItem(LS_DISMISSED_CARDS);
    return j ? JSON.parse(j) : [];
  } catch (e) {
    return [];
  }
}
function writeDismissedCards(ids) {
  try {
    localStorage.setItem(LS_DISMISSED_CARDS, JSON.stringify(ids));
  } catch (e) {}
}
function restoreDismissedPanels() {
  var ids = readDismissedCards();
  ids.forEach(function (id) {
    var c = document.getElementById(id);
    if (c) {
      c.classList.remove('card--dismissed');
      c.removeAttribute('hidden');
    }
  });
  writeDismissedCards([]);
  var hint = document.getElementById('settings-ls-hint');
  if (hint) hint.textContent = 'All panels restored.';
}

async function testConnectionPing() {
  var el = document.getElementById('settings-ping');
  if (el) el.textContent = 'Testing…';
  var t0 = performance.now();
  try {
    var r = await fetch(api('/wifi/status'));
    var ms = Math.round(performance.now() - t0);
    if (el) el.textContent = (r.ok ? 'HTTP ' + r.status : 'HTTP ' + r.status) + ' · ' + ms + ' ms';
    if (window.WalleConnection && r.ok) WalleConnection.markHttpOk();
  } catch (e) {
    if (el) el.textContent = 'Failed: ' + (e && e.message ? e.message : 'network');
  }
}

function copyBaseUrl() {
  var b = getBaseUrl() || window.location.origin;
  var el = document.getElementById('settings-ping');
  function done(msg) {
    if (el) el.textContent = msg;
  }
  try {
    if (navigator.clipboard && navigator.clipboard.writeText) {
      navigator.clipboard.writeText(b).then(function () { done('Copied: ' + b); }).catch(function () { done(b); });
    } else {
      done(b);
    }
  } catch (e) {
    done(String(b));
  }
}

function updateDockTimelineFsm(fsmName) {
  var fsm = String(fsmName || 'IDLE').toUpperCase();
  var idx = -1;
  if (fsm === 'SEARCHING') idx = 0;
  else if (fsm === 'ALIGNING') idx = 1;
  else if (fsm === 'APPROACH') idx = 2;
  else if (fsm === 'DOCKED') idx = 2;
  else if (fsm === 'CHARGING') idx = 3;
  var order = ['seek', 'align', 'seat', 'charge'];
  document.querySelectorAll('#dock-timeline .dock-step').forEach(function (el) {
    el.classList.remove('dock-step--done', 'dock-step--current', 'dock-step--pending');
    var stepKey = el.getAttribute('data-step') || '';
    var si = order.indexOf(stepKey);
    if (idx < 0 || si < 0) {
      el.classList.add('dock-step--pending');
      return;
    }
    if (si < idx) el.classList.add('dock-step--done');
    else if (si === idx) el.classList.add('dock-step--current');
    else el.classList.add('dock-step--pending');
  });
}

function refreshDockPanel(force) {
  var battery = stateCache.battery || {};
  var auto = stateCache.auto || {};
  var pct =
    battery.percent != null
      ? battery.percent
      : battery.voltage != null
        ? Math.min(100, Math.max(0, ((battery.voltage - 10.5) / 2.1) * 100))
        : null;

  function applyBatteryToDock() {
    setById('dock-voltage', battery.voltage != null ? battery.voltage + ' V' : '—');
    setById('dock-current', pct != null ? Math.round(pct) + '% SoC' : '—');
    var dbar = document.getElementById('dock-charge-bar');
    if (dbar && pct != null) {
      dbar.style.width = pct + '%';
      dbar.className = 'progress-fill' + (pct < 20 ? ' critical' : pct < 40 ? ' low' : '');
    }
  }
  applyBatteryToDock();

  setById('dock-stage', auto.rthActive ? 'Return home' : auto.state != null ? String(auto.state) : '—');
  setById('dock-rth', auto.rthActive ? 'Active' : 'Idle');

  fetch(api('/api/dock/status'))
    .then(function (r) {
      return r.json();
    })
    .then(function (d) {
      if (!d || typeof d !== 'object') return;
      var fsm = d.fsm != null ? String(d.fsm) : '—';
      setById('dock-fsm', fsm);
      var pill = document.getElementById('dock-fsm-pill');
      if (pill) pill.textContent = fsm;
      var sub = document.getElementById('dock-hero-sub');
      if (sub) {
        if (d.active) sub.textContent = 'Docking controller is running.';
        else if (String(fsm).toUpperCase() === 'CHARGING') sub.textContent = 'On charge · contacts closed.';
        else if (String(fsm).toUpperCase() === 'DOCKED') sub.textContent = 'Seated on dock.';
        else sub.textContent = 'Idle · ready to start docking.';
      }
      setById('dock-active', d.active ? 'Yes' : 'No');
      setById('dock-node-online', d.dock_node_online ? 'Online' : 'Offline');
      var flags = d.dock_flags;
      setById('dock-flags', flags != null ? '0x' + Number(flags).toString(16) : '—');
      var beam = document.getElementById('dock-beam');
      if (beam) {
        var charging = (Number(flags) & 2) !== 0;
        beam.textContent = charging
          ? 'Charge flag active (fleet)'
          : 'No charge flag · IR/beam not detailed in API';
        beam.className = 'value dim' + (charging ? ' ok' : '');
      }
      var hint = document.getElementById('dock-node-hint');
      if (hint) {
        hint.textContent = d.dock_node_online ? 'Dock ESP reachable from base.' : 'Dock node offline or not reporting.';
      }
      updateDockTimelineFsm(fsm);
      var hero = document.querySelector('.dock-hero');
      if (hero) {
        hero.classList.toggle('dock-hero--active', !!d.active);
        hero.classList.toggle('dock-hero--charge', String(fsm).toUpperCase() === 'CHARGING');
      }
    })
    .catch(function () {
      if (force) {
        setById('dock-fsm', '—');
        var pill = document.getElementById('dock-fsm-pill');
        if (pill) pill.textContent = 'Unreachable';
        var sub = document.getElementById('dock-hero-sub');
        if (sub) sub.textContent = 'Could not load /api/dock/status';
      }
    });
}

// ─── Command feedback (latency, HUD, global toast) ─────────
function showFeedbackToast(ok, text) {
  var el = document.getElementById('feedback-toast');
  if (!el) return;
  el.textContent = text;
  el.className = 'feedback-toast visible ' + (ok ? 'ok' : 'err');
  clearTimeout(window._fbToastTimer);
  window._fbToastTimer = setTimeout(function () {
    el.classList.remove('visible');
  }, 2200);
}

function logActivity(text, icon) {
  pushActivity(text, icon);
}

function recordCommandHud(ok, pathShort, latencyMs, httpStatus) {
  var lat = document.getElementById('hud-latency');
  var cmd = document.getElementById('hud-last-cmd');
  if (lat) lat.textContent = latencyMs != null ? latencyMs + ' ms' : '\u2014';
  if (cmd) {
    cmd.textContent = (ok ? 'OK ' : 'FAIL ') + pathShort + (httpStatus ? ' [' + httpStatus + ']' : '');
    cmd.className = 'hud-pill hud-cmd ' + (ok ? 'ok' : 'fail');
  }
}

function updateCmdInflight(delta) {
  _cmdInflight = Math.max(0, _cmdInflight + delta);
  var q = document.getElementById('hud-queue');
  if (q) q.textContent = 'In flight: ' + _cmdInflight;
}

async function apiCall(path) {
  var start = performance.now();
  var shortPath = path.length > 48 ? path.slice(0, 46) + '\u2026' : path;
  updateCmdInflight(1);
  try {
    var r = await fetch(api(path));
    var latency = Math.round(performance.now() - start);
    var t = await r.text();
    var ok = r.ok;
    recordCommandHud(ok, shortPath, latency, r.status);
    logActivity('CMD ' + (ok ? 'OK' : 'ERR') + ' ' + shortPath + ' (' + latency + 'ms)', ok ? '\u2713' : '\u2717');
    showFeedbackToast(ok, (ok ? 'OK' : 'HTTP ' + r.status) + ' \u00B7 ' + shortPath + ' \u00B7 ' + latency + 'ms');
    if (currentVisiblePage === 'home') {
      showToast(ok ? '\u2713' : '\u26A0', (ok ? 'Command OK' : 'Command failed') + ' (' + latency + 'ms)');
    }
    updateCmdInflight(-1);
    return { ok: ok, status: r.status, body: t };
  } catch (e) {
    var latency = Math.round(performance.now() - start);
    recordCommandHud(false, shortPath, latency, 0);
    logActivity('CMD FAIL ' + shortPath, '\u2717');
    showFeedbackToast(false, 'Network error \u00B7 ' + shortPath);
    if (currentVisiblePage === 'home') showToast('\u26A0', 'Command failed');
    updateCmdInflight(-1);
    throw e;
  }
}

// ─── E-Stop (hold to confirm) ────────────────────────────────
function initEstopHold() {
  var btn = document.getElementById('estop-btn');
  if (!btn || btn.dataset.holdBound === '1') return;
  btn.dataset.holdBound = '1';
  var holdMs = 720;
  var timer = null;
  function arm() {
    clearTimeout(timer);
    btn.classList.add('estop-holding');
    timer = setTimeout(function () {
      btn.classList.remove('estop-holding');
      doEStop(true);
    }, holdMs);
  }
  function disarm() {
    clearTimeout(timer);
    btn.classList.remove('estop-holding');
  }
  btn.addEventListener('mousedown', arm);
  btn.addEventListener('mouseup', disarm);
  btn.addEventListener('mouseleave', disarm);
  btn.addEventListener('touchstart', function (e) {
    e.preventDefault();
    arm();
  }, { passive: false });
  btn.addEventListener('touchend', disarm);
  btn.addEventListener('touchcancel', disarm);
}

async function doEStop(fromHold) {
  if (navigator.vibrate) navigator.vibrate([30, 40, 80]);
  document.body.classList.add('estop-latched');
  setTimeout(function () {
    document.body.classList.remove('estop-latched');
  }, 3500);
  try {
    await fetch(api('/stop'));
  } catch (_) {}
  showFeedbackToast(true, 'E-STOP \u00B7 motors released');
  showToast('\u26D4', 'Emergency stop');
  if (window.speechSynthesis && fromHold) {
    try {
      var u = new SpeechSynthesisUtterance('Emergency stop');
      u.volume = 0.35;
      window.speechSynthesis.speak(u);
    } catch (e) {}
  }
}

// ─── Geo-fence (Safety page, localStorage + live GPS from /api/autonomy) ───
function haversineMeters(lat1, lon1, lat2, lon2) {
  var R = 6371000;
  var toRad = Math.PI / 180;
  var p1 = lat1 * toRad;
  var p2 = lat2 * toRad;
  var dLat = (lat2 - lat1) * toRad;
  var dLon = (lon2 - lon1) * toRad;
  var a =
    Math.sin(dLat / 2) * Math.sin(dLat / 2) +
    Math.cos(p1) * Math.cos(p2) * Math.sin(dLon / 2) * Math.sin(dLon / 2);
  return 2 * R * Math.atan2(Math.sqrt(a), Math.sqrt(Math.max(0, 1 - a)));
}

function defaultGeofenceConfig() {
  return {
    enabled: false,
    centerLat: null,
    centerLon: null,
    radiusM: 50,
    marginM: 2,
    mode: 'warn'
  };
}

function readGeofenceConfig() {
  try {
    var j = localStorage.getItem(LS_GEOFENCE);
    if (!j) return defaultGeofenceConfig();
    var o = JSON.parse(j);
    var d = defaultGeofenceConfig();
    if (typeof o.enabled === 'boolean') d.enabled = o.enabled;
    if (o.centerLat != null && !isNaN(Number(o.centerLat))) d.centerLat = Number(o.centerLat);
    if (o.centerLon != null && !isNaN(Number(o.centerLon))) d.centerLon = Number(o.centerLon);
    if (o.radiusM != null && !isNaN(Number(o.radiusM))) d.radiusM = Math.min(5000, Math.max(5, Number(o.radiusM)));
    if (o.marginM != null && !isNaN(Number(o.marginM))) d.marginM = Math.min(500, Math.max(0, Number(o.marginM)));
    if (o.mode === 'warn' || o.mode === 'soft' || o.mode === 'estop') d.mode = o.mode;
    return d;
  } catch (e) {
    return defaultGeofenceConfig();
  }
}

function writeGeofenceConfig(c) {
  try {
    localStorage.setItem(LS_GEOFENCE, JSON.stringify(c));
  } catch (e) {}
}

function applyGeofenceToForm() {
  var c = readGeofenceConfig();
  var en = document.getElementById('geofence-enabled');
  var la = document.getElementById('geofence-lat');
  var lo = document.getElementById('geofence-lon');
  var ra = document.getElementById('geofence-radius');
  var mar = document.getElementById('geofence-margin');
  var mo = document.getElementById('geofence-mode');
  if (en) en.checked = !!c.enabled;
  if (la) la.value = c.centerLat != null ? String(c.centerLat) : '';
  if (lo) lo.value = c.centerLon != null ? String(c.centerLon) : '';
  if (ra) {
    ra.value = String(c.radiusM);
    var ro = document.getElementById('geofence-radius-readout');
    if (ro) ro.textContent = c.radiusM + ' m';
  }
  if (mar) mar.value = String(c.marginM);
  if (mo) mo.value = c.mode;
}

function gatherGeofenceFromForm() {
  var c = readGeofenceConfig();
  var en = document.getElementById('geofence-enabled');
  var la = document.getElementById('geofence-lat');
  var lo = document.getElementById('geofence-lon');
  var ra = document.getElementById('geofence-radius');
  var mar = document.getElementById('geofence-margin');
  var mo = document.getElementById('geofence-mode');
  if (en) c.enabled = !!en.checked;
  if (la && la.value.trim() !== '') {
    var x = parseFloat(la.value);
    c.centerLat = isNaN(x) ? null : x;
  } else c.centerLat = null;
  if (lo && lo.value.trim() !== '') {
    var y = parseFloat(lo.value);
    c.centerLon = isNaN(y) ? null : y;
  } else c.centerLon = null;
  if (ra) c.radiusM = Math.min(5000, Math.max(5, parseInt(ra.value, 10) || 50));
  if (mar) c.marginM = Math.min(500, Math.max(0, parseInt(mar.value, 10) || 0));
  if (mo && (mo.value === 'warn' || mo.value === 'soft' || mo.value === 'estop')) c.mode = mo.value;
  return c;
}

function geofenceSave() {
  var c = gatherGeofenceFromForm();
  writeGeofenceConfig(c);
  showFeedbackToast(true, 'Geo-fence saved');
  _gfPrevInside = null;
  if (typeof fetchStatus === 'function') fetchStatus();
}

function geofenceUseRobotGps() {
  var auto = stateCache && stateCache.auto;
  if (!auto || !auto.gpsValid || auto.lat == null || auto.lon == null) {
    showFeedbackToast(false, 'No valid robot GPS yet');
    return;
  }
  var la = document.getElementById('geofence-lat');
  var lo = document.getElementById('geofence-lon');
  if (la) la.value = String(Number(auto.lat));
  if (lo) lo.value = String(Number(auto.lon));
  geofenceSave();
}

function geofenceResetDefaults() {
  writeGeofenceConfig(defaultGeofenceConfig());
  applyGeofenceToForm();
  _gfPrevInside = null;
  _gfEstopLatched = false;
  showFeedbackToast(true, 'Geo-fence reset to defaults');
  if (typeof fetchStatus === 'function') fetchStatus();
}

function setGeofencePill(text, kind) {
  var pill = document.getElementById('geofence-pill');
  if (pill) {
    pill.textContent = text;
    pill.className = 'geofence-pill' + (kind ? ' geofence-pill--' + kind : '');
  }
}

function setGeofenceDetail(line) {
  var det = document.getElementById('geofence-detail');
  if (det) det.textContent = line || '—';
}

function evaluateGeofence(auto) {
  var c = readGeofenceConfig();
  if (!document.getElementById('geofence-pill')) return;

  if (!c.enabled) {
    setGeofencePill('Off', '');
    setGeofenceDetail('Monitoring disabled.');
    return;
  }

  if (c.centerLat == null || c.centerLon == null || isNaN(c.centerLat) || isNaN(c.centerLon)) {
    setGeofencePill('Setup', 'warn');
    setGeofenceDetail('Set center latitude and longitude (or use robot GPS).');
    return;
  }

  if (!auto || !auto.gpsValid || auto.lat == null || auto.lon == null) {
    setGeofencePill('No GPS', 'warn');
    setGeofenceDetail('Waiting for valid GPS from robot…');
    return;
  }

  var dist = haversineMeters(c.centerLat, c.centerLon, Number(auto.lat), Number(auto.lon));
  var effR = Math.max(0, c.radiusM - c.marginM);
  var inside = dist <= effR;

  var distStr = dist < 1000 ? dist.toFixed(1) + ' m' : (dist / 1000).toFixed(2) + ' km';
  var innerStr =
    'Dist ' +
    distStr +
    ' · limit ' +
    effR +
    ' m' +
    (c.marginM > 0 ? ' (radius ' + c.radiusM + ' − margin ' + c.marginM + ')' : '');

  if (inside) {
    setGeofencePill('Inside', 'ok');
    setGeofenceDetail(innerStr);
    if (_gfPrevInside === false) {
      showFeedbackToast(true, 'Geo-fence: back inside zone');
      logActivity('Geo-fence: inside zone', '\u2708\uFE0F');
    }
    _gfEstopLatched = false;
    _gfPrevInside = true;
    return;
  }

  setGeofencePill('Outside', 'stop');
  setGeofenceDetail(innerStr);

  var breach = _gfPrevInside !== false;
  _gfPrevInside = false;

  if (!breach) return;

  if (c.mode === 'warn') {
    showFeedbackToast(false, 'Geo-fence: outside boundary');
  } else if (c.mode === 'soft') {
    showFeedbackToast(false, 'Geo-fence: outside boundary');
    logActivity('Geo-fence breach (outside ' + distStr + ')', '\u26A0\uFE0F');
  } else if (c.mode === 'estop' && !_gfEstopLatched) {
    _gfEstopLatched = true;
    showFeedbackToast(false, 'Geo-fence: E-STOP');
    logActivity('Geo-fence E-STOP (outside boundary)', '\u26D4');
    if (typeof doEStop === 'function') doEStop(false);
  }
}

function refreshGeofenceDisplay() {
  var auto = stateCache && stateCache.auto;
  evaluateGeofence(auto || {});
}

function initGeofencePanel() {
  if (_gfPanelBound) return;
  if (!document.getElementById('geofence-radius')) return;
  _gfPanelBound = true;
  applyGeofenceToForm();

  var ra = document.getElementById('geofence-radius');
  var ro = document.getElementById('geofence-radius-readout');
  if (ra && ro) {
    ra.addEventListener('input', function () {
      ro.textContent = ra.value + ' m';
    });
  }

  var saveDeb = null;
  function debSave() {
    clearTimeout(saveDeb);
    saveDeb = setTimeout(function () {
      writeGeofenceConfig(gatherGeofenceFromForm());
      _gfPrevInside = null;
    }, 320);
  }

  ['geofence-enabled', 'geofence-lat', 'geofence-lon', 'geofence-margin', 'geofence-mode'].forEach(function (id) {
    var el = document.getElementById(id);
    if (el) el.addEventListener('change', debSave);
  });
  if (ra) ra.addEventListener('change', debSave);
}

// ─── Status & API ───────────────────────────────────────────
async function fetchStatus() {
  try {
    const [wifi, battery, auto, imu, settings] = await Promise.all([
      fetch(api('/wifi/status')).then(r => r.json()).catch(() => ({})),
      fetch(api('/battery/status')).then(r => r.json()).catch(() => ({})),
      fetch(api('/api/autonomy')).then(r => r.json()).catch(() => ({})),
      fetch(api('/imu/status')).then(r => r.json()).catch(() => ({})),
      fetch(api('/settings')).then(r => r.json()).catch(() => ({}))
    ]);
    stateCache = { wifi, battery, auto, imu, settings };

    updateAiAutonomyPanel(auto);
    if (driveMode === 'ai' && currentVisiblePage === 'drive') {
      updateDriveAiPanel(auto);
    }

    // Connection badge
    const dot = document.getElementById('conn-dot'), lbl = document.getElementById('conn-label');
    if (dot) dot.className = wifi.state === 2 ? 'sta' : wifi.state === 1 ? 'pulse' : 'ap';
    if (lbl) lbl.textContent = wifi.state === 2 ? (wifi.sta_ip || 'Connected') : wifi.state === 1 ? 'Connecting…' : 'AP';

    // Home
    setById('home-battery', battery.voltage != null ? battery.voltage + ' V' : '-');
    setById('home-state', auto.state || '-');
    setById('home-emotion', auto.emotion || '-');
    const pct = battery.percent != null ? battery.percent : (battery.voltage != null ? Math.min(100, Math.max(0, (battery.voltage - 10.5) / 2.1 * 100)) : 80);
    const pctDock =
      battery.percent != null
        ? battery.percent
        : battery.voltage != null
          ? Math.min(100, Math.max(0, (battery.voltage - 10.5) / 2.1 * 100))
          : null;
    const bar = document.getElementById('home-battery-bar');
    if (bar) { bar.style.width = pct + '%'; bar.className = 'progress-fill' + (pct < 20 ? ' critical' : pct < 40 ? ' low' : ''); }

    setFaceMood(auto.emotion || (auto.enabled ? 'curious' : 'happy'));
    if (pct < 25) setTheme('low-battery');
    else if (auto.rthActive) setTheme('docking');
    else setTheme('');

    var stName = auto.state != null ? String(auto.state) : '';
    var thinkEl = document.getElementById('ai-thinking');
    var busyBrain =
      auto.enabled &&
      (auto.objectDetected ||
        /explor|invest|think|plan|avoid|nav|dock/i.test(stName) ||
        (auto.interest != null && auto.interest > 55));
    if (thinkEl) thinkEl.hidden = !busyBrain;
    var decision = '\u2014';
    if (auto.objectDetected) decision = 'Slowing — object ahead';
    else if (auto.rthActive) decision = 'Return home';
    else if (auto.enabled) decision = 'Autonomy: ' + (stName || 'active');
    else decision = 'Manual / idle';
    setById('ai-decision', decision);
    var confPct =
      auto.interest != null ? Math.min(100, Math.max(0, Math.round(Number(auto.interest)))) : null;
    setById('ai-confidence', confPct != null ? confPct + '%' : '\u2014');
    setById('ai-interest', auto.interest != null ? String(auto.interest) : '\u2014');

    // Network
    setById('net-ap', wifi.ap_ssid || 'WALL-E-Control');
    setById('net-sta', wifi.sta_ssid || 'Not connected');
    updateNetworkPageUplink(wifi);

    // Dock page — battery + autonomy (full dock FSM from refreshDockPanel when Dock tab open)
    setById('dock-stage', auto.rthActive ? 'Return home' : auto.state != null ? String(auto.state) : '—');
    setById('dock-rth', auto.rthActive ? 'Active' : 'Idle');
    setById('dock-voltage', battery.voltage != null ? battery.voltage + ' V' : '—');
    setById('dock-current', pctDock != null ? Math.round(pctDock) + '% SoC' : '—');
    var dbar0 = document.getElementById('dock-charge-bar');
    if (dbar0 && pctDock != null) {
      dbar0.style.width = pctDock + '%';
      dbar0.className = 'progress-fill' + (pctDock < 20 ? ' critical' : pctDock < 40 ? ' low' : '');
    }

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
    const prof = maxSpeed <= 128 ? 'low' : maxSpeed >= 255 ? 'high' : 'normal';
    if (sp) sp.value = prof;
    var sps = document.getElementById('set-speed-profile');
    if (sps) sps.value = prof;
    var readout = document.getElementById('set-max-speed-readout');
    if (readout) readout.textContent = String(maxSpeed);

    // Navigation
    setById('nav-home', auto.gpsValid ? 'Set' : 'Not set');
    setById('nav-current', auto.gpsValid ? (auto.lat + ', ' + auto.lon) : '-');
    if (window.NavWorldContext) NavWorldContext.refresh(wifi);
    if (window.NavMissionPanel) NavMissionPanel.syncFromState(stateCache);
    setById('nav-battery', battery.voltage != null ? battery.voltage.toFixed(2) + ' V' : '—');

    evaluateGeofence(auto);

    updateToastsFromState(stateCache);

    setById('stat-batt', battery.voltage != null ? battery.voltage.toFixed(2) + ' V' : '-');
    setById('stat-batt-sub', pct != null ? Math.round(pct) + '%' : '-');
    setById('stat-heading', imu.heading != null ? imu.heading + '\u00B0' : '-');
    setById('stat-rssi', wifi.rssi != null ? wifi.rssi + ' dBm' : '-');
    setById('stat-wifi-sub', wifi.sta_ssid || 'signal');
    var spAvg = (typeof tankLeft === 'number' && typeof tankRight === 'number')
      ? Math.round((Math.abs(tankLeft) + Math.abs(tankRight)) / 2) : 0;
    setById('stat-speed', String(spAvg));
    var wp = document.getElementById('w-power');
    if (wp) wp.textContent = battery.voltage != null ? battery.voltage.toFixed(2) + ' V' : '-';

    if (imu.heading != null) {
      _imuHist.push(imu.heading);
      if (_imuHist.length > 80) _imuHist.shift();
      drawSparkImu();
    }

    pushTele(_battHist, pct != null ? Number(pct) : null, _TELE_CAP);
    if (auto.sonar != null) pushTele(_sonarHist, Math.min(250, Math.max(0, Number(auto.sonar))), _TELE_CAP);
    drawSparkStrip('spark-batt', _battHist, '#34d399', 0, 100);
    drawSparkStrip('spark-sonar', _sonarHist, '#f5a623', 0, 200);

    if (window.LrosNavigation && typeof LrosNavigation.syncFromState === 'function') {
      LrosNavigation.syncFromState(stateCache);
    }
    updateNavMapHud();

    if (window.WalleConnection) WalleConnection.markHttpOk();

    if (currentVisiblePage === 'home') {
      fetch(api('/api/laser/status')).then(function (r) { return r.json(); }).then(refreshLaserPanel).catch(function () {});
    }
  } catch (_) {}
}

var _laserScanOn = false;

function refreshLaserPanel(j) {
  if (!j) return;
  setById('laser-status', j.on ? 'On' : 'Off');
  var btn = document.getElementById('laser-toggle-btn');
  if (btn) btn.textContent = j.on ? 'Laser OFF' : 'Laser ON';
  _laserScanOn = !!j.scan;
  var sb = document.getElementById('laser-scan-btn');
  if (sb) sb.textContent = _laserScanOn ? 'Stop scan' : 'Scan';
  var mood = document.getElementById('laser-mood');
  if (mood && j.mood != null) mood.value = String(j.mood);
}

function bindLaserControls() {
  var br = document.getElementById('laser-bright');
  if (br && !br._laserBound) {
    br._laserBound = true;
    br.addEventListener('change', function () {
      fetch(api('/api/laser/brightness?value=' + encodeURIComponent(br.value))).catch(function () {});
    });
  }
  var pan = document.getElementById('laser-pan');
  var tilt = document.getElementById('laser-tilt');
  if (!pan || !tilt || pan._laserSmoothBound) return;
  pan._laserSmoothBound = true;
  tilt._laserSmoothBound = true;
  var deb = null;
  function sched() {
    clearTimeout(deb);
    deb = setTimeout(function () {
      fetch(api('/api/laser/smooth?pan=' + encodeURIComponent(pan.value) + '&tilt=' + encodeURIComponent(tilt.value) + '&delay=25')).catch(function () {});
    }, 200);
  }
  pan.addEventListener('input', sched);
  tilt.addEventListener('input', sched);
}

function laserToggleOnOff() {
  var br = document.getElementById('laser-bright');
  var v = br ? parseInt(br.value, 10) : 0;
  fetch(api('/api/laser/status'))
    .then(function (r) { return r.json(); })
    .then(function (j) {
      if (j && j.on) return fetch(api('/api/laser/off'));
      var b = v > 0 ? v : 255;
      return fetch(api('/api/laser/brightness?value=' + b));
    })
    .then(function () { return fetch(api('/api/laser/status')); })
    .then(function (r) { return r.json(); })
    .then(refreshLaserPanel)
    .catch(function () {});
}

function laserApplyAim() {
  var p = document.getElementById('laser-pan');
  var t = document.getElementById('laser-tilt');
  if (!p || !t) return;
  fetch(api('/api/laser/set?pan=' + encodeURIComponent(p.value) + '&tilt=' + encodeURIComponent(t.value)))
    .then(function () { return fetch(api('/api/laser/status')); })
    .then(function (r) { return r.json(); })
    .then(refreshLaserPanel)
    .catch(function () {});
}

function laserPointFire() {
  var p = document.getElementById('laser-pan');
  var t = document.getElementById('laser-tilt');
  var ms = document.getElementById('laser-fire-ms');
  if (!p || !t) return;
  var time = ms ? parseInt(ms.value, 10) || 1000 : 1000;
  if (time > 10000) time = 10000;
  if (time < 50) time = 50;
  fetch(api('/api/laser/fire?pan=' + encodeURIComponent(p.value) + '&tilt=' + encodeURIComponent(t.value) + '&time=' + time)).catch(function () {});
}

function laserScanToggle() {
  var next = !_laserScanOn;
  fetch(api('/api/laser/scan?enable=' + (next ? '1' : '0')))
    .then(function () { return fetch(api('/api/laser/status')); })
    .then(function (r) { return r.json(); })
    .then(refreshLaserPanel)
    .catch(function () {});
}

function laserMoodApply() {
  var m = document.getElementById('laser-mood');
  if (!m) return;
  fetch(api('/api/laser/mood?mood=' + encodeURIComponent(m.value))).catch(function () {});
}

function setById(id, v) {
  const el = document.getElementById(id);
  if (el && el.textContent !== undefined) el.textContent = v;
}

/** Network page — hero orbit, chips, RSSI bar (IDs in #page-network). */
function updateNetworkPageUplink(wifi) {
  var pg = document.getElementById('page-network');
  if (!pg || !wifi) return;
  var st = wifi.state != null ? Number(wifi.state) : -1;
  var mode = 'ap';
  var title = 'Access point';
  var sub = 'Broadcasting operator SSID — join to configure STA';
  if (st === 1) {
    mode = 'connecting';
    title = 'Handshaking…';
    sub = 'STA association in progress…';
  } else if (st === 2) {
    mode = 'linked';
    title = 'Station linked';
    sub = 'Home network active — AP + STA online';
  } else if (st === 3) {
    mode = 'failed';
    title = 'Link failed';
    sub = 'STA could not connect — AP still available for retry';
  }
  pg.setAttribute('data-net-state', mode);
  var orb = document.getElementById('net-hero-orbit');
  if (orb) orb.setAttribute('data-link', mode);
  setById('net-hero-state', title);
  setById('net-hero-sub', sub);
  var chip = document.getElementById('net-sta-chip');
  if (chip) {
    chip.textContent = st === 2 ? 'Linked' : st === 1 ? '…' : st === 3 ? 'Fault' : 'STA';
    chip.className =
      'net-chip' +
      (st === 2 ? ' net-chip--live' : st === 3 ? ' net-chip--warn' : '');
  }
  var topo = document.getElementById('net-topo-chip');
  if (topo) topo.textContent = st === 2 ? 'Online' : 'Local';

  var rssi = wifi.rssi;
  var rssiEl = document.getElementById('net-rssi-hero');
  var bar = document.getElementById('net-rssi-hero-bar');
  var cap = document.getElementById('net-rssi-caption');
  var track = document.getElementById('net-rssi-track');
  if (rssi != null && rssi !== '' && st === 2) {
    var n = Number(rssi);
    if (rssiEl) rssiEl.textContent = n + ' dBm';
    var pct = Math.min(100, Math.max(0, 2 * (n + 100)));
    if (bar) bar.style.width = pct + '%';
    if (cap) cap.textContent = 'Signal quality (approximate)';
    if (track) track.setAttribute('aria-valuenow', String(n));
  } else {
    if (rssiEl) rssiEl.textContent = '—';
    if (bar) bar.style.width = '0%';
    if (cap) {
      cap.textContent =
        st === 2 ? 'RSSI pending…' : 'No STA link — RSSI shown when connected';
    }
    if (track) track.setAttribute('aria-valuenow', '-100');
  }

  setById('net-ap-ip', wifi.ap_ip && String(wifi.ap_ip).length ? wifi.ap_ip : '—');
  setById('net-ap-clients', wifi.ap_clients != null ? String(wifi.ap_clients) : '—');
  var staIp = wifi.sta_ip && String(wifi.sta_ip).length ? wifi.sta_ip : '—';
  setById('net-sta-ip', staIp);
  setById('net-hero-ip', staIp);
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
    const nets = await fetch(api('/wifi/scan')).then(r => r.json());
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
  const btn = document.getElementById('net-connect-btn');
  if (!ssid) { showToast('\u26A0', 'Enter SSID'); return; }
  if (btn) btn.disabled = true;
  try {
    await fetch(api('/wifi/connect?ssid=' + encodeURIComponent(ssid) + '&password=' + encodeURIComponent(pass)));
    showToast('\uD83D\uDCE1', 'Connecting...');
    setTimeout(fetchStatus, 2000);
  } catch (e) { showToast('\u26A0', 'Failed'); }
  if (btn) btn.disabled = false;
}

// ─── Auto dock, Set home ────────────────────────────────────
function triggerAutoDock() {
  showToast('\uD83D\uDCE1', 'Docking...');
  apiCall('/api/dock/start');
  switchTab('docking');
}

function setHome() {
  apiCall('/api/autonomy/set_home');
  showToast('\uD83D\uCCCD', 'Home set');
}

// ─── Vision (camera node · VisionPacket telemetry) ─────────
var _visionRefreshTimer = null;
var _visionStatusTimer = null;
var _visionSnapMs = 900;

function visionOnImgError() {
  var img = document.getElementById('fpv-img');
  var ph = document.getElementById('fpv-placeholder');
  if (img) img.style.display = 'none';
  if (ph) ph.style.display = 'flex';
}

function setVisionRefreshMs(ms) {
  var n = parseInt(ms, 10);
  if (!isNaN(n) && n >= 250) _visionSnapMs = n;
}

function visionZoneName(z) {
  var m = { 0: 'LEFT', 1: 'CENTER', 2: 'RIGHT', 3: '—' };
  return m[z] != null ? m[z] : String(z);
}
function visionDistName(d) {
  var m = { 0: 'FAR', 1: 'MID', 2: 'CLOSE' };
  return m[d] != null ? m[d] : String(d);
}
function visionClassName(c) {
  var m = { 0: 'UNKNOWN', 1: 'HUMAN\u2011LIKE', 2: 'OBJECT', 3: 'LIGHT' };
  return m[c] != null ? m[c] : String(c);
}
function visionEventName(e) {
  var m = { 0: '—', 1: 'TARGET\u00A0DETECTED', 2: 'APPROACHING', 3: 'CENTERED', 4: 'LOST' };
  return m[e] != null ? m[e] : 'EV\u00A0' + e;
}
function visionColourName(id) {
  var m = { 0: '—', 1: 'SKIN', 2: 'RED', 3: 'GREEN', 4: 'BLUE', 5: 'CUSTOM' };
  return m[id] != null ? m[id] : String(id);
}

function applyVisionTelemetry(v) {
  if (!v || typeof v !== 'object') return;
  var el = function (id, txt) {
    var n = document.getElementById(id);
    if (n) n.textContent = txt == null ? '\u2014' : String(txt);
  };
  var connected = !!v.connected;
  var pill = document.getElementById('vision-link-pill');
  if (pill) {
    pill.textContent = connected ? 'LINK \u00B7 OK' : 'LINK \u00B7 DOWN';
    pill.classList.toggle('ok', connected);
    pill.classList.toggle('bad', !connected);
  }
  el('vision-behave-pill', 'BRAIN \u00B7 ' + (v.state || '\u2014'));
  var age = v.ageMs != null ? v.ageMs : '\u2014';
  el('vision-age-pill', '\u0394 ' + age + ' ms');

  el('vision-m-motion', v.motion ? 'YES' : 'no');
  el('vision-m-int', v.motionIntensity != null ? v.motionIntensity : '\u2014');
  el('vision-m-xy', (v.targetX != null && v.targetY != null) ? (v.targetX + ', ' + v.targetY) : '\u2014');
  el('vision-m-zone', visionZoneName(v.zone));
  el('vision-m-objsz', v.objectSize != null ? v.objectSize : '\u2014');
  el('vision-m-bbox', (v.bboxW != null && v.bboxH != null) ? (v.bboxW + ' \u00D7 ' + v.bboxH) : '\u2014');

  el('vision-m-objclass', v.objectClassStr || ('#' + (v.objectClass != null ? v.objectClass : '—')));
  el('vision-m-colour', visionColourName(v.colourId));
  el('vision-m-colourconf', v.colourConfidence != null ? (v.colourConfidence + ' / 100') : '\u2014');
  el('vision-m-blobdet', v.blobDetected ? 'YES' : 'no');
  el('vision-m-blobxy', (v.blobX != null && v.blobY != null) ? (v.blobX + ', ' + v.blobY) : '\u2014');
  el('vision-m-classify', visionClassName(v.classification));

  el('vision-m-lock', v.targetLocked ? 'LOCKED' : 'search');
  el('vision-m-lockconf', v.targetLockConfidence != null ? v.targetLockConfidence : '\u2014');
  el('vision-m-vevent', visionEventName(v.visionEvent));
  el('vision-m-ip', v.visionNodeIp != null ? v.visionNodeIp : '\u2014');
  el('vision-m-enabled', v.enabled ? 'ON' : 'OFF');

  el('vision-event-big', visionEventName(v.visionEvent));
  el('vision-class-big', visionClassName(v.classification));
  el('vision-dist-big', visionDistName(v.distanceBand));

  var colLine = visionColourName(v.colourId);
  if (v.colourConfidence != null) colLine += ' \u00B7 ' + v.colourConfidence + '%';
  el('vision-colour-line', colLine);

  var blobLine = v.blobDetected ? ('size ' + (v.blobSize != null ? v.blobSize : '?')) : 'none';
  el('vision-blob-line', blobLine);

  var scene = [];
  if (v.motion) scene.push('motion');
  if (v.blobDetected) scene.push('blob');
  if (v.targetLocked) scene.push('lock');
  el('vision-scene', scene.length ? scene.join(' \u00B7 ') : (connected ? 'Idle \u00B7 link OK' : 'No vision packets'));

  el('vision-hud-frameid', 'FR ' + (v.frameID != null ? v.frameID : '\u2014'));
  el('vision-hud-zone-hud', 'ZN ' + visionZoneName(v.zone));
  el('vision-hud-lock-hud', v.targetLocked ? 'LK OK' : 'LK —');

  var ret = document.getElementById('vision-reticle');
  if (ret) {
    var tx = typeof v.targetX === 'number' ? v.targetX : 0;
    var ty = typeof v.targetY === 'number' ? v.targetY : 0;
    var px = Math.max(4, Math.min(96, (tx / 160) * 100));
    var py = Math.max(4, Math.min(96, (ty / 120) * 100));
    ret.style.left = px + '%';
    ret.style.top = py + '%';
    var showRet = !!(v.motion || v.targetLocked || v.blobDetected);
    ret.classList.toggle('on', showRet);
  }
}

function refreshVisionPanel(showToastOnErr) {
  fetch(api('/api/vision/status'))
    .then(function (r) { return r.json(); })
    .then(function (j) { applyVisionTelemetry(j); })
    .catch(function () {
      if (showToastOnErr) showToast('\u26A0', 'Vision status failed');
    });
}

function initVision() {
  if (_visionRefreshTimer) {
    clearInterval(_visionRefreshTimer);
    _visionRefreshTimer = null;
  }
  if (_visionStatusTimer) {
    clearInterval(_visionStatusTimer);
    _visionStatusTimer = null;
  }
  var img = document.getElementById('fpv-img');
  var ph = document.getElementById('fpv-placeholder');
  var rs = document.getElementById('vision-refresh-ms');
  if (rs && rs.value) setVisionRefreshMs(rs.value);

  function loadSnap() {
    if (!img || !ph) return;
    fetch(api('/api/vision/snapshot_url'))
      .then(function (r) { return r.text(); })
      .then(function (url) {
        url = (url || '').trim();
        if (!url) {
          img.style.display = 'none';
          ph.style.display = 'flex';
          return;
        }
        img.style.display = 'block';
        img.src = url + (url.indexOf('?') >= 0 ? '&' : '?') + 't=' + Date.now();
        ph.style.display = 'none';
      })
      .catch(function () { visionOnImgError(); });
  }
  if (img) {
    img.onload = function () { img.style.display = 'block'; if (ph) ph.style.display = 'none'; };
    img.onerror = visionOnImgError;
  }
  loadSnap();
  refreshVisionPanel(false);
  _visionRefreshTimer = setInterval(loadSnap, _visionSnapMs);
  _visionStatusTimer = setInterval(function () { refreshVisionPanel(false); }, 500);
}

function snapshot() {
  fetch(api('/api/vision/snapshot_url'))
    .then(function (r) { return r.text(); })
    .then(function (url) {
      url = (url || '').trim();
      if (!url) { showToast('\u26A0', 'No vision node'); return; }
      window.open(url, '_blank');
    })
    .catch(function () { showToast('\u26A0', 'Snapshot error'); });
}
function toggleNightMode() { showToast('\uD83C\uDF19', 'Night mode (not wired on base)'); }

// ─── Audio ──────────────────────────────────────────────────
/** Map soundboard names to DFPlayer-style track ids (1–255); firmware requires numeric id */
function soundIdFromUi(id) {
  if (typeof id === 'number' && !isNaN(id)) return id;
  var map = { beep: 1, curious: 2, happy: 3, sad: 4 };
  var s = String(id).toLowerCase();
  if (map[s] != null) return map[s];
  var n = parseInt(s, 10);
  return isNaN(n) ? 1 : n;
}
function playSound(id) {
  var tid = soundIdFromUi(id);
  showToast('\uD83D\uDD0A', String(id));
  apiCall('/api/audio/play?id=' + tid);
}

function setVolume(v) {
  document.getElementById('audio-vol-val').textContent = v;
  apiCall('/api/audio/volume?value=' + v);
}

// ─── AI ─────────────────────────────────────────────────────
/** WALLE_EMOTION_* ids from walle_emotion_pose.h (firmware /api/emotion/set?id=) */
function behaviourToEmotionId(mode) {
  var m = String(mode || '').toLowerCase();
  if (m === 'curious') return 1;
  if (m === 'happy') return 2;
  if (m === 'sad') return 3;
  if (m === 'shy' || m === 'scared') return 4;
  if (m === 'tired') return 5;
  if (m === 'excited') return 2;
  return 0;
}
function setBehaviourMode(mode) {
  apiCall('/api/emotion/set?id=' + behaviourToEmotionId(mode));
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
  fetch(api('/api/files/list')).then(r => r.json()).then(d => {
    const list = document.getElementById('file-list');
    if (list) list.innerHTML = (d.files || []).length ? d.files.map(f => '<div class="log-item">' + f + '</div>').join('') : '<div class="log-item value dim">No files</div>';
  }).catch(() => {});
}

function pushTele(arr, v, cap) {
  if (v == null || isNaN(v)) return;
  arr.push(v);
  while (arr.length > cap) arr.shift();
}

function drawSparkStrip(canvasId, data, stroke, vmin, vmax) {
  var c = document.getElementById(canvasId);
  if (!c || !c.getContext || data.length < 2) return;
  var ctx = c.getContext('2d');
  var w = c.width;
  var h = c.height;
  ctx.fillStyle = '#0a0c10';
  ctx.fillRect(0, 0, w, h);
  ctx.strokeStyle = stroke;
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  var n = data.length;
  var span = vmax - vmin || 1;
  for (var i = 0; i < n; i++) {
    var x = (i / (n - 1)) * (w - 4) + 2;
    var y = h - 2 - ((data[i] - vmin) / span) * (h - 4);
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

function drawSparkImu() {
  var c = document.getElementById('spark-imu');
  if (!c || !c.getContext) return;
  var ctx = c.getContext('2d');
  var w = c.width, h = c.height;
  ctx.fillStyle = '#0a0c10';
  ctx.fillRect(0, 0, w, h);
  var n = _imuHist.length;
  if (n < 2) return;
  ctx.strokeStyle = '#34d399';
  ctx.lineWidth = 1.5;
  ctx.beginPath();
  for (var i = 0; i < n; i++) {
    var x = (i / (n - 1)) * w;
    var ang = ((_imuHist[i] % 360) + 360) % 360;
    var y = h - (ang / 360) * (h - 4) - 2;
    if (i === 0) ctx.moveTo(x, y);
    else ctx.lineTo(x, y);
  }
  ctx.stroke();
}

function pushActivity(text, icon) {
  var feed = document.getElementById('activity-feed');
  if (!feed) return;
  var row = document.createElement('div');
  row.className = 'feed-row';
  var t = new Date();
  row.innerHTML = '<span class="feed-t">' + t.toLocaleTimeString() + '</span><span class="feed-m">' +
    (icon ? icon + ' ' : '') + String(text) + '</span>';
  feed.insertBefore(row, feed.firstChild);
  while (feed.children.length > 30) feed.removeChild(feed.lastChild);
}

function readWidgetOrder() {
  try {
    var j = localStorage.getItem(LS_WIDGET_ORDER);
    return j ? JSON.parse(j) : [];
  } catch (e) {
    return [];
  }
}
function saveWidgetOrder(grid) {
  try {
    var ids = Array.from(grid.querySelectorAll('.widget-tile')).map(function (t) {
      return t.dataset.widget || '';
    });
    localStorage.setItem(LS_WIDGET_ORDER, JSON.stringify(ids));
  } catch (e) {}
}
function applyWidgetOrder() {
  var grid = document.getElementById('widget-grid');
  if (!grid) return;
  var order = readWidgetOrder();
  if (!order.length) return;
  var map = {};
  Array.from(grid.querySelectorAll('.widget-tile')).forEach(function (t) {
    if (t.dataset.widget) map[t.dataset.widget] = t;
  });
  order.forEach(function (id) {
    if (map[id]) grid.appendChild(map[id]);
  });
}

function initWidgetDrag() {
  var grid = document.getElementById('widget-grid');
  if (!grid) return;
  applyWidgetOrder();
  var dragEl = null;
  grid.addEventListener('dragstart', function (e) {
    var t = e.target.closest('.widget-tile');
    if (!t) return;
    dragEl = t;
    e.dataTransfer.effectAllowed = 'move';
    t.classList.add('dragging');
  });
  grid.addEventListener('dragend', function (e) {
    var t = e.target.closest('.widget-tile');
    if (t) t.classList.remove('dragging');
    dragEl = null;
  });
  grid.addEventListener('dragover', function (e) {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';
  });
  grid.addEventListener('drop', function (e) {
    e.preventDefault();
    var target = e.target.closest('.widget-tile');
    if (!dragEl || !target || dragEl === target) return;
    var rect = target.getBoundingClientRect();
    var before = e.clientY < rect.top + rect.height / 2;
    if (before) grid.insertBefore(dragEl, target);
    else grid.insertBefore(dragEl, target.nextSibling);
    dragEl = null;
    saveWidgetOrder(grid);
  });
}

function ensureCardId(card) {
  if (card.id) return card.id;
  var page = card.closest('.page');
  var pid = page && page.id ? page.id : 'orphan';
  var idx = 0;
  if (page) {
    Array.from(page.querySelectorAll('.card')).forEach(function (c, i) {
      if (c === card) idx = i;
    });
  }
  card.id = 'walle-' + pid + '-c' + idx;
  return card.id;
}

function ensureMaxBackdrop() {
  var el = document.getElementById('card-max-backdrop');
  if (el) return el;
  el = document.createElement('div');
  el.id = 'card-max-backdrop';
  el.className = 'card-max-backdrop';
  el.hidden = true;
  el.setAttribute('aria-hidden', 'true');
  document.body.appendChild(el);
  el.addEventListener('click', function () {
    closeMaxCard();
  });
  return el;
}

function closeMaxCard() {
  document.querySelectorAll('.card--maximized').forEach(function (c) {
    c.classList.remove('card--maximized');
    if (c._walleParent) {
      c._walleParent.appendChild(c);
      c._walleParent = null;
    }
  });
  var b = document.getElementById('card-max-backdrop');
  if (b) {
    b.hidden = true;
    b.setAttribute('aria-hidden', 'true');
  }
}

function toggleMaxCard(card) {
  if (card.classList.contains('card--maximized')) {
    closeMaxCard();
    return;
  }
  closeMaxCard();
  card.classList.add('card--maximized');
  if (!card._walleParent) card._walleParent = card.parentNode;
  document.body.appendChild(card);
  var b = ensureMaxBackdrop();
  b.hidden = false;
  b.setAttribute('aria-hidden', 'false');
}

function applyDismissedFromStorage() {
  var ids = readDismissedCards();
  ids.forEach(function (id) {
    var c = document.getElementById(id);
    if (c) {
      c.classList.add('card--dismissed');
      c.setAttribute('hidden', '');
    }
  });
}

function dismissCard(card) {
  var id = ensureCardId(card);
  closeMaxCard();
  card.classList.add('card--dismissed');
  card.setAttribute('hidden', '');
  var ids = readDismissedCards();
  if (ids.indexOf(id) < 0) ids.push(id);
  writeDismissedCards(ids);
}

function initCardChrome() {
  if (document.body.dataset.walleCardChrome) return;
  document.body.dataset.walleCardChrome = '1';
  ensureMaxBackdrop();
  document.querySelectorAll('#app .card:not(.card--no-chrome)').forEach(function (card) {
    if (card.closest('#cyd-landing')) return;
    var header = card.querySelector(':scope > .card-header');
    if (!header || header.querySelector('.card-win-actions')) return;
    ensureCardId(card);
    var main = document.createElement('div');
    main.className = 'card-header-main';
    while (header.firstChild) main.appendChild(header.firstChild);
    header.appendChild(main);
    var actions = document.createElement('div');
    actions.className = 'card-win-actions';
    actions.innerHTML =
      '<button type="button" class="card-win-btn" data-action="min" title="Minimize" aria-label="Minimize">\u2212</button>' +
      '<button type="button" class="card-win-btn" data-action="max" title="Maximize" aria-label="Maximize">\u25a1</button>' +
      '<button type="button" class="card-win-btn card-win-btn--close" data-action="close" title="Hide panel" aria-label="Hide panel">\u00d7</button>';
    header.appendChild(actions);
    card.classList.add('card--has-chrome');
  });
  applyDismissedFromStorage();
  document.addEventListener('click', function (e) {
    var btn = e.target.closest('.card-win-btn');
    if (!btn) return;
    var card = btn.closest('.card');
    if (!card) return;
    e.preventDefault();
    var act = btn.dataset.action;
    if (act === 'min') {
      if (card.classList.contains('card--maximized')) closeMaxCard();
      card.classList.toggle('card--minimized');
    } else if (act === 'max') {
      if (card.classList.contains('card--minimized')) card.classList.remove('card--minimized');
      toggleMaxCard(card);
    } else if (act === 'close') {
      dismissCard(card);
    }
  });
  document.addEventListener('keydown', function (e) {
    if (e.key === 'Escape') closeMaxCard();
  });
}

function readHomeCardOrder() {
  try {
    var j = localStorage.getItem(LS_HOME_CARD_ORDER);
    return j ? JSON.parse(j) : [];
  } catch (e) {
    return [];
  }
}
function saveHomeCardOrder() {
  var page = document.getElementById('page-home');
  if (!page) return;
  try {
    var ids = Array.from(page.querySelectorAll(':scope > .card')).map(function (c) {
      return c.id || '';
    });
    localStorage.setItem(LS_HOME_CARD_ORDER, JSON.stringify(ids));
  } catch (e) {}
}
function applyHomeCardOrder() {
  var page = document.getElementById('page-home');
  if (!page) return;
  var order = readHomeCardOrder();
  if (!order.length) return;
  order.forEach(function (id) {
    if (!id) return;
    var el = document.getElementById(id);
    if (el && el.parentNode === page) page.appendChild(el);
  });
}

function initHomeCardReorder() {
  var page = document.getElementById('page-home');
  if (!page || page.dataset.walleHomeReorder) return;
  page.dataset.walleHomeReorder = '1';
  applyHomeCardOrder();
  var dragCard = null;
  page.addEventListener('dragstart', function (e) {
    var h = e.target.closest('.card-drag-handle');
    if (!h) return;
    dragCard = h.closest('.card');
    if (!dragCard || dragCard.parentNode !== page) return;
    e.dataTransfer.effectAllowed = 'move';
    e.dataTransfer.setData('text/plain', dragCard.id);
    dragCard.classList.add('card--dragging');
  });
  page.addEventListener('dragend', function () {
    if (dragCard) dragCard.classList.remove('card--dragging');
    dragCard = null;
  });
  page.addEventListener('dragover', function (e) {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';
  });
  page.addEventListener('drop', function (e) {
    e.preventDefault();
    if (!dragCard) return;
    var target = e.target.closest('.card');
    if (!target || target === dragCard || target.parentNode !== page) return;
    var rect = target.getBoundingClientRect();
    var before = e.clientY < rect.top + rect.height / 2;
    if (before) page.insertBefore(dragCard, target);
    else page.insertBefore(dragCard, target.nextSibling);
    dragCard = null;
    saveHomeCardOrder();
  });
  Array.from(page.querySelectorAll(':scope > .card')).forEach(function (card) {
    if (card.querySelector('.card-drag-handle')) return;
    var header = card.querySelector(':scope > .card-header');
    if (!header) return;
    var h = document.createElement('span');
    h.className = 'card-drag-handle';
    h.setAttribute('draggable', 'true');
    h.setAttribute('title', 'Drag to reorder (dashboard)');
    h.setAttribute('aria-grabbed', 'false');
    h.textContent = '::';
    header.insertBefore(h, header.firstChild);
  });
}

function setFpvOpacity(v) {
  var img = document.getElementById('fpv-img');
  if (img) img.style.opacity = v;
}

function setFpvScale(v) {
  var img = document.getElementById('fpv-img');
  if (img) img.style.transform = 'scale(' + v + ')';
}

function sendVoiceCmd() {
  var i = document.getElementById('voice-cmd');
  if (!i || !i.value.trim()) return;
  var text = i.value.trim();
  if (window.WalleConnection && WalleConnection.send({ cmd: 'voice', text: text })) {
    pushActivity('Voice: ' + text, '\uD83C\uDFA4');
    i.value = '';
    return;
  }
  apiCall('/api/voice/command?text=' + encodeURIComponent(text));
  pushActivity('Voice: ' + text, '\uD83C\uDFA4');
  i.value = '';
}

function sendAiChat() {
  var i = document.getElementById('ai-input');
  if (!i || !i.value.trim()) return;
  var text = i.value.trim();
  var log = document.getElementById('ai-chat-log');
  if (log) {
    var row = document.createElement('div');
    row.className = 'ai-chat-row user';
    row.textContent = 'You: ' + text;
    log.appendChild(row);
  }
  i.value = '';
  if (window.WalleConnection && WalleConnection.send({ cmd: 'ai', text: text })) return;
  fetch(api('/api/ai/chat?text=' + encodeURIComponent(text)))
    .then(function (r) { return r.text(); })
    .then(function (t) {
      if (!log) return;
      var row2 = document.createElement('div');
      row2.className = 'ai-chat-row bot';
      row2.textContent = 'WALL-E: ' + t.slice(0, 400);
      log.appendChild(row2);
      log.scrollTop = log.scrollHeight;
    })
    .catch(function () {
      if (!log) return;
      var row2 = document.createElement('div');
      row2.className = 'ai-chat-row bot';
      row2.textContent = 'WALL-E: (no response — check API or WebSocket)';
      log.appendChild(row2);
      log.scrollTop = log.scrollHeight;
    });
}

function queueMission() {
  var typeEl = document.getElementById('mission-type');
  var repEl = document.getElementById('mission-repeat');
  var t = typeEl ? typeEl.value : 'patrol';
  var r = repEl ? repEl.value : 'once';
  missionQueue.push({ type: t, repeat: r, t: Date.now() });
  var el = document.getElementById('mission-queue');
  if (el) {
    el.innerHTML = missionQueue.map(function (m, i) {
      return '<div class="log-item">' + (i + 1) + '. ' + m.type + ' \u2014 ' + m.repeat + '</div>';
    }).join('');
  }
  pushActivity('Mission queued: ' + t + ' (' + r + ')', '\uD83C\uDFAF');
}

function filterLogs() {
  var cat = document.getElementById('log-cat');
  var sev = document.getElementById('log-sev');
  var cv = cat ? cat.value : 'all';
  var sv = sev ? sev.value : 'all';
  document.querySelectorAll('#log-list .log-item').forEach(function (el) {
    var c = el.getAttribute('data-cat') || 'system';
    var s = el.getAttribute('data-sev') || 'info';
    var ok = (cv === 'all' || c === cv) && (sv === 'all' || s === sv);
    el.style.display = ok ? '' : 'none';
  });
}

function exportLogs() {
  var lines = [];
  document.querySelectorAll('#log-list .log-item').forEach(function (el) {
    if (el.style.display === 'none') return;
    lines.push(el.textContent || '');
  });
  var blob = new Blob([lines.join('\n')], { type: 'text/plain;charset=utf-8' });
  var a = document.createElement('a');
  a.href = URL.createObjectURL(blob);
  a.download = 'walle-logs-' + Date.now() + '.txt';
  a.click();
  setTimeout(function () { URL.revokeObjectURL(a.href); }, 2000);
}

function saveWsUrl() {
  var inp = document.getElementById('ws-url-input');
  if (!inp || !window.WalleConnection) return;
  WalleConnection.setWsUrl(inp.value.trim());
  WalleConnection.connect();
  var w = document.getElementById('set-ws-url');
  if (w) w.value = inp.value.trim();
}

function loadConnectionSettings() {
  var b = document.getElementById('set-base-url');
  var w = document.getElementById('set-ws-url');
  var w2 = document.getElementById('ws-url-input');
  if (b) b.value = getBaseUrl();
  if (w && window.WalleConnection) w.value = WalleConnection.getWsUrl();
  if (w2 && window.WalleConnection) w2.value = WalleConnection.getWsUrl();
  loadUiPreferencesToForm();
  var readout = document.getElementById('set-max-speed-readout');
  if (readout && typeof maxSpeed === 'number') readout.textContent = String(maxSpeed);
}

function applySettings() {
  var b = document.getElementById('set-base-url');
  var w = document.getElementById('set-ws-url');
  try {
    if (b) localStorage.setItem('walle_base_url', b.value.trim());
    if (w && window.WalleConnection) WalleConnection.setWsUrl(w.value.trim());
  } catch (e) {}
  if (window.WalleConnection) WalleConnection.connect();
  fetchStatus();
  initVision();
  if (document.getElementById('ws-url-input') && w) document.getElementById('ws-url-input').value = w.value.trim();
}

function toggleProxMute() {
  if (!window.ProximityAlert) return;
  ProximityAlert.setMuted(!ProximityAlert.isMuted());
}

function navMissionStart() {
  if (window.NavMissionPanel && NavMissionPanel.start) NavMissionPanel.start();
  else {
    pushActivity('Navigation: start route', '\u2693');
    showToast('\u2693', 'Route start — use Send route when firmware exposes /api/navigation');
  }
}

function navMissionPause() {
  if (window.NavMissionPanel && NavMissionPanel.pause) NavMissionPanel.pause();
  else {
    pushActivity('Navigation: paused', '\u23F8');
    showToast('\u23F8', 'Pause (API not implemented)');
  }
}

function navMissionResume() {
  if (window.NavMissionPanel && NavMissionPanel.resume) NavMissionPanel.resume();
}

function navMissionAbort() {
  if (window.NavMissionPanel && NavMissionPanel.abort) NavMissionPanel.abort();
}

function devConsoleSim() {
  var c = document.getElementById('dev-console');
  if (!c) return;
  var line = document.createElement('div');
  var extra = stateCache.wifi && stateCache.wifi.rssi != null ? 'RSSI ' + stateCache.wifi.rssi + ' dBm' : 'heartbeat';
  line.textContent = '[' + new Date().toISOString() + '] ' + extra;
  c.appendChild(line);
  c.scrollTop = c.scrollHeight;
}

// ─── Developer ──────────────────────────────────────────────
function devFetch() {
  const inp = document.getElementById('dev-cmd');
  if (!inp) return;
  var u = inp.value.trim();
  if (!u) return;
  var url = /^https?:\/\//i.test(u) ? u : api(u.charAt(0) === '/' ? u : '/' + u);
  fetch(url).then(r => r.text()).then(t => showToast('\u2713', t.slice(0,50))).catch(e => showToast('\u26A0', String(e)));
}

// ─── Cinematic CYD landing (first paint) ───────────────────
var _lrosIntroFinished = false;

function prefersReducedMotionLanding() {
  try {
    return window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches;
  } catch (e) {
    return false;
  }
}

function finishLanding() {
  if (_lrosIntroFinished) return;
  _lrosIntroFinished = true;
  try {
    sessionStorage.setItem('lros_intro_done', '1');
  } catch (e) {}
  var land = document.getElementById('cyd-landing');
  document.body.classList.add('lros-app-revealed');
  if (land) {
    land.classList.add('cyd-landing--exit');
    land.setAttribute('aria-hidden', 'true');
    setTimeout(function () {
      land.setAttribute('hidden', '');
      land.style.display = 'none';
    }, prefersReducedMotionLanding() ? 200 : 900);
  }
  initAll();
}

function bootLros() {
  var force =
    typeof location !== 'undefined' &&
    /[?&]intro=1(?:&|$)/.test(location.search || '');
  var noIntro =
    typeof location !== 'undefined' &&
    /[?&]nointro=1(?:&|$)/.test(location.search || '');
  var skipIntro = false;
  try {
    skipIntro = (sessionStorage.getItem('lros_intro_done') === '1' && !force) || noIntro;
  } catch (e) {
    if (noIntro) skipIntro = true;
  }
  if (skipIntro) {
    document.body.classList.add('lros-app-revealed');
    _lrosIntroFinished = true;
    initAll();
    return;
  }
  var land = document.getElementById('cyd-landing');
  if (!land) {
    document.body.classList.add('lros-app-revealed');
    _lrosIntroFinished = true;
    initAll();
    return;
  }
  land.removeAttribute('hidden');
  land.style.display = '';
  land.setAttribute('aria-hidden', 'false');
  if (prefersReducedMotionLanding()) {
    land.classList.add('cyd-landing--reduced');
  }
  requestAnimationFrame(function () {
    land.classList.add('cyd-landing--animate');
  });
  var enter = document.getElementById('cyd-landing-enter');
  var skip = document.getElementById('cyd-landing-skip');
  function onEnter() {
    finishLanding();
  }
  if (enter) enter.addEventListener('click', onEnter);
  if (skip) skip.addEventListener('click', onEnter);
  if (enter) enter.focus();
  var autoMs = prefersReducedMotionLanding() ? 400 : 6200;
  setTimeout(function () {
    if (!_lrosIntroFinished) finishLanding();
  }, autoMs);
}

// ─── Init ───────────────────────────────────────────────────
function initAll() {
  applyUiPreferencesFromStorage();
  loadConnectionSettings();
  if (window.WalleConnection) {
    WalleConnection.connect();
    var _wsFetchThrottle = 0;
    WalleConnection.onMessage(function () {
      var n = Date.now();
      if (n - _wsFetchThrottle < 450) return;
      _wsFetchThrottle = n;
      if (typeof fetchStatus === 'function') fetchStatus();
    });
  }
  initEstopHold();
  initJoystick();
  initHeadPad();
  initTankSliders();
  initWidgetDrag();
  initCardChrome();
  initHomeCardReorder();
  initGeofencePanel();
  if (window.ProximityAlert) ProximityAlert.setMuted(ProximityAlert.isMuted());
  fetchStatus();
  pollNodeHealth();
  pollMotionOperator();
  setInterval(fetchStatus, 5000);
  setInterval(pollNodeHealth, 1500);
  setInterval(pollMotionOperator, 1500);
  setInterval(pollVisionEvents, 2500);
  try { fetch(api('/stop')); } catch(_) {}
  pushActivity('Dashboard ready', '\uD83C\uDFE0');
  setTimeout(function () { showToast('\uD83D\uDE0A', "Hi! I'm WALL-E"); }, 3000);
  window.addEventListener('online', function () {
    if (currentVisiblePage === 'navigation' && window.NavWorldContext) NavWorldContext.refresh(stateCache.wifi, { force: true });
  });
  window.addEventListener('offline', function () {
    if (currentVisiblePage === 'navigation' && window.NavWorldContext) NavWorldContext.refresh(stateCache.wifi, { force: true });
  });
}
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', bootLros);
} else {
  bootLros();
}
