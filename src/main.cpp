// ESP32-S3-Touch-LCD-2  大臉小豬 v4 — 智能寵物版
// 含：情緒+3D+便便WiFi+飛蟲+晝夜+火焰+智能寵物
#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include <QMI8658.h>
#include <LovyanGFX.hpp>
#include "WiFiFood.h"
#include "FlyBug.h"

// ============================================================
// LovyanGFX 硬體配置
// ============================================================
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  lgfx::Light_PWM     _light_instance;
  lgfx::Touch_CST816S _touch_instance;
public:
  LGFX() {
    { auto cfg = _bus_instance.config();
      cfg.spi_host = SPI2_HOST; cfg.spi_mode = 0;
      cfg.freq_write = 40000000; cfg.freq_read = 8000000;
      cfg.spi_3wire = false; cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 39; cfg.pin_mosi = 38; cfg.pin_miso = -1; cfg.pin_dc = 42;
      _bus_instance.config(cfg); _panel_instance.setBus(&_bus_instance); }
    { auto cfg = _panel_instance.config();
      cfg.pin_cs = 45; cfg.pin_rst = -1; cfg.pin_busy = -1;
      cfg.memory_width = 240; cfg.memory_height = 320;
      cfg.panel_width = 240; cfg.panel_height = 320;
      cfg.invert = true; cfg.rgb_order = false;
      _panel_instance.config(cfg); }
    { auto cfg = _light_instance.config();
      cfg.pin_bl = 1; cfg.freq = 44100; cfg.pwm_channel = 7;
      _light_instance.config(cfg); _panel_instance.setLight(&_light_instance); }
    { auto cfg = _touch_instance.config();
      cfg.i2c_addr = 0x15; cfg.i2c_port = 0;
      cfg.pin_sda = 48; cfg.pin_scl = 47;
      cfg.pin_int = -1; cfg.pin_rst = -1;
      cfg.freq = 400000; cfg.x_max = 240; cfg.y_max = 320;
      _touch_instance.config(cfg); _panel_instance.setTouch(&_touch_instance); }
    setPanel(&_panel_instance);
  }
};

LGFX tft;
LGFX_Sprite canvas(&tft);

// ============================================================
// IMU
// ============================================================
QMI8658 imu;
bool imuOK = false;
float ax = 0, ay = 0, az = 1;
float gx = 0, gy = 0, gz = 0;
float rotX = 0, rotY = 0;
unsigned long lastImuTime = 0;

int currentView = 0;

// ============================================================
// 🌅 晝夜系統
// ============================================================
#define TIME_WIFI_SSID ""
#define TIME_WIFI_PASS ""

bool ntpOK = false;
int  hourNow = 8;
bool isDay = true;
uint32_t bootMillis = 0;
uint32_t lastTimeCheck = 0;
uint32_t lastNtpTry = 0;

// ============================================================
// 🧠 智能寵物狀態
// ============================================================
float loveMeter = 0;
uint32_t lastInteraction = 0;
uint32_t nextIdleAction = 0;
uint32_t welcomeUntil = 0;
bool     welcomeMode = false;
int16_t  fingerX = 0, fingerY = 0;
bool     fingerTracking = false;
uint32_t fingerTrackEnd = 0;
uint32_t wakeRageUntil = 0;

const char* loveLevel() {
  if (loveMeter >= 80) return "SOULMATE";
  if (loveMeter >= 50) return "FRIEND";
  if (loveMeter >= 20) return "BUDDY";
  if (loveMeter >= 5)  return "STRANGER";
  return "WARY";
}

// ============================================================
// GUI 佈局
// ============================================================
#define VIEW_BTN_X  232
#define VIEW_BTN_Y  6
#define VIEW_BTN_W  80
#define VIEW_BTN_H  22

#define COLD_BTN_X  8
#define COLD_BTN_Y  198
#define COLD_BTN_W  152
#define COLD_BTN_H  36

#define HEAT_BTN_X  160
#define HEAT_BTN_Y  198
#define HEAT_BTN_W  152
#define HEAT_BTN_H  36

#define EFFECT_TEXT_Y 182

// ============================================================
// 情緒
// ============================================================
enum EyeMood {
  EMO_NEUTRAL, EMO_HAPPY, EMO_SAD, EMO_ANGRY,
  EMO_SLEEPING, EMO_THINKING, EMO_LOVE, EMO_SURPRISED,
  EMO_OK, EMO_FOCUS, EMO_SQUINT, EMO_HUNGRY, EMO_SICK,
  EMO_DIZZY, EMO_COLD, EMO_HEAT,
  EMO_ANNOYED,
  EMO_SHY,
  EMO_PROUD
};

int moodPriority(EyeMood m) {
  switch (m) {
    case EMO_SLEEPING: return 10;
    case EMO_DIZZY:    return 9;
    case EMO_COLD:     return 8;
    case EMO_HEAT:     return 8;
    case EMO_ANNOYED:  return 6;
    case EMO_PROUD:    return 5;
    case EMO_HAPPY:    return 4;
    case EMO_LOVE:     return 4;
    case EMO_SHY:      return 3;
    case EMO_SURPRISED:return 3;
    case EMO_ANGRY:    return 3;
    case EMO_SAD:      return 3;
    case EMO_THINKING: return 2;
    case EMO_SQUINT:   return 2;
    case EMO_SICK:     return 2;
    case EMO_HUNGRY:   return 2;
    case EMO_OK:       return 2;
    case EMO_FOCUS:    return 2;
    default:           return 1;
  }
}

const char* moodName(EyeMood m) {
  const char* names[] = {
    "NEUTRAL","HAPPY","SAD","ANGRY","SLEEPING","THINKING","LOVE","SURPRISED",
    "OK","FOCUS","SQUINT","HUNGRY","SICK","DIZZY","COLD","HEAT","ANNOYED",
    "SHY","PROUD"
  };
  if (m <= EMO_PROUD) return names[m];
  return "?";
}

#define PIG_PINK        0xFC9F
#define PIG_PINK_DARK   0xD145
#define PIG_SNOUT       0xF7BE
#define PIG_NOSE_HOLE   0x8000
#define PIG_BLUSH       0xFB4F
#define PIG_EYE_WHITE   0xFFFF
#define PIG_EYE_BLACK   0x0000

#define MAX_PARTICLES 20
struct Particle {
  float x, y, vx, vy;
  int life, maxLife;
  uint16_t color;
  bool active;
};

// ★ 火焰粒子
#define MAX_FIRE 60
struct FireP {
  float x, y, vx, vy;
  int life;
  bool active;
};
static FireP fireP[MAX_FIRE];

struct PigBody { int cx, cy, r; float breathe = 0, bounce = 0; };
struct Eye {
  int cx, cy, w, h;
  float lidOpen = 1.0f, targetLid = 1.0f;
  float pupilScale = 1.0f, targetPupilScale = 1.0f;
};

struct EyeSystem {
  PigBody pig;
  Eye left, right;
  EyeMood mood = EMO_NEUTRAL;
  unsigned long moodEnd = 0;
  uint16_t color = PIG_PINK, targetColor = PIG_PINK;
  float lookX = 0, lookY = 0, targetLookX = 0, targetLookY = 0;
  bool blinking = false;
  unsigned long blinkStart = 0, nextBlink = 0;
  uint16_t bgColor = TFT_BLACK;
  bool initialized = false;
  unsigned long okStart = 0;
  bool okActive = false;
  unsigned long lastTypingMs = 0;
  bool typingActive = false;
  unsigned long touchStart = 0;
  int16_t touchX = 0, touchY = 0;
  bool touching = false;
  unsigned long lastTapMs = 0;
  float breathPhase = 0;
  Particle particles[MAX_PARTICLES];
  EyeMood lastMood = EMO_NEUTRAL;
};

EyeSystem eyes;

// ============================================================
// 習慣養成
// ============================================================
struct Habits {
  int fliesKilled = 0;
  int wifiMeals = 0;
  int shakes = 0;
  int touches = 0;
  int coldsUsed = 0;
  int heatsUsed = 0;
  uint32_t sessionStart = 0;
};
static Habits habits;

static bool eggFlies10 = false;
static bool eggFlies25 = false;
static bool eggFlies50 = false;
static bool eggWifi10 = false;

// ============================================================
// 前置宣告
// ============================================================
void setMood(EyeMood m, unsigned long durationMs = 0);
void setMoodForced(EyeMood m, unsigned long durationMs = 0);
bool hitViewButton(int16_t x, int16_t y);
bool hitColdButton(int16_t x, int16_t y);
bool hitHeatButton(int16_t x, int16_t y);
void drawViewButton(LovyanGFX& gfx);
void drawBottomButtons(LovyanGFX& gfx);
void drawEyes(LovyanGFX& gfx);
void draw3DView(LovyanGFX& gfx);
void draw3DPigView(LovyanGFX& gfx);

// ============================================================
// 邏輯系統
// ============================================================
enum DeskMood { MOOD_AWAKE, MOOD_SLEEPY, MOOD_ASLEEP };
const uint32_t AWAKE_MS     = 20000;
const uint32_t SLEEPY_MS    = 40000;
const float    MOTION_GYRO  = 0.35f;
const float    FLICK_THRESH = 2.0f;

DeskMood  currentMood  = MOOD_AWAKE;
uint32_t  lastMotion   = 0;
float     prevGmag     = 0;
int       peakCount    = 0;
uint32_t  windowStart  = 0;
uint32_t  lastPeak     = 0;
uint32_t  lastFlick    = 0;
uint32_t  dizzyUntil   = 0;

const EyeMood exprCycle[] = {
  EMO_NEUTRAL, EMO_HAPPY, EMO_LOVE, EMO_ANGRY,
  EMO_SURPRISED, EMO_THINKING, EMO_SAD, EMO_DIZZY
};
const int EXPR_COUNT = sizeof(exprCycle) / sizeof(exprCycle[0]);
int exprIndex = 0;

static uint32_t flyLockUntil = 0;
static float    flyLockX = 0, flyLockY = 0;
static float    headSmoothX = 0, headSmoothY = 0;
static float    flyDistance = 999;

static uint32_t yayUntil = 0;
static uint32_t jumpUntil = 0;

// ============================================================
// 顏色工具
// ============================================================
uint16_t lerpColor(uint16_t c1, uint16_t c2, float t) {
  if (t <= 0) return c1;
  if (t >= 1) return c2;
  int r1 = (c1 >> 11) & 0x1F, g1 = (c1 >> 5) & 0x3F, b1 = c1 & 0x1F;
  int r2 = (c2 >> 11) & 0x1F, g2 = (c2 >> 5) & 0x3F, b2 = c2 & 0x1F;
  return (((int)(r1 + (r2 - r1) * t)) << 11) |
         (((int)(g1 + (g2 - g1) * t)) << 5)  |
         ((int)(b1 + (b2 - b1) * t));
}

