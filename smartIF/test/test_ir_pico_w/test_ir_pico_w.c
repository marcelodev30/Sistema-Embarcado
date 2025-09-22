// main.c - Gravador IR v5 (Corrigido)
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

// --- CONFIGURAÇÕES ---
const int IR_PIN = 16;
#define MAX_PULSES 500
#define TIMEOUT_US 250000 
#define MIN_PULSES_TO_SHOW 40 

typedef enum { STATE_IDLE, STATE_RECEIVING } ir_state_t;

// --- VARIÁVEIS GLOBAIS ---
volatile uint32_t pulse_times[MAX_PULSES];
volatile uint16_t pulse_count = 0;
volatile uint64_t last_event_us = 0;
volatile ir_state_t current_state = STATE_IDLE;

void gpio_callback(uint gpio, uint32_t events) {
    if (pulse_count >= MAX_PULSES) return;
    if (current_state == STATE_IDLE) {
        current_state = STATE_RECEIVING;
    }
    uint64_t current_us = time_us_64();
    if (last_event_us != 0) {
        pulse_times[pulse_count++] = (uint32_t)(current_us - last_event_us);
    }
    last_event_us = current_us;
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    
    printf("\n\n--- TESTE DE SANIDADE DO RECEPTOR IR ---\n");
    printf("USE UM CONTROLE REMOTO SIMPLES (TV, SOM, ETC)\n");
    printf("\n✅ Pronto! Pressione um botão no controle de TESTE.\n");
    printf("----------------------------------------------------------------------\n");

    gpio_set_irq_enabled_with_callback(IR_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    while (true) {
        if (current_state == STATE_RECEIVING) {
            if (time_us_64() - last_event_us > TIMEOUT_US) {
                // Desabilita IRQ para processar dados
                gpio_set_irq_enabled(IR_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

                if (pulse_count > MIN_PULSES_TO_SHOW) {
                    printf("📡 Sinal VÁLIDO capturado! Pulsos/Pausas medidos: %d\n\n", pulse_count);
                    printf("uint32_t dados_brutos[] = {");
                    for (int i = 0; i < pulse_count; i++) {
                        printf("%u", pulse_times[i]);
                        if (i < pulse_count - 1) {
                            printf(", ");
                        }
                    }
                    printf("};\n\n");
                } else {
                    printf("⚠️ Captura inválida ou muito curta (%d pulsos).\n\n", pulse_count);
                }
                
                printf("----------------------------------------------------------------------\n");
                printf("Pronto para a próxima captura...\n");

                // Reset do estado
                pulse_count = 0;
                last_event_us = 0;
                current_state = STATE_IDLE;
                
                // Reabilita IRQ
                gpio_set_irq_enabled(IR_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

                // Pequeno delay para evitar flood de prints
                sleep_ms(200); 
            }
        }
    }
}
