/*
 * Web dashboard for 4-relay paludarium controller
 */
#ifndef PAGE_HTML_H
#define PAGE_HTML_H

const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>Paludarium 4-Relay</title>
  <style>
    *{margin:0;padding:0;box-sizing:border-box}
    body{font-family:system-ui,-apple-system,sans-serif;background:#0f172a;color:#e2e8f0;padding:16px;min-height:100vh}
    main{max-width:600px;margin:0 auto}
    header{background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.1);border-radius:12px;padding:14px 16px;margin-bottom:16px;display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:8px}
    h1{font-size:18px;color:#7dd3fc}
    .badge{padding:4px 10px;border-radius:6px;font-size:11px;font-weight:600}
    .badge.online{background:rgba(34,197,94,0.2);color:#22c55e}
    .badge.offline{background:rgba(239,68,68,0.2);color:#ef4444}
    .card{background:rgba(255,255,255,0.05);border:1px solid rgba(255,255,255,0.1);border-radius:12px;padding:16px;margin-bottom:12px}
    .card h2{font-size:12px;color:#94a3b8;margin-bottom:12px;font-weight:600;text-transform:uppercase}
    .relay-grid{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}
    .relay{ padding:16px;border:none;border-radius:10px;font-weight:600;color:#fff;cursor:pointer;transition:transform .15s,box-shadow .15s;font-size:14px}
    .relay.on{background:#16a34a;box-shadow:0 0 0 2px rgba(22,163,74,0.4)}
    .relay.off{background:#475569}
    .relay:hover{transform:scale(1.02)}
    .mode-row{display:flex;gap:8px;flex-wrap:wrap}
    .mode-btn{padding:10px 16px;border:1px solid rgba(255,255,255,0.2);background:rgba(255,255,255,0.06);color:#e2e8f0;border-radius:8px;cursor:pointer;font-weight:600;font-size:13px}
    .mode-btn.active{background:#2563eb;border-color:#2563eb;color:#fff}
    .sensors{display:grid;grid-template-columns:repeat(2,1fr);gap:10px}
    .sensor{background:rgba(0,0,0,0.25);border-radius:8px;padding:10px;text-align:center}
    .sensor .v{font-size:20px;font-weight:700;color:#7dd3fc}
    .sensor .l{font-size:10px;color:#94a3b8;text-transform:uppercase;margin-top:4px}
    .wifi-section{margin-top:16px}
    .wifi-section input, .wifi-section select{width:100%;padding:8px 10px;margin-bottom:8px;background:rgba(255,255,255,0.06);border:1px solid rgba(255,255,255,0.15);border-radius:8px;color:#e2e8f0}
    .wifi-section button{padding:8px 14px;margin-right:8px;margin-top:4px;background:#2563eb;border:none;border-radius:8px;color:#fff;font-weight:600;cursor:pointer}
    .wifi-section button.secondary{background:rgba(255,255,255,0.1);color:#e2e8f0}
    #wifiMsg{font-size:12px;margin-top:6px;color:#94a3b8}
  </style>
</head>
<body>
  <main>
    <header>
      <h1>Paludarium 4-Relay</h1>
      <span id="status" class="badge offline">--</span>
    </header>

    <div class="card">
      <h2>Mode</h2>
      <div class="mode-row">
        <button class="mode-btn active" id="btnAuto">Auto</button>
        <button class="mode-btn" id="btnManual">Manual</button>
      </div>
    </div>

    <div class="card">
      <h2>Relays</h2>
      <div class="relay-grid">
        <button class="relay off" data-idx="0" id="r0">Relay 1</button>
        <button class="relay off" data-idx="1" id="r1">Relay 2</button>
        <button class="relay off" data-idx="2" id="r2">Relay 3</button>
        <button class="relay off" data-idx="3" id="r3">Relay 4</button>
      </div>
    </div>

    <div class="card">
      <h2>Sensors</h2>
      <div class="sensors">
        <div class="sensor"><div class="v" id="temp">--</div><div class="l">Temp C</div></div>
        <div class="sensor"><div class="v" id="humid">--</div><div class="l">Humidity %</div></div>
        <div class="sensor"><div class="v" id="water">--</div><div class="l">Water</div></div>
      </div>
    </div>

    <div class="card">
      <h2>Light (30x4 matrix)</h2>
      <div class="mode-row" style="margin-bottom:10px">
        <button class="mode-btn" id="ledOff">Off</button>
        <button class="mode-btn" id="ledManual">Manual</button>
        <button class="mode-btn active" id="ledAuto">Auto</button>
      </div>
      <label style="font-size:12px;color:#94a3b8">Brightness</label>
      <input type="range" id="ledBright" min="0" max="255" value="120" style="width:100%;margin-top:4px">
      <span id="ledBrightVal" style="font-size:12px;margin-left:8px">120</span>
    </div>

    <div class="card">
      <h2>Time &amp; Schedule</h2>
      <p style="font-size:14px;margin-bottom:8px"><span id="timeDisplay">--:--</span> <span id="timeSyncBadge" class="badge offline">No NTP</span></p>
      <label style="font-size:11px;color:#94a3b8">Day start (min)</label>
      <input type="number" id="dayStart" min="0" max="1439" value="360" style="width:80px;padding:6px;margin:4px 8px 0 0">
      <label style="font-size:11px;color:#94a3b8">Day end (min)</label>
      <input type="number" id="dayEnd" min="0" max="1439" value="1200" style="width:80px;padding:6px;margin:4px 8px 0 0">
      <button class="secondary" id="btnSaveSchedule" style="margin-top:8px">Save schedule</button>
      <div id="scheduleMsg" style="font-size:12px;margin-top:6px;color:#94a3b8"></div>
    </div>

    <div class="card wifi-section">
      <h2>Wi-Fi</h2>
      <button class="secondary" id="btnScan">Scan</button>
      <select id="wifiList"><option value="">Select network...</option></select>
      <input type="password" id="wifiPass" placeholder="Password">
      <button id="btnSaveWifi">Save &amp; Restart</button>
      <div id="wifiMsg"></div>
    </div>
  </main>
  <script>
    const statusEl = document.getElementById('status');
    const relayIds = ['r0','r1','r2','r3'];
    const labels = ['Relay 1','Relay 2','Relay 3','Relay 4'];

    function updateUI(data) {
      if (data.wifi) { statusEl.textContent = 'ONLINE'; statusEl.className = 'badge online'; }
      else if (data.ap) { statusEl.textContent = 'AP'; statusEl.className = 'badge offline'; }
      else { statusEl.textContent = 'OFFLINE'; statusEl.className = 'badge offline'; }

      document.getElementById('temp').textContent = (data.tp != null) ? data.tp.toFixed(1) + '°' : '--';
      document.getElementById('humid').textContent = (data.hm != null) ? data.hm.toFixed(0) + '%' : '--';
      document.getElementById('water').textContent = data.wh ? 'High' : 'OK';
      if (data.time != null) document.getElementById('timeDisplay').textContent = data.time;
      const syncEl = document.getElementById('timeSyncBadge');
      if (syncEl) { syncEl.textContent = data.timeSynced ? 'NTP synced' : 'No NTP'; syncEl.className = data.timeSynced ? 'badge online' : 'badge offline'; }
      const lb = document.getElementById('ledBright'), lv = document.getElementById('ledBrightVal');
      if (lb && lv && data.ledBrightness != null) { lb.value = data.ledBrightness; lv.textContent = data.ledBrightness; }
      if (data.ledMode != null) {
        const o = document.getElementById('ledOff'), m = document.getElementById('ledManual'), a = document.getElementById('ledAuto');
        if (o) o.classList.toggle('active', data.ledMode === 0);
        if (m) m.classList.toggle('active', data.ledMode === 1);
        if (a) a.classList.toggle('active', data.ledMode === 2);
      }

      const arr = data.relays || [];
      relayIds.forEach((id, i) => {
        const btn = document.getElementById(id);
        if (!btn) return;
        btn.textContent = labels[i];
        btn.className = (arr[i] === 1) ? 'relay on' : 'relay off';
      });

      document.getElementById('btnAuto').classList.toggle('active', !!data.auto);
      document.getElementById('btnManual').classList.toggle('active', !data.auto);
    }

    function fetchAPI() {
      fetch('/api').then(r => r.json()).then(updateUI).catch(() => {});
    }

    document.querySelectorAll('.relay[data-idx]').forEach(btn => {
      btn.addEventListener('click', () => {
        const idx = parseInt(btn.getAttribute('data-idx'), 10);
        const on = !btn.classList.contains('on');
        fetch('/r?i=' + idx + '&s=' + (on ? 1 : 0)).then(r => { if (r.ok) fetchAPI(); });
      });
    });

    document.getElementById('btnAuto').addEventListener('click', () => {
      fetch('/system/mode', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: '{"mode":"auto"}' })
        .then(r => { if (r.ok) fetchAPI(); });
    });
    document.getElementById('btnManual').addEventListener('click', () => {
      fetch('/system/mode', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: '{"mode":"manual"}' })
        .then(r => { if (r.ok) fetchAPI(); });
    });

    document.getElementById('ledOff').addEventListener('click', () => {
      fetch('/led/mode', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: '{"mode":"off"}' }).then(r => { if (r.ok) fetchAPI(); });
    });
    document.getElementById('ledManual').addEventListener('click', () => {
      fetch('/led/mode', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: '{"mode":"manual"}' }).then(r => { if (r.ok) fetchAPI(); });
    });
    document.getElementById('ledAuto').addEventListener('click', () => {
      fetch('/led/mode', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: '{"mode":"auto"}' }).then(r => { if (r.ok) fetchAPI(); });
    });
    document.getElementById('ledBright').addEventListener('input', () => {
      const v = parseInt(document.getElementById('ledBright').value, 10);
      document.getElementById('ledBrightVal').textContent = v;
      fetch('/led/brightness', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ value: v }) }).then(r => { if (r.ok) fetchAPI(); });
    });
    document.getElementById('btnSaveSchedule').addEventListener('click', () => {
      const dayStart = parseInt(document.getElementById('dayStart').value, 10) || 360;
      const dayEnd = parseInt(document.getElementById('dayEnd').value, 10) || 1200;
      document.getElementById('scheduleMsg').textContent = 'Saving...';
      fetch('/time/schedule', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ dayStart: dayStart, dayEnd: dayEnd }) })
        .then(r => r.json()).then(() => { document.getElementById('scheduleMsg').textContent = 'Saved'; }).catch(() => { document.getElementById('scheduleMsg').textContent = 'Error'; });
    });

    document.getElementById('btnScan').addEventListener('click', () => {
      document.getElementById('wifiMsg').textContent = 'Scanning...';
      fetch('/wifi/scan').then(r => r.json()).then(nets => {
        const sel = document.getElementById('wifiList');
        sel.innerHTML = '<option value="">Select network...</option>';
        (nets || []).sort((a,b) => (b.rssi || 0) - (a.rssi || 0)).forEach(n => {
          const o = document.createElement('option');
          o.value = n.ssid || '';
          o.textContent = (n.ssid || '') + ' (' + (n.rssi || 0) + ' dBm)';
          sel.appendChild(o);
        });
        document.getElementById('wifiMsg').textContent = 'Found ' + (nets ? nets.length : 0) + ' networks';
      }).catch(() => { document.getElementById('wifiMsg').textContent = 'Scan failed'; });
    });

    document.getElementById('btnSaveWifi').addEventListener('click', () => {
      const ssid = document.getElementById('wifiList').value;
      const pass = document.getElementById('wifiPass').value;
      if (!ssid) { document.getElementById('wifiMsg').textContent = 'Select a network'; return; }
      document.getElementById('wifiMsg').textContent = 'Saving...';
      fetch('/wifi/connect', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ ssid: ssid, pass: pass })
      }).then(r => r.json()).then(() => {
        document.getElementById('wifiMsg').textContent = 'Saved. Restarting...';
      }).catch(() => { document.getElementById('wifiMsg').textContent = 'Error'; });
    });

    fetchAPI();
    setInterval(fetchAPI, 5000);
  </script>
</body>
</html>
)rawliteral";

#endif
