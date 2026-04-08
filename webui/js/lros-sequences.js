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
