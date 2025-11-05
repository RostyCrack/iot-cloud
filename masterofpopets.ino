#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>

// ==================== CONFIGURACIÓN I2C ====================
#define ARDUINO_SLAVE_ADDR 0x08  // Dirección del Arduino esclavo
#define SDA_PIN 21
#define SCL_PIN 22

// ==================== CONFIGURACIÓN DE HARDWARE ====================
const int LED_PIN = 13;   // LED indicador

// ==================== CREDENCIALES WIFI ====================
const char* WIFI_NAME = "MOVISTAR_FIBRA_29B0";
const char* WIFI_PASSWORD = "12345678";

// ==================== THINGSPEAK CONFIG (HTTP) ====================
const char* server = "http://api.thingspeak.com/update";
const char* apiKey = "94CYBVF7RWANLVQP";

// ==================== VARIABLES ====================
float receivedValue = 0.0;
unsigned long previousMillis = 0;
const long interval = 15000; // ThingSpeak requiere 15 seg entre envíos

// ==================== FSM ====================
enum State {
  STATE_CONNECT_WIFI,
  STATE_READ_I2C,
  STATE_CHECK_ALERT,
  STATE_PUBLISH_HTTP,
  STATE_IDLE
};

State currentState = STATE_CONNECT_WIFI;

// ==================== FUNCIONES ====================
float leerDatoDesdeArduino() {
  Wire.requestFrom(ARDUINO_SLAVE_ADDR, 10);  // solicita hasta 10 bytes
  String data = "";

  while (Wire.available()) {
    char c = Wire.read();
    data += c;
  }

  if (data.length() > 0) {
    float valor = data.toFloat();
    Serial.printf("📩 Dato recibido del Arduino: %.2f\n", valor);
    return valor;
  } else {
    Serial.println("⚠️ No se recibió dato del Arduino.");
    return NAN;
  }
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // I2C como maestro
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("🔌 ESP32 iniciado como Maestro I2C");

  // WiFi
  WiFi.mode(WIFI_STA);
}

// ==================== LOOP ====================
void loop() {
  switch (currentState) {

    // -------------------- Conectar WiFi --------------------
    case STATE_CONNECT_WIFI:
      Serial.println("STATE_CONNECT_WIFI");

      if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Conectando a WiFi...");
        WiFi.begin(WIFI_NAME, WIFI_PASSWORD);

        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
          delay(500);
          Serial.print(".");
          attempts++;
        }

        if (WiFi.status() == WL_CONNECTED) {
          Serial.println("\n✅ WiFi conectado: " + String(WiFi.localIP()));
          currentState = STATE_READ_I2C;
        } else {
          Serial.println("\n⚠️ Falló conexión WiFi. Reintentando...");
          delay(2000);
        }
      } else {
        currentState = STATE_READ_I2C;
      }
      break;

    // -------------------- Leer del Arduino --------------------
    case STATE_READ_I2C:
      Serial.println("STATE_READ_I2C");
      receivedValue = leerDatoDesdeArduino();

      if (!isnan(receivedValue)) {
        currentState = STATE_CHECK_ALERT;
      } else {
        delay(1000);
      }
      break;

    // -------------------- Revisar umbral --------------------
    case STATE_CHECK_ALERT:
      Serial.println("STATE_CHECK_ALERT");

      if (receivedValue > 30.0) {
        digitalWrite(LED_PIN, HIGH);
      } else {
        digitalWrite(LED_PIN, LOW);
      }

      currentState = STATE_PUBLISH_HTTP;
      break;

    // -------------------- Enviar a ThingSpeak --------------------
    case STATE_PUBLISH_HTTP:
      Serial.println("STATE_PUBLISH_HTTP");

      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;

        String url = String(server) + "?api_key=" + apiKey + "&field1=" + String(receivedValue, 2);
        Serial.println("🌍 Enviando a ThingSpeak:");
        Serial.println(url);

        http.begin(url);
        int httpCode = http.GET();

        if (httpCode > 0) {
          Serial.printf("✅ HTTP %d OK\n", httpCode);
        } else {
          Serial.printf("❌ Error HTTP: %s\n", http.errorToString(httpCode).c_str());
        }

        http.end();
      } else {
        Serial.println("⚠️ WiFi desconectado, reintentando...");
        currentState = STATE_CONNECT_WIFI;
        break;
      }

      previousMillis = millis();
      currentState = STATE_IDLE;
      break;

    // --------------------  Esperar siguiente ciclo --------------------
    case STATE_IDLE:
      if (millis() - previousMillis >= interval) {
        currentState = STATE_READ_I2C;
      }
      break;
  }
}