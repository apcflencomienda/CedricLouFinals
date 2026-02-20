# 💡 Lumos: The AI Environment Companion

A portable, AI-driven workspace companion powered by the **Arduino Uno R4 WiFi**.

Unlike static sensor kits with fixed thresholds, Lumos uses **Google Gemini AI** to dynamically adapt to your environment. Tell it where you are, and it adjusts its behavior accordingly.

---

## 🏗️ Project Structure

```
CedricLouFinals/
├── lumos/                  # Arduino sketch
│   └── lumos.ino
├── web/                    # Web dashboard (deploy to XAMPP)
│   ├── index.html          # Dashboard UI
│   ├── style.css           # Custom styles
│   ├── app.js              # Frontend logic
│   ├── db.php              # Database config + API key
│   ├── api_gemini.php      # Gemini API helper
│   ├── api_sensor.php      # Sensor data endpoint
│   ├── api_chat.php        # Chat endpoint
│   ├── api_get_command.php  # Arduino command polling
│   └── api_history.php     # History endpoint
├── database/
│   └── lumos_db.sql        # MySQL schema
└── README.md
```

---

## 🚀 Setup Guide

### 1. Database Setup
1. Open **phpMyAdmin** at `http://localhost/phpmyadmin`
2. Click **Import** → select `database/lumos_db.sql` → click **Go**
3. Verify the `lumos_db` database has 4 tables

### 2. Web Dashboard
1. Copy the entire `web/` folder to `C:\xampp\htdocs\lumos\`
2. Edit `C:\xampp\htdocs\lumos\db.php`
3. Replace `YOUR_GEMINI_API_KEY_HERE` with your key from [aistudio.google.com](https://aistudio.google.com/apikey)
4. Open browser → `http://localhost/lumos/`

### 3. Arduino Setup
1. Open `lumos/lumos.ino` in **Arduino IDE**
2. Update these lines with your WiFi and PC's IP:
   ```cpp
   const char* WIFI_SSID     = "YOUR_WIFI_SSID";
   const char* WIFI_PASSWORD  = "YOUR_WIFI_PASSWORD";
   const char* SERVER_IP      = "192.168.1.100";  // Your PC's local IP
   ```
3. Install required libraries: **ArduinoHttpClient**, **ArduinoJson**
   - If using DHT11: also install **DHT sensor library** by Adafruit (includes Adafruit Unified Sensor)
4. Select board: **Arduino Uno R4 WiFi**
5. To switch to DHT11: uncomment `// #define USE_DHT11` at the top of `lumos.ino`
6. Upload!

---

## 🔧 Hardware Wiring

| Component | Arduino Pin | Notes |
|---|---|---|
| Photoresistor | A0 | With 10kΩ resistor voltage divider |
| Thermistor | A1 | With 10kΩ resistor voltage divider *(default temp sensor)* |
| DHT11 *(optional)* | A2 | Single data wire + 10kΩ pull-up to 5V; uncomment `#define USE_DHT11` |
| Push Button | D2 | INPUT_PULLUP (other leg to GND) |
| RGB LED Red | D9 | Via ~220Ω resistor |
| RGB LED Green | D10 | Via ~220Ω resistor |
| RGB LED Blue | D11 | Via ~220Ω resistor |
| Active Buzzer | D7 | (+) to D7, (−) to GND |

---

## 👥 Team
- **Encomienda, Francis Frederick** — Hardware, Backend
- **Valdez, Jasper Lou** — AI, Frontend

*Embedded Systems Finals Project*
