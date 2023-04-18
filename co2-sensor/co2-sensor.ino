#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "SparkFun_SCD30_Arduino_Library.h"
#include "TM1637.h"

SCD30 airSensor;

RTC_DATA_ATTR int counter = 0;

const char *ssid = "";
const char *password = "";
String endpoint = "";

const int CLK = D2;
const int DIO = D3;

TM1637 tm(CLK, DIO);

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
    Serial.println("Sensor not detected!");
    delay(1000);
  }
  airSensor.setTemperatureOffset(1.8);

  delay(1000);
}

void setup()
{
  Serial.begin(115200);
  tm.begin();
  tm.setBrightness(2);

  connectSensor();
  connectWifi();
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

    tm.display(co2, true, true);

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

  if (WiFi.status() == WL_CONNECTED && co2 != 0)
  {
    HTTPClient http;

    String co2_prom = "#TYPE rpi_home_co2 gauge\nrpi_home_co2 " + String(co2) + "\n";
    String hum_prom = "#TYPE rpi_home_humidity gauge\nrpi_home_humidity " + String(hum) + "\n";
    String temp_prom = "#TYPE rpi_home_temperature gauge\nrpi_home_temperature " + String(temp) + "\n";

    String body = co2_prom + hum_prom + temp_prom;

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
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Wifi disconnected");
    connectWifi();
  }

  counter += 1;

  delay(15000);
}
