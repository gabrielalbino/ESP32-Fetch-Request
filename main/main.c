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
volatile int blinkOnce = 0;

void PiscaLed(void* params){
  gpio_pad_select_gpio(LED);   
  gpio_set_direction(LED, GPIO_MODE_OUTPUT);
  int estado = 0;

  while(true){
    if(blinkLed){
      gpio_set_level(LED, estado);
      estado = !estado;
    }
    else if (blinkOnce) {
      gpio_set_level(LED, 0);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      gpio_set_level(LED, 1);
      blinkOnce = 0;
    }
    else if (estado == 0){
      estado = !estado;
      gpio_set_level(LED, estado);
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void RealizaHTTPRequest(void * params)
{
  ESP_LOGI("RealizaHTTPRequest", "Espaço livre na HEAP %dkb", xPortGetFreeHeapSize()); 
  while(true)
  {
    if (xSemaphoreTake(conexaoWifiSemaphore, portMAX_DELAY))
    {
      blinkLed = 0;
      printf("peguei semafaro task request");
      ESP_LOGI("Main Task", "Realiza HTTP Request");
      get_external_ip_request();
      get_location_request();
      get_weather_request();
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
    pthread_create(&threadWifi, NULL, &wifi_start, NULL);
    xTaskCreate(&PiscaLed, "Pisca LED", 512, NULL, 1, NULL);
    xTaskCreate(&RealizaHTTPRequest,  "Processa HTTP", 40960, NULL, 3, NULL);
    
}
