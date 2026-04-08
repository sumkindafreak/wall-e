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
