#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <esp_now.h>
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

struct msg_data
{
  float lux;
  float temperature;
  float humidity;
  float vBat;
  int num_tries;
};

struct msg_data data = {-5, -5, -5, -5, 0};

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  memcpy(&data, incomingData, sizeof(msg_data));

  Serial.printf("lux: %f, temperature: %f, humidity: %f, vBat: %f, retries: %d\n", data.lux, data.temperature, data.humidity, data.vBat, data.num_tries);
}

void connectWifi()
{
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(200);
    Serial.print(".");
  }

  Serial.print("Connected!");
  Serial.println(WiFi.localIP());

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb(OnDataRecv);
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
  tm.setBrightness(1);

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

    int offset = 1;
    if (co2 >= 1000)
    {
      offset = 0;
    }
    tm.display(co2, true, false, offset);

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

    if (data.lux > -4)
    {
      String lux_esp = "#TYPE home_lux gauge\nhome_lux " + String(data.lux) + "\n";
      String temp_esp = "#TYPE home_temp gauge\nhome_temp " + String(data.temperature) + "\n";
      String hum_esp = "#TYPE home_hum gauge\nhome_hum " + String(data.humidity) + "\n";
      String vBat_esp = "#TYPE home_vBat gauge\nhome_vBat " + String(data.vBat) + "\n";

      data = {-5, -5, -5, -5, 1};

      body = body + lux_esp + temp_esp + hum_esp + vBat_esp;
    }

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
