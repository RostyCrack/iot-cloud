#include <Wire.h>

// Pin de conexión del LM35 y del LED
const int pinLM35 = A1;
const int pinLed  = 2;

// Dirección I2C del esclavo
const byte I2C_ADDRESS = 0x08;

// Umbral de temperatura en grados Celsius
const float TEMP_UMBRAL = 30.0;

// Variable global para almacenar la última temperatura leída
float temperatura = 0.0;

unsigned long previousMillis = 0;
const unsigned long intervalo = 1000; // 1 segundo

void setup() {
  Serial.begin(9600);
  pinMode(pinLed, OUTPUT);

  // Inicializar el esclavo I2C
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(enviarTemperatura); // Cuando el maestro solicita datos

  Serial.println("Esclavo I2C iniciado en dirección 0x08");
}

void loop() {
  unsigned long currentMillis = millis();

  // Cada segundo lee el sensor y actualiza la temperatura
  if (currentMillis - previousMillis >= intervalo) {
    previousMillis = currentMillis;

    // Leer el valor analógico del LM35
    int valorAnalogico = analogRead(pinLM35);

    // Convertir a temperatura (10 mV/°C)
    temperatura = (valorAnalogico * 5.0 * 100.0) / 1024.0;

    // Mostrar temperatura en el monitor serial
    Serial.print("Temperatura actual: ");
    Serial.print(temperatura);
    Serial.println(" °C");

    // Encender o apagar el LED según el umbral
    digitalWrite(pinLed, temperatura >= TEMP_UMBRAL ? HIGH : LOW);
  }
}

// Función llamada automáticamente cuando el maestro solicita datos
// Su objetivo es enviar la temperatura a través de I2C al Maestro
void enviarTemperatura() {
  // Convertir la temperatura a cadena con 2 decimales
  char buffer[10];
  dtostrf(temperatura, 5, 2, buffer);
  Wire.write(buffer);
}