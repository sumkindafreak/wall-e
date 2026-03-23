#pragma once
#include <Arduino.h>
// WALL-E LROS Web UI - Built from webui/
// Replace WALLE_PAGE in web_page.h with WALLE_PAGE_LROS to use this UI.
const char WALLE_PAGE_LROS[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
  <meta name="theme-color" content="#0a0c10">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <title>WALL-E - LROS</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;600;700&family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
  <style>/* WALL-E LROS - Living Robot Operating System - Full Design System */
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

:root {
  --bg: #0a0c10; --surface: #0f1219; --surface2: #161b24; --surface3: #1c222d;
  --border: #252d3a; --border2: #2f3a4a;
  --accent: #f5a623; --accent-dim: #c4841c; --accent-glow: rgba(245,166,35,0.25);
  --ok: #3ddc84; --warn: #f5a623; --stop: #e63946; --stop-glow: rgba(230,57,70,0.3);
  --info: #5eb3f6;
  --txt: #e4e8f0; --txt-mid: #8b95a8; --txt-dim: #5a6375;
  --tab-h: 52px; --topbar-h: 52px; --estop-h: 56px; --radius: 12px; --radius-sm: 8px;
  --ease-out: cubic-bezier(0.22,1,0.36,1); --dur: 0.25s;
}
[data-theme="low-battery"] { --accent: #e63946; --accent-glow: rgba(230,57,70,0.2); }
[data-theme="docking"] { --accent: #3ddc84; --accent-glow: rgba(61,220,132,0.2); }
[data-theme="fault"] { --accent: #e63946; --accent-glow: rgba(230,57,70,0.3); }
[data-theme="sleep"] { --accent: #5a6b8a; --accent-glow: rgba(90,107,138,0.15); }

html, body { height: 100%; background: var(--bg); color: var(--txt);
  font-family: 'Inter', -apple-system, sans-serif; -webkit-tap-highlight-color: transparent;
  touch-action: manipulation; overflow: hidden; }
#app { display: flex; flex-direction: column; height: 100%; }

/* Top bar */
#topbar { flex-shrink: 0; display: flex; align-items: center; justify-content: space-between;
  height: var(--topbar-h); padding: 0 16px; background: var(--surface); border-bottom: 1px solid var(--border); }
#topbar-title { font-family: 'Orbitron', monospace; font-size: 0.9rem; font-weight: 700;
  letter-spacing: 0.12em; color: var(--accent); }
#topbar-title span { color: var(--txt-dim); font-weight: 400; font-size: 0.65rem; margin-left: 6px; }
#conn-badge { display: flex; align-items: center; gap: 6px; font-size: 0.7rem; color: var(--txt-dim);
  background: var(--surface2); border: 1px solid var(--border); border-radius: 20px;
  padding: 5px 12px; cursor: pointer; transition: border-color var(--dur), background var(--dur); }
#conn-badge:hover { border-color: var(--accent); background: var(--surface3); }
#conn-dot { width: 6px; height: 6px; border-radius: 50%; flex-shrink: 0; }
#conn-dot.ap { background: var(--warn); }
#conn-dot.sta { background: var(--ok); }
#conn-dot.off { background: var(--txt-dim); }
#conn-dot.pulse { background: var(--accent); animation: pulse 0.8s infinite; }
@keyframes pulse { 0%,100%{opacity:1} 50%{opacity:0.4} }

/* Face */
.face-container { display: flex; justify-content: center; align-items: center; padding: 24px; min-height: 120px; }
.face { width: 90px; height: 90px; background: radial-gradient(ellipse 50% 50% at 50% 50%, var(--surface2) 0%, var(--surface) 100%);
  border: 2px solid var(--border); border-radius: 50%;
  box-shadow: inset 0 0 30px rgba(0,0,0,0.5), 0 0 20px var(--accent-glow);
  display: flex; align-items: center; justify-content: center; transition: all var(--dur); }
.face[data-mood="curious"] { animation: look 3s ease-in-out infinite; }
.face[data-mood="sleep"] { opacity: 0.5; }
.face-eyes { display: flex; gap: 14px; margin-bottom: 4px; }
.face-eye { width: 14px; height: 18px; background: var(--accent); border-radius: 50%;
  box-shadow: 0 0 8px var(--accent-glow); animation: blink 4s ease-in-out infinite; }
.face[data-mood="sleep"] .face-eye { height: 3px; border-radius: 2px; }
@keyframes blink { 0%,45%,55%,100%{transform:scaleY(1)} 50%{transform:scaleY(0.15)} }
@keyframes look { 0%,100%{transform:rotate(0)} 50%{transform:rotate(3deg)} }

/* Toasts */
#toast-stack { position: fixed; bottom: calc(var(--estop-h) + 16px); left: 50%; transform: translateX(-50%);
  z-index: 1000; display: flex; flex-direction: column; gap: 8px;
  max-width: calc(100% - 32px); width: 340px; pointer-events: none; }
.toast { padding: 12px 16px; background: rgba(15,18,25,0.95); border: 1px solid var(--border);
  border-radius: var(--radius-sm); font-size: 0.85rem; animation: toast-in 0.35s var(--ease-out);
  box-shadow: 0 4px 20px rgba(0,0,0,0.4); pointer-events: auto; }
@keyframes toast-in { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }

/* Tab bar */
#tabbar { flex-shrink: 0; display: flex; overflow-x: auto; -webkit-overflow-scrolling: touch;
  scrollbar-width: none; background: var(--surface); border-bottom: 1px solid var(--border); }
