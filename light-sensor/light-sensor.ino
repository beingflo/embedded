#include <Wire.h>
#include <BH1750.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

uint8_t receiver[] = {0x24, 0x4C, 0xAB, 0x82, 0xF5, 0x0C};

esp_now_peer_info_t peerInfo;

BH1750 lightMeter;
int success = 0;

float measurement = 0;

struct msg_data
{
  float measurement;
  int num_tries;
};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  if (status == ESP_NOW_SEND_SUCCESS)
  {
    success = 1;
  }
}

void configure_esp_now()
{
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  if (esp_now_register_send_cb(OnDataSent) != ESP_OK)
  {
    Serial.println("Error registering callback");
    return;
  }

  memcpy(peerInfo.peer_addr, receiver, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Error adding peer");
    return;
  }
}

void configure_lightmeter()
{
  Wire.begin();

  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE);
}

void take_measurement()
{
  while (!lightMeter.measurementReady(true))
  {
    yield();
  }

  float lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lux");

  measurement = lux;

  lightMeter.configure(BH1750::ONE_TIME_HIGH_RES_MODE);
}

void transmit_data()
{
  esp_wifi_start();
  WiFi.mode(WIFI_STA);

  int num_tries = 0;

  // Resend until data was received by recipient
  while (!success && num_tries < 5)
  {
    esp_err_t result = ESP_FAIL;

    num_tries += 1;

    int send_tries = 0;
    // Try sending until successfully sent
    while (result != ESP_OK && send_tries < 20)
    {
      send_tries += 1;

      struct msg_data data = {measurement, num_tries};

      result = esp_now_send(receiver, (uint8_t *)&data, sizeof(msg_data));

      if (result != ESP_OK)
      {
        Serial.println("Error sending the data");
        break;
      }
    }

    if (send_tries == 20)
    {
      Serial.println("ESP can't send");
    }

    // Allow time for callback to signal success
    delay(100);
  }

  if (num_tries == 5)
  {
    Serial.println("Retries exhausted");
  }

  success = 0;

  esp_wifi_stop();
}

void setup()
{
  Serial.begin(115200);
  Serial.setTxTimeoutMs(0);

  configure_esp_now();

  configure_lightmeter();
}

void loop()
{
  take_measurement();

  transmit_data();

  esp_sleep_enable_timer_wakeup(1000000);
  esp_light_sleep_start();
}