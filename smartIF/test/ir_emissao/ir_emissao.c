// main.c - Versão Final e Corrigida para Teste de Transmissão
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ir_tx.pio.h" // Incluindo o header C GERADO

// --- CONFIGURAÇÕES ---
#define IR_TX_PIN 16

// <<< CORREÇÃO 1: Adicionada a definição da macro 'count_of'
#define count_of(a) (sizeof(a) / sizeof((a)[0]))

// Código capturado por você para o teste
uint32_t dados_brutos[] = {517, 650, 494, 624, 519, 625, 520, 624, 520, 625, 520, 623, 521, 623, 1633, 579, 1678, 625, 1634, 580, 1678, 625, 522, 623, 1633, 652, 1604, 580, 1679, 625, 520, 651, 1605, 626, 520, 653, 491, 652, 492, 624, 521, 624, 520, 623, 520, 624, 1631, 580, 566, 623, 1636, 649, 1607, 580, 1679, 653, 1606, 623, 1635, 652, 1605, 653, 41253, 9147, 2263, 652, 96724, 9195, 2216, 579};

void send_ir_raw(PIO pio, uint sm, const uint32_t* timings, uint len) {
    // <<< CORREÇÃO 2: Usando a constante definida no arquivo .pio
    float carrier_period_us = 1000000.0 / ir_tx_CARRIER_FREQ; 
    
    for (int i = 0; i < len; ++i) {
        if (i % 2 == 0) { // MARK
            uint32_t mark_us = timings[i];
            uint32_t num_carrier_cycles = (uint32_t)(mark_us / carrier_period_us);
            pio_sm_put_blocking(pio, sm, num_carrier_cycles);
        } else { // SPACE
            sleep_us(timings[i]);
        }
    }
}

int main() {
    stdio_init_all();
    sleep_ms(3000); 
    
    PIO pio = pio0;
    uint sm = pio_claim_unused_sm(pio, true);
    uint offset = pio_add_program(pio, &ir_tx_program);
    ir_tx_program_init(pio, sm, offset, IR_TX_PIN);
    
    printf("\n\n--- Transmissor de Teste IR (Compilado Corretamente) ---\n");
    printf("Enviando código de %d pulsos/pausas a cada 5 segundos.\n", count_of(dados_brutos));
    printf("Aponte o módulo emissor para o ar condicionado!\n");
    printf("----------------------------------------------------------\n");

    while (true) {
        printf("Enviando sinal IR...\n");
        send_ir_raw(pio, sm, dados_brutos, count_of(dados_brutos));
        printf("Sinal enviado.\n");
        sleep_ms(5000);
    }
}