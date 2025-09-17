#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h" // Para o Mutex
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "managed_components/lioff__ds3231/ds3231.h" // Biblioteca do DS3231
#include "driver/gpio.h"
#include "cJSON.h" // Biblioteca para parsear JSON

// --- Configurações ---
#define WIFI_SSID         "SEU_WIFI_SSID"
#define WIFI_PASSWORD     "SUA_SENHA_WIFI"
#define MQTT_BROKER       "mqtt://mqtt.eclipseprojects.io"
#define MQTT_COMMAND_TOPIC "esp32/relay/command"
#define RELAY_GPIO        4

// Estrutura para nosso "quadro de avisos"
typedef struct {
    struct tm target_time; // Horário agendado
    int relay_state;       // Ação (0 ou 1)
    bool active;           // Flag para saber se há um agendamento ativo
} Schedule;

// O "quadro de avisos" global e volátil
volatile Schedule g_schedule;
// O "crachá de acesso" (Mutex) para o quadro de avisos
SemaphoreHandle_t g_schedule_mutex;

static const char *TAG = "ALARM_MQTT";
static i2c_dev_t rtc_dev;

// --- Funções de Wi-Fi e MQTT (omitidas por brevidade, são as mesmas dos exemplos anteriores) ---
void wifi_init(void); 
void mqtt_app_start(void); 

// Função que processa os eventos MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    if (event->event_id == MQTT_EVENT_CONNECTED) {
        ESP_LOGI(TAG, "Conectado ao MQTT Broker!");
        esp_mqtt_client_subscribe(event->client, MQTT_COMMAND_TOPIC, 0); // Se inscreve no tópico de comando
    } else if (event->event_id == MQTT_EVENT_DATA) {
        ESP_LOGI(TAG, "Mensagem recebida no tópico %.*s", event->topic_len, event->topic);
        
        // Parseia o JSON recebido
        cJSON *root = cJSON_Parse(event->data);
        if (root) {
            cJSON *time_item = cJSON_GetObjectItem(root, "time");
            cJSON *state_item = cJSON_GetObjectItem(root, "state");

            if (cJSON_IsString(time_item) && cJSON_IsNumber(state_item)) {
                // Pega o "crachá" para acessar o quadro de avisos
                if (xSemaphoreTake(g_schedule_mutex, portMAX_DELAY) == pdTRUE) {
                    // Atualiza o quadro de avisos com o novo agendamento
                    strptime(time_item->valuestring, "%H:%M:%S", &g_schedule.target_time);
                    g_schedule.relay_state = state_item->valueint;
                    g_schedule.active = true;
                    //ESP_LOGI(TAG, "ALARME AGENDADO para %02d:%02d:%02d, Ação: %d", 
                        g_schedule.target_time.tm_hour, g_schedule.target_time.tm_min, g_schedule.target_time.tm_sec, g_schedule.relay_state);
                    
                    // Devolve o "crachá"
                    xSemaphoreGive(g_schedule_mutex);
                }
            }
            cJSON_Delete(root);
        }
    }
}

// A tarefa do "Vigia do Relógio"
void alarm_checker_task(void *pvParameters) {
    struct tm current_time;
    Schedule local_schedule; // Uma cópia local para não segurar o mutex por muito tempo

    while (1) {
        // Dorme por 1 segundo. ESSENCIAL para não sobrecarregar a CPU.
        vTaskDelay(pdMS_TO_TICKS(1000));
        
        // Pega o crachá para ler o agendamento de forma segura
        if (xSemaphoreTake(g_schedule_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            local_schedule = g_schedule; // Copia o agendamento para a variável local
            xSemaphoreGive(g_schedule_mutex); // Devolve o crachá
        } else {
            continue; // Se não conseguiu o crachá, tenta de novo no próximo segundo
        }

        // Se não houver alarme ativo, não faz nada
        if (!local_schedule.active) {
            continue;
        }

        // Lê a hora atual do RTC
        if (ds3231_get_time(&rtc_dev, &current_time) == ESP_OK) {
            // Compara a hora atual com a hora agendada
            if (current_time.tm_hour == local_schedule.target_time.tm_hour &&
                current_time.tm_min  == local_schedule.target_time.tm_min &&
                current_time.tm_sec  == local_schedule.target_time.tm_sec)
            {
                ESP_LOGW(TAG, "ALARME DISPARADO! Acionando relé para o estado %d", local_schedule.relay_state);
                gpio_set_level(RELAY_GPIO, local_schedule.relay_state);
                
                // Desarma o alarme para não disparar de novo no mesmo segundo
                if (xSemaphoreTake(g_schedule_mutex, portMAX_DELAY) == pdTRUE) {
                    g_schedule.active = false;
                    xSemaphoreGive(g_schedule_mutex);
                }
            }
        }
    }
}

void app_main(void) {
    // ... inicialização do Wi-Fi ...
    
    // Inicializa o GPIO do relé
    gpio_reset_pin(RELAY_GPIO);
    gpio_set_direction(RELAY_GPIO, GPIO_MODE_OUTPUT);

    // ... inicialização do RTC I2C e DS3231 ...

    // Cria o Mutex para proteger nosso "quadro de avisos"
    g_schedule_mutex = xSemaphoreCreateMutex();
    g_schedule.active = false; // Começa sem nenhum alarme ativo

    // ... inicialização do MQTT (passando nosso mqtt_event_handler) ...

    // Cria e inicia a tarefa "Vigia do Relógio"
    xTaskCreate(alarm_checker_task, "alarm_checker", 4096, NULL, 5, NULL);
}