uint16_t hsvToRGB565(float h, float s, float v) {
  int i = (int)(h / 60) % 6;
  float f = h / 60 - (int)(h / 60);
  float p = v * (1 - s), q = v * (1 - s * f), t = v * (1 - s * (1 - f));
  float r, g, b;
  switch (i) {
    case 0: r=v; g=t; b=p; break;
    case 1: r=q; g=v; b=p; break;
    case 2: r=p; g=v; b=t; break;
    case 3: r=p; g=q; b=v; break;
    case 4: r=t; g=p; b=v; break;
    default: r=v; g=p; b=q; break;
  }
  return ((int)(r * 31) << 11) | ((int)(g * 63) << 5) | (int)(b * 31);
}

// ============================================================
// 🌅 時間更新
// ============================================================
void updateTime() {
  if (millis() - lastTimeCheck < 1000) return;
  lastTimeCheck = millis();

  if (!ntpOK && strlen(TIME_WIFI_SSID) > 0 &&
      millis() - lastNtpTry > 30000) {
    lastNtpTry = millis();
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.begin(TIME_WIFI_SSID, TIME_WIFI_PASS);
      Serial.println("[🌐] Trying WiFi for NTP...");
    } else {
      configTime(8 * 3600, 0, "pool.ntp.org", "time.google.com");
      struct tm ti;
      if (getLocalTime(&ti, 100)) {
        ntpOK = true;
        hourNow = ti.tm_hour;
        Serial.printf("[🌐] NTP synced: %02d:%02d\n", ti.tm_hour, ti.tm_min);
      }
    }
  }

  if (ntpOK && WiFi.status() == WL_CONNECTED) {
    struct tm ti;
    if (getLocalTime(&ti, 10)) hourNow = ti.tm_hour;
  } else if (!ntpOK) {
    uint32_t elapsed = millis() - bootMillis;
    hourNow = (8 + (elapsed / 300000)) % 24;
  }

  bool wasDay = isDay;
  isDay = (hourNow >= 6 && hourNow < 18);
  if (wasDay != isDay) {
    Serial.printf("[🌅] %s now (hour=%d)\n", isDay ? "DAY" : "NIGHT", hourNow);
  }
}

uint16_t getSkyColor() {
  int h = hourNow;
  if (h >= 7 && h < 17)  return 0x1082;
  if (h >= 17 && h < 19) return 0xFD20;
  if (h >= 19 || h < 5)  return 0x0000;
  if (h >= 5 && h < 7)   return 0xFD20;
  return 0x1082;
}

// ============================================================
// 按鈕判定
// ============================================================
bool hitViewButton(int16_t x, int16_t y) {
  return (x >= VIEW_BTN_X && x < VIEW_BTN_X + VIEW_BTN_W &&
          y >= VIEW_BTN_Y && y < VIEW_BTN_Y + VIEW_BTN_H);
}
bool hitColdButton(int16_t x, int16_t y) {
  return (x >= COLD_BTN_X && x < COLD_BTN_X + COLD_BTN_W &&
          y >= COLD_BTN_Y && y < COLD_BTN_Y + COLD_BTN_H);
}
bool hitHeatButton(int16_t x, int16_t y) {
  return (x >= HEAT_BTN_X && x < HEAT_BTN_X + HEAT_BTN_W &&
          y >= HEAT_BTN_Y && y < HEAT_BTN_Y + HEAT_BTN_H);
}

// ============================================================
// 按鈕繪製
// ============================================================
void drawViewButton(LovyanGFX& gfx) {
  gfx.fillRoundRect(VIEW_BTN_X, VIEW_BTN_Y, VIEW_BTN_W, VIEW_BTN_H, 8, 0x2124);
  gfx.drawRoundRect(VIEW_BTN_X, VIEW_BTN_Y, VIEW_BTN_W, VIEW_BTN_H, 8, 0x07FF);
  const char* label;
  uint16_t col;
  if (currentView == 0)      { label = "3D >";   col = 0x07FF; }
  else if (currentView == 1) { label = "PIG3 >"; col = 0xFC9F; }
  else                       { label = "PIG >";  col = 0x07E0; }
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(col, 0x2124);
  gfx.setTextSize(1);
  gfx.drawString(label, VIEW_BTN_X + VIEW_BTN_W / 2, VIEW_BTN_Y + VIEW_BTN_H / 2);
}

void drawBottomButtons(LovyanGFX& gfx) {
  bool coldActive = (eyes.mood == EMO_COLD);
  uint16_t coldBg = coldActive ? 0x07FF : 0x18E3;
  gfx.fillRoundRect(COLD_BTN_X, COLD_BTN_Y, COLD_BTN_W, COLD_BTN_H, 10, coldBg);
  gfx.drawRoundRect(COLD_BTN_X, COLD_BTN_Y, COLD_BTN_W, COLD_BTN_H, 10, 0x07FF);
  gfx.drawRoundRect(COLD_BTN_X+1, COLD_BTN_Y+1, COLD_BTN_W-2, COLD_BTN_H-2, 9, 0x07FF);
  int snx = COLD_BTN_X + 26;
  int sny = COLD_BTN_Y + COLD_BTN_H / 2;
  uint16_t sc = coldActive ? 0x0000 : 0x07FF;
  for (int i = 0; i < 3; i++) {
    float a = i * 60 * DEG_TO_RAD;
    gfx.drawLine(snx - cos(a) * 9, sny - sin(a) * 9,
                 snx + cos(a) * 9, sny + sin(a) * 9, sc);
  }
  gfx.fillCircle(snx, sny, 2, sc);
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(coldActive ? 0x0000 : 0xFFFF, coldBg);
  gfx.setTextSize(2);
  gfx.drawString("COLD", COLD_BTN_X + COLD_BTN_W / 2 + 12, COLD_BTN_Y + COLD_BTN_H / 2);

  bool heatActive = (eyes.mood == EMO_HEAT);
  uint16_t heatBg = heatActive ? 0xF800 : 0x18E3;
  gfx.fillRoundRect(HEAT_BTN_X, HEAT_BTN_Y, HEAT_BTN_W, HEAT_BTN_H, 10, heatBg);
  gfx.drawRoundRect(HEAT_BTN_X, HEAT_BTN_Y, HEAT_BTN_W, HEAT_BTN_H, 10, 0xF800);
  gfx.drawRoundRect(HEAT_BTN_X+1, HEAT_BTN_Y+1, HEAT_BTN_W-2, HEAT_BTN_H-2, 9, 0xF800);
  int hux = HEAT_BTN_X + 26;
  int huy = HEAT_BTN_Y + HEAT_BTN_H / 2;
  uint16_t hc = heatActive ? 0xFFFF : 0xFFE0;
  gfx.fillCircle(hux, huy, 5, hc);
  for (int i = 0; i < 8; i++) {
    float a = i * 45 * DEG_TO_RAD;
    gfx.drawLine(hux + cos(a) * 7, huy + sin(a) * 7,
                 hux + cos(a) * 10, huy + sin(a) * 10, hc);
  }
  gfx.setTextColor(0xFFFF, heatBg);
  gfx.drawString("HEAT", HEAT_BTN_X + HEAT_BTN_W / 2 + 12, HEAT_BTN_Y + HEAT_BTN_H / 2);
}

