#include <esp_now.h>
#include <WiFi.h>

uint8_t receiver[] = {0x24, 0x4C, 0xAB, 0x82, 0xF5, 0x0C};

esp_now_peer_info_t peerInfo;

RTC_DATA_ATTR int counter = 0;
int success = 0;

struct msg_data
{
  int counter;
  int num_tries;
};

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");

  if (status == ESP_NOW_SEND_SUCCESS)
  {
    success = 1;
  }
}

void setup()
{
  Serial.begin(115200);
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

  int num_tries = 0;

  // Resend until data was received by recipient
  while (!success)
  {
    esp_err_t result = ESP_FAIL;

    // Try sending until successfully sent
    while (result != ESP_OK)
    {
      num_tries += 1;

      struct msg_data data = {counter, num_tries};

      result = esp_now_send(receiver, (uint8_t *)&data, sizeof(msg_data));

      if (result != ESP_OK)
      {
        Serial.println("Error sending the data");
      }
    }

    // Allow time for callback to signal success
    delay(100);
  }

  counter += 1;

  esp_sleep_enable_timer_wakeup(1);

  esp_deep_sleep_start();
}

void loop()
{
}