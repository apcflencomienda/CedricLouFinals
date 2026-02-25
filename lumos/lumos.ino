// ============================================
// Lumos: The AI Environment Companion
// Arduino Uno R4 WiFi Sketch
// ============================================

// ============================================
// SENSOR SELECTION — choose ONE temperature source
// Comment out USE_DHT11 to use the Thermistor (default)
// Uncomment USE_DHT11 to use the DHT11 sensor instead
// ============================================


#include "Arduino_LED_Matrix.h"
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <WiFiS3.h>



// ============================================
// CONFIGURATION - UPDATE THESE!
// ============================================
const char *WIFI_SSID = "PLDTHOMEFIBRspM8A";
const char *WIFI_PASSWORD = "Superadm!n1234";
const char *SERVER_IP = "192.168.1.9"; // Your PC's local IP
const int SERVER_PORT = 80;
const String API_BASE = "/lumos/"; // Path on server

// ============================================
// PIN DEFINITIONS
// ============================================

// Sensors
const int PHOTORESISTOR_PIN = A0; // Photoresistor (Light)
const int THERMISTOR_PIN = A1;    // Thermistor (Temperature)
const int BUTTON_PIN = 2;         // Push Button

// Outputs
const int RGB_RED_PIN = 11;   // RGB LED Red
const int RGB_GREEN_PIN = 10; // RGB LED Green
const int RGB_BLUE_PIN = 9;   // RGB LED Blue
const int BUZZER_PIN = 13;    // Active Buzzer

// Thermistor: set R0 to the nominal resistance of YOUR thermistor at 25°C
// Arduino basic kit = 10kΩ NTC (model: MF52-103)
const float THERMISTOR_R0 = 10000.0;  // 10kΩ NTC (standard Arduino kit thermistor)
const float THERMISTOR_B  = 3950.0;   // Beta coefficient

// ============================================
// GLOBALS
// ============================================
ArduinoLEDMatrix matrix;
WiFiClient wifi;
HttpClient http = HttpClient(wifi, SERVER_IP, SERVER_PORT);

unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 10000; // 10 seconds
bool lastButtonState = HIGH;
unsigned long lastButtonPressTime = 0;

// Current command from AI
String currentMessage = "Lumos Ready!";
String currentColor = "#4488FF";
bool currentBuzzer = false;

// LED Matrix frame patterns
const uint32_t HAPPY_FRAME[3] = {0x1C8A4BB2, 0x00000000, 0x00000000};

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("  Lumos - AI Environment Companion");
  Serial.println("================================");

  // Initialize LED Matrix
  matrix.begin();

  // Initialize pins
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);


  Serial.println("[SENSOR] Using Thermistor for temperature");

  // Start with blue LED
  setRGB(0x44, 0x88, 0xFF);

  // Connect to WiFi
  connectWiFi();

  // Show ready message on matrix
  showTextOnMatrix("Lumos Ready!");

  Serial.println("Setup complete! Starting main loop...\n");
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
  unsigned long currentTime = millis();



  // Debug: print raw button pin state every 3 seconds
  static unsigned long lastBtnDebug = 0;
  if (currentTime - lastBtnDebug >= 3000) {
    Serial.print("[BTN] Pin 2 raw state: ");
    Serial.println(digitalRead(BUTTON_PIN) == LOW ? "LOW (pressed)" : "HIGH (not pressed)");
    lastBtnDebug = currentTime;
  }

  // Check button press (with strict 5-second debounce)
  bool buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {
    if (currentTime - lastButtonPressTime > 5000) {
      Serial.println("[BUTTON] Pressed! Sending sensor data now...");
      readAndSendSensors();
      lastButtonPressTime = currentTime;
      lastSendTime = currentTime; // reset auto-send timer
    } else {
      Serial.println("[BUTTON] Ignored (Please wait 5s between presses)");
    }
  }
  lastButtonState = buttonState;

  // Poll for command updates (every 5 seconds)
  static unsigned long lastPollTime = 0;
  if (currentTime - lastPollTime >= 5000) {
    pollForCommand();
    lastPollTime = currentTime;
  }

  delay(100);
}

