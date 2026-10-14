# 🐷 WiFi Pig

> 一隻會吃 WiFi、拉便便、還會跑去廁所的桌面小豬。

**ESP32-S3-Touch-LCD-2** 上的互動式小豬——具備 15 種情緒、3D 視圖、IMU 傾斜控制、觸控互動，以及會廣播假 WiFi 熱點的惡作劇功能。

<p align="center">
  <img src="https://img.shields.io/badge/ESP32--S3-Touch--LCD--2-blue" alt="Board">
  <img src="https://img.shields.io/badge/LovyanGFX-1.1.16+-green" alt="LovyanGFX">
  <img src="https://img.shields.io/badge/PlatformIO-Arduino-orange" alt="PlatformIO">
  <img src="https://img.shields.io/badge/license-MIT-yellow" alt="License">
</p>

---

## ✨ 功能特色

### 🐷 大臉小豬（15 種情緒）
| 情緒 | 觸發方式 | 視覺 |
|------|---------|------|
| 中性 | 預設 | 呼吸 + 隨機眨眼 |
| 開心 | 聊天 / 甩動 | 彎月眼 + 粒子 |
| 愛心 | 說「愛」 | 愛心眼 + 心跳 |
| 驚訝 | 雙擊 / 醒來 | 大眼 + 星光 |
| 睡覺 | 靜置 40 秒 | 閉眼 + Z 飄 |
| 瞇眼 | 長按 1.5 秒 | 半閉眼 |
| 暈眩 | 劇烈搖晃 | 螺旋眼 + 星星 |
| 冷 | 點 COLD | 冰藍 + 雪花 + BRRR |
| 熱 | 點 HEAT | 紅臉 + 汗滴 + HOT |
| 生氣 | 甩動循環 | 紅臉 + 生氣眉 |
| 難過 | 甩動循環 | 灰色 + 眼淚 |
| 思考 | 甩動循環 | 眼睛漂移 + ? |

### 🎲 3D 視圖
- **3D 方塊**：彩虹線框 + 8 個頂點隨板子傾斜旋轉
- **3D 小豬**：低多邊形模型（14 個立方體組成）+ 深度著色

### 📶 WiFi 當食物
小豬會**掃描附近所有 WiFi 訊號**當作食物：
| WiFi 數量 | 食物 |
|-----------|------|
| 0 | 沒食物 😢 |
| 1~3 | 🥕 紅蘿蔔 |
| 4~6 | 🍩 甜甜圈 |
| 7~10 | 🎂 蛋糕 |
| 11+ | 🍽️ 大餐 |

**進食循環**：
```
SNIFFING (嗅) → EATING (吃) → DIGESTING (消化) → 🚽 TOILET (拉) → RESTING (飽)
```

### 💩 便便 WiFi 惡作劇
小豬自己會**廣播假 WiFi 熱點**，SSID 每 5 秒輪換：
- `Pile of Poo 💩`
- `Free Poo WiFi`
- `PoopNet-2.4G`
- `💩 Hotspot 💩`
- `Smelly WiFi`
- `NotAPoo`
- `Poop Guest`

**上廁所時還會換成**：
- `🚽 In the loo...`
- `🚽 BRB 💩`

> 鄰居掃 WiFi 會看到便便一直在變 😂

### 🚽 廁所動畫
進食後進入 4 秒廁所場景：
1. 🚪 白門 + WC 標牌
2. 🐷 豬屁股蹲上馬桶
3. 😖 用力表情（臉漲紅 + 抖動 + 汗滴）
4. 💩 投彈動畫
5. 💦 水花四濺
6. 🌀 螺旋沖水

### 📐 IMU 控制
- **傾斜板子** → 眼睛跟隨、3D 旋轉
- **甩一下** → 切換情緒
- **劇烈搖晃** → 暈眩 3 秒

### 👆 觸控互動
- **單擊** → 眼睛看向手指
- **雙擊** → 驚訝
- **長按 1.5 秒** → 瞇眼
- **長按 3 秒** → 暈眩
- **點右上角** → 切換 PIG / 3D / PIG3
- **點 COLD / HEAT** → 冷熱模式切換

---

## 🛠️ 硬體需求

| 零件 | 型號 |
|------|------|
| 開發板 | **Waveshare ESP32-S3-Touch-LCD-2** |
| 螢幕 | ST7789 240×320 |
| 觸控 | CST816S（I2C） |
| IMU | QMI8658（I2C） |

### 引腳對應

