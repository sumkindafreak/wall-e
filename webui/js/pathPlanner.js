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
