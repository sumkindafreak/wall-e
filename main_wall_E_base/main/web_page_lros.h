#pragma once
#include <Arduino.h>
// WALL-E LROS Web UI - Built from webui/ (run: webui/build-embed.ps1)
// Served at GET / via web_server.cpp
const char WALLE_PAGE_LROS[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
  <meta name="theme-color" content="#12100e">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <title>WALL-E - LROS</title>
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Orbitron:wght@400;600;700&family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
  <style>
/* WALL-E LROS - Living Robot Operating System - Full Design System */
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

:root {
  /* Workshop floor — soot & warm shadow */
  --bg: #0a0c0f;
  --surface: #151311;
  --surface2: #1b1815;
  --surface3: #242019;
  /* Oxidized steel & rust lip */
  --border: #4a3f38;
  --border2: #5c4d42;
  --rust: #8b5a2b;
  --rust-dark: #5d4037;
  /* Weathered industrial yellow (aged paint on metal) */
  --accent: #d4a834;
  --accent-dim: #8f7028;
  --accent-glow: rgba(212, 168, 52, 0.24);
  --emerald: #3ecf8e;
  --emerald-dim: #0d8059;
  --emerald-glow: rgba(62, 207, 142, 0.18);
  --ok: #4ade80;
  --warn: #d4a834;
  --stop: #c53030;
  --stop-glow: rgba(197, 48, 48, 0.35);
  --info: #8db4c9;
  --txt: #e8e4dc;
  --txt-mid: #9a9088;
  --txt-dim: #6b635c;
  --tab-h: 52px;
  --topbar-h: 52px;
  --estop-h: 56px;
  --radius: 12px;
  --radius-sm: 8px;
  --ease-out: cubic-bezier(0.22, 1, 0.36, 1);
  --dur: 0.25s;
  --shell-scroll-offset: 200px;
}
[data-theme="low-battery"] {
  --accent: #d14b4b;
  --accent-glow: rgba(209, 75, 75, 0.22);
}
[data-theme="docking"] {
  --accent: #3ecf8e;
  --accent-glow: rgba(62, 207, 142, 0.2);
}
[data-theme="fault"] {
  --accent: #d14b4b;
  --accent-glow: rgba(209, 75, 75, 0.28);
}
[data-theme="sleep"] {
  --accent: #6b7288;
  --accent-glow: rgba(107, 114, 136, 0.14);
}

html {
  height: 100%;
  background: var(--bg);
}
html,
body {
  color: var(--txt);
  font-family: 'Inter', -apple-system, sans-serif;
  -webkit-tap-highlight-color: transparent;
  touch-action: manipulation;
}
body {
  margin: 0;
  min-height: 100%;
  min-height: 100vh;
  overflow: hidden;
  background-color: var(--bg);
  background-image:
    radial-gradient(ellipse 100% 70% at 50% -25%, rgba(139, 90, 43, 0.16) 0%, transparent 55%),
    radial-gradient(ellipse 70% 55% at 0% 100%, rgba(93, 64, 55, 0.22) 0%, transparent 52%),
    radial-gradient(ellipse 65% 45% at 100% 85%, rgba(62, 48, 40, 0.2) 0%, transparent 48%),
    linear-gradient(185deg, #12100e 0%, var(--bg) 55%);
  background-attachment: fixed;
}
body::before {
  content: '';
  position: fixed;
  inset: 0;
  z-index: 0;
  pointer-events: none;
  opacity: 0.045;
  background-image: url("data:image/svg+xml,%3Csvg viewBox='0 0 256 256' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='n'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.85' numOctaves='4' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23n)'/%3E%3C/svg%3E");
}
#app {
  position: relative;
  z-index: 1;
  display: flex;
  flex-direction: column;
  min-height: 100%;
  min-height: 100vh;
  min-height: 100dvh;
  width: 100%;
  padding-bottom: calc(var(--estop-h) + 52px);
}

/* Top bar — stamped metal strip */
#topbar {
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: var(--topbar-h);
  padding: 0 16px;
  background: linear-gradient(180deg, #221e1a 0%, #151311 48%, #12100e 100%);
  border-bottom: 1px solid var(--border);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.04), inset 0 -1px 0 rgba(0, 0, 0, 0.45), 0 1px 0 rgba(139, 90, 43, 0.12);
}
#topbar-title {
  font-family: 'Orbitron', monospace;
  font-size: 0.9rem;
  font-weight: 700;
  letter-spacing: 0.12em;
  color: var(--accent);
  text-shadow: 0 0 24px var(--accent-glow);
}
#topbar-title span { color: var(--txt-dim); font-weight: 400; font-size: 0.65rem; margin-left: 6px; }
#conn-badge {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 0.7rem;
  color: var(--txt-mid);
  background: linear-gradient(165deg, var(--surface3) 0%, var(--surface2) 100%);
  border: 1px solid var(--border);
  border-radius: 20px;
  padding: 5px 12px;
  cursor: pointer;
  transition: border-color var(--dur), background var(--dur), box-shadow var(--dur);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.05);
}
#conn-badge:hover {
  border-color: rgba(212, 168, 52, 0.45);
  background: var(--surface3);
  box-shadow: 0 0 12px rgba(212, 168, 52, 0.12);
}
#conn-dot { width: 6px; height: 6px; border-radius: 50%; flex-shrink: 0; }
#conn-dot.ap { background: var(--warn); }
#conn-dot.sta { background: var(--ok); }
#conn-dot.off { background: var(--txt-dim); }
#conn-dot.pulse { background: var(--accent); animation: pulse 0.8s infinite; }
@keyframes pulse { 0%,100%{opacity:1} 50%{opacity:0.4} }

/* Face — photo hero (reference character art) */
.face-container {
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 16px 16px 8px;
  min-height: 140px;
}
.face.face--photo {
  margin: 0;
  max-width: min(92vw, 280px);
  transition: filter var(--dur), opacity var(--dur);
}
.face-frame {
  display: block;
  padding: 14px;
  border-radius: 14px;
  background: linear-gradient(165deg, rgba(93, 64, 55, 0.45) 0%, rgba(26, 21, 18, 0.95) 55%, rgba(10, 12, 16, 1) 100%);
  border: 1px solid var(--border2);
  box-shadow:
    0 0 0 1px rgba(212, 160, 23, 0.12),
    0 12px 40px rgba(0, 0, 0, 0.55),
    inset 0 1px 0 rgba(255, 255, 255, 0.05);
}
.face-img {
  display: block;
  width: 100%;
  max-width: 260px;
  height: auto;
  border-radius: 8px;
  object-fit: contain;
  background: radial-gradient(ellipse 70% 60% at 50% 40%, #2a221c 0%, #0a0c10 100%);
  transition: transform 0.5s var(--ease-out), filter 0.45s var(--ease-out), opacity var(--dur);
}
/* Mood — subtle filters on the photo (setFaceMood / body[data-mood]) */
.face.face--photo[data-mood="curious"] .face-img {
  animation: walle-look 4.2s ease-in-out infinite;
}
.face.face--photo[data-mood="sleep"] .face-img {
  opacity: 0.58;
  filter: brightness(0.88) grayscale(0.25);
}
.face.face--photo[data-mood="sad"] .face-img {
  filter: saturate(0.62) brightness(0.94);
}
.face.face--photo[data-mood="tired"] .face-img {
  filter: saturate(0.75) brightness(0.92);
}
.face.face--photo[data-mood="scared"] .face-img {
  filter: contrast(1.06) saturate(0.9) hue-rotate(-6deg);
}
.face.face--photo[data-mood="shy"] .face-img {
  filter: saturate(0.85) brightness(0.96);
}
@keyframes walle-look {
  0%, 100% { transform: rotate(0deg) scale(1); }
  35% { transform: rotate(-1.2deg) scale(1.01); }
  65% { transform: rotate(1.4deg) scale(1.01); }
}

/* Toasts */
#toast-stack { position: fixed; bottom: calc(var(--estop-h) + 16px); left: 50%; transform: translateX(-50%);
  z-index: 1000; display: flex; flex-direction: column; gap: 8px;
  max-width: calc(100% - 32px); width: 340px; pointer-events: none; }
.toast {
  padding: 12px 16px;
  background: linear-gradient(165deg, rgba(30, 26, 22, 0.98) 0%, rgba(18, 16, 14, 0.98) 100%);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  font-size: 0.85rem;
  animation: toast-in 0.35s var(--ease-out);
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.5), inset 0 1px 0 rgba(139, 90, 43, 0.1);
  pointer-events: auto;
}
@keyframes toast-in { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }

/* Tab bar */
#tabbar {
  flex-shrink: 0;
  display: flex;
  overflow-x: auto;
  -webkit-overflow-scrolling: touch;
  scrollbar-width: none;
  background: linear-gradient(180deg, #181614 0%, #12100e 100%);
  border-bottom: 1px solid var(--border);
  box-shadow: inset 0 1px 0 rgba(139, 90, 43, 0.08);
}
#tabbar::-webkit-scrollbar { display: none; }
.tab { flex: 0 0 auto; min-width: 60px; height: var(--tab-h); display: flex; flex-direction: column;
  align-items: center; justify-content: center; gap: 2px; font-size: 0.58rem; font-weight: 600;
  letter-spacing: 0.05em; text-transform: uppercase; color: var(--txt-dim); cursor: pointer;
  border-bottom: 2px solid transparent; transition: color var(--dur), border-color var(--dur); }
.tab svg { width: 18px; height: 18px; stroke: currentColor; }
.tab:hover { color: var(--txt-mid); }
.tab.active { color: var(--accent); border-bottom-color: var(--accent); }

/* Pages — main scroll area (must fill viewport under chrome) */
#pages {
  flex: 1 1 0;
  min-height: 0;
  overflow: hidden;
  position: relative;
}
.page {
  position: absolute;
  inset: 0;
  display: none;
  flex-direction: column;
  padding: 16px;
  gap: 16px;
  min-height: 0;
  max-height: 100%;
  overflow-y: auto;
  overflow-x: hidden;
  -webkit-overflow-scrolling: touch;
  overscroll-behavior: contain;
  scrollbar-gutter: stable;
}
.page.active {
  display: flex;
  animation: page-in 0.3s var(--ease-out);
}
@keyframes page-in { from { opacity: 0.7; } to { opacity: 1; } }

/* Cards — bent sheet metal with rust lip */
.card {
  background: linear-gradient(165deg, var(--surface2) 0%, var(--surface) 55%, #141210 100%);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  overflow: hidden;
  display: flex;
  flex-direction: column;
  min-height: 0;
  max-width: 100%;
  flex-shrink: 0;
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.045),
    inset 0 -1px 0 rgba(0, 0, 0, 0.35),
    0 0 0 1px rgba(35, 28, 24, 0.9),
    0 4px 20px rgba(0, 0, 0, 0.42);
}
.card-header {
  padding: 12px 16px;
  background: linear-gradient(180deg, #201c19 0%, #161311 100%);
  border-bottom: 1px solid var(--border);
  font-size: 0.7rem;
  font-weight: 700;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--txt-mid);
  display: flex;
  align-items: center;
  justify-content: space-between;
  flex-shrink: 0;
  box-shadow: inset 0 1px 0 rgba(139, 90, 43, 0.12);
}
.card-body {
  padding: 16px;
  flex: 1 1 auto;
  min-height: 0;
  overflow-x: hidden;
  overflow-y: visible;
}

/* Card chrome: minimize / maximize / hide + home reorder handle */
.card.card--has-chrome > .card-header {
  justify-content: flex-start;
  gap: 8px;
}
.card-header-main {
  flex: 1;
  min-width: 0;
  display: flex;
  align-items: center;
  justify-content: flex-start;
  gap: 8px;
  flex-wrap: wrap;
}
.net-card-head .card-header-main {
  justify-content: space-between;
}
.card-win-actions {
  display: flex;
  align-items: center;
  gap: 2px;
  flex-shrink: 0;
  margin-left: auto;
}
.card-win-btn {
  width: 26px;
  height: 22px;
  padding: 0;
  display: inline-flex;
  align-items: center;
  justify-content: center;
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  background: linear-gradient(180deg, #2a2826 0%, #1a1918 100%);
  color: var(--txt-mid);
  font-size: 0.75rem;
  line-height: 1;
  cursor: pointer;
  transition: border-color var(--dur), color var(--dur), background var(--dur);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.04);
}
.card-win-btn:hover {
  border-color: rgba(212, 168, 52, 0.45);
  color: var(--accent);
}
.card-win-btn--close:hover {
  border-color: rgba(220, 80, 80, 0.55);
  color: #f0b0b0;
}
.card--minimized .card-body {
  display: none !important;
}
.card--minimized.card--has-chrome > .card-header {
  border-bottom-color: transparent;
  box-shadow: none;
}
.card-max-backdrop {
  position: fixed;
  inset: 0;
  z-index: 10000;
  background: rgba(6, 0, 0, 0.72);
  backdrop-filter: blur(3px);
  -webkit-backdrop-filter: blur(3px);
}
.card--maximized {
  position: fixed !important;
  left: max(12px, env(safe-area-inset-left, 0px));
  right: max(12px, env(safe-area-inset-right, 0px));
  top: max(12px, env(safe-area-inset-top, 0px));
  bottom: max(12px, env(safe-area-inset-bottom, 0px));
  width: auto !important;
  max-width: none !important;
  z-index: 10001;
  max-height: none !important;
  min-height: 0;
  box-shadow:
    0 0 0 2px rgba(212, 168, 52, 0.28),
    0 24px 80px rgba(0, 0, 0, 0.65);
}
.card--maximized .card-body {
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  flex: 1;
  min-height: 0;
}
.card-drag-handle {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 1.35rem;
  min-height: 1.35rem;
  cursor: grab;
  color: var(--txt-dim);
  font-size: 0.65rem;
  font-weight: 700;
  letter-spacing: 0.06em;
  user-select: none;
  opacity: 0.55;
  flex-shrink: 0;
  border-radius: var(--radius-sm);
}
.card-drag-handle:hover,
.card-drag-handle:focus-visible {
  opacity: 1;
  color: var(--accent);
  outline: none;
  background: rgba(212, 168, 52, 0.08);
}
.card-drag-handle:active {
  cursor: grabbing;
}
.card--dragging {
  opacity: 0.88;
  outline: 1px dashed rgba(212, 168, 52, 0.55);
  outline-offset: 3px;
}
html[data-reduced-motion="1"] .card-max-backdrop {
  backdrop-filter: none;
  -webkit-backdrop-filter: none;
}

/* Geo-fence (Safety page) */
.geofence-panel .geofence-lead {
  font-size: 0.78rem;
  color: var(--txt-mid);
  line-height: 1.45;
  margin: 0 0 14px;
}
.geofence-check {
  display: flex;
  align-items: center;
  gap: 10px;
  font-size: 0.85rem;
  font-weight: 600;
  color: var(--txt);
  cursor: pointer;
}
.geofence-check input {
  width: 18px;
  height: 18px;
  accent-color: var(--accent);
}
.geofence-grid {
  display: grid;
  grid-template-columns: 1fr;
  gap: 0 14px;
}
@media (min-width: 520px) {
  .geofence-grid {
    grid-template-columns: 1fr 1fr;
  }
}
.geofence-readout {
  font-family: 'Orbitron', monospace;
  font-size: 0.75rem;
  color: var(--accent);
}
.geofence-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
  margin-top: 4px;
  margin-bottom: 14px;
}
.geofence-status {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  gap: 10px 14px;
  padding: 12px 14px;
  border-radius: var(--radius-sm);
  border: 1px solid var(--border);
  background: rgba(0, 0, 0, 0.2);
  font-size: 0.78rem;
  color: var(--txt-mid);
}
.geofence-pill {
  font-size: 0.62rem;
  font-weight: 700;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  padding: 5px 12px;
  border-radius: 999px;
  border: 1px solid var(--border);
  background: var(--surface2);
  color: var(--txt-dim);
}
.geofence-pill--ok {
  color: var(--emerald);
  border-color: rgba(62, 207, 142, 0.45);
  box-shadow: 0 0 10px rgba(62, 207, 142, 0.15);
}
.geofence-pill--warn {
  color: var(--warn);
  border-color: rgba(212, 168, 52, 0.45);
}
.geofence-pill--stop {
  color: var(--stop);
  border-color: rgba(220, 80, 80, 0.5);
  box-shadow: 0 0 12px rgba(220, 80, 80, 0.12);
}
.geofence-detail {
  flex: 1;
  min-width: 0;
  line-height: 1.4;
}

/* Status rows */
.status-row { display: flex; justify-content: space-between; align-items: center;
  padding: 10px 0; border-bottom: 1px solid var(--border); font-size: 0.85rem; }
.status-row:last-child { border-bottom: none; }
.status-row .label { color: var(--txt-dim); }
.status-row .value { font-weight: 600; font-family: Consolas, monospace; }
.status-row .value.ok { color: var(--ok); }
.status-row .value.dim.ok { color: var(--ok); }
.status-row .value.warn { color: var(--warn); }
.status-row .value.dim { color: var(--txt-dim); }

/* Buttons — aged paint on steel */
.btn {
  padding: 10px 16px;
  background: linear-gradient(180deg, #e4c255 0%, #c9a227 42%, #7a6220 100%);
  border: 1px solid var(--rust-dark);
  border-radius: var(--radius-sm);
  color: #1a1408;
  font-size: 0.8rem;
  font-weight: 600;
  cursor: pointer;
  transition: filter var(--dur), box-shadow var(--dur);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.22), 0 2px 6px rgba(0, 0, 0, 0.35);
}
.btn:hover {
  filter: brightness(1.06);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.28), 0 3px 10px rgba(0, 0, 0, 0.4);
}
.btn-ghost {
  background: linear-gradient(165deg, rgba(36, 32, 28, 0.6) 0%, rgba(20, 18, 16, 0.95) 100%);
  border: 1px solid var(--border);
  color: var(--txt-mid);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.04);
}
.btn-ghost:hover {
  border-color: rgba(212, 168, 52, 0.45);
  color: var(--accent);
}
.btn-stop {
  background: linear-gradient(180deg, #e85c5c 0%, #b91c1c 50%, #7f1212 100%);
  color: #fff;
  border-color: #5c1010;
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.2), 0 2px 8px rgba(0, 0, 0, 0.45);
}
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

/* Drive page */
.drive-hero {
  background: linear-gradient(165deg, rgba(245, 166, 35, 0.06) 0%, var(--surface2) 40%, var(--surface) 100%);
  border-color: rgba(245, 166, 35, 0.2);
}
.drive-hero-top {
  display: flex;
  flex-direction: column;
  gap: 14px;
}
.drive-mode-seg {
  display: grid;
  grid-template-columns: 1fr;
  gap: 8px;
}
@media (min-width: 520px) {
  .drive-mode-seg {
    grid-template-columns: repeat(3, 1fr);
  }
}
.drive-mode-btn {
  display: flex !important;
  align-items: center;
  gap: 10px;
  justify-content: flex-start;
  width: 100%;
  min-height: 52px;
  padding: 10px 14px !important;
  border-radius: var(--radius-sm) !important;
  text-align: left;
}
.drive-mode-btn:not(.btn-ghost) {
  border-color: rgba(245, 166, 35, 0.45);
  box-shadow: 0 0 20px rgba(245, 166, 35, 0.12);
}
.drive-mode-ic {
  font-size: 1.1rem;
  opacity: 0.95;
  flex-shrink: 0;
  width: 1.4em;
  text-align: center;
}
.drive-mode-txt {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 2px;
  line-height: 1.2;
}
.drive-mode-txt strong {
  font-size: 0.78rem;
  font-weight: 700;
  letter-spacing: 0.04em;
}
.drive-mode-sub {
  font-size: 0.58rem;
  font-weight: 500;
  color: var(--txt-dim);
  text-transform: uppercase;
  letter-spacing: 0.07em;
}
.drive-link-stack {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 6px;
  padding-top: 4px;
  border-top: 1px solid var(--border);
}
@media (min-width: 520px) {
  .drive-link-stack {
    flex-direction: row;
    align-items: center;
    justify-content: space-between;
  }
}
.drive-ws-badge {
  font-family: 'Orbitron', monospace;
  font-size: 0.62rem;
  font-weight: 700;
  letter-spacing: 0.1em;
  padding: 5px 12px;
  border-radius: 999px;
  background: var(--surface3);
  border: 1px solid var(--border2);
  color: var(--txt-mid);
}
.drive-link-hint {
  font-size: 0.68rem;
  color: var(--txt-dim);
  line-height: 1.35;
  max-width: 48ch;
}
.drive-cockpit-body {
  padding-top: 12px;
}
.drive-cockpit-hint {
  font-size: 0.75rem;
  color: var(--txt-mid);
  line-height: 1.45;
  margin: 0 0 16px;
  padding: 10px 12px;
  background: rgba(0, 0, 0, 0.2);
  border-radius: var(--radius-sm);
  border-left: 3px solid var(--accent);
}
.drive-dual-cockpit {
  display: flex;
  flex-direction: column;
  width: 100%;
  padding: 4px 0 12px;
}
.drive-dual-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px 16px;
  align-items: start;
  width: 100%;
  max-width: 520px;
  margin: 0 auto;
}
@media (max-width: 420px) {
  .drive-dual-row {
    grid-template-columns: 1fr;
    gap: 20px;
  }
}
.drive-dual-col {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
}
.drive-dual-label {
  font-family: 'Orbitron', monospace;
  font-size: 0.62rem;
  font-weight: 700;
  letter-spacing: 0.2em;
  text-transform: uppercase;
  color: var(--txt-dim);
}
.drive-dual-meta {
  font-size: 0.65rem;
  color: var(--txt-mid);
  text-align: center;
}
.drive-head-wrap {
  padding: 8px 0 4px;
}
.drive-head-rim {
  padding: 14px;
}
.drive-head-joy {
  width: 128px;
  height: 128px;
}
.drive-head-joy .joystick-stick {
  width: 44px;
  height: 44px;
  margin: -22px 0 0 -22px;
}
.drive-joystick-wrap {
  padding: 8px 0 20px;
}
.joystick-rim {
  display: inline-block;
  padding: 20px;
  border-radius: 50%;
  background: radial-gradient(circle at 50% 40%, rgba(245, 166, 35, 0.1) 0%, transparent 55%),
    radial-gradient(circle at 50% 100%, rgba(0, 0, 0, 0.45) 0%, transparent 45%);
  border: 1px solid var(--border2);
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.45), inset 0 0 0 1px rgba(255, 255, 255, 0.03);
}
.drive-tank-panel {
  padding: 8px 0 12px;
}
.drive-tank-meters {
  display: flex;
  gap: 12px;
  margin-bottom: 8px;
}
.drive-tank-meter {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 8px;
}
.drive-tank-tag {
  font-family: 'Orbitron', monospace;
  font-size: 0.65rem;
  font-weight: 700;
  color: var(--txt-dim);
  width: 1.25em;
}
.drive-tank-bar {
  flex: 1;
  height: 10px;
  background: var(--border2);
  border-radius: 5px;
  overflow: hidden;
  border: 1px solid var(--border);
}
.drive-tank-fill {
  height: 100%;
  width: 0%;
  background: linear-gradient(90deg, var(--accent-dim), var(--accent));
  border-radius: 5px;
  transition: width 0.08s linear;
}
.drive-tank-fill-r {
  background: linear-gradient(90deg, var(--emerald-dim), var(--emerald));
}
.drive-tank-sliders {
  margin-top: 8px;
}
.drive-tank-roller-hint {
  font-size: 0.72rem;
  color: var(--txt-mid);
  margin: 10px 0 8px;
  text-align: center;
  line-height: 1.4;
}
.drive-tank-rollers {
  display: flex;
  justify-content: center;
  gap: 28px;
  padding: 8px 0 16px;
  touch-action: none;
}
.drive-roller-col {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
}
.drive-roller-label {
  font-size: 0.62rem;
  text-transform: uppercase;
  letter-spacing: 0.12em;
  color: var(--txt-dim);
}
.drive-roller-track {
  position: relative;
  width: 52px;
  height: min(52vw, 220px);
  min-height: 160px;
  border-radius: 26px;
  background: linear-gradient(180deg, var(--surface3) 0%, var(--surface) 100%);
  border: 1px solid var(--border2);
  box-shadow: inset 0 4px 16px rgba(0, 0, 0, 0.45);
  cursor: grab;
}
.drive-roller-track:active {
  cursor: grabbing;
}
.drive-roller-groove {
  position: absolute;
  left: 50%;
  top: 12px;
  bottom: 12px;
  width: 4px;
  margin-left: -2px;
  border-radius: 2px;
  background: rgba(0, 0, 0, 0.35);
}
.drive-roller-thumb {
  position: absolute;
  left: 50%;
  width: 44px;
  height: 28px;
  margin-left: -22px;
  border-radius: 8px;
  background: linear-gradient(180deg, var(--accent) 0%, var(--accent-dim) 100%);
  border: 1px solid rgba(255, 255, 255, 0.12);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.4);
  bottom: 50%;
  transform: translateY(50%);
  pointer-events: none;
}
.drive-roller-col:last-child .drive-roller-thumb {
  background: linear-gradient(180deg, var(--emerald) 0%, var(--emerald-dim) 100%);
}
.drive-motor-hud {
  display: grid;
  grid-template-columns: 1fr auto 1fr;
  gap: 10px;
  align-items: stretch;
  margin-top: 8px;
  padding-top: 14px;
  border-top: 1px solid var(--border);
}
.drive-motor-cell {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 10px 8px;
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
}
.drive-motor-center {
  min-width: 120px;
}
.drive-motor-lbl {
  font-size: 0.58rem;
  text-transform: uppercase;
  letter-spacing: 0.1em;
  color: var(--txt-dim);
}
.drive-motor-val {
  font-size: 1rem;
  font-weight: 700;
  color: var(--accent);
}
.drive-motor-avg {
  font-size: 0.95rem;
  font-weight: 700;
  color: var(--txt);
}
.drive-speed-track {
  width: 100%;
  height: 6px;
  background: var(--border2);
  border-radius: 3px;
  overflow: hidden;
  margin-top: 4px;
}
.drive-speed-fill {
  height: 100%;
  width: 0%;
  background: linear-gradient(90deg, var(--warn), var(--accent));
  border-radius: 3px;
  transition: width 0.08s linear;
}
.drive-grid-two {
  display: grid;
  grid-template-columns: 1fr;
  gap: 16px;
}
@media (min-width: 640px) {
  .drive-grid-two {
    grid-template-columns: 1fr 1fr;
  }
}
.drive-speed-card .card-header,
.drive-mission-card .card-header {
  border-bottom-color: rgba(245, 166, 35, 0.15);
}
.drive-ai-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 10px;
  margin-bottom: 12px;
}
@media (min-width: 480px) {
  .drive-ai-grid {
    grid-template-columns: repeat(3, 1fr);
  }
}
.drive-ai-kpi {
  padding: 10px 12px;
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  min-height: 56px;
}
.drive-ai-kpi-lbl {
  display: block;
  font-size: 0.55rem;
  text-transform: uppercase;
  letter-spacing: 0.08em;
  color: var(--txt-dim);
  margin-bottom: 4px;
}
.drive-ai-kpi-val {
  font-size: 0.8rem;
  font-weight: 600;
  color: var(--txt);
  word-break: break-word;
}
.drive-ai-foot {
  margin: 12px 0 0;
}
.drive-ai-actions {
  margin-top: 0;
}

/* E-Stop base — extended below (fixed, pulse, hold) */

/* Override banner — authority warnings (severity variants) */
#override-banner { display: none; position: fixed; top: 0; left: 0; right: 0; z-index: 999;
  padding: 10px 16px; background: var(--warn); color: #000; font-size: 0.8rem; font-weight: 600; text-align: center;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.35); }
#override-banner.visible { display: block; }
#override-banner.severity--info { background: rgba(59, 130, 246, 0.92); color: #fff; }
#override-banner.severity--warn { background: var(--warn); color: #000; }
#override-banner.severity--danger { background: var(--stop); color: #fff; }
#app:has(#override-banner.visible) { padding-top: 42px; }

/* Global operator strip — control authority + motion (always visible) */
#operator-strip {
  flex-shrink: 0;
  display: flex;
  align-items: stretch;
  padding: 8px 12px;
  background: linear-gradient(180deg, #1a1612 0%, #141210 100%);
  border-bottom: 1px solid var(--border);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.03);
}
.operator-strip-inner {
  display: flex;
  flex-wrap: wrap;
  gap: 6px 8px;
  align-items: center;
  width: 100%;
  row-gap: 8px;
}
.operator-chip {
  display: inline-flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 2px;
  padding: 6px 10px;
  min-height: 44px;
  min-width: 0;
  background: rgba(0, 0, 0, 0.25);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  box-sizing: border-box;
}
.operator-chip--wide { flex: 1 1 120px; }
.operator-chip--lock { flex: 2 1 200px; }
.operator-chip-lbl {
  font-size: 0.55rem;
  text-transform: uppercase;
  letter-spacing: 0.08em;
  color: var(--txt-dim);
}
.operator-chip-val {
  font-family: 'Orbitron', monospace;
  font-size: 0.72rem;
  font-weight: 600;
  color: var(--accent);
  text-shadow: 0 0 12px rgba(245, 166, 35, 0.2);
  max-width: 100%;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
#operator-strip.operator-strip--stale .operator-chip-val { color: var(--warn); }
#operator-strip.operator-strip--stale #op-link-val { color: var(--accent); }
#operator-strip.operator-strip--offline .operator-chip-val { color: var(--txt-dim); }
#operator-strip.operator-strip--offline #op-authority-val { color: var(--stop); }
#operator-strip.operator-strip--locked .operator-chip--lock { border-color: rgba(239, 68, 68, 0.5); }

/* Drive page: dim controls when base reports drive locked (E-stop stays in footer, outside this page) */
.drive-lock-msg {
  margin: 0 0 12px;
  padding: 10px 12px;
  font-size: 0.75rem;
  color: var(--warn);
  background: rgba(0, 0, 0, 0.35);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
}
#page-drive.drive-console-locked .drive-manual-zone,
#page-drive.drive-console-locked #drive-joystick,
#page-drive.drive-console-locked #drive-tank {
  pointer-events: none;
  opacity: 0.55;
  filter: grayscale(0.2);
}
#page-drive.drive-console-locked #drive-deck {
  pointer-events: none;
  opacity: 0.95;
}
#page-drive.drive-console-locked #drive-deck .drive-mode-btn {
  pointer-events: auto;
  opacity: 1;
  filter: none;
}
#page-drive.drive-console-locked #drive-ai-panel {
  opacity: 0.55;
  pointer-events: none;
  filter: grayscale(0.15);
}
#page-drive.drive-console-locked[data-authority="ai"] #drive-ai-panel {
  pointer-events: auto;
  opacity: 1;
  filter: none;
}

/* Grid of nav tiles */
.nav-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 12px; }
.nav-tile {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 20px;
  background: linear-gradient(165deg, var(--surface2) 0%, var(--surface) 100%);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  cursor: pointer;
  transition: border-color var(--dur), background var(--dur), box-shadow var(--dur);
  text-decoration: none;
  color: var(--txt);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.04), 0 2px 12px rgba(0, 0, 0, 0.25);
}
.nav-tile:hover {
  border-color: rgba(212, 168, 52, 0.45);
  background: var(--surface3);
  box-shadow: 0 0 16px rgba(212, 168, 52, 0.1);
}
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
.net-list {
  max-height: min(240px, 40vh);
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  overscroll-behavior: contain;
}
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
.log-list {
  max-height: min(320px, 45vh);
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  overscroll-behavior: contain;
}
.log-item { padding: 8px 12px; border-bottom: 1px solid var(--border); font-size: 0.8rem; }
.log-item .log-time { color: var(--txt-dim); font-size: 0.7rem; margin-right: 8px; }

/* Progress bar */
.progress-bar { height: 8px; background: var(--border2); border-radius: 4px; overflow: hidden; }
.progress-fill { height: 100%; background: var(--accent); border-radius: 4px; transition: width var(--dur); }
.progress-fill.low { background: var(--warn); }
.progress-fill.critical { background: var(--stop); }

/* Operator status strip (node health) */
#status-strip {
  flex-shrink: 0;
  display: flex;
  align-items: center;
  gap: 10px;
  min-height: 44px;
  padding: 6px 12px;
  background: linear-gradient(180deg, #1c1815 0%, #12100e 100%);
  border-bottom: 1px solid var(--border);
  box-shadow: inset 0 1px 0 rgba(139, 90, 43, 0.1), inset 0 -1px 0 rgba(0, 0, 0, 0.35);
}
.status-eye-wrap { flex-shrink: 0; display: flex; align-items: center; justify-content: center; }
.status-eye { width: 30px; height: 30px; border-radius: 50%;
  background: radial-gradient(circle at 35% 35%, #3a3530 0%, #0a0c10 70%);
  border: 2px solid var(--border2);
  box-shadow: 0 0 14px rgba(212,168,52,0.28), inset 0 0 12px rgba(0,0,0,0.6);
  display: flex; align-items: center; justify-content: center;
  animation: eye-hb 2.4s cubic-bezier(0.45,0,0.55,1) infinite; }
.status-eye-lens { width: 10px; height: 12px; border-radius: 50%; background: var(--accent);
  box-shadow: 0 0 10px var(--accent-glow); opacity: 0.95; }
@keyframes eye-hb { 0%,100% { transform: scale(1); box-shadow: 0 0 14px rgba(212,168,52,0.3); }
  50% { transform: scale(1.04); box-shadow: 0 0 22px rgba(212,168,52,0.48); } }
.node-pills { flex: 1; display: flex; flex-wrap: wrap; align-items: center; justify-content: center;
  gap: 6px; min-width: 0; }
.node-pill { font-family: 'Orbitron', monospace; font-size: 0.58rem; font-weight: 700; letter-spacing: 0.06em;
  padding: 4px 8px; border-radius: 999px; border: 1px solid var(--border2); color: var(--txt-dim);
  background: var(--surface2); transition: color var(--dur), border-color var(--dur), transform 0.35s var(--ease-out), opacity var(--dur); }
.node-pill.ok { color: var(--ok); border-color: rgba(61,220,132,0.45); background: rgba(61,220,132,0.08); }
.node-pill.warn { color: var(--warn); border-color: rgba(245,166,35,0.5); }
.node-pill.off { color: var(--txt-dim); opacity: 0.45; border-color: var(--border); }
.node-pill.edge { animation: pill-pop 0.45s var(--ease-out); }
@keyframes pill-pop { from { transform: scale(0.92); opacity: 0.6; } to { transform: scale(1); opacity: 1; } }
.status-right { flex-shrink: 0; display: flex; align-items: center; gap: 10px; }
.status-batt { display: flex; align-items: center; gap: 6px; min-width: 72px; }
.status-batt-icon { font-size: 0.9rem; opacity: 0.9; }
.status-batt-track { flex: 1; width: 56px; height: 8px; background: var(--border2); border-radius: 4px; overflow: hidden; }
.status-batt-fill { height: 100%; width: 0%; background: linear-gradient(90deg, var(--warn), var(--ok));
  border-radius: 4px; transition: width 0.6s cubic-bezier(0.22,1,0.36,1); }
.status-batt-fill.low { background: linear-gradient(90deg, var(--stop), var(--warn)); }
.status-dock-ic { width: 28px; height: 28px; border-radius: 8px; border: 1px solid var(--border2);
  display: flex; align-items: center; justify-content: center; font-size: 0.65rem; color: var(--txt-dim);
  background: var(--surface2); transition: box-shadow var(--dur), border-color var(--dur), color var(--dur); }
.status-dock-ic.charging { color: var(--ok); border-color: rgba(61,220,132,0.5);
  box-shadow: 0 0 16px rgba(61,220,132,0.35); }

/* ─── Dashboard: stat cards, activity, widget grid ─── */
.stat-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; }
@media (min-width: 480px) { .stat-grid { grid-template-columns: repeat(4, 1fr); } }
.stat-card {
  background: linear-gradient(165deg, var(--surface2) 0%, var(--surface) 100%);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  padding: 12px;
  min-height: 72px;
  border-left: 3px solid var(--accent);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.04), 0 2px 10px rgba(0, 0, 0, 0.3);
}
.stat-card.emerald { border-left-color: var(--emerald); }
.stat-label { font-size: 0.62rem; text-transform: uppercase; letter-spacing: 0.08em; color: var(--txt-dim); }
.stat-value { font-family: 'Orbitron', monospace; font-size: 1.1rem; font-weight: 700; color: var(--txt); margin-top: 4px; }
.stat-sub { font-size: 0.65rem; color: var(--txt-mid); margin-top: 2px; }

