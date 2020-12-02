#include <stdio.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "wifi.h"
#include "http_client.h"
#include <pthread.h>

#define LED 2
char* strIP;
xSemaphoreHandle conexaoWifiSemaphore;
int blinkLed = 1;
int blinkOnce = 0;
int alreadyConfigured = 0;

void turnLedOn(){
  gpio_set_level(LED, 1);
}
void turnLedOff(){
  gpio_set_level(LED, 0);
}

void PiscaLed(void* params){
  gpio_pad_select_gpio(LED);   
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  int estado = 0;
  while (true)
  {
    if (blinkLed)
    {
      estado = !estado;
      estado ? turnLedOn() : turnLedOff();
    }
    else if (blinkOnce) {
      turnLedOff();
      blinkOnce = 0;
    }
    else{
      estado = 1;
      turnLedOn();
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void RealizaHTTPRequest(void * params)
{
  while(true)
  {
    if (alreadyConfigured || xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
    {
      blinkLed = 0;
      if(!alreadyConfigured){
        //dando um tempinho antes de fazer a primeira requisição pra ver o led piscando
        vTaskDelay(3000 / portTICK_PERIOD_MS);
      }
      blinkOnce = 1;
      if(!alreadyConfigured){
        get_external_ip_request();
        get_location_request();
      }
      get_weather_request();
      alreadyConfigured = 1;
      vTaskDelay(5*60*1000 / portTICK_PERIOD_MS);
    }
  }
}

void app_main(void)
{
  strIP = malloc(17 * sizeof(char));
  // Inicializa o NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    conexaoWifiSemaphore = xSemaphoreCreateBinary();
    pthread_t threadWifi;
    pthread_create(&threadWifi, NULL, (void*)&wifi_start, NULL);
    xTaskCreate(&PiscaLed, "Pisca LED", 512, NULL, 1, NULL);
    xTaskCreate(&RealizaHTTPRequest,  "Processa HTTP", 40960, NULL, 3, NULL);
    
}