#tabbar::-webkit-scrollbar { display: none; }
.tab { flex: 0 0 auto; min-width: 60px; height: var(--tab-h); display: flex; flex-direction: column;
  align-items: center; justify-content: center; gap: 2px; font-size: 0.58rem; font-weight: 600;
  letter-spacing: 0.05em; text-transform: uppercase; color: var(--txt-dim); cursor: pointer;
  border-bottom: 2px solid transparent; transition: color var(--dur), border-color var(--dur); }
.tab svg { width: 18px; height: 18px; stroke: currentColor; }
.tab:hover { color: var(--txt-mid); }
.tab.active { color: var(--accent); border-bottom-color: var(--accent); }

/* Pages */
#pages { flex: 1; overflow: hidden; position: relative; }
.page { position: absolute; inset: 0; overflow-y: auto; overscroll-behavior: contain; display: none;
  flex-direction: column; padding: 16px; gap: 16px; }
.page.active { display: flex; animation: page-in 0.3s var(--ease-out); }
@keyframes page-in { from { opacity: 0.7; } to { opacity: 1; } }

/* Cards */
.card { background: var(--surface2); border: 1px solid var(--border); border-radius: var(--radius); overflow: hidden; }
.card-header { padding: 12px 16px; background: var(--surface); border-bottom: 1px solid var(--border);
  font-size: 0.7rem; font-weight: 700; letter-spacing: 0.08em; text-transform: uppercase; color: var(--txt-mid);
  display: flex; align-items: center; justify-content: space-between; }
.card-body { padding: 16px; }

/* Status rows */
.status-row { display: flex; justify-content: space-between; align-items: center;
  padding: 10px 0; border-bottom: 1px solid var(--border); font-size: 0.85rem; }
.status-row:last-child { border-bottom: none; }
.status-row .label { color: var(--txt-dim); }
.status-row .value { font-weight: 600; font-family: Consolas, monospace; }
.status-row .value.ok { color: var(--ok); }
.status-row .value.warn { color: var(--warn); }
.status-row .value.dim { color: var(--txt-dim); }

/* Buttons */
.btn { padding: 10px 16px; background: var(--accent); border: none; border-radius: var(--radius-sm);
  color: #000; font-size: 0.8rem; font-weight: 600; cursor: pointer; transition: filter var(--dur); }
.btn:hover { filter: brightness(1.1); }
.btn-ghost { background: transparent; border: 1px solid var(--border); color: var(--txt-mid); }
.btn-ghost:hover { border-color: var(--accent); color: var(--accent); }
.btn-stop { background: var(--stop); color: #fff; }
.btn-small { padding: 5px 10px; font-size: 0.7rem; }

/* Form */
.form-group { margin-bottom: 12px; }
.form-group label { display: block; font-size: 0.7rem; font-weight: 600; color: var(--txt-dim); margin-bottom: 4px; }
.form-group input, .form-group select {
  width: 100%; padding: 10px 12px; background: var(--surface); border: 1px solid var(--border2);
  border-radius: var(--radius-sm); color: var(--txt); font-size: 0.9rem; }
.form-group input:focus { border-color: var(--accent); outline: none; }

/* Joystick */
.joystick-wrap { display: flex; justify-content: center; padding: 20px 0; touch-action: none; }
.joystick-container { position: relative; width: 160px; height: 160px; border-radius: 50%;
  background: var(--surface2); border: 2px solid var(--border); box-shadow: inset 0 0 24px rgba(0,0,0,0.4);
  cursor: pointer; }
.joystick-stick { position: absolute; width: 52px; height: 52px; left: 50%; top: 50%; margin: -26px 0 0 -26px;
  border-radius: 50%; background: var(--accent); border: 2px solid var(--border2);
  box-shadow: 0 0 15px var(--accent-glow); pointer-events: none; transition: transform 0.06s ease-out; }

/* Tank sliders */
.tank-row { display: flex; gap: 16px; align-items: center; margin: 12px 0; }
.tank-col { flex: 1; }
.tank-col label { font-size: 0.7rem; color: var(--txt-dim); display: block; margin-bottom: 4px; }
.tank-col input[type=range] { width: 100%; -webkit-appearance: none; height: 6px;
  background: var(--border2); border-radius: 3px; }
.tank-col input[type=range]::-webkit-slider-thumb {
  -webkit-appearance: none; width: 24px; height: 24px; border-radius: 50%;
  background: var(--accent); border: 2px solid var(--bg); cursor: pointer; }

/* E-Stop */
#estop-bar { flex-shrink: 0; display: flex; align-items: center; justify-content: center;
  height: var(--estop-h); background: var(--surface); border-top: 1px solid var(--border); padding: 8px; }
#estop-btn { width: 100%; max-width: 200px; padding: 12px 24px; background: var(--stop); border: none;
  border-radius: var(--radius); color: #fff; font-family: 'Orbitron', monospace; font-size: 0.85rem;
  font-weight: 700; letter-spacing: 0.15em; cursor: pointer; transition: all var(--dur); }
