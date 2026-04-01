#include <WiFi.h>
#include <WebServer.h>
#include <BluetoothSerial.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// Pines
#define PIN_BZ 2
#define PIN_RED 4
#define PIN_GN 16
#define PIN_POT 34

BluetoothSerial SerialBT;
WebServer server(80);
Adafruit_MPU6050 mpu;

// Variables
float mag = 0, umbral = 4.5;
String statusIA = "ESTRUCTURA SEGURA";

// PWM
const int ch_bz = 0;
const int ch_led = 1;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("GeoNode-VZ_BT");

  // PWM
  ledcSetup(ch_bz, 5000, 8);
  ledcAttachPin(PIN_BZ, ch_bz);

  ledcSetup(ch_led, 5000, 8);
  ledcAttachPin(PIN_RED, ch_led);

  pinMode(PIN_GN, OUTPUT);

  // WiFi Triple
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("JOSE89_EMERGENCIA");
  WiFi.begin("Colorina2 2", "Coraje198624");

  // MPU
  Wire.begin(21, 22);
  if (!mpu.begin()) {
    Serial.println("Error MPU");
    while (1);
  }

  // WEB ligera
  server.on("/data", []() {
    String json = "{\"mag\":" + String(mag) +
                  ",\"status\":\"" + statusIA +
                  "\",\"umbral\":" + String(umbral) + "}";
    server.send(200, "application/json", json);
  });

  server.begin();
}

void loop() {
  server.handleClient();

  sensors_event_t a, g, t;
  mpu.getEvent(&a, &g, &t);

  // Potenciómetro dinámico
  umbral = map(analogRead(PIN_POT), 0, 4095, 10, 80) / 10.0;

  // Magnitud (optimizado sin pow)
  float raw = sqrt(
    a.acceleration.x * a.acceleration.x +
    a.acceleration.y * a.acceleration.y +
    (a.acceleration.z - 9.81) * (a.acceleration.z - 9.81)
  );

  // Filtro EMA
  mag = (0.2 * raw) + (0.8 * mag);

  if (mag > umbral) {
    statusIA = (mag > 7.0) ? "RIESGO CRITICO" : "SISMO DETECTADO";

    int lvl = map(constrain(mag * 10, 10, 100), 10, 100, 50, 255);

    ledcWrite(ch_bz, lvl);
    ledcWrite(ch_led, lvl);
    digitalWrite(PIN_GN, LOW);

    SerialBT.printf("ALERTA: %.2f | %s\n", mag, statusIA.c_str());

  } else {
    statusIA = "ESTRUCTURA SEGURA";

    ledcWrite(ch_bz, 0);
    ledcWrite(ch_led, 0);
    digitalWrite(PIN_GN, HIGH);
  }

  delay(50);
}