// ============================================================
// 3D 方塊
// ============================================================
float cubeVerts[8][3] = {
  {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
  {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}
};
int cubeEdges[12][2] = {
  {0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},
  {0,4},{1,5},{2,6},{3,7}
};

void draw3DCube(LovyanGFX& gfx, int cx, int cy, float size) {
  float sx[8], sy[8], sz[8];
  float cosX = cos(rotX), sinX = sin(rotX);
  float cosY = cos(rotY), sinY = sin(rotY);
  for (int i = 0; i < 8; i++) {
    float x = cubeVerts[i][0], y = cubeVerts[i][1], z = cubeVerts[i][2];
    float x1 =  x * cosY + z * sinY;
    float z1 = -x * sinY + z * cosY;
    float y1 = y * cosX - z1 * sinX;
    float z2 = y * sinX + z1 * cosX;
    float f = 4.0f / (4.0f + z2);
    sx[i] = cx + x1 * size * f;
    sy[i] = cy + y1 * size * f;
    sz[i] = z2;
  }
  float minZ = 999, maxZ = -999;
  for (int i = 0; i < 8; i++) {
    if (sz[i] < minZ) minZ = sz[i];
    if (sz[i] > maxZ) maxZ = sz[i];
  }
  if (maxZ - minZ < 0.01f) maxZ = minZ + 0.01f;
  unsigned long now = millis();
  for (int i = 0; i < 12; i++) {
    int a = cubeEdges[i][0], b = cubeEdges[i][1];
    float t = ((sz[a]+sz[b])/2.0f - minZ) / (maxZ - minZ);
    float hue = fmod(now / 20.0f + i * 30.0f, 360.0f);
    uint16_t c = hsvToRGB565(hue, 1.0f, 1.0f - t * 0.7f);
    gfx.drawLine(sx[a], sy[a], sx[b], sy[b], c);
    gfx.drawLine(sx[a]+1, sy[a], sx[b]+1, sy[b], c);
  }
  for (int i = 0; i < 8; i++) {
    float t = (sz[i] - minZ) / (maxZ - minZ);
    float hue = fmod(now / 20.0f + i * 45.0f, 360.0f);
    uint16_t c = hsvToRGB565(hue, 1.0f, 1.0f - t * 0.5f);
    gfx.fillCircle(sx[i], sy[i], 3, c);
    gfx.fillCircle(sx[i], sy[i], 1, 0xFFFF);
  }
}

void draw3DView(LovyanGFX& gfx) {
  gfx.fillScreen(TFT_BLACK);
  gfx.setTextDatum(TC_DATUM);
  gfx.setTextColor(0x07FF, TFT_BLACK);
  gfx.setTextSize(2);
  gfx.drawString("3D CUBE", gfx.width() / 2, 6);
  draw3DCube(gfx, gfx.width() / 2, gfx.height() / 2 + 10, 55.0f);
  drawViewButton(gfx);
  WiFiFood::draw(gfx, gfx.width()/2, gfx.height()/2, 60, TFT_BLACK);
}

// ============================================================
// 3D 小豬
// ============================================================
#define PIG3D_MAX_VERTS 128
#define PIG3D_MAX_TRIS 256
struct Vertex3 { float x, y, z; };
struct Tri3 { int a, b, c; uint16_t color; float depth; };
Vertex3 pigVerts[PIG3D_MAX_VERTS];
Tri3 pigTris[PIG3D_MAX_TRIS];
int pigVertCount = 0, pigTriCount = 0;

void addBox3D(float cx, float cy, float cz, float sx, float sy, float sz, uint16_t color) {
  if (pigVertCount + 8 > PIG3D_MAX_VERTS) return;
  if (pigTriCount + 12 > PIG3D_MAX_TRIS) return;
  int base = pigVertCount;
  pigVerts[pigVertCount++] = {cx-sx, cy-sy, cz-sz};
  pigVerts[pigVertCount++] = {cx+sx, cy-sy, cz-sz};
  pigVerts[pigVertCount++] = {cx+sx, cy+sy, cz-sz};
  pigVerts[pigVertCount++] = {cx-sx, cy+sy, cz-sz};
  pigVerts[pigVertCount++] = {cx-sx, cy-sy, cz+sz};
  pigVerts[pigVertCount++] = {cx+sx, cy-sy, cz+sz};
  pigVerts[pigVertCount++] = {cx+sx, cy+sy, cz+sz};
  pigVerts[pigVertCount++] = {cx-sx, cy+sy, cz+sz};
  pigTris[pigTriCount++] = {base+4, base+5, base+6, color, 0};
  pigTris[pigTriCount++] = {base+4, base+6, base+7, color, 0};
  pigTris[pigTriCount++] = {base+1, base+0, base+3, color, 0};
  pigTris[pigTriCount++] = {base+1, base+3, base+2, color, 0};
  pigTris[pigTriCount++] = {base+0, base+4, base+7, color, 0};
  pigTris[pigTriCount++] = {base+0, base+7, base+3, color, 0};
  pigTris[pigTriCount++] = {base+5, base+1, base+2, color, 0};
  pigTris[pigTriCount++] = {base+5, base+2, base+6, color, 0};
  pigTris[pigTriCount++] = {base+0, base+1, base+5, color, 0};
  pigTris[pigTriCount++] = {base+0, base+5, base+4, color, 0};
  pigTris[pigTriCount++] = {base+2, base+3, base+7, color, 0};
  pigTris[pigTriCount++] = {base+2, base+7, base+6, color, 0};
}

void buildPig3D() {
  pigVertCount = pigTriCount = 0;
  uint16_t pink = 0xFC9F, pinkDark = 0xD145, snout = 0xF7BE;
  addBox3D(0, 0, 0, 0.55f, 0.45f, 0.85f, pink);
  addBox3D(0, -0.15f, 1.05f, 0.48f, 0.40f, 0.40f, pink);
  addBox3D(0, -0.15f, 1.52f, 0.25f, 0.18f, 0.08f, snout);
  addBox3D(-0.32f, -0.58f, 1.05f, 0.12f, 0.15f, 0.06f, pinkDark);
  addBox3D( 0.32f, -0.58f, 1.05f, 0.12f, 0.15f, 0.06f, pinkDark);
  addBox3D(-0.38f, 0.55f, -0.45f, 0.12f, 0.15f, 0.12f, pinkDark);
  addBox3D( 0.38f, 0.55f, -0.45f, 0.12f, 0.15f, 0.12f, pinkDark);
  addBox3D(-0.38f, 0.55f,  0.45f, 0.12f, 0.15f, 0.12f, pinkDark);
  addBox3D( 0.38f, 0.55f,  0.45f, 0.12f, 0.15f, 0.12f, pinkDark);
}

void renderPig3D(LovyanGFX& gfx, int cx, int cy, float size, float rX, float rY, float rZ) {
  float cosX = cosf(rX), sinX = sinf(rX);
  float cosY = cosf(rY), sinY = sinf(rY);
  float cosZ = cosf(rZ), sinZ = sinf(rZ);
  static float projX[PIG3D_MAX_VERTS], projY[PIG3D_MAX_VERTS], projZ[PIG3D_MAX_VERTS];
  for (int i = 0; i < pigVertCount; i++) {
    float x = pigVerts[i].x, y = pigVerts[i].y, z = pigVerts[i].z;
    float x1 = x * cosZ - y * sinZ, y1 = x * sinZ + y * cosZ;
    float x2 = x1 * cosY + z * sinY, z1 = -x1 * sinY + z * cosY;
    float y2 = y1 * cosX - z1 * sinX, z2 = y1 * sinX + z1 * cosX;
    float f = 4.0f / (4.0f + z2);
    projX[i] = cx + x2 * size * f;
    projY[i] = cy - y2 * size * f;
    projZ[i] = z2;
  }
  for (int i = 0; i < pigTriCount; i++)
    pigTris[i].depth = (projZ[pigTris[i].a] + projZ[pigTris[i].b] + projZ[pigTris[i].c]) / 3.0f;
  for (int i = 0; i < pigTriCount - 1; i++)
    for (int j = 0; j < pigTriCount - 1 - i; j++)
      if (pigTris[j].depth < pigTris[j+1].depth) {
        Tri3 t = pigTris[j]; pigTris[j] = pigTris[j+1]; pigTris[j+1] = t;
      }
  for (int i = 0; i < pigTriCount; i++) {
    Tri3& t = pigTris[i];
    float shade = 1.0f - (t.depth + 1.5f) * 0.20f;
    if (shade < 0.4f) shade = 0.4f;
    if (shade > 1.1f) shade = 1.1f;
    uint16_t c = t.color;
    int r = (int)(((c >> 11) & 0x1F) * shade);
    int g = (int)(((c >> 5) & 0x3F) * shade);
    int b = (int)((c & 0x1F) * shade);
    if (r > 31) r = 31; if (g > 63) g = 63; if (b > 31) b = 31;
    gfx.fillTriangle(projX[t.a], projY[t.a], projX[t.b], projY[t.b],
                     projX[t.c], projY[t.c], (r << 11) | (g << 5) | b);
  }
}

void draw3DPigView(LovyanGFX& gfx) {
  gfx.fillScreen(TFT_BLACK);
  gfx.setTextDatum(TC_DATUM);
  gfx.setTextColor(0xFC9F, TFT_BLACK);
  gfx.setTextSize(2);
  gfx.drawString("3D PIG", gfx.width() / 2, 6);
  renderPig3D(gfx, gfx.width() / 2, gfx.height() / 2 + 10, 60.0f, rotX, rotY, 0);
  drawViewButton(gfx);
  WiFiFood::draw(gfx, gfx.width()/2, gfx.height()/2, 60, TFT_BLACK);
}

// ============================================================
// IMU
// ============================================================
void initIMU() {
  Wire.begin(48, 47, 400000UL);
  if (imu.begin()) { imuOK = true; Serial.println("[IMU] QMI8658 OK"); }
  else             { imuOK = false; Serial.println("[IMU] QMI8658 NOT found"); }
  lastImuTime = millis();
}

void updateIMU() {
  if (!imuOK) { rotX += 0.012f; rotY += 0.018f; return; }
  imu.readAccel(ax, ay, az);
  imu.readGyro(gx, gy, gz);
  float targetRotX = atan2(ay, az);
  float targetRotY = -atan2(ax, az);
  rotX += (targetRotX - rotX) * 0.15f;
  rotY += (targetRotY - rotY) * 0.15f;
  if (currentView == 0 && !eyes.touching && FlyBug::activeCount() == 0) {
    float tiltX = constrain(ax * 1.8f, -1.0f, 1.0f);
    float tiltY = constrain(ay * 1.8f, -1.0f, 1.0f);
    eyes.targetLookX = eyes.targetLookX * 0.92f + tiltX * 0.08f;
    eyes.targetLookY = eyes.targetLookY * 0.92f + tiltY * 0.08f;
  }
}

void processIMUEvents(unsigned long now) {
  if (!imuOK) { lastMotion = now; return; }
  float gmag = sqrtf(gx*gx + gy*gy + gz*gz);
  if (gmag > MOTION_GYRO) lastMotion = now;
  bool rising = (gmag > FLICK_THRESH && prevGmag <= FLICK_THRESH);
  prevGmag = gmag;
  if (rising) {
    if (now - windowStart > 1500) { windowStart = now; peakCount = 0; }
    peakCount++; lastPeak = now;
  }
  if (peakCount >= 5) {
    habits.shakes++;
    jumpUntil = 0;
    yayUntil = 0;
    dizzyUntil = now + 3000;
    peakCount = 0; lastMotion = now;
    setMoodForced(EMO_DIZZY, 3000);
    Serial.printf("[🌀] DIZZY (shakes=%d)\n", habits.shakes);
  } else if (now >= dizzyUntil && peakCount >= 1 &&
             (now - lastPeak) > 400 && (now - lastFlick) > 700) {
    int r = random(100);
    if (r < 30)      setMood(EMO_SURPRISED, 800);
    else if (r < 60) setMood(EMO_HAPPY, 1000);
    else if (r < 85) setMood(EMO_SHY, 1200);
    else             setMood(EMO_ANNOYED, 1000);
    lastFlick = now; peakCount = 0; lastMotion = now;
  }
}

// ============================================================
// 🧠 無聊行為
// ============================================================
void updateIdleBehavior(unsigned long now) {
  if (currentView != 0) return;
  if (FlyBug::activeCount() > 0) return;
  if (eyes.touching) return;
  if (eyes.mood == EMO_SLEEPING || eyes.mood == EMO_DIZZY ||
      eyes.mood == EMO_COLD || eyes.mood == EMO_HEAT ||
      eyes.mood == EMO_ANNOYED || eyes.mood == EMO_HAPPY ||
      eyes.mood == EMO_LOVE || eyes.mood == EMO_SHY ||
      eyes.mood == EMO_PROUD) return;

  if (now - lastInteraction < 5000) return;
  if (now < nextIdleAction) return;

  int r = random(100);

  if (loveMeter < 20) {
    if (r < 60) {
      eyes.targetLookX = random(-80, 81) / 100.0f;
      eyes.targetLookY = random(-40, 41) / 100.0f;
      Serial.println("[🧠] Look around");
    } else if (r < 80) {
      eyes.blinking = true;
      eyes.blinkStart = now;
    }
  } else {
    if (r < 30) {
      eyes.targetLookX = random(-80, 81) / 100.0f;
      eyes.targetLookY = random(-40, 41) / 100.0f;
      Serial.println("[🧠] Look around");
    } else if (r < 50) {
      setMood(EMO_SQUINT, 1500);
      Serial.println("[🧠] Yawn");
    } else if (r < 65) {
      setMood(EMO_THINKING, 2000);
      Serial.println("[🧠] Thinking");
    } else if (r < 80) {
      setMood(EMO_HAPPY, 1200);
      Serial.println("[🧠] Happy peek");
    } else if (r < 90) {
      setMood(EMO_SQUINT, 2500);
      Serial.println("[🧠] Sleepy yawn");
    } else {
      eyes.targetLookX = 0;
      eyes.targetLookY = 0;
      Serial.println("[🧠] Look at you");
    }
  }

  nextIdleAction = now + random(3000, 8000);
}

void updateMoodState(unsigned long now) {
  uint32_t still = now - lastMotion;
  DeskMood prevMood = currentMood;

  uint32_t awakeLimit  = isDay ? AWAKE_MS  : (AWAKE_MS  / 2);
  uint32_t sleepyLimit = isDay ? SLEEPY_MS : (SLEEPY_MS / 2);

  if (still < awakeLimit)       currentMood = MOOD_AWAKE;
  else if (still < sleepyLimit) currentMood = MOOD_SLEEPY;
  else                          currentMood = MOOD_ASLEEP;

  // ★ 從睡眠醒來 → 起床氣窗口
  if (prevMood == MOOD_ASLEEP && currentMood != MOOD_ASLEEP) {
    wakeRageUntil = now + 3000;
    Serial.println("[😾] Wake rage window open");
  }

  if (eyes.mood == EMO_COLD || eyes.mood == EMO_HEAT ||
      eyes.mood == EMO_ANNOYED || eyes.mood == EMO_DIZZY) return;

  if (currentMood == MOOD_ASLEEP && prevMood != MOOD_ASLEEP) {
    setMood(EMO_SLEEPING, 0);
  } else if (currentMood == MOOD_SLEEPY && prevMood == MOOD_AWAKE) {
    if (eyes.mood != EMO_SLEEPING) setMood(EMO_SQUINT, 0);
  } else if (currentMood == MOOD_AWAKE && prevMood != MOOD_AWAKE) {
    int r = random(100);
    if (r < 50)      setMood(EMO_SURPRISED, 800);
    else if (r < 80) setMood(EMO_SQUINT, 600);
    else             setMood(EMO_ANNOYED, 800);
  }
}

// ============================================================
// 豬臉系統
// ============================================================
void initEyes(LovyanGFX& gfx) {
  int W = gfx.width(), H = gfx.height();
  eyes.pig.cx = W / 2;
  eyes.pig.cy = 108;
  eyes.pig.r  = 72;
  int eyeW = 38, eyeH = 42, gap = 38;
  eyes.left.cx  = eyes.pig.cx - gap / 2 - eyeW / 2;
  eyes.right.cx = eyes.pig.cx + gap / 2 + eyeW / 2;
  eyes.left.cy  = eyes.right.cy = eyes.pig.cy - 10;
  eyes.left.w   = eyes.right.w = eyeW;
  eyes.left.h   = eyes.right.h = eyeH;
  eyes.left.lidOpen = eyes.right.lidOpen = 1.0f;
  eyes.left.targetLid = eyes.right.targetLid = 1.0f;
  eyes.nextBlink = millis() + random(2500, 5000);
  for (int i = 0; i < MAX_PARTICLES; i++) eyes.particles[i].active = false;
  for (int i = 0; i < MAX_FIRE; i++) fireP[i].active = false;
  eyes.initialized = true;
}

void spawnParticles(uint16_t color, int count = 8) {
  int spawned = 0;
  for (int i = 0; i < MAX_PARTICLES && spawned < count; i++) {
    if (eyes.particles[i].active) continue;
    bool fromLeft = random(0, 2);
    int cx = fromLeft ? eyes.left.cx : eyes.right.cx;
    int cy = (fromLeft ? eyes.left.cy : eyes.right.cy) + 20;
    eyes.particles[i].x = cx + random(-20, 21);
    eyes.particles[i].y = cy + random(-10, 11);
    eyes.particles[i].vx = random(-30, 31) / 10.0f;
    eyes.particles[i].vy = random(-50, -10) / 10.0f;
    eyes.particles[i].life = 30;
    eyes.particles[i].maxLife = 30;
    eyes.particles[i].color = color;
    eyes.particles[i].active = true;
    spawned++;
  }
}

void updateParticles() {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    Particle& p = eyes.particles[i];
    if (!p.active) continue;
    p.x += p.vx; p.y += p.vy;
    p.vy += 0.15f; p.vx *= 0.98f;
    p.life--;
    if (p.life <= 0) p.active = false;
  }
}

