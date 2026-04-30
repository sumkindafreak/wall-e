#include "eve_web_server.h"
#include "config.h"

#if EVE_ENABLE_DOCKED_WEBUI
#include "audio_control.h"
#include "battery_monitor.h"
#include "eve_desktop_companion.h"
#include "eve_status_manager.h"
#include "mic_input.h"
#include "neopixel_control.h"
#include "servo_control.h"
#include "state_machine.h"
#include <WebServer.h>
#include <WiFi.h>

static WebServer s_server(80);
static bool s_started = false;
static bool s_apUp = false;
static uint32_t s_lastLogMs = 0;
static uint32_t s_lastDockedMs = 0;
static uint32_t s_lastOffLogMs = 0;

static const char EVE_WEB_PAGE[] PROGMEM = R"HTML(
<!doctype html><html lang="en"><head>
<meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>EVE Companion Console</title>
<style>
:root{--bg:#eefaff;--card:#ffffffd8;--line:#bfefff;--ink:#103244;--muted:#5d8190;--blue:#8de7ff;--blue2:#d8f7ff;--green:#74e6b1;--warn:#ffd166}
*{box-sizing:border-box}body{margin:0;font-family:system-ui,-apple-system,Segoe UI,sans-serif;background:radial-gradient(circle at 12% 0,#fff 0,#eefaff 34%,#dff7ff 100%);color:var(--ink)}
.wrap{max-width:1180px;margin:0 auto;padding:16px}.hero{border:1px solid var(--line);border-radius:28px;padding:18px;background:linear-gradient(135deg,#fff,#e8fbff);box-shadow:0 18px 60px #8de7ff44}
.kicker{letter-spacing:.22em;text-transform:uppercase;color:#4bbce0;font-size:12px}.title{font-size:38px;line-height:1;margin:6px 0}.sub{color:var(--muted);max-width:780px}.chips{display:flex;flex-wrap:wrap;gap:8px;margin-top:12px}.pill{border-radius:999px;padding:7px 11px;background:var(--blue2);border:1px solid var(--line);font-weight:800}.pill.ok{background:#e4fff3}.pill.warn{background:#fff4cf}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(260px,1fr));gap:14px;margin-top:14px}.span{grid-column:1/-1}.card{background:var(--card);border:1px solid var(--line);border-radius:22px;padding:15px;box-shadow:0 8px 28px #79dfff22}.card h2{font-size:18px;margin:0 0 12px}.row{display:flex;justify-content:space-between;gap:12px;border-top:1px solid #e8f9ff;padding:8px 0}.row:first-of-type{border-top:0}.val{font-weight:800}.tiny{font-size:12px;color:var(--muted)}pre{white-space:pre-wrap;word-break:break-word;max-height:190px;overflow:auto}
button{border:0;border-radius:15px;background:linear-gradient(135deg,var(--blue),#fff);color:var(--ink);padding:10px 12px;font-weight:900;box-shadow:0 8px 18px #8de7ff55}button.green{background:linear-gradient(135deg,var(--green),#fff)}button.warn{background:linear-gradient(135deg,var(--warn),#fff)}button.ghost{background:#fff;border:1px solid var(--line)}
.buttons{display:grid;grid-template-columns:repeat(auto-fit,minmax(108px,1fr));gap:9px}.controls{display:grid;grid-template-columns:1fr 1fr;gap:10px}label{display:block;font-size:13px;color:var(--muted);font-weight:800;margin-top:7px}input[type=range]{width:100%}.last{margin-top:10px;color:var(--muted);font-size:13px}
</style></head><body><div class="wrap">
<section class="hero"><div class="kicker">EVE docked companion</div><h1 class="title">EVE Companion Console</h1><p class="sub">Interact with EVE, tune her personality, test her senses, and run safe dock-only manual controls.</p><div class="chips"><span class="pill" id="link">loading...</span><span class="pill" id="state">state -</span><span class="pill" id="peer">peer -</span><span class="pill" id="bat">battery -</span><span class="pill" id="mic-en">mic -</span></div></section>
<div class="grid">
<div class="card span"><h2>Interact With EVE</h2><div class="buttons"><button class="green" onclick="interact('hello')">Say Hi</button><button onclick="interact('look')">Look At Me</button><button onclick="interact('curious')">Curious</button><button class="green" onclick="interact('happy')">Happy</button><button onclick="interact('sleepy')">Sleepy</button><button class="warn" onclick="interact('wake')">Wake Up</button><button onclick="interact('calm')">Calm Down</button><button onclick="interact('attention')">Attention</button><button onclick="interact('play')">Play</button></div><div class="last" id="last-action">Ready.</div></div>
<div class="card"><h2>Live Status</h2><div class="row"><span>Docked</span><span class="val" id="docked">-</span></div><div class="row"><span>Dock mimic</span><span class="val" id="mimic">-</span></div><div class="row"><span>AP</span><span class="val" id="ap">-</span></div><div class="row"><span>Session</span><span class="val" id="session">-</span></div><div class="row"><span>Heap</span><span class="val" id="heap">-</span></div><div class="row"><span>Current</span><span class="val" id="cur">-</span></div></div>
<div class="card"><h2>Manual Pose</h2><label>Head pan <span id="headv">90</span></label><input id="head" type="range" min="45" max="135" value="90" oninput="headv.textContent=this.value"><label>Right arm <span id="armv">90</span></label><input id="arm" type="range" min="0" max="180" value="90" oninput="armv.textContent=this.value"><div class="buttons"><button onclick="pose()">Send Pose</button><button class="ghost" onclick="posePreset('neutral')">Neutral</button><button class="warn" onclick="posePreset('stop')">Stop</button></div></div>
<div class="card"><h2>Glow & Audio</h2><label>Glow pattern <span id="glowv">5</span></label><input id="glow" type="range" min="0" max="9" value="5" oninput="glowv.textContent=this.value"><label>Audio track <span id="trackv">1</span></label><input id="track" type="range" min="0" max="20" value="1" oninput="trackv.textContent=this.value"><div class="buttons"><button onclick="setGlow()">Set Glow</button><button onclick="playAudio()">Play Track</button><button class="green" onclick="pulse()">Soft Pulse</button></div></div>
<div class="card"><h2>Personality</h2><div class="buttons"><button onclick="profile('calm')">Calm</button><button onclick="profile('curious')">Curious</button><button onclick="profile('sleepy')">Sleepy</button><button onclick="profile('excited')">Excited</button><button onclick="profile('desk')">Desk</button></div><label>Curiosity <span id="curiosityv">70</span></label><input id="curiosity" type="range" min="0" max="100" value="70" oninput="curiosityv.textContent=this.value"><label>Comfort <span id="comfortv">55</span></label><input id="comfort" type="range" min="0" max="100" value="55" oninput="comfortv.textContent=this.value"><label>Excitement <span id="excitementv">35</span></label><input id="excitement" type="range" min="0" max="100" value="35" oninput="excitementv.textContent=this.value"><label>Sleepiness <span id="sleepinessv">20</span></label><input id="sleepiness" type="range" min="0" max="100" value="20" oninput="sleepinessv.textContent=this.value"><label>Responsiveness <span id="responsev">65</span></label><input id="response" type="range" min="0" max="100" value="65" oninput="responsev.textContent=this.value"><label>Activity <span id="activityv">50</span></label><input id="activity" type="range" min="0" max="100" value="50" oninput="activityv.textContent=this.value"><button onclick="personality()">Update Buddy</button></div>
<div class="card"><h2>Mic Sense</h2><div class="row"><span>Level</span><span class="val" id="mic-level">-</span></div><div class="row"><span>Ambient</span><span class="val" id="mic-amb">-</span></div><div class="row"><span>Last event</span><span class="val" id="mic-event">-</span></div><div class="row"><span>Spike / clap / quiet</span><span class="val" id="mic-flags">-</span></div><label>Spike threshold</label><input id="mic-spike" type="range" min="50" max="8000" value="1800"><label>Clap threshold</label><input id="mic-clap" type="range" min="100" max="12000" value="4200"><label>Quiet threshold</label><input id="mic-quiet" type="range" min="10" max="2000" value="180"><label>Cooldown ms</label><input id="mic-cooldown" type="range" min="250" max="10000" value="2500"><div class="buttons"><button onclick="saveMic(1)">Mic On</button><button class="ghost" onclick="saveMic(0)">Mic Off</button><button class="green" onclick="testMic()">Test</button></div></div>
<div class="card"><h2>Dock Test</h2><p class="tiny">Bench-only controls. Leave mimic off for normal docking.</p><div class="buttons"><button class="warn" onclick="dockMimic(1)">Mimic Dock</button><button class="ghost" onclick="dockMimic(0)">Mimic Off</button><button class="ghost" onclick="resetState()">Reset State</button></div></div>
<div class="card span"><h2>Diagnostics</h2><pre id="raw">-</pre></div>
</div></div>
<script>
function set(id,v){var e=document.getElementById(id);if(e)e.textContent=v}
function call(p){return fetch(p,{cache:'no-store'}).then(r=>r.json()).then(j=>{set('last-action',(j.event||j.profile||j.action||j.ok?'OK':'Failed')+' '+p);refresh();return j}).catch(e=>set('last-action','Request failed: '+p))}
function refresh(){fetch('/api/status',{cache:'no-store'}).then(r=>r.json()).then(j=>{set('link',j.docked?'Docked WebUI active':'Not docked');set('state','state '+(j.state||'-'));set('peer','peer '+(j.uart_peer||'-'));set('bat','battery '+(j.bat_pct||0)+'% '+Number(j.bat_v||0).toFixed(2)+'V');set('docked',j.docked?'yes':'no');set('mimic',j.dock_mimic?'on':'off');set('ap',j.ap_up?'up':'off');set('session',j.session||0);set('heap',j.heap);set('cur',Number(j.bat_a||0).toFixed(2)+'A');var m=j.mic||{};set('mic-en','mic '+(m.enabled?'on':'off'));set('mic-level',m.level!=null?m.level:'-');set('mic-amb',m.ambient!=null?m.ambient:'-');set('mic-event',m.last_event||'-');set('mic-flags','S:'+(m.spike?'Y':'N')+' C:'+(m.clap?'Y':'N')+' Q:'+(m.quiet?'Y':'N'));set('raw',JSON.stringify(j,null,2));}).catch(()=>set('link','offline'))}
function interact(a){call('/api/interaction?action='+encodeURIComponent(a))}
function pose(){call('/api/control/pose?head='+head.value+'&arm='+arm.value)}
function posePreset(p){call('/api/control/pose?preset='+p)}
function pulse(){call('/api/pulse')}
function setGlow(){call('/api/control/glow?pattern='+glow.value)}
function playAudio(){call('/api/control/audio?track='+track.value)}
function profile(p){call('/api/control/personality_profile?profile='+p)}
function personality(){call('/api/personality?curiosity='+curiosity.value+'&comfort='+comfort.value+'&excitement='+excitement.value+'&sleepiness='+sleepiness.value+'&responsiveness='+response.value+'&activity='+activity.value)}
function saveMic(en){call('/api/mic/settings?spike='+document.getElementById('mic-spike').value+'&clap='+document.getElementById('mic-clap').value+'&quiet='+document.getElementById('mic-quiet').value+'&cooldown='+document.getElementById('mic-cooldown').value+'&enabled='+en)}
function testMic(){call('/api/mic/test')}
function dockMimic(en){call('/api/control/dock_mimic?enabled='+en)}
function resetState(){call('/api/control/reset_state')}
setInterval(refresh,1000);refresh();
</script></body></html>
)HTML";

static void addCors() {
  s_server.sendHeader("Access-Control-Allow-Origin", "*");
}

static void sendOkJson(const char* action) {
  String j = "{\"ok\":true";
  if (action && *action) {
    j += ",\"action\":\"";
    j += action;
    j += "\"";
  }
  j += "}";
  addCors();
  s_server.send(200, "application/json", j);
}

static void applyPersonalityValues(int curiosity, int comfort, int excitement, int sleepiness, int responsiveness, int activity) {
  char json[192];
  snprintf(json, sizeof(json),
           "{\"cmd\":\"personality\",\"curiosity\":%d,\"comfort\":%d,\"excitement\":%d,"
           "\"sleepiness\":%d,\"responsiveness\":%d,\"activity\":%d}",
           constrain(curiosity, 0, 100),
           constrain(comfort, 0, 100),
           constrain(excitement, 0, 100),
           constrain(sleepiness, 0, 100),
           constrain(responsiveness, 0, 100),
           constrain(activity, 0, 100));
  eveDesktopCompanionApplyConfigJson(json);
}

static void performInteraction(const char* action) {
  if (!action) action = "";
  if (strcmp(action, "hello") == 0) {
    servoSetHeadPanTarget(90); servoSetRightArmTarget(132); neopixelSetPattern(5); audioPlayTrack(1);
  } else if (strcmp(action, "look") == 0) {
    servoSetHeadPanTarget(90); servoSetRightArmTarget(105); neopixelSetPattern(1);
  } else if (strcmp(action, "curious") == 0) {
    servoSetHeadPanTarget(108); servoSetRightArmTarget(104); neopixelSetPattern(5); applyPersonalityValues(88, 55, 45, 10, 78, 58);
  } else if (strcmp(action, "happy") == 0) {
    servoSetHeadPanTarget(90); servoSetRightArmTarget(145); neopixelSetPattern(5); audioPlayTrack(1); applyPersonalityValues(70, 82, 70, 5, 82, 65);
  } else if (strcmp(action, "sleepy") == 0) {
    servoSetHeadPanTarget(90); servoSetRightArmTarget(72); neopixelSetPattern(4); applyPersonalityValues(28, 78, 8, 88, 30, 18);
  } else if (strcmp(action, "wake") == 0) {
    servoSetHeadPanTarget(90); servoSetRightArmTarget(118); neopixelSetPattern(5); applyPersonalityValues(72, 55, 62, 5, 80, 72);
  } else if (strcmp(action, "calm") == 0) {
    servoSetHeadPanTarget(90); servoSetRightArmTarget(88); neopixelSetPattern(1); applyPersonalityValues(45, 85, 12, 35, 45, 28);
  } else if (strcmp(action, "attention") == 0) {
    servoSetHeadPanTarget(74); servoSetRightArmTarget(120); neopixelSetPattern(5);
  } else if (strcmp(action, "play") == 0) {
    servoSetHeadPanTarget(110); servoSetRightArmTarget(150); neopixelSetPattern(5); audioPlayTrack(2); applyPersonalityValues(82, 62, 78, 0, 85, 82);
  }
}

static void handleRoot() {
  addCors();
  s_server.send_P(200, "text/html", EVE_WEB_PAGE);
}

static void handleStatus() {
  addCors();
  String j = eveStatusManagerGetJSON();
  if (j.endsWith("}")) j.remove(j.length() - 1);
  j += ",\"docked\":"; j += stateMachineIsDocked() ? "true" : "false";
  j += ",\"dock_mimic\":"; j += stateMachineIsDockMimic() ? "true" : "false";
  j += ",\"ap_up\":"; j += s_apUp ? "true" : "false";
  j += ",\"session\":"; j += (uint32_t)stateMachineGetSessionId();
  j += ",\"ap_ssid\":\""; j += EVE_WEBUI_AP_SSID; j += "\"";
  j += ",\"mic\":"; j += getMicStatusJson();
  j += "}";
  s_server.send(200, "application/json", j);
}

static void handlePose() {
  int head = s_server.hasArg("head") ? s_server.arg("head").toInt() : 90;
  int arm = s_server.hasArg("arm") ? s_server.arg("arm").toInt() : 90;
  head = constrain(head, 45, 135);
  arm = constrain(arm, 0, 180);
  servoSetHeadPanTarget((int16_t)head);
  servoSetRightArmTarget((int16_t)arm);
  addCors();
  s_server.send(200, "application/json", "{\"ok\":true}");
}

static void handlePulse() {
  neopixelSetPattern(5);
  addCors();
  s_server.send(200, "application/json", "{\"ok\":true}");
}

static void handlePersonality() {
  int curiosity = s_server.hasArg("curiosity") ? s_server.arg("curiosity").toInt() : 70;
  int comfort = s_server.hasArg("comfort") ? s_server.arg("comfort").toInt() : 55;
  int excitement = s_server.hasArg("excitement") ? s_server.arg("excitement").toInt() : 35;
  int sleepiness = s_server.hasArg("sleepiness") ? s_server.arg("sleepiness").toInt() : 20;
  int responsiveness = s_server.hasArg("responsiveness") ? s_server.arg("responsiveness").toInt() : 65;
  int activity = s_server.hasArg("activity") ? s_server.arg("activity").toInt() : 50;
  char json[192];
  snprintf(json, sizeof(json),
           "{\"cmd\":\"personality\",\"curiosity\":%d,\"comfort\":%d,\"excitement\":%d,"
           "\"sleepiness\":%d,\"responsiveness\":%d,\"activity\":%d}",
           constrain(curiosity, 0, 100), constrain(comfort, 0, 100), constrain(excitement, 0, 100),
           constrain(sleepiness, 0, 100), constrain(responsiveness, 0, 100), constrain(activity, 0, 100));
  bool ok = eveDesktopCompanionApplyConfigJson(json);
  addCors();
  s_server.send(ok ? 200 : 400, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

static void handleInteraction() {
  String actionArg = s_server.hasArg("action") ? s_server.arg("action") : "";
  const char* action = actionArg.c_str();
  performInteraction(action);
  String j = "{\"ok\":true,\"event\":\"";
  j += action;
  j += "\"}";
  addCors();
  s_server.send(200, "application/json", j);
}

static void handleControlPose() {
  String preset = s_server.hasArg("preset") ? s_server.arg("preset") : "";
  int head = s_server.hasArg("head") ? s_server.arg("head").toInt() : 90;
  int arm = s_server.hasArg("arm") ? s_server.arg("arm").toInt() : 90;
  if (preset == "neutral" || preset == "stop") {
    head = 90;
    arm = 90;
  }
  servoSetHeadPanTarget((int16_t)constrain(head, 45, 135));
  servoSetRightArmTarget((int16_t)constrain(arm, 0, 180));
  sendOkJson(preset.length() ? preset.c_str() : "pose");
}

static void handleControlGlow() {
  uint8_t pattern = (uint8_t)constrain(s_server.hasArg("pattern") ? s_server.arg("pattern").toInt() : 1, 0, 255);
  neopixelSetPattern(pattern);
  sendOkJson("glow");
}

static void handleControlAudio() {
  uint8_t track = (uint8_t)constrain(s_server.hasArg("track") ? s_server.arg("track").toInt() : 1, 0, 255);
  audioPlayTrack(track);
  sendOkJson("audio");
}

static void handleControlDockMimic() {
  bool enabled = s_server.hasArg("enabled") && s_server.arg("enabled") != "0";
  stateMachineSetDockMimic(enabled, EVE_DOCK_MIMIC_FAKE_CHARGING != 0);
  sendOkJson(enabled ? "dock_mimic_on" : "dock_mimic_off");
}

static void handleControlResetState() {
  stateMachineInit();
  sendOkJson("reset_state");
}

static void handlePersonalityProfile() {
  String profile = s_server.hasArg("profile") ? s_server.arg("profile") : "desk";
  if (profile == "calm") applyPersonalityValues(45, 85, 12, 35, 45, 28);
  else if (profile == "curious") applyPersonalityValues(88, 55, 45, 10, 78, 58);
  else if (profile == "sleepy") applyPersonalityValues(28, 78, 8, 88, 30, 18);
  else if (profile == "excited") applyPersonalityValues(82, 62, 78, 0, 85, 82);
  else applyPersonalityValues(70, 65, 35, 20, 65, 50);
  String j = "{\"ok\":true,\"profile\":\"";
  j += profile;
  j += "\"}";
  addCors();
  s_server.send(200, "application/json", j);
}

static void handleMicStatus() {
  addCors();
  s_server.send(200, "application/json", getMicStatusJson());
}

static void handleMicSettings() {
  EveMicSettings settings = micGetSettings();
  if (s_server.hasArg("enabled")) settings.reactionsEnabled = s_server.arg("enabled") != "0";
  if (s_server.hasArg("spike")) settings.spikeThreshold = s_server.arg("spike").toFloat();
  if (s_server.hasArg("clap")) settings.clapThreshold = s_server.arg("clap").toFloat();
  if (s_server.hasArg("quiet")) settings.quietThreshold = s_server.arg("quiet").toFloat();
  if (s_server.hasArg("cooldown")) settings.reactionCooldownMs = (uint32_t)s_server.arg("cooldown").toInt();
  micSetSettings(settings);
  addCors();
  s_server.send(200, "application/json", getMicStatusJson());
}

static void handleMicTest() {
  neopixelSetPattern(5);
  servoSetHeadPanTarget(102);
  servoSetRightArmTarget(130);
  addCors();
  s_server.send(200, "application/json", "{\"ok\":true,\"event\":\"test_mic_reaction\"}");
}

static void startWebUi() {
  if (s_apUp) return;
  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(EVE_WEBUI_AP_SSID,
                        EVE_WEBUI_AP_PASS,
                        EVE_WEBUI_AP_CHANNEL,
                        false,
                        EVE_WEBUI_AP_MAX_CLIENTS);
  if (!s_started) {
    s_server.on("/", HTTP_GET, handleRoot);
    s_server.on("/api/status", HTTP_GET, handleStatus);
    s_server.on("/api/pose", HTTP_GET, handlePose);
    s_server.on("/api/pulse", HTTP_GET, handlePulse);
    s_server.on("/api/personality", HTTP_GET, handlePersonality);
    s_server.on("/api/interaction", HTTP_GET, handleInteraction);
    s_server.on("/api/control/pose", HTTP_GET, handleControlPose);
    s_server.on("/api/control/glow", HTTP_GET, handleControlGlow);
    s_server.on("/api/control/audio", HTTP_GET, handleControlAudio);
    s_server.on("/api/control/dock_mimic", HTTP_GET, handleControlDockMimic);
    s_server.on("/api/control/reset_state", HTTP_GET, handleControlResetState);
    s_server.on("/api/control/personality_profile", HTTP_GET, handlePersonalityProfile);
    s_server.on("/api/mic/status", HTTP_GET, handleMicStatus);
    s_server.on("/api/mic/settings", HTTP_GET, handleMicSettings);
    s_server.on("/api/mic/test", HTTP_GET, handleMicTest);
    s_server.begin();
    s_started = true;
  }
  s_apUp = ok;
  Serial.print(F("[EVE][WEB] AP "));
  Serial.print(ok ? F("up SSID=") : F("failed SSID="));
  Serial.print(EVE_WEBUI_AP_SSID);
  Serial.print(F(" IP="));
  Serial.println(WiFi.softAPIP());
  Serial.print(F("[EVE][WEB] state="));
  Serial.print(stateMachineGetStateName());
  Serial.print(F(" peer="));
  Serial.print(stateMachineGetPeerLabel());
  Serial.print(F(" session="));
  Serial.println((unsigned long)stateMachineGetSessionId());
}

static void stopWebUi() {
  if (!s_apUp) return;
  s_server.stop();
  s_started = false;
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  s_apUp = false;
  Serial.println(F("[EVE][WEB] AP stopped"));
}
#endif

void eveWebServerInit(void) {
#if EVE_ENABLE_DOCKED_WEBUI
  Serial.println(F("[EVE][WEB] docked WebUI ready"));
#endif
}

void eveWebServerTick(bool docked) {
#if EVE_ENABLE_DOCKED_WEBUI
  uint32_t now = millis();
  if (docked) {
    s_lastDockedMs = now;
  }
  const bool keepAlive = docked || (s_apUp && s_lastDockedMs != 0 &&
                                    (uint32_t)(now - s_lastDockedMs) < EVE_DOCKED_WEBUI_OFF_GRACE_MS);
  if (keepAlive) {
    startWebUi();
    s_server.handleClient();
#if EVE_SERIAL_VERBOSE_LOGS
    if ((uint32_t)(now - s_lastLogMs) > 15000u) {
      s_lastLogMs = now;
      Serial.print(F("[EVE][WEB] active http://"));
      Serial.println(WiFi.softAPIP());
    }
#endif
  } else {
#if EVE_SERIAL_VERBOSE_LOGS
    if (!s_apUp && (uint32_t)(now - s_lastOffLogMs) > 5000u) {
      s_lastOffLogMs = now;
      Serial.print(F("[EVE][WEB] AP off; state="));
      Serial.print(stateMachineGetStateName());
      Serial.print(F(" peer="));
      Serial.print(stateMachineGetPeerLabel());
      Serial.print(F(" session="));
      Serial.println((unsigned long)stateMachineGetSessionId());
    }
#endif
    stopWebUi();
  }
#else
  (void)docked;
#endif
}