.activity-feed {
  max-height: min(220px, 42vh);
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  overscroll-behavior: contain;
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  padding: 8px;
  font-size: 0.78rem;
}
.activity-feed .feed-row { padding: 6px 8px; border-bottom: 1px solid var(--border); display: flex; gap: 8px; }
.activity-feed .feed-row:last-child { border-bottom: none; }
.activity-feed .feed-t { color: var(--txt-dim); font-size: 0.65rem; flex-shrink: 0; }
.activity-feed .feed-m { color: var(--txt-mid); }

.widget-grid {
  display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px;
}
@media (min-width: 520px) { .widget-grid { grid-template-columns: repeat(3, 1fr); } }
.widget-tile {
  background: var(--surface2); border: 1px dashed var(--border2); border-radius: var(--radius-sm);
  min-height: 88px; padding: 10px; cursor: grab; position: relative;
  transition: border-color var(--dur), box-shadow var(--dur);
}
.widget-tile:active { cursor: grabbing; }
.widget-tile.dragging { opacity: 0.6; box-shadow: 0 8px 24px rgba(0,0,0,0.45); }
.widget-tile h4 { font-size: 0.62rem; text-transform: uppercase; color: var(--emerald); margin-bottom: 6px; }
.widget-tile .mini { font-size: 0.72rem; color: var(--txt-dim); }

/* WebSocket / connection badges */
#ws-status-badge, .conn-pill {
  font-size: 0.62rem; padding: 4px 10px; border-radius: 999px; border: 1px solid var(--border);
  background: var(--surface2); color: var(--txt-dim); font-family: monospace;
}
#ws-status-badge[data-state="ws"] { border-color: var(--emerald); color: var(--emerald); }
#ws-status-badge[data-state="http"] { border-color: var(--accent); color: var(--accent); }
#ws-status-badge[data-state="error"], #ws-status-badge[data-state="disconnected"] { border-color: var(--stop); color: var(--stop); }

/* Navigation layout */
.nav-layout { display: flex; flex-direction: column; gap: 12px; min-height: 0; }
@media (min-width: 900px) {
  .nav-layout { flex-direction: row; align-items: flex-start; }
  .nav-map-wrap { flex: 1; min-width: 0; min-height: 0; }
  .nav-sidebar {
    width: 260px;
    flex-shrink: 0;
    display: flex;
    flex-direction: column;
    gap: 12px;
    max-height: calc(100svh - var(--shell-scroll-offset));
    max-height: calc(100vh - var(--shell-scroll-offset));
    overflow-x: hidden;
    overflow-y: auto;
    -webkit-overflow-scrolling: touch;
    overscroll-behavior: contain;
    padding-right: 4px;
    scrollbar-gutter: stable;
  }
}
.nav-toolbar {
  display: flex; flex-wrap: wrap; gap: 8px; align-items: center; margin-bottom: 8px;
}
.nav-toolbar--split {
  justify-content: space-between;
  align-items: flex-start;
}
.nav-toolbar-right {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
}
.nav-map-stage {
  display: flex;
  flex-direction: column;
  border-radius: var(--radius);
  border: 1px solid var(--border);
  overflow: hidden;
  background: #0b0f14;
  margin-bottom: 8px;
}
.nav-map-body {
  position: relative;
  flex: 1;
  min-height: min(48vh, 480px);
  width: 100%;
}
.nav-map-controls {
  display: flex;
  flex-wrap: wrap;
  gap: 8px 12px;
  align-items: center;
  padding: 8px 10px;
  background: linear-gradient(180deg, rgba(22, 27, 36, 0.95) 0%, rgba(15, 18, 25, 0.85) 100%);
  border-bottom: 1px solid var(--border);
}
.nav-map-check {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  font-size: 0.72rem;
  color: var(--txt-mid);
  cursor: pointer;
}
.nav-map-field {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  font-size: 0.68rem;
  color: var(--txt-dim);
}
.nav-map-select {
  max-width: 200px;
  padding: 4px 8px;
  border-radius: 6px;
  border: 1px solid var(--border);
  background: var(--surface2);
  color: var(--txt);
  font-size: 0.72rem;
}
.nav-map-hud {
  display: flex;
  flex-wrap: wrap;
  gap: 6px;
  padding: 8px 10px;
  border-bottom: 1px solid var(--border);
  background: rgba(0, 0, 0, 0.2);
}
.nav-hud-chip {
  display: inline-flex;
  flex-direction: column;
  gap: 2px;
  padding: 6px 8px;
  min-width: 0;
  background: rgba(0, 0, 0, 0.25);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
}
.nav-hud-lbl {
  font-size: 0.52rem;
  text-transform: uppercase;
  letter-spacing: 0.06em;
  color: var(--txt-dim);
}
.nav-hud-val {
  font-family: 'Orbitron', ui-monospace, monospace;
  font-size: 0.68rem;
  color: var(--accent);
  max-width: 140px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
.nav-map-fallback {
  padding: 12px 14px;
  font-size: 0.78rem;
  color: var(--warn);
  background: rgba(230, 57, 70, 0.08);
  border-bottom: 1px solid var(--border);
}
.nav-map-container {
  position: absolute;
  inset: 0;
  z-index: 2;
}
.nav-map-container .maplibregl-map {
  font-family: 'Inter', sans-serif;
}
.nav-map-help {
  margin: 6px 0 0;
  line-height: 1.4;
}
#nav-map-canvas {
  position: absolute;
  inset: 0;
  z-index: 1;
  width: 100%;
  height: 100%;
  background: #0a0e14;
  border: none;
  border-radius: 0;
  display: block;
  cursor: crosshair;
  touch-action: none;
}
.waypoint-item { display: flex; align-items: center; justify-content: space-between; padding: 6px 0; border-bottom: 1px solid var(--border); font-size: 0.78rem; }
.waypoint-item button { padding: 2px 8px; }

/* Navigation — world context (internet + router) */
.nav-world-strip {
  display: flex; flex-wrap: wrap; gap: 8px; align-items: center;
  margin-bottom: 4px;
}
.nav-world-hint {
  font-size: 0.72rem; color: var(--txt-dim); line-height: 1.45; margin-bottom: 10px; max-width: 920px;
}
.nav-pill {
  font-size: 0.62rem; padding: 5px 10px; border-radius: 999px; border: 1px solid var(--border);
  background: var(--surface2); color: var(--txt-mid); font-family: ui-monospace, Consolas, monospace;
  max-width: 100%;
}
.nav-pill-ok { border-color: rgba(52, 211, 153, 0.45); color: var(--emerald); }
.nav-pill-warn { border-color: rgba(245, 166, 35, 0.5); color: var(--warn); }
.nav-pill-dim { color: var(--txt-dim); }
.nav-map-real {
  position: relative; width: 100%; border-radius: var(--radius); overflow: hidden;
  border: 1px solid var(--border); background: var(--surface); margin-bottom: 8px; min-height: 120px;
}
.nav-osm-img {
  display: block; width: 100%; height: auto; max-height: 200px; object-fit: cover;
  vertical-align: middle;
}
.nav-osm-placeholder {
  position: absolute; inset: 0; display: flex; align-items: center; justify-content: center;
  padding: 16px; text-align: center; font-size: 0.75rem; color: var(--txt-dim); background: var(--surface2);
}
.nav-world-card .card-body { font-size: 0.82rem; }
.status-row .value.mono { font-family: ui-monospace, Consolas, monospace; font-size: 0.78rem; }
.mono { font-family: ui-monospace, Consolas, monospace; font-size: 0.78rem; }

/* Navigation — mission command deck */
.nav-mission-deck {
  margin-top: 16px;
  padding: 16px;
  background: linear-gradient(165deg, var(--surface2) 0%, var(--surface) 45%, var(--bg) 100%);
  border: 1px solid var(--border);
  border-radius: var(--radius);
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.35);
}
.nav-mission-deck-head {
  display: flex;
  flex-wrap: wrap;
  align-items: flex-start;
  justify-content: space-between;
  gap: 12px;
  margin-bottom: 16px;
}
.nav-mission-h2 {
  font-family: 'Orbitron', ui-monospace, sans-serif;
  font-size: 0.95rem;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--accent);
  margin-bottom: 4px;
}
.nav-mission-sub { font-size: 0.78rem; color: var(--txt-mid); max-width: 52ch; line-height: 1.45; }
.nav-mission-state-pill {
  font-family: ui-monospace, Consolas, monospace;
  font-size: 0.68rem;
  font-weight: 700;
  letter-spacing: 0.12em;
  padding: 8px 14px;
  border-radius: 999px;
  border: 1px solid var(--border);
  background: var(--surface3);
  color: var(--txt-dim);
}
.nav-mission-state-pill.state-idle { color: var(--txt-mid); }
.nav-mission-state-pill.state-armed { color: var(--warn); border-color: rgba(245, 166, 35, 0.45); }
.nav-mission-state-pill.state-running { color: var(--emerald); border-color: rgba(52, 211, 153, 0.4); box-shadow: 0 0 16px var(--emerald-glow); }
.nav-mission-state-pill.state-paused { color: var(--info); }
.nav-mission-state-pill.state-complete { color: var(--emerald); }
.nav-mission-state-pill.state-aborted { color: var(--stop); border-color: rgba(230, 57, 70, 0.4); }

.nav-mission-hero-grid {
  display: grid;
  gap: 16px;
  margin-bottom: 12px;
}
@media (min-width: 720px) {
  .nav-mission-hero-grid {
    grid-template-columns: minmax(0, 1.2fr) auto minmax(0, 1fr);
    align-items: center;
  }
}
.nav-mission-plan-summary { font-size: 0.82rem; color: var(--txt-mid); margin-bottom: 10px; }
.nav-mission-eta-row {
  display: flex;
  flex-wrap: wrap;
  gap: 8px 12px;
  align-items: center;
  font-size: 0.72rem;
}
.nav-mission-ring-wrap {
  position: relative;
  width: 128px;
  height: 128px;
  margin: 0 auto;
  flex-shrink: 0;
}
.nav-mission-ring {
  width: 128px;
  height: 128px;
  border-radius: 50%;
  background: conic-gradient(var(--emerald) calc(var(--p, 0) * 1%), var(--surface3) 0);
  position: relative;
}
.nav-mission-ring::after {
  content: '';
  position: absolute;
  inset: 14px;
  border-radius: 50%;
  background: var(--bg);
  border: 1px solid var(--border);
}
.nav-mission-ring-center {
  position: absolute;
  inset: 0;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  pointer-events: none;
}
.nav-mission-pct { font-family: 'Orbitron', monospace; font-size: 1.1rem; color: var(--txt); }
.nav-mission-pct-label { font-size: 0.58rem; text-transform: uppercase; letter-spacing: 0.15em; color: var(--txt-dim); }

.nav-mission-kpi-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 10px;
}
@media (min-width: 520px) {
  .nav-mission-kpi-grid { grid-template-columns: repeat(3, 1fr); }
}
.nav-mission-kpi {
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  padding: 8px 10px;
}
.nav-mission-kpi-label {
  display: block;
  font-size: 0.58rem;
  text-transform: uppercase;
  letter-spacing: 0.08em;
  color: var(--txt-dim);
  margin-bottom: 4px;
}
.nav-mission-kpi-val { font-size: 0.8rem; font-weight: 600; color: var(--txt); word-break: break-word; }

.nav-mission-batt-warn {
  font-size: 0.72rem;
  color: var(--stop);
  margin: 8px 0 12px;
}

.nav-mission-phase-strip {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 16px;
}
.nav-phase {
  flex: 1;
  min-width: 72px;
  text-align: center;
  padding: 8px 6px;
  border-radius: var(--radius-sm);
  font-size: 0.68rem;
  border: 1px solid var(--border);
  background: var(--surface);
}
.nav-phase span {
  display: block;
  font-size: 0.55rem;
  opacity: 0.7;
  margin-bottom: 2px;
}
.nav-phase-pending { color: var(--txt-dim); }
.nav-phase-active {
  border-color: var(--accent);
  color: var(--accent);
  box-shadow: 0 0 12px var(--accent-glow);
}
.nav-phase-done { border-color: rgba(52, 211, 153, 0.35); color: var(--emerald); }

.nav-mission-split {
  display: grid;
  gap: 12px;
  margin-bottom: 14px;
}
@media (min-width: 800px) {
  .nav-mission-split { grid-template-columns: 1fr 1fr; }
}
.nav-pf-row {
  display: flex;
  align-items: flex-start;
  gap: 10px;
  padding: 6px 0;
  font-size: 0.78rem;
  cursor: pointer;
}
.nav-pf-row input { margin-top: 3px; }
.nav-mission-hint {
  margin-top: 10px;
  font-size: 0.68rem;
  color: var(--txt-dim);
  line-height: 1.45;
}
.nav-mission-log {
  max-height: min(220px, 36vh);
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  font-size: 0.72rem;
  font-family: ui-monospace, Consolas, monospace;
  min-height: 120px;
}
.nav-mission-log-row { padding: 4px 0; border-bottom: 1px solid var(--border); color: var(--txt-mid); }
.nav-mission-log-row.dim { color: var(--txt-dim); border: none; }

.nav-mission-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
}

/* Sparkline canvas */
.spark-wrap { height: 48px; background: var(--surface); border-radius: var(--radius-sm); margin-top: 6px; }

/* AI / Autonomy page */
.ai-brain-strip { margin-bottom: 12px; }
.brain-pill {
  display: inline-block;
  padding: 8px 14px;
  border-radius: 999px;
  border: 1px solid var(--border);
  background: var(--surface3);
  font-family: ui-monospace, Consolas, monospace;
  font-size: 0.72rem;
  font-weight: 700;
  letter-spacing: 0.1em;
  text-transform: uppercase;
  color: var(--accent);
}
.brain-pill.brain-error {
  color: var(--stop);
  border-color: rgba(230, 57, 70, 0.35);
  box-shadow: 0 0 12px var(--stop-glow);
}
.status-row .value.safety-stop { color: var(--stop); font-weight: 700; }

/* AI chat */
.ai-chat-log {
  min-height: 120px;
  max-height: min(220px, 38vh);
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  overscroll-behavior: contain;
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  padding: 10px;
  font-size: 0.8rem;
  margin-bottom: 8px;
}
.ai-chat-row { margin-bottom: 8px; }
.ai-chat-row.user { color: var(--accent); }
.ai-chat-row.bot { color: var(--emerald); }

/* Mission timeline */
.mission-timeline {
  border-left: 2px solid var(--border);
  padding-left: 12px;
  margin: 8px 0;
  max-height: min(280px, 42vh);
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
}
.mission-timeline .step { position: relative; padding-bottom: 12px; font-size: 0.78rem; }
.mission-timeline .step::before {
  content: ''; position: absolute; left: -17px; top: 4px; width: 8px; height: 8px; border-radius: 50%;
  background: var(--accent);
}

/* Settings */
.settings-section { margin-bottom: 16px; }

/* Log filters */
.log-filters { display: flex; flex-wrap: wrap; gap: 6px; margin-bottom: 8px; }
.log-filters select, .log-filters button { font-size: 0.72rem; padding: 4px 8px; }

/* Developer console */
.dev-console {
  font-family: ui-monospace, Consolas, monospace;
  font-size: 0.72rem;
  background: #050608;
  color: #a8f0c8;
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  padding: 10px;
  min-height: 140px;
  max-height: min(220px, 40vh);
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  overscroll-behavior: contain;
  white-space: pre-wrap;
}

/* Vision overlay controls (legacy + toolbar) */
.vision-controls { display: flex; flex-wrap: wrap; gap: 12px; align-items: center; margin-top: 8px; font-size: 0.75rem; }
.vision-controls input[type=range] { width: 120px; }

