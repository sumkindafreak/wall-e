/**
 * Navigation page — expanded mission command deck (client-side state + planner metrics).
 * Depends: LrosNavigation.getRouteInfo (optional)
 */
(function (global) {
  'use strict';

  var ST = { IDLE: 'idle', ARMED: 'armed', RUNNING: 'running', PAUSED: 'paused', COMPLETE: 'complete', ABORTED: 'aborted' };
  var state = ST.IDLE;
  var progress = 0;
  var runStartedMs = 0;
  var runPausedAccum = 0;
  var pauseStartedMs = 0;
  var targetEtaSec = 0;
  var tickTimer = null;
  var lastLog = [];

  function $(id) {
    return document.getElementById(id);
  }

  function setText(id, t) {
    var el = $(id);
    if (el) el.textContent = t;
  }

  function logLine(msg) {
    var now = new Date();
    var ts = now.toTimeString().slice(0, 8);
    var line = '[' + ts + '] ' + msg;
    lastLog.unshift(line);
    if (lastLog.length > 80) lastLog.pop();
    var box = $('nav-mission-log');
    if (box) {
      box.innerHTML = lastLog.map(function (l) { return '<div class="nav-mission-log-row">' + esc(l) + '</div>'; }).join('');
    }
    try {
      if (typeof pushActivity === 'function') pushActivity(msg, '\u2693');
    } catch (e) {}
  }

  function esc(s) {
    return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  }

  function routeInfo() {
    if (global.LrosNavigation && typeof LrosNavigation.getRouteInfo === 'function') {
      return LrosNavigation.getRouteInfo();
    }
    return { waypointCount: 0, pathLength: 0, etaSeconds: 0, hasPath: false, segmentHint: 0 };
  }

  function preflightOk() {
    var ids = ['nav-pf-battery', 'nav-pf-link', 'nav-pf-path', 'nav-pf-clear'];
    for (var i = 0; i < ids.length; i++) {
      var el = $(ids[i]);
      if (el && !el.checked) return false;
    }
    return true;
  }

  function updatePreflightGate() {
    var arm = $('nav-mission-btn-arm');
    if (arm) {
      var r = routeInfo();
      var canArm = preflightOk() && r.waypointCount > 0;
      arm.disabled = !canArm;
      arm.title = canArm ? 'Arm mission (required before Start)' : 'Need waypoints + preflight';
    }
  }

  function setState(s) {
    state = s;
    var pill = $('nav-mission-state-pill');
    if (pill) {
      pill.textContent = s.toUpperCase();
      pill.className = 'nav-mission-state-pill state-' + s;
    }
    var sub = $('nav-mission-sub');
    if (sub) {
      var hints = {
        idle: 'Plan waypoints on the map, complete preflight, then Arm.',
        armed: 'Mission armed — press Start to execute (simulated until firmware API).',
        running: 'Preview run in progress (simulated). Live execution will follow once navigation is implemented on the robot.',
        paused: 'Hold — Resume or Abort.',
        complete: 'Finished (simulated). Reset or edit waypoints for another run.',
        aborted: 'Stopped. Reset or replan.'
      };
      sub.textContent = hints[s] || '';
    }
    updatePhaseStrip();
    updateActionButtons();
    updatePreflightGate();
  }

  function updatePhaseStrip() {
    var keys = ['plan', 'arm', 'exec', 'verify', 'done'];
    if (state === ST.COMPLETE) {
      keys.forEach(function (k) {
        var el = $('nav-phase-' + k);
        if (!el) return;
        el.classList.remove('nav-phase-pending', 'nav-phase-active', 'nav-phase-done');
        el.classList.add('nav-phase-done');
      });
      return;
    }
    var step = 0;
    if (state === ST.IDLE || state === ST.ABORTED) step = 0;
    else if (state === ST.ARMED) step = 1;
    else if (state === ST.RUNNING || state === ST.PAUSED) step = 2;
    keys.forEach(function (k, i) {
      var el = $('nav-phase-' + k);
      if (!el) return;
      el.classList.remove('nav-phase-pending', 'nav-phase-active', 'nav-phase-done');
      if (i < step) el.classList.add('nav-phase-done');
      else if (i === step) el.classList.add('nav-phase-active');
      else el.classList.add('nav-phase-pending');
    });
  }

  function updateActionButtons() {
    var arm = $('nav-mission-btn-arm');
    var start = $('nav-mission-btn-start');
    var pause = $('nav-mission-btn-pause');
    var resume = $('nav-mission-btn-resume');
    var abort = $('nav-mission-btn-abort');
    var reset = $('nav-mission-btn-reset');
    if (arm) arm.style.display = state === ST.IDLE || state === ST.ABORTED ? '' : 'none';
    if (start) {
      start.style.display = state === ST.ARMED ? '' : 'none';
      start.disabled = false;
    }
    if (pause) pause.style.display = state === ST.RUNNING ? '' : 'none';
    if (resume) resume.style.display = state === ST.PAUSED ? '' : 'none';
    if (abort) abort.style.display = state === ST.RUNNING || state === ST.PAUSED ? '' : 'none';
    if (reset) reset.style.display = state === ST.COMPLETE || state === ST.ABORTED ? '' : 'none';
  }

  function updateProgressUi() {
    var r = routeInfo();
    var pct = Math.max(0, Math.min(100, Math.round(progress)));
    var ring = $('nav-mission-ring-fill');
    if (ring) ring.style.setProperty('--p', String(pct)); /* 0–100 for conic-gradient */

    setText('nav-mission-pct', pct + '%');
    setText('nav-mission-eta-large', r.etaSeconds ? '~' + r.etaSeconds + ' s' : '—');
    setText('nav-mission-route-len', r.pathLength > 0 ? '~' + r.pathLength.toFixed(1) + ' u' : '—');
    setText('nav-mission-wp-count', String(r.waypointCount));
    setText('nav-mission-segments', r.hasPath ? String(Math.max(0, r.waypointCount)) : '0');

    var rem = targetEtaSec > 0 ? Math.max(0, Math.ceil((targetEtaSec * (100 - pct)) / 100)) : 0;
    setText('nav-mission-eta-remain', state === ST.RUNNING && rem ? '~' + rem + ' s' : state === ST.PAUSED ? 'held' : '—');
  }

  function syncMetricsFromDom(s) {
    if (!s) return;
    if (s.battery && s.battery.voltage != null) setText('nav-mission-battery', s.battery.voltage.toFixed(2) + ' V');
    if (s.auto) {
      if (s.auto.sonar != null) setText('nav-mission-sonar', s.auto.sonar + ' cm');
      setText('nav-mission-autonomy', s.auto.enabled ? (s.auto.state || 'on') : 'off');
      if (s.auto.rthActive) setText('nav-mission-rth', 'active');
      else setText('nav-mission-rth', '—');
    }
    if (s.imu && s.imu.heading != null) setText('nav-mission-heading', s.imu.heading + '\u00B0');
  }

  function tick() {
    if (state !== ST.RUNNING) return;
    var elapsed = Date.now() - runStartedMs - runPausedAccum;
    var dur = Math.max(3000, targetEtaSec * 1000);
    progress = Math.min(100, (elapsed / dur) * 100);
    if (progress >= 99.5) {
      progress = 100;
      stopTimer();
      setState(ST.COMPLETE);
      logLine('Mission complete (simulated — wire firmware for real execution).');
      if (typeof showToast === 'function') showToast('\u2705', 'Mission complete');
    }
    updateProgressUi();
  }

  function stopTimer() {
    if (tickTimer) {
      clearInterval(tickTimer);
      tickTimer = null;
    }
  }

  function startTimer() {
    stopTimer();
    tickTimer = setInterval(tick, 200);
  }

  function onRouteChanged() {
    var r = routeInfo();
    targetEtaSec = r.etaSeconds || 0;
    updateProgressUi();
    if (state === ST.IDLE || state === ST.ABORTED) {
      setText('nav-mission-plan-summary', r.waypointCount ? r.waypointCount + ' waypoint(s), ~' + (r.etaSeconds || 0) + ' s ETA' : 'No waypoints — tap the map to plan.');
    }
  }

  function init() {
    var deck = $('nav-mission-deck');
    if (!deck) return;

    ['nav-pf-battery', 'nav-pf-link', 'nav-pf-path', 'nav-pf-clear'].forEach(function (id) {
      var el = $(id);
      if (el && !el._navMissionBound) {
        el._navMissionBound = true;
        el.addEventListener('change', updatePreflightGate);
      }
    });

    var arm = $('nav-mission-btn-arm');
    if (arm && !arm._bound) {
      arm._bound = true;
      arm.addEventListener('click', function () {
        var r = routeInfo();
        if (!r.waypointCount) {
          if (typeof showToast === 'function') showToast('\u26A0', 'Add at least one waypoint');
          return;
        }
        if (!preflightOk()) {
          if (typeof showToast === 'function') showToast('\u26A0', 'Complete preflight checklist');
          return;
        }
        setState(ST.ARMED);
        logLine('Mission armed: ' + r.waypointCount + ' waypoint(s).');
      });
    }

    var st = $('nav-mission-btn-start');
    if (st && !st._bound) {
      st._bound = true;
      st.addEventListener('click', function () {
        startMission();
      });
    }
    var pu = $('nav-mission-btn-pause');
    if (pu && !pu._bound) {
      pu._bound = true;
      pu.addEventListener('click', function () { pauseMission(); });
    }
    var rs = $('nav-mission-btn-resume');
    if (rs && !rs._bound) {
      rs._bound = true;
      rs.addEventListener('click', function () { resumeMission(); });
    }
    var ab = $('nav-mission-btn-abort');
    if (ab && !ab._bound) {
      ab._bound = true;
      ab.addEventListener('click', function () { abortMission(); });
    }
    var rst = $('nav-mission-btn-reset');
    if (rst && !rst._bound) {
      rst._bound = true;
      rst.addEventListener('click', function () { resetMission(); });
    }

    setState(ST.IDLE);
    onRouteChanged();
    updatePreflightGate();
  }

  function startMission() {
    if (state !== ST.ARMED) {
      if (typeof showToast === 'function') showToast('\u26A0', 'Arm the mission first');
      return;
    }
    var r = routeInfo();
    if (!r.waypointCount) {
      if (typeof showToast === 'function') showToast('\u26A0', 'Plan a route on the map first');
      return;
    }
    progress = 0;
    runPausedAccum = 0;
    runStartedMs = Date.now();
    targetEtaSec = Math.max(1, r.etaSeconds || 30);
    setState(ST.RUNNING);
    logLine('Execute: ' + r.waypointCount + ' WP, ~' + targetEtaSec + ' s ETA (simulated).');
    if (typeof showToast === 'function') showToast('\u2693', 'Mission running');
    startTimer();
    updateProgressUi();
  }

  function pauseMission() {
    if (state !== ST.RUNNING) return;
    pauseStartedMs = Date.now();
    setState(ST.PAUSED);
    stopTimer();
    logLine('Paused by operator.');
    if (typeof showToast === 'function') showToast('\u23F8', 'Mission paused');
  }

  function resumeMission() {
    if (state !== ST.PAUSED) return;
    runPausedAccum += Date.now() - pauseStartedMs;
    setState(ST.RUNNING);
    logLine('Resumed.');
    if (typeof showToast === 'function') showToast('\u25B6', 'Mission resumed');
    startTimer();
  }

  function abortMission() {
    stopTimer();
    progress = 0;
    setState(ST.ABORTED);
    logLine('Aborted.');
    if (typeof showToast === 'function') showToast('\u26D4', 'Mission aborted');
    updateProgressUi();
  }

  function resetMission() {
    progress = 0;
    stopTimer();
    setState(ST.IDLE);
    logLine('Reset.');
    updateProgressUi();
    onRouteChanged();
  }

  function syncFromState(s) {
    syncMetricsFromDom(s);
    var lowBatt = s && s.battery && s.battery.voltage != null && s.battery.voltage < 11.0;
    var w = $('nav-mission-batt-warn');
    if (w) {
      w.hidden = !lowBatt;
      w.textContent = lowBatt ? 'Voltage low — charge before long routes.' : '';
    }
  }

  global.NavMissionPanel = {
    init: init,
    syncFromState: syncFromState,
    onRouteChanged: onRouteChanged,
    start: startMission,
    pause: pauseMission,
    resume: resumeMission,
    abort: abortMission
  };
})(typeof window !== 'undefined' ? window : this);
