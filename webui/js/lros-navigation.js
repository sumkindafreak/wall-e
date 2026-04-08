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