#estop-btn:hover { background: #ff4d5a; box-shadow: 0 0 20px var(--stop-glow); }
#estop-btn:active { transform: scale(0.98); }

/* Override banner */
#override-banner { display: none; position: fixed; top: 0; left: 0; right: 0; z-index: 999;
  padding: 10px 16px; background: var(--warn); color: #000; font-size: 0.8rem; font-weight: 600; text-align: center; }
#override-banner.visible { display: block; }

/* Grid of nav tiles */
.nav-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 12px; }
.nav-tile { display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 8px;
  padding: 20px; background: var(--surface2); border: 1px solid var(--border); border-radius: var(--radius);
  cursor: pointer; transition: border-color var(--dur), background var(--dur); text-decoration: none; color: var(--txt); }
.nav-tile:hover { border-color: var(--accent); background: var(--surface3); }
.nav-tile svg { width: 28px; height: 28px; stroke: var(--accent); }

/* Map canvas */
#map-canvas { width: 100%; height: 220px; background: var(--surface); border: 1px solid var(--border);
  border-radius: var(--radius); display: block; }

/* Topology map */
.topology-svg { width: 100%; height: 180px; background: var(--surface); border-radius: var(--radius); }

/* Slider */
input[type=range] { -webkit-appearance: none; width: 100%; height: 6px; background: var(--border2);
  border-radius: 3px; outline: none; }
input[type=range]::-webkit-slider-thumb {
  -webkit-appearance: none; width: 20px; height: 20px; border-radius: 50%;
  background: var(--accent); border: 2px solid var(--bg); cursor: pointer; }

/* Network list */
.net-list { max-height: 200px; overflow-y: auto; }
.net-item { display: flex; align-items: center; gap: 10px; padding: 10px 12px; border-radius: 8px;
  background: var(--surface); border: 1px solid var(--border); margin-bottom: 4px; cursor: pointer;
  transition: border-color var(--dur); }