void drawParticles(LovyanGFX& gfx) {
  for (int i = 0; i < MAX_PARTICLES; i++) {
    Particle& p = eyes.particles[i];
    if (!p.active) continue;
    int alpha = (p.life * 255) / p.maxLife;
    uint16_t c = p.color;
    if (alpha < 100) c = lerpColor(eyes.bgColor, p.color, alpha / 255.0f);
    gfx.fillCircle(p.x, p.y, (p.life > 15) ? 2 : 1, c);
  }
}

void setMood(EyeMood m, unsigned long durationMs) {
  int newPri = moodPriority(m);
  int curPri = moodPriority(eyes.mood);

  if (curPri > newPri && eyes.moodEnd > millis() && eyes.mood != EMO_NEUTRAL) {
    Serial.printf("[Mood] %s(x%d) rejected by %s(x%d)\n",
                  moodName(m), newPri, moodName(eyes.mood), curPri);
    return;
  }

  if (m != eyes.lastMood) {
    switch (m) {
      case EMO_HAPPY:     spawnParticles(PIG_PINK, 6); break;
      case EMO_LOVE:      spawnParticles(0xF81F, 8); break;
      case EMO_SURPRISED: spawnParticles(0xFFE0, 10); break;
      case EMO_ANGRY:     spawnParticles(0xF800, 5); break;
      case EMO_OK:        spawnParticles(0x07E0, 8); break;
      case EMO_DIZZY:     spawnParticles(0xFFE0, 12); break;
      case EMO_COLD:      spawnParticles(0x07FF, 10); break;
      case EMO_HEAT:      spawnParticles(0xF800, 10); break;
      case EMO_ANNOYED:   spawnParticles(0x7BEF, 5); break;
      case EMO_SHY:       spawnParticles(0xF81F, 5); break;
      case EMO_PROUD:     spawnParticles(0xFFE0, 15); break;
      default: break;
    }
    eyes.lastMood = m;
  }
  eyes.mood = m;
  eyes.moodEnd = (durationMs > 0) ? millis() + durationMs : 0;

  switch (m) {
    case EMO_NEUTRAL:
      eyes.targetColor = PIG_PINK;
      eyes.left.targetLid = eyes.right.targetLid = 1.0f;
      eyes.left.targetPupilScale = eyes.right.targetPupilScale = 1.0f;
      break;
    case EMO_HAPPY:     eyes.targetColor = PIG_PINK; break;
    case EMO_SAD:       eyes.targetColor = 0xAD55; break;
    case EMO_ANGRY:     eyes.targetColor = 0xF800; break;
    case EMO_LOVE:      eyes.targetColor = 0xF81F; break;
    case EMO_SURPRISED:
      eyes.targetColor = 0xFFE0;
      eyes.left.targetPupilScale = eyes.right.targetPupilScale = 1.3f;
      break;
    case EMO_SLEEPING:
      eyes.targetColor = 0x5D9B;
      eyes.left.targetLid = eyes.right.targetLid = 0.0f;
      break;
    case EMO_THINKING:  eyes.targetColor = PIG_PINK; break;
    case EMO_OK:
      eyes.targetColor = 0x07E0;
      eyes.okStart = millis();
      eyes.okActive = true;
      break;
    case EMO_FOCUS:
      eyes.targetColor = PIG_PINK;
      eyes.targetLookY = 0.3f;
      eyes.left.targetPupilScale = eyes.right.targetPupilScale = 0.75f;
      break;
    case EMO_SQUINT:
      eyes.targetColor = 0xFFE0;
      eyes.left.targetLid = eyes.right.targetLid = 0.35f;
      break;
    case EMO_HUNGRY:     eyes.targetColor = 0xFC00; break;
    case EMO_SICK:       eyes.targetColor = 0x7BEF; break;
    case EMO_DIZZY:
      eyes.targetColor = 0x5D9B;
      eyes.left.targetPupilScale = eyes.right.targetPupilScale = 1.0f;
      eyes.left.targetLid = eyes.right.targetLid = 1.0f;
      break;
    case EMO_COLD:
      eyes.targetColor = 0x5D7F;
      eyes.left.targetPupilScale = eyes.right.targetPupilScale = 0.7f;
      eyes.left.targetLid = eyes.right.targetLid = 1.0f;
      break;
    case EMO_HEAT:
      eyes.targetColor = 0xF800;
      eyes.left.targetPupilScale = eyes.right.targetPupilScale = 1.25f;
      eyes.left.targetLid = eyes.right.targetLid = 0.7f;
      break;
    case EMO_ANNOYED:
      eyes.targetColor = 0xFBE0;
      eyes.left.targetPupilScale = eyes.right.targetPupilScale = 1.0f;
      eyes.left.targetLid = eyes.right.targetLid = 0.45f;
      break;
    case EMO_SHY:
      eyes.targetColor = 0xFBE0;
      eyes.left.targetPupilScale = eyes.right.targetPupilScale = 0.85f;
      eyes.left.targetLid = eyes.right.targetLid = 0.55f;
      break;
    case EMO_PROUD:
      eyes.targetColor = 0xFFE0;
      eyes.left.targetPupilScale = eyes.right.targetPupilScale = 1.15f;
      eyes.left.targetLid = eyes.right.targetLid = 1.0f;
      break;
  }
}

void setMoodForced(EyeMood m, unsigned long durationMs) {
  setMood(m, durationMs);
}

void lookAt(int tx, int ty) {
  int centerX = (eyes.left.cx + eyes.right.cx) / 2;
  int centerY = eyes.left.cy;
  eyes.targetLookX = constrain((tx - centerX) / 100.0f, -1.0f, 1.0f);
  eyes.targetLookY = constrain((ty - centerY) / 80.0f, -1.0f, 1.0f);
}

void onTouchStart(int16_t tx, int16_t ty) {
  habits.touches++;
  eyes.touching = true;
  eyes.touchStart = millis();
  eyes.touchX = tx; eyes.touchY = ty;
  lookAt(tx, ty);
  eyes.left.targetPupilScale = eyes.right.targetPupilScale = 1.15f;

  unsigned long now = millis();
  bool wasLongAbsent = (now - lastInteraction > 30000) && (lastInteraction > 0);
  lastInteraction = now;

  // ★ 打招呼：超過 30 秒沒互動，再摸會超開心
  if (wasLongAbsent || welcomeMode) {
    welcomeMode = true;
    welcomeUntil = now + 2000;
    loveMeter += 0.3f;
    if (loveMeter > 100) loveMeter = 100;
    setMoodForced(EMO_HAPPY, 2000);
    spawnParticles(0xF81F, 10);
    spawnParticles(0xFFE0, 10);
    Serial.println("[🐶] WELCOME BACK!");
    eyes.lastTapMs = now;
    return;
  }

  // ★ 起床氣
  if (now < wakeRageUntil) {
    int r = random(100);
    if (r < 60) {
      setMoodForced(EMO_ANGRY, 1500);
      Serial.println("[😠] DON'T WAKE ME!");
    } else if (r < 85) {
      setMoodForced(EMO_ANNOYED, 1500);
      Serial.println("[😒] Ugh...");
    } else {
      Serial.println("[💤] ...zzz... (pretending)");
    }
    wakeRageUntil = 0;
    eyes.lastTapMs = now;
    return;
  }

  // ★ 愛意值增加
  loveMeter += 0.05f;
  if (loveMeter > 100) loveMeter = 100;

  // ★ 手指追蹤
  fingerX = tx;
  fingerY = ty;
  fingerTracking = true;
  fingerTrackEnd = now + 300;

  // 雙擊判斷
  if (now - eyes.lastTapMs < 400) {
    int r = random(100);
    if (loveMeter > 50) {
      if (r < 40)      setMood(EMO_LOVE, 1500);
      else if (r < 70) setMood(EMO_HAPPY, 1200);
      else             setMood(EMO_SHY, 1000);
    } else {
      if (r < 60)      setMood(EMO_SURPRISED, 500);
      else if (r < 90) setMood(EMO_SHY, 600);
      else             setMood(EMO_HAPPY, 800);
    }
  }
  eyes.lastTapMs = now;
}

