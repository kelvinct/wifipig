# 🐷 WiFi Pig

> 一隻會吃 WiFi、拉便便、噴火、撒嬌、生氣、追飛蟲的桌面小豬。

**ESP32-S3-Touch-LCD-2** 上的互動式智能寵物 — 具備 19 種情緒、3D 視圖、IMU 傾斜控制、觸控互動、飛蟲追蹤、晝夜系統、火焰噴射、便便 WiFi 惡作劇、以及會自己找事做的 AI 行為。

<p align="center">
  <img src="https://img.shields.io/badge/ESP32--S3-Touch--LCD--2-blue" alt="Board">
  <img src="https://img.shields.io/badge/LovyanGFX-1.1.16+-green" alt="LovyanGFX">
  <img src="https://img.shields.io/badge/PlatformIO-Arduino-orange" alt="PlatformIO">
  <img src="https://img.shields.io/badge/version-v4.0-purple" alt="Version">
  <img src="https://img.shields.io/badge/license-MIT-yellow" alt="License">
</p>

---

## ✨ 功能總覽

| 分類 | 功能 |
|------|------|
| 🐷 **情緒系統** | 19 種情緒、優先級互斥、平滑過渡 |
| 🧠 **智能寵物** | 無聊行為、打招呼、愛意值、手指追蹤、起床氣 |
| 🐝 **飛蟲系統** | 追蹤鎖定、瞳孔縮放、打蟲遊戲、隨機反應 |
| 🎲 **3D 視圖** | 彩虹方塊、低多邊形小豬、IMU 即時旋轉 |
| 🌅 **晝夜系統** | NTP 真實時間 or 模擬時間、背景變色、太陽/月亮 |
| 💩 **便便 WiFi** | 廣播假熱點、輪換 SSID、廁所動畫 |
| 🔥 **火焰噴射** | HEAT 模式豬嘴吐火、向四周擴散 |
| 🎮 **觸控互動** | 單擊、雙擊、長按、拖動追蹤 |
| 📐 **IMU 控制** | 傾斜跟隨、甩動切換、搖晃暈眩 |
| 🥚 **隱藏彩蛋** | 10/25/50 殺飛蟲成就 |

---

## 🧠 智能寵物系統

小豬不再只是被動反應 — 它會**自己找事做**：

### 🎭 無聊行為（5 秒沒互動）
- 👀 **四處張望**：眼睛隨機飄移
- 🥱 **打哈欠**：瞇眼 + 慢慢睜開
- 🤔 **假裝思考**：`?` 氣泡飄動
- 😊 **撒嬌看你**：開心 + 看你一眼
- 💤 **愛睏**：半閉眼 + 想睡

### 🐶 打招呼
超過 **30 秒沒互動**後再摸 → 豬超開心迎接：
- 彎月眼 `^ ^`
- 粉紅 + 金黃粒子爆發
- 2 秒超高興狀態

### 💕 愛意值系統
隱藏數值 0~100，決定行為親密度：
| 愛意值 | 等級 | 行為 |
|--------|------|------|
| 0-5 | WARY | 冷淡，只四處看 |
| 5-20 | STRANGER | 陌生人，開始撒嬌 |
| 20-50 | BUDDY | 伙伴，會打哈欠 |
| 50-80 | FRIEND | 朋友，更多愛心 |
| 80+ | SOULMATE | 靈魂伴侶，超黏人 |

**愛意值來源：**
- 觸控 +0.05
- 打招呼 +0.3
- 打死飛蟲 +5
- 自然衰減 -0.02/秒

### 👆 手指追蹤
手指在螢幕上**滑動**時眼睛持續跟著，停 300ms 後停止追蹤。

### 😾 起床氣
從睡眠（ASLEEP）醒來後 **3 秒內觸控**，豬會：
- 60% 😠 生氣 `DON'T WAKE ME!`
- 25% 😒 不爽
- 15% 💤 裝睡不理你