.net-item:hover, .net-item.selected { border-color: var(--accent); background: rgba(245,166,35,0.06); }
.net-name { flex: 1; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.net-rssi { font-size: 0.75rem; color: var(--txt-dim); font-family: monospace; }

/* Video / FPV */
.fpv-container { width: 100%; background: #000; border-radius: var(--radius); overflow: hidden;
  aspect-ratio: 4/3; display: flex; align-items: center; justify-content: center; }
.fpv-container img { max-width: 100%; max-height: 100%; object-fit: contain; }

/* Log list */
.log-list { max-height: 280px; overflow-y: auto; }
.log-item { padding: 8px 12px; border-bottom: 1px solid var(--border); font-size: 0.8rem; }
.log-item .log-time { color: var(--txt-dim); font-size: 0.7rem; margin-right: 8px; }

/* Progress bar */
.progress-bar { height: 8px; background: var(--border2); border-radius: 4px; overflow: hidden; }
.progress-fill { height: 100%; background: var(--accent); border-radius: 4px; transition: width var(--dur); }
.progress-fill.low { background: var(--warn); }
.progress-fill.critical { background: var(--stop); }
</style>
</head>
<body>
<div id="app">
  <div id="override-banner" aria-live="polite">Local Control Active - CYD touchscreen has control</div>

  <header id="topbar">
    <h1 id="topbar-title">WALL-E <span>LROS</span></h1>
    <div id="conn-badge" onclick="switchTab('network')">
      <div id="conn-dot" class="ap"></div>
      <span id="conn-label">AP</span>
    </div>
  </header>

  <nav id="tabbar">
    <div class="tab active" data-tab="home" onclick="switchTab('home')"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M3 9l9-7 9 7v11a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/><polyline points="9 22 9 12 15 12 15 22"/></svg>Home</div>
    <div class="tab" data-tab="drive" onclick="switchTab('drive')"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M12 2v3M12 19v3M2 12h3M19 12h3"/></svg>Drive</div>
    <div class="tab" data-tab="docking" onclick="switchTab('docking')"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><rect x="2" y="7" width="20" height="14" rx="2"/><path d="M16 21V5a2 2 0 0 0-2-2h-4a2 2 0 0 0-2 2v16"/></svg>Dock</div>
    <div class="tab" data-tab="network" onclick="switchTab('network')"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M5 12.55a11 11 0 0 1 14.08 0"/><path d="M1.42 9a16 16 0 0 1 21.16 0"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><circle cx="12" cy="20" r="1" fill="currentColor"/></svg>Network</div>
    <div class="tab" data-tab="more" onclick="switchTab('more')"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><circle cx="12" cy="12" r="1"/><circle cx="19" cy="12" r="1"/><circle cx="5" cy="12" r="1"/></svg>More</div>
  </nav>

  <main id="pages">
    <!-- HOME -->
    <div class="page active" id="page-home">
      <div class="face-container">
        <div class="face" id="walle-face" data-mood="happy" aria-label="WALL-E face">
          <div class="face-eyes"><div class="face-eye"></div><div class="face-eye"></div></div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Quick Status</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Battery</span><span class="value" id="home-battery">-</span></div>
          <div class="progress-bar" style="margin:8px 0"><div class="progress-fill" id="home-battery-bar" style="width:80%"></div></div>
          <div class="status-row"><span class="label">State</span><span class="value" id="home-state">-</span></div>
          <div class="status-row"><span class="label">Emotion</span><span class="value" id="home-emotion">-</span></div>
        </div>
      </div>
      <div style="display:flex;gap:12px;flex-wrap:wrap">
        <button class="btn" onclick="switchTab('drive')">Drive</button>
        <button class="btn btn-ghost" onclick="switchTab('docking')">Docking</button>
        <button class="btn btn-ghost" onclick="switchTab('network')">Network</button>
      </div>
    </div>

    <!-- DRIVE -->
    <div class="page" id="page-drive">
      <div class="card">
        <div class="card-header">Control Mode</div>
        <div class="card-body" style="display:flex;gap:8px;flex-wrap:wrap">
          <button class="btn btn-small" id="mode-joystick" onclick="setDriveMode('joystick')">Joystick</button>
          <button class="btn btn-small btn-ghost" id="mode-tank" onclick="setDriveMode('tank')">Tank</button>
          <button class="btn btn-small btn-ghost" id="mode-ai" onclick="setDriveMode('ai')">AI Assist</button>
        </div>
      </div>
      <div id="drive-joystick" class="joystick-wrap">
        <div class="joystick-container" id="joystick"><div class="joystick-stick" id="joystick-stick"></div></div>
      </div>
      <div id="drive-tank" style="display:none">
        <div class="tank-row"><div class="tank-col"><label>Left</label><input type="range" id="tank-left" min="-255" max="255" value="0"></div><div class="tank-col"><label>Right</label><input type="range" id="tank-right" min="-255" max="255" value="0"></div></div>
      </div>
      <div class="card">
        <div class="card-body">
          <div class="status-row"><span class="label">Speed</span><span class="value" id="drive-speed">0 / 255</span></div>
          <div class="form-group"><label>Speed profile</label><select id="speed-profile" onchange="applySpeedProfile()"><option value="low">Low</option><option value="normal" selected>Normal</option><option value="high">High</option></select></div>
        </div>
      </div>
      <div style="display:flex;gap:12px">
        <button class="btn" onclick="triggerAutoDock()">Go to Dock</button>
        <button class="btn btn-ghost" onclick="setHome()">Set Home</button>
      </div>
    </div>

    <!-- NAVIGATION -->
    <div class="page" id="page-navigation">
      <div class="card">
        <div class="card-header">Mission Map</div>
        <div class="card-body" style="padding:8px">
          <canvas id="map-canvas" width="400" height="220"></canvas>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Saved Locations</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Home</span><span class="value dim" id="nav-home">Not set</span></div>
          <div class="status-row"><span class="label">Current</span><span class="value" id="nav-current">-</span></div>
          <button class="btn btn-small" onclick="addWaypoint()" style="margin-top:8px">Add Waypoint</button>
        </div>
      </div>
    </div>

    <!-- DOCKING -->
    <div class="page" id="page-docking">
      <div class="card">
        <div class="card-header">Approach Alignment</div>
        <div class="card-body" style="text-align:center;padding:24px">
          <div style="font-size:3rem;margin-bottom:12px">ðŸ“</div>
          <div class="status-row"><span class="label">Stage</span><span class="value" id="dock-stage">-</span></div>
          <div class="status-row"><span class="label">Beam</span><span class="value" id="dock-beam">-</span></div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Charge Status</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Current</span><span class="value" id="dock-current">-</span></div>
          <div class="progress-bar" style="margin:8px 0"><div class="progress-fill" id="dock-charge-bar" style="width:0%"></div></div>
        </div>
      </div>
      <div style="display:flex;gap:12px">
        <button class="btn" onclick="triggerAutoDock()">Start Docking</button>
        <button class="btn btn-ghost" onclick="apiCall('/api/dock/cancel')">Cancel</button>
      </div>
    </div>

    <!-- VISION -->
    <div class="page" id="page-vision">
      <div class="card">
        <div class="card-header">Vision Status</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Connected</span><span class="value" id="vision-connected">-</span></div>
          <div class="status-row"><span class="label">State</span><span class="value" id="vision-state">-</span></div>
          <div class="status-row"><span class="label">Motion</span><span class="value" id="vision-motion">-</span></div>
          <div class="status-row"><span class="label">Target</span><span class="value" id="vision-target">-</span></div>
          <div class="status-row"><span class="label">Size/Class</span><span class="value" id="vision-size">-</span></div>
          <div class="status-row"><span class="label">Age</span><span class="value" id="vision-age">-</span></div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Snapshot</div>
        <div class="card-body" style="display:flex;gap:8px;flex-wrap:wrap">
          <button class="btn btn-small" onclick="snapshot()">Open Snapshot</button>
          <button class="btn btn-small btn-ghost" onclick="toggleNightMode()">Night mode</button>
        </div>
      </div>
    </div>

    <!-- AUDIO -->
    <div class="page" id="page-audio">
      <div class="card">
        <div class="card-header">Soundboard</div>
        <div class="card-body" style="display:flex;gap:8px;flex-wrap:wrap">
          <button class="btn btn-small" onclick="playSound('beep')">Beep</button>
          <button class="btn btn-small" onclick="playSound('happy')">Happy</button>
          <button class="btn btn-small" onclick="playSound('sad')">Sad</button>
          <button class="btn btn-small" onclick="playSound('curious')">Curious</button>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Volume</div>
        <div class="card-body">
          <input type="range" id="audio-volume" min="0" max="255" value="180" onchange="setVolume(this.value)">
          <div class="status-row"><span class="label">Level</span><span class="value" id="audio-vol-val">180</span></div>
        </div>
      </div>
    </div>

    <!-- AI / PERSONALITY -->
    <div class="page" id="page-ai">
      <div class="card">
        <div class="card-header">Behaviour Mode</div>
        <div class="card-body">
          <select id="ai-mode" onchange="setBehaviourMode(this.value)">
            <option value="curious">Curious</option><option value="happy">Happy</option><option value="shy">Shy</option>
            <option value="tired">Tired</option><option value="excited">Excited</option>
          </select>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Personality</div>
        <div class="card-body">
          <div class="form-group"><label>Curiosity</label><input type="range" id="ai-curiosity" min="0" max="100" value="50"></div>
          <div class="form-group"><label>Energy</label><input type="range" id="ai-energy" min="0" max="100" value="70"></div>
          <div class="status-row"><span class="label">Learning</span><span class="value"><input type="checkbox" id="ai-learning" checked></span></div>
        </div>
      </div>
    </div>

    <!-- MISSIONS -->
    <div class="page" id="page-missions">
      <div class="card">
        <div class="card-header">Mission List</div>
        <div class="card-body">
          <div id="mission-list">
            <div class="status-row"><span class="label">Patrol</span><button class="btn btn-small" onclick="runMission('patrol')">Run</button></div>
            <div class="status-row"><span class="label">Return Home</span><button class="btn btn-small" onclick="runMission('rth')">Run</button></div>
          </div>
        </div>
      </div>
    </div>

    <!-- TELEMETRY -->
    <div class="page" id="page-telemetry">
      <div class="card">
        <div class="card-header">IMU</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Heading</span><span class="value" id="tel-heading">-</span></div>
          <div class="status-row"><span class="label">Pitch/Roll</span><span class="value" id="tel-orient">-</span></div>
          <button class="btn btn-small btn-ghost" onclick="apiCall('/imu/recalibrate')">Recalibrate</button>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Obstacle Sensors</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Sonar</span><span class="value" id="tel-sonar">-</span></div>
          <div class="progress-bar" style="margin:8px 0"><div class="progress-fill" id="tel-sonar-bar" style="width:100%"></div></div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">GPS</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Fix</span><span class="value" id="tel-gps">-</span></div>
          <div class="status-row"><span class="label">Satellites</span><span class="value" id="tel-sats">-</span></div>
        </div>
      </div>
    </div>

    <!-- POWER -->
    <div class="page" id="page-power">
      <div class="card">
        <div class="card-header">Battery</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Voltage</span><span class="value" id="pwr-voltage">-</span></div>
          <div class="status-row"><span class="label">Est. runtime</span><span class="value" id="pwr-runtime">-</span></div>
          <div class="progress-bar" style="margin:8px 0"><div class="progress-fill" id="pwr-bar" style="width:80%"></div></div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Sleep</div>
        <div class="card-body">
          <button class="btn btn-ghost" onclick="apiCall('/api/sleep')">Enter Sleep</button>
        </div>
      </div>
    </div>

    <!-- NETWORK -->
    <div class="page" id="page-network">
      <div class="card">
        <div class="card-header">Network Topology</div>
        <div class="card-body" style="padding:12px">
          <svg class="topology-svg" id="topology-svg" viewBox="0 0 320 160">
            <rect width="320" height="160" fill="var(--surface)"/>
            <circle cx="160" cy="50" r="24" fill="var(--surface2)" stroke="var(--accent)" stroke-width="2"/>
            <text x="160" y="55" text-anchor="middle" fill="var(--txt)" font-size="10">WALL-E</text>
            <line x1="120" y1="74" x2="80" y2="120" stroke="var(--border2)" stroke-width="2"/>
            <line x1="160" y1="74" x2="160" y2="120" stroke="var(--border2)" stroke-width="2"/>
            <line x1="200" y1="74" x2="240" y2="120" stroke="var(--border2)" stroke-width="2"/>
            <circle cx="80" cy="130" r="18" fill="var(--surface2)" stroke="var(--ok)" stroke-width="1"/>
            <text x="80" y="135" text-anchor="middle" fill="var(--txt)" font-size="8">Dock</text>
            <circle cx="160" cy="130" r="18" fill="var(--surface2)" stroke="var(--ok)" stroke-width="1"/>
            <text x="160" y="135" text-anchor="middle" fill="var(--txt)" font-size="8">CYD</text>
            <circle cx="240" cy="130" r="18" fill="var(--surface2)" stroke="var(--txt-dim)" stroke-width="1"/>
            <text x="240" y="135" text-anchor="middle" fill="var(--txt-dim)" font-size="8">Vision</text>
          </svg>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Wiâ€‘Fi Status</div>
        <div class="card-body">
          <div class="status-row"><span class="label">AP</span><span class="value" id="net-ap">WALL-E-Control</span></div>
          <div class="status-row"><span class="label">Home</span><span class="value dim" id="net-sta">Not connected</span></div>
          <button class="btn btn-small" onclick="showNetworkForm()">Connect to network</button>
        </div>
      </div>
      <div class="card" id="network-form-card" style="display:none">
        <div class="card-header">Connect</div>
        <div class="card-body">
          <div class="form-group"><label>Network</label><input type="text" id="net-ssid" placeholder="SSID"></div>
          <div class="form-group"><label>Password</label><input type="password" id="net-pass" placeholder="Password"></div>
          <div id="network-list" class="net-list"></div>
          <button class="btn btn-small btn-ghost" onclick="doScan()" style="margin-bottom:8px">Scan</button>
          <button class="btn" onclick="doConnect()">Connect</button>
        </div>
      </div>
    </div>

    <!-- FILES -->
    <div class="page" id="page-files">
      <div class="card">
        <div class="card-header">Storage</div>
        <div class="card-body">
          <div id="file-list" class="log-list">
            <div class="log-item value dim">No file API - use /api/files/list</div>
          </div>
          <button class="btn btn-small" onclick="refreshFiles()" style="margin-top:8px">Refresh</button>
        </div>
      </div>
    </div>

    <!-- SAFETY -->
    <div class="page" id="page-safety">
      <div class="card">
        <div class="card-header">Emergency</div>
        <div class="card-body">
          <button class="btn btn-stop" onclick="doEStop()">Emergency Stop</button>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Safety</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Child-safe</span><span class="value"><input type="checkbox" id="safety-child"></span></div>
          <div class="status-row"><span class="label">Geo-fence</span><span class="value dim">-</span></div>
        </div>
      </div>
    </div>

    <!-- LOGS -->
    <div class="page" id="page-logs">
      <div class="card">
        <div class="card-header">Activity Timeline</div>
        <div class="card-body">
          <div id="log-list" class="log-list">
            <div class="log-item"><span class="log-time">--:--</span>System ready</div>
          </div>
        </div>
      </div>
    </div>

    <!-- SECURITY -->
    <div class="page" id="page-security">
      <div class="card">
        <div class="card-header">Access</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Trusted devices</span><span class="value dim">-</span></div>
          <button class="btn btn-small btn-ghost">Add device</button>
        </div>
      </div>
    </div>

    <!-- DEVELOPER -->
    <div class="page" id="page-developer">
      <div class="card">
        <div class="card-header">API Endpoints</div>
        <div class="card-body" style="font-size:0.75rem;font-family:monospace">
          <div>/drive?left=&right=</div><div>/stop</div><div>/speed?value=</div><div>/wifi/status</div><div>/wifi/scan</div><div>/wifi/connect</div>
          <div>/api/autonomy</div><div>/api/autonomy/enable</div><div>/api/autonomy/set_home</div><div>/api/vision/status</div><div>/api/vision/snapshot_url</div>
          <div>/imu/status</div><div>/battery/status</div><div>/servo/set</div><div>/settings</div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Raw Command</div>
        <div class="card-body">
          <input type="text" id="dev-cmd" placeholder="/api/autonomy" style="margin-bottom:8px">
          <button class="btn btn-small" onclick="devFetch()">GET</button>
        </div>
      </div>
    </div>

    <!-- MORE (nav grid) -->
    <div class="page" id="page-more">
      <div class="nav-grid">
        <a class="nav-tile" href="#" onclick="switchTab('navigation');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><circle cx="12" cy="12" r="10"/><path d="M2 12h20M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/></svg>Navigation</a>
        <a class="nav-tile" href="#" onclick="switchTab('vision');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4l2-3h6l2 3h4a2 2 0 0 1 2 2z"/><circle cx="12" cy="13" r="4"/></svg>Vision</a>
        <a class="nav-tile" href="#" onclick="switchTab('audio');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/><path d="M19.07 4.93a10 10 0 0 1 0 14.14M15.54 8.46a5 5 0 0 1 0 7.07"/></svg>Audio</a>
        <a class="nav-tile" href="#" onclick="switchTab('ai');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M12 2a10 10 0 0 1 10 10c0 5.5-4.5 10-10 10S2 17.5 2 12 6.5 2 12 2z"/><circle cx="12" cy="12" r="2"/></svg>AI</a>
        <a class="nav-tile" href="#" onclick="switchTab('missions');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>Missions</a>
        <a class="nav-tile" href="#" onclick="switchTab('telemetry');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M12 20V10"/><path d="M18 20V4"/><path d="M6 20v-4"/></svg>Telemetry</a>
        <a class="nav-tile" href="#" onclick="switchTab('power');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"/></svg>Power</a>
        <a class="nav-tile" href="#" onclick="switchTab('files');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/></svg>Files</a>
        <a class="nav-tile" href="#" onclick="switchTab('safety');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>Safety</a>
        <a class="nav-tile" href="#" onclick="switchTab('logs');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/></svg>Logs</a>
        <a class="nav-tile" href="#" onclick="switchTab('security');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>Security</a>
        <a class="nav-tile" href="#" onclick="switchTab('developer');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><polyline points="16 18 22 12 16 6"/><polyline points="8 6 2 12 8 18"/></svg>Developer</a>
      </div>
    </div>
  </main>

  <footer id="estop-bar">
    <button id="estop-btn" type="button" onclick="doEStop()">E-STOP</button>
  </footer>
</div>

<div id="toast-stack" aria-live="polite"></div>

<script>/**
 * WALL-E LROS - Living Robot Operating System
 * Full Web Console JavaScript
 */
const BASE = '';
const TOAST_DURATION = 6000;
const FAILSAFE_MS = 440;

let toastId = 0;
let tankLeft = 0, tankRight = 0, maxSpeed = 255;
let driveMode = 'joystick';
let hbTimer = null;
let stateCache = {};
let cydOverride = false;

const JOY_DEAD = 0.12, JOY_MAX = 40;

// â”€â”€â”€ Navigation â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
function switchTab(name) {
  document.querySelectorAll('.tab').forEach(t => t.classList.toggle('active', t.dataset.tab === name));
  document.querySelectorAll('.page').forEach(p => p.classList.toggle('active', p.id === 'page-' + name));
  if (name === 'network') { fetchStatus(); showNetworkForm(false); }
  if (name === 'vision') initVision();
  if (name === 'navigation') drawMap();
  if (name === 'telemetry' || name === 'power') fetchStatus();
  if (name === 'drive') { document.getElementById('drive-joystick').style.display = driveMode === 'joystick' ? 'flex' : 'none'; document.getElementById('drive-tank').style.display = driveMode === 'tank' ? 'block' : 'none'; }
}

// â”€â”€â”€ Toasts (emotional presence) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
function showToast(emoji, text) {
  const stack = document.getElementById('toast-stack');
  const el = document.createElement('div');
  el.className = 'toast';
  el.innerHTML = '<span style="margin-right:8px">' + emoji + '</span>' + text;
  stack.appendChild(el);
  while (stack.children.length > 3) stack.removeChild(stack.firstChild);
  setTimeout(() => el.remove(), TOAST_DURATION);
}

function updateToastsFromState(s) {
  if (s.battery && s.battery.voltage < 10.5) showToast('\uD83D\uDD0B', "I'm getting tired");
  if (s.auto && s.auto.rthActive) showToast('\uD83D\uDCE1', 'Searching for my dock');
  if (s.auto && s.auto.enabled && s.auto.interest > 70) showToast('\uD83D\uDE0A', 'I like being with you');
}

// â”€â”€â”€ Face & theme â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

// â”€â”€â”€ Drive â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
  if ((left||right) && !hbTimer) hbTimer = setInterval(() => sendDrive(tankLeft, tankRight), FAILSAFE_MS);
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
  const joy = document.getElementById('joystick');
  const stick = document.getElementById('joystick-stick');
  if (!joy || !stick) return;
  joy.addEventListener('mousedown', e => { e.preventDefault(); stick.classList.add('held'); onJoyMove(e.clientX, e.clientY); });
  joy.addEventListener('mousemove', e => { if (stick.classList.contains('held')) onJoyMove(e.clientX, e.clientY); });
  joy.addEventListener('mouseup', onJoyEnd);
  joy.addEventListener('mouseleave', onJoyEnd);
  joy.addEventListener('touchstart', e => { e.preventDefault(); stick.classList.add('held'); onJoyMove(e.touches[0].clientX, e.touches[0].clientY); }, { passive: false });
  joy.addEventListener('touchmove', e => { e.preventDefault(); if (e.touches.length) onJoyMove(e.touches[0].clientX, e.touches[0].clientY); }, { passive: false });
  joy.addEventListener('touchend', e => { if (!e.touches.length) onJoyEnd(); }, { passive: false });
}

