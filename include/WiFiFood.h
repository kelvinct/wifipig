// include/WiFiFood.h — 乾淨版（不重疊）
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <LovyanGFX.hpp>

namespace WiFiFood {

// ════════════════ SSID 清單 ════════════════
const char* POO_SSIDS[] = {
  "Pile of Poo 💩", "Free Poo WiFi", "PoopNet-2.4G",
  "💩 Hotspot 💩", "Smelly WiFi", "NotAPoo",
  "Poop Guest", "💩💩💩"
};
const int POO_COUNT = sizeof(POO_SSIDS) / sizeof(POO_SSIDS[0]);
const uint32_t POO_SWITCH_MS = 5000;
const char* TOILET_SSID  = "🚽 In the loo...";
const char* TOILET_SSID2 = "🚽 BRB 💩";

// ════════════════ 狀態 ════════════════
enum Phase { PHASE_SNIFFING, PHASE_EATING, PHASE_DIGESTING, PHASE_TOILET, PHASE_RESTING };
enum Food { FOOD_NONE, FOOD_APPLE, FOOD_CARROT, FOOD_DONUT, FOOD_CAKE, FOOD_FEAST };
enum MoodId { MOOD_NEUTRAL, MOOD_HAPPY, MOOD_SAD };

constexpr uint32_t EAT_DURATION    = 3000;
constexpr uint32_t DIGEST_DURATION = 4000;
constexpr uint32_t TOILET_DURATION = 4000;
constexpr uint32_t REST_DURATION   = 8000;
constexpr uint32_t SCAN_INTERVAL   = 3000;

static Phase    _phase      = PHASE_SNIFFING;
static Food     _food       = FOOD_NONE;
static uint32_t _phaseStart = 0;
static uint32_t _lastScan   = 0;
static int      _networks   = 0;
static int      _strongest  = -100;
static int      _bars       = 0;
static bool     _scanning   = false;
static int      _pooIndex   = 0;
static uint32_t _lastPooSwitch = 0;

typedef void (*MoodCallback)(int moodId, unsigned long duration);
static MoodCallback _moodCb = nullptr;
inline void setMoodCallback(MoodCallback cb) { _moodCb = cb; }
inline void _notifyMood(int id, unsigned long dur) { if (_moodCb) _moodCb(id, dur); }

inline Phase getPhase()    { return _phase; }
inline Food  getFood()     { return _food; }
inline int   getNetworks() { return _networks; }
inline int   getBars()     { return _bars; }
inline bool  isScanning()  { return _scanning; }
inline bool  isInToilet()  { return _phase == PHASE_TOILET; }
inline const char* getPooSSID() {
  if (_phase == PHASE_TOILET)
    return ((millis() / 800) % 2) ? TOILET_SSID : TOILET_SSID2;
  if (_pooIndex < 0) return POO_SSIDS[0];
  return POO_SSIDS[_pooIndex];
}

inline int _rssiToBars(int rssi) {
  if (rssi > -55) return 4;
  if (rssi > -65) return 3;
  if (rssi > -75) return 2;
  if (rssi > -85) return 1;
  return 0;
}

inline void _pickFood() {
  if (_networks == 0)       _food = FOOD_NONE;
  else if (_networks <= 3)  _food = FOOD_CARROT;
  else if (_networks <= 6)  _food = FOOD_DONUT;
  else if (_networks <= 10) _food = FOOD_CAKE;
  else                      _food = FOOD_FEAST;
}

inline void begin() {
  WiFi.mode(WIFI_AP_STA);
  delay(100);
  WiFi.softAP(POO_SSIDS[0], nullptr, 1, 0, 4);
  Serial.printf("[💩] AP: \"%s\"  IP=%s\n",
                POO_SSIDS[0], WiFi.softAPIP().toString().c_str());
  _pooIndex = 0;
  _lastPooSwitch = millis();
  _phase = PHASE_SNIFFING;
  _phaseStart = millis();
}

inline void _updatePooAP() {
  uint32_t now = millis();

  if (_phase == PHASE_TOILET) {
    static uint32_t lastToiletSwitch = 0;
    if (now - lastToiletSwitch < 800) return;
    lastToiletSwitch = now;
    const char* ssid = ((now / 800) % 2) ? TOILET_SSID : TOILET_SSID2;
    WiFi.softAPdisconnect(false);
    delay(20);
    WiFi.softAP(ssid, nullptr, 1, 0, 4);
    _pooIndex = -1;
    return;
  }

  if (_pooIndex < 0) {
    _pooIndex = 0;
    WiFi.softAPdisconnect(false);
    delay(20);
    WiFi.softAP(POO_SSIDS[0], nullptr, 1, 0, 4);
    _lastPooSwitch = now;
    return;
  }

  if (now - _lastPooSwitch < POO_SWITCH_MS) return;
  _lastPooSwitch = now;
  _pooIndex = (_pooIndex + 1) % POO_COUNT;
  WiFi.softAPdisconnect(false);
  delay(30);
  WiFi.softAP(POO_SSIDS[_pooIndex], nullptr, 1, 0, 4);
}

inline void update() {
  uint32_t now = millis();
  uint32_t elapsed = now - _phaseStart;
  _updatePooAP();

  switch (_phase) {
    case PHASE_SNIFFING:
      if (_scanning) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
          _scanning = false;
          _networks = n;
          _strongest = -100;
          for (int i = 0; i < n; i++) {
            int r = WiFi.RSSI(i);
            if (r > _strongest) _strongest = r;
          }
          _bars = _rssiToBars(_strongest);
          WiFi.scanDelete();
          if (_networks > 0) {
            _pickFood();
            _phase = PHASE_EATING;
            _phaseStart = now;
            _notifyMood(MOOD_HAPPY, EAT_DURATION);
          } else {
            _notifyMood(MOOD_SAD, 2000);
            _phaseStart = now;
          }
        } else if (n == WIFI_SCAN_FAILED) {
          _scanning = false;
        }
      } else if (now - _lastScan > SCAN_INTERVAL) {
        _lastScan = now;
        WiFi.scanNetworks(true, false);
        _scanning = true;
      }
      break;