---

## 🐝 飛蟲系統 v2

惱人的飛蟲會繞著豬臉飛，豬的反應：

### 🎯 追蹤 v2（眼睛鎖定）
- **逐帧追蹤**：100ms → 33ms，瞬間鎖定
- **眼球移動 ×10**：蟲飛哪看哪
- **豬頭轉向 ×8**：頭也跟著轉
- **瞳孔縮放**：蟲靠近（<50px）放大 1.25x
- **雙高光**：蟲近時加第二個白色高光
- **鎖定效果**：蟲消失後仍看 500ms

### 😠 被煩情緒
飛蟲出現 → 自動進入 `ANNOYED`：
- 瞇眼 + 皺眉
- 雙手揮動趕蟲
- `ANNOYED...` 文字抖動

### 🎊 打蟲遊戲
用手指點飛蟲（命中半徑 20px）→ 隨機反應：
| 機率 | 反應 |
|------|------|
| 70% | 😄 超開心（彈跳 + 粒子） |
| 18% | 💕 愛心眼 |
| 8% | 👑 驕傲（+皇冠） |
| 4% | 😳 害羞 |

### 🥚 隱藏彩蛋
| 成就 | 觸發 | 效果 |
|------|------|------|
| 🥚 10 殺 | 打死 10 隻 | 驕傲 + 皇冠 + 金粒子 |
| 🥚 25 殺 | 打死 25 隻 | 愛心 + 粉粒子 |
| 🥚 50 殺 | 打死 50 隻 | 大開心 + 雙重粒子 |

---

## 🎨 19 種情緒

| 情緒 | 觸發 | 視覺 |
|------|------|------|
| NEUTRAL | 預設 | 呼吸 + 隨機眨眼 |
| HAPPY | 聊天 / 打蟲 | 彎月眼 + 粒子 |
| LOVE | 雙擊 / 打蟲 | 愛心眼 + 心跳 |
| SURPRISED | 雙擊 / 醒來 | 大眼 + 星光 |
| SLEEPING | 靜置 40 秒 | 閉眼 + Z 飄 |
| SQUINT | 長按 1.5 秒 | 半閉眼 |
| DIZZY | 劇烈搖晃 | 螺旋眼 + 星星 |
| COLD | 點 COLD | 冰藍 + 雪花 + BRRR |
| HEAT | 點 HEAT | **🔥 火焰噴射** |
| ANNOYED | 飛蟲出現 | 瞇眼 + 皺眉 + 揮手 |
| ANGRY | 甩動 / 起床氣 | 紅臉 + 生氣眉 |
| SAD | 甩動循環 | 灰色 + 眼淚 |
| THINKING | 無聊時 | 眼睛漂移 + ? 氣泡 |
| SHY | 雙擊 / 打蟲 | 眼睛往下看 + 臉紅 |
| PROUD | 打蟲成就 | 大眼睛 + 星芒 + 皇冠 |
| OK | 特殊觸發 | 綠色勾勾動畫 |
| FOCUS | 內部使用 | 瞳孔縮小 |
| HUNGRY | 內部使用 | 橘色 |
| SICK | 內部使用 | 灰色 |

---

## 🌅 晝夜系統

### 兩種時間模式

**模擬（預設）**：
- 開機 = 08:00
- 每 5 分鐘真實時間 = 1 遊戲小時
- 2 小時循環一天

**NTP 真實時間**（填 WiFi 帳密）：
```cpp
#define TIME_WIFI_SSID "你的WiFi名稱"
#define TIME_WIFI_PASS "你的密碼"
```

### 時段效果
| 時段 | 小時 | 背景 | 圖示 | 豬行為 |
|------|------|------|------|--------|
| 清晨 | 05-07 | 橘 `0xFD20` | 🌅 | 醒來 |
| 白天 | 07-17 | 深藍灰 `0x1082` | ☀️ | 活潑 |
| 黃昏 | 17-19 | 橘 `0xFD20` | 🌇 | 慢慢安靜 |
| 夜晚 | 19-05 | 純黑 + 星空 | 🌙 | 20 秒自動睡 |

