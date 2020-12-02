#include "http_client.h"

#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include <string.h>
#include "cJSON.h"

#define TAG "HTTP"
#define IP_STACK_APIKEY      CONFIG_IP_STACK_APIKEY
#define WEATHER_APIKEY      CONFIG_WEATHER_APIKEY

char externalIp[17];
char ipRequest[1000];
double lat, longi;
double atual, max, min, umidade;
int setupReady = 0;
esp_err_t _http_event_handle(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (esp_http_client_is_chunked_response(evt->client))
            {
                char buffer[500];
                sprintf(buffer, "%.*s", evt->data_len, (char *)evt->data);
                strcat(ipRequest, buffer);
                if(*(char *)evt->data != '{'){
                    cJSON *json = cJSON_Parse(ipRequest);
                    if(cJSON_HasObjectItem(json,"country_code")){
                        lat = cJSON_GetObjectItemCaseSensitive(json, "latitude")->valuedouble;
                        longi = cJSON_GetObjectItemCaseSensitive(json, "longitude")->valuedouble;
                        setupReady = 1;
                        ipRequest[0] = '\0';
                    }
                    cJSON_Delete(json);
                }
            }
            else if (!esp_http_client_is_chunked_response(evt->client) && *(char*)evt->data >= '0' && *(char*)evt->data <= '9' ){
                memset(externalIp, 0, 17);
                int i1 = 0, i2= 0, i3= 0, i4= 0;
                sscanf((char *)evt->data,
                                      "%d.%d.%d.%d",
                                      &i1,
                                      &i2,
                                      &i3,
                                      &i4);
                sprintf(externalIp, "%d.%d.%d.%d", i1, i2, i3, i4);
            }
            else {
                char buffer[500];
                sprintf(buffer, "%.*s", evt->data_len, (char *)evt->data);
                strcat(ipRequest, buffer);
                if(*(char *)evt->data != '{'){
                    cJSON *json = cJSON_Parse(ipRequest);
                    if(cJSON_HasObjectItem(json,"weather")){
                        cJSON *mainJSON = cJSON_GetObjectItem(json, "main");
                        atual = (cJSON_GetObjectItemCaseSensitive(mainJSON, "temp")->valuedouble - 273.15);
                        max = (cJSON_GetObjectItemCaseSensitive(mainJSON, "temp_max")->valuedouble - 273.15);
                        min = (cJSON_GetObjectItemCaseSensitive(mainJSON, "temp_min")->valuedouble - 273.15);
                        umidade = cJSON_GetObjectItemCaseSensitive(mainJSON, "humidity")->valuedouble;
                        printf("\n\n\n\nAtual: %.2lf\nMáxima: %.2lf\nMinima: %.2lf\nUmidade: %.2lf\n\n\n\n", atual, max, min, umidade);
                    }
                    cJSON_Delete(json);
                }
            }
            break;
        default:
            break;
        }
    return ESP_OK;
}

void get_external_ip_request(){
    externalIp[0] = 'E';
    esp_http_client_config_t config = {
        .url = "https://myexternalip.com/raw",
        .event_handler = _http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);

    esp_http_client_cleanup(client);
}

void get_location_request(){
    ipRequest[0] = '\0';
    while (*externalIp == 'E')
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    char requestURL[100];
    sprintf (requestURL, "http://api.ipstack.com/%s\?access_key=%s", externalIp, IP_STACK_APIKEY);
    printf("Fazendo request: %s\n", requestURL);
    esp_http_client_config_t config = {
        .url = requestURL,
        .event_handler = _http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);

    esp_http_client_cleanup(client);
}

void get_weather_request(){
    ipRequest[0] = '\0';
    while (!setupReady)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    char requestURL[200];
    sprintf (requestURL, "http://api.openweathermap.org/data/2.5/weather?lat=%lf&lon=%lf&appid=%s", lat, longi, WEATHER_APIKEY);
    printf("Fazendo request: %s\n", requestURL);
    esp_http_client_config_t config = {
        .url = requestURL,
        .event_handler = _http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_perform(client);

    esp_http_client_cleanup(client);
}