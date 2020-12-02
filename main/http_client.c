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
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA: %s\n\n",(char *)evt->data);
            if (esp_http_client_is_chunked_response(evt->client)) {
                char buffer[500];
                sprintf(buffer, "%.*s", evt->data_len, (char *)evt->data);
                strcat(ipRequest, buffer);
                ESP_LOGI(TAG, "trucado startando em: %c\n\nValor ate agr: %s\n\n",*(char *)evt->data, ipRequest);
                if(*(char *)evt->data != '{'){
                    cJSON *json = cJSON_Parse(ipRequest);
                    ESP_LOGI(TAG, "cJSON_HasObjectItem(country_code): %d", (int) cJSON_HasObjectItem(json, "country_code"));
                    if(cJSON_HasObjectItem(json,"country_code")){
                        ESP_LOGI(TAG, "LIDANDO COM LOCATION REQUEST");
                        lat = cJSON_GetObjectItemCaseSensitive(json, "latitude")->valuedouble;
                        longi = cJSON_GetObjectItemCaseSensitive(json, "longitude")->valuedouble;
                        setupReady = 1;
                        ipRequest[0] = '\0';
                    }
                    else{
                        ESP_LOGI(TAG, "LIDANDO COM WEATHER REQUEST");
                        atual = cJSON_GetObjectItemCaseSensitive(json, "temp")->valuedouble;
                        max = cJSON_GetObjectItemCaseSensitive(json, "temp_max")->valuedouble;
                        min = cJSON_GetObjectItemCaseSensitive(json, "temp_min")->valuedouble;
                        umidade = cJSON_GetObjectItemCaseSensitive(json, "humidity")->valuedouble;
                        printf("\n\n\n\nAtual: %.2lf\nMáxima: %.2lf\nMinima: %.2lf\nUmidade: %.2lf\n\n\n\n", atual, max, min, umidade);
                    }
                    cJSON_Delete(json);
                }
            }
            else if (!esp_http_client_is_chunked_response(evt->client) && *(char*)evt->data >= '0' && *(char*)evt->data <= '9' ){
                ESP_LOGI(TAG, "LIDANDO COM EXTERNAL IP REQUEST");
                char *temp = (char*)evt->data;
                //cleaning the response that sometimes cames dirty ??)
                while((*temp >= '0' && *temp <= '9') || *temp == '.'){
                    temp++;
                }
                *++temp = '\0';
                strcpy(externalIp, (char*)evt->data);
            }

            break;
        default:
            break;
        }
    return ESP_OK;
}

void get_external_ip_request(){
    externalIp[0] = 'E';
    ESP_LOGI(TAG, "REQUEST DO IP EXTERNO");
    esp_http_client_config_t config = {
        .url = "https://myexternalip.com/raw",
        .event_handler = _http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
    ESP_LOGI(TAG, "Status = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));
    }
    esp_http_client_cleanup(client);
}

void get_location_request(){
    ipRequest[0] = '\0';
    while (*externalIp == 'E')
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    char requestURL[100];
    sprintf (requestURL, "http://api.ipstack.com/%s?access_key=%s", externalIp, IP_STACK_APIKEY);
    printf("FAzendo request: %s\n", requestURL);
    esp_http_client_config_t config = {
        .url = requestURL,
        .event_handler = _http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
    ESP_LOGI(TAG, "Status = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));
    }
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
    printf("FAzendo request: %s\n", requestURL);
    esp_http_client_config_t config = {
        .url = requestURL,
        .event_handler = _http_event_handle,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
    ESP_LOGI(TAG, "Status = %d, content_length = %d",
            esp_http_client_get_status_code(client),
            esp_http_client_get_content_length(client));
    }
    esp_http_client_cleanup(client);
}