void onTouchEnd(unsigned long durationMs) {
  eyes.touching = false;
  eyes.left.targetPupilScale = eyes.right.targetPupilScale = 1.0f;
  if (durationMs < 500) {
    eyes.blinking = true; eyes.blinkStart = millis();
  } else if (durationMs > 2000) {
    setMood(EMO_SQUINT, 800);
  }
}

// ============================================================
// 豬臉繪製
// ============================================================
void drawPigBody(LovyanGFX& gfx, int bounceY = 0) {
  int cx = eyes.pig.cx;
  int cy = eyes.pig.cy + bounceY;
  int r  = eyes.pig.r;
  gfx.fillTriangle(cx-r+10, cy-r+20, cx-r+30, cy-r-30, cx-30, cy-r+10, eyes.color);
  gfx.fillTriangle(cx-r+18, cy-r+20, cx-r+30, cy-r-15, cx-38, cy-r+15, PIG_PINK_DARK);
  gfx.fillTriangle(cx+r-10, cy-r+20, cx+r-30, cy-r-30, cx+30, cy-r+10, eyes.color);
  gfx.fillTriangle(cx+r-18, cy-r+20, cx+r-30, cy-r-15, cx+38, cy-r+15, PIG_PINK_DARK);
  gfx.fillCircle(cx, cy, r, eyes.color);
  gfx.fillCircle(cx-r/3, cy-r/3, r/4, lerpColor(eyes.color, TFT_WHITE, 0.25f));
  int snoutW = 38, snoutH = 26;
  int snoutX = cx, snoutY = cy + 24;
  gfx.fillEllipse(snoutX, snoutY, snoutW/2, snoutH/2, PIG_SNOUT);
  gfx.drawEllipse(snoutX, snoutY, snoutW/2, snoutH/2, PIG_PINK_DARK);
  gfx.fillEllipse(snoutX-8, snoutY, 4, 6, PIG_NOSE_HOLE);
  gfx.fillEllipse(snoutX+8, snoutY, 4, 6, PIG_NOSE_HOLE);
  gfx.fillEllipse(cx-r+16, cy+14, 11, 6, lerpColor(eyes.color, PIG_BLUSH, 0.6f));
  gfx.fillEllipse(cx+r-16, cy+14, 11, 6, lerpColor(eyes.color, PIG_BLUSH, 0.6f));
}

void drawEyeNormal(LovyanGFX& gfx, Eye& e) {
  int visibleH = (int)(e.h * e.lidOpen);
  if (visibleH < 3) visibleH = 3;
  bool hasFly = FlyBug::activeCount() > 0;
  float eyeMoveMul   = hasFly ? 10.0f : 6.0f;
  float pupilMoveMul = hasFly ? 7.0f  : 4.0f;
  int cx = e.cx + (int)(eyes.lookX * eyeMoveMul);
  int cy = e.cy + (int)(eyes.lookY * (eyeMoveMul * 0.7f));
  int hw = e.w / 2, hh = visibleH / 2;
  int radius = min(hw, hh) / 2;
  if (radius < 2) radius = 2;
  gfx.fillRoundRect(cx-hw-2, cy-hh-2, e.w+4, visibleH+4, radius+2, eyes.bgColor);
  gfx.fillRoundRect(cx-hw, cy-hh, e.w, visibleH, radius, PIG_EYE_WHITE);
  if (visibleH > 8) {
    int pupilR = (int)(min(hw, hh) * 0.55f * e.pupilScale);
    int px = cx + (int)(eyes.lookX * pupilMoveMul);
    int py = cy + (int)(eyes.lookY * (pupilMoveMul * 0.7f));
    int maxOffsetX = hw - pupilR;
    int maxOffsetY = visibleH/2 - pupilR;
    if (px - cx > maxOffsetX) px = cx + maxOffsetX;
    if (px - cx < -maxOffsetX) px = cx - maxOffsetX;
    if (py - cy > maxOffsetY) py = cy + maxOffsetY;
    if (py - cy < -maxOffsetY) py = cy - maxOffsetY;
    gfx.fillCircle(px, py, pupilR, PIG_EYE_BLACK);
    gfx.fillCircle(px - pupilR/3, py - pupilR/3, pupilR/3, PIG_EYE_WHITE);
    if (hasFly && e.pupilScale > 1.15f) {
      gfx.fillCircle(px + pupilR/3, py + pupilR/4, pupilR/5, PIG_EYE_WHITE);
    }
  }
}

void drawEyeHappy(LovyanGFX& gfx, Eye& e) {
  int cx = e.cx, cy = e.cy + 6, r = e.w / 2 - 4;
  for (int i = 0; i < 4; i++)
    gfx.drawArc(cx, cy, r-i, r-i-1, 200, 340, PIG_EYE_BLACK);
}

void drawEyeHeart(LovyanGFX& gfx, Eye& e) {
  unsigned long now = millis();
  float pulse = 1.0f + sin(now / 150.0) * 0.15f;
  int cx = e.cx, cy = e.cy;
  int r = (int)(e.w / 4 * pulse);
  gfx.fillCircle(cx-r, cy-r/2, r, 0xF81F);
  gfx.fillCircle(cx+r, cy-r/2, r, 0xF81F);
  gfx.fillTriangle(cx-r*2, cy, cx+r*2, cy, cx, cy+r*2, 0xF81F);
}

void drawEyeAngry(LovyanGFX& gfx, Eye& e, bool isRight) {
  int hw = e.w / 2, hh = e.h / 3;
  for (int i = 0; i < hh * 2; i++) {
    int y = e.cy - hh + i;
    int offset = isRight ? (i / 3) : -(i / 3);
    gfx.drawFastHLine(e.cx - hw + offset, y, e.w, PIG_EYE_BLACK);
  }
}

void drawEyeDizzy(LovyanGFX& gfx, Eye& e) {
  int cx = e.cx, cy = e.cy;
  gfx.fillCircle(cx, cy, e.w/2, PIG_EYE_WHITE);
  gfx.drawCircle(cx, cy, e.w/2, PIG_EYE_BLACK);
  unsigned long now = millis();
  float rot = now / 200.0;
  for (int i = 0; i < 3; i++) {
    int r = (e.w/2) - 3 - i * 4;
    if (r < 2) break;
    float a0 = fmod(rot + i * 60, 360);
    gfx.drawArc(cx, cy, r, r-1, a0, a0 + 300, PIG_EYE_BLACK);
  }
  gfx.fillCircle(cx, cy, 1, PIG_EYE_BLACK);
}

void drawEyeCold(LovyanGFX& gfx, Eye& e) {
  unsigned long now = millis();
  int chatter = (int)(sin(now / 40.0) * 2);
  for (int i = 0; i < 3; i++)
    gfx.drawArc(e.cx, e.cy + 2 + chatter, e.w/2 - i, e.w/2 - i - 1,
                180, 360, PIG_EYE_BLACK);
}

void drawEyeHeat(LovyanGFX& gfx, Eye& e) {
  int cx = e.cx, cy = e.cy;
  gfx.fillCircle(cx, cy, e.w/2, PIG_EYE_WHITE);
  gfx.fillRoundRect(cx - e.w/2, cy - e.h/2 - 2, e.w, e.h/2 + 4, 3, eyes.color);
  gfx.drawLine(cx - e.w/2 + 3, cy, cx + e.w/2 - 3, cy, PIG_EYE_BLACK);
}

void drawEyeAnnoyed(LovyanGFX& gfx, Eye& e, bool isRight) {
  int cx = e.cx, cy = e.cy;
  int hw = e.w / 2, hh = e.h / 2;
  gfx.fillRoundRect(cx - hw, cy - 2, e.w, 5, 2, PIG_EYE_BLACK);
  int browY = cy - hh - 6;
  int browInnerX = cx + (isRight ? -12 : 12);
  int browOuterX = cx + (isRight ? 12 : -12);
  int browInnerY = browY + 6;
  int browOuterY = browY - 2;
  for (int i = 0; i < 3; i++) {
    gfx.drawLine(browInnerX, browInnerY + i,
                 browOuterX, browOuterY + i, PIG_EYE_BLACK);
  }
}

void drawEyeShy(LovyanGFX& gfx, Eye& e) {
  int cx = e.cx, cy = e.cy + 6;
  int hw = e.w / 2;
  gfx.fillRoundRect(cx-hw, cy-8, e.w, 16, 4, eyes.bgColor);
  gfx.fillRoundRect(cx-hw, cy-8, e.w, 16, 4, PIG_EYE_WHITE);
  gfx.fillCircle(cx, cy + 2, 6, PIG_EYE_BLACK);
  gfx.fillCircle(cx-2, cy, 2, PIG_EYE_WHITE);
}

void drawEyeProud(LovyanGFX& gfx, Eye& e) {
  int cx = e.cx, cy = e.cy;
  gfx.fillCircle(cx, cy, e.w/2, PIG_EYE_WHITE);
  gfx.fillCircle(cx, cy - 1, 9, PIG_EYE_BLACK);
  gfx.fillCircle(cx - 3, cy - 4, 3, PIG_EYE_WHITE);
  gfx.fillCircle(cx + 3, cy + 2, 2, PIG_EYE_WHITE);
  unsigned long now = millis();
  for (int i = 0; i < 4; i++) {
    float a = (now / 200.0 + i * 90) * DEG_TO_RAD;
    int sx = cx + (int)(cos(a) * 12);
    int sy = cy + (int)(sin(a) * 12);
    gfx.drawPixel(sx, sy, 0xFFE0);
    gfx.drawPixel(sx+1, sy, 0xFFE0);
    gfx.drawPixel(sx, sy+1, 0xFFE0);
  }
}

void drawColdEffect(LovyanGFX& gfx) {
  unsigned long now = millis();
  for (int i = 0; i < 8; i++) {
    float t = fmod(now / 800.0f + i * 0.13f, 1.0f);
    int sx = eyes.pig.cx - 60 + i * 18;
    int sy = 60 + (int)(t * 110);
    uint16_t c = 0xEFFF;
    gfx.drawLine(sx-4, sy, sx+4, sy, c);
    gfx.drawLine(sx, sy-4, sx, sy+4, c);
  }
  int chatter = (int)(sin(now / 40.0) * 2);
  int mY = eyes.pig.cy + eyes.pig.r - 26 + chatter;
  gfx.drawLine(eyes.pig.cx - 10, mY, eyes.pig.cx + 10, mY, PIG_EYE_BLACK);
  gfx.drawLine(eyes.pig.cx - 10, mY + 3, eyes.pig.cx + 10, mY + 3, PIG_EYE_BLACK);
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(0x07FF, eyes.bgColor);
  gfx.setTextSize(2);
  gfx.drawString("BRRR...", eyes.pig.cx, EFFECT_TEXT_Y);
}

