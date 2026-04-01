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