/* ─── Vision page (camera node · HUD) ─── */
.vision-page {
  gap: 20px;
  background:
    radial-gradient(ellipse 100% 80% at 50% -20%, rgba(253, 128, 32, 0.09) 0%, transparent 55%),
    linear-gradient(180deg, rgba(12, 10, 8, 0.6) 0%, transparent 120px);
}
.vision-page-head {
  display: flex;
  flex-wrap: wrap;
  align-items: flex-start;
  justify-content: space-between;
  gap: 16px;
  padding-bottom: 4px;
  border-bottom: 1px solid rgba(245, 166, 35, 0.12);
}
.vision-kicker {
  font-family: ui-monospace, Consolas, monospace;
  font-size: 0.62rem;
  letter-spacing: 0.22em;
  color: var(--accent);
  margin: 0 0 6px 0;
  opacity: 0.9;
}
.vision-cinematic-title {
  font-size: 1.35rem;
  font-weight: 600;
  letter-spacing: 0.02em;
  background: linear-gradient(110deg, #fff 0%, #f5d4a8 45%, var(--accent) 100%);
  -webkit-background-clip: text;
  background-clip: text;
  color: transparent;
  margin: 0 0 8px 0;
}
.vision-page-lead { max-width: 52rem; margin: 0; line-height: 1.55; color: var(--txt-dim); font-size: 0.82rem; }
.vision-head-badges { display: flex; flex-wrap: wrap; gap: 8px; align-items: center; justify-content: flex-end; }
.vision-pill {
  font-family: ui-monospace, Consolas, monospace;
  font-size: 0.62rem;
  padding: 6px 12px;
  border-radius: 999px;
  border: 1px solid var(--border);
  background: rgba(0, 0, 0, 0.35);
  letter-spacing: 0.06em;
}
.vision-pill.ok { border-color: rgba(52, 211, 153, 0.45); color: #6ee7b7; box-shadow: 0 0 20px rgba(52, 211, 153, 0.12); }
.vision-pill.bad { border-color: rgba(248, 113, 113, 0.4); color: #fca5a5; }
.vision-pill-behave { color: #fde68a; border-color: rgba(253, 186, 116, 0.35); }
.vision-pill-age { color: var(--txt-dim); }

.vision-fpv-stage { display: flex; flex-direction: column; gap: 12px; }
.vision-fpv-wrap {
  position: relative;
  border-radius: 12px;
  border: 1px solid rgba(245, 166, 35, 0.2);
  box-shadow:
    0 0 0 1px rgba(0, 0, 0, 0.5),
    0 12px 48px rgba(0, 0, 0, 0.45),
    inset 0 0 60px rgba(245, 166, 35, 0.04);
  overflow: hidden;
  background: #020203;
}
.vision-fpv-wrap .vision-fpv-img {
  display: block;
  width: 100%;
  height: 100%;
  object-fit: contain;
  transform-origin: center center;
  transition: opacity 0.2s ease;
}
.vision-hud {
  position: absolute;
  inset: 0;
  pointer-events: none;
  z-index: 2;
}
.vision-hud-frame {
  position: absolute;
  inset: 10px;
  border: 1px solid rgba(245, 166, 35, 0.15);
  border-radius: 4px;
}
.vision-hud-crosshair {
  position: absolute;
  left: 50%;
  top: 50%;
  width: 22px;
  height: 22px;
  margin: -11px 0 0 -11px;
  border: 1px solid rgba(255, 255, 255, 0.12);
  border-radius: 50%;
  box-shadow: 0 0 0 1px rgba(0, 0, 0, 0.5);
}
.vision-hud-crosshair::before,
.vision-hud-crosshair::after {
  content: "";
  position: absolute;
  background: rgba(255, 255, 255, 0.2);
}
.vision-hud-crosshair::before { left: 50%; top: 0; bottom: 0; width: 1px; margin-left: -0.5px; }
.vision-hud-crosshair::after { top: 50%; left: 0; right: 0; height: 1px; margin-top: -0.5px; }
.vision-hud-reticle {
  position: absolute;
  left: 50%;
  top: 50%;
  width: 28px;
  height: 28px;
  margin: -14px 0 0 -14px;
  border: 2px solid rgba(253, 128, 32, 0.85);
  border-radius: 50%;
  box-shadow: 0 0 12px rgba(253, 128, 32, 0.35);
  opacity: 0;
  transition: left 0.12s ease-out, top 0.12s ease-out, opacity 0.2s;
}
.vision-hud-reticle.on { opacity: 1; }
.vision-hud-corner {
  position: absolute;
  font-family: ui-monospace, Consolas, monospace;
  font-size: 0.55rem;
  letter-spacing: 0.12em;
  color: rgba(255, 255, 255, 0.35);
  padding: 6px 8px;
}
.vision-hud-tl { top: 0; left: 0; }
.vision-hud-tr { top: 0; right: 0; text-align: right; }
.vision-hud-bl { bottom: 0; left: 0; }
.vision-hud-br { bottom: 0; right: 0; text-align: right; }

.vision-fpv-placeholder {
  position: absolute;
  inset: 0;
  z-index: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 8px;
  text-align: center;
  padding: 24px;
  background: radial-gradient(ellipse at center, rgba(15, 18, 24, 0.92) 0%, #050608 100%);
  color: var(--txt-dim);
  font-size: 0.8rem;
}
.vision-ph-icon { font-size: 2.2rem; line-height: 1; color: rgba(245, 166, 35, 0.35); }
.vision-ph-title { font-weight: 600; color: var(--txt); }
.vision-ph-hint { font-size: 0.72rem; max-width: 18rem; opacity: 0.85; }

.vision-toolbar {
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
}
.vision-toolbar-sliders { margin-top: 0; flex: 1; min-width: 200px; }
.vision-ctl { display: flex; flex-direction: column; gap: 4px; font-size: 0.65rem; color: var(--txt-dim); font-family: ui-monospace, monospace; }
.vision-ctl span { letter-spacing: 0.08em; }
.vision-ctl input[type=range] { width: min(140px, 100%); }
.vision-toolbar-actions { display: flex; flex-wrap: wrap; gap: 8px; }
.vision-btn-primary {
  border-color: rgba(245, 166, 35, 0.45) !important;
  box-shadow: 0 0 20px rgba(245, 166, 35, 0.12);
}

.vision-triage {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
  gap: 12px;
}
.vision-triage-card {
  padding: 14px 16px;
  border-radius: 10px;
  border: 1px solid var(--border);
  background: linear-gradient(145deg, rgba(20, 18, 16, 0.95) 0%, rgba(8, 10, 12, 0.98) 100%);
  position: relative;
  overflow: hidden;
}
.vision-triage-card::before {
  content: "";
  position: absolute;
  top: 0; left: 0; right: 0; height: 2px;
  opacity: 0.6;
}
.vision-triage-event::before { background: linear-gradient(90deg, #f97316, #fb923c); }
.vision-triage-class::before { background: linear-gradient(90deg, #38bdf8, #22d3ee); }
.vision-triage-dist::before { background: linear-gradient(90deg, #a78bfa, #c084fc); }
.vision-triage-label {
  font-size: 0.58rem;
  letter-spacing: 0.22em;
  color: var(--txt-dim);
  display: block;
  margin-bottom: 8px;
}
.vision-triage-value {
  font-family: ui-monospace, Consolas, monospace;
  font-size: 1.05rem;
  font-weight: 600;
  color: #fff;
  display: block;
  line-height: 1.25;
}
.vision-triage-sub { font-size: 0.72rem; color: var(--txt-dim); margin-top: 6px; display: block; }

.vision-panels-grid {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
  gap: 14px;
}
.vision-card .vision-card-h {
  font-family: ui-monospace, monospace;
  letter-spacing: 0.14em;
  font-size: 0.62rem !important;
  color: rgba(245, 166, 35, 0.9);
}
.vision-metric-grid-tight {
  grid-template-columns: repeat(auto-fit, minmax(140px, 1fr));
}
.vision-card-span { grid-column: 1 / -1; }
@media (min-width: 900px) {
  .vision-card-span { grid-column: span 1; }
}
.vision-metric-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 10px 12px;
}
.vision-metric {
  display: flex;
  flex-direction: column;
  gap: 4px;
  padding: 8px 10px;
  border-radius: 8px;
  background: rgba(0, 0, 0, 0.25);
  border: 1px solid rgba(255, 255, 255, 0.04);
}
.vision-metric-wide { grid-column: 1 / -1; }
.vm-label { font-size: 0.58rem; color: var(--txt-dim); letter-spacing: 0.1em; text-transform: uppercase; }
.vm-val { font-size: 0.82rem; font-weight: 500; color: var(--txt); word-break: break-word; }
.vm-mono { font-family: ui-monospace, Consolas, monospace; font-size: 0.78rem; }

.vision-page .fpv-container { aspect-ratio: 4/3; min-height: 200px; }

/* ─── Desktop / large screens (browser + embedded) ─── */
@media (min-width: 900px) {
  html { font-size: 16px; }
  .page { padding: 20px 24px; gap: 20px; }
  #topbar { padding: 0 24px; }
  #topbar-title { font-size: 1rem; }
  #topbar-title span { font-size: 0.7rem; }
  #topbar #conn-badge { font-size: 0.75rem; padding: 6px 14px; }
  #status-strip { padding: 8px 20px; min-height: 48px; }
  .node-pill { font-size: 0.62rem; padding: 5px 10px; }
  #tabbar {
    justify-content: center;
    flex-wrap: wrap;
    padding: 0 16px 4px;
    gap: 4px;
  }
  .tab {
    min-width: 72px;
    padding: 0 14px;
    font-size: 0.62rem;
  }
  .tab svg { width: 20px; height: 20px; }
  .card-header { padding: 14px 18px; font-size: 0.72rem; }
  .card-body { padding: 18px; }
  .nav-grid { grid-template-columns: repeat(3, 1fr); max-width: 960px; margin: 0 auto; width: 100%; }
  .stat-grid { gap: 12px; }
  .activity-feed { max-height: min(260px, 44vh); font-size: 0.82rem; }
  .widget-grid { gap: 12px; }
  .log-list { max-height: min(360px, 50vh); }
  .joystick-container:not(.drive-head-joy) { width: 180px; height: 180px; }
  .joystick-container:not(.drive-head-joy) .joystick-stick { width: 56px; height: 56px; margin: -28px 0 0 -28px; }
  .nav-map-body { max-height: min(55vh, 520px); }
  .ai-chat-log { min-height: 140px; max-height: min(280px, 40vh); }
  .dev-console { max-height: min(280px, 45vh); }
}

@media (min-width: 1200px) {
  body {
    background-color: #080706;
    background-image:
      radial-gradient(ellipse 80% 55% at 50% 0%, rgba(139, 90, 43, 0.15) 0%, transparent 55%),
      radial-gradient(ellipse 70% 50% at 0% 100%, rgba(93, 64, 55, 0.12) 0%, transparent 50%),
      linear-gradient(180deg, #12100e 0%, #0a0c0f 100%);
  }
  #app {
    max-width: 1320px;
    margin-left: auto;
    margin-right: auto;
    border-left: 1px solid var(--border);
    border-right: 1px solid var(--border);
    box-shadow: 0 0 0 1px rgba(40, 32, 28, 0.95), 0 12px 64px rgba(0, 0, 0, 0.55), inset 0 0 80px rgba(139, 90, 43, 0.04);
  }
  #estop-bar {
    border-left: none;
    border-right: none;
  }
  .nav-grid { grid-template-columns: repeat(4, 1fr); max-width: 1100px; }
  .widget-grid { grid-template-columns: repeat(4, 1fr); }
}

@media (min-width: 900px) and (hover: hover) and (pointer: fine) {
  .btn { padding: 11px 18px; font-size: 0.82rem; }
  .btn-small { padding: 6px 12px; font-size: 0.72rem; }
}

/* Command feedback HUD (all pages) */
.command-hud {
  position: fixed;
  bottom: calc(var(--estop-h) + 8px);
  left: 50%;
  transform: translateX(-50%);
  z-index: 850;
  display: flex;
  flex-wrap: wrap;
  align-items: center;
  justify-content: center;
  gap: 6px;
  max-width: calc(100% - 24px);
  pointer-events: none;
}
.hud-pill {
  font-size: 0.65rem;
  font-family: ui-monospace, Consolas, monospace;
  padding: 4px 10px;
  border-radius: 999px;
  background: linear-gradient(165deg, rgba(32, 28, 24, 0.95) 0%, rgba(18, 16, 14, 0.98) 100%);
  border: 1px solid var(--border);
  color: var(--txt-mid);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.04);
}
.hud-pill.hud-cmd.ok { border-color: rgba(61, 220, 132, 0.45); color: var(--ok); }
.hud-pill.hud-cmd.fail { border-color: rgba(230, 57, 70, 0.55); color: var(--stop); }

.feedback-toast {
  position: fixed;
  top: 12px;
  left: 50%;
  transform: translateX(-50%) translateY(-120%);
  z-index: 1100;
  padding: 10px 18px;
  border-radius: var(--radius-sm);
  font-size: 0.78rem;
  font-weight: 600;
  max-width: min(92vw, 480px);
  text-align: center;
  transition: transform 0.35s var(--ease-out);
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.5);
  pointer-events: none;
}
.feedback-toast.visible { transform: translateX(-50%) translateY(0); }
.feedback-toast.ok { background: rgba(61, 220, 132, 0.15); border: 1px solid rgba(61, 220, 132, 0.5); color: var(--ok); }
.feedback-toast.err { background: rgba(230, 57, 70, 0.12); border: 1px solid rgba(230, 57, 70, 0.45); color: #ff8a90; }

/* E-STOP: serious, always visible, pulse */
#estop-bar {
  position: fixed;
  bottom: 0;
  left: 0;
  right: 0;
  z-index: 900;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: var(--estop-h);
  gap: 4px;
  background: linear-gradient(0deg, #12100e 0%, #1a1614 100%);
  border-top: 1px solid var(--border);
  box-shadow: 0 -4px 24px rgba(0, 0, 0, 0.5), inset 0 1px 0 rgba(139, 90, 43, 0.1);
  padding: 8px 8px max(8px, env(safe-area-inset-bottom));
}
.estop-hint {
  font-size: 0.62rem;
  color: var(--txt-dim);
  letter-spacing: 0.06em;
  text-transform: uppercase;
}
#estop-btn {
  width: 100%;
  padding: 12px 24px;
  background: var(--stop);
  border: none;
  border-radius: var(--radius);
  color: #fff;
  font-family: 'Orbitron', monospace;
  font-size: 0.85rem;
  font-weight: 700;
  letter-spacing: 0.15em;
  cursor: pointer;
  transition: filter var(--dur), transform 0.1s;
  animation: pulse-stop 2.4s ease-in-out infinite;
  max-width: min(360px, 100%);
}
#estop-btn:hover { filter: brightness(1.08); }
@keyframes pulse-stop {
  0%, 100% { box-shadow: 0 0 0 rgba(230, 57, 70, 0.35); }
  50% { box-shadow: 0 0 22px rgba(230, 57, 70, 0.75); }
}
#estop-btn.estop-holding {
  transform: scale(0.97);
  filter: brightness(1.15);
  animation: none;
  box-shadow: 0 0 28px rgba(230, 57, 70, 0.95);
}
body.estop-latched #estop-btn {
  background: #7a1520 !important;
  animation: pulse-stop 0.6s ease-in-out infinite;
}

/* Joystick vector (intent arrow) */
.joystick-vector {
  position: absolute;
  left: 50%;
  bottom: 50%;
  width: 3px;
  height: 42%;
  margin-left: -1.5px;
  transform-origin: bottom center;
  background: linear-gradient(to top, transparent, var(--accent));
  border-radius: 2px;
  opacity: 0;
  pointer-events: none;
  transition: opacity 0.08s linear;
}
.joystick-container.joystick-active .joystick-vector { opacity: 0.85; }

/* Brain / AI */
.ai-thinking {
  padding: 8px 12px;
  margin-bottom: 10px;
  border-radius: var(--radius-sm);
  background: rgba(52, 211, 153, 0.08);
  border: 1px dashed rgba(52, 211, 153, 0.35);
  color: var(--emerald);
  font-size: 0.78rem;
  animation: think-pulse 1.8s ease-in-out infinite;
}
@keyframes think-pulse {
  0%, 100% { opacity: 0.75; }
  50% { opacity: 1; }
}
.spark-label { font-size: 0.62rem; color: var(--txt-dim); margin: 8px 0 4px; }

/* Mood → global atmosphere (rust + weathered accents) */
body[data-mood="happy"] { --accent: #d4a834; --accent-glow: rgba(212, 168, 52, 0.22); }
body[data-mood="curious"] { --accent: #5eb3f6; --accent-glow: rgba(94, 179, 246, 0.22); }
body[data-mood="sad"] { --accent: #8b7a6a; --accent-glow: rgba(139, 122, 106, 0.18); }
body[data-mood="tired"] { --accent: #9a8a78; filter: saturate(0.9); }
body[data-mood="sleep"] { --accent: #5c5a58; }
body[data-mood="scared"] { --accent: #d14b4b; }

/* Fixed-height or dense lists — always scroll inside */
.ai-chat-log,
.dev-console,
#nav-waypoint-list,
#mission-queue {
  overflow-y: auto;
  -webkit-overflow-scrolling: touch;
  overscroll-behavior: contain;
}
#nav-waypoint-list {
  max-height: min(200px, 38vh);
}
#mission-queue {
  max-height: min(260px, 45vh);
}

/* ─── Page shell (all tabs) ─── */
.page-head {
  margin-bottom: 12px;
  padding-bottom: 12px;
  border-bottom: 1px solid var(--border);
  flex-shrink: 0;
  box-shadow: 0 1px 0 rgba(139, 90, 43, 0.12);
}
.page-title {
  font-family: 'Orbitron', monospace;
  font-size: 1.1rem;
  font-weight: 700;
  letter-spacing: 0.055em;
  color: var(--txt);
}
.page-lead {
  font-size: 0.82rem;
  color: var(--txt-dim);
  line-height: 1.45;
  margin-top: 6px;
  max-width: 58ch;
}

/* More menu — secondary screens (shared with Vision / Network tone) */
.more-subpage,
.more-hub-page {
  gap: 18px;
  background:
    radial-gradient(ellipse 100% 70% at 50% -15%, rgba(253, 128, 32, 0.06) 0%, transparent 50%),
    linear-gradient(180deg, rgba(12, 10, 8, 0.45) 0%, transparent 100px);
}
.subpage-head {
  display: flex;
  flex-wrap: wrap;
  align-items: flex-start;
  justify-content: space-between;
  gap: 14px;
  margin-bottom: 2px;
  padding-bottom: 14px;
  border-bottom: 1px solid rgba(245, 166, 35, 0.1);
  flex-shrink: 0;
}
.subpage-head-main {
  flex: 1;
  min-width: min(100%, 220px);
}
.subpage-kicker {
  font-family: ui-monospace, Consolas, monospace;
  font-size: 0.6rem;
  letter-spacing: 0.2em;
  color: var(--accent);
  margin: 0 0 6px 0;
  opacity: 0.92;
}
.more-subpage .subpage-title.page-title,
.more-hub-page .subpage-title.page-title {
  font-size: 1.2rem;
  font-weight: 600;
  letter-spacing: 0.04em;
  background: linear-gradient(110deg, #fff 0%, #f5d4a8 42%, var(--accent) 100%);
  -webkit-background-clip: text;
  background-clip: text;
  color: transparent;
  margin: 0 0 6px 0;
}
.subpage-deck {
  display: flex;
  flex-direction: column;
  gap: 14px;
}
@media (min-width: 720px) {
  .subpage-deck--2col {
    display: grid;
    grid-template-columns: repeat(2, minmax(0, 1fr));
    gap: 14px;
    align-items: start;
  }
  .subpage-deck--2col > .subpage-card-span {
    grid-column: 1 / -1;
  }
}

/* Sequence generator — coming soon */
.coming-soon-panel {
  position: relative;
  overflow: hidden;
  min-height: 260px;
}
.coming-soon-panel::before {
  content: '';
  position: absolute;
  inset: 0;
  background: radial-gradient(ellipse 85% 55% at 50% 0%, rgba(245, 166, 35, 0.14) 0%, transparent 58%);
  pointer-events: none;
}
.coming-soon-panel-inner {
  position: relative;
  z-index: 1;
  padding: 10px 8px 6px;
  text-align: center;
}
.coming-soon-badge {
  display: inline-block;
  font-family: 'Orbitron', ui-monospace, monospace;
  font-size: 0.72rem;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  color: var(--accent);
  border: 1px solid rgba(245, 166, 35, 0.42);
  border-radius: 999px;
  padding: 6px 14px;
  margin-bottom: 14px;
  background: rgba(0, 0, 0, 0.35);
  box-shadow: 0 0 24px rgba(245, 166, 35, 0.08);
}
.coming-soon-lead {
  color: var(--txt-mid);
  font-size: 0.9rem;
  line-height: 1.55;
  margin: 0 0 14px;
  max-width: 36em;
  margin-left: auto;
  margin-right: auto;
}
.coming-soon-list {
  text-align: left;
  max-width: 22em;
  margin: 0 auto 16px;
  padding-left: 1.15em;
  color: var(--txt-dim);
  font-size: 0.84rem;
  line-height: 1.65;
}
.coming-soon-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  justify-content: center;
  align-items: center;
}

/* Sequence generator editor */
.seq-toolbar {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  align-items: center;
  margin-bottom: 8px;
}
.seq-select {
  width: 100%;
  max-width: 100%;
  padding: 8px 10px;
  border-radius: 8px;
  border: 1px solid var(--border);
  background: var(--surface2);
  color: var(--txt);
  font-size: 0.85rem;
}
.seq-meta-row {
  display: grid;
  grid-template-columns: auto 1fr;
  gap: 8px 12px;
  align-items: center;
}
.seq-name-input,
.seq-id-input {
  width: 100%;
  padding: 8px 10px;
  border-radius: 8px;
  border: 1px solid var(--border);
  background: var(--surface2);
  color: var(--txt);
  font-size: 0.85rem;
}
.seq-table-wrap {
  overflow-x: auto;
  margin: 10px 0;
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
}
.seq-editor-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 0.78rem;
}
.seq-editor-table th,
.seq-editor-table td {
  padding: 8px 10px;
  border-bottom: 1px solid var(--border);
  vertical-align: top;
}
.seq-editor-table th {
  text-align: left;
  color: var(--txt-dim);
  font-weight: 600;
  text-transform: uppercase;
  font-size: 0.62rem;
  letter-spacing: 0.06em;
}
.seq-editor-table .input-compact {
  width: 100%;
  min-width: 72px;
  padding: 6px 8px;
  border-radius: 6px;
  border: 1px solid var(--border);
  background: var(--surface);
  color: var(--txt);
}
.seq-editor-table textarea {
  width: 100%;
  min-width: 160px;
  min-height: 44px;
  padding: 6px 8px;
  border-radius: 6px;
  border: 1px solid var(--border);
  background: var(--surface);
  color: var(--accent);
  resize: vertical;
}
.seq-editor-table select {
  max-width: 160px;
  padding: 6px 8px;
  border-radius: 6px;
  border: 1px solid var(--border);
  background: var(--surface2);
  color: var(--txt);
}
.seq-status-line {
  font-family: 'Orbitron', ui-monospace, monospace;
  font-size: 0.8rem;
  color: var(--accent);
  margin: 0;
}

.more-subpage .more-deck-card,
.more-hub-page .more-deck-card {
  border-color: rgba(245, 166, 35, 0.14);
  background: linear-gradient(165deg, var(--surface2) 0%, var(--surface) 100%);
  box-shadow:
    inset 0 1px 0 rgba(255, 255, 255, 0.04),
    0 4px 22px rgba(0, 0, 0, 0.22);
}
.more-hub-page .page-head {
  border-bottom-color: rgba(245, 166, 35, 0.12);
  box-shadow: 0 1px 0 rgba(139, 90, 43, 0.1);
}
.more-hub-grid {
  padding: 4px 0 8px;
}
.nav-grid.more-hub-grid {
  gap: 14px;
}
.nav-grid.more-hub-grid .nav-tile {
  min-height: 96px;
  border-radius: 14px;
  border-color: rgba(245, 166, 35, 0.12);
  background: linear-gradient(168deg, rgba(30, 26, 22, 0.95) 0%, var(--surface) 100%);
}
.nav-grid.more-hub-grid .nav-tile:hover {
  border-color: rgba(212, 168, 52, 0.5);
  box-shadow: 0 0 20px rgba(212, 168, 52, 0.12);
}

/* Developer — API block */
.dev-api-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
  gap: 6px 12px;
  font-size: 0.72rem;
  font-family: ui-monospace, Consolas, monospace;
  color: var(--txt-mid);
  line-height: 1.45;
}
.dev-api-grid code {
  display: block;
  padding: 4px 8px;
  border-radius: 6px;
  background: rgba(0, 0, 0, 0.35);
  border: 1px solid var(--border);
}
.dev-raw-row {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  align-items: center;
}
.dev-raw-row input[type='text'] {
  flex: 1;
  min-width: 180px;
}
.soundboard-pad {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  align-items: center;
}

.page-actions,
.dock-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 4px;
}

/* ─── Dock station page ─── */
.dock-hero {
  display: flex;
  flex-direction: column;
  gap: 16px;
  background: linear-gradient(145deg, var(--surface2) 0%, var(--surface) 100%);
  border-color: var(--border2);
  box-shadow: 0 0 0 1px rgba(61, 220, 132, 0.06);
  transition: box-shadow var(--dur), border-color var(--dur);
}
.dock-hero--active {
  border-color: rgba(245, 166, 35, 0.45);
  box-shadow: 0 0 24px rgba(245, 166, 35, 0.12);
}
.dock-hero--charge {
  border-color: rgba(61, 220, 132, 0.5);
  box-shadow: 0 0 28px rgba(61, 220, 132, 0.15);
}
.dock-hero-main {
  display: flex;
  align-items: flex-start;
  gap: 14px;
}
.dock-hero-icon {
  font-size: 2.25rem;
  line-height: 1;
  filter: drop-shadow(0 0 12px rgba(245, 166, 35, 0.35));
}
.dock-fsm-pill {
  display: inline-block;
  font-family: 'Orbitron', monospace;
  font-size: 0.95rem;
  font-weight: 700;
  letter-spacing: 0.12em;
  color: var(--accent);
  padding: 6px 12px;
  border-radius: 999px;
  background: rgba(245, 166, 35, 0.12);
  border: 1px solid rgba(245, 166, 35, 0.35);
}
.dock-hero-sub {
  font-size: 0.8rem;
  color: var(--txt-mid);
  margin-top: 8px;
  line-height: 1.4;
}
.dock-hero-metrics {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 10px;
}
@media (min-width: 520px) {
  .dock-hero-metrics {
    grid-template-columns: repeat(3, 1fr);
  }
}
.dock-metric {
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  padding: 10px 12px;
  min-height: 56px;
}
.dock-metric-label {
  display: block;
  font-size: 0.58rem;
  text-transform: uppercase;
  letter-spacing: 0.08em;
  color: var(--txt-dim);
  margin-bottom: 4px;
}
.dock-metric-val {
  font-size: 0.85rem;
  font-weight: 600;
  color: var(--txt);
}
.dock-grid-two {
  display: grid;
  grid-template-columns: 1fr;
  gap: 16px;
}
@media (min-width: 640px) {
  .dock-grid-two {
    grid-template-columns: 1fr 1fr;
  }
}
.dock-charge-bar-wrap {
  margin-top: 10px;
}
.dock-timeline-card .card-body {
  padding-top: 12px;
}
.dock-timeline {
  list-style: none;
  display: flex;
  flex-direction: column;
  gap: 0;
  counter-reset: dockli;
}
.dock-step {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px 0;
  border-bottom: 1px solid var(--border);
  font-size: 0.85rem;
  color: var(--txt-mid);
  position: relative;
  padding-left: 8px;
  border-left: 3px solid transparent;
  transition: color var(--dur), border-color var(--dur), background var(--dur);
}
.dock-step:last-child {
  border-bottom: none;
}
.dock-step-n {
  flex-shrink: 0;
  width: 28px;
  height: 28px;
  border-radius: 50%;
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 0.72rem;
  font-weight: 700;
  font-family: 'Orbitron', monospace;
  background: var(--surface3);
  border: 1px solid var(--border2);
  color: var(--txt-dim);
}
.dock-step--pending .dock-step-n {
  opacity: 0.55;
}
.dock-step--done {
  color: var(--txt-dim);
  border-left-color: rgba(61, 220, 132, 0.35);
}
.dock-step--done .dock-step-n {
  background: rgba(61, 220, 132, 0.15);
  border-color: rgba(61, 220, 132, 0.45);
  color: var(--ok);
}
.dock-step--current {
  color: var(--txt);
  background: rgba(245, 166, 35, 0.06);
  border-left-color: var(--accent);
  margin-left: -8px;
  padding-left: 16px;
  border-radius: 0 var(--radius-sm) var(--radius-sm) 0;
}
.dock-step--current .dock-step-n {
  background: var(--accent);
  border-color: var(--accent-dim);
  color: #000;
  box-shadow: 0 0 14px var(--accent-glow);
}
.dock-node-strip {
  display: flex;
  align-items: center;
  gap: 12px;
  flex-wrap: wrap;
}
.dock-page-node.node-pill {
  font-size: 0.65rem;
  padding: 6px 14px;
}

/* ─── Settings page ─── */
.settings-layout {
  display: grid;
  grid-template-columns: 1fr;
  gap: 16px;
}
@media (min-width: 720px) {
  .settings-layout {
    grid-template-columns: repeat(2, 1fr);
  }
  .settings-feature.settings-wide {
    grid-column: 1 / -1;
  }
}
.settings-section {
  display: flex;
  flex-direction: column;
  gap: 2px;
}
.field-hint {
  display: block;
  font-size: 0.68rem;
  color: var(--txt-dim);
  margin-top: 4px;
  line-height: 1.35;
}
.settings-btn-row {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-top: 8px;
}
.settings-ping {
  font-size: 0.75rem;
  color: var(--emerald);
  margin-top: 10px;
  min-height: 1.2em;
}
.settings-ping:empty {
  margin-top: 4px;
}
.settings-check {
  display: flex;
  align-items: flex-start;
  gap: 10px;
  font-size: 0.82rem;
  color: var(--txt-mid);
  margin-bottom: 10px;
  cursor: pointer;
}
.settings-check input {
  margin-top: 3px;
}
.settings-node-grid {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
  margin-bottom: 8px;
}
.settings-node-grid .node-pill {
  font-size: 0.62rem;
  padding: 5px 10px;
}

/* UI prefs: dense + reduced motion */
html.ui-dense .page {
  padding: 12px;
  gap: 12px;
}
html.ui-dense .page-head,
html.ui-dense .subpage-head {
  padding-bottom: 8px;
  margin-bottom: 8px;
}
html.ui-dense .card-body {
  padding: 12px;
}
html.ui-dense .status-row {
  padding: 8px 0;
}
html[data-reduced-motion="1"] .page.active {
  animation: none;
}
html[data-reduced-motion="1"] .toast {
  animation: none;
}
html[data-reduced-motion="1"] .face.face--photo[data-mood="curious"] .face-img {
  animation: none;
}
html[data-reduced-motion="1"] .node-pill.edge {
  animation: none;
}

/* Rust theme — text selection & scroll */
::selection {
  background: rgba(212, 168, 52, 0.22);
  color: var(--txt);
}
.page::-webkit-scrollbar {
  width: 10px;
}
.page::-webkit-scrollbar-track {
  background: var(--surface);
  border-radius: 6px;
}
.page::-webkit-scrollbar-thumb {
  background: linear-gradient(180deg, #8b5a2b 0%, #5d4037 100%);
  border-radius: 6px;
  border: 2px solid var(--surface);
}
.page::-webkit-scrollbar-thumb:hover {
  background: linear-gradient(180deg, #a06a35 0%, #6d4c3d 100%);
}

/* ═══ Network page — Fleet uplink console ═══ */
.net-page {
  gap: 20px;
}
.net-page-head .page-title {
  font-family: 'Orbitron', monospace;
  letter-spacing: 0.08em;
}
.net-page-kicker {
  font-size: 0.58rem;
  letter-spacing: 0.38em;
  text-transform: uppercase;
  color: var(--accent);
  margin-bottom: 8px;
  text-shadow: 0 0 20px var(--accent-glow);
}
.net-hero {
  position: relative;
  border-radius: var(--radius);
  overflow: hidden;
  border: 1px solid var(--border2);
  box-shadow:
    0 0 0 1px rgba(0, 0, 0, 0.5),
    0 24px 48px rgba(0, 0, 0, 0.45),
    inset 0 1px 0 rgba(255, 255, 255, 0.04);
}
.net-hero-bg {
  position: absolute;
  inset: 0;
  background:
    radial-gradient(ellipse 90% 80% at 50% -20%, rgba(212, 168, 52, 0.18) 0%, transparent 55%),
    radial-gradient(ellipse 70% 50% at 100% 100%, rgba(197, 48, 48, 0.08) 0%, transparent 45%),
    linear-gradient(165deg, #161311 0%, var(--surface) 100%);
  pointer-events: none;
}
.net-hero-scan {
  position: absolute;
  inset: 0;
  opacity: 0.12;
  background: repeating-linear-gradient(
    0deg,
    transparent,
    transparent 3px,
    rgba(0, 0, 0, 0.2) 3px,
    rgba(0, 0, 0, 0.2) 4px
  );
  animation: net-scan-drift 10s linear infinite;
  pointer-events: none;
}
@keyframes net-scan-drift {
  to {
    transform: translateY(24px);
  }
}
.net-hero-inner {
  position: relative;
  z-index: 1;
  display: grid;
  grid-template-columns: minmax(120px, 160px) 1fr;
  gap: 20px;
  align-items: center;
  padding: 24px 20px 28px;
  max-width: 720px;
  margin: 0 auto;
}
@media (max-width: 520px) {
  .net-hero-inner {
    grid-template-columns: 1fr;
    text-align: center;
    justify-items: center;
  }
}
.net-hero-orbit {
  position: relative;
  width: 140px;
  height: 140px;
  margin: 0 auto;
}
.net-hero-core {
  position: absolute;
  inset: 36%;
  border-radius: 50%;
  background: radial-gradient(circle at 35% 30%, rgba(255, 220, 160, 0.25), transparent 55%),
    radial-gradient(circle at 50% 100%, rgba(0, 0, 0, 0.6), transparent 50%),
    linear-gradient(160deg, var(--surface3), #0c0b0a);
  border: 2px solid var(--accent);
  box-shadow: 0 0 28px var(--accent-glow), inset 0 0 20px rgba(0, 0, 0, 0.5);
}
.net-hero-ring {
  position: absolute;
  inset: 0;
  border-radius: 50%;
  border: 1px solid rgba(212, 168, 52, 0.22);
  pointer-events: none;
}
.net-hero-ring--1 {
  inset: 6%;
  animation: net-ring-pulse 3.2s ease-in-out infinite;
}
.net-hero-ring--2 {
  inset: 18%;
  animation: net-ring-pulse 3.2s ease-in-out infinite 0.4s;
  opacity: 0.75;
}
.net-hero-ring--3 {
  inset: 30%;
  animation: net-ring-pulse 3.2s ease-in-out infinite 0.8s;
  opacity: 0.5;
}
@keyframes net-ring-pulse {
  0%,
  100% {
    transform: scale(1);
    opacity: 0.5;
  }
  50% {
    transform: scale(1.03);
    opacity: 1;
  }
}
.net-hero-orbit[data-link='linked'] .net-hero-core {
  border-color: var(--emerald);
  box-shadow: 0 0 32px var(--emerald-glow), inset 0 0 20px rgba(0, 0, 0, 0.45);
}
.net-hero-orbit[data-link='linked'] .net-hero-ring {
  border-color: rgba(62, 207, 142, 0.35);
}
.net-hero-orbit[data-link='connecting'] .net-hero-core {
  animation: net-core-breathe 1.2s ease-in-out infinite;
}
@keyframes net-core-breathe {
  0%,
  100% {
    box-shadow: 0 0 24px var(--accent-glow);
  }
  50% {
    box-shadow: 0 0 40px rgba(212, 168, 52, 0.45);
  }
}
.net-hero-orbit[data-link='failed'] .net-hero-core {
  border-color: var(--stop);
  box-shadow: 0 0 24px var(--stop-glow);
}
.net-hero-label {
  font-size: 0.58rem;
  letter-spacing: 0.35em;
  text-transform: uppercase;
  color: var(--txt-dim);
  margin-bottom: 6px;
}
.net-hero-state {
  font-family: 'Orbitron', monospace;
  font-size: clamp(1.35rem, 4vw, 1.85rem);
  font-weight: 700;
  letter-spacing: 0.06em;
  color: var(--txt);
  line-height: 1.2;
  margin-bottom: 8px;
}
.net-hero-sub {
  font-size: 0.82rem;
  color: var(--txt-mid);
  line-height: 1.5;
  margin-bottom: 18px;
  max-width: 36em;
}
.net-hero-metrics {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
  margin-bottom: 12px;
}
@media (max-width: 420px) {
  .net-hero-metrics {
    grid-template-columns: 1fr;
  }
}
.net-metric {
  padding: 10px 12px;
  background: rgba(0, 0, 0, 0.28);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
}
.net-metric-lbl {
  display: block;
  font-size: 0.55rem;
  text-transform: uppercase;
  letter-spacing: 0.14em;
  color: var(--txt-dim);
  margin-bottom: 4px;
}
.net-metric-val {
  font-size: 0.95rem;
  color: var(--accent);
}
.net-rssi-track {
  height: 8px;
  border-radius: 4px;
  background: var(--border2);
  overflow: hidden;
  border: 1px solid rgba(0, 0, 0, 0.35);
}
.net-rssi-fill {
  height: 100%;
  width: 0%;
  border-radius: 4px;
  background: linear-gradient(90deg, var(--stop), var(--warn), var(--emerald));
  box-shadow: 0 0 12px rgba(62, 207, 142, 0.25);
  transition: width 0.35s var(--ease-out);
}
.net-rssi-caption {
  font-size: 0.65rem;
  color: var(--txt-dim);
  margin-top: 8px;
  letter-spacing: 0.04em;
}
.net-split {
  display: grid;
  grid-template-columns: 1fr;
  gap: 16px;
}
@media (min-width: 900px) {
  .net-split {
    grid-template-columns: 1.15fr 0.85fr;
    align-items: start;
  }
}
.net-glass-card {
  background: linear-gradient(165deg, rgba(36, 32, 28, 0.95) 0%, var(--surface) 100%);
  border: 1px solid var(--border2);
  box-shadow: inset 0 1px 0 rgba(255, 255, 255, 0.03), 0 8px 32px rgba(0, 0, 0, 0.35);
}
.net-card-head {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  border-bottom-color: rgba(212, 168, 52, 0.12) !important;
}
.net-chip {
  font-size: 0.55rem;
  font-weight: 700;
  letter-spacing: 0.12em;
  text-transform: uppercase;
  padding: 4px 10px;
  border-radius: 999px;
  border: 1px solid var(--border);
  color: var(--txt-dim);
  background: rgba(0, 0, 0, 0.25);
}
.net-chip--live {
  color: var(--emerald);
  border-color: rgba(62, 207, 142, 0.45);
  box-shadow: 0 0 12px var(--emerald-glow);
}
.net-chip--warn {
  color: var(--warn);
  border-color: rgba(212, 168, 52, 0.45);
}
.net-chip--dim {
  opacity: 0.85;
}
.net-topology-body {
  padding: 8px 12px 14px !important;
}
.net-topology-svg {
  width: 100%;
  height: auto;
  display: block;
  max-height: 220px;
}
.net-topo-bg {
  fill: var(--surface);
}
.net-link {
  stroke-dasharray: 8 6;
  animation: net-dash 1.2s linear infinite;
}
.net-page[data-net-state='linked'] .net-link {
  stroke: rgba(62, 207, 142, 0.55);
}
@keyframes net-dash {
  to {
    stroke-dashoffset: -28;
  }
}
.net-node {
  fill: var(--surface2);
  stroke-width: 1.5;
}
.net-node--hub {
  stroke: var(--accent);
  filter: drop-shadow(0 0 8px var(--accent-glow));
}
.net-node--ok {
  stroke: var(--emerald);
}
.net-node--dim {
  stroke: var(--txt-dim);
  opacity: 0.85;
}
.net-node-txt {
  fill: var(--accent);
  font-size: 11px;
  font-family: 'Orbitron', monospace;
  font-weight: 700;
}
.net-node-sub {
  fill: var(--txt-mid);
  font-size: 8px;
}
.net-node-lbl {
  fill: var(--txt);
  font-size: 9px;
}
.net-spec-col {
  display: flex;
  flex-direction: column;
  gap: 16px;
}
.net-spec-row {
  display: flex;
  justify-content: space-between;
  align-items: baseline;
  gap: 12px;
  padding: 10px 0;
  border-bottom: 1px solid rgba(255, 255, 255, 0.04);
}
.net-spec-row:last-of-type {
  border-bottom: none;
}
.net-spec-k {
  font-size: 0.68rem;
  color: var(--txt-dim);
  text-transform: uppercase;
  letter-spacing: 0.1em;
}
.net-spec-v {
  font-size: 0.85rem;
  color: var(--txt);
  text-align: right;
  word-break: break-all;
}
.net-btn-provision {
  width: 100%;
  margin-top: 8px;
}
.net-mission-card .card-body {
  padding-top: 12px;
}
.net-mission-lead {
  font-size: 0.78rem;
  color: var(--txt-mid);
  line-height: 1.5;
  margin-bottom: 16px;
}
.net-mission-actions {
  display: flex;
  flex-wrap: wrap;
  gap: 10px;
  margin-top: 16px;
  align-items: center;
}
.net-list--rich {
  max-height: min(240px, 42vh);
  overflow-y: auto;
  border-radius: var(--radius-sm);
  border: 1px solid var(--border);
  background: rgba(0, 0, 0, 0.2);
}
.net-list--rich .net-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  gap: 12px;
  padding: 12px 14px;
  border-bottom: 1px solid var(--border);
  cursor: pointer;
  transition: background var(--dur);
}
.net-list--rich .net-item:last-child {
  border-bottom: none;
}
.net-list--rich .net-item:hover,
.net-list--rich .net-item.selected {
  background: rgba(212, 168, 52, 0.08);
}
.net-list--rich .net-name {
  font-weight: 600;
  color: var(--txt);
}
.net-list--rich .net-rssi {
  font-size: 0.72rem;
  color: var(--emerald);
  font-variant-numeric: tabular-nums;
}
html[data-reduced-motion='1'] .net-hero-scan,
html[data-reduced-motion='1'] .net-link,
html[data-reduced-motion='1'] .net-hero-ring--1,
html[data-reduced-motion='1'] .net-hero-ring--2,
html[data-reduced-motion='1'] .net-hero-ring--3 {
  animation: none !important;
}
html[data-reduced-motion='1'] .net-hero-orbit[data-link='connecting'] .net-hero-core {
  animation: none !important;
}

/* ═══ Cinematic CYD landing (startup) ═══ */
.cyd-landing {
  position: fixed;
  inset: 0;
  z-index: 10000;
  display: flex;
  align-items: center;
  justify-content: center;
  overflow: hidden;
  -webkit-font-smoothing: antialiased;
}
.cyd-landing[hidden] {
  display: none !important;
}
.cyd-landing-bg {
  position: absolute;
  inset: 0;
  background:
    radial-gradient(ellipse 90% 55% at 50% 15%, rgba(139, 90, 43, 0.28) 0%, transparent 52%),
    radial-gradient(ellipse 100% 70% at 50% 100%, #080706 0%, #000 100%),
    #0a0c0f;
}
.cyd-landing-vignette {
  position: absolute;
  inset: 0;
  box-shadow: inset 0 0 100px 36px rgba(0, 0, 0, 0.88);
  pointer-events: none;
}
.cyd-landing-scan {
  position: absolute;
  inset: 0;
  background: repeating-linear-gradient(
    0deg,
    transparent,
    transparent 2px,
    rgba(0, 0, 0, 0.11) 2px,
    rgba(0, 0, 0, 0.11) 3px
  );
  opacity: 0.4;
  animation: cyd-scan-drift 7s linear infinite;
  pointer-events: none;
}
@keyframes cyd-scan-drift {
  0% {
    transform: translateY(0);
  }
  100% {
    transform: translateY(28px);
  }
}
.cyd-landing-grain {
  position: absolute;
  inset: 0;
  opacity: 0.055;
  pointer-events: none;
  background-image: url("data:image/svg+xml,%3Csvg viewBox='0 0 256 256' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='g'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.9' numOctaves='4' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23g)'/%3E%3C/svg%3E");
}
.cyd-landing-grid {
  position: absolute;
  inset: -40% -20% -20%;
  background-image:
    linear-gradient(rgba(212, 168, 52, 0.045) 1px, transparent 1px),
    linear-gradient(90deg, rgba(212, 168, 52, 0.045) 1px, transparent 1px);
  background-size: 40px 40px;
  transform: perspective(420px) rotateX(58deg);
  transform-origin: 50% 80%;
  opacity: 0;
  pointer-events: none;
}
.cyd-landing--animate .cyd-landing-grid {
  animation: cyd-grid-in 2.2s ease-out 0.2s forwards;
}
@keyframes cyd-grid-in {
  to {
    opacity: 0.4;
  }
}
.cyd-landing-beam {
  position: absolute;
  top: -15%;
  left: -40%;
  width: 90%;
  height: 130%;
  background: linear-gradient(
    100deg,
    transparent 38%,
    rgba(212, 168, 52, 0.07) 50%,
    transparent 62%
  );
  animation: cyd-beam-sweep 6s ease-in-out infinite;
  pointer-events: none;
}
@keyframes cyd-beam-sweep {
  0%,
  100% {
    transform: translateX(-8%) skewX(-2deg);
    opacity: 0.35;
  }
  50% {
    transform: translateX(18%) skewX(2deg);
    opacity: 0.75;
  }
}
.cyd-landing-inner {
  position: relative;
  z-index: 2;
  text-align: center;
  max-width: min(94vw, 540px);
  padding: 28px 20px 32px;
}
.cyd-landing-kicker {
  font-size: 0.58rem;
  letter-spacing: 0.42em;
  text-transform: uppercase;
  color: var(--txt-dim);
  opacity: 0;
  transform: translateY(14px);
}
.cyd-landing--animate .cyd-landing-kicker {
  animation: cyd-fade-up 1s ease-out 0.15s forwards;
}
.cyd-landing-logo {
  font-family: 'Orbitron', monospace;
  font-size: clamp(2.4rem, 11vw, 3.85rem);
  font-weight: 700;
  letter-spacing: 0.06em;
  margin: 14px 0 10px;
  line-height: 1.05;
  opacity: 0;
  transform: scale(0.88);
}
.cyd-landing--animate .cyd-landing-logo {
  animation: cyd-logo-in 1.15s cubic-bezier(0.22, 1, 0.36, 1) 0.35s forwards;
}
.cyd-landing-logo-w {
  color: var(--txt);
}
.cyd-landing-logo-dot {
  color: var(--accent);
  margin: 0 0.04em;
  text-shadow: 0 0 20px var(--accent-glow);
}
.cyd-landing-logo-e {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 1.12em;
  height: 1.12em;
  margin-left: 0.06em;
  border-radius: 50%;
  background: radial-gradient(circle at 30% 30%, #e04545 0%, #9b1818 55%, #5c0a0a 100%);
  color: #fff;
  font-size: 0.82em;
  vertical-align: middle;
  box-shadow: 0 0 28px rgba(185, 40, 40, 0.55), inset 0 -2px 6px rgba(0, 0, 0, 0.35);
}
.cyd-landing-sub {
  font-size: 0.82rem;
  color: var(--txt-mid);
  letter-spacing: 0.04em;
  line-height: 1.45;
  opacity: 0;
  transform: translateY(10px);
}
.cyd-landing--animate .cyd-landing-sub {
  animation: cyd-fade-up 0.95s ease-out 0.85s forwards;
}
.cyd-landing-cyd-block {
  margin: 26px 0 8px;
}
.cyd-landing-eye-rig {
  opacity: 0;
  transform: translateY(28px) scale(0.88);
}
.cyd-landing--animate .cyd-landing-eye-rig {
  animation: cyd-eye-rig 1.35s cubic-bezier(0.22, 1, 0.36, 1) 0.95s forwards;
}
.cyd-landing-eye-housing {
  display: inline-flex;
  gap: 12px;
  padding: 14px 18px 12px;
  background: linear-gradient(175deg, #2c2824 0%, #141210 100%);
  border-radius: 22px;
  border: 1px solid var(--border);
  box-shadow:
    inset 0 2px 14px rgba(0, 0, 0, 0.65),
    0 0 0 1px rgba(0, 0, 0, 0.5),
    0 16px 48px rgba(0, 0, 0, 0.55);
}
.cyd-landing-lens {
  width: 42px;
  height: 50px;
  border-radius: 50%;
  background: radial-gradient(ellipse 45% 50% at 40% 38%, #2a2826 0%, #0d0c0b 65%, #000 100%);
  border: 2px solid #2a2622;
  position: relative;
  overflow: hidden;
}
.cyd-landing-lens-glare {
  position: absolute;
  inset: -20%;
  background: linear-gradient(118deg, transparent 38%, rgba(212, 168, 52, 0.45) 48%, transparent 58%);
  animation: cyd-glare 2.8s ease-in-out infinite;
}
.cyd-landing-lens-glare--delay {
  animation-delay: 1.1s;
}
@keyframes cyd-glare {
  0%,
  100% {
    transform: translateX(-35%) rotate(-6deg);
    opacity: 0.35;
  }
  50% {
    transform: translateX(35%) rotate(4deg);
    opacity: 0.95;
  }
}
.cyd-landing-neck {
  width: 36px;
  height: 14px;
  margin: 0 auto;
  background: linear-gradient(90deg, #3a3530, #1a1816, #3a3530);
  border-radius: 0 0 10px 10px;
  box-shadow: inset 0 2px 4px rgba(0, 0, 0, 0.5);
}
.cyd-landing-cyd-meta {
  margin-top: 14px;
}
.cyd-landing-cyd-tag {
  font-family: 'Orbitron', monospace;
  font-size: 0.88rem;
  font-weight: 700;
  letter-spacing: 0.45em;
  color: var(--accent);
  text-shadow: 0 0 22px var(--accent-glow);
}
.cyd-landing-cyd-sub {
  display: block;
  font-size: 0.68rem;
  color: var(--txt-dim);
  margin-top: 8px;
  letter-spacing: 0.06em;
}
.cyd-landing-boot {
  margin-top: 22px;
  padding: 14px 16px;
  text-align: left;
  background: rgba(0, 0, 0, 0.35);
  border: 1px solid var(--border);
  border-radius: var(--radius-sm);
  box-shadow: inset 0 0 24px rgba(0, 0, 0, 0.4);
}
.cyd-landing-line {
  font-family: ui-monospace, 'Cascadia Mono', Consolas, monospace;
  font-size: 0.7rem;
  color: var(--txt-mid);
  margin: 5px 0;
  opacity: 0;
  transform: translateX(-10px);
}
.cyd-landing--animate .cyd-landing-line--1 {
  animation: cyd-line-in 0.55s ease-out 1.65s forwards;
}
.cyd-landing--animate .cyd-landing-line--2 {
  animation: cyd-line-in 0.55s ease-out 2.15s forwards;
}
.cyd-landing--animate .cyd-landing-line--3 {
  animation: cyd-line-in 0.55s ease-out 2.65s forwards;
}
.cyd-landing--animate .cyd-landing-line--4 {
  animation: cyd-line-in 0.6s ease-out 3.25s forwards;
}
@keyframes cyd-line-in {
  to {
    opacity: 1;
    transform: translateX(0);
  }
}
.cyd-landing-prompt {
  color: var(--accent);
  margin-right: 6px;
  font-weight: 600;
}
.cyd-landing-progress {
  margin: 18px 0 6px;
  opacity: 0;
  transform: translateY(8px);
}
.cyd-landing--animate .cyd-landing-progress {
  animation: cyd-fade-up 0.75s ease 1.1s forwards;
}
.cyd-landing-progress-track {
  height: 5px;
  background: var(--border);
  border-radius: 3px;
  overflow: hidden;
  border: 1px solid rgba(0, 0, 0, 0.35);
}
.cyd-landing-progress-fill {
  height: 100%;
  width: 0%;
  border-radius: 3px;
  background: linear-gradient(90deg, #5d4037, var(--accent), #e8c255);
  box-shadow: 0 0 14px rgba(212, 168, 52, 0.35);
}
.cyd-landing--animate .cyd-landing-progress-fill {
  animation: cyd-progress 4.4s cubic-bezier(0.22, 1, 0.36, 1) 0.9s forwards;
}
@keyframes cyd-progress {
  to {
    width: 100%;
  }
}
.cyd-landing-progress-label {
  display: block;
  font-size: 0.58rem;
  letter-spacing: 0.14em;
  text-transform: uppercase;
  color: var(--txt-dim);
  margin-top: 8px;
}
.cyd-landing-actions {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
  margin-top: 22px;
  opacity: 0;
  transform: translateY(18px);
}
.cyd-landing--animate .cyd-landing-actions {
  animation: cyd-fade-up 0.85s ease 3.45s forwards;
}
.cyd-landing-enter {
  min-width: 232px;
  font-size: 0.82rem;
  letter-spacing: 0.06em;
}
.cyd-landing-skip {
  background: transparent;
  border: none;
  color: var(--txt-dim);
  font-size: 0.68rem;
  letter-spacing: 0.04em;
  cursor: pointer;
  padding: 10px 16px;
  text-decoration: underline;
  text-underline-offset: 3px;
}
.cyd-landing-skip:hover {
  color: var(--accent);
}
@keyframes cyd-fade-up {
  to {
    opacity: 1;
    transform: translateY(0);
  }
}
@keyframes cyd-logo-in {
  to {
    opacity: 1;
    transform: scale(1);
  }
}
@keyframes cyd-eye-rig {
  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}
.cyd-landing--exit {
  pointer-events: none;
  animation: cyd-landing-exit 0.88s cubic-bezier(0.55, 0, 0.15, 1) forwards;
}
@keyframes cyd-landing-exit {
  to {
    opacity: 0;
    transform: scale(1.03);
    filter: blur(8px);
  }
}
.cyd-landing--reduced .cyd-landing-scan,
.cyd-landing--reduced .cyd-landing-beam,
.cyd-landing--reduced .cyd-landing-lens-glare {
  animation: none !important;
}
.cyd-landing--reduced .cyd-landing-grid {
  opacity: 0.35;
  animation: none !important;
}
.cyd-landing--reduced .cyd-landing-kicker,
.cyd-landing--reduced .cyd-landing-logo,
.cyd-landing--reduced .cyd-landing-sub,
.cyd-landing--reduced .cyd-landing-eye-rig,
.cyd-landing--reduced .cyd-landing-line,
.cyd-landing--reduced .cyd-landing-progress,
.cyd-landing--reduced .cyd-landing-actions {
  animation: none !important;
  opacity: 1 !important;
  transform: none !important;
}
.cyd-landing--reduced .cyd-landing-progress-fill {
  width: 100% !important;
  animation: none !important;
}
body.lros-app-revealed #app {
  animation: lros-app-bloom 0.6s ease-out;
}
@keyframes lros-app-bloom {
  from {
    opacity: 0.92;
    filter: saturate(0.85);
  }
  to {
    opacity: 1;
    filter: none;
  }
}

</style>
  <link rel="manifest" href="manifest.json">
  <meta name="mobile-web-app-capable" content="yes">
</head>
<body>
<!-- Cinematic CYD startup (dismiss → initAll) -->
<div id="cyd-landing" class="cyd-landing" role="dialog" aria-modal="true" aria-labelledby="cyd-landing-title" aria-describedby="cyd-landing-desc" hidden>
  <div class="cyd-landing-bg" aria-hidden="true"></div>
  <div class="cyd-landing-vignette" aria-hidden="true"></div>
  <div class="cyd-landing-scan" aria-hidden="true"></div>
  <div class="cyd-landing-grain" aria-hidden="true"></div>
  <div class="cyd-landing-grid" aria-hidden="true"></div>
  <div class="cyd-landing-beam" aria-hidden="true"></div>
  <div class="cyd-landing-inner">
    <p class="cyd-landing-kicker">BnL Ship · Waste Allocation</p>
    <h1 id="cyd-landing-title" class="cyd-landing-logo"><span class="cyd-landing-logo-w">WALL</span><span class="cyd-landing-logo-dot">·</span><span class="cyd-landing-logo-e">E</span></h1>
    <p id="cyd-landing-desc" class="cyd-landing-sub">Living Robot Operating System · remote console</p>
    <div class="cyd-landing-cyd-block">
      <div class="cyd-landing-eye-rig" aria-hidden="true">
        <div class="cyd-landing-eye-housing">
          <span class="cyd-landing-lens"><span class="cyd-landing-lens-glare"></span></span>
          <span class="cyd-landing-lens"><span class="cyd-landing-lens-glare cyd-landing-lens-glare--delay"></span></span>
        </div>
        <div class="cyd-landing-neck"></div>
      </div>
      <div class="cyd-landing-cyd-meta">
        <span class="cyd-landing-cyd-tag">CYD</span>
        <span class="cyd-landing-cyd-sub">Master touchscreen · command surface</span>
      </div>
    </div>
    <div class="cyd-landing-boot" aria-live="polite">
      <p class="cyd-landing-line cyd-landing-line--1"><span class="cyd-landing-prompt">&gt;</span> Handshake with base…</p>
      <p class="cyd-landing-line cyd-landing-line--2"><span class="cyd-landing-prompt">&gt;</span> Fleet mesh standby…</p>
      <p class="cyd-landing-line cyd-landing-line--3"><span class="cyd-landing-prompt">&gt;</span> CYD optics calibrated…</p>
      <p class="cyd-landing-line cyd-landing-line--4"><span class="cyd-landing-prompt">&gt;</span> <strong>Link ready.</strong></p>
    </div>
    <div class="cyd-landing-progress" aria-hidden="true">
      <div class="cyd-landing-progress-track"><div class="cyd-landing-progress-fill"></div></div>
      <span class="cyd-landing-progress-label">Establishing secure channel</span>
    </div>
    <div class="cyd-landing-actions">
      <button type="button" class="btn cyd-landing-enter" id="cyd-landing-enter">Initialize console</button>
      <button type="button" class="cyd-landing-skip" id="cyd-landing-skip">Skip intro</button>
    </div>
  </div>
</div>

<div id="app">
  <div id="override-banner" aria-live="polite">Local Control Active - CYD touchscreen has control</div>

  <div id="operator-strip" role="region" aria-label="Control authority and motion">
    <div class="operator-strip-inner">
      <div class="operator-chip" title="Who currently owns drive / motion authority">
        <span class="operator-chip-lbl">Authority</span>
        <span class="operator-chip-val" id="op-authority-val">—</span>
      </div>
      <div class="operator-chip" title="CYD vs browser drive policy (any / cyd_only / web_only)">
        <span class="operator-chip-lbl">Policy</span>
        <span class="operator-chip-val" id="op-policy-val">—</span>
      </div>
      <div class="operator-chip" title="High-level motion state from base">
        <span class="operator-chip-lbl">Motion</span>
        <span class="operator-chip-val" id="op-motion-val">—</span>
      </div>
      <div class="operator-chip" title="Drive profile ramp / limits">
        <span class="operator-chip-lbl">Drive profile</span>
        <span class="operator-chip-val" id="op-profile-val">—</span>
      </div>
      <div class="operator-chip" title="EVE companion UART (base ↔ EVE)">
        <span class="operator-chip-lbl">EVE</span>
        <span class="operator-chip-val" id="op-eve-val">—</span>
      </div>
      <div class="operator-chip" title="Browser ↔ base link (HTTP + optional WebSocket)">
        <span class="operator-chip-lbl">Link</span>
        <span class="operator-chip-val" id="op-link-val">—</span>
      </div>
      <div class="operator-chip operator-chip--wide" title="Last command freshness">
        <span class="operator-chip-lbl">Command age</span>
        <span class="operator-chip-val" id="op-fresh-val">—</span>
      </div>
      <div class="operator-chip operator-chip--lock" title="Why controls may be disabled">
        <span class="operator-chip-lbl">Lock</span>
        <span class="operator-chip-val" id="op-lock-val">—</span>
      </div>
    </div>
  </div>

  <div id="status-strip" role="region" aria-label="Fleet status">
    <div class="status-eye-wrap" title="Heartbeat">
      <div id="status-eye" class="status-eye"><span class="status-eye-lens"></span></div>
    </div>
    <div class="node-pills" id="node-pills">
      <span class="node-pill ok" data-pill="base" title="Base">BASE</span>
      <span class="node-pill off" data-pill="master" title="CYD Master">CYD</span>
      <span class="node-pill off" data-pill="audio" title="Audio">AUD</span>
      <span class="node-pill off" data-pill="dock" title="Dock">DOCK</span>
      <span class="node-pill off" data-pill="vision" title="Vision">VIS</span>
      <span class="node-pill off" data-pill="eve" title="EVE (UART)">EVE</span>
    </div>
    <div class="status-right">
      <div class="status-batt" id="status-batt" title="Battery">
        <span class="status-batt-icon">&#9889;</span>
        <div class="status-batt-track"><div class="status-batt-fill" id="status-batt-fill"></div></div>
      </div>
      <div class="status-dock-ic" id="status-dock-ic" title="Dock charge">&#9632;</div>
    </div>
  </div>

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
    <div class="tab" data-tab="settings" onclick="switchTab('settings')"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/></svg>Set</div>
    <div class="tab" data-tab="more" onclick="switchTab('more')"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><circle cx="12" cy="12" r="1"/><circle cx="19" cy="12" r="1"/><circle cx="5" cy="12" r="1"/></svg>More</div>
  </nav>

  <main id="pages">
    <!-- HOME -->
    <div class="page active" id="page-home">
      <header class="page-head">
        <h2 class="page-title">Dashboard</h2>
        <p class="page-lead">At-a-glance status, laser, widgets, and quick links to drive and dock.</p>
      </header>
      <div class="face-container">
        <figure class="face face--photo" id="walle-face" data-mood="happy" aria-label="WALL-E">
          <span class="face-frame">
            <img class="face-img" src="assets/walle-character.png" alt="WALL-E" width="260" height="260" decoding="async" loading="eager">
          </span>
        </figure>
      </div>
      <div class="stat-grid" id="home-stats">
        <div class="stat-card"><div class="stat-label">Battery</div><div class="stat-value" id="stat-batt">-</div><div class="stat-sub" id="stat-batt-sub">-</div></div>
        <div class="stat-card emerald"><div class="stat-label">Speed</div><div class="stat-value" id="stat-speed">0</div><div class="stat-sub">avg L/R</div></div>
        <div class="stat-card"><div class="stat-label">Heading</div><div class="stat-value" id="stat-heading">-</div><div class="stat-sub">IMU</div></div>
        <div class="stat-card emerald"><div class="stat-label">Wi‑Fi</div><div class="stat-value" id="stat-rssi">-</div><div class="stat-sub" id="stat-wifi-sub">signal</div></div>
      </div>
      <div class="card">
        <div class="card-header">Quick Status</div>
        <div class="card-body">
          <div class="status-row"><span class="label">State</span><span class="value" id="home-state">-</span></div>
          <div class="status-row"><span class="label">Emotion / mood</span><span class="value" id="home-emotion">-</span></div>
          <div class="progress-bar" style="margin:8px 0"><div class="progress-fill" id="home-battery-bar" style="width:80%"></div></div>
          <div class="status-row"><span class="label">Voltage</span><span class="value" id="home-battery">-</span></div>
        </div>
      </div>
      <div class="card" id="brain-card">
        <div class="card-header">System brain</div>
        <div class="card-body">
          <div class="ai-thinking" id="ai-thinking" hidden>WALL-E is thinking…</div>
          <div class="status-row"><span class="label">AI decision</span><span class="value" id="ai-decision">—</span></div>
          <div class="status-row"><span class="label">Confidence</span><span class="value ok" id="ai-confidence">—</span></div>
          <div class="status-row"><span class="label">Interest</span><span class="value" id="ai-interest">—</span></div>
        </div>
      </div>
      <div class="card" id="laser-card">
        <div class="card-header">Laser pointer</div>
        <div class="card-body">
          <p class="stat-sub" style="margin:0 0 8px">Laser on base GPIO (default 18 in firmware) · aim = pan + tilt 0–100</p>
          <div class="status-row"><span class="label">Beam</span><span class="value" id="laser-status">Off</span></div>
          <div class="form-group"><label for="laser-pan">Pan</label><input type="range" id="laser-pan" min="0" max="100" value="50"></div>
          <div class="form-group"><label for="laser-tilt">Tilt</label><input type="range" id="laser-tilt" min="0" max="100" value="50"></div>
          <div class="form-group"><label for="laser-bright">Brightness</label><input type="range" id="laser-bright" min="0" max="255" value="0"></div>
          <div class="form-group"><label for="laser-fire-ms">Fire duration (ms)</label><input type="number" id="laser-fire-ms" min="100" max="10000" value="1000" style="width:100%;max-width:120px"></div>
          <div style="display:flex;gap:8px;flex-wrap:wrap;margin-top:10px">
            <button type="button" class="btn btn-small" id="laser-toggle-btn" onclick="laserToggleOnOff()">Laser ON</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="laserApplyAim()">Aim</button>
            <button type="button" class="btn btn-small" onclick="laserPointFire()">Point &amp; fire</button>
          </div>
          <div style="display:flex;gap:8px;align-items:center;flex-wrap:wrap;margin-top:10px">
            <button type="button" class="btn btn-small btn-ghost" id="laser-scan-btn" onclick="laserScanToggle()">Scan</button>
            <label for="laser-mood" class="stat-sub">Mood</label>
            <select id="laser-mood" onchange="laserMoodApply()">
              <option value="0">Curious</option>
              <option value="1">Angry</option>
              <option value="2">Happy</option>
            </select>
          </div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Activity feed</div>
        <div class="card-body" style="padding:0">
          <div class="activity-feed" id="activity-feed"></div>
        </div>
      </div>
      <div class="card">
        <div class="card-header">Widgets <span style="font-weight:400;color:var(--txt-dim);font-size:0.65rem">drag to reorder</span></div>
        <div class="card-body">
          <div class="widget-grid" id="widget-grid">
            <div class="widget-tile" draggable="true" data-widget="cam"><h4>Camera</h4><div class="mini">Vision stream</div></div>
            <div class="widget-tile" draggable="true" data-widget="map"><h4>Map</h4><div class="mini">Tap More → Nav</div></div>
            <div class="widget-tile" draggable="true" data-widget="power"><h4>Power</h4><div class="mini" id="w-power">-</div></div>
            <div class="widget-tile" draggable="true" data-widget="tel"><h4>Telemetry</h4><div class="mini">IMU / sonar</div></div>
            <div class="widget-tile" draggable="true" data-widget="voice"><h4>Voice</h4><div class="mini">Audio page</div></div>
            <div class="widget-tile" draggable="true" data-widget="quick"><h4>Quick actions</h4><div class="mini"><button type="button" class="btn btn-small" onclick="switchTab('drive')">Drive</button></div></div>
          </div>
        </div>
      </div>
      <div class="page-actions">
        <button class="btn" onclick="switchTab('drive')">Drive</button>
        <button class="btn btn-ghost" onclick="switchTab('docking')">Docking</button>
        <button class="btn btn-ghost" onclick="switchTab('network')">Network</button>
      </div>
    </div>

    <!-- DRIVE -->
    <div class="page" id="page-drive" data-mode="joystick">
      <header class="page-head">
        <h2 class="page-title">Drive</h2>
        <p class="page-lead">CYD-style control: head pan/tilt + drive mix, dual tread rollers, or AI assist with manual override.</p>
      </header>

      <div class="drive-lock-msg" id="drive-lock-msg" role="status" aria-live="polite" hidden></div>

      <div class="drive-hero card">
        <div class="card-body drive-hero-top">
          <div class="drive-mode-seg" id="drive-deck" data-active-mode="joystick" role="tablist" aria-label="Drive mode">
            <button type="button" class="btn btn-small drive-mode-btn" id="mode-joystick" onclick="setDriveMode('joystick')" role="tab" aria-selected="true">
              <span class="drive-mode-ic" aria-hidden="true">&#9678;</span>
              <span class="drive-mode-txt"><strong>Joystick</strong><span class="drive-mode-sub">Head + drive</span></span>
            </button>
            <button type="button" class="btn btn-small btn-ghost drive-mode-btn" id="mode-tank" onclick="setDriveMode('tank')" role="tab" aria-selected="false">
              <span class="drive-mode-ic" aria-hidden="true">&#9618;</span>
              <span class="drive-mode-txt"><strong>Tank</strong><span class="drive-mode-sub">Dual rollers</span></span>
            </button>
            <button type="button" class="btn btn-small btn-ghost drive-mode-btn" id="mode-ai" onclick="setDriveMode('ai')" role="tab" aria-selected="false">
              <span class="drive-mode-ic" aria-hidden="true">&#9889;</span>
              <span class="drive-mode-txt"><strong>AI assist</strong><span class="drive-mode-sub">Brain + override</span></span>
            </button>
          </div>
          <div class="drive-link-stack">
            <span class="drive-ws-badge" id="ws-status-badge" title="WebSocket or HTTP fallback">HTTP</span>
            <span class="drive-link-hint">Commands use <span class="mono">/drive</span> · failsafe ~440&nbsp;ms</span>
          </div>
        </div>
      </div>

      <div class="drive-manual-zone" id="drive-manual-zone">
        <div class="card drive-cockpit">
          <div class="card-header">Manual output</div>
          <div class="card-body drive-cockpit-body">
            <p class="drive-cockpit-hint" id="drive-cockpit-hint">Head: aim the face. Drive: same mix as CYD — forward/back both tracks, left/right turn.</p>
            <div id="drive-joystick" class="drive-dual-cockpit">
              <div class="drive-dual-row">
                <div class="drive-dual-col drive-dual-col--head">
                  <span class="drive-dual-label">Head</span>
                  <div class="joystick-wrap drive-head-wrap">
                    <div class="joystick-rim drive-head-rim">
                      <div class="joystick-container drive-head-joy" id="head-joystick" aria-label="Head pan and tilt">
                        <div class="joystick-vector" id="head-vector" aria-hidden="true"></div>
                        <div class="joystick-stick" id="head-stick"></div>
                      </div>
                    </div>
                  </div>
                  <span class="drive-dual-meta mono" id="head-servo-readout" aria-live="polite">Pan — · Tilt —</span>
                </div>
                <div class="drive-dual-col drive-dual-col--drive">
                  <span class="drive-dual-label">Drive</span>
                  <div class="joystick-wrap drive-joystick-wrap">
                    <div class="joystick-rim">
                      <div class="joystick-container" id="joystick" aria-label="Drive joystick">
                        <div class="joystick-vector" id="joystick-vector" aria-hidden="true"></div>
                        <div class="joystick-stick" id="joystick-stick"></div>
                      </div>
                    </div>
                  </div>
                </div>
              </div>
            </div>
            <div id="drive-tank" class="drive-tank-panel" style="display:none">
              <div class="drive-tank-meters">
                <div class="drive-tank-meter">
                  <span class="drive-tank-tag">L</span>
                  <div class="drive-tank-bar"><div class="drive-tank-fill" id="drive-tank-l-bar" style="width:0%"></div></div>
                </div>
                <div class="drive-tank-meter">
                  <span class="drive-tank-tag">R</span>
                  <div class="drive-tank-bar"><div class="drive-tank-fill drive-tank-fill-r" id="drive-tank-r-bar" style="width:0%"></div></div>
                </div>
              </div>
              <p class="drive-tank-roller-hint">Drag each roller — top forward, bottom reverse. Two-finger for both treads.</p>
              <div class="drive-tank-rollers">
                <div class="drive-roller-col">
                  <span class="drive-roller-label">L tread</span>
                  <div class="drive-roller-track" id="tank-roller-left" role="slider" tabindex="0" aria-label="Left tread" aria-valuemin="-255" aria-valuemax="255" aria-valuenow="0">
                    <div class="drive-roller-groove" aria-hidden="true"></div>
                    <div class="drive-roller-thumb" id="tank-thumb-left"></div>
                  </div>
                </div>
                <div class="drive-roller-col">
                  <span class="drive-roller-label">R tread</span>
                  <div class="drive-roller-track" id="tank-roller-right" role="slider" tabindex="0" aria-label="Right tread" aria-valuemin="-255" aria-valuemax="255" aria-valuenow="0">
                    <div class="drive-roller-groove" aria-hidden="true"></div>
                    <div class="drive-roller-thumb" id="tank-thumb-right"></div>
                  </div>
                </div>
              </div>
              <input type="hidden" id="tank-left" value="0">
              <input type="hidden" id="tank-right" value="0">
            </div>
            <div class="drive-motor-hud">
              <div class="drive-motor-cell">
                <span class="drive-motor-lbl">Left</span>
                <span class="drive-motor-val mono" id="drive-motor-l">0</span>
              </div>
              <div class="drive-motor-cell drive-motor-center">
                <span class="drive-motor-lbl">Avg command</span>
                <span class="drive-motor-avg mono" id="drive-speed">0 / 255</span>
                <div class="drive-speed-track" aria-hidden="true"><div class="drive-speed-fill" id="drive-speed-bar" style="width:0%"></div></div>
              </div>
              <div class="drive-motor-cell">
                <span class="drive-motor-lbl">Right</span>
                <span class="drive-motor-val mono" id="drive-motor-r">0</span>
              </div>
            </div>
          </div>
        </div>
      </div>

      <div id="drive-ai-panel" class="card drive-ai-card" style="display:none">
        <div class="card-header">AI assist</div>
        <div class="card-body">
          <div class="drive-ai-grid">
            <div class="drive-ai-kpi"><span class="drive-ai-kpi-lbl">Autonomy</span><span class="drive-ai-kpi-val" id="drive-ai-enabled">—</span></div>
            <div class="drive-ai-kpi"><span class="drive-ai-kpi-lbl">State</span><span class="drive-ai-kpi-val mono" id="drive-ai-state">—</span></div>
            <div class="drive-ai-kpi"><span class="drive-ai-kpi-lbl">Brain</span><span class="drive-ai-kpi-val mono" id="drive-ai-unified">—</span></div>
            <div class="drive-ai-kpi"><span class="drive-ai-kpi-lbl">Safety</span><span class="drive-ai-kpi-val" id="drive-ai-safety">—</span></div>
            <div class="drive-ai-kpi"><span class="drive-ai-kpi-lbl">Emotion</span><span class="drive-ai-kpi-val" id="drive-ai-emotion">—</span></div>
            <div class="drive-ai-kpi"><span class="drive-ai-kpi-lbl">Override</span><span class="drive-ai-kpi-val" id="drive-ai-manual">—</span></div>
          </div>
          <div class="drive-ai-actions page-actions">
            <button type="button" class="btn btn-small" id="drive-ai-start" onclick="aiAssistStart()">Start AI</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="aiAssistStop()">Stop AI</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="aiAssistTakeOver()">Take over</button>
            <button type="button" class="btn btn-small" onclick="aiAssistResume()">Resume AI</button>
          </div>
          <p class="stat-sub drive-ai-foot">Joystick or tank overrides motors; Take over pauses autonomy until Resume.</p>
        </div>
      </div>

      <div class="drive-grid-two">
        <div class="card drive-speed-card">
          <div class="card-header">Speed cap</div>
          <div class="card-body">
            <p class="stat-sub" style="margin:0 0 10px">Motor command ceiling from <span class="mono">/settings</span> · applies to all modes.</p>
            <div class="form-group" style="margin-bottom:0"><label for="speed-profile">Profile</label>
              <select id="speed-profile" onchange="applySpeedProfile()">
                <option value="low">Low (128)</option>
                <option value="normal" selected>Normal (200)</option>
                <option value="high">High (255)</option>
              </select>
            </div>
          </div>
        </div>
        <div class="card drive-mission-card">
          <div class="card-header">Shortcuts</div>
          <div class="card-body">
            <p class="stat-sub" style="margin:0 0 12px">Docking and home use the same APIs as the Dock tab.</p>
            <div class="page-actions" style="margin-top:0">
              <button type="button" class="btn btn-small" onclick="triggerAutoDock()">Go to dock</button>
              <button type="button" class="btn btn-small btn-ghost" onclick="setHome()">Set home</button>
              <button type="button" class="btn btn-small btn-ghost" onclick="switchTab('navigation')">Navigation</button>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- NAVIGATION -->
    <div class="page more-subpage" id="page-navigation">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">LROS · NAVIGATION</p>
          <h2 class="page-title subpage-title">Navigation</h2>
          <p class="page-lead">Map, waypoints, world &amp; weather context, and mission command deck.</p>
        </div>
      </header>
      <div class="nav-world-strip" role="status" aria-live="polite">
        <span class="nav-pill nav-pill-dim" id="nav-pill-browser" title="Browser network reachability">Browser: …</span>
        <span class="nav-pill nav-pill-dim" id="nav-pill-router" title="Robot Wi‑Fi station status">Robot: …</span>
        <span class="nav-pill nav-pill-dim" id="nav-pill-wan" title="Internet + latency hint">Internet: …</span>
      </div>
      <p class="nav-world-hint">Live map &amp; weather use your phone or PC internet connection (not the ESP). Connect the robot to Wi‑Fi for RSSI and station IP below.</p>
      <div class="nav-layout">
        <div class="nav-map-wrap">
          <div class="nav-toolbar nav-toolbar--split">
            <span class="stat-sub">MapLibre field map · click adds waypoints (or home/dock) · pan/zoom · planner canvas fallback if tiles fail</span>
            <span class="nav-toolbar-right">
              <span id="nav-eta" class="conn-pill">—</span>
              <button type="button" class="btn btn-small btn-ghost" id="prox-mute-btn" onclick="toggleProxMute()" title="Proximity beeps">&#128276;</button>
            </span>
          </div>
          <div class="nav-map-real" id="nav-map-real" hidden>
            <img id="nav-osm-img" class="nav-osm-img" width="640" height="200" alt="" decoding="async" />
            <div class="nav-osm-placeholder" id="nav-osm-placeholder">OpenStreetMap preview appears when location data is available.</div>
          </div>

          <div class="nav-map-stage" id="nav-map-stage">
            <div class="nav-map-controls">
              <button type="button" class="btn btn-small" id="nav-map-center" title="Center camera on robot">Center robot</button>
              <label class="nav-map-check" title="Keep camera on robot">
                <input type="checkbox" id="nav-map-follow" /> Follow
              </label>
              <label class="nav-map-check" title="Rotate map with IMU heading">
                <input type="checkbox" id="nav-map-heading-up" /> Heading-up
              </label>
              <label class="nav-map-field">Click
                <select id="nav-map-click-mode" class="nav-map-select">
                  <option value="waypoint">Adds waypoint</option>
                  <option value="home">Sets home marker (local)</option>
                  <option value="dock">Sets dock marker (local)</option>
                </select>
              </label>
              <label class="nav-map-field">Base layer
                <select id="nav-map-layer-preset" class="nav-map-select" title="Reloads page when changed">
                  <option value="embed" selected>Embedded (offline)</option>
                  <option value="demo">MapLibre demo style (online)</option>
                </select>
              </label>
            </div>
            <div class="nav-map-hud" id="nav-map-hud" aria-label="Mission HUD">
              <span class="nav-hud-chip"><span class="nav-hud-lbl">Autonomy</span><span class="nav-hud-val" id="nav-hud-autonomy">—</span></span>
              <span class="nav-hud-chip"><span class="nav-hud-lbl">Motion</span><span class="nav-hud-val" id="nav-hud-motion">—</span></span>
              <span class="nav-hud-chip"><span class="nav-hud-lbl">Drive</span><span class="nav-hud-val" id="nav-hud-profile">—</span></span>
              <span class="nav-hud-chip"><span class="nav-hud-lbl">Next WP</span><span class="nav-hud-val" id="nav-hud-next-wp">—</span></span>
              <span class="nav-hud-chip"><span class="nav-hud-lbl">Route</span><span class="nav-hud-val" id="nav-hud-route-m">—</span></span>
              <span class="nav-hud-chip"><span class="nav-hud-lbl">ETA</span><span class="nav-hud-val" id="nav-hud-eta">—</span></span>
              <span class="nav-hud-chip"><span class="nav-hud-lbl">GPS</span><span class="nav-hud-val" id="nav-hud-gps">—</span></span>
              <span class="nav-hud-chip"><span class="nav-hud-lbl">Heading</span><span class="nav-hud-val" id="nav-hud-hdg">—</span></span>
              <span class="nav-hud-chip"><span class="nav-hud-lbl">Tel age</span><span class="nav-hud-val" id="nav-hud-tel-age">—</span></span>
            </div>
            <div class="nav-map-fallback" id="nav-map-fallback" role="status" hidden></div>
            <div class="nav-map-body">
              <div id="nav-map-container" class="nav-map-container" aria-label="MapLibre map"></div>
              <canvas id="nav-map-canvas" width="600" height="320"></canvas>
            </div>
          </div>

          <p class="nav-map-help stat-sub">Configure <span class="mono">localStorage</span> keys <span class="mono">lros_map_style_url</span> (style.json) and/or <span class="mono">lros_map_raster_url</span> (raster <span class="mono">{z}/{x}/{y}</span> template) for LAN or offline tiles. No Google tiles.</p>
          <div style="display:flex;gap:8px;margin-top:8px;flex-wrap:wrap">
            <button type="button" class="btn btn-small" onclick="navClearWaypoints()">Clear WP</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="navSendRouteToRobot()">Send route</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="switchTab('drive')">Manual drive</button>
          </div>
        </div>
        <div class="nav-sidebar">
          <div class="card nav-world-card">
            <div class="card-header">World (internet)</div>
            <div class="card-body">
              <div class="status-row"><span class="label">Public IP</span><span class="value mono" id="nav-wan-ip">—</span></div>
              <div class="status-row"><span class="label">Approx. place</span><span class="value" id="nav-wan-loc">—</span></div>
              <div class="status-row"><span class="label">ISP / org</span><span class="value dim" id="nav-wan-org">—</span></div>
              <div class="status-row"><span class="label">Weather</span><span class="value" id="nav-weather">—</span></div>
              <div class="status-row"><span class="label">Lookup RTT</span><span class="value" id="nav-inet-rtt">—</span></div>
              <div class="status-row"><span class="label">Clock (TZ)</span><span class="value dim" id="nav-local-clock">—</span></div>
              <button type="button" class="btn btn-small btn-ghost" id="nav-geo-btn">Use this device’s GPS</button>
            </div>
          </div>
          <div class="card">
            <div class="card-header">Robot Wi‑Fi</div>
            <div class="card-body">
              <div class="status-row"><span class="label">STA IP</span><span class="value mono" id="nav-sta-ip">—</span></div>
              <div class="status-row"><span class="label">SSID</span><span class="value" id="nav-sta-ssid">—</span></div>
              <div class="status-row"><span class="label">RSSI</span><span class="value" id="nav-rssi">—</span></div>
            </div>
          </div>
          <div class="card">
            <div class="card-header">Waypoints</div>
            <div class="card-body">
              <ul id="nav-waypoint-list" style="list-style:none;padding:0;margin:0;min-height:48px"></ul>
              <div class="status-row"><span class="label">Home</span><span class="value dim" id="nav-home">Not set</span></div>
              <div class="status-row"><span class="label">Robot GPS</span><span class="value" id="nav-current">-</span></div>
            </div>
          </div>
          <div class="card">
            <div class="card-header">Obstacles (demo)</div>
            <div class="card-body">
              <div class="status-row"><span class="label">LiDAR</span><span class="value" style="color:#e63946">zones</span></div>
              <div class="status-row"><span class="label">Ultrasonic</span><span class="value" style="color:#f5a623">zones</span></div>
            </div>
          </div>
          <div class="card">
            <div class="card-header">WebSocket</div>
            <div class="card-body">
              <div class="form-group"><label>ws:// URL (optional)</label><input type="text" id="ws-url-input" placeholder="ws://192.168.4.1/ws"></div>
              <button type="button" class="btn btn-small" onclick="saveWsUrl()">Save &amp; reconnect</button>
            </div>
          </div>
        </div>
      </div>

      <section class="nav-mission-deck" id="nav-mission-deck" aria-label="Mission command">
        <div class="nav-mission-deck-head">
          <div>
            <h2 class="nav-mission-h2">Mission command</h2>
            <p class="nav-mission-sub" id="nav-mission-sub">Plan waypoints, complete preflight, then Arm.</p>
          </div>
          <div class="nav-mission-state-pill state-idle" id="nav-mission-state-pill">IDLE</div>
        </div>

        <div class="nav-mission-hero-grid">
          <div class="nav-mission-summary">
            <p class="nav-mission-plan-summary" id="nav-mission-plan-summary">No waypoints — tap the map to plan.</p>
            <div class="nav-mission-eta-row">
              <span class="stat-sub">Planner route</span>
              <span class="conn-pill mono" id="nav-mission-route-len">—</span>
              <span class="stat-sub">Path length (same as map)</span>
              <span class="conn-pill" id="nav-dist">—</span>
            </div>
          </div>
          <div class="nav-mission-ring-wrap" aria-hidden="true">
            <div class="nav-mission-ring" id="nav-mission-ring-fill" style="--p:0"></div>
            <div class="nav-mission-ring-center">
              <span class="nav-mission-pct" id="nav-mission-pct">0%</span>
              <span class="nav-mission-pct-label">exec</span>
            </div>
          </div>
          <div class="nav-mission-kpi-grid">
            <div class="nav-mission-kpi"><span class="nav-mission-kpi-label">ETA (plan)</span><span class="nav-mission-kpi-val" id="nav-mission-eta-large">—</span></div>
            <div class="nav-mission-kpi"><span class="nav-mission-kpi-label">ETA remain</span><span class="nav-mission-kpi-val" id="nav-mission-eta-remain">—</span></div>
            <div class="nav-mission-kpi"><span class="nav-mission-kpi-label">Waypoints</span><span class="nav-mission-kpi-val" id="nav-mission-wp-count">0</span></div>
            <div class="nav-mission-kpi"><span class="nav-mission-kpi-label">Segments</span><span class="nav-mission-kpi-val" id="nav-mission-segments">0</span></div>
            <div class="nav-mission-kpi"><span class="nav-mission-kpi-label">Battery</span><span class="nav-mission-kpi-val" id="nav-mission-battery">—</span></div>
            <div class="nav-mission-kpi"><span class="nav-mission-kpi-label">Sonar</span><span class="nav-mission-kpi-val" id="nav-mission-sonar">—</span></div>
            <div class="nav-mission-kpi"><span class="nav-mission-kpi-label">Heading</span><span class="nav-mission-kpi-val" id="nav-mission-heading">—</span></div>
            <div class="nav-mission-kpi"><span class="nav-mission-kpi-label">Autonomy</span><span class="nav-mission-kpi-val" id="nav-mission-autonomy">—</span></div>
            <div class="nav-mission-kpi"><span class="nav-mission-kpi-label">Return home</span><span class="nav-mission-kpi-val" id="nav-mission-rth">—</span></div>
          </div>
        </div>
        <p class="nav-mission-batt-warn" id="nav-mission-batt-warn" hidden></p>

        <div class="nav-mission-phase-strip" role="list">
          <div class="nav-phase nav-phase-pending" id="nav-phase-plan" role="listitem"><span>1</span> Plan</div>
          <div class="nav-phase nav-phase-pending" id="nav-phase-arm" role="listitem"><span>2</span> Arm</div>
          <div class="nav-phase nav-phase-pending" id="nav-phase-exec" role="listitem"><span>3</span> Execute</div>
          <div class="nav-phase nav-phase-pending" id="nav-phase-verify" role="listitem"><span>4</span> Verify</div>
          <div class="nav-phase nav-phase-pending" id="nav-phase-done" role="listitem"><span>5</span> Done</div>
        </div>

        <div class="nav-mission-split">
          <div class="nav-mission-preflight card">
            <div class="card-header">Preflight</div>
            <div class="card-body">
              <label class="nav-pf-row"><input type="checkbox" id="nav-pf-battery" /> Battery sufficient for route</label>
              <label class="nav-pf-row"><input type="checkbox" id="nav-pf-link" /> Robot link / HTTP OK</label>
              <label class="nav-pf-row"><input type="checkbox" id="nav-pf-path" /> Path reviewed on map</label>
              <label class="nav-pf-row"><input type="checkbox" id="nav-pf-clear" /> Area clear / safe to move</label>
              <p class="nav-mission-hint">Confirm each item before you arm. <strong>Send route</strong> uploads the plan to the Brain as GPS waypoints (<code class="mono">POST /api/navigation/route</code>) when GPS and compass are valid. Mission progress on this page stays a preview until you wire live telemetry into the UI.</p>
            </div>
          </div>
          <div class="nav-mission-log-wrap card">
            <div class="card-header">Mission log</div>
            <div class="card-body nav-mission-log" id="nav-mission-log"><div class="nav-mission-log-row dim">Events appear here.</div></div>
          </div>
        </div>

        <div class="nav-mission-actions">
          <button type="button" class="btn btn-small" id="nav-mission-btn-arm">Arm mission</button>
          <button type="button" class="btn btn-small" id="nav-mission-btn-start" style="display:none">Start</button>
          <button type="button" class="btn btn-small btn-ghost" id="nav-mission-btn-pause" style="display:none">Pause</button>
          <button type="button" class="btn btn-small" id="nav-mission-btn-resume" style="display:none">Resume</button>
          <button type="button" class="btn btn-small btn-ghost" id="nav-mission-btn-abort" style="display:none">Abort</button>
          <button type="button" class="btn btn-small btn-ghost" id="nav-mission-btn-reset" style="display:none">Reset</button>
          <button type="button" class="btn btn-small" onclick="triggerAutoDock()">Return home (dock)</button>
          <button type="button" class="btn btn-small btn-ghost" onclick="switchTab('missions')">Missions page</button>
        </div>
      </section>
    </div>

    <!-- DOCKING -->
    <div class="page" id="page-docking">
      <header class="page-head">
        <h2 class="page-title">Docking station</h2>
        <p class="page-lead">Align with the charging dock, monitor dock node health, and manage return-home.</p>
      </header>

      <div class="dock-hero card">
        <div class="dock-hero-main">
          <div class="dock-hero-icon" aria-hidden="true">&#128268;</div>
          <div>
            <div class="dock-fsm-pill" id="dock-fsm-pill">—</div>
            <p class="dock-hero-sub" id="dock-hero-sub">Pull live status from the Brain.</p>
          </div>
        </div>
        <div class="dock-hero-metrics">
          <div class="dock-metric"><span class="dock-metric-label">Dock FSM</span><span class="dock-metric-val mono" id="dock-fsm">—</span></div>
          <div class="dock-metric"><span class="dock-metric-label">Seek active</span><span class="dock-metric-val" id="dock-active">—</span></div>
          <div class="dock-metric"><span class="dock-metric-label">Return home</span><span class="dock-metric-val" id="dock-rth">—</span></div>
          <div class="dock-metric"><span class="dock-metric-label">Dock node</span><span class="dock-metric-val" id="dock-node-online">—</span></div>
          <div class="dock-metric"><span class="dock-metric-label">Flags</span><span class="dock-metric-val mono dim" id="dock-flags">—</span></div>
        </div>
      </div>

      <div class="dock-grid-two">
        <div class="card">
          <div class="card-header">Approach &amp; sensors</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Autonomy stage</span><span class="value" id="dock-stage">—</span></div>
            <div class="status-row"><span class="label">IR / beam</span><span class="value dim" id="dock-beam">Awaiting dock telemetry</span></div>
            <p class="stat-sub" style="margin-top:10px">Beam and IR alignment details will appear here when exposed on <span class="mono">/api/dock/status</span> or ESP-NOW telemetry.</p>
          </div>
        </div>
        <div class="card">
          <div class="card-header">Power on base</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Voltage</span><span class="value" id="dock-voltage">—</span></div>
            <div class="status-row"><span class="label">Estimate</span><span class="value" id="dock-current">—</span></div>
            <div class="progress-bar dock-charge-bar-wrap"><div class="progress-fill" id="dock-charge-bar" style="width:0%"></div></div>
            <p class="stat-sub">Charging hint uses fleet health when the dock reports charge flags.</p>
          </div>
        </div>
      </div>

      <div class="card dock-timeline-card">
        <div class="card-header">Typical sequence</div>
        <div class="card-body">
          <ol class="dock-timeline" id="dock-timeline">
            <li class="dock-step" data-step="seek"><span class="dock-step-n">1</span><span>Seek dock / IR lock</span></li>
            <li class="dock-step" data-step="align"><span class="dock-step-n">2</span><span>Approach &amp; align</span></li>
            <li class="dock-step" data-step="seat"><span class="dock-step-n">3</span><span>Seat on contacts</span></li>
            <li class="dock-step" data-step="charge"><span class="dock-step-n">4</span><span>Charge</span></li>
          </ol>
        </div>
      </div>

      <div class="card">
        <div class="card-header">Fleet node — dock</div>
        <div class="card-body dock-node-strip">
          <span class="node-pill dock-page-node" data-pill="dock" id="dock-page-node-pill" title="Dock ESP">DOCK</span>
          <span class="stat-sub" id="dock-node-hint">Matches strip above when online.</span>
        </div>
      </div>

      <div class="page-actions dock-actions">
        <button type="button" class="btn" onclick="triggerAutoDock()">Start docking</button>
        <button type="button" class="btn btn-ghost" onclick="apiCall('/api/dock/cancel')">Cancel homing</button>
        <button type="button" class="btn btn-ghost" onclick="refreshDockPanel(true)">Refresh status</button>
        <button type="button" class="btn btn-ghost" onclick="switchTab('network')">Network</button>
      </div>
    </div>

    <!-- VISION — camera node (VisionPacket / recognition pipeline) -->
    <div class="page vision-page" id="page-vision">
      <header class="vision-page-head">
        <div class="vision-page-titles">
          <p class="vision-kicker">NODE · OPTICS</p>
          <h2 class="page-title vision-cinematic-title">Camera intelligence</h2>
          <p class="page-lead vision-page-lead">Live snapshot, behaviour engine, and full recognition telemetry from the vision node (motion → blob → classify → events).</p>
        </div>
        <div class="vision-head-badges">
          <span class="vision-pill vision-pill-link" id="vision-link-pill" title="ESP-NOW / UDP link">LINK · …</span>
          <span class="vision-pill vision-pill-behave" id="vision-behave-pill" title="Base behaviour state">BRAIN · —</span>
          <span class="vision-pill vision-pill-age" id="vision-age-pill" title="Packet age">Δ — ms</span>
        </div>
      </header>

      <section class="vision-fpv-stage" aria-label="Live camera">
        <div class="vision-fpv-wrap fpv-container" id="vision-fpv-wrap">
          <img id="fpv-img" class="vision-fpv-img" src="" alt="Camera snapshot" onerror="visionOnImgError()">
          <div class="vision-hud" id="vision-hud" aria-hidden="true">
            <div class="vision-hud-frame"></div>
            <div class="vision-hud-crosshair"></div>
            <div class="vision-hud-reticle" id="vision-reticle" title="Target"></div>
            <div class="vision-hud-corner vision-hud-tl"><span>CAM</span></div>
            <div class="vision-hud-corner vision-hud-tr"><span id="vision-hud-frameid">FR —</span></div>
            <div class="vision-hud-corner vision-hud-bl"><span id="vision-hud-zone-hud">ZONE —</span></div>
            <div class="vision-hud-corner vision-hud-br"><span id="vision-hud-lock-hud">LOCK —</span></div>
          </div>
          <div id="fpv-placeholder" class="vision-fpv-placeholder">
            <span class="vision-ph-icon" aria-hidden="true">&#9673;</span>
            <span class="vision-ph-title">No camera link</span>
            <span class="vision-ph-hint">Vision node IP will appear when packets arrive on the base.</span>
          </div>
        </div>
        <div class="vision-toolbar">
          <div class="vision-controls vision-toolbar-sliders">
            <label class="vision-ctl"><span>Blend</span><input type="range" id="fpv-opacity" min="0.2" max="1" step="0.05" value="1" oninput="setFpvOpacity(this.value)"></label>
            <label class="vision-ctl"><span>Zoom</span><input type="range" id="fpv-scale" min="0.55" max="1.25" step="0.05" value="1" oninput="setFpvScale(this.value)"></label>
            <label class="vision-ctl vision-ctl-tight"><span>Refresh</span><input type="range" id="vision-refresh-ms" min="400" max="3000" step="100" value="900" oninput="setVisionRefreshMs(this.value)" title="Snapshot interval"></label>
          </div>
          <div class="vision-toolbar-actions">
            <button type="button" class="btn btn-small vision-btn-primary" onclick="snapshot()">Open snapshot</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="refreshVisionPanel(true)">Sync telemetry</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="toggleNightMode()">Night</button>
          </div>
        </div>
      </section>

      <section class="vision-triage" aria-label="Recognition summary">
        <div class="vision-triage-card vision-triage-event">
          <span class="vision-triage-label">Event</span>
          <span class="vision-triage-value" id="vision-event-big">—</span>
          <span class="vision-triage-sub" id="vision-scene">Awaiting packets…</span>
        </div>
        <div class="vision-triage-card vision-triage-class">
          <span class="vision-triage-label">Classification</span>
          <span class="vision-triage-value" id="vision-class-big">—</span>
          <span class="vision-triage-sub" id="vision-colour-line">Colour —</span>
        </div>
        <div class="vision-triage-card vision-triage-dist">
          <span class="vision-triage-label">Distance</span>
          <span class="vision-triage-value" id="vision-dist-big">—</span>
          <span class="vision-triage-sub" id="vision-blob-line">Blob —</span>
        </div>
      </section>

      <div class="vision-panels-grid">
        <div class="card vision-card">
          <div class="card-header vision-card-h">Tracking &amp; motion</div>
          <div class="card-body vision-metric-grid">
            <div class="vision-metric"><span class="vm-label">Motion</span><span class="vm-val" id="vision-m-motion">—</span></div>
            <div class="vision-metric"><span class="vm-label">Intensity</span><span class="vm-val" id="vision-m-int">—</span></div>
            <div class="vision-metric"><span class="vm-label">Target X/Y</span><span class="vm-val vm-mono" id="vision-m-xy">—</span></div>
            <div class="vision-metric"><span class="vm-label">Zone</span><span class="vm-val" id="vision-m-zone">—</span></div>
            <div class="vision-metric"><span class="vm-label">Object size</span><span class="vm-val" id="vision-m-objsz">—</span></div>
            <div class="vision-metric"><span class="vm-label">BBox W×H</span><span class="vm-val vm-mono" id="vision-m-bbox">—</span></div>
          </div>
        </div>
        <div class="card vision-card">
          <div class="card-header vision-card-h">Recognition layer</div>
          <div class="card-body vision-metric-grid">
            <div class="vision-metric"><span class="vm-label">Class (obj)</span><span class="vm-val" id="vision-m-objclass">—</span></div>
            <div class="vision-metric"><span class="vm-label">Colour ID</span><span class="vm-val" id="vision-m-colour">—</span></div>
            <div class="vision-metric"><span class="vm-label">Conf.</span><span class="vm-val" id="vision-m-colourconf">—</span></div>
            <div class="vision-metric"><span class="vm-label">Blob</span><span class="vm-val" id="vision-m-blobdet">—</span></div>
            <div class="vision-metric"><span class="vm-label">Blob pos</span><span class="vm-val vm-mono" id="vision-m-blobxy">—</span></div>
            <div class="vision-metric"><span class="vm-label">Classify</span><span class="vm-val" id="vision-m-classify">—</span></div>
          </div>
        </div>
        <div class="card vision-card vision-card-span">
          <div class="card-header vision-card-h">Lock &amp; node</div>
          <div class="card-body vision-metric-grid vision-metric-grid-tight">
            <div class="vision-metric"><span class="vm-label">Target lock</span><span class="vm-val" id="vision-m-lock">—</span></div>
            <div class="vision-metric"><span class="vm-label">Lock conf.</span><span class="vm-val" id="vision-m-lockconf">—</span></div>
            <div class="vision-metric"><span class="vm-label">Vision event</span><span class="vm-val" id="vision-m-vevent">—</span></div>
            <div class="vision-metric vision-metric-wide"><span class="vm-label">Node IP</span><span class="vm-val vm-mono" id="vision-m-ip">—</span></div>
            <div class="vision-metric"><span class="vm-label">Engine</span><span class="vm-val" id="vision-m-enabled">—</span></div>
          </div>
        </div>
      </div>
    </div>

    <!-- AUDIO -->
    <div class="page more-subpage" id="page-audio">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">NODE · AUDIO</p>
          <h2 class="page-title subpage-title">Audio</h2>
          <p class="page-lead">Voice commands, soundboard, and output level to the audio node.</p>
        </div>
      </header>
      <div class="subpage-deck">
        <div class="card more-deck-card">
          <div class="card-header">Voice commands</div>
          <div class="card-body">
            <div class="form-group"><label>Speak or type command</label><input type="text" id="voice-cmd" placeholder="e.g. dock, stop, happy" onkeydown="if(event.key==='Enter')sendVoiceCmd()"></div>
            <button type="button" class="btn btn-small" onclick="sendVoiceCmd()">Send to robot</button>
            <p class="stat-sub" style="margin-top:8px">Requires /api/audio/command or similar on base.</p>
          </div>
        </div>
        <div class="card more-deck-card">
          <div class="card-header">Soundboard</div>
          <div class="card-body soundboard-pad">
            <button type="button" class="btn btn-small" onclick="playSound('beep')">Beep</button>
            <button type="button" class="btn btn-small" onclick="playSound('happy')">Happy</button>
            <button type="button" class="btn btn-small" onclick="playSound('sad')">Sad</button>
            <button type="button" class="btn btn-small" onclick="playSound('curious')">Curious</button>
          </div>
        </div>
        <div class="card more-deck-card">
          <div class="card-header">Volume</div>
          <div class="card-body">
            <input type="range" id="audio-volume" min="0" max="255" value="180" onchange="setVolume(this.value)">
            <div class="status-row"><span class="label">Level</span><span class="value" id="audio-vol-val">180</span></div>
          </div>
        </div>
      </div>
    </div>

    <!-- AI / AUTONOMY -->
    <div class="page more-subpage" id="page-ai">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">BRAIN · AUTONOMY</p>
          <h2 class="page-title subpage-title">AI &amp; autonomy</h2>
          <p class="page-lead">Unified brain status, assistant chat, behaviour, and personality from the base.</p>
        </div>
      </header>
      <div class="subpage-deck subpage-deck--2col">
      <div class="card more-deck-card">
        <div class="card-header">Autonomy &amp; brain</div>
        <div class="card-body">
          <div class="ai-brain-strip">
            <span class="brain-pill" id="ai-pill-unified" title="Unified autonomy state">—</span>
          </div>
          <div class="status-row"><span class="label">Autonomy</span><span class="value" id="ai-autonomy-enabled">—</span></div>
          <div class="status-row"><span class="label">Engine state</span><span class="value" id="ai-autonomy-state">—</span></div>
          <div class="status-row"><span class="label">Brain (unified)</span><span class="value" id="ai-unified">—</span></div>
          <div class="status-row"><span class="label">Safety</span><span class="value" id="ai-safety">—</span></div>
          <div class="status-row"><span class="label">Override</span><span class="value" id="ai-manual">—</span></div>
          <div class="status-row"><span class="label">Sonar</span><span class="value" id="ai-sonar">—</span></div>
          <div class="status-row"><span class="label">Interest</span><span class="value" id="ai-interest">—</span></div>
          <div class="status-row"><span class="label">Heading</span><span class="value" id="ai-heading">—</span></div>
          <div class="status-row"><span class="label">Emotion</span><span class="value" id="ai-emotion-live">—</span></div>
          <div class="status-row"><span class="label">Pose</span><span class="value" id="ai-pose-emotion">—</span></div>
          <div class="status-row"><span class="label">Object seen</span><span class="value" id="ai-object">—</span></div>
          <div class="status-row"><span class="label">Return home</span><span class="value" id="ai-rth">—</span></div>
          <div class="status-row"><span class="label">GPS</span><span class="value" id="ai-gps">—</span></div>
          <div class="status-row"><span class="label">Position</span><span class="value dim" id="ai-gps-detail">—</span></div>
          <div style="display:flex;gap:8px;flex-wrap:wrap;margin-top:12px">
            <button type="button" class="btn btn-small" onclick="aiAssistStart()">Start AI</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="aiAssistStop()">Stop AI</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="aiAssistTakeOver()">Take over</button>
            <button type="button" class="btn btn-small" onclick="aiAssistResume()">Resume</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="setHome()">Set home</button>
          </div>
          <p class="stat-sub" style="margin-top:10px">Unified brain runs on the base; use Drive for joystick/tank. Values refresh while this tab is open.</p>
        </div>
      </div>
      <div class="card more-deck-card">
        <div class="card-header">AI assistant</div>
        <div class="card-body">
          <div class="ai-chat-log" id="ai-chat-log">
            <div class="ai-chat-row bot">WALL-E: Ready. Ask for status or set behaviour.</div>
          </div>
          <div class="form-group"><input type="text" id="ai-input" placeholder="Type a command..." onkeydown="if(event.key==='Enter')sendAiChat()"></div>
          <button type="button" class="btn btn-small" onclick="sendAiChat()">Send</button>
        </div>
      </div>
      <div class="card more-deck-card">
        <div class="card-header">Behaviour mode</div>
        <div class="card-body">
          <select id="ai-mode" onchange="setBehaviourMode(this.value)">
            <option value="curious">Curious</option><option value="happy">Happy</option><option value="shy">Shy</option>
            <option value="tired">Tired</option><option value="excited">Excited</option>
          </select>
        </div>
      </div>
      <div class="card more-deck-card">
        <div class="card-header">Personality (live from base)</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Curiosity</span><span class="value" id="ai-p-cur">—</span></div>
          <div class="status-row"><span class="label">Bravery</span><span class="value" id="ai-p-bra">—</span></div>
          <div class="status-row"><span class="label">Energy</span><span class="value" id="ai-p-nrg">—</span></div>
          <div class="status-row"><span class="label">Randomness</span><span class="value" id="ai-p-rnd">—</span></div>
          <p class="stat-sub" style="margin:10px 0 8px">Sliders (local UI; syncs from robot when polling)</p>
          <div class="form-group"><label>Curiosity</label><input type="range" id="ai-curiosity" min="0" max="100" value="50"></div>
          <div class="form-group"><label>Energy</label><input type="range" id="ai-energy" min="0" max="100" value="70"></div>
          <div class="status-row"><span class="label">Learning</span><span class="value"><input type="checkbox" id="ai-learning" checked></span></div>
        </div>
      </div>
      </div>
    </div>

    <!-- MISSIONS -->
    <div class="page more-subpage" id="page-missions">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">OPS · MISSIONS</p>
          <h2 class="page-title subpage-title">Missions</h2>
          <p class="page-lead">Queue missions and run presets — wired when the Brain exposes mission APIs.</p>
        </div>
      </header>
      <div class="subpage-deck">
      <div class="card more-deck-card">
        <div class="card-header">New mission</div>
        <div class="card-body">
          <div class="form-group"><label>Type</label>
            <select id="mission-type"><option value="patrol">Patrol</option><option value="waypoint">Waypoint</option><option value="explore">Explore</option><option value="dock">Dock</option></select>
          </div>
          <div class="form-group"><label>Repeat</label>
            <select id="mission-repeat"><option value="once">Once</option><option value="daily">Daily</option><option value="weekly">Weekly</option></select>
          </div>
          <button type="button" class="btn btn-small" onclick="queueMission()">Add to queue</button>
        </div>
      </div>
      <div class="card more-deck-card">
        <div class="card-header">Queue</div>
        <div class="card-body" id="mission-queue"><div class="log-item value dim">Empty</div></div>
      </div>
      <div class="card more-deck-card">
        <div class="card-header">Timeline</div>
        <div class="card-body">
          <div class="mission-timeline">
            <div class="step">Boot</div>
            <div class="step">Awaiting mission API</div>
          </div>
        </div>
      </div>
      <div class="card more-deck-card">
        <div class="card-header">Presets</div>
        <div class="card-body">
          <div id="mission-list">
            <div class="status-row"><span class="label">Patrol</span><button class="btn btn-small" onclick="runMission('patrol')">Run</button></div>
            <div class="status-row"><span class="label">Return Home</span><button class="btn btn-small" onclick="runMission('rth')">Run</button></div>
          </div>
        </div>
      </div>
      </div>
    </div>

    <!-- SEQUENCE GENERATOR -->
    <div class="page more-subpage" id="page-sequence">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">SHOW · SEQUENCER</p>
          <h2 class="page-title subpage-title">Sequence generator</h2>
          <p class="page-lead">Timed cue stacks stored on the Brain (flash). Edit steps, save, then run — navigation_route needs GPS + compass like the Navigation page.</p>
        </div>
      </header>
      <div class="subpage-deck subpage-deck--2col">
        <div class="card more-deck-card">
          <div class="card-header">Library</div>
          <div class="card-body">
            <div class="form-group"><label>On robot</label>
              <select id="seq-library-select" class="seq-select"></select>
            </div>
            <div class="seq-toolbar">
              <button type="button" class="btn btn-small" id="seq-btn-refresh">Refresh</button>
              <button type="button" class="btn btn-small btn-ghost" id="seq-btn-load">Load</button>
              <button type="button" class="btn btn-small btn-ghost" id="seq-btn-delete">Delete</button>
              <button type="button" class="btn btn-small btn-ghost" id="seq-btn-new">New</button>
            </div>
            <p class="stat-sub">API: <span class="mono">/api/sequences/*</span> — persisted in NVS.</p>
          </div>
        </div>
        <div class="card more-deck-card">
          <div class="card-header">Transport</div>
          <div class="card-body">
            <div class="seq-toolbar">
              <button type="button" class="btn btn-small" id="seq-btn-save">Save to robot</button>
              <button type="button" class="btn btn-small" id="seq-btn-run">Run</button>
              <button type="button" class="btn btn-small btn-ghost" id="seq-btn-stop">Stop</button>
            </div>
            <p class="seq-status-line" id="seq-run-status">—</p>
            <div class="coming-soon-actions" style="margin-top:10px">
              <button type="button" class="btn btn-small btn-ghost" onclick="switchTab('missions')">Missions</button>
              <button type="button" class="btn btn-small btn-ghost" onclick="switchTab('navigation')">Navigation</button>
              <button type="button" class="btn btn-small" onclick="switchTab('more')">Back to menu</button>
            </div>
          </div>
        </div>
        <div class="card more-deck-card subpage-card-span">
          <div class="card-header">Editor</div>
          <div class="card-body">
            <div class="form-group seq-meta-row">
              <label>Name</label><input type="text" id="seq-name" class="seq-name-input" placeholder="Show opener" />
              <label>Id</label><input type="text" id="seq-id" class="mono seq-id-input" readonly />
            </div>
            <p class="stat-sub">Each row: time <span class="mono">at_ms</span> from start, <span class="mono">kind</span>, then extra fields as JSON (e.g. <span class="mono">"emotion":"happy"</span>, <span class="mono">"track":1</span>, <span class="mono">"route":{...}</span> for navigation).</p>
            <div class="seq-table-wrap">
              <table class="seq-editor-table">
                <thead><tr><th>at_ms</th><th>kind</th><th>params (JSON)</th><th></th></tr></thead>
                <tbody id="seq-steps-tbody"></tbody>
              </table>
            </div>
            <button type="button" class="btn btn-small btn-ghost" id="seq-add-step">+ Add step</button>
          </div>
        </div>
      </div>
    </div>

    <!-- TELEMETRY -->
    <div class="page more-subpage" id="page-telemetry">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">SENSORS · TELEMETRY</p>
          <h2 class="page-title subpage-title">Telemetry</h2>
          <p class="page-lead">IMU, sonar, and GPS — refreshed with the main status poll.</p>
        </div>
      </header>
      <div class="subpage-deck subpage-deck--2col">
      <div class="card more-deck-card">
        <div class="card-header">IMU (live)</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Heading</span><span class="value" id="tel-heading">-</span></div>
          <div class="status-row"><span class="label">Pitch/Roll</span><span class="value" id="tel-orient">-</span></div>
          <canvas id="spark-imu" class="spark-wrap" width="300" height="48"></canvas>
          <button class="btn btn-small btn-ghost" onclick="apiCall('/imu/recalibrate')">Recalibrate</button>
        </div>
      </div>
      <div class="card more-deck-card">
        <div class="card-header">Obstacle Sensors</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Sonar</span><span class="value" id="tel-sonar">-</span></div>
          <div class="progress-bar" style="margin:8px 0"><div class="progress-fill" id="tel-sonar-bar" style="width:100%"></div></div>
          <div class="spark-label">Last ~30s @ poll</div>
          <canvas id="spark-sonar" class="spark-wrap" width="300" height="40"></canvas>
        </div>
      </div>
      <div class="card more-deck-card">
        <div class="card-header">GPS</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Fix</span><span class="value" id="tel-gps">-</span></div>
          <div class="status-row"><span class="label">Satellites</span><span class="value" id="tel-sats">-</span></div>
        </div>
      </div>
      </div>
    </div>

    <!-- EVE companion (UART + bond + target assist) -->
    <div class="page more-subpage" id="page-eve">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">LROS · EVE</p>
          <h2 class="page-title subpage-title">EVE</h2>
          <p class="page-lead">UART link, shared voicebox, bond state, and target-assist (same data as <span class="mono">/api/living/telemetry</span> plus <span class="mono">/api/eve/status</span>).</p>
        </div>
      </header>
      <div class="subpage-deck subpage-deck--2col">
        <div class="card more-deck-card">
          <div class="card-header">UART bridge</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Link</span><span class="value" id="eve-uart-link">—</span></div>
            <div class="status-row"><span class="label">Last RX age</span><span class="value mono" id="eve-uart-age">—</span></div>
            <div class="status-row"><span class="label">Last frame</span><span class="value mono" id="eve-uart-type">—</span></div>
            <div class="status-row"><span class="label">Frames / CRC err</span><span class="value mono" id="eve-uart-frames">—</span></div>
            <p class="stat-sub" style="margin:8px 0 0">Payload preview</p>
            <div class="log-item mono" style="word-break:break-all;max-height:4.5em;overflow:auto" id="eve-uart-payload">—</div>
          </div>
        </div>
        <div class="card more-deck-card">
          <div class="card-header">Voice &amp; bond</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Voicebox mode</span><span class="value" id="eve-voicebox">—</span></div>
            <div class="status-row"><span class="label">Shared WALL-E+EVE</span><span class="value" id="eve-vb-shared">—</span></div>
            <div class="status-row"><span class="label">Bond strength</span><span class="value" id="eve-bond-str">—</span></div>
            <div class="status-row"><span class="label">Trust / comfort / curious</span><span class="value mono" id="eve-bond-tcc">—</span></div>
            <div class="status-row"><span class="label">Shared dock events</span><span class="value" id="eve-bond-docks">—</span></div>
          </div>
        </div>
        <div class="card more-deck-card subpage-card-span">
          <div class="card-header">Target assist (EVE → base)</div>
          <div class="card-body">
            <p class="stat-sub" style="margin:0 0 10px">Biases treads when EVE reports target zone JSON over UART.</p>
            <div class="status-row"><span class="label">State</span><span class="value mono" id="eve-as-state">—</span></div>
            <div class="status-row"><span class="label">Zone</span><span class="value mono" id="eve-as-zone">—</span></div>
            <div class="status-row"><span class="label">Bias</span><span class="value mono" id="eve-as-bias">—</span></div>
            <div class="status-row"><span class="label">Assist data</span><span class="value" id="eve-as-stale">—</span></div>
          </div>
        </div>
        <div class="card more-deck-card">
          <div class="card-header">Dock panel</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Dock FSM</span><span class="value mono" id="eve-dock-fsm">—</span></div>
            <div class="status-row"><span class="label">Dock active</span><span class="value" id="eve-dock-active">—</span></div>
            <div class="status-row"><span class="label">Dock node</span><span class="value" id="eve-dock-node">—</span></div>
            <div class="status-row"><span class="label">Transport</span><span class="value mono" id="eve-transport">UART only</span></div>
          </div>
        </div>
        <div class="card more-deck-card">
          <div class="card-header">Personality tuner</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Curiosity</span><span class="value"><input type="range" min="0" max="100" value="70" id="eve-tune-curiosity"></span></div>
            <div class="status-row"><span class="label">Responsiveness</span><span class="value"><input type="range" min="0" max="100" value="65" id="eve-tune-response"></span></div>
            <div class="status-row"><span class="label">Activity</span><span class="value"><input type="range" min="0" max="100" value="50" id="eve-tune-activity"></span></div>
            <button class="btn" onclick="pushEvePersonality()">Push to EVE</button>
            <p class="stat-sub" id="eve-tune-status">Sends over existing WALL-E ↔ EVE UART.</p>
          </div>
        </div>
        <div class="card more-deck-card">
          <div class="card-header">EVE mic sense</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Last sound event</span><span class="value mono" id="eve-mic-event">—</span></div>
            <div class="status-row"><span class="label">Level / ambient</span><span class="value mono" id="eve-mic-level">—</span></div>
            <div class="status-row"><span class="label">Spike / clap / quiet</span><span class="value mono" id="eve-mic-flags">—</span></div>
            <div class="status-row"><span class="label">Spike threshold</span><span class="value"><input type="range" min="50" max="8000" value="1800" id="eve-mic-spike"></span></div>
            <div class="status-row"><span class="label">Clap threshold</span><span class="value"><input type="range" min="100" max="12000" value="4200" id="eve-mic-clap"></span></div>
            <div class="status-row"><span class="label">Quiet threshold</span><span class="value"><input type="range" min="10" max="2000" value="180" id="eve-mic-quiet"></span></div>
            <button class="btn" onclick="pushEveMicSettings()">Save mic settings</button>
            <button class="btn ghost" onclick="testEveMicReaction()">Test reaction</button>
            <p class="stat-sub" id="eve-mic-status">Mic data arrives through existing UART EVENT/status traffic.</p>
          </div>
        </div>
        <div class="card more-deck-card">
          <div class="card-header">Memory / learning viewer</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Entries</span><span class="value mono" id="eve-learning-count">—</span></div>
            <div class="log-item mono" style="word-break:break-all;max-height:8em;overflow:auto" id="eve-learning-list">—</div>
          </div>
        </div>
        <div class="card more-deck-card">
          <div class="card-header">Live UART monitor</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Peer</span><span class="value mono" id="eve-uart-peer">—</span></div>
            <div class="log-item mono" style="word-break:break-all;max-height:8em;overflow:auto" id="eve-uart-monitor">—</div>
          </div>
        </div>
      </div>
    </div>

    <!-- POWER -->
    <div class="page more-subpage" id="page-power">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">ENERGY · POWER</p>
          <h2 class="page-title subpage-title">Power</h2>
          <p class="page-lead">Battery detail, trend sparkline, and low-power sleep when supported.</p>
        </div>
      </header>
      <div class="subpage-deck subpage-deck--2col">
      <div class="card more-deck-card">
        <div class="card-header">Battery</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Voltage</span><span class="value" id="pwr-voltage">-</span></div>
          <div class="status-row"><span class="label">Est. runtime</span><span class="value" id="pwr-runtime">-</span></div>
          <div class="progress-bar" style="margin:8px 0"><div class="progress-fill" id="pwr-bar" style="width:80%"></div></div>
          <div class="spark-label">Battery % (last ~30s)</div>
          <canvas id="spark-batt" class="spark-wrap" width="300" height="40"></canvas>
        </div>
      </div>
      <div class="card more-deck-card">
        <div class="card-header">Sleep</div>
        <div class="card-body">
          <button class="btn btn-ghost" onclick="apiCall('/api/sleep')">Enter Sleep</button>
        </div>
      </div>
      </div>
    </div>

    <!-- NETWORK — Fleet uplink console -->
    <div class="page net-page" id="page-network" data-net-state="ap">
      <header class="page-head net-page-head">
        <p class="net-page-kicker">BnL · Fleet communications</p>
        <h2 class="page-title">Network</h2>
        <p class="page-lead">Live uplink, mesh topology, and provisioning — same backbone as the CYD console.</p>
      </header>

      <section class="net-hero" aria-labelledby="net-hero-title">
        <div class="net-hero-bg" aria-hidden="true"></div>
        <div class="net-hero-scan" aria-hidden="true"></div>
        <div class="net-hero-inner">
          <div class="net-hero-orbit" id="net-hero-orbit" data-link="ap" aria-hidden="true">
            <span class="net-hero-core"></span>
            <span class="net-hero-ring net-hero-ring--1"></span>
            <span class="net-hero-ring net-hero-ring--2"></span>
            <span class="net-hero-ring net-hero-ring--3"></span>
          </div>
          <div class="net-hero-copy">
            <p class="net-hero-label" id="net-hero-title">Uplink</p>
            <p class="net-hero-state" id="net-hero-state">Access point</p>
            <p class="net-hero-sub" id="net-hero-sub">Broadcasting operator SSID — join to configure STA</p>
            <div class="net-hero-metrics">
              <div class="net-metric">
                <span class="net-metric-lbl">RSSI (STA)</span>
                <span class="net-metric-val mono" id="net-rssi-hero">—</span>
              </div>
              <div class="net-metric">
                <span class="net-metric-lbl">Station IPv4</span>
                <span class="net-metric-val mono" id="net-hero-ip">—</span>
              </div>
            </div>
            <div class="net-rssi-track" role="progressbar" aria-label="Signal strength" aria-valuemin="-100" aria-valuemax="-30" aria-valuenow="-80" id="net-rssi-track">
              <div class="net-rssi-fill" id="net-rssi-hero-bar" style="width:0%"></div>
            </div>
            <p class="net-rssi-caption" id="net-rssi-caption">No STA link — RSSI shown when connected</p>
          </div>
        </div>
      </section>

      <div class="net-split">
        <div class="card net-glass-card net-topology-card">
          <div class="card-header net-card-head">
            <span>Fleet topology</span>
            <span class="net-chip net-chip--dim" id="net-topo-chip">Mesh</span>
          </div>
          <div class="card-body net-topology-body">
            <svg class="net-topology-svg" id="topology-svg" viewBox="0 0 360 200" xmlns="http://www.w3.org/2000/svg" aria-label="Fleet topology diagram">
              <defs>
                <linearGradient id="net-glow" x1="0%" y1="0%" x2="100%" y2="100%">
                  <stop offset="0%" style="stop-color:var(--accent);stop-opacity:0.35"/>
                  <stop offset="100%" style="stop-color:var(--rust);stop-opacity:0.08"/>
                </linearGradient>
                <filter id="net-blur" x="-20%" y="-20%" width="140%" height="140%">
                  <feGaussianBlur stdDeviation="1.2" result="b"/>
                  <feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge>
                </filter>
              </defs>
              <rect width="360" height="200" class="net-topo-bg"/>
              <g class="net-topo-links" stroke="var(--border2)" fill="none" stroke-width="1.5" stroke-linecap="round">
                <path class="net-link net-link--a" d="M 130 78 L 72 148"/>
                <path class="net-link net-link--b" d="M 180 72 L 180 148"/>
                <path class="net-link net-link--c" d="M 230 78 L 288 148"/>
              </g>
              <g class="net-topo-nodes">
                <circle cx="180" cy="48" r="28" class="net-node net-node--hub"/>
                <text x="180" y="52" text-anchor="middle" class="net-node-txt">BASE</text>
                <text x="180" y="64" text-anchor="middle" class="net-node-sub">WALL·E</text>
                <circle cx="72" cy="158" r="22" class="net-node net-node--ok"/>
                <text x="72" y="162" text-anchor="middle" class="net-node-lbl">Dock</text>
                <circle cx="180" cy="158" r="22" class="net-node net-node--ok"/>
                <text x="180" y="162" text-anchor="middle" class="net-node-lbl">CYD</text>
                <circle cx="288" cy="158" r="22" class="net-node net-node--dim"/>
                <text x="288" y="162" text-anchor="middle" class="net-node-lbl">Vision</text>
              </g>
            </svg>
          </div>
        </div>

        <div class="net-spec-col">
          <div class="card net-glass-card">
            <div class="card-header net-card-head">
              <span>Access point</span>
              <span class="net-chip net-chip--live">Always on</span>
            </div>
            <div class="card-body">
              <div class="net-spec-row"><span class="net-spec-k">SSID</span><span class="net-spec-v mono" id="net-ap">WALL-E-Control</span></div>
              <div class="net-spec-row"><span class="net-spec-k">IPv4</span><span class="net-spec-v mono" id="net-ap-ip">—</span></div>
              <div class="net-spec-row"><span class="net-spec-k">Clients</span><span class="net-spec-v mono" id="net-ap-clients">0</span></div>
            </div>
          </div>
          <div class="card net-glass-card">
            <div class="card-header net-card-head">
              <span>Station (home)</span>
              <span class="net-chip" id="net-sta-chip">STA</span>
            </div>
            <div class="card-body">
              <div class="net-spec-row"><span class="net-spec-k">SSID</span><span class="net-spec-v mono" id="net-sta">Not connected</span></div>
              <div class="net-spec-row"><span class="net-spec-k">IPv4</span><span class="net-spec-v mono" id="net-sta-ip">—</span></div>
              <button type="button" class="btn btn-small net-btn-provision" onclick="showNetworkForm()">Provision STA link</button>
            </div>
          </div>
        </div>
      </div>

      <div class="card net-glass-card net-mission-card" id="network-form-card" style="display:none">
        <div class="card-header net-card-head">
          <span>Provisioning</span>
          <span class="net-chip net-chip--warn">Credentials</span>
        </div>
        <div class="card-body">
          <p class="net-mission-lead">Scan for SSIDs, select a row, then transmit credentials to the base radio.</p>
          <div class="form-group"><label for="net-ssid">Network (SSID)</label><input type="text" id="net-ssid" placeholder="SSID" autocomplete="off"></div>
          <div class="form-group"><label for="net-pass">Password</label><input type="password" id="net-pass" placeholder="Password" autocomplete="off"></div>
          <div id="network-list" class="net-list net-list--rich"></div>
          <div class="net-mission-actions">
            <button type="button" class="btn btn-small btn-ghost" onclick="doScan()">Scan airspace</button>
            <button type="button" class="btn" id="net-connect-btn" onclick="doConnect()">Establish link</button>
          </div>
        </div>
      </div>
    </div>

    <!-- FILES -->
    <div class="page more-subpage" id="page-files">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">STORAGE · FILES</p>
          <h2 class="page-title subpage-title">Files</h2>
          <p class="page-lead">Onboard storage listing when <span class="mono">/api/files/list</span> is enabled.</p>
        </div>
      </header>
      <div class="subpage-deck">
        <div class="card more-deck-card">
          <div class="card-header">Storage</div>
          <div class="card-body">
            <div id="file-list" class="log-list">
              <div class="log-item value dim">No file API - use /api/files/list</div>
            </div>
            <button type="button" class="btn btn-small" onclick="refreshFiles()" style="margin-top:8px">Refresh</button>
          </div>
        </div>
      </div>
    </div>

    <!-- SAFETY -->
    <div class="page more-subpage" id="page-safety">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">OPS · SAFETY</p>
          <h2 class="page-title subpage-title">Safety</h2>
          <p class="page-lead">Emergency stop and policy toggles — use E-STOP in the bar for immediate cut.</p>
        </div>
      </header>
      <div class="subpage-deck">
      <div class="card more-deck-card">
        <div class="card-header">Emergency</div>
        <div class="card-body">
          <button class="btn btn-stop" onclick="doEStop()">Emergency Stop</button>
        </div>
      </div>
      <div class="card more-deck-card">
        <div class="card-header">Safety</div>
        <div class="card-body">
          <div class="status-row"><span class="label">Child-safe</span><span class="value"><input type="checkbox" id="safety-child"></span></div>
          <p class="stat-sub" style="margin:0">Geo-fence is configured in the panel below (browser only until firmware adds enforcement).</p>
        </div>
      </div>
      <div class="card more-deck-card" id="geofence-card">
        <div class="card-header">Geo-fence</div>
        <div class="card-body geofence-panel">
          <p class="geofence-lead">Circular boundary from a center point. Uses live GPS from <span class="mono">/api/autonomy</span> (same as Navigation). Settings are stored in this browser.</p>
          <div class="form-group geofence-row">
            <label class="geofence-check"><input type="checkbox" id="geofence-enabled"> Enable geo-fence monitoring</label>
          </div>
          <div class="geofence-grid">
            <div class="form-group"><label for="geofence-lat">Center latitude</label><input type="number" id="geofence-lat" step="any" placeholder="e.g. 37.7749"></div>
            <div class="form-group"><label for="geofence-lon">Center longitude</label><input type="number" id="geofence-lon" step="any" placeholder="-122.4194"></div>
          </div>
          <div class="form-group">
            <label for="geofence-radius">Radius <span class="geofence-readout" id="geofence-radius-readout">50 m</span></label>
            <input type="range" id="geofence-radius" min="5" max="5000" value="50" step="5">
          </div>
          <div class="geofence-grid">
            <div class="form-group"><label for="geofence-margin">Inner margin (buffer)</label><input type="number" id="geofence-margin" min="0" max="500" value="2" step="1" title="Shrinks the allowed disk inward (m)"></div>
            <div class="form-group"><label for="geofence-mode">When outside boundary</label>
              <select id="geofence-mode">
                <option value="warn">Banner warning</option>
                <option value="soft">Banner + activity log</option>
                <option value="estop">E-STOP (HTTP /stop)</option>
              </select>
            </div>
          </div>
          <div class="geofence-actions">
            <button type="button" class="btn btn-small" id="geofence-use-gps" onclick="geofenceUseRobotGps()">Use robot GPS as center</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="geofenceSave()">Save</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="geofenceResetDefaults()">Defaults</button>
          </div>
          <div class="geofence-status" id="geofence-status" role="status">
            <span class="geofence-pill" id="geofence-pill">Off</span>
            <span class="geofence-detail" id="geofence-detail">—</span>
          </div>
        </div>
      </div>
      </div>
    </div>

    <!-- LOGS -->
    <div class="page more-subpage" id="page-logs">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">DIAG · LOGS</p>
          <h2 class="page-title subpage-title">Logs</h2>
          <p class="page-lead">Filter by category and severity, then export for debugging.</p>
        </div>
      </header>
      <div class="subpage-deck">
      <div class="card more-deck-card">
        <div class="card-header">System logs</div>
        <div class="card-body">
          <div class="log-filters">
            <select id="log-cat" onchange="filterLogs()">
              <option value="all">All</option><option value="system">System</option><option value="nav">Navigation</option>
              <option value="ai">AI</option><option value="sensor">Sensor</option><option value="safety">Safety</option>
              <option value="network">Network</option><option value="user">User</option>
            </select>
            <select id="log-sev" onchange="filterLogs()">
              <option value="all">Any severity</option><option value="info">Info</option><option value="warn">Warning</option>
              <option value="error">Error</option><option value="critical">Critical</option>
            </select>
            <button type="button" class="btn btn-small btn-ghost" onclick="exportLogs()">Export</button>
          </div>
          <div id="log-list" class="log-list">
            <div class="log-item" data-cat="system" data-sev="info"><span class="log-time">--:--</span>[info] System ready</div>
          </div>
        </div>
      </div>
      </div>
    </div>

    <!-- SECURITY -->
    <div class="page more-subpage" id="page-security">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">ACCESS · SECURITY</p>
          <h2 class="page-title subpage-title">Security</h2>
          <p class="page-lead">Trusted devices and access rules when the firmware adds pairing.</p>
        </div>
      </header>
      <div class="subpage-deck subpage-deck--2col">
        <div class="card more-deck-card">
          <div class="card-header">Access</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Trusted devices</span><span class="value dim" id="sec-trusted-readout">None (firmware pairing TBD)</span></div>
            <button type="button" class="btn btn-small btn-ghost" disabled title="Coming soon">Add device</button>
            <p class="stat-sub" style="margin-top:10px">Pairing and MAC allowlists will bind to the base when exposed on <span class="mono">/api/security</span>.</p>
          </div>
        </div>
        <div class="card more-deck-card">
          <div class="card-header">Policies</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Local console</span><span class="value dim">This browser session</span></div>
            <div class="status-row"><span class="label">API auth</span><span class="value dim">Open (AP / LAN)</span></div>
            <div class="status-row"><span class="label">OTA / keys</span><span class="value dim">Configure on device</span></div>
          </div>
        </div>
      </div>
    </div>

    <!-- SETTINGS -->
    <div class="page more-subpage" id="page-settings">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">CONSOLE · SETTINGS</p>
          <h2 class="page-title subpage-title">Console settings</h2>
          <p class="page-lead">Connection targets, drive limits, and how this browser UI behaves. Stored locally except speed profile (robot).</p>
        </div>
      </header>

      <div class="settings-layout">
        <div class="card settings-feature more-deck-card">
          <div class="card-header">Connection</div>
          <div class="card-body settings-section">
            <div class="form-group"><label for="set-base-url">Base URL</label><input type="text" id="set-base-url" placeholder="http://192.168.4.1" autocomplete="url"><span class="field-hint">Empty = same origin as this page.</span></div>
            <div class="form-group"><label for="set-ws-url">WebSocket</label><input type="text" id="set-ws-url" placeholder="ws://192.168.4.1/ws" autocomplete="off"><span class="field-hint">Optional telemetry / command stream.</span></div>
            <div class="settings-btn-row">
              <button type="button" class="btn btn-small" onclick="applySettings()">Apply &amp; reconnect</button>
              <button type="button" class="btn btn-small btn-ghost" onclick="testConnectionPing()">Test HTTP</button>
              <button type="button" class="btn btn-small btn-ghost" onclick="copyBaseUrl()">Copy base URL</button>
            </div>
            <div class="settings-ping" id="settings-ping" role="status"></div>
          </div>
        </div>

        <div class="card settings-feature">
          <div class="card-header">Drive limits (robot)</div>
          <div class="card-body">
            <p class="stat-sub" style="margin:0 0 10px">Maps to <span class="mono">/settings</span> · max motor command cap.</p>
            <div class="form-group"><label for="set-speed-profile">Speed profile</label>
              <select id="set-speed-profile" onchange="applySpeedFromSettings()">
                <option value="low">Low (128)</option>
                <option value="normal" selected>Normal (200)</option>
                <option value="high">High (255)</option>
              </select>
            </div>
            <div class="status-row"><span class="label">Current max</span><span class="value mono" id="set-max-speed-readout">—</span></div>
          </div>
        </div>

        <div class="card settings-feature more-deck-card">
          <div class="card-header">Display &amp; comfort</div>
          <div class="card-body">
            <label class="settings-check"><input type="checkbox" id="set-dense"> <strong>Dense layout</strong> — tighter spacing on large screens.</label>
            <label class="settings-check"><input type="checkbox" id="set-reduced-motion"> <strong>Reduce motion</strong> — fewer transitions.</label>
            <div class="form-group" style="margin-top:12px"><label for="set-ui-scale">UI scale</label>
              <select id="set-ui-scale">
                <option value="1">Default</option>
                <option value="1.05">105%</option>
                <option value="1.1">110%</option>
              </select>
            </div>
            <button type="button" class="btn btn-small" onclick="saveUiPreferences()">Save display prefs</button>
          </div>
        </div>

        <div class="card settings-feature more-deck-card">
          <div class="card-header">Nodes snapshot</div>
          <div class="card-body">
            <p class="stat-sub" style="margin:0 0 10px">Same fleet pills as the top strip; useful when tuning Wi‑Fi.</p>
            <div class="settings-node-grid" aria-label="Node status">
              <span class="node-pill off" data-pill="base">BASE</span>
              <span class="node-pill off" data-pill="master">CYD</span>
              <span class="node-pill off" data-pill="audio">AUD</span>
              <span class="node-pill off" data-pill="dock">DOCK</span>
              <span class="node-pill off" data-pill="vision">VIS</span>
              <span class="node-pill off" data-pill="eve">EVE</span>
            </div>
            <p class="stat-sub" id="settings-node-updated">Updated with <span class="mono">/api/system/health</span> + <span class="mono">/api/living/telemetry</span> (EVE pill).</p>
          </div>
        </div>

        <div class="card settings-feature settings-wide more-deck-card">
          <div class="card-header">About</div>
          <div class="card-body">
            <div class="status-row"><span class="label">Console</span><span class="value">WALL-E LROS web</span></div>
            <div class="status-row"><span class="label">Storage</span><span class="value dim" id="settings-ls-hint">localStorage for URLs &amp; UI</span></div>
            <button type="button" class="btn btn-small btn-ghost" onclick="restoreDismissedPanels()">Restore hidden panels</button>
            <button type="button" class="btn btn-small btn-ghost" onclick="clearLocalConsolePrefs()">Reset local UI prefs</button>
          </div>
        </div>
      </div>
    </div>

    <!-- DEVELOPER -->
    <div class="page more-subpage" id="page-developer">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">DEV · TOOLS</p>
          <h2 class="page-title subpage-title">Developer</h2>
          <p class="page-lead">Diagnostics, simulated console, API reference, and raw HTTP GET helper.</p>
        </div>
      </header>
      <div class="subpage-deck subpage-deck--2col">
        <div class="card more-deck-card">
          <div class="card-header">Hardware diagnostics</div>
          <div class="card-body">
            <div class="status-row"><span class="label">MCU</span><span class="value dim">ESP32-S3 (client)</span></div>
            <div class="status-row"><span class="label">Firmware</span><span class="value dim" id="dev-fw">—</span></div>
          </div>
        </div>
        <div class="card more-deck-card">
          <div class="card-header">Serial console (simulated)</div>
          <div class="card-body">
            <div class="dev-console" id="dev-console"></div>
            <button type="button" class="btn btn-small" onclick="devConsoleSim()">Add line</button>
          </div>
        </div>
        <div class="card more-deck-card subpage-card-span">
          <div class="card-header">API endpoints</div>
          <div class="card-body">
            <div class="dev-api-grid" aria-label="Common REST paths">
              <code>/drive?left=&amp;right=</code><code>/stop</code><code>/speed?value=</code><code>/wifi/status</code><code>/wifi/scan</code><code>/wifi/connect</code>
              <code>/api/autonomy</code><code>/api/autonomy/enable</code><code>/api/autonomy/set_home</code>
              <code>/api/navigation/route</code><code>/api/navigation/status</code><code>/api/navigation/stop</code>
              <code>/imu/status</code><code>/battery/status</code><code>/servo/set</code><code>/settings</code>
              <code>/api/eve/status</code><code>/api/living/telemetry</code><code>/api/motion/operator</code>
            </div>
          </div>
        </div>
        <div class="card more-deck-card subpage-card-span">
          <div class="card-header">Raw command</div>
          <div class="card-body">
            <div class="dev-raw-row">
              <input type="text" id="dev-cmd" placeholder="/api/autonomy" autocomplete="off">
              <button type="button" class="btn btn-small" onclick="devFetch()">GET</button>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- MORE (nav grid) -->
    <div class="page more-hub-page" id="page-more">
      <header class="subpage-head">
        <div class="subpage-head-main">
          <p class="subpage-kicker">LROS · MENU</p>
          <h2 class="page-title subpage-title">More</h2>
          <p class="page-lead">Secondary screens — navigation, vision, audio, AI, missions, sequence generator, telemetry, power, and tools.</p>
        </div>
      </header>
      <div class="nav-grid more-hub-grid">
        <a class="nav-tile" href="#" onclick="switchTab('navigation');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><circle cx="12" cy="12" r="10"/><path d="M2 12h20M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"/></svg>Navigation</a>
        <a class="nav-tile" href="#" onclick="switchTab('vision');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M23 19a2 2 0 0 1-2 2H3a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h4l2-3h6l2 3h4a2 2 0 0 1 2 2z"/><circle cx="12" cy="13" r="4"/></svg>Vision</a>
        <a class="nav-tile" href="#" onclick="switchTab('audio');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><polygon points="11 5 6 9 2 9 2 15 6 15 11 19 11 5"/><path d="M19.07 4.93a10 10 0 0 1 0 14.14M15.54 8.46a5 5 0 0 1 0 7.07"/></svg>Audio</a>
        <a class="nav-tile" href="#" onclick="switchTab('eve');return false" title="EVE UART, bond, assist"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><circle cx="8" cy="12" r="3"/><circle cx="16" cy="9" r="2.5"/><path d="M11 12h2M14 7l-1 2"/></svg>EVE</a>
        <a class="nav-tile" href="#" onclick="switchTab('ai');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M12 2a10 10 0 0 1 10 10c0 5.5-4.5 10-10 10S2 17.5 2 12 6.5 2 12 2z"/><circle cx="12" cy="12" r="2"/></svg>AI &amp; Auto</a>
        <a class="nav-tile" href="#" onclick="switchTab('missions');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><polyline points="22 12 18 12 15 21 9 3 6 12 2 12"/></svg>Missions</a>
        <a class="nav-tile" href="#" onclick="switchTab('sequence');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><rect x="2" y="5" width="20" height="14" rx="1"/><line x1="7" y1="5" x2="7" y2="19"/><line x1="17" y1="5" x2="17" y2="19"/><path d="M10 9h1M13 9h1M10 12h1M13 12h1M10 15h1M13 15h1" stroke-linecap="round"/></svg>Sequence</a>
        <a class="nav-tile" href="#" onclick="switchTab('telemetry');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M12 20V10"/><path d="M18 20V4"/><path d="M6 20v-4"/></svg>Telemetry</a>
        <a class="nav-tile" href="#" onclick="switchTab('power');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"/></svg>Power</a>
        <a class="nav-tile" href="#" onclick="switchTab('files');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/></svg>Files</a>
        <a class="nav-tile" href="#" onclick="switchTab('safety');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z"/></svg>Safety</a>
        <a class="nav-tile" href="#" onclick="switchTab('logs');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><path d="M14 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V8z"/><polyline points="14 2 14 8 20 8"/><line x1="16" y1="13" x2="8" y2="13"/><line x1="16" y1="17" x2="8" y2="17"/></svg>Logs</a>
        <a class="nav-tile" href="#" onclick="switchTab('security');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2"/><path d="M7 11V7a5 5 0 0 1 10 0v4"/></svg>Security</a>
        <a class="nav-tile" href="#" onclick="switchTab('developer');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><polyline points="16 18 22 12 16 6"/><polyline points="8 6 2 12 8 18"/></svg>Developer</a>
        <a class="nav-tile" href="#" onclick="switchTab('settings');return false"><svg viewBox="0 0 24 24" fill="none" stroke-width="2"><circle cx="12" cy="12" r="3"/><path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2"/></svg>Settings</a>
      </div>
    </div>
  </main>

  <footer id="estop-bar">
    <span class="estop-hint">Hold to stop · releases motors</span>
    <button id="estop-btn" type="button" aria-label="Hold to emergency stop">E-STOP</button>
  </footer>
</div>

<div id="feedback-toast" class="feedback-toast" role="status" aria-live="polite"></div>

<div id="command-hud" class="command-hud" aria-live="polite">
  <span class="hud-pill" id="hud-latency" title="Last command RTT">— ms</span>
  <span class="hud-pill hud-cmd" id="hud-last-cmd" title="Last API command">Ready</span>
  <span class="hud-pill" id="hud-queue" title="Commands in flight">In flight: 0</span>
</div>

<div id="toast-stack" aria-live="polite"></div>

<script>
/**
 * WalleConnection — singleton WebSocket manager with HTTP fallback.
 * Base firmware may not expose WS yet; connection state still drives UI badges.
 */
(function (global) {
  'use strict';

  var state = 'disconnected'; // disconnected | connecting | ws | http | error
  var ws = null;
  var reconnectTimer = null;
  var reconnectAttempt = 0;
  var listeners = [];
  var messageListeners = [];
  var lastWsUrl = '';

  function getWsUrl() {
    return localStorage.getItem('walle_ws_url') || '';
  }

  function setWsUrl(url) {
    if (url === undefined || url === null) return;
    localStorage.setItem('walle_ws_url', url.trim());
  }

  function emitState() {
    listeners.forEach(function (fn) {
      try {
        fn(state);
      } catch (e) {}
    });
    var el = document.getElementById('ws-status-badge');
    if (el) {
      el.dataset.state = state;
      el.textContent =
        state === 'ws'
          ? 'Live (WS)'
          : state === 'http'
            ? 'HTTP'
            : state === 'connecting'
              ? '…'
              : state === 'error'
                ? 'Error'
                : 'Offline';
    }
  }

  function setState(s) {
    state = s;
    emitState();
  }

  function onState(fn) {
    listeners.push(fn);
    fn(state);
  }

  function onMessage(fn) {
    messageListeners.push(fn);
  }

  function dispatchMessage(obj) {
    messageListeners.forEach(function (fn) {
      try {
        fn(obj);
      } catch (e) {}
    });
  }

  function clearReconnect() {
    if (reconnectTimer) {
      clearTimeout(reconnectTimer);
      reconnectTimer = null;
    }
  }

  function scheduleReconnect() {
    clearReconnect();
    var url = getWsUrl();
    if (!url) return;
    var delay = Math.min(30000, 1000 * Math.pow(2, reconnectAttempt));
    reconnectTimer = setTimeout(function () {
      reconnectAttempt++;
      connect();
    }, delay);
  }

  function connect() {
    var url = getWsUrl();
    if (!url) {
      setState('http');
      return;
    }

    if (ws && ws.readyState === WebSocket.OPEN) return;

    setState('connecting');
    try {
      ws = new WebSocket(url);
    } catch (e) {
      setState('error');
      scheduleReconnect();
      return;
    }

    ws.onopen = function () {
      reconnectAttempt = 0;
      setState('ws');
      lastWsUrl = url;
    };

    ws.onclose = function () {
      setState('http');
      scheduleReconnect();
    };

    ws.onerror = function () {
      setState('error');
    };

    ws.onmessage = function (ev) {
      try {
        var o = JSON.parse(ev.data);
        dispatchMessage(o);
      } catch (e) {
        dispatchMessage({ raw: ev.data });
      }
    };
  }

  function disconnect() {
    clearReconnect();
    if (ws) {
      try {
        ws.close();
      } catch (e) {}
      ws = null;
    }
    setState('disconnected');
  }

  function send(obj) {
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(typeof obj === 'string' ? obj : JSON.stringify(obj));
      return true;
    }
    return false;
  }

  global.WalleConnection = {
    getState: function () {
      return state;
    },
    getWsUrl: getWsUrl,
    setWsUrl: setWsUrl,
    onState: onState,
    onMessage: onMessage,
    connect: connect,
    disconnect: disconnect,
    send: send,
    /** Call after first successful fetch to show HTTP as live */
    markHttpOk: function () {
      if (state !== 'ws') setState('http');
    },
  };
})(typeof window !== 'undefined' ? window : this);


/**
 * Grid A* + Catmull-Rom spline smoothing for navigation preview (client-side).
 * Coordinates are abstract grid units; map view scales to canvas pixels.
 */
(function (global) {
  'use strict';

  function key(x, y) {
    return x + ',' + y;
  }

  function parseKey(k) {
    var p = k.split(',');
    return { x: parseInt(p[0], 10), y: parseInt(p[1], 10) };
  }

  /** A* on 4-connected grid; obstacles: Set of "x,y" strings */
  function astar(start, goal, obstacles, w, h) {
    var open = [];
    var came = {};
    var g = {};
    var sk = key(start.x, start.y),
      gk = key(goal.x, goal.y);
    g[sk] = 0;
    open.push({ x: start.x, y: start.y, f: heuristic(start, goal) });

    function heuristic(a, b) {
      return Math.abs(a.x - b.x) + Math.abs(a.y - b.y);
    }

    function neighbors(x, y) {
      var o = [];
      [
        [0, 1],
        [0, -1],
        [1, 0],
        [-1, 0],
      ].forEach(function (d) {
        var nx = x + d[0],
          ny = y + d[1];
        if (nx < 0 || ny < 0 || nx >= w || ny >= h) return;
        if (obstacles.has(key(nx, ny))) return;
        o.push({ x: nx, y: ny });
      });
      return o;
    }

    var guard = 0;
    while (open.length && guard++ < w * h * 4) {
      open.sort(function (a, b) {
        return a.f - b.f;
      });
      var cur = open.shift();
      var ck = key(cur.x, cur.y);
      if (ck === gk) {
        var path = [];
        var at = gk;
        while (at) {
          path.push(parseKey(at));
          at = came[at];
        }
        return path.reverse();
      }
      neighbors(cur.x, cur.y).forEach(function (n) {
        var nk = key(n.x, n.y);
        var tentative = g[ck] + 1;
        if (g[nk] === undefined || tentative < g[nk]) {
          came[nk] = ck;
          g[nk] = tentative;
          open.push({ x: n.x, y: n.y, f: tentative + heuristic(n, goal) });
        }
      });
    }
    return [];
  }

  /** Catmull-Rom through points (at least 2) */
  function catmullRom(points, segments) {
    if (points.length < 2) return points.slice();
    segments = segments || 8;
    var out = [];
    for (var i = 0; i < points.length - 1; i++) {
      var p0 = points[Math.max(0, i - 1)];
      var p1 = points[i];
      var p2 = points[Math.min(points.length - 1, i + 1)];
      var p3 = points[Math.min(points.length - 1, i + 2)];
      for (var t = 0; t < segments; t++) {
        var u = t / segments;
        var u2 = u * u,
          u3 = u2 * u;
        var x =
          0.5 *
          (2 * p1.x +
            (-p0.x + p2.x) * u +
            (2 * p0.x - 5 * p1.x + 4 * p2.x - p3.x) * u2 +
            (-p0.x + 3 * p1.x - 3 * p2.x + p3.x) * u3);
        var y =
          0.5 *
          (2 * p1.y +
            (-p0.y + p2.y) * u +
            (2 * p0.y - 5 * p1.y + 4 * p2.y - p3.y) * u2 +
            (-p0.y + 3 * p1.y - 3 * p2.y + p3.y) * u3);
        out.push({ x: x, y: y });
      }
    }
    out.push(points[points.length - 1]);
    return out;
  }

  function pathLength(pts) {
    var L = 0;
    for (var i = 1; i < pts.length; i++) {
      var dx = pts[i].x - pts[i - 1].x,
        dy = pts[i].y - pts[i - 1].y;
      L += Math.sqrt(dx * dx + dy * dy);
    }
    return L;
  }

  global.PathPlanner = {
    astar: astar,
    catmullRom: catmullRom,
    pathLength: pathLength,
    key: key,
  };
})(typeof window !== 'undefined' ? window : this);


/**
 * Proximity alerts via Web Audio API (beeps). Mute persisted in localStorage.
 */
(function (global) {
  'use strict';

  var ctx = null;
  var muted = localStorage.getItem('walle_proximity_mute') === '1';

  function ensureCtx() {
    if (!ctx) {
      var AC = window.AudioContext || window.webkitAudioContext;
      if (!AC) return null;
      ctx = new AC();
    }
    return ctx;
  }

  function beep(freq, duration, vol) {
    if (muted) return;
    var c = ensureCtx();
    if (!c) return;
    if (c.state === 'suspended') c.resume();
    var o = c.createOscillator();
    var g = c.createGain();
    o.connect(g);
    g.connect(c.destination);
    o.frequency.value = freq || 880;
    o.type = 'sine';
    g.gain.value = vol || 0.08;
    o.start();
    setTimeout(function () {
      o.stop();
    }, duration || 120);
  }

  /** distance 0..1 where 1 = far, 0 = critical */
  function alertFromProximity(distNorm) {
    if (distNorm > 0.35) return;
    var urgency = 1 - distNorm / 0.35;
    beep(400 + urgency * 400, 80 + urgency * 120, 0.05 + urgency * 0.08);
  }

  function setMuted(m) {
    muted = !!m;
    localStorage.setItem('walle_proximity_mute', muted ? '1' : '0');
    var btn = document.getElementById('prox-mute-btn');
    if (btn) btn.textContent = muted ? '\uD83D\uDD15' : '\uD83D\uDD14';
  }

  function isMuted() {
    return muted;
  }

  global.ProximityAlert = {
    beep: beep,
    alertFromProximity: alertFromProximity,
    setMuted: setMuted,
    isMuted: isMuted,
  };
})(typeof window !== 'undefined' ? window : this);


/**
 * Navigation page — real-world context when the browser can reach the internet
 * and (optionally) the robot reports Wi‑Fi station link. Uses public APIs (no keys).
 */
(function (global) {
  'use strict';

  var GEO_CACHE_MS = 10 * 60 * 1000;
  var WEATHER_CACHE_MS = 15 * 60 * 1000;
  var MIN_REFRESH_MS = 8000;

  var lastFullFetch = 0;
  var lastGeo = null;
  var lastWeather = null;
  var lastLat = null;
  var lastLon = null;
  var deviceOverride = null;

  function $(id) {
    return document.getElementById(id);
  }

  function setText(id, text) {
    var el = $(id);
    if (el) el.textContent = text;
  }

  function setHidden(id, hidden) {
    var el = $(id);
    if (el) el.hidden = !!hidden;
  }

  function wmoCodeDesc(code) {
    if (code == null) return '';
    var c = Number(code);
    if (c === 0) return 'Clear';
    if (c <= 3) return 'Partly cloudy';
    if (c <= 48) return 'Fog';
    if (c <= 57) return 'Drizzle';
    if (c <= 67) return 'Rain';
    if (c <= 77) return 'Snow';
    if (c <= 82) return 'Rain showers';
    if (c <= 86) return 'Snow showers';
    if (c <= 99) return 'Storm';
    return '';
  }

  function routerLabel(wifi) {
    if (!wifi || wifi.state !== 2) return 'Robot: not on Wi‑Fi';
    var ssid = wifi.sta_ssid || 'LAN';
    var ip = wifi.sta_ip || '';
    return ip ? 'Robot: ' + ssid + ' · ' + ip : 'Robot: ' + ssid;
  }

  function updatePills(wifi, browserOnline, wanOk, rttMs) {
    var b = $('nav-pill-browser');
    if (b) {
      b.textContent = browserOnline ? 'Browser: online' : 'Browser: offline';
      b.className = 'nav-pill' + (browserOnline ? ' nav-pill-ok' : ' nav-pill-warn');
    }
    var r = $('nav-pill-router');
    if (r) {
      var linked = wifi && wifi.state === 2;
      r.textContent = linked ? routerLabel(wifi) : 'Robot: AP / no STA';
      r.className = 'nav-pill' + (linked ? ' nav-pill-ok' : ' nav-pill-dim');
    }
    var w = $('nav-pill-wan');
    if (w) {
      if (!browserOnline) {
        w.textContent = 'Internet: —';
        w.className = 'nav-pill nav-pill-dim';
      } else if (wanOk) {
        w.textContent = rttMs != null ? 'Internet: OK · ~' + rttMs + ' ms' : 'Internet: OK';
        w.className = 'nav-pill nav-pill-ok';
      } else {
        w.textContent = 'Internet: unreachable';
        w.className = 'nav-pill nav-pill-warn';
      }
    }
  }

  function updateRouterRows(wifi) {
    setText('nav-sta-ip', wifi && wifi.sta_ip != null ? String(wifi.sta_ip) : '—');
    setText('nav-sta-ssid', wifi && wifi.sta_ssid ? String(wifi.sta_ssid) : '—');
    setText('nav-rssi', wifi && wifi.rssi != null ? wifi.rssi + ' dBm' : '—');
  }

  async function fetchJsonWithTiming(url) {
    var t0 = performance.now();
    var res = await fetch(url, { cache: 'no-store' });
    var ms = Math.round(performance.now() - t0);
    if (!res.ok) throw new Error(String(res.status));
    var j = await res.json();
    return { ms: ms, data: j };
  }

  /** Try ipapi.co, then ipwho.is */
  async function fetchGeo() {
    try {
      var r = await fetchJsonWithTiming('https://ipapi.co/json/');
      var d = r.data;
      if (d && d.error) throw new Error(d.reason || 'ipapi');
      return {
        ms: r.ms,
        ip: d.ip,
        city: d.city,
        region: d.region,
        country: d.country_name || d.country,
        lat: d.latitude != null ? Number(d.latitude) : null,
        lon: d.longitude != null ? Number(d.longitude) : null,
        org: d.org,
        tz: d.timezone
      };
    } catch (e1) {
      var r2 = await fetchJsonWithTiming('https://ipwho.is/');
      var d2 = r2.data;
      if (!d2 || d2.success === false) throw new Error('geo');
      return {
        ms: r2.ms,
        ip: d2.ip,
        city: d2.city,
        region: d2.region,
        country: d2.country,
        lat: d2.latitude != null ? Number(d2.latitude) : null,
        lon: d2.longitude != null ? Number(d2.longitude) : null,
        org: d2.connection && d2.connection.isp ? d2.connection.isp : '',
        tz: d2.timezone && d2.timezone.id
      };
    }
  }

  async function fetchWeather(lat, lon) {
    var u =
      'https://api.open-meteo.com/v1/forecast?latitude=' +
      encodeURIComponent(lat) +
      '&longitude=' +
      encodeURIComponent(lon) +
      '&current=temperature_2m,relative_humidity_2m,weather_code,wind_speed_10m&timezone=auto';
    var res = await fetch(u, { cache: 'no-store' });
    if (!res.ok) throw new Error('wx');
    var j = await res.json();
    var cur = j.current;
    if (!cur) return null;
    return {
      temp: cur.temperature_2m,
      rh: cur.relative_humidity_2m,
      code: cur.weather_code,
      wind: cur.wind_speed_10m
    };
  }

  function formatClock(tz) {
    try {
      var opts = { dateStyle: 'medium', timeStyle: 'short' };
      if (tz) return new Intl.DateTimeFormat(undefined, Object.assign({ timeZone: tz }, opts)).format(new Date());
    } catch (e) {}
    return new Intl.DateTimeFormat(undefined, { dateStyle: 'medium', timeStyle: 'short' }).format(new Date());
  }

  function applyOsm(lat, lon) {
    var img = $('nav-osm-img');
    var ph = $('nav-osm-placeholder');
    var wrap = $('nav-map-real');
    if (!img || !wrap) return;
    if (lat == null || lon == null || !isFinite(lat) || !isFinite(lon)) {
      img.removeAttribute('src');
      img.alt = '';
      if (ph) ph.hidden = false;
      return;
    }
    if (ph) ph.hidden = true;
    var z = 13;
    var wpx = Math.min(1024, Math.max(400, Math.floor(wrap.clientWidth || 640)));
    var url =
      'https://staticmap.openstreetmap.de/staticmap.php?center=' +
      lat +
      ',' +
      lon +
      '&zoom=' +
      z +
      '&size=' +
      wpx +
      'x200&maptype=mapnik&markers=' +
      lat +
      ',' +
      lon +
      ',lightblue1';
    img.alt = 'Map near ' + lat.toFixed(4) + ', ' + lon.toFixed(4);
    img.src = url;
    img.onerror = function () {
      if (ph) ph.hidden = false;
    };
  }

  function applyGeoToUi(geo, weather, rttMs) {
    if (!geo) return;
    setText('nav-wan-ip', geo.ip || '—');
    var loc =
      [geo.city, geo.region, geo.country].filter(Boolean).join(', ') || '—';
    setText('nav-wan-loc', loc);
    if (geo.org) setText('nav-wan-org', geo.org);
    else setText('nav-wan-org', '—');

    if (weather && weather.temp != null) {
      var parts = [
        Math.round(Number(weather.temp)) + '°C',
        wmoCodeDesc(weather.code),
        weather.wind != null ? 'wind ' + Math.round(weather.wind) + ' km/h' : ''
      ].filter(Boolean);
      setText('nav-weather', parts.join(' · '));
    } else {
      setText('nav-weather', '—');
    }

    setText('nav-inet-rtt', rttMs != null ? '~' + rttMs + ' ms (IP lookup)' : '—');
    setText('nav-local-clock', formatClock(geo.tz));

    var lat = deviceOverride ? deviceOverride.lat : geo.lat;
    var lon = deviceOverride ? deviceOverride.lon : geo.lon;
    if (lat != null && lon != null) {
      lastLat = lat;
      lastLon = lon;
      applyOsm(lat, lon);
      setHidden('nav-map-real', false);
    }
  }

  function coordsForWeather(geo) {
    if (deviceOverride) return { lat: deviceOverride.lat, lon: deviceOverride.lon };
    if (geo && geo.lat != null && geo.lon != null) return { lat: geo.lat, lon: geo.lon };
    return null;
  }

  /**
   * @param {object} wifi - from /wifi/status
   * @param {object} opt - { force: boolean }
   */
  function refresh(wifi, opt) {
    if (!$('nav-wan-ip')) return;

    var now = Date.now();
    var force = opt && opt.force;
    var browserOnline = typeof navigator !== 'undefined' && navigator.onLine;
    updateRouterRows(wifi || {});

    if (!force && now - lastFullFetch < MIN_REFRESH_MS) {
      if (!browserOnline) updatePills(wifi, false, false, null);
      else if (lastGeo) updatePills(wifi, true, true, lastGeo.rttMs);
      else updatePills(wifi, true, false, null);
      return;
    }

    if (!browserOnline) {
      lastFullFetch = now;
      updatePills(wifi, false, false, null);
      setText('nav-wan-ip', '—');
      setText('nav-wan-loc', 'Connect this device to the internet for live data.');
      setText('nav-weather', '—');
      setText('nav-inet-rtt', '—');
      setText('nav-local-clock', formatClock(null));
      setText('nav-wan-org', '—');
      setHidden('nav-map-real', true);
      return;
    }

    var geoStale = force || !lastGeo || now - lastGeo.t > GEO_CACHE_MS;
    var c = coordsForWeather(lastGeo);

    if (!geoStale && lastGeo) {
      updatePills(wifi, true, true, lastGeo.rttMs);
      var wxStale =
        force ||
        !lastWeather ||
        now - lastWeather.t > WEATHER_CACHE_MS ||
        (c && lastWeather && (lastWeather.lat !== c.lat || lastWeather.lon !== c.lon));
      if (wxStale && c) {
        fetchWeather(c.lat, c.lon)
          .then(function (wx) {
            lastWeather = { t: Date.now(), lat: c.lat, lon: c.lon, wx: wx };
            applyGeoToUi(lastGeo, wx, lastGeo.rttMs);
          })
          .catch(function () {
            applyGeoToUi(lastGeo, null, lastGeo.rttMs);
          });
      } else {
        applyGeoToUi(lastGeo, lastWeather && lastWeather.wx, lastGeo.rttMs);
      }
      lastFullFetch = now;
      return;
    }

    lastFullFetch = now;

    fetchGeo()
      .then(function (geo) {
        lastGeo = Object.assign({ t: now, rttMs: geo.ms }, geo);
        var cc = coordsForWeather(lastGeo);
        updatePills(wifi, true, true, geo.ms);

        if (cc) {
          return fetchWeather(cc.lat, cc.lon).then(function (wx) {
            lastWeather = { t: now, lat: cc.lat, lon: cc.lon, wx: wx };
            applyGeoToUi(lastGeo, wx, geo.ms);
          });
        }
        applyGeoToUi(lastGeo, null, geo.ms);
      })
      .catch(function () {
        updatePills(wifi, true, false, null);
        setText('nav-wan-ip', '—');
        setText('nav-wan-loc', 'Could not reach geolocation service (firewall / offline).');
        setText('nav-weather', '—');
        setText('nav-inet-rtt', '—');
        setText('nav-wan-org', '—');
        setText('nav-local-clock', formatClock(null));
        setHidden('nav-map-real', true);
      });
  }

  function requestDeviceLocation() {
    if (!navigator.geolocation) {
      if (typeof showToast === 'function') showToast('\u26A0', 'Geolocation not supported');
      return;
    }
    navigator.geolocation.getCurrentPosition(
      function (pos) {
        deviceOverride = { lat: pos.coords.latitude, lon: pos.coords.longitude };
        lastGeo = null;
        lastWeather = null;
        refresh({}, { force: true });
        if (typeof showToast === 'function') showToast('\uD83D\uDCCD', 'Using this device position for map & weather');
      },
      function () {
        if (typeof showToast === 'function') showToast('\u26A0', 'Location permission denied');
      },
      { enableHighAccuracy: false, timeout: 12000, maximumAge: 60000 }
    );
  }

  function bindGeoButton() {
    var btn = $('nav-geo-btn');
    if (btn && !btn._navGeoBound) {
      btn._navGeoBound = true;
      btn.addEventListener('click', requestDeviceLocation);
    }
  }

  global.NavWorldContext = {
    refresh: refresh,
    requestDeviceLocation: requestDeviceLocation,
    bindGeoButton: bindGeoButton
  };
})(typeof window !== 'undefined' ? window : this);


/**
 * Navigation map — canvas pan/zoom, waypoints, mock obstacles, marching ants route.
 * Depends: PathPlanner, ProximityAlert (optional)
 */
(function () {
  'use strict';

  var canvas, ctx;
  var scale = 1,
    offsetX = 0,
    offsetY = 0;
  var dragging = false,
    lastX,
    lastY;
  var waypoints = [];
  var obstacles = []; // {x,y,r,type} type 'lidar'|'sonic'
  var robot = { x: 50, y: 50, heading: 0 };
  /** Local tangent plane: grid (50,50) = originLat/originLon; 1 unit ≈ metersPerGridUnit */
  var _originLat = 40.7128;
  var _originLon = -74.006;
  var _metersPerGridUnit = 0.5;
  var _originFromGps = false;
  var breadcrumbs = []; // { lng, lat } for trail on MapLibre
  var _lastBreadMs = 0;
  var _lastBreadPos = null;
  var pathLine = [];
  var antsOffset = 0;
  var animId = null;
  var gridW = 100,
    gridH = 100;

  function worldToScreen(wx, wy) {
    return { x: wx * scale + offsetX, y: wy * scale + offsetY };
  }

  function screenToWorld(sx, sy) {
    return { x: (sx - offsetX) / scale, y: (sy - offsetY) / scale };
  }

  function gridToLngLat(x, y) {
    var dx = (x - 50) * _metersPerGridUnit;
    var dy = (50 - y) * _metersPerGridUnit;
    var lat = _originLat + dy / 111320;
    var lon = _originLon + dx / (111320 * Math.cos((_originLat * Math.PI) / 180));
    return { lng: lon, lat: lat };
  }

  function lngLatToGrid(lng, lat) {
    var dy = (lat - _originLat) * 111320;
    var dx = (lng - _originLon) * 111320 * Math.cos((_originLat * Math.PI) / 180);
    return {
      x: 50 + dx / _metersPerGridUnit,
      y: 50 - dy / _metersPerGridUnit
    };
  }

  function clampGrid(pt) {
    return {
      x: Math.max(0, Math.min(gridW - 1e-6, pt.x)),
      y: Math.max(0, Math.min(gridH - 1e-6, pt.y))
    };
  }

  function addWaypointFromLngLat(lng, lat) {
    var g = clampGrid(lngLatToGrid(lng, lat));
    waypoints.push(g);
    rebuildPath();
    updateWaypointPanel();
    notifyMapRefresh();
  }

  function recordBreadcrumb(lng, lat) {
    var now = Date.now();
    if (now - _lastBreadMs < 1100) return;
    if (_lastBreadPos) {
      var dLat = lat - _lastBreadPos.lat;
      var dLon = lng - _lastBreadPos.lng;
      if (dLat * dLat + dLon * dLon < 2e-10) return;
    }
    _lastBreadMs = now;
    _lastBreadPos = { lng: lng, lat: lat };
    breadcrumbs.push({ lng: lng, lat: lat });
    if (breadcrumbs.length > 220) breadcrumbs.shift();
  }

  function getMapAnchors() {
    try {
      var h = localStorage.getItem('walle_nav_home');
      var d = localStorage.getItem('walle_nav_dock');
      return {
        home: h ? JSON.parse(h) : null,
        dock: d ? JSON.parse(d) : null
      };
    } catch (e) {
      return { home: null, dock: null };
    }
  }

  function setHomeAnchor(lat, lng) {
    try {
      localStorage.setItem('walle_nav_home', JSON.stringify({ lat: lat, lng: lng }));
    } catch (e) {}
    notifyMapRefresh();
  }

  function setDockAnchor(lat, lng) {
    try {
      localStorage.setItem('walle_nav_dock', JSON.stringify({ lat: lat, lng: lng }));
    } catch (e) {}
    notifyMapRefresh();
  }

  function getMapSnapshot() {
    var anchors = getMapAnchors();
    return {
      origin: { lat: _originLat, lon: _originLon },
      robot: { x: robot.x, y: robot.y, heading: robot.heading },
      robotLngLat: gridToLngLat(robot.x, robot.y),
      waypoints: waypoints.map(function (w) {
        return gridToLngLat(w.x, w.y);
      }),
      pathLine: pathLine.map(function (p) {
        return gridToLngLat(p.x, p.y);
      }),
      obstacles: obstacles,
      home: anchors.home,
      dock: anchors.dock,
      breadcrumbs: breadcrumbs.slice(),
      gridSize: { w: gridW, h: gridH }
    };
  }

  function notifyMapRefresh() {
    if (window.LrosMapNav && typeof LrosMapNav.refreshFromPlanner === 'function') {
      LrosMapNav.refreshFromPlanner();
    }
  }

  function init() {
    canvas = document.getElementById('nav-map-canvas');
    if (!canvas) return;
    ctx = canvas.getContext('2d');
    resize();
    window.addEventListener('resize', resize);

    // Mock obstacles
    obstacles = [
      { x: 62, y: 48, r: 4, type: 'lidar' },
      { x: 40, y: 65, r: 3, type: 'sonic' },
    ];

    canvas.addEventListener(
      'wheel',
      function (e) {
        e.preventDefault();
        var z = e.deltaY > 0 ? 0.9 : 1.1;
        scale = Math.max(0.2, Math.min(4, scale * z));
      },
      { passive: false }
    );

    var dragDist = 0;
    canvas.addEventListener('mousedown', function (e) {
      dragging = true;
      dragDist = 0;
      lastX = e.clientX;
      lastY = e.clientY;
    });
    window.addEventListener('mousemove', function (e) {
      if (!dragging) return;
      dragDist += Math.abs(e.clientX - lastX) + Math.abs(e.clientY - lastY);
      offsetX += e.clientX - lastX;
      offsetY += e.clientY - lastY;
      lastX = e.clientX;
      lastY = e.clientY;
    });
    window.addEventListener('mouseup', function () {
      dragging = false;
    });

    canvas.addEventListener('click', function (e) {
      if (window.LrosMapNav && LrosMapNav.isMapActive && LrosMapNav.isMapActive()) return;
      if (dragDist > 5) return;
      var rect = canvas.getBoundingClientRect();
      var w = screenToWorld(e.clientX - rect.left, e.clientY - rect.top);
      if (w.x >= 0 && w.x < gridW && w.y >= 0 && w.y < gridH) {
        waypoints.push({ x: w.x, y: w.y });
        rebuildPath();
        updateWaypointPanel();
      }
    });

    if (animId) cancelAnimationFrame(animId);
    loop();
    if (window.LrosMapNav && typeof LrosMapNav.init === 'function') {
      try {
        LrosMapNav.init();
      } catch (e) {}
    }
  }

  function resize() {
    if (!canvas) return;
    var wrap = canvas.parentElement;
    if (wrap && wrap.clientWidth) {
      var w = wrap.clientWidth - 8;
      canvas.width = Math.min(1200, Math.max(280, w));
      canvas.height = Math.min(560, Math.floor(canvas.width * 0.55));
    }
  }

  function notifyRouteChange() {
    if (window.NavMissionPanel && typeof NavMissionPanel.onRouteChanged === 'function') {
      NavMissionPanel.onRouteChanged();
    }
  }

  function rebuildPath() {
    pathLine = [];
    if (waypoints.length < 2) {
      if (waypoints.length === 1) pathLine = [robot, waypoints[0]];
      notifyRouteChange();
      notifyMapRefresh();
      return;
    }
    var obsSet = new Set();
    obstacles.forEach(function (o) {
      for (var dx = -o.r; dx <= o.r; dx++)
        for (var dy = -o.r; dy <= o.r; dy++)
          if (dx * dx + dy * dy <= o.r * o.r + 1)
            obsSet.add(PathPlanner.key(Math.floor(o.x + dx), Math.floor(o.y + dy)));
    });

    var full = [robot];
    for (var i = 0; i < waypoints.length; i++) full.push(waypoints[i]);
    pathLine = [];
    for (var j = 0; j < full.length - 1; j++) {
      var a = { x: Math.floor(full[j].x), y: Math.floor(full[j].y) };
      var b = { x: Math.floor(full[j + 1].x), y: Math.floor(full[j + 1].y) };
      var seg = PathPlanner.astar(a, b, obsSet, gridW, gridH);
      if (seg.length) {
        if (pathLine.length) seg.shift();
        pathLine = pathLine.concat(seg);
      } else pathLine.push(full[j], full[j + 1]);
    }
    if (pathLine.length > 2) pathLine = PathPlanner.catmullRom(pathLine, 4);
    notifyRouteChange();
    notifyMapRefresh();
  }

  function getRouteInfo() {
    var len = PathPlanner.pathLength(pathLine);
    var eta = pathLine.length ? Math.round(len * 2) : 0;
    return {
      waypointCount: waypoints.length,
      pathLength: len,
      etaSeconds: eta,
      hasPath: pathLine.length > 1,
      segmentHint: Math.max(0, waypoints.length)
    };
  }

  function draw() {
    if (!ctx || !canvas) return;
    var w = canvas.width,
      h = canvas.height;
    ctx.fillStyle = '#0a0e14';
    ctx.fillRect(0, 0, w, h);
    ctx.save();
    ctx.translate(offsetX, offsetY);
    ctx.scale(scale, scale);

    // Grid
    ctx.strokeStyle = '#1e2836';
    ctx.lineWidth = 1 / scale;
    for (var gx = 0; gx <= gridW; gx += 10) {
      ctx.beginPath();
      ctx.moveTo(gx, 0);
      ctx.lineTo(gx, gridH);
      ctx.stroke();
    }
    for (var gy = 0; gy <= gridH; gy += 10) {
      ctx.beginPath();
      ctx.moveTo(0, gy);
      ctx.lineTo(gridW, gy);
      ctx.stroke();
    }

    // Historical path (amber, faint)
    ctx.strokeStyle = 'rgba(245, 166, 35, 0.25)';
    ctx.lineWidth = 2 / scale;
    ctx.setLineDash([]);
    ctx.beginPath();
    ctx.moveTo(30, 80);
    ctx.lineTo(45, 70);
    ctx.lineTo(55, 85);
    ctx.stroke();

    // Obstacles + halos
    obstacles.forEach(function (o) {
      var fill = o.type === 'lidar' ? 'rgba(230,57,70,0.35)' : 'rgba(245,166,35,0.35)';
      ctx.fillStyle = fill;
      ctx.beginPath();
      ctx.arc(o.x, o.y, o.r + 2, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = o.type === 'lidar' ? '#e63946' : '#f5a623';
      ctx.beginPath();
      ctx.arc(o.x, o.y, o.r, 0, Math.PI * 2);
      ctx.fill();
    });

    // Route marching ants
    if (pathLine.length > 1) {
      ctx.strokeStyle = '#3ddc84';
      ctx.lineWidth = 2.5 / scale;
      ctx.setLineDash([6, 6]);
      ctx.lineDashOffset = -antsOffset;
      ctx.beginPath();
      ctx.moveTo(pathLine[0].x, pathLine[0].y);
      for (var i = 1; i < pathLine.length; i++) ctx.lineTo(pathLine[i].x, pathLine[i].y);
      ctx.stroke();
      ctx.setLineDash([]);
    }

    // Waypoints
    waypoints.forEach(function (wp, idx) {
      ctx.fillStyle = '#5eb3f6';
      ctx.beginPath();
      ctx.arc(wp.x, wp.y, 2.5, 0, Math.PI * 2);
      ctx.fill();
      ctx.fillStyle = '#e4e8f0';
      ctx.font = 8 / scale + 'px sans-serif';
      ctx.fillText(String(idx + 1), wp.x + 3, wp.y - 3);
    });

    // Robot + heading + pulse
    var t = Date.now() / 1000;
    var pulse = 4 + Math.sin(t * 4) * 1.5;
    ctx.strokeStyle = 'rgba(61,220,132,0.4)';
    ctx.lineWidth = 1 / scale;
    ctx.beginPath();
    ctx.arc(robot.x, robot.y, pulse + 6, 0, Math.PI * 2);
    ctx.stroke();

    // Path prediction (intent ray)
    var hr = (robot.heading * Math.PI) / 180;
    ctx.strokeStyle = 'rgba(94, 179, 246, 0.5)';
    ctx.lineWidth = 1.8 / scale;
    ctx.setLineDash([4, 5]);
    ctx.beginPath();
    ctx.moveTo(robot.x, robot.y);
    ctx.lineTo(robot.x + Math.cos(hr) * 24, robot.y + Math.sin(hr) * 24);
    ctx.stroke();
    ctx.setLineDash([]);

    ctx.fillStyle = '#f5a623';
    ctx.save();
    ctx.translate(robot.x, robot.y);
    ctx.rotate((robot.heading * Math.PI) / 180);
    ctx.beginPath();
    ctx.moveTo(4, 0);
    ctx.lineTo(-3, 2.5);
    ctx.lineTo(-3, -2.5);
    ctx.closePath();
    ctx.fill();
    ctx.restore();

    ctx.restore();

    // ETA badge (screen space)
    var len = PathPlanner.pathLength(pathLine);
    var el = document.getElementById('nav-eta');
    if (el) el.textContent = pathLine.length ? '~' + Math.round(len * 2) + ' s est.' : '—';
    var elD = document.getElementById('nav-dist');
    if (elD) elD.textContent = pathLine.length > 1 ? '~' + len.toFixed(1) + ' planner units' : '—';
    var elR = document.getElementById('nav-mission-route-len');
    if (elR) elR.textContent = pathLine.length > 1 ? '~' + len.toFixed(1) + ' u' : '—';
  }

  var _lastProxMs = 0;
  function loop() {
    antsOffset += 0.4;
    var navVisible = document.getElementById('page-navigation') && document.getElementById('page-navigation').classList.contains('active');
    if (navVisible) draw();
    // Proximity mock — throttle beeps (~2 Hz max)
    var now = Date.now();
    if (navVisible && window.ProximityAlert && obstacles.length && now - _lastProxMs > 450) {
      _lastProxMs = now;
      var minD = 999;
      obstacles.forEach(function (o) {
        var dx = o.x - robot.x,
          dy = o.y - robot.y;
        minD = Math.min(minD, Math.sqrt(dx * dx + dy * dy) - o.r);
      });
      var norm = Math.min(1, minD / 30);
      ProximityAlert.alertFromProximity(norm);
    }
    animId = requestAnimationFrame(loop);
  }

  function updateWaypointPanel() {
    var ul = document.getElementById('nav-waypoint-list');
    if (!ul) return;
    ul.innerHTML = waypoints
      .map(function (w, i) {
        return (
          '<li class="waypoint-item"><span>' +
          (i + 1) +
          '</span> (' +
          w.x.toFixed(0) +
          ',' +
          w.y.toFixed(0) +
          ') <button type="button" class="btn btn-small btn-ghost" data-i="' +
          i +
          '">×</button></li>'
        );
      })
      .join('');
    ul.querySelectorAll('button[data-i]').forEach(function (btn) {
      btn.onclick = function () {
        var i = parseInt(btn.getAttribute('data-i'), 10);
        waypoints.splice(i, 1);
        rebuildPath();
        updateWaypointPanel();
        notifyMapRefresh();
      };
    });
  }

  function clearWaypoints() {
    waypoints = [];
    rebuildPath();
    updateWaypointPanel();
    notifyMapRefresh();
  }

  function getMissionPayload() {
    return {
      frame: 'grid',
      scale_m_per_unit: 0.5,
      robot: { x: robot.x, y: robot.y },
      waypoints: waypoints.map(function (w) {
        return { x: w.x, y: w.y };
      })
    };
  }

  function sendRouteToRobot() {
    if (!waypoints.length) {
      if (typeof showToast === 'function') showToast('\u26A0', 'Add at least one waypoint on the map');
      return;
    }
    var payload = getMissionPayload();
    var hdr = Object.assign({ 'Content-Type': 'application/json' },
      typeof apiAuthHeaders === 'function' ? apiAuthHeaders() : {});
    fetch(typeof api === 'function' ? api('/api/navigation/route') : '/api/navigation/route', {
      method: 'POST',
      headers: hdr,
      body: JSON.stringify(payload)
    })
      .then(function (r) {
        return r.json();
      })
      .then(function (j) {
        if (j && j.ok) {
          if (typeof showToast === 'function')
            showToast('\u2693', 'Route uploaded — ' + (j.count || 0) + ' waypoint(s). Autonomy + waypoint mode on.');
          if (typeof pushActivity === 'function')
            pushActivity('Navigation route uploaded (' + (j.count || 0) + ' WP)', '\u2693');
        } else {
          var err = j && j.error ? j.error : 'unknown';
          if (typeof showToast === 'function') showToast('\u26A0', 'Navigation: ' + err);
        }
      })
      .catch(function () {
        if (typeof showToast === 'function') showToast('\u26A0', 'Failed to reach /api/navigation/route');
      });
  }

  /** Live heading + sonar hint from HTTP poll (stateCache) */
  function syncFromState(cache) {
    if (cache && cache.auto && cache.auto.gpsValid && cache.auto.lat != null && cache.auto.lon != null) {
      var lat = Number(cache.auto.lat);
      var lon = Number(cache.auto.lon);
      if (!_originFromGps) {
        _originLat = lat;
        _originLon = lon;
        _originFromGps = true;
      }
      var g = lngLatToGrid(lon, lat);
      robot.x = Math.max(0, Math.min(gridW - 1e-6, g.x));
      robot.y = Math.max(0, Math.min(gridH - 1e-6, g.y));
      recordBreadcrumb(lon, lat);
      notifyMapRefresh();
    }
    if (!cache || !cache.imu) {
      notifyMapRefresh();
      return;
    }
    if (cache.imu.heading != null) robot.heading = Number(cache.imu.heading);
    if (cache.auto && cache.auto.sonar != null) {
      var d = Math.min(400, Math.max(5, Number(cache.auto.sonar)));
      var rad = (robot.heading * Math.PI) / 180;
      var threat = Math.max(3, 18 - d / 15);
      var fx = robot.x + Math.cos(rad) * (d / 25);
      var fy = robot.y + Math.sin(rad) * (d / 25);
      var found = -1;
      for (var i = 0; i < obstacles.length; i++) {
        if (obstacles[i].type === 'sonic') {
          found = i;
          break;
        }
      }
      var o = { x: fx, y: fy, r: threat, type: 'sonic' };
      if (found >= 0) obstacles[found] = o;
      else if (obstacles.length < 12) obstacles.push(o);
    }
    notifyMapRefresh();
  }

  window.LrosNavigation = {
    init: init,
    clearWaypoints: clearWaypoints,
    sendRouteToRobot: sendRouteToRobot,
    syncFromState: syncFromState,
    getRouteInfo: getRouteInfo,
    getMissionPayload: getMissionPayload,
    gridToLngLat: gridToLngLat,
    lngLatToGrid: lngLatToGrid,
    addWaypointFromLngLat: addWaypointFromLngLat,
    getMapSnapshot: getMapSnapshot,
    setHomeAnchor: setHomeAnchor,
    setDockAnchor: setDockAnchor,
    getMapAnchors: getMapAnchors,
    notifyMapRefresh: notifyMapRefresh
  };
})();

/** Called from index.html navigation toolbar — keeps onclick free of `&&` (linter-safe). */
function navClearWaypoints() {
  if (window.LrosNavigation) window.LrosNavigation.clearWaypoints();
}
function navSendRouteToRobot() {
  if (window.LrosNavigation) window.LrosNavigation.sendRouteToRobot();
}



/**
 * LROS Navigation — MapLibre GL map (local / lawful tiles; no Google).
 * Optional CDN load; graceful fallback to canvas planner if MapLibre or style fails.
 *
 * Config (localStorage):
 *   lros_map_style_url   — full URL to style.json (vector or raster+glyphs)
 *   lros_map_raster_url  — optional {z}/{x}/{y} template for a raster overlay (OSM-compatible)
 *
 * Globals (optional, set before load):
 *   window.LROS_MAPLIBRE_JS   — override script URL (default: jsdelivr MapLibre 4.7.1)
 *   window.LROS_MAP_STYLE_URL — initial style.json URL (same as localStorage)
 */
(function (global) {
  'use strict';

  var map = null;
  var mapReady = false;
  var useFallback = false;
  var _mapInitStarted = false;
  var _controlsWired = false;
  var followRobot = false;
  var headingUp = false;
  var clickMode = 'waypoint'; // waypoint | home | dock
  var _refreshTimer = null;

  function log() {
    if (typeof console !== 'undefined' && console.debug) {
      console.debug.apply(console, ['[LROS MapLibre]'].concat([].slice.call(arguments)));
    }
  }

  function getStyleUrl() {
    try {
      if (global.LROS_MAP_STYLE_URL) return String(global.LROS_MAP_STYLE_URL);
      var u = localStorage.getItem('lros_map_style_url');
      if (u) return u;
    } catch (e) {}
    return '';
  }

  function getRasterTemplate() {
    try {
      return localStorage.getItem('lros_map_raster_url') || '';
    } catch (e) {
      return '';
    }
  }

  /** Offline-safe minimal style; optional OSM-style raster overlay (user-supplied URL). */
  function buildEmbeddedStyle() {
    var rasterTpl = getRasterTemplate();
    var sources = {};
    var layers = [
      {
        id: 'lros-bg',
        type: 'background',
        paint: { 'background-color': '#0b0f14' }
      }
    ];
    if (rasterTpl) {
      sources['lros-raster'] = {
        type: 'raster',
        tiles: [rasterTpl],
        tileSize: 256,
        attribution: 'Local / OSM-compatible tiles (configure lros_map_raster_url)'
      };
      layers.push({
        id: 'lros-raster-layer',
        type: 'raster',
        source: 'lros-raster',
        paint: { 'raster-opacity': 0.85 }
      });
    }
    return {
      version: 8,
      name: 'LROS Local',
      metadata: { 'lros:embedded': true },
      sources: sources,
      layers: layers
    };
  }

  function geoCircleFeature(lng, lat, radiusM) {
    var sides = 48;
    var coords = [];
    var R = 6371000;
    var lat1 = (lat * Math.PI) / 180;
    var lng1 = (lng * Math.PI) / 180;
    var d = radiusM / R;
    for (var i = 0; i <= sides; i++) {
      var brng = (i / sides) * 2 * Math.PI;
      var lat2 = Math.asin(Math.sin(lat1) * Math.cos(d) + Math.cos(lat1) * Math.sin(d) * Math.cos(brng));
      var lng2 =
        lng1 +
        Math.atan2(
          Math.sin(brng) * Math.sin(d) * Math.cos(lat1),
          Math.cos(d) - Math.sin(lat1) * Math.sin(lat2)
        );
      coords.push([(lng2 * 180) / Math.PI, (lat2 * 180) / Math.PI]);
    }
    return {
      type: 'Feature',
      properties: {},
      geometry: { type: 'Polygon', coordinates: [coords] }
    };
  }

  function readGeofenceForMap() {
    if (typeof readGeofenceConfig === 'function') {
      var c = readGeofenceConfig();
      if (c && c.enabled && c.centerLat != null && c.centerLon != null) {
        return { lat: c.centerLat, lon: c.centerLon, radiusM: c.radiusM || 50 };
      }
    }
    try {
      var j = localStorage.getItem('walle_geofence_v1');
      if (!j) return null;
      var o = JSON.parse(j);
      if (!o || !o.enabled || o.centerLat == null || o.centerLon == null) return null;
      return { lat: Number(o.centerLat), lon: Number(o.centerLon), radiusM: o.radiusM != null ? Number(o.radiusM) : 50 };
    } catch (e) {
      return null;
    }
  }

  function snapshotToGeoJSON() {
    if (!global.LrosNavigation || typeof LrosNavigation.getMapSnapshot !== 'function') {
      return null;
    }
    var s = LrosNavigation.getMapSnapshot();
    var features = [];

    if (s.breadcrumbs && s.breadcrumbs.length > 1) {
      features.push({
        type: 'Feature',
        properties: { k: 'crumb' },
        geometry: {
          type: 'LineString',
          coordinates: s.breadcrumbs.map(function (p) {
            return [p.lng, p.lat];
          })
        }
      });
    }

    if (s.pathLine && s.pathLine.length > 1) {
      features.push({
        type: 'Feature',
        properties: { k: 'route' },
        geometry: {
          type: 'LineString',
          coordinates: s.pathLine.map(function (p) {
            return [p.lng, p.lat];
          })
        }
      });
    }

    (s.waypoints || []).forEach(function (p, i) {
      features.push({
        type: 'Feature',
        properties: { k: 'wp', i: i + 1 },
        geometry: { type: 'Point', coordinates: [p.lng, p.lat] }
      });
    });

    var rr = s.robotLngLat;
    features.push({
      type: 'Feature',
      properties: { k: 'robot' },
      geometry: { type: 'Point', coordinates: [rr.lng, rr.lat] }
    });

    var headDeg = s.robot && s.robot.heading != null ? Number(s.robot.heading) : 0;
    var br = (headDeg * Math.PI) / 180;
    var distM = 12;
    var dN = (distM * Math.cos(br)) / 111320;
    var dE = (distM * Math.sin(br)) / (111320 * Math.cos((rr.lat * Math.PI) / 180));
    features.push({
      type: 'Feature',
      properties: { k: 'head' },
      geometry: {
        type: 'LineString',
        coordinates: [
          [rr.lng, rr.lat],
          [rr.lng + dE, rr.lat + dN]
        ]
      }
    });

    if (s.home && s.home.lat != null && s.home.lng != null) {
      features.push({
        type: 'Feature',
        properties: { k: 'home' },
        geometry: { type: 'Point', coordinates: [s.home.lng, s.home.lat] }
      });
    }
    if (s.dock && s.dock.lat != null && s.dock.lng != null) {
      features.push({
        type: 'Feature',
        properties: { k: 'dock' },
        geometry: { type: 'Point', coordinates: [s.dock.lng, s.dock.lat] }
      });
    }

    var gf = readGeofenceForMap();
    if (gf) {
      var gfeat = geoCircleFeature(gf.lon, gf.lat, gf.radiusM);
      gfeat.properties = { k: 'geofence' };
      features.push(gfeat);
    }

    return { type: 'FeatureCollection', features: features };
  }

  function setSourceData() {
    if (!map || !mapReady) return;
    var gj = snapshotToGeoJSON();
    if (!gj) return;
    var src = map.getSource('lros-dynamic');
    if (src && src.setData) src.setData(gj);
  }

  function applyBearing() {
    if (!map || !mapReady) return;
    var s = global.LrosNavigation && LrosNavigation.getMapSnapshot ? LrosNavigation.getMapSnapshot() : null;
    var h = s && s.robot ? Number(s.robot.heading) || 0 : 0;
    map.setBearing(headingUp ? -h : 0);
  }

  function maybeFollow() {
    if (!map || !mapReady || !followRobot) return;
    var s = global.LrosNavigation && LrosNavigation.getMapSnapshot ? LrosNavigation.getMapSnapshot() : null;
    if (!s || !s.robotLngLat) return;
    map.easeTo({
      center: [s.robotLngLat.lng, s.robotLngLat.lat],
      duration: 380,
      essential: true
    });
  }

  function refreshFromPlanner() {
    if (useFallback || !mapReady) return;
    if (_refreshTimer) return;
    _refreshTimer = setTimeout(function () {
      _refreshTimer = null;
      setSourceData();
      applyBearing();
      maybeFollow();
    }, 40);
  }

  function showFallbackUI(msg) {
    useFallback = true;
    mapReady = false;
    var ph = document.getElementById('nav-map-fallback');
    var mc = document.getElementById('nav-map-container');
    var cv = document.getElementById('nav-map-canvas');
    var body = document.querySelector('.nav-map-body');
    if (ph) {
      ph.hidden = false;
      ph.textContent = msg || 'MapLibre unavailable — using planner canvas below.';
    }
    if (mc) mc.style.display = 'none';
    if (body) body.style.minHeight = 'min(48vh, 480px)';
    if (cv) {
      cv.style.display = 'block';
      cv.style.position = 'relative';
    }
    log('fallback:', msg);
  }

  function wireControls() {
    if (_controlsWired) return;
    _controlsWired = true;
    var fb = document.getElementById('nav-map-follow');
    if (fb) {
      fb.addEventListener('change', function () {
        followRobot = !!fb.checked;
        log('followRobot', followRobot);
      });
    }
    var hu = document.getElementById('nav-map-heading-up');
    if (hu) {
      hu.addEventListener('change', function () {
        headingUp = !!hu.checked;
        applyBearing();
      });
    }
    var ctr = document.getElementById('nav-map-center');
    if (ctr) {
      ctr.addEventListener('click', function () {
        maybeFollow();
        var s = LrosNavigation.getMapSnapshot();
        if (map && s && s.robotLngLat) {
          map.flyTo({ center: [s.robotLngLat.lng, s.robotLngLat.lat], zoom: Math.max(map.getZoom(), 16), duration: 600 });
        }
      });
    }
    var cm = document.getElementById('nav-map-click-mode');
    if (cm) {
      cm.addEventListener('change', function () {
        clickMode = cm.value || 'waypoint';
        log('clickMode', clickMode);
      });
    }
    var lyr = document.getElementById('nav-map-layer-preset');
    if (lyr) {
      lyr.addEventListener('change', function () {
        var v = lyr.value;
        if (v === 'embed') {
          try {
            localStorage.removeItem('lros_map_style_url');
          } catch (e) {}
          location.reload();
        } else if (v === 'demo') {
          try {
            localStorage.setItem('lros_map_style_url', 'https://demotiles.maplibre.org/style.json');
          } catch (e) {}
          location.reload();
        }
      });
    }
  }

  function addLayers() {
    if (!map) return;
    map.addSource('lros-dynamic', {
      type: 'geojson',
      data: { type: 'FeatureCollection', features: [] }
    });

    map.addLayer({
      id: 'lros-crumb',
      type: 'line',
      source: 'lros-dynamic',
      filter: ['==', ['get', 'k'], 'crumb'],
      paint: { 'line-color': '#f5a623', 'line-width': 2, 'line-opacity': 0.35 }
    });
    map.addLayer({
      id: 'lros-route',
      type: 'line',
      source: 'lros-dynamic',
      filter: ['==', ['get', 'k'], 'route'],
      paint: { 'line-color': '#34d399', 'line-width': 4, 'line-opacity': 0.9 }
    });
    map.addLayer({
      id: 'lros-head',
      type: 'line',
      source: 'lros-dynamic',
      filter: ['==', ['get', 'k'], 'head'],
      paint: { 'line-color': '#5eb3f6', 'line-width': 2, 'line-opacity': 0.8 }
    });
    map.addLayer({
      id: 'lros-geofence-fill',
      type: 'fill',
      source: 'lros-dynamic',
      filter: ['==', ['get', 'k'], 'geofence'],
      paint: { 'fill-color': '#e63946', 'fill-opacity': 0.12 }
    });
    map.addLayer({
      id: 'lros-geofence-line',
      type: 'line',
      source: 'lros-dynamic',
      filter: ['==', ['get', 'k'], 'geofence'],
      paint: { 'line-color': '#e63946', 'line-width': 2, 'line-opacity': 0.5 }
    });
    map.addLayer({
      id: 'lros-wp',
      type: 'circle',
      source: 'lros-dynamic',
      filter: ['==', ['get', 'k'], 'wp'],
      paint: {
        'circle-radius': 7,
        'circle-color': '#5eb3f6',
        'circle-stroke-width': 2,
        'circle-stroke-color': '#0b0f14'
      }
    });
    map.addLayer({
      id: 'lros-home',
      type: 'circle',
      source: 'lros-dynamic',
      filter: ['==', ['get', 'k'], 'home'],
      paint: {
        'circle-radius': 8,
        'circle-color': '#34d399',
        'circle-stroke-width': 2,
        'circle-stroke-color': '#0b0f14'
      }
    });
    map.addLayer({
      id: 'lros-dock',
      type: 'circle',
      source: 'lros-dynamic',
      filter: ['==', ['get', 'k'], 'dock'],
      paint: {
        'circle-radius': 8,
        'circle-color': '#a78bfa',
        'circle-stroke-width': 2,
        'circle-stroke-color': '#0b0f14'
      }
    });
    map.addLayer({
      id: 'lros-robot',
      type: 'circle',
      source: 'lros-dynamic',
      filter: ['==', ['get', 'k'], 'robot'],
      paint: {
        'circle-radius': 10,
        'circle-color': '#f5a623',
        'circle-stroke-width': 3,
        'circle-stroke-color': '#fff'
      }
    });
  }

  function onMapClick(e) {
    var lng = e.lngLat.lng;
    var lat = e.lngLat.lat;
    if (clickMode === 'home') {
      if (typeof showToast === 'function') showToast('\u2302', 'Home marker set on map (local)');
      LrosNavigation.setHomeAnchor(lat, lng);
    } else if (clickMode === 'dock') {
      if (typeof showToast === 'function') showToast('\u26EF', 'Dock marker set on map (local)');
      LrosNavigation.setDockAnchor(lat, lng);
    } else {
      LrosNavigation.addWaypointFromLngLat(lng, lat);
    }
  }

  function initMapInstance() {
    var container = document.getElementById('nav-map-container');
    if (!container || !global.maplibregl) return;

    var styleUrl = getStyleUrl();
    var initOpts = {
      container: container,
      attributionControl: true,
      maxZoom: 20,
      minZoom: 2
    };

    function afterLoad() {
      mapReady = true;
      addLayers();
      var s = LrosNavigation.getMapSnapshot();
      map.jumpTo({
        center: [s.robotLngLat.lng, s.robotLngLat.lat],
        zoom: 17,
        pitch: 0
      });
      map.on('click', onMapClick);
      map.on('moveend', function () {
        log('moveend', map.getCenter().toArray());
      });
      refreshFromPlanner();
      var ph = document.getElementById('nav-map-fallback');
      var cv = document.getElementById('nav-map-canvas');
      var mc = document.getElementById('nav-map-container');
      if (ph) ph.hidden = true;
      if (cv) {
        cv.style.display = 'none';
        cv.style.position = 'absolute';
      }
      if (mc) mc.style.display = 'block';
      log('map ready');
    }

    if (styleUrl) {
      initOpts.style = styleUrl;
      try {
        map = new maplibregl.Map(initOpts);
        map.on('load', afterLoad);
        map.on('error', function (ev) {
          log('map error', ev);
          showFallbackUI('Map style failed to load — check lros_map_style_url or network.');
        });
      } catch (err) {
        log('map ctor', err);
        showFallbackUI(String(err));
      }
    } else {
      try {
        initOpts.style = buildEmbeddedStyle();
        map = new maplibregl.Map(initOpts);
        map.on('load', afterLoad);
      } catch (err2) {
        log('embedded style', err2);
        showFallbackUI(String(err2));
      }
    }

    global.addEventListener('resize', function () {
      if (map && mapReady) map.resize();
    });
  }

  function loadScript(url, cb) {
    var s = document.createElement('script');
    s.src = url;
    s.async = true;
    s.crossOrigin = 'anonymous';
    s.onload = function () {
      cb(null);
    };
    s.onerror = function () {
      cb(new Error('script load failed'));
    };
    document.head.appendChild(s);
  }

  function ensureCss(href) {
    if (document.querySelector('link[data-lros-maplibre]')) return;
    var l = document.createElement('link');
    l.rel = 'stylesheet';
    l.href = href;
    l.setAttribute('data-lros-maplibre', '1');
    document.head.appendChild(l);
  }

  function init() {
    if (_mapInitStarted) {
      onTabShow();
      return;
    }
    _mapInitStarted = true;
    wireControls();
    var jsUrl =
      global.LROS_MAPLIBRE_JS ||
      'https://cdn.jsdelivr.net/npm/maplibre-gl@4.7.1/dist/maplibre-gl.min.js';
    var cssUrl =
      global.LROS_MAPLIBRE_CSS ||
      'https://cdn.jsdelivr.net/npm/maplibre-gl@4.7.1/dist/maplibre-gl.css';

    if (global.maplibregl) {
      initMapInstance();
      return;
    }
    ensureCss(cssUrl);
    loadScript(jsUrl, function (err) {
      if (err || !global.maplibregl) {
        showFallbackUI('Could not load MapLibre GL (offline?). Planner canvas remains available.');
        return;
      }
      initMapInstance();
    });
  }

  function onTabShow() {
    if (map && mapReady) {
      map.resize();
      refreshFromPlanner();
    }
  }

  global.LrosMapNav = {
    init: init,
    refreshFromPlanner: refreshFromPlanner,
    isMapActive: function () {
      return mapReady && !useFallback;
    },
    onTabShow: onTabShow
  };
})(typeof window !== 'undefined' ? window : this);


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
    var lowBatt = s && s.battery && s.battery.voltage != null && s.battery.voltage < 6.5;
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


/**
 * LROS Sequence generator — load/save/run timelines on the Brain (Preferences).
 * API: GET /api/sequences/list, get, save (POST JSON), delete (POST), run, stop, status
 *
 * Step fields: optional "when": { "battery_pct_min", "battery_pct_max", "dock_fsm", "vision_event" }
 * — if present and not satisfied, the step is skipped (see sequence_engine.cpp).
 */
(function (global) {
  'use strict';

  var _pollTimer = null;

  function api(path) {
    if (typeof global.api === 'function') return global.api(path);
    var p = path.charAt(0) === '/' ? path : '/' + path;
    if (typeof document !== 'undefined') {
      var inp = document.getElementById('set-base-url');
      if (inp && String(inp.value || '').trim()) {
        return String(inp.value).trim().replace(/\/$/, '') + p;
      }
    }
    var b = (typeof global.getBaseUrl === 'function' && global.getBaseUrl()) || '';
    if (!b && typeof global.BASE !== 'undefined') b = global.BASE;
    return (b || '') + p;
  }

  function log() {
    if (typeof console !== 'undefined' && console.debug) {
      console.debug.apply(console, ['[LrosSequences]'].concat([].slice.call(arguments)));
    }
  }

  function fetchJson(url, opts) {
    return fetch(url, opts || {}).then(function (r) {
      return r.text().then(function (t) {
        try {
          return { ok: r.ok, status: r.status, json: JSON.parse(t), raw: t };
        } catch (e) {
          return { ok: r.ok, status: r.status, json: null, raw: t };
        }
      });
    });
  }

  function el(id) {
    return document.getElementById(id);
  }

  function newId() {
    return 'seq_' + Date.now().toString(36) + '_' + Math.random().toString(36).slice(2, 7);
  }

  function defaultSteps() {
    return [
      { at_ms: 0, kind: 'emotion', emotion: 'happy' },
      { at_ms: 500, kind: 'audio_play', track: 1 },
      { at_ms: 1500, kind: 'motor_stop' }
    ];
  }

  function stepExtraFields(step) {
    var o = {};
    Object.keys(step).forEach(function (k) {
      if (k !== 'at_ms' && k !== 'kind') o[k] = step[k];
    });
    return JSON.stringify(o, null, 0);
  }

  function renderStepsTable(steps) {
    var tb = el('seq-steps-tbody');
    if (!tb) return;
    tb.innerHTML = '';
    (steps || []).forEach(function (s, idx) {
      var tr = document.createElement('tr');
      tr.dataset.index = String(idx);
      var at = s.at_ms != null ? s.at_ms : 0;
      var kind = s.kind || 'noop';
      tr.innerHTML =
        '<td><input type="number" class="seq-at input-compact" min="0" step="100" value="' +
        at +
        '" /></td>' +
        '<td><select class="seq-kind">' +
        KIND_OPTIONS +
        '</select></td>' +
        '<td><textarea class="seq-params mono" rows="2" placeholder="Extra JSON"></textarea></td>' +
        '<td><button type="button" class="btn btn-small btn-ghost seq-row-del">×</button></td>';
      tb.appendChild(tr);
      var sel = tr.querySelector('.seq-kind');
      if (sel) sel.value = kind;
      var ta = tr.querySelector('.seq-params');
      if (ta) ta.value = stepExtraFields(s);
    });
    bindStepRows();
  }

  var KIND_OPTIONS = [
    'noop',
    'emotion',
    'audio_play',
    'audio_volume',
    'wait',
    'motor_stop',
    'drive_tank',
    'navigation_route',
    'laser_off',
    'laser_on',
    'laser_brightness',
    'laser_fire',
    'laser_mood'
  ]
    .map(function (k) {
      return '<option value="' + k + '">' + k + '</option>';
    })
    .join('');

  function bindStepRows() {
    var tb = el('seq-steps-tbody');
    if (!tb) return;
    tb.querySelectorAll('.seq-row-del').forEach(function (btn) {
      btn.onclick = function () {
        var tr = btn.closest('tr');
        if (tr) tr.remove();
      };
    });
  }

  function readStepsFromDOM() {
    var rows = document.querySelectorAll('#seq-steps-tbody tr');
    var steps = [];
    rows.forEach(function (tr) {
      var at = parseInt(tr.querySelector('.seq-at').value, 10);
      if (isNaN(at)) at = 0;
      var kind = tr.querySelector('.seq-kind').value;
      var extra = {};
      try {
        var raw = (tr.querySelector('.seq-params').value || '').trim();
        if (raw) extra = JSON.parse(raw);
      } catch (e) {
        log('bad JSON in step row', e);
        throw new Error('Invalid JSON in step params');
      }
      var o = Object.assign({ at_ms: at, kind: kind }, extra);
      steps.push(o);
    });
    steps.sort(function (a, b) {
      return (a.at_ms || 0) - (b.at_ms || 0);
    });
    return steps;
  }

  function setStatus(text) {
    var s = el('seq-run-status');
    if (s) s.textContent = text;
  }

  function refreshLibrary() {
    return fetchJson(api('/api/sequences/list')).then(function (res) {
      if (!res.json || !res.json.ok) {
        setStatus('List failed (offline or old firmware?)');
        log('list', res.raw);
        return;
      }
      var sel = el('seq-library-select');
      if (!sel) return;
      var cur = sel.value;
      sel.innerHTML = '<option value="">— Select —</option>';
      (res.json.sequences || []).forEach(function (s) {
        var opt = document.createElement('option');
        opt.value = s.id;
        opt.textContent = (s.name || s.id) + ' (' + (s.step_count || 0) + ' steps)';
        sel.appendChild(opt);
      });
      if (cur && [].some.call(sel.options, function (o) { return o.value === cur; })) {
        sel.value = cur;
      }
      setStatus('Loaded library (' + (res.json.sequences || []).length + ' sequences)');
    });
  }

  function loadSelected() {
    var sel = el('seq-library-select');
    if (!sel || !sel.value) return;
    return fetchJson(api('/api/sequences/get?id=' + encodeURIComponent(sel.value))).then(function (res) {
      if (!res.json || !res.json.ok || !res.json.sequence) {
        setStatus('Load failed');
        return;
      }
      var seq = res.json.sequence;
      el('seq-id').value = seq.id || '';
      el('seq-name').value = seq.name || '';
      renderStepsTable(seq.steps || []);
      setStatus('Loaded ' + seq.id);
    });
  }

  function saveToRobot() {
    var id = (el('seq-id').value || '').trim();
    var name = (el('seq-name').value || '').trim();
    if (!id) {
      id = newId();
      el('seq-id').value = id;
    }
    var steps;
    try {
      steps = readStepsFromDOM();
    } catch (e) {
      alert(e.message);
      return;
    }
    var body = JSON.stringify({ id: id, name: name || 'Untitled', steps: steps });
    log('save', body.length);
    var h = Object.assign({ 'Content-Type': 'application/json' },
      typeof apiAuthHeaders === 'function' ? apiAuthHeaders() : {});
    return fetch(api('/api/sequences/save'), {
      method: 'POST',
      headers: h,
      body: body
    })
      .then(function (r) {
        return r.json();
      })
      .then(function (j) {
        if (!j.ok) throw new Error(j.error || 'save failed');
        setStatus('Saved to robot');
        return refreshLibrary();
      })
      .catch(function (e) {
        setStatus('Save error: ' + e.message);
        log(e);
      });
  }

  function deleteSelected() {
    var sel = el('seq-library-select');
    if (!sel || !sel.value) return;
    var id = sel.value;
    if (!confirm('Delete sequence ' + id + '?')) return;
    var h2 = Object.assign({ 'Content-Type': 'application/json' },
      typeof apiAuthHeaders === 'function' ? apiAuthHeaders() : {});
    return fetch(api('/api/sequences/delete'), {
      method: 'POST',
      headers: h2,
      body: JSON.stringify({ id: id })
    })
      .then(function (r) {
        return r.json();
      })
      .then(function (j) {
        if (!j.ok) throw new Error(j.error || 'delete failed');
        setStatus('Deleted');
        if (el('seq-id').value === id) {
          el('seq-id').value = '';
          el('seq-name').value = '';
          renderStepsTable([]);
        }
        return refreshLibrary();
      })
      .catch(function (e) {
        setStatus('Delete error: ' + e.message);
      });
  }

  function runSequence() {
    var id = (el('seq-id').value || '').trim();
    if (!id) {
      setStatus('Set an id or load from library');
      return;
    }
    return fetchJson(api('/api/sequences/run?id=' + encodeURIComponent(id))).then(function (res) {
      if (!res.json || !res.json.ok) {
        var er = (res.json && res.json.error) ? res.json.error : (res.raw || 'error');
        setStatus('Run failed: ' + er);
        return;
      }
      setStatus('Running…');
    });
  }

  function stopSequence() {
    return fetchJson(api('/api/sequences/stop')).then(function (res) {
      setStatus('Stopped');
    });
  }

  function pollStatus() {
    if (typeof document === 'undefined' || !document.getElementById('page-sequence')) return;
    if (!document.getElementById('page-sequence').classList.contains('active')) return;
    fetchJson(api('/api/sequences/status')).then(function (res) {
      if (!res.json || !res.json.ok) return;
      var s = el('seq-run-status');
      if (s && res.json.running) {
        s.textContent = 'Running step ' + (res.json.step || 0) + ' · ' + (res.json.id || '');
      }
    });
  }

  function wire() {
    var add = el('seq-add-step');
    if (add && !add.dataset.bound) {
      add.dataset.bound = '1';
      add.addEventListener('click', function () {
        var tb = el('seq-steps-tbody');
        if (!tb) return;
        var tr = document.createElement('tr');
        tr.innerHTML =
          '<td><input type="number" class="seq-at input-compact" min="0" step="100" value="0" /></td>' +
          '<td><select class="seq-kind">' +
          KIND_OPTIONS +
          '</select></td>' +
          '<td><textarea class="seq-params mono" rows="2" placeholder="{ }"></textarea></td>' +
          '<td><button type="button" class="btn btn-small btn-ghost seq-row-del">×</button></td>';
        tb.appendChild(tr);
        bindStepRows();
      });
    }
    var b = el('seq-btn-new');
    if (b && !b.dataset.bound) {
      b.dataset.bound = '1';
      b.addEventListener('click', function () {
        el('seq-id').value = newId();
        el('seq-name').value = 'New sequence';
        renderStepsTable(defaultSteps());
      });
    }
    if (el('seq-btn-refresh') && !el('seq-btn-refresh').dataset.bound) {
      el('seq-btn-refresh').dataset.bound = '1';
      el('seq-btn-refresh').addEventListener('click', refreshLibrary);
    }
    if (el('seq-btn-load') && !el('seq-btn-load').dataset.bound) {
      el('seq-btn-load').dataset.bound = '1';
      el('seq-btn-load').addEventListener('click', loadSelected);
    }
    if (el('seq-btn-save') && !el('seq-btn-save').dataset.bound) {
      el('seq-btn-save').dataset.bound = '1';
      el('seq-btn-save').addEventListener('click', saveToRobot);
    }
    if (el('seq-btn-delete') && !el('seq-btn-delete').dataset.bound) {
      el('seq-btn-delete').dataset.bound = '1';
      el('seq-btn-delete').addEventListener('click', deleteSelected);
    }
    if (el('seq-btn-run') && !el('seq-btn-run').dataset.bound) {
      el('seq-btn-run').dataset.bound = '1';
      el('seq-btn-run').addEventListener('click', runSequence);
    }
    if (el('seq-btn-stop') && !el('seq-btn-stop').dataset.bound) {
      el('seq-btn-stop').dataset.bound = '1';
      el('seq-btn-stop').addEventListener('click', stopSequence);
    }
  }

  function onTabShow() {
    wire();
    if (!el('seq-id').value) {
      el('seq-id').value = newId();
      el('seq-name').value = 'My sequence';
      renderStepsTable(defaultSteps());
    }
    refreshLibrary();
    if (_pollTimer) clearInterval(_pollTimer);
    _pollTimer = setInterval(pollStatus, 2500);
  }

  function onTabHide() {
    if (_pollTimer) {
      clearInterval(_pollTimer);
      _pollTimer = null;
    }
  }

  global.LrosSequences = {
    onTabShow: onTabShow,
    onTabHide: onTabHide,
    refreshLibrary: refreshLibrary
  };
})(typeof window !== 'undefined' ? window : this);


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
  if (name === 'eve') pollLivingTelemetry();
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

// EVE 2S pack (~7.4 V nominal): match firmware EVE_BAT_MIN_V 6.0 / EVE_BAT_MAX_V 8.4.
function updateToastsFromState(s) {
  if (!s) return;
  if (s.battery && s.battery.voltage < 6.5) {
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
  setById('op-eve-val', '—');
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
      snu.innerHTML = 'Last sync <span class="mono">' + new Date().toLocaleTimeString() + '</span> · <span class="mono">/api/system/health</span> + EVE from <span class="mono">/api/living/telemetry</span>';
    }
  }).catch(function() {});
}

/** EVE pill + operator chip + EVE page — UART is not an ESP-NOW node; use living + /api/eve/status */
function applyLivingEveUI(liv, uart, panel, learning, monitor) {
  var eveOn = liv && liv.eve_uart === true;
  var nodes = document.querySelectorAll('.node-pill[data-pill="eve"]');
  var prev = _pillState.eve;
  _pillState.eve = eveOn;
  nodes.forEach(function (el) {
    el.classList.remove('ok', 'warn', 'off');
    if (eveOn) el.classList.add('ok');
    else el.classList.add('off');
    if (prev !== undefined && prev !== eveOn) {
      el.classList.add('edge');
      setTimeout(function () { el.classList.remove('edge'); }, 500);
    }
  });
  var opE = document.getElementById('op-eve-val');
  if (opE) {
    if (uart && uart.ok !== false) {
      if (uart.link_ok) {
        var ag = Number(uart.last_rx_age_ms) || 0;
        opE.textContent = ag < 20000 ? 'LIVE' : 'STALE';
      } else {
        opE.textContent = 'OFF';
      }
    } else if (liv && typeof liv.eve_uart === 'boolean') {
      opE.textContent = liv.eve_uart ? 'ON' : 'OFF';
    } else {
      opE.textContent = '—';
    }
  }
  try {
    window.__lrosLiving = liv;
  } catch (e) {}
  try {
    window.__lrosEveUart = uart;
  } catch (e) {}
  if (currentVisiblePage !== 'eve') return;
  if (uart && uart.ok !== false) {
    setById('eve-uart-link', uart.link_ok ? 'Connected' : 'No link');
    setById('eve-uart-age', uart.last_rx_age_ms != null ? String(uart.last_rx_age_ms) + ' ms' : '—');
    setById('eve-uart-type', uart.last_type || '—');
    setById('eve-uart-frames', (uart.frames_ok != null ? uart.frames_ok : '—') + ' / ' + (uart.crc_errors != null ? uart.crc_errors : '—'));
    var pl = uart.payload;
    if (pl == null) setById('eve-uart-payload', '—');
    else {
      var t = String(pl);
      if (t.length > 400) t = t.slice(0, 400) + '…';
      setById('eve-uart-payload', t);
    }
  } else {
    setById('eve-uart-link', '—');
    setById('eve-uart-age', '—');
    setById('eve-uart-type', '—');
    setById('eve-uart-frames', '—');
    setById('eve-uart-payload', '—');
  }
  if (liv) {
    setById('eve-voicebox', liv.voicebox_mode != null ? String(liv.voicebox_mode) : '—');
    setById('eve-vb-shared', liv.voicebox_shared != null ? (liv.voicebox_shared ? 'yes' : 'no') : '—');
    setById('eve-bond-str', liv.bond_strength != null ? String(liv.bond_strength) : '—');
    var t = [];
    if (liv.bond_trust != null) t.push('T' + liv.bond_trust);
    if (liv.bond_comfort != null) t.push('C' + liv.bond_comfort);
    if (liv.bond_curious != null) t.push('Q' + liv.bond_curious);
    setById('eve-bond-tcc', t.length ? t.join(' · ') : '—');
    setById('eve-bond-docks', liv.bond_shared_docks != null ? String(liv.bond_shared_docks) : '—');
    var ea = liv.eve_assist;
    if (ea) {
      setById('eve-as-state', ea.state_name != null ? String(ea.state_name) : (ea.state != null ? String(ea.state) : '—'));
      setById('eve-as-zone', ea.zone != null ? String(ea.zone) : '—');
      setById('eve-as-bias', ea.bias != null ? String(ea.bias) : '—');
      setById('eve-as-stale', ea.stale ? 'Stale (ignore for motion)' : 'Fresh');
    } else {
      setById('eve-as-state', '—');
      setById('eve-as-zone', '—');
      setById('eve-as-bias', '—');
      setById('eve-as-stale', '—');
    }
  }
  if (panel && panel.ok) {
    setById('eve-transport', panel.transport || 'uart_only');
    var d = panel.dock || {};
    setById('eve-dock-fsm', d.fsm != null ? String(d.fsm) : '—');
    setById('eve-dock-active', d.active != null ? (d.active ? 'yes' : 'no') : '—');
    setById('eve-dock-node', d.dock_node_online != null ? (d.dock_node_online ? 'online' : 'offline') : '—');
  }
  var mic = null;
  if (uart && uart.payload) {
    try {
      var up = typeof uart.payload === 'string' ? JSON.parse(uart.payload) : uart.payload;
      if (up && (up.event || up.level != null || up.ambient != null)) mic = up;
    } catch (e) {}
  }
  if (!mic && panel && panel.eve && panel.eve.mic) mic = panel.eve.mic;
  if (mic) {
    setById('eve-mic-event', mic.event || mic.last_event || '—');
    var lev = mic.level != null ? String(mic.level) : '—';
    var amb = mic.ambient != null ? String(mic.ambient) : '—';
    setById('eve-mic-level', lev + ' / ' + amb);
    var flags = [];
    if (mic.spike != null) flags.push('S:' + (mic.spike ? 'Y' : 'N'));
    if (mic.clap != null) flags.push('C:' + (mic.clap ? 'Y' : 'N'));
    if (mic.quiet != null) flags.push('Q:' + (mic.quiet ? 'Y' : 'N'));
    setById('eve-mic-flags', flags.length ? flags.join(' · ') : 'UART event');
  }
  if (learning && learning.ok) {
    var entries = Array.isArray(learning.entries) ? learning.entries : [];
    setById('eve-learning-count', String(entries.length));
    var preview = entries.slice(-8).map(function (e) {
      if (!e) return '';
      return '[' + (e.ms || e.ts || '?') + '] ' + (e.d || e.trigger || JSON.stringify(e));
    }).join('\n');
    setById('eve-learning-list', preview || 'No memory events yet');
  }
  if (monitor && monitor.ok) {
    setById('eve-uart-peer', monitor.peer || '—');
    setById('eve-uart-monitor', JSON.stringify(monitor.bridge || monitor).slice(0, 900));
  }
}

function pushEvePersonality() {
  var c = document.getElementById('eve-tune-curiosity');
  var r = document.getElementById('eve-tune-response');
  var a = document.getElementById('eve-tune-activity');
  var qs = '?curiosity=' + encodeURIComponent(c ? c.value : 70) +
           '&responsiveness=' + encodeURIComponent(r ? r.value : 65) +
           '&activity=' + encodeURIComponent(a ? a.value : 50);
  fetch(api('/api/eve/personality' + qs), { cache: 'no-store', headers: apiAuthHeaders() })
    .then(function (res) { return res.json(); })
    .then(function (j) {
      setById('eve-tune-status', j && j.ok ? 'Personality pushed over UART' : 'EVE personality push failed');
    }).catch(function () {
      setById('eve-tune-status', 'EVE personality push failed');
    });
}

function pushEveMicSettings() {
  var s = document.getElementById('eve-mic-spike');
  var c = document.getElementById('eve-mic-clap');
  var q = document.getElementById('eve-mic-quiet');
  var qs = '?enabled=1&spike=' + encodeURIComponent(s ? s.value : 1800) +
           '&clap=' + encodeURIComponent(c ? c.value : 4200) +
           '&quiet=' + encodeURIComponent(q ? q.value : 180) +
           '&cooldown=2500';
  fetch(api('/api/eve/mic' + qs), { cache: 'no-store', headers: apiAuthHeaders() })
    .then(function (res) { return res.json(); })
    .then(function (j) {
      setById('eve-mic-status', j && j.ok ? 'Mic settings pushed over UART' : 'EVE mic settings push failed');
    }).catch(function () {
      setById('eve-mic-status', 'EVE mic settings push failed');
    });
}

function testEveMicReaction() {
  fetch(api('/api/eve/mic?test=1'), { cache: 'no-store', headers: apiAuthHeaders() })
    .then(function (res) { return res.json(); })
    .then(function (j) {
      setById('eve-mic-status', j && j.ok ? 'Mic test sent over UART' : 'EVE mic test failed');
    }).catch(function () {
      setById('eve-mic-status', 'EVE mic test failed');
    });
}

function pollLivingTelemetry() {
  var h = apiAuthHeaders();
  Promise.all([
    fetch(api('/api/living/telemetry'), { cache: 'no-store', headers: h }).then(function (r) { return r.json(); }).catch(function () { return null; }),
    fetch(api('/api/eve/status'), { cache: 'no-store', headers: h }).then(function (r) { return r.json(); }).catch(function () { return null; }),
    fetch(api('/api/companion/panel'), { cache: 'no-store', headers: h }).then(function (r) { return r.json(); }).catch(function () { return null; }),
    fetch(api('/api/learning/shared'), { cache: 'no-store', headers: h }).then(function (r) { return r.json(); }).catch(function () { return null; }),
    fetch(api('/api/uart/log'), { cache: 'no-store', headers: h }).then(function (r) { return r.json(); }).catch(function () { return null; })
  ]).then(function (arr) {
    applyLivingEveUI(arr[0], arr[1], arr[2], arr[3], arr[4]);
  });
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
        ? Math.min(100, Math.max(0, ((battery.voltage - 6.0) / 2.4) * 100))
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
    const pct = battery.percent != null ? battery.percent : (battery.voltage != null ? Math.min(100, Math.max(0, (battery.voltage - 6.0) / 2.4 * 100)) : 80);
    const pctDock =
      battery.percent != null
        ? battery.percent
        : battery.voltage != null
          ? Math.min(100, Math.max(0, (battery.voltage - 6.0) / 2.4 * 100))
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
  pollLivingTelemetry();
  setInterval(fetchStatus, 5000);
  setInterval(pollNodeHealth, 1500);
  setInterval(pollMotionOperator, 1500);
  setInterval(pollLivingTelemetry, 2000);
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

</script>
</body>
</html>
)rawliteral";