    case PHASE_EATING:
      if (elapsed >= EAT_DURATION) {
        _phase = PHASE_DIGESTING;
        _phaseStart = now;
      }
      break;

    case PHASE_DIGESTING:
      if (elapsed >= DIGEST_DURATION) {
        _phase = PHASE_TOILET;
        _phaseStart = now;
        _lastPooSwitch = 0;
      }
      break;

    case PHASE_TOILET:
      if (elapsed >= TOILET_DURATION) {
        _phase = PHASE_RESTING;
        _phaseStart = now;
        _networks = 0;
        _bars = 0;
      }
      break;

    case PHASE_RESTING:
      if (elapsed >= REST_DURATION) {
        _phase = PHASE_SNIFFING;
        _phaseStart = now;
        _lastScan = now;
        _notifyMood(MOOD_NEUTRAL, 0);
      }
      break;
  }
}

// ════════════════ 便便圖示 ════════════════
inline void drawPoo(LovyanGFX& g, int cx, int cy, int bars, int s = 1) {
  const uint16_t brown    = 0x8200;
  const uint16_t brownMid = 0xA2C6;
  const uint16_t brownHi  = 0xCBA7;
  const uint16_t arcOn    = 0x07E0;
  const uint16_t arcOff   = 0x4208;

  for (int i = 0; i < 4; i++) {
    int r = (16 + i * 5) * s;
    uint16_t c = (i < bars) ? arcOn : arcOff;
    int th = s > 1 ? 3 : 2;
    g.drawArc(cx, cy, r, r - th, 150, 210, c);
    g.drawArc(cx, cy, r, r - th, -30, 30, c);
  }

  g.fillEllipse(cx, cy + 9*s, 14*s, 5*s, brown);
  g.fillEllipse(cx - 2*s, cy + 7*s, 10*s, 3*s, brownMid);
  g.fillEllipse(cx, cy + 2*s, 12*s, 5*s, brown);
  g.fillEllipse(cx - 2*s, cy, 9*s, 3*s, brownMid);
  g.fillEllipse(cx, cy - 5*s, 8*s, 4*s, brown);
  g.fillEllipse(cx - 1*s, cy - 6*s, 6*s, 2*s, brownMid);
  g.fillCircle(cx, cy - 10*s, 2*s, brown);
  g.fillCircle(cx, cy - 11*s, 1*s, brownHi);

  g.fillCircle(cx - 4*s, cy - 1*s, 2, TFT_WHITE);
  g.fillCircle(cx + 4*s, cy - 1*s, 2, TFT_WHITE);
  g.fillCircle(cx - 4*s, cy, 1, TFT_BLACK);
  g.fillCircle(cx + 4*s, cy, 1, TFT_BLACK);
  if (bars >= 1) {
    g.drawArc(cx, cy + 3*s, 3*s, 2*s, 0, 180, TFT_BLACK);
  } else {
    g.drawArc(cx, cy + 5*s, 3*s, 2*s, 180, 360, TFT_BLACK);
  }
}