// Tank sliders
document.addEventListener('DOMContentLoaded', () => {
  const tl = document.getElementById('tank-left'), tr = document.getElementById('tank-right');
  if (tl && tr) {
    const send = () => { const l = parseInt(tl.value,10), r = parseInt(tr.value,10); sendDrive(l,r); if (l||r) startTankHB(); else clearInterval(hbTimer); };
    tl.addEventListener('input', send);
    tr.addEventListener('input', send);
  }
});

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

// â”€â”€â”€ E-Stop â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
async function doEStop() {
  if (navigator.vibrate) navigator.vibrate(100);
  try { await fetch(BASE + '/stop'); } catch(_) {}
  showToast('\u26D4', 'Stopped');
}

// â”€â”€â”€ Status & API â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
    if (lbl) lbl.textContent = wifi.state === 2 ? (wifi.sta_ip || 'Connected') : wifi.state === 1 ? 'Connectingâ€¦' : 'AP';

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

// â”€â”€â”€ Network wizard â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

// â”€â”€â”€ Auto dock, Set home â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
function triggerAutoDock() {
  showToast('\uD83D\uDCE1', 'Docking...');
  apiCall('/api/autonomy/enable?enable=1');
  switchTab('docking');
}

function setHome() {
  apiCall('/api/autonomy/set_home');
  showToast('\uD83D\uCCCD', 'Home set');
}

