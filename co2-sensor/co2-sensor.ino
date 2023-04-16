#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h"

SCD30 airSensor;

RTC_DATA_ATTR int counter = 0;

const char *ssid = "";
const char *password = "";

String endpoint = "";

void connectWifi()
{
  WiFi.begin(ssid, password);

  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    Serial.print(".");
  }

  Serial.print("Connected!");
  Serial.println(WiFi.localIP());
}

void connectSensor()
{
  Wire.begin();

  while (airSensor.begin() == false)
  {
    Serial.println("Air sensor not detected. Please check wiring. Freezing...");
    delay(1000);
  }
}

void setup()
{
  Serial.begin(115200);

  connectWifi();
  connectSensor();
}

void loop()
{
  uint16_t co2 = 0;
  float temp = 0.0;
  float hum = 0.0;

  if (airSensor.dataAvailable())
  {
    co2 = airSensor.getCO2();
    temp = airSensor.getTemperature();
    hum = airSensor.getHumidity();

    Serial.print("co2(ppm):");
    Serial.print(co2);

    Serial.print(" temp(C):");
    Serial.print(temp);

    Serial.print(" humidity(%):");
    Serial.print(hum);

    Serial.println();
  }
  else
  {
    Serial.println("Waiting for new data");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;

    String body = "#TYPE rpi_home_co2 gauge\nrpi_home_co2 " + String(co2) + "\n";

    http.begin(endpoint.c_str());

    int responseCode = http.POST(body);

    if (responseCode > 0)
    {
      Serial.print("HTTP Response: ");
      Serial.println(responseCode);
    }
    else
    {
      Serial.print("Error: ");
      Serial.println(responseCode);
    }

    http.end();
  }

  counter += 1;

  delay(8000);
  // esp_sleep_enable_timer_wakeup(1);
  // Serial.println("Enter sleep");
  // esp_deep_sleep_start();
}