void drawHeatEffect(LovyanGFX& gfx) {
  unsigned long now = millis();
  int mouthX = eyes.pig.cx;
  int mouthY = eyes.pig.cy + eyes.pig.r - 22;

  static uint32_t lastSpawn = 0;
  if (now - lastSpawn > 35) {
    lastSpawn = now;
    int spawnCount = random(2, 4);
    for (int k = 0; k < spawnCount; k++) {
      for (int i = 0; i < MAX_FIRE; i++) {
        if (fireP[i].active) continue;
        fireP[i].x = mouthX + random(-4, 5);
        fireP[i].y = mouthY + random(-3, 4);
        float angle = random(0, 628) / 100.0f;
        float speed = random(20, 60) / 10.0f;
        fireP[i].vx = cosf(angle) * speed;
        fireP[i].vy = sinf(angle) * speed - 0.6f;
        fireP[i].life = random(20, 40);
        fireP[i].active = true;
        break;
      }
    }
  }

  for (int i = 0; i < MAX_FIRE; i++) {
    if (!fireP[i].active) continue;
    fireP[i].x += fireP[i].vx;
    fireP[i].y += fireP[i].vy;
    fireP[i].vy -= 0.08f;
    fireP[i].vx *= 0.97f;
    fireP[i].vy *= 0.97f;
    fireP[i].life--;
    if (fireP[i].life <= 0) fireP[i].active = false;
  }

  for (int i = 0; i < MAX_FIRE; i++) {
    if (!fireP[i].active) continue;
    int l = fireP[i].life;
    uint16_t c;
    if (l > 30)      c = 0xFFFF;
    else if (l > 22) c = 0xFFE0;
    else if (l > 15) c = 0xFD20;
    else if (l > 8)  c = 0xFBE0;
    else             c = 0xF800;
    int size = (l > 28) ? 3 : (l > 15 ? 2 : 1);
    gfx.fillCircle(fireP[i].x, fireP[i].y, size, c);
    if (l > 18 && size >= 2) {
      gfx.fillCircle(fireP[i].x, fireP[i].y, size - 1, 0xFFFF);
    }
  }

  for (int i = 0; i < 4; i++) {
    float t = fmod(now / 1200.0f + i * 0.25f, 1.0f);
    int sx = eyes.pig.cx - 45 + i * 30;
    int sy = eyes.pig.cy - eyes.pig.r + (int)(t * (eyes.pig.r * 1.3f));
    if (sy > EFFECT_TEXT_Y - 6) continue;
    gfx.fillCircle(sx, sy, 3, 0x5D9B);
    gfx.fillTriangle(sx-3, sy, sx+3, sy, sx, sy-6, 0x5D9B);
  }

  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(0xF800, eyes.bgColor);
  gfx.setTextSize(2);
  gfx.drawString("HOT...", eyes.pig.cx, EFFECT_TEXT_Y);
}

void drawAnnoyedEffect(LovyanGFX& gfx) {
  unsigned long now = millis();
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(0xFBE0, eyes.bgColor);
  gfx.setTextSize(2);
  float wobble = sin(now / 100.0) * 2;
  gfx.drawString("ANNOYED...", eyes.pig.cx + wobble, EFFECT_TEXT_Y);
  float wavePhase = sin(now / 150.0);
  int handX = eyes.pig.cx + eyes.pig.r - 10 + (int)(wavePhase * 8);
  int handY = eyes.pig.cy + 20;
  gfx.fillCircle(handX, handY, 6, PIG_PINK);
  gfx.drawCircle(handX, handY, 6, PIG_PINK_DARK);
  int handX2 = eyes.pig.cx - eyes.pig.r + 10 - (int)(wavePhase * 8);
  int handY2 = eyes.pig.cy + 20;
  gfx.fillCircle(handX2, handY2, 6, PIG_PINK);
  gfx.drawCircle(handX2, handY2, 6, PIG_PINK_DARK);
}

void drawShyEffect(LovyanGFX& gfx) {
  unsigned long now = millis();
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(0xF81F, eyes.bgColor);
  gfx.setTextSize(2);
  float wobble = sin(now / 200.0) * 2;
  gfx.drawString("...///", eyes.pig.cx + wobble, EFFECT_TEXT_Y);
  int blushOffset = (int)(sin(now / 200.0) * 2);
  gfx.fillEllipse(eyes.pig.cx - eyes.pig.r + 16,
                  eyes.pig.cy + 14 + blushOffset, 14, 8, 0xF800);
  gfx.fillEllipse(eyes.pig.cx + eyes.pig.r - 16,
                  eyes.pig.cy + 14 + blushOffset, 14, 8, 0xF800);
}

void drawProudEffect(LovyanGFX& gfx) {
  unsigned long now = millis();
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(0xFFE0, eyes.bgColor);
  gfx.setTextSize(2);
  float wobble = sin(now / 150.0) * 3;
  gfx.drawString("* PROUD *", eyes.pig.cx + wobble, EFFECT_TEXT_Y);
  int cy = eyes.pig.cy - eyes.pig.r - 8;
  for (int i = 0; i < 3; i++) {
    int px = eyes.pig.cx - 20 + i * 20;
    gfx.fillTriangle(px - 6, cy, px + 6, cy, px, cy - 12, 0xFFE0);
  }
}

void drawSleepEffect(LovyanGFX& gfx) {
  unsigned long now = millis();
  gfx.fillRoundRect(eyes.left.cx - eyes.left.w/2, eyes.left.cy - 2,
                    eyes.left.w, 5, 2, PIG_EYE_BLACK);
  gfx.fillRoundRect(eyes.right.cx - eyes.right.w/2, eyes.right.cy - 2,
                    eyes.right.w, 5, 2, PIG_EYE_BLACK);
  for (int i = 0; i < 3; i++) {
    unsigned long t = (now + i * 500) % 1500;
    int offsetY = t / 30;
    int size = 1 + i;
    int alpha = 255 - (offsetY * 4);
    if (alpha < 0) continue;
    uint16_t c = lerpColor(eyes.bgColor, 0x5D9B, alpha / 255.0f);
    gfx.setTextDatum(MC_DATUM);
    gfx.setTextColor(c, eyes.bgColor);
    gfx.setTextSize(size);
    gfx.drawString("Z", eyes.right.cx + 36 + i * 8 + offsetY/3,
                   eyes.right.cy - 28 - offsetY);
  }
}

void drawThinkingEffect(LovyanGFX& gfx) {
  drawEyeNormal(gfx, eyes.left);
  drawEyeNormal(gfx, eyes.right);
  unsigned long now = millis();
  float bounce = sin(now / 200.0) * 3;
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(0xFFE0, eyes.bgColor);
  gfx.setTextSize(3);
  gfx.drawString("?", eyes.pig.cx + 60, eyes.pig.cy - 70 + bounce);
}

void drawSurpriseEffect(LovyanGFX& gfx) {
  int r = eyes.left.w / 2 + 6;
  gfx.fillCircle(eyes.left.cx, eyes.left.cy, r, PIG_EYE_WHITE);
  gfx.fillCircle(eyes.right.cx, eyes.right.cy, r, PIG_EYE_WHITE);
  gfx.fillCircle(eyes.left.cx, eyes.left.cy, r*0.6f, PIG_EYE_BLACK);
  gfx.fillCircle(eyes.right.cx, eyes.right.cy, r*0.6f, PIG_EYE_BLACK);
  gfx.fillCircle(eyes.left.cx - r/3, eyes.left.cy - r/3, r/4, PIG_EYE_WHITE);
  gfx.fillCircle(eyes.right.cx - r/3, eyes.right.cy - r/3, r/4, PIG_EYE_WHITE);
  unsigned long now = millis();
  for (int i = 0; i < 6; i++) {
    float angle = (i * 60 + now / 15.0) * DEG_TO_RAD;
    int dist = r + 12 + sin(now / 200.0 + i) * 3;
    int sx = eyes.left.cx + dist * cos(angle);
    int sy = eyes.left.cy + dist * sin(angle);
    uint16_t c = (i % 2 == 0) ? 0xFFE0 : 0xFC9F;
    gfx.drawPixel(sx, sy, c);
    gfx.drawPixel(sx-1, sy, c); gfx.drawPixel(sx+1, sy, c);
    gfx.drawPixel(sx, sy-1, c); gfx.drawPixel(sx, sy+1, c);
  }
}

void drawOkEffect(LovyanGFX& gfx) {
  unsigned long elapsed = millis() - eyes.okStart;
  if (elapsed > 1800) { eyes.okActive = false; return; }
  drawEyeNormal(gfx, eyes.left);
  drawEyeNormal(gfx, eyes.right);
  int cx = gfx.width() / 2;
  int baseY = eyes.pig.cy + 80;
  for (int i = 0; i < 5; i++) {
    gfx.drawLine(cx - 30 + i, baseY, cx - 8 + i, baseY + 22, TFT_GREEN);
    gfx.drawLine(cx - 8 + i, baseY + 22, cx + 30 + i, baseY - 26, TFT_GREEN);
  }
  gfx.setTextDatum(TC_DATUM);
  gfx.setTextColor(TFT_GREEN, TFT_BLACK);
  gfx.setTextSize(3);
  gfx.drawString("OK", cx, baseY + 35);
}

void drawDizzyEffect(LovyanGFX& gfx) {
  unsigned long now = millis();
  for (int i = 0; i < 3; i++) {
    float angle = (now / 300.0 + i * 120) * DEG_TO_RAD;
    int r = 45;
    int sx = eyes.pig.cx + r * cos(angle);
    int sy = eyes.pig.cy - 48 + r * sin(angle) * 0.35;
    uint16_t starColors[3] = {0xFFE0, 0x07FF, 0xF81F};
    uint16_t sc = starColors[i];
    gfx.drawPixel(sx, sy, sc);
    gfx.drawLine(sx-3, sy, sx+3, sy, sc);
    gfx.drawLine(sx, sy-3, sx, sy+3, sc);
  }
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextColor(0xFFE0, eyes.bgColor);
  gfx.setTextSize(2);
  float wobble = sin(now / 150.0) * 3;
  gfx.drawString("DIZZY...", eyes.pig.cx + wobble, EFFECT_TEXT_Y);
}

void drawGlow(LovyanGFX& gfx) {
  if (eyes.mood == EMO_SLEEPING) return;
  float glow = (sin(eyes.breathPhase) + 1.0f) * 0.5f;
  for (int i = 0; i < 2; i++) {
    int r = eyes.pig.r + 6 + i * 7 + glow * 4;
    uint16_t c = lerpColor(eyes.bgColor, eyes.color, 0.12f - i * 0.05f);
    gfx.drawCircle(eyes.pig.cx, eyes.pig.cy, r, c);
  }
}