// ════════════════ ★ 新設計：頂部狀態膠囊 ════════════════
inline void _drawStatusCapsule(LovyanGFX& g) {
  // ─── 主膠囊 ───
  int x = 8, y = 6, w = 190, h = 22;
  g.fillRoundRect(x, y, w, h, 8, 0x2124);
  g.drawRoundRect(x, y, w, h, 8, 0x5D9B);

  // 左：便便圖示（小）
  drawPoo(g, x + 14, y + 11, _bars, 1);

  // 中：SSID 或狀態
  g.setTextDatum(ML_DATUM);
  g.setTextSize(1);

  const char* ssid = POO_SSIDS[_pooIndex < 0 ? 0 : _pooIndex];
  if (_phase == PHASE_TOILET) ssid = "IN THE LOO...";

  char buf[32];
  snprintf(buf, sizeof(buf), "%s", ssid);
  if (strlen(buf) > 18) buf[18] = 0;

  g.setTextColor(0xCBA7, 0x2124);
  g.drawString(buf, x + 30, y + h / 2);

  // 右：狀態標籤
  const char* state = "";
  uint16_t stateCol = 0xCBA7;

  switch (_phase) {
    case PHASE_SNIFFING:  state = _scanning ? "SCAN" : "HUNGRY"; stateCol = _scanning ? 0xFFE0 : 0xF800; break;
    case PHASE_EATING:    state = "EAT";   stateCol = 0x07E0; break;
    case PHASE_DIGESTING: state = "BURP";  stateCol = 0xFFE0; break;
    case PHASE_TOILET:    state = "🚽";    stateCol = 0xFE00; break;
    case PHASE_RESTING:   state = "FULL";  stateCol = 0x5D9B; break;
  }

  g.setTextDatum(MR_DATUM);
  g.setTextColor(stateCol, 0x2124);
  g.drawString(state, x + w - 8, y + h / 2);
}

// ════════════════ 進度條（頂欄中央）════════════════
inline void _drawTopProgress(LovyanGFX& g) {
  int x = 202, y = 6, w = 24, h = 22;

  // 垂直進度條背景
  g.fillRoundRect(x, y, w, h, 6, 0x2124);
  g.drawRoundRect(x, y, w, h, 6, 0x5D9B);

  int fillH = 0;
  uint16_t fc = 0x07E0;
  uint32_t now = millis();

  switch (_phase) {
    case PHASE_EATING: {
      uint32_t e = now - _phaseStart;
      fillH = ((h - 4) * e) / EAT_DURATION;
      fc = 0x07E0;
      break;
    }
    case PHASE_DIGESTING: {
      uint32_t e = now - _phaseStart;
      fillH = (h - 4) - ((h - 4) * e) / DIGEST_DURATION;
      fc = 0xFFE0;
      break;
    }
    case PHASE_TOILET: {
      uint32_t e = now - _phaseStart;
      fillH = ((h - 4) * e) / TOILET_DURATION;
      fc = 0xFE00;
      break;
    }
    case PHASE_SNIFFING:
      fillH = _scanning ? (h - 4) / 4 : 0;
      fc = 0xFFE0;
      break;
    case PHASE_RESTING:
      fillH = 0;
      break;
  }

  if (fillH > 0) {
    // 從底部往上填充
    g.fillRoundRect(x + 2, y + 2 + (h - 4 - fillH), w - 4, fillH, 4, fc);
  }
}