// â”€â”€â”€ Map â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

// â”€â”€â”€ Vision â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
function initVision() {
  fetch(BASE + '/api/vision/status').then(r => r.json()).then(s => {
    document.getElementById('vision-connected').textContent = s.connected ? 'Yes' : 'No';
    document.getElementById('vision-state').textContent = s.state || '-';
    document.getElementById('vision-motion').textContent = s.motion ? 'Motion' : 'None';
    document.getElementById('vision-target').textContent = '(' + s.targetX + ',' + s.targetY + ')';
    document.getElementById('vision-size').textContent = s.objectSize + ' / ' + (s.objectClassStr || s.objectClass);
    document.getElementById('vision-age').textContent = s.ageMs + ' ms';
  }).catch(() => {
    document.getElementById('vision-connected').textContent = 'No data';
  });
}

function snapshot() {
  fetch(BASE + '/api/vision/snapshot_url').then(r => r.text()).then(url => {
    if (!url) { showToast('\u26A0', 'No vision node IP'); return; }
    window.open(url, '_blank');
  }).catch(() => showToast('\u26A0', 'Snapshot error'));
}
function toggleNightMode() { showToast('\uD83C\uDF19', 'Night mode (API not implemented)'); }

// â”€â”€â”€ Audio â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
function playSound(id) {
  showToast('\uD83D\uDD0A', id);
  apiCall('/api/audio/play?id=' + id);
}