void drawCelestialBody(LovyanGFX& gfx) {
  int sx = 210, sy = 16;
  if (isDay) {
    gfx.fillCircle(sx, sy, 6, 0xFD20);
    for (int i = 0; i < 8; i++) {
      float a = i * 45 * DEG_TO_RAD;
      gfx.drawLine(sx + cos(a) * 7, sy + sin(a) * 7,
                   sx + cos(a) * 10, sy + sin(a) * 10, 0xFFE0);
    }
  } else {
    gfx.fillCircle(sx, sy, 7, 0xFFEC);
    gfx.fillCircle(sx + 4, sy - 2, 6, eyes.bgColor);
    gfx.drawPixel(sx - 8, sy - 4, 0xFFFF);
    gfx.drawPixel(sx + 8, sy + 4, 0xFFFF);
  }
}

// ★ 愛意值指示燈
void drawLoveMeter(LovyanGFX& gfx) {
  int lx = 300, ly = 42;
  // 背景
  gfx.drawCircle(lx, ly, 5, 0x5D9B);
  // 填充
  int fillR = (int)(5 * (loveMeter / 100.0f));
  if (fillR > 0) {
    uint16_t c = (loveMeter > 50) ? 0xF81F :
                 (loveMeter > 20) ? 0xFFE0 :
                                    0x7BEF;
    gfx.fillCircle(lx, ly, fillR, c);
  }
  // 打招呼時加個心
  if (welcomeMode) {
    gfx.fillCircle(lx - 3, ly - 3, 2, 0xF81F);
    gfx.fillCircle(lx + 3, ly - 3, 2, 0xF81F);
    gfx.fillTriangle(lx - 5, ly - 2, lx + 5, ly - 2, lx, ly + 5, 0xF81F);
  }
}

void drawEyes(LovyanGFX& gfx) {
  uint16_t skyColor = getSkyColor();
  gfx.fillRect(0, 0, gfx.width(), gfx.height(), skyColor);
  eyes.bgColor = skyColor;

  if (!isDay) {
    unsigned long now = millis();
    static int starSeedX[20], starSeedY[20];
    static bool seeded = false;
    if (!seeded) {
      for (int i = 0; i < 20; i++) {
        starSeedX[i] = random(10, 310);
        starSeedY[i] = random(40, 180);
      }
      seeded = true;
    }
    for (int i = 0; i < 20; i++) {
      int twinkle = (now / 200 + i * 37) % 4;
      if (twinkle == 0) continue;
      gfx.drawPixel(starSeedX[i], starSeedY[i], 0xFFFF);
    }
  }

  int jumpOffset = 0;
  bool isDizzyNow = (eyes.mood == EMO_DIZZY);
  if (!isDizzyNow && millis() < jumpUntil) {
    float t = (jumpUntil - millis()) / 500.0f;
    jumpOffset = -(int)(sinf((1.0f - t) * PI) * 8);
  }

  eyes.pig.bounce = sin(eyes.breathPhase) * 2.5f;
  int bounceY = (int)eyes.pig.bounce + jumpOffset;

  int headOffsetX = 0;
  int headOffsetY = 0;
  if (FlyBug::activeCount() > 0) {
    headSmoothX += (eyes.lookX * 8.0f - headSmoothX) * 0.35f;
    headSmoothY += (eyes.lookY * 6.0f - headSmoothY) * 0.35f;
    headOffsetX = (int)headSmoothX;
    headOffsetY = (int)headSmoothY;
  } else {
    headSmoothX *= 0.85f;
    headSmoothY *= 0.85f;
    headOffsetX = (int)headSmoothX;
    headOffsetY = (int)headSmoothY;
  }
  eyes.pig.cx += headOffsetX;
  eyes.pig.cy += headOffsetY;
  eyes.left.cx += headOffsetX;
  eyes.right.cx += headOffsetX;
  eyes.left.cy += headOffsetY;
  eyes.right.cy += headOffsetY;

  static int dizzyOffsetX = 0;
  static int dizzyOffsetY = 0;
  if (isDizzyNow) {
    unsigned long now = millis();
    dizzyOffsetX = (int)(sin(now / 80.0) * 6);
    dizzyOffsetY = (int)(cos(now / 60.0) * 3);
    eyes.pig.cx += dizzyOffsetX; eyes.pig.cy += dizzyOffsetY;
    eyes.left.cx += dizzyOffsetX; eyes.right.cx += dizzyOffsetX;
    eyes.left.cy += dizzyOffsetY; eyes.right.cy += dizzyOffsetY;
  }

  drawGlow(gfx);
  drawPigBody(gfx, bounceY);

  switch (eyes.mood) {
    case EMO_HAPPY:
      drawEyeHappy(gfx, eyes.left); drawEyeHappy(gfx, eyes.right); break;
    case EMO_LOVE:
      drawEyeHeart(gfx, eyes.left); drawEyeHeart(gfx, eyes.right); break;
    case EMO_ANGRY:
      drawEyeAngry(gfx, eyes.left, false); drawEyeAngry(gfx, eyes.right, true); break;
    case EMO_SURPRISED: drawSurpriseEffect(gfx); break;
    case EMO_SLEEPING:  drawSleepEffect(gfx); break;
    case EMO_THINKING:  drawThinkingEffect(gfx); break;
    case EMO_OK:        drawOkEffect(gfx); break;
    case EMO_DIZZY:
      drawEyeDizzy(gfx, eyes.left); drawEyeDizzy(gfx, eyes.right);
      drawDizzyEffect(gfx); break;
    case EMO_COLD:
      drawEyeCold(gfx, eyes.left); drawEyeCold(gfx, eyes.right);
      drawColdEffect(gfx); break;
    case EMO_HEAT:
      drawEyeHeat(gfx, eyes.left); drawEyeHeat(gfx, eyes.right);
      drawHeatEffect(gfx); break;
    case EMO_ANNOYED:
      drawEyeAnnoyed(gfx, eyes.left, false);
      drawEyeAnnoyed(gfx, eyes.right, true);
      drawAnnoyedEffect(gfx); break;
    case EMO_SHY:
      drawEyeShy(gfx, eyes.left); drawEyeShy(gfx, eyes.right);
      drawShyEffect(gfx); break;
    case EMO_PROUD:
      drawEyeProud(gfx, eyes.left); drawEyeProud(gfx, eyes.right);
      drawProudEffect(gfx); break;
    default:
      drawEyeNormal(gfx, eyes.left); drawEyeNormal(gfx, eyes.right); break;
  }
  drawParticles(gfx);

  if (isDizzyNow) {
    eyes.pig.cx -= dizzyOffsetX; eyes.pig.cy -= dizzyOffsetY;
    eyes.left.cx -= dizzyOffsetX; eyes.right.cx -= dizzyOffsetX;
    eyes.left.cy -= dizzyOffsetY; eyes.right.cy -= dizzyOffsetY;
  }

  if (FlyBug::activeCount() > 0) {
    eyes.pig.cx -= headOffsetX;
    eyes.pig.cy -= headOffsetY;
    eyes.left.cx -= headOffsetX;
    eyes.right.cx -= headOffsetX;
    eyes.left.cy -= headOffsetY;
    eyes.right.cy -= headOffsetY;
  }

  WiFiFood::draw(gfx, eyes.pig.cx, eyes.pig.cy, eyes.pig.r, eyes.bgColor);

  if (!WiFiFood::isInToilet()) {
    FlyBug::draw(gfx);
  }

  drawCelestialBody(gfx);
  drawLoveMeter(gfx);
  drawViewButton(gfx);
  drawBottomButtons(gfx);
}