// ============================================
// WIFI CONNECTION
// ============================================
void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi FAILED! Check credentials.");
    showTextOnMatrix("WiFi Error");
  }
}

// ============================================
// READ SENSORS & SEND TO SERVER
// ============================================
void readAndSendSensors() {
  // Read photoresistor (light level as percentage)
  // Wiring: GND → LDR → A0 → 10kΩ → 5V
  // Bright = LDR low resistance = lower A0 voltage = lower raw value → invert
  int lightRaw = analogRead(PHOTORESISTOR_PIN);
  float lightVoltage = lightRaw * (5.0 / 1023.0);
  float lightPercent = (1.0 - (lightRaw / 1023.0)) * 100.0; // Inverted: low raw = bright

  // Read Thermistor temperature (Celsius, using voltage divider)
  // Wiring: 5V → Thermistor → A1 → R_FIXED(10kΩ) → GND
  int thermistorRaw = analogRead(THERMISTOR_PIN);
  float thermVoltage = thermistorRaw * (5.0 / 1023.0);
  float R_FIXED = 10000.0; // 10kΩ resistor from A1 to GND
  // Clamp to avoid division by zero
  if (thermVoltage < 0.01) thermVoltage = 0.01;
  if (thermVoltage > 4.99) thermVoltage = 4.99;
  // Thermistor on TOP: 5V → Thermistor → A1 → 10kΩ → GND
  // Rt = R_fixed * (Vcc - Va) / Va
  float resistance = R_FIXED * (5.0 - thermVoltage) / thermVoltage;
  // Steinhart-Hart simplified (Beta equation)
  float steinhart;
  steinhart = resistance / THERMISTOR_R0;        // (R/Ro)
  steinhart = log(steinhart);                    // ln(R/Ro)
  steinhart /= THERMISTOR_B;                     // 1/B * ln(R/Ro)
  steinhart += 1.0 / (25.0 + 273.15);            // + (1/To), To=298.15K
  steinhart = 1.0 / steinhart;                   // Invert
  float temperature = steinhart - 273.15;        // Convert to Celsius

  Serial.println("--- Sensor Reading ---");
  Serial.print("Thermistor Raw: "); Serial.println(thermistorRaw);
  Serial.print("Thermistor Voltage: "); Serial.println(thermVoltage, 3);
  Serial.print("Thermistor Resistance: "); Serial.print(resistance, 1); Serial.println(" ohm");
  Serial.print("Temperature: "); Serial.print(temperature, 1); Serial.println(" C");
  if (temperature < -10.0 || temperature > 60.0) {
    Serial.println("[WARN] Temperature out of range — check thermistor wiring/R0 constant!");
    Serial.print("  Expected R at 25C: "); Serial.println(THERMISTOR_R0);
    Serial.print("  Measured R: "); Serial.println(resistance, 1);
    Serial.println("  If temp is too cold, increase THERMISTOR_R0 to 100000.0");
  }
  Serial.print("Photoresistor Raw: "); Serial.println(lightRaw);
  Serial.print("Photoresistor Voltage: "); Serial.println(lightVoltage, 3);
  Serial.print("Light Level: "); Serial.print(lightPercent, 1); Serial.println(" %");

  // Send to server
  sendSensorData(temperature, lightPercent);
}

// ============================================
// SEND SENSOR DATA VIA HTTP POST
// ============================================
void sendSensorData(float temp, float light) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi not connected. Reconnecting...");
    connectWiFi();
    return;
  }

  // Build JSON payload
  String jsonPayload = "{\"temperature\":" + String(temp, 1) +
                       ",\"light_level\":" + String(light, 1) + "}";

  Serial.print("[HTTP] POST sensor data: ");
  Serial.println(jsonPayload);

  http.beginRequest();
  http.post(API_BASE + "api_sensor.php");
  http.sendHeader("Content-Type", "application/json");
  http.sendHeader("Content-Length", jsonPayload.length());
  http.beginBody();
  http.print(jsonPayload);
  http.endRequest();

  int statusCode = http.responseStatusCode();
  String response = http.responseBody();

  Serial.print("[HTTP] Response code: ");
  Serial.println(statusCode);
  Serial.print("[HTTP] Response body: ");
  Serial.println(response); // Always print full response for debugging

  if (statusCode == 200) {
    parseAndApplyCommand(response);
  } else {
    Serial.println("[ERROR] Server returned error");
  }
}