function setVolume(v) {
  document.getElementById('audio-vol-val').textContent = v;
  apiCall('/api/audio/volume?value=' + v);
}

// â”€â”€â”€ AI â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
function setBehaviourMode(mode) {
  apiCall('/api/personality/mode?mode=' + mode);
  setFaceMood(mode);
}

// â”€â”€â”€ Missions â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
function runMission(id) {
  if (id === 'rth') triggerAutoDock();
  else showToast('\uD83C\uDFAF', 'Mission ' + id);
}

function addWaypoint() { showToast('\uD83D\uCCCD', 'Add waypoint (API not implemented)'); }

// â”€â”€â”€ Files â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
function refreshFiles() {
  fetch(BASE + '/api/files/list').then(r => r.json()).then(d => {
    const list = document.getElementById('file-list');
    if (list) list.innerHTML = (d.files || []).length ? d.files.map(f => '<div class="log-item">' + f + '</div>').join('') : '<div class="log-item value dim">No files</div>';
  }).catch(() => {});
}

// â”€â”€â”€ Developer â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
function devFetch() {
  const inp = document.getElementById('dev-cmd');
  if (!inp) return;
  fetch(BASE + inp.value).then(r => r.text()).then(t => showToast('\u2713', t.slice(0,50))).catch(e => showToast('\u26A0', String(e)));
}

// â”€â”€â”€ Init â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
document.addEventListener('DOMContentLoaded', () => {
  initJoystick();
  fetchStatus();
  setInterval(fetchStatus, 5000);
  try { fetch(BASE + '/stop'); } catch(_) {}
  setTimeout(() => showToast('\uD83D\uDE0A', "Hi! I'm WALL-E"), 1500);
});
</script>
</body>
</html>

)rawliteral";