#include <Arduino.h>

void light_sleep_setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
}

void light_sleep_loop()
{
  digitalWrite(LED_BUILTIN, HIGH);
  esp_sleep_enable_timer_wakeup(10000000);
  esp_light_sleep_start();
  digitalWrite(LED_BUILTIN, LOW);
  esp_sleep_enable_timer_wakeup(10000000);
  esp_light_sleep_start();
}