// ============================================
// POLL FOR LATEST COMMAND
// ============================================
void pollForCommand() {
  if (WiFi.status() != WL_CONNECTED)
    return;

  http.get(API_BASE + "api_get_command.php");

  int statusCode = http.responseStatusCode();
  String response = http.responseBody();

  Serial.print("[POLL] Status: ");
  Serial.print(statusCode);
  Serial.print(" | Body: ");
  Serial.println(response);

  if (statusCode == 200) {
    parseAndApplyCommand(response);
  }
}

// ============================================
// PARSE AI RESPONSE & APPLY COMMANDS
// ============================================
void parseAndApplyCommand(String jsonResponse) {
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, jsonResponse);

  if (error) {
    Serial.print("[JSON] Parse error: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract values
  const char *colorHex = doc["color_hex"] | "#4488FF";
  const char *message = doc["message"] | "Lumos Active";
  bool buzzer = doc["buzzer"] | false;

  // Update current state
  currentColor = String(colorHex);
  currentMessage = String(message);
  currentBuzzer = buzzer;

  Serial.println("--- AI Command ---");
  Serial.print("Color: ");
  Serial.println(currentColor);
  Serial.print("Message: ");
  Serial.println(currentMessage);
  Serial.print("Buzzer: ");
  Serial.println(currentBuzzer ? "ON" : "OFF");

  // Apply RGB color
  applyColor(currentColor);

  // Apply buzzer
  if (currentBuzzer) {
    buzzAlert();
  }

  // Show message on LED matrix
  showTextOnMatrix(currentMessage);
}

// ============================================
// RGB LED CONTROL
// ============================================
void setRGB(int r, int g, int b) {
  // Dim factor: 0.0 (off) to 1.0 (full brightness). Adjust as needed.
  const float BRIGHTNESS = 0.15;
  analogWrite(RGB_RED_PIN,   (int)(r * BRIGHTNESS));
  analogWrite(RGB_GREEN_PIN, (int)(g * BRIGHTNESS));
  analogWrite(RGB_BLUE_PIN,  (int)(b * BRIGHTNESS));
}

void applyColor(String hexColor) {
  // Parse "#RRGGBB" hex string
  if (hexColor.length() >= 7 && hexColor.charAt(0) == '#') {
    long color = strtol(hexColor.substring(1).c_str(), NULL, 16);
    int r = (color >> 16) & 0xFF;
    int g = (color >> 8) & 0xFF;
    int b = color & 0xFF;
    setRGB(r, g, b);
  }
}

// ============================================
// BUZZER CONTROL
// ============================================
void buzzAlert() {
  // Three short beeps for alert
  for (int i = 0; i < 3; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

// ============================================
// LED MATRIX DISPLAY
// ============================================
void showTextOnMatrix(String text) {
  Serial.print("[MATRIX] Showing: ");
  Serial.println(text);

  // Flash the matrix to acknowledge the message
  // Full bright for 300ms
  byte fullOn[8][12];
  for (int r = 0; r < 8; r++)
    for (int c = 0; c < 12; c++)
      fullOn[r][c] = 1;
  matrix.renderBitmap(fullOn, 8, 12);
  delay(300);

  // Clear
  byte off[8][12] = {};
  matrix.renderBitmap(off, 8, 12);
  delay(150);

  // Flash again
  matrix.renderBitmap(fullOn, 8, 12);
  delay(300);

  matrix.renderBitmap(off, 8, 12);
}
