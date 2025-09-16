#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Tag para usar nos logs (ajuda a identificar de onde vem a mensagem)
static const char *TAG = "MEU_APP";

// A função principal em um projeto ESP-IDF é a app_main()
void app_main(void)
{
    int contador = 0;

    // Loop infinito, como o loop() do Arduino, mas dentro de uma "tarefa" principal.
    while (1) {
        // Imprime uma mensagem de log do tipo "Informação"
        ESP_LOGI(TAG, "Olá, mundo do ESP-IDF! Contagem: %d", contador++);

        // Pausa a tarefa por 1000 milissegundos (1 segundo).
        // vTaskDelay é a forma correta de criar um "delay" no FreeRTOS.
        // A constante portTICK_PERIOD_MS ajuda a converter ms para "ticks" do sistema.
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}