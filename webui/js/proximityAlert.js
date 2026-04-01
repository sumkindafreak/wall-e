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
