// include/FlyBug.h — 惱人的飛蟲（含位置查詢）
#pragma once
#include <Arduino.h>
#include <LovyanGFX.hpp>

namespace FlyBug {

// ════════════════ 可調參數 ════════════════
constexpr int   MAX_FLIES        = 3;
constexpr uint32_t SPAWN_MIN_MS  = 2000;
constexpr uint32_t SPAWN_MAX_MS  = 6000;
constexpr uint32_t LIFE_MIN_MS   = 5000;
constexpr uint32_t LIFE_MAX_MS   = 12000;

// ════════════════ 飛蟲結構 ════════════════
struct Fly {
  float    angle;
  float    radiusX;
  float    radiusY;
  float    speed;
  float    wobble;
  float    wingPhase;
  float    x, y;
  uint32_t bornAt;
  uint32_t life;
  bool     active;
};

static Fly _flies[MAX_FLIES];
static uint32_t _nextSpawn = 0;
static int _pigCx = 160, _pigCy = 108, _pigR = 72;
static bool _inited = false;

// ════════════════ 內部：隨機生成一隻蟲 ════════════════
inline void _spawnFly(Fly& f, uint32_t now) {
  f.angle    = random(0, 628) / 100.0f;
  f.radiusX  = _pigR + random(5, 25);
  f.radiusY  = _pigR * 0.7f + random(-5, 15);
  f.speed    = (random(30, 80) / 100.0f) * (random(0, 2) ? 1.0f : -1.0f);
  f.wobble   = random(0, 628) / 100.0f;
  f.wingPhase = random(0, 628) / 100.0f;
  f.life     = random(LIFE_MIN_MS, LIFE_MAX_MS);
  f.bornAt   = now;
  f.active   = true;
}

// ════════════════ 啟動 ════════════════
inline void begin(int pigCx, int pigCy, int pigR) {
  _pigCx = pigCx;
  _pigCy = pigCy;
  _pigR  = pigR;
  for (int i = 0; i < MAX_FLIES; i++) _flies[i].active = false;
  _nextSpawn = millis() + random(1500, 3000);
  _inited = true;
  Serial.println("[🐝] FlyBug started");
}

// ════════════════ 更新位置 ════════════════
inline void update(uint32_t now) {
  if (!_inited) return;

  if (now >= _nextSpawn) {
    for (int i = 0; i < MAX_FLIES; i++) {
      if (!_flies[i].active) {
        _spawnFly(_flies[i], now);
        _nextSpawn = now + random(SPAWN_MIN_MS, SPAWN_MAX_MS);
        Serial.printf("[🐝] New fly #%d\n", i);
        break;
      }
    }
  }

  for (int i = 0; i < MAX_FLIES; i++) {
    Fly& f = _flies[i];
    if (!f.active) continue;

    if (now - f.bornAt > f.life) {
      f.active = false;
      continue;
    }

    f.angle  += f.speed * 0.03f;
    f.wobble += 0.3f;
    f.wingPhase += 0.9f;

    float wobbleY = sinf(f.wobble) * 3.0f;
    f.x = _pigCx + cosf(f.angle) * f.radiusX;
    f.y = _pigCy + sinf(f.angle) * f.radiusY + wobbleY;
  }
}

// ════════════════ 查詢 ════════════════
inline int activeCount() {
  int n = 0;
  for (int i = 0; i < MAX_FLIES; i++) if (_flies[i].active) n++;
  return n;
}

// ★ 取得最近飛蟲的位置
inline bool getNearestFly(int px, int py, int& outX, int& outY) {
  int bestDist = 999999;
  bool found = false;
  for (int i = 0; i < MAX_FLIES; i++) {
    if (!_flies[i].active) continue;
    int dx = (int)_flies[i].x - px;
    int dy = (int)_flies[i].y - py;
    int d2 = dx * dx + dy * dy;
    if (d2 < bestDist) {
      bestDist = d2;
      outX = (int)_flies[i].x;
      outY = (int)_flies[i].y;
      found = true;
    }
  }
  return found;
}

// ════════════════ 畫單隻蟲 ════════════════
inline void _drawOneFly(LovyanGFX& g, const Fly& f) {
  int x = (int)f.x;
  int y = (int)f.y;

  // 影子
  g.fillEllipse(x, y + 5, 3, 1, 0x1082);

  // 翅膀
  float wingOffset = sinf(f.wingPhase) * 2.0f;
  uint16_t wingCol = 0xCE79;
  g.fillEllipse(x - 3 - (int)wingOffset, y - 2, 2, 1, wingCol);
  g.fillEllipse(x + 3 + (int)wingOffset, y - 2, 2, 1, wingCol);

  // 身體
  g.fillEllipse(x, y, 2, 1, 0x0000);
  g.fillEllipse(x, y + 1, 2, 1, 0x0000);
  g.drawPixel(x - 1, y - 1, 0x8410);
  g.drawPixel(x + 1, y - 1, 0x8410);

  // 頭
  g.fillCircle(x, y - 2, 1, 0x0000);

  // 紅眼
  uint16_t eyeCol = ((millis() / 200) % 2) ? 0xF800 : 0xA000;
  g.drawPixel(x - 1, y - 2, eyeCol);
  g.drawPixel(x + 1, y - 2, eyeCol);

  // 抖動尾跡
  if ((millis() / 100 + (int)f.angle) % 3 == 0) {
    g.drawPixel(x + (random(-3, 4)), y + (random(-3, 4)), 0x7BEF);
  }
}

// ════════════════ 主繪製 ════════════════
inline void draw(LovyanGFX& g) {
  if (!_inited) return;
  for (int i = 0; i < MAX_FLIES; i++) {
    if (_flies[i].active) _drawOneFly(g, _flies[i]);
  }
}

inline void clear() {
  for (int i = 0; i < MAX_FLIES; i++) _flies[i].active = false;
  _nextSpawn = millis() + random(1500, 3000);
}

} // namespace FlyBug