---

## 🔥 火焰噴射

按 **HEAT** 按鈕 → 豬嘴開始噴火：

- 🐷 從嘴巴噴出（`pig.cy + pig.r - 22`）
- 🌪️ 向四面八方擴散（0~2π 隨機）
- ⬆️ 稍微往上飄
- 🎨 顏色漸變：**白 → 亮黃 → 橙 → 紅**
- ✨ 大火焰有白色內核
- ⏱️ 每顆存活 20~40 幀

---

## 📶 WiFi 當食物

小豬會**掃描附近 WiFi**當作食物：

| WiFi 數量 | 食物 |
|-----------|------|
| 0 | 沒食物 😢 |
| 1~3 | 🥕 紅蘿蔔 |
| 4~6 | 🍩 甜甜圈 |
| 7~10 | 🎂 蛋糕 |
| 11+ | 🍽️ 大餐 |

### 進食循環
```
SNIFFING (嗅) → EATING (吃) → DIGESTING (消化) → 🚽 TOILET (拉) → RESTING (飽)
```

---

## 💩 便便 WiFi 惡作劇

小豬自己會**廣播假 WiFi 熱點**，SSID 每 5 秒輪換：

- `Pile of Poo 💩`
- `Free Poo WiFi`
- `PoopNet-2.4G`
- `💩 Hotspot 💩`
- `Smelly WiFi`
- `NotAPoo`
- `Poop Guest`
- `💩💩💩`

**上廁所時還會換成**：
- `🚽 In the loo...`
- `🚽 BRB 💩`

> 鄰居掃 WiFi 會看到便便一直在變 😂

---

## 🚽 廁所動畫

進食後進入 **4 秒廁所場景**：

1. 🚪 白門 + WC 標牌
2. 🐷 豬屁股蹲上馬桶
3. 😖 用力表情（臉漲紅 + 抖動 + 汗滴）
4. 💩 投彈動畫
5. 💦 水花四濺
6. 🌀 螺旋沖水

---

## 🎲 3D 視圖

按右上角按鈕循環切換三種模式：

| 模式 | 說明 |
|------|------|
| **PIG** | 大臉豬（情緒系統） |
| **3D CUBE** | 彩虹線框方塊 |
| **3D PIG** | 低多邊形 3D 豬 |

**特色：**
- 🌈 彩虹色隨時間循環
- 📐 透視投影
- 💡 深度著色（遠暗近亮）
- 🎯 隨板子傾斜即時旋轉

---

## 📐 IMU 控制

- **傾斜板子** → 眼睛跟隨、3D 旋轉
- **甩一下** → 隨機切換情緒
- **劇烈搖晃** → 暈眩 3 秒

---

## 👆 觸控互動

