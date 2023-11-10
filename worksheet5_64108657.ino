#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Wi-Fi credentials
const char* SSID = "Feezoh";
const char* PASSWORD = "123456Ff";

// HTTP server settings
const char* SERVER_URL = "http://192.168.137.152:3000/test";

// DHT sensor configuration
const int SENSOR_PIN = D4;
DHT sensor(SENSOR_PIN, DHT11);

// NTP configuration
const long UTC_OFFSET = 25200; // Thailand offset (7 hours)
const int UPDATE_INTERVAL = 60000; // NTP update interval (ms)
WiFiUDP udpClient;
NTPClient timeClient(udpClient, "pool.ntp.org", UTC_OFFSET, UPDATE_INTERVAL);

// HTTP client instance
WiFiClient client;
HTTPClient http;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  sensor.begin();
  timeClient.begin();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected");
}

void loop() {
  static unsigned long lastUpdate = 0;
  const unsigned long DELAY_TIME = 10000; // Delay between updates (ms)

  if ((millis() - lastUpdate) > DELAY_TIME) {
    timeClient.update();

    float humidityLevel = sensor.readHumidity();
    float tempCelsius = sensor.readTemperature();

    if (isnan(humidityLevel) || isnan(tempCelsius)) {
      Serial.println("Failed to read from DHT sensor");
    } else {
      Serial.printf("Humidity: %.2f%%\n", humidityLevel);
      Serial.printf("Temperature: %.2f°C\n", tempCelsius);

      time_t currentEpoch = timeClient.getEpochTime();
      struct tm* currentTime = gmtime(&currentEpoch);
      char formattedTime[25];
      strftime(formattedTime, sizeof(formattedTime), "%Y-%m-%d %H:%M:%S", currentTime);

      StaticJsonDocument<300> jsonDoc;
      JsonObject data = jsonDoc.createNestedObject(formattedTime);
      data["humidity"] = humidityLevel;
      data["temperature"] = tempCelsius;

      String payload;
      serializeJson(jsonDoc, payload);
      http.begin(client, SERVER_URL);
      http.addHeader("Content-Type", "application/json");
      int httpCode = http.PATCH(payload);

      if (httpCode > 0) {
        String response = http.getString();
        Serial.printf("HTTP Response code: %d\n", httpCode);
        Serial.println("Response payload:");
        Serial.println(response);
      } else {
        Serial.printf("HTTP PATCH error: %d\n", httpCode);
      }

      http.end();
    }

    lastUpdate = millis();
  }
}