```
螢幕：
  SCLK = 39   MOSI = 38   MISO = -1
  CS   = 45   DC   = 42   RST  = -1
  BL   = 1

觸控（I2C）：
  SDA = 48   SCL = 47   addr = 0x15

IMU（共用 I2C）：
  SDA = 48   SCL = 47   addr = 0x6B
```

---

## 🚀 安裝

### 1. 安裝 PlatformIO

如果還沒裝，先安裝 [VS Code](https://code.visualstudio.com/) + [PlatformIO 擴充套件](https://platformio.org/install/ide?install=vscode)。

### 2. Clone 專案

```bash
git clone https://github.com/你的帳號/wifipig.git
cd wifipig
```

### 3. 建置與上傳

```bash
pio run -t upload
```

或直接在 VS Code 點左下角 **→ (Upload)** 按鈕。

---

## 📁 專案結構

```
wifipig/
├── include/
│   └── WiFiFood.h        # WiFi 掃描 / 進食 / 廁所 / 便便廣播模組
├── src/
│   └── main.cpp          # 主程式：小豬 + 3D + 冷熱 + GUI
├── platformio.ini        # PlatformIO 配置
├── .gitignore
└── README.md
```

---

## ⚙️ 配置

### 修改便便 SSID

編輯 `include/WiFiFood.h`：

```cpp
const char* POO_SSIDS[] = {
  "Pile of Poo 💩",
  "Free Poo WiFi",
  // 加更多...
};
```

### 修改進食時間

在 `include/WiFiFood.h` 頂部：

```cpp
constexpr uint32_t EAT_DURATION    = 3000;   // 吃 3 秒
constexpr uint32_t DIGEST_DURATION = 4000;   // 消化 4 秒
constexpr uint32_t TOILET_DURATION = 4000;   // 廁所 4 秒
constexpr uint32_t REST_DURATION   = 8000;   // 飽足 8 秒
constexpr uint32_t SCAN_INTERVAL   = 3000;   // 掃描間隔
```

### 修改豬臉大小

在 `src/main.cpp` 的 `initEyes()`：

```cpp
eyes.pig.cy = 108;   // 垂直位置
eyes.pig.r  = 72;    // 半徑
```

---

## 📊 Serial Monitor 輸出

```
=== 大臉小豬 + 便便 WiFi ===
[IMU] QMI8658 OK
[3D Pig] 104 verts, 156 tris
[💩] AP: "Pile of Poo 💩"  IP=192.168.4.1
[Food] Sniffing...
[Food] Found 12 APs (strongest -45)
[Food] Eating...
[💩] Now broadcasting: "Free Poo WiFi"
[Food] Digesting...
[Food] Going to TOILET 🚽
[💩] Now broadcasting: "🚽 In the loo..."
[Food] Done in loo, resting...
[Food] Hungry again
```

---

## ⚠️ 注意事項

1. **法律提醒**：純搞笑的 SSID（如 `Pile of Poo`）安全。**請勿**使用真實品牌名（Starbucks、TP-Link）或假裝政府/醫院熱點，可能構成詐騙。
2. **路徑限制**：專案路徑**不能有中文或空格**，否則 Xtensa 工具鏈會編譯失敗。建議 `C:\PlatformIO\Projects\wifipig`。
3. **OneDrive 衝突**：不要把專案放在 OneDrive 同步資料夾內，`.map` 檔案會被鎖住導致 linker 失敗。
4. **Emoji SSID**：某些老舊設備不支援 emoji，會顯示為 `???`。可加 ASCII 備用。

---

## 🗺️ Roadmap

- [x] 大臉小豬 15 種情緒
- [x] 3D 方塊 / 3D 小豬
- [x] IMU 傾斜控制
- [x] 觸控互動
- [x] WiFi 掃描當食物
- [x] 便便 WiFi 廣播
- [x] 廁所動畫
- [ ] 藍牙版本
- [ ] 多語言 UI
- [ ] 語音互動
- [ ] 手機 App 控制

---

## 📜 License

MIT License — 隨意使用、修改、散布。

---

## 🙏 致謝

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX) — 強大的 ESP32 繪圖庫
- [QMI8658-Arduino-Library](https://github.com/lahavg/QMI8658-Arduino-Library) — IMU 驅動
- [Waveshare](https://www.waveshare.com/) — ESP32-S3-Touch-LCD-2 開發板

---

<p align="center">
  <b>🐷 Made with ❤️ and 💩</b>
</p>