| 動作 | 效果 |
|------|------|
| **單擊** | 眼睛看向手指 |
| **雙擊** | 驚訝 / 害羞 / 開心（隨機） |
| **長按 1.5 秒** | 瞇眼 |
| **長按 3 秒** | 暈眩 |
| **點飛蟲** | 打死飛蟲（隨機反應） |
| **點右上角** | 切換視圖 |
| **點 COLD / HEAT** | 冷熱模式 |
| **拖動** | 眼睛持續追蹤 |

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
先安裝 [VS Code](https://code.visualstudio.com/) + [PlatformIO 擴充套件](https://platformio.org/install/ide?install=vscode)。

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
│   ├── WiFiFood.h        # WiFi 掃描 / 進食 / 廁所 / 便便廣播
│   └── FlyBug.h          # 飛蟲系統（含打中檢測）
├── src/
│   └── main.cpp          # 主程式（豬臉 + 3D + 智能寵物 + 火焰）
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
```cpp
constexpr uint32_t EAT_DURATION    = 3000;   // 吃 3 秒
constexpr uint32_t DIGEST_DURATION = 4000;   // 消化 4 秒
constexpr uint32_t TOILET_DURATION = 4000;   // 廁所 4 秒
constexpr uint32_t REST_DURATION   = 8000;   // 飽足 8 秒
```

### 修改飛蟲參數
`include/FlyBug.h`：
```cpp
constexpr int   MAX_FLIES        = 3;       // 最多幾隻蟲
constexpr uint32_t SPAWN_MIN_MS  = 2000;
constexpr uint32_t SPAWN_MAX_MS  = 6000;
```

### 修改火焰
`src/main.cpp` 的 `drawHeatEffect()`：
```cpp
float speed = random(20, 60) / 10.0f;   // 噴速
int spawnCount = random(2, 4);           // 每幀生成數量
```

### 修改時間流速
`updateTime()`：
```cpp
hourNow = (8 + (elapsed / 300000)) % 24;  // 每 5 分鐘 = 1 小時
// 想更快 → 改 60000（每分鐘 = 1 小時）
```

---

## 📊 Serial Monitor 輸出

```
=== 大臉小豬 v4 — 智能寵物版 ===
[IMU] QMI8658 OK
[3D Pig] 104 verts, 156 tris
[💩] AP: "Pile of Poo 💩"  IP=192.168.4.1
[🐝] FlyBug started
Ready!
[🧠] Look around              ← 無聊行為
[🧠] Yawn
[🐝] New fly #0
[🐷] Fly killed! total=1 love=5.1
[🐶] WELCOME BACK!            ← 打招呼
[😾] Wake rage window open    ← 起床氣窗口
[😠] DON'T WAKE ME!
🥚 EGG: 10 flies killed! PROUD!
```

---

## ⚠️ 注意事項

1. **法律提醒**：純搞笑的 SSID（如 `Pile of Poo`）安全。**請勿**使用真實品牌名（Starbucks、TP-Link）或假裝政府/醫院熱點，可能構成詐騙。
2. **路徑限制**：專案路徑**不能有中文或空格**，否則 Xtensa 工具鏈會編譯失敗。建議 `C:\PlatformIO\Projects\wifipig`。
3. **OneDrive 衝突**：不要把專案放在 OneDrive 同步資料夾內，`.map` 檔案會被鎖住導致 linker 失敗。
4. **Emoji SSID**：某些老舊設備不支援 emoji，會顯示為 `???`。可加 ASCII 備用。
5. **監視器波特率**：必須 **115200**，否則會看到亂碼。
6. **如果看不到開機訊息**：在 `platformio.ini` 加了 `monitor_rts = 0` 和 `monitor_dtr = 0` 後，打開監視器不會重置板子，要**手動按 RST 按鈕**才能看到啟動訊息。

---

## 🗺️ Roadmap

- [x] 大臉小豬 19 種情緒
- [x] 3D 方塊 / 3D 小豬
- [x] IMU 傾斜控制
- [x] 觸控互動
- [x] WiFi 掃描當食物
- [x] 便便 WiFi 廣播
- [x] 廁所動畫
- [x] 飛蟲追蹤 v2（鎖定 + 瞳孔縮放）
- [x] 打蟲遊戲 + 彩蛋
- [x] 晝夜系統
- [x] 火焰噴射
- [x] 智能寵物（無聊行為 / 打招呼 / 愛意值 / 起床氣）
- [x] 情緒優先級系統
- [ ] 藍牙版本
- [ ] 語音互動
- [ ] 手機 App 控制
- [ ] 飛蟲種類多樣化
- [ ] 存檔系統（斷電記憶愛意值）

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
  <b>🐷 Made with ❤️ 💩 🔥 and 🐝</b>
</p>
