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