// ════════════════ ★ 主繪製 ════════════════
inline void draw(LovyanGFX& g, int pigCx, int pigCy, int pigR, uint16_t bg) {
  uint32_t now = millis();

  // ─── 廁所模式：全螢幕覆蓋 ───
  if (_phase == PHASE_TOILET) {
    uint32_t elapsed = now - _phaseStart;
    float t = constrain((float)elapsed / TOILET_DURATION, 0.0f, 1.0f);

    g.fillScreen(0x0000);

    // 地板
    g.fillRect(0, 170, g.width(), 70, 0x18E3);
    g.drawLine(0, 170, g.width(), 170, 0x5D9B);

    // 門
    int doorX = pigCx - 50, doorY = 40;
    g.fillRoundRect(doorX, doorY, 100, 100, 6, 0x4208);
    g.drawRoundRect(doorX, doorY, 100, 100, 6, 0x8410);
    g.fillCircle(doorX + 92, doorY + 50, 4, 0xFFE0);
    // 門牌 WC
    g.fillRoundRect(doorX + 36, doorY + 8, 28, 18, 3, 0x07FF);
    g.setTextDatum(MC_DATUM);
    g.setTextColor(0xFFFF, 0x07FF);
    g.setTextSize(1);
    g.drawString("WC", doorX + 50, doorY + 17);

    // 馬桶
    int baseY = 180;
    g.fillEllipse(pigCx, baseY, 36, 10, 0xFFFF);
    g.drawEllipse(pigCx, baseY, 36, 10, 0xC618);
    g.fillEllipse(pigCx, baseY - 12, 30, 16, 0xFFFF);
    g.drawEllipse(pigCx, baseY - 12, 30, 16, 0xC618);
    g.fillEllipse(pigCx, baseY - 14, 20, 10, 0x07FF);
    g.fillEllipse(pigCx, baseY - 14, 18, 8, 0x5D9B);

    // 水波
    int ripple = (int)((now / 100) % 16);
    g.drawEllipse(pigCx, baseY - 14, 10 + ripple / 2, 4 + ripple / 4, 0x07FF);

    // 豬屁股
    int pigY = baseY - 32;
    uint16_t pigCol = 0xFC9F;
    if (t > 0.3f && t < 0.7f) pigCol = 0xFC00;   // 用力變紅
    g.fillEllipse(pigCx, pigY, 20, 16, pigCol);
    g.fillEllipse(pigCx - 2, pigY - 3, 12, 7, 0xD145);
    // 腿
    g.fillRect(pigCx - 18, pigY + 8, 6, 8, 0xFC9F);
    g.fillRect(pigCx + 12, pigY + 8, 6, 8, 0xFC9F);
    // 尾巴
    g.drawCircle(pigCx + 22, pigY - 2, 4, 0xD145);
    g.drawCircle(pigCx + 25, pigY - 5, 3, 0xD145);
    // 耳朵
    g.fillTriangle(pigCx - 16, pigY - 12, pigCx - 8, pigY - 12, pigCx - 12, pigY - 24, 0xFC9F);
    g.fillTriangle(pigCx + 8, pigY - 12, pigCx + 16, pigY - 12, pigCx + 12, pigY - 24, 0xFC9F);

    // 汗滴
    if (t > 0.3f && t < 0.7f) {
      g.fillCircle(pigCx - 22, pigY - 8, 2, 0x5D9B);
      g.fillCircle(pigCx + 22, pigY - 10, 2, 0x5D9B);
    }

    // 投彈
    if (t > 0.4f && t < 0.75f) {
      float dropT = (t - 0.4f) / 0.35f;
      int poopY = pigY + 14 + (int)(dropT * 24);
      g.fillCircle(pigCx, poopY, 4 - (int)(dropT * 2), 0x8200);
    }

    // 水花
    if (t > 0.7f && t < 0.85f) {
      float sT = (t - 0.7f) / 0.15f;
      for (int i = 0; i < 6; i++) {
        float a = i * 60 * DEG_TO_RAD;
        int sx = pigCx + (int)(cos(a) * sT * 28);
        int sy = baseY - 14 + (int)(sin(a) * sT * 12);
        g.fillCircle(sx, sy, 2, 0x07FF);
      }
    }

    // 沖水
    if (t > 0.8f) {
      float sT = (t - 0.8f) / 0.2f;
      for (int i = 0; i < 8; i++) {
        float a = (i * 45 + sT * 720) * DEG_TO_RAD;
        int r = 16 - (int)(sT * 10);
        g.drawPixel(pigCx + (int)(cos(a) * r),
                    baseY - 14 + (int)(sin(a) * r * 0.5f), 0x07FF);
      }
    }

    // 文字
    g.setTextDatum(TC_DATUM);
    uint16_t tc = ((now / 300) % 2) ? 0xFE00 : 0xFFE0;
    g.setTextColor(tc, 0x0000);
    g.setTextSize(2);
    g.drawString("IN THE LOO", g.width() / 2, 8);
    g.setTextSize(1);
    g.setTextColor(0xCBA7, 0x0000);
    g.drawString("BRB 💩", g.width() / 2, 28);

    // 底部進度
    int bw = 120, bx = (g.width() - bw) / 2, by = g.height() - 16;
    g.drawRoundRect(bx, by, bw, 8, 3, 0x5D9B);
    int fw = (int)((bw - 4) * t);
    if (fw > 0) {
      uint16_t c = (t < 0.8f) ? 0xFE00 : 0x07E0;
      g.fillRoundRect(bx + 2, by + 2, fw, 4, 2, c);
    }
    return;
  }

  // ─── 正常模式 ───
  // 頂欄：狀態膠囊 + 垂直進度 + 切換按鈕
  _drawStatusCapsule(g);
  _drawTopProgress(g);

  // 主區：動畫
  switch (_phase) {
    case PHASE_SNIFFING: {
      if (!_scanning) break;
      int cx = g.width() / 2, cy = 40;
      for (int i = 0; i < 3; i++) {
        uint32_t ph = (now + i * 200) % 600;
        int r = 8 + ph / 12;
        int a = 255 - ph / 3;
        if (a < 30) continue;
        g.drawCircle(cx, cy, r, 0xFFE0);
      }
      float ang = (now / 200.0f) * DEG_TO_RAD;
      g.drawLine(cx, cy, cx + cos(ang) * 25, cy + sin(ang) * 25, 0xFFE0);
      g.fillCircle(cx, cy, 3, 0x07FF);
      g.setTextDatum(MC_DATUM);
      g.setTextColor(0xFFE0, bg);
      g.setTextSize(1);
      g.drawString("Sniffing WiFi...", cx, cy + 22);
      break;
    }

    case PHASE_EATING: {
      uint32_t e = now - _phaseStart;
      float t = constrain((float)e / EAT_DURATION, 0.0f, 1.0f);
      int sx = pigCx + pigR + 10, sy = pigCy - 10;
      int mx = pigCx + 20,       my = pigCy + pigR - 20;
      int fx = sx + (int)((mx - sx) * t);
      int fy = sy + (int)((my - sy) * t);
      float sc = 1.0f - t * 0.6f;
      if (sc > 0.15f) {
        // 簡易食物圖示
        g.fillCircle(fx, fy, (int)(10 * sc), 0xF800);
        g.fillCircle(fx - 2, fy - 2, (int)(4 * sc), 0xFC00);
        g.fillTriangle(fx, fy - 10 * sc, fx - 4 * sc, fy - 14 * sc,
                       fx + 4 * sc, fy - 12 * sc, 0x07E0);
      }
      int chewY = pigCy + pigR - 25;
      int op = (int)(fabs(sin(now / 80.0)) * 5) + 2;
      g.fillEllipse(pigCx, chewY, 10, op, 0x0000);
      break;
    }

    case PHASE_DIGESTING: {
      uint32_t e = now - _phaseStart;
      for (int i = 0; i < 3; i++) {
        uint32_t bs = i * 1200;
        if (e < bs) continue;
        uint32_t ba = e - bs;
        if (ba > 1500) continue;
        float t = ba / 1500.0f;
        int bx = pigCx + 30 + (int)(t * 25);
        int by = pigCy + 20 - (int)(t * 60);
        int br = 3 + (int)(t * 3);
        g.drawCircle(bx, by, br, 0xFFFF);
      }
      g.setTextDatum(MC_DATUM);
      g.setTextColor(0xFFE0, bg);
      g.setTextSize(2);
      float w = sin(now / 150.0) * 2;
      g.drawString("BURP~", pigCx + w, 182);
      break;
    }

    case PHASE_RESTING: {
      int by = pigCy + pigR - 15 + (int)(sin(now / 500.0) * 2);
      g.fillCircle(pigCx, by, 18, 0xFC9F);
      g.drawCircle(pigCx, by, 18, 0xD145);
      g.setTextDatum(MC_DATUM);
      g.setTextColor(0xFFFF, 0xFC9F);
      g.setTextSize(2);
      g.drawString(":)", pigCx, by);
      break;
    }
  }
}

} // namespace WiFiFood