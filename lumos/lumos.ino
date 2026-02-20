// ============================================
// Lumos: The AI Environment Companion
// Arduino Uno R4 WiFi Sketch
// ============================================

// ============================================
// SENSOR SELECTION — choose ONE temperature source
// Comment out USE_DHT11 to use the Thermistor (default)
// Uncomment USE_DHT11 to use the DHT11 sensor instead
// ============================================
// #define USE_DHT11

#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include "Arduino_LED_Matrix.h"
#include <ArduinoJson.h>

#ifdef USE_DHT11
  #include <DHT.h>
  #define DHT_PIN  A2        // DHT11 data pin
  #define DHT_TYPE DHT11
  DHT dht(DHT_PIN, DHT_TYPE);
#endif

// ============================================
// CONFIGURATION - UPDATE THESE!
// ============================================
const char* WIFI_SSID     = "RIP YES KING!!!!";
const char* WIFI_PASSWORD  = "hamburger";
const char* SERVER_IP      = "10.230.155.52";  // Your PC's local IP
const int   SERVER_PORT    = 80;
const String API_BASE      = "/lumos/";  // Path on server

// ============================================
// PIN DEFINITIONS
// ============================================
// Sensors
const int PHOTORESISTOR_PIN = A0;
const int THERMISTOR_PIN    = A1;  // Used when USE_DHT11 is NOT defined
const int BUTTON_PIN        = 2;

// Outputs
const int RGB_RED_PIN   = 9;
const int RGB_GREEN_PIN = 10;
const int RGB_BLUE_PIN  = 11;
const int BUZZER_PIN    = 7;

// ============================================
// THERMISTOR CONFIG (10k NTC)
// ============================================
const float THERMISTOR_NOMINAL  = 10000.0;
const float TEMPERATURE_NOMINAL = 25.0;
const float B_COEFFICIENT       = 3950.0;
const float SERIES_RESISTOR     = 10000.0;

// ============================================
// GLOBALS
// ============================================
ArduinoLEDMatrix matrix;
WiFiClient wifi;
HttpClient http = HttpClient(wifi, SERVER_IP, SERVER_PORT);

unsigned long lastSendTime = 0;
const unsigned long SEND_INTERVAL = 10000; // 10 seconds
bool lastButtonState = HIGH;

// Current command from AI
String currentMessage = "Lumos Ready!";
String currentColor = "#4488FF";
bool currentBuzzer = false;

// LED Matrix scrolling
const uint32_t SCROLL_DELAY = 100;
unsigned long lastScrollTime = 0;
int scrollPosition = 0;

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

#ifdef USE_DHT11
    dht.begin();
    Serial.println("[SENSOR] Using DHT11 for temperature");
#else
    Serial.println("[SENSOR] Using Thermistor for temperature");
#endif

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
    // Check button press
    bool buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == LOW && lastButtonState == HIGH) {
        Serial.println("[BUTTON] Pressed! Sending sensor data now...");
        readAndSendSensors();
        delay(300); // debounce
    }
    lastButtonState = buttonState;

    // Periodic sensor reading
    unsigned long currentTime = millis();
    if (currentTime - lastSendTime >= SEND_INTERVAL) {
        readAndSendSensors();
        lastSendTime = currentTime;
    }

    // Poll for command updates (every other cycle)
    static unsigned long lastPollTime = 0;
    if (currentTime - lastPollTime >= SEND_INTERVAL / 2) {
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
    int lightRaw = analogRead(PHOTORESISTOR_PIN);
    float lightPercent = (lightRaw / 1023.0) * 100.0;

    // --- Temperature Reading ---
    float temperature = NAN;

#ifdef USE_DHT11
    // DHT11 path
    temperature = dht.readTemperature();
    if (isnan(temperature)) {
        Serial.println("[DHT11] Failed to read temperature!");
        temperature = 0.0;
    }
#else
    // Thermistor path (Steinhart-Hart)
    int thermRaw = analogRead(THERMISTOR_PIN);
    float resistance = SERIES_RESISTOR / (1023.0 / thermRaw - 1.0);
    temperature = resistance / THERMISTOR_NOMINAL;
    temperature = log(temperature);
    temperature /= B_COEFFICIENT;
    temperature += 1.0 / (TEMPERATURE_NOMINAL + 273.15);
    temperature = 1.0 / temperature;
    temperature -= 273.15;
#endif

    Serial.println("--- Sensor Reading ---");
    Serial.print("Temperature: ");
    Serial.print(temperature, 1);
    Serial.println(" C");
    Serial.print("Light Level: ");
    Serial.print(lightPercent, 1);
    Serial.println(" %");

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
    String jsonPayload = "{\"temperature\":" + String(temp, 1) + ",\"light_level\":" + String(light, 1) + "}";

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

    if (statusCode == 200) {
        parseAndApplyCommand(response);
    } else {
        Serial.println("[ERROR] Server returned error");
        Serial.println(response);
    }
}

// ============================================
// POLL FOR LATEST COMMAND
// ============================================
void pollForCommand() {
    if (WiFi.status() != WL_CONNECTED) return;

    http.get(API_BASE + "api_get_command.php");

    int statusCode = http.responseStatusCode();
    String response = http.responseBody();

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
    const char* colorHex = doc["color_hex"] | "#4488FF";
    const char* message  = doc["message"] | "Lumos Active";
    bool buzzer          = doc["buzzer"] | false;

    // Update current state
    currentColor   = String(colorHex);
    currentMessage = String(message);
    currentBuzzer  = buzzer;

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
    analogWrite(RGB_RED_PIN, r);
    analogWrite(RGB_GREEN_PIN, g);
    analogWrite(RGB_BLUE_PIN, b);
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
// LED MATRIX TEXT DISPLAY
// ============================================
void showTextOnMatrix(String text) {
    // Use the built-in text scroll feature
    Serial.print("[MATRIX] Showing: ");
    Serial.println(text);

    // The Arduino R4 LED Matrix can display scrolling text
    matrix.beginDraw();
    matrix.stroke(0xFFFFFFFF);
    matrix.textScrollSpeed(50);

    // Set text to scroll
    const char* cText = text.c_str();
    matrix.textFont(Font_5x7);
    matrix.beginText(0, 1, 0xFFFFFF);
    matrix.println(cText);
    matrix.endText(SCROLL_LEFT);

    matrix.endDraw();
}
