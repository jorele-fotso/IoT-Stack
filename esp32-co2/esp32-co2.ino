/*
 * ESP32 CO2 sensor -> InfluxDB 2.x
 *
 * Reads CO2 concentration (ppm) from an MH-Z19B sensor over UART and
 * writes it to InfluxDB through the HTTP write API using line protocol.
 *
 * Wiring (MH-Z19B -> ESP32):
 *   Vin -> 5V
 *   GND -> GND
 *   Tx  -> GPIO16 (UART2 RX)
 *   Rx  -> GPIO17 (UART2 TX)
 *
 * Credentials are stored in secrets.h (see secrets.h.example).
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include "secrets.h"

// ---- Configuration ---------------------------------------------------------
static const int      SENSOR_RX_PIN      = 16;
static const int      SENSOR_TX_PIN      = 17;
static const uint32_t SENSOR_BAUD        = 9600;
static const uint32_t SEND_INTERVAL_MS   = 30000;  // time between measurements
static const uint32_t WIFI_TIMEOUT_MS    = 20000;
static const uint32_t SENSOR_TIMEOUT_MS  = 1000;

static const char *MEASUREMENT = "co2";
static const char *DEVICE_TAG  = "esp32-01";  // identifies this device in InfluxDB

// MH-Z19B "read CO2 concentration" command
static const uint8_t READ_CMD[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};

HardwareSerial sensorSerial(2);

// ---- Sensor ----------------------------------------------------------------

// Returns the CO2 concentration in ppm, or -1 on error.
int readCo2() {
  // Flush stale bytes before sending a new request
  while (sensorSerial.available()) sensorSerial.read();

  sensorSerial.write(READ_CMD, sizeof(READ_CMD));

  uint8_t response[9];
  uint32_t start = millis();
  size_t received = 0;
  while (received < sizeof(response) && millis() - start < SENSOR_TIMEOUT_MS) {
    if (sensorSerial.available()) {
      response[received++] = sensorSerial.read();
    }
  }
  if (received < sizeof(response)) return -1;

  // Validate header and checksum
  if (response[0] != 0xFF || response[1] != 0x86) return -1;
  uint8_t checksum = 0;
  for (int i = 1; i < 8; i++) checksum += response[i];
  checksum = 0xFF - checksum + 1;
  if (checksum != response[8]) return -1;

  return (response[2] << 8) | response[3];
}

// ---- Wi-Fi -----------------------------------------------------------------

bool connectWifi() {
  if (WiFi.status() == WL_CONNECTED) return true;

  Serial.printf("Connecting to Wi-Fi '%s'", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  Serial.println("Wi-Fi connection failed");
  return false;
}

// ---- InfluxDB --------------------------------------------------------------

// Sends one data point using the InfluxDB v2 write API (line protocol).
bool sendToInfluxDb(int co2ppm) {
  String url = String(INFLUXDB_URL) + "/api/v2/write?org=" + INFLUXDB_ORG +
               "&bucket=" + INFLUXDB_BUCKET + "&precision=s";

  // Line protocol: measurement,tag=value field=value
  // The timestamp is omitted, so the server assigns the current time.
  String body = String(MEASUREMENT) + ",device=" + DEVICE_TAG + " ppm=" + co2ppm + "i";

  HTTPClient http;
  http.begin(url);
  http.addHeader("Authorization", String("Token ") + INFLUXDB_TOKEN);
  http.addHeader("Content-Type", "text/plain; charset=utf-8");

  int status = http.POST(body);
  http.end();

  if (status == 204) return true;  // 204 No Content = success

  Serial.printf("InfluxDB write failed, HTTP status: %d\n", status);
  return false;
}

// ---- Arduino entry points --------------------------------------------------

void setup() {
  Serial.begin(115200);
  sensorSerial.begin(SENSOR_BAUD, SERIAL_8N1, SENSOR_RX_PIN, SENSOR_TX_PIN);
  connectWifi();
}

void loop() {
  if (connectWifi()) {
    int co2 = readCo2();
    if (co2 < 0) {
      Serial.println("Failed to read CO2 sensor");
    } else {
      Serial.printf("CO2: %d ppm\n", co2);
      if (sendToInfluxDb(co2)) Serial.println("Data sent to InfluxDB");
    }
  }
  delay(SEND_INTERVAL_MS);
}