unsigned long lastEyeUpdate = 0;
void updateEyes(LovyanGFX& gfx, unsigned long now) {
  if (!eyes.initialized) return;
  bool isActive = eyes.okActive || eyes.typingActive || eyes.touching ||
                  eyes.blinking || eyes.mood == EMO_SURPRISED ||
                  eyes.mood == EMO_LOVE || eyes.mood == EMO_DIZZY ||
                  eyes.mood == EMO_COLD || eyes.mood == EMO_HEAT ||
                  eyes.mood == EMO_ANNOYED ||
                  eyes.mood == EMO_SHY || eyes.mood == EMO_PROUD ||
                  WiFiFood::getPhase() == WiFiFood::PHASE_EATING ||
                  WiFiFood::getPhase() == WiFiFood::PHASE_DIGESTING ||
                  FlyBug::activeCount() > 0 ||
                  fingerTracking || welcomeMode;
  unsigned long interval = isActive ? 33 : 80;
  if (now - lastEyeUpdate < interval) return;
  lastEyeUpdate = now;

  bool needRedraw = false;
  if (eyes.color != eyes.targetColor) {
    eyes.color = lerpColor(eyes.color, eyes.targetColor, 0.15f);
    needRedraw = true;
  }
  eyes.breathPhase += (eyes.mood == EMO_NEUTRAL || eyes.mood == EMO_HAPPY ||
                       eyes.mood == EMO_FOCUS) ? 0.08f : 0.05f;

  if (eyes.mood == EMO_HAPPY && eyes.moodEnd > 0 &&
      eyes.moodEnd - millis() > 1500 &&
      eyes.mood != EMO_DIZZY) {
    yayUntil = millis() + 300;
  }
  if (now < yayUntil && eyes.mood != EMO_DIZZY) {
    eyes.left.targetPupilScale = eyes.right.targetPupilScale = 1.4f;
  }

  // ★ 手指追蹤（優先於飛蟲）
  if (fingerTracking && !eyes.touching) {
    int dx = fingerX - eyes.pig.cx;
    int dy = fingerY - eyes.pig.cy;
    eyes.targetLookX = constrain(dx / 80.0f, -1.0f, 1.0f);
    eyes.targetLookY = constrain(dy / 80.0f, -1.0f, 1.0f);
    needRedraw = true;
  }

  // 飛蟲追蹤
  int flyX, flyY;
  bool hasFly = FlyBug::getNearestFly(eyes.pig.cx, eyes.pig.cy, flyX, flyY);
  if (hasFly) {
    flyLockUntil = now + 500;
    flyLockX = flyX; flyLockY = flyY;
    int dx = flyX - eyes.pig.cx;
    int dy = flyY - eyes.pig.cy;
    flyDistance = sqrtf(dx * dx + dy * dy);
    float lx = constrain(dx / 40.0f, -1.3f, 1.3f);
    float ly = constrain(dy / 40.0f, -1.3f, 1.3f);
    eyes.targetLookX = lx;
    eyes.targetLookY = ly;
    float targetPupil = 1.0f;
    if (flyDistance < 50)      targetPupil = 1.25f;
    else if (flyDistance < 80) targetPupil = 1.1f;
    eyes.left.targetPupilScale  = targetPupil;
    eyes.right.targetPupilScale = targetPupil;
    needRedraw = true;
  } else {
    flyDistance = 999;
  }

  float lerp = 0.2f;
  if (hasFly) lerp = 0.5f;
  if (abs(eyes.lookX - eyes.targetLookX) > 0.005f) {
    eyes.lookX += (eyes.targetLookX - eyes.lookX) * lerp; needRedraw = true;
  }
  if (abs(eyes.lookY - eyes.targetLookY) > 0.005f) {
    eyes.lookY += (eyes.targetLookY - eyes.lookY) * lerp; needRedraw = true;
  }
  for (auto* e : {&eyes.left, &eyes.right}) {
    if (abs(e->pupilScale - e->targetPupilScale) > 0.01f) {
      float pScale = hasFly ? 0.25f : 0.15f;
      e->pupilScale += (e->targetPupilScale - e->pupilScale) * pScale;
      needRedraw = true;
    }
  }

  static bool flyAnnoyed = false;
  if (FlyBug::activeCount() > 0 && !flyAnnoyed && !eyes.touching) {
    setMood(EMO_ANNOYED, 0);
    flyAnnoyed = true;
  } else if (FlyBug::activeCount() == 0 && flyAnnoyed) {
    flyAnnoyed = false;
  }

  if (eyes.touching && now - eyes.touchStart > 1500) {
    if (eyes.mood != EMO_SQUINT) setMood(EMO_SQUINT, 0);
  }
  if (eyes.touching && now - eyes.touchStart > 3000 &&
      eyes.mood != EMO_DIZZY) {
    setMoodForced(EMO_DIZZY, 3000);
  }
  if (!eyes.blinking && now >= eyes.nextBlink &&
      eyes.mood != EMO_SLEEPING && eyes.mood != EMO_HAPPY &&
      eyes.mood != EMO_LOVE && eyes.mood != EMO_OK &&
      eyes.mood != EMO_SURPRISED && eyes.mood != EMO_DIZZY &&
      eyes.mood != EMO_COLD && eyes.mood != EMO_HEAT &&
      eyes.mood != EMO_ANNOYED) {
    eyes.blinking = true; eyes.blinkStart = now;
    eyes.left.targetLid = eyes.right.targetLid = 0.0f;
    needRedraw = true;
  }
  if (eyes.blinking && now - eyes.blinkStart > 140) {
    eyes.blinking = false;
    if (eyes.mood != EMO_SQUINT && eyes.mood != EMO_ANNOYED)
      eyes.left.targetLid = eyes.right.targetLid = 1.0f;
    eyes.nextBlink = now + random(2500, 6500);
    needRedraw = true;
  }
  for (auto* e : {&eyes.left, &eyes.right}) {
    if (abs(e->lidOpen - e->targetLid) > 0.05f) {
      float speed = eyes.blinking ? 0.6f : 0.25f;
      e->lidOpen += (e->targetLid - e->lidOpen) * speed;
      needRedraw = true;
    }
  }
  if (eyes.mood == EMO_NEUTRAL && !eyes.typingActive && !eyes.touching && !hasFly && !fingerTracking) {
    float breath = sin(eyes.breathPhase) * 0.02f;
    eyes.left.targetPupilScale = eyes.right.targetPupilScale = 1.0f + breath;
  }
  if (eyes.moodEnd > 0 && now > eyes.moodEnd && !eyes.okActive) {
    eyes.moodEnd = 0;
    setMood(EMO_NEUTRAL, 0);
    eyes.targetLookX = 0; eyes.targetLookY = 0;
    needRedraw = true;
  }
  if (eyes.mood == EMO_THINKING && random(0, 15) == 0) {
    eyes.targetLookX = random(-40, 41) / 100.0f;
    eyes.targetLookY = -0.3f;
    needRedraw = true;
  }
  if (eyes.okActive) {
    if (now - eyes.okStart > 1800) { eyes.okActive = false; setMood(EMO_NEUTRAL, 0); }
    needRedraw = true;
  }
  bool hasParticles = false;
  for (int i = 0; i < MAX_PARTICLES; i++)
    if (eyes.particles[i].active) { hasParticles = true; break; }
  if (hasParticles) { updateParticles(); needRedraw = true; }
  if (eyes.mood == EMO_SURPRISED || eyes.mood == EMO_THINKING ||
      eyes.mood == EMO_LOVE || eyes.mood == EMO_SLEEPING ||
      eyes.mood == EMO_DIZZY ||
      eyes.mood == EMO_COLD || eyes.mood == EMO_HEAT ||
      eyes.mood == EMO_ANNOYED ||
      eyes.mood == EMO_SHY || eyes.mood == EMO_PROUD ||
      WiFiFood::isScanning() ||
      WiFiFood::getPhase() == WiFiFood::PHASE_TOILET ||
      FlyBug::activeCount() > 0 ||
      fingerTracking || welcomeMode) needRedraw = true;

  if (eyes.mood == EMO_HEAT) {
    bool anyFire = false;
    for (int i = 0; i < MAX_FIRE; i++)
      if (fireP[i].active) { anyFire = true; break; }
    if (anyFire) needRedraw = true;
  }

  static int breathFrame = 0;
  if (eyes.mood == EMO_NEUTRAL || eyes.mood == EMO_HAPPY || eyes.mood == EMO_FOCUS) {
    if (++breathFrame >= 3) { breathFrame = 0; needRedraw = true; }
  }
  if (needRedraw) drawEyes(gfx);
}

// ============================================================
// Touch
// ============================================================
void handleTouch(unsigned long now) {
  uint16_t tx, ty;
  bool touched = tft.getTouch(&tx, &ty);
  static bool lastTouchState = false;
  static unsigned long touchDownTime = 0;

  if (touched) {
    if (!lastTouchState) {
      touchDownTime = now;
      lastMotion = now;

      // 打死飛蟲
      if (currentView == 0 && !WiFiFood::isInToilet() &&
          eyes.mood != EMO_DIZZY &&
          eyes.mood != EMO_COLD &&
          eyes.mood != EMO_HEAT) {
        int flyIdx = FlyBug::hitTest(tx, ty, 20);
        if (flyIdx >= 0) {
          FlyBug::killFly(flyIdx);
          habits.fliesKilled++;
          loveMeter += 5;
          if (loveMeter > 100) loveMeter = 100;

          int r = random(100);
          if (r < 70) {
            setMoodForced(EMO_HAPPY, 2500);
            spawnParticles(0xF81F, 8);
            spawnParticles(0xFFE0, 8);
            spawnParticles(0x07FF, 8);
            spawnParticles(0x07E0, 6);
            jumpUntil = millis() + 500;
            yayUntil = millis() + 300;
          } else if (r < 88) {
            setMoodForced(EMO_LOVE, 2000);
          } else if (r < 96) {
            setMoodForced(EMO_PROUD, 2500);
          } else {
            setMoodForced(EMO_SHY, 1500);
          }

          Serial.printf("[🐷] Fly killed! total=%d love=%.1f\n",
                        habits.fliesKilled, loveMeter);

          if (habits.fliesKilled >= 10 && !eggFlies10) {
            eggFlies10 = true;
            setMoodForced(EMO_PROUD, 3000);
            spawnParticles(0xFFE0, 20);
            Serial.println("🥚 EGG: 10 flies killed! PROUD!");
          }
          if (habits.fliesKilled >= 25 && !eggFlies25) {
            eggFlies25 = true;
            setMoodForced(EMO_LOVE, 3000);
            spawnParticles(0xF81F, 20);
            Serial.println("🥚 EGG: 25 flies killed!");
          }
          if (habits.fliesKilled >= 50 && !eggFlies50) {
            eggFlies50 = true;
            setMoodForced(EMO_HAPPY, 5000);
            spawnParticles(0xFFE0, 20);
            spawnParticles(0xF81F, 20);
            Serial.println("🥚 EGG: 50 flies! LEGENDARY!");
          }

          lastTouchState = true;
          return;
        }
      }

      if (hitViewButton(tx, ty)) {
        currentView = (currentView + 1) % 3;
        canvas.fillScreen(TFT_BLACK);
        if (currentView == 0)      drawEyes(canvas);
        else if (currentView == 1) draw3DView(canvas);
        else                       draw3DPigView(canvas);
        canvas.pushSprite(0, 0);
        lastTouchState = true;
        return;
      }

      if (currentView == 0) {
        if (hitColdButton(tx, ty)) {
          habits.coldsUsed++;
          setMoodForced(eyes.mood == EMO_COLD ? EMO_NEUTRAL : EMO_COLD, 0);
          lastTouchState = true; return;
        }
        if (hitHeatButton(tx, ty)) {
          habits.heatsUsed++;
          setMoodForced(eyes.mood == EMO_HEAT ? EMO_NEUTRAL : EMO_HEAT, 0);
          lastTouchState = true; return;
        }
      }
      if (currentView == 0) onTouchStart(tx, ty);
    } else {
      // ★ 拖動中更新手指位置
      fingerX = tx;
      fingerY = ty;
      fingerTrackEnd = now + 300;
    }
  } else {
    if (lastTouchState) {
      if (currentView == 0) onTouchEnd(now - touchDownTime);
    }
  }
  lastTouchState = touched;
}

// ============================================================
// setup / loop
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== 大臉小豬 v4 — 智能寵物版 ===");

  randomSeed(esp_random());

  tft.init();
  tft.setRotation(1);
  tft.setBrightness(200);

  canvas.setColorDepth(16);
  canvas.setPsram(true);
  canvas.createSprite(320, 240);

  initIMU();
  buildPig3D();
  initEyes(canvas);

  WiFiFood::setMoodCallback([](int id, unsigned long dur) {
    switch (id) {
      case WiFiFood::MOOD_HAPPY: {
        habits.wifiMeals++;
        setMood(EMO_HAPPY, dur);
        if (habits.wifiMeals >= 10 && !eggWifi10) {
          eggWifi10 = true;
          Serial.println("🥚 EGG: 10 WiFi meals!");
        }
        break;
      }
      case WiFiFood::MOOD_SAD:   setMood(EMO_SAD, dur); break;
      default:                   setMood(EMO_NEUTRAL, 0); break;
    }
  });
  WiFiFood::begin();

  FlyBug::begin(eyes.pig.cx, eyes.pig.cy, eyes.pig.r);

  habits.sessionStart = millis();
  bootMillis = millis();
  lastInteraction = millis();
  nextIdleAction = millis() + 5000;

  canvas.fillScreen(TFT_BLACK);
  drawEyes(canvas);
  canvas.pushSprite(0, 0);

  lastMotion = millis();
  Serial.println("Ready!");
}

void loop() {
  unsigned long now = millis();
  updateIMU();
  updateTime();
  WiFiFood::update();
  FlyBug::update(now);
  processIMUEvents(now);
  updateIdleBehavior(now);
  updateMoodState(now);
  handleTouch(now);

  // ★ 手指追蹤結束
  if (fingerTracking && now > fingerTrackEnd) {
    fingerTracking = false;
  }

  // ★ 打招呼結束
  if (welcomeMode && now > welcomeUntil) {
    welcomeMode = false;
  }

  // ★ 愛意值衰減
  static uint32_t lastDecay = 0;
  if (now - lastDecay > 1000) {
    lastDecay = now;
    loveMeter -= 0.02f;
    if (loveMeter < 0) loveMeter = 0;
  }

  if (currentView == 0)      updateEyes(canvas, now);
  else if (currentView == 1) draw3DView(canvas);
  else                       draw3DPigView(canvas);

  canvas.pushSprite(0, 0);
  delay(10);
}