#include <string.h>
#include <stdio.h>
#include "hc06.h"
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hc06.h"
#include "hardware/gpio.h"

#define LED_PIN 10
#define POWER_BUTTON_PIN 11
#define BTN_COLOR_1_PIN 12
#define BTN_COLOR_2_PIN 13
#define BTN_COLOR_3_PIN 14
#define BTN_COLOR_4_PIN 15
#define BTN_COLOR_5_PIN 16
#define JOYSTICK_X_ADC_CHANNEL 0
#define JOYSTICK_Y_ADC_CHANNEL 1

QueueHandle_t xQueueBTN;
QueueHandle_t xQueueADC;

volatile bool ledState = false;

typedef struct {
    int btnId;
    bool isPowerButton;
} ButtonPress;

typedef struct {
    int axis; // 0 para X, 1 para Y
    int val;  // Valor filtrado da leitura
} adc_t;

void button_callback(uint gpio, uint32_t events);
void adc_task(void *p);
void uart_task(void *p);
void button_task(void *p);
void led_task(void *p);
void setup_buttons(void);
void hc06_task(void *p);

void button_callback(uint gpio, uint32_t events) {
    static TickType_t lastDebounceTime = 0;
    const TickType_t debounceDelay = pdMS_TO_TICKS(50); // 50 ms debounce time
    
    TickType_t currentTime = xTaskGetTickCount();
    if (currentTime - lastDebounceTime > debounceDelay) {
        // Suficiente tempo desde a última interrupção passou, podemos considerar isso como um evento válido de botão
        ButtonPress btnPress;
        btnPress.btnId = gpio;
        btnPress.isPowerButton = (gpio == POWER_BUTTON_PIN);
        xQueueSendFromISR(xQueueBTN, &btnPress, NULL);
        
        // Atualizar o último tempo de debouncing
        lastDebounceTime = currentTime;
    }
}

void button_task(void *p) {
    ButtonPress btnPress;
    while (true) {
        if (xQueueReceive(xQueueBTN, &btnPress, portMAX_DELAY)) {
            if (btnPress.isPowerButton) {
                ledState = !ledState; // Alterna o estado do LED
                // Aqui você poderia chamar uma função ou enviar um sinal para outra tarefa, se necessário
            } else {
                // Lógica para outros botões, se necessário
            }
        }
    }
}


void adc_task(void *p) {
    uint axis = (uint)p; // 0 para X, 1 para Y
    adc_t adcValue;
    adcValue.axis = axis;
    
    // Configura o GPIO para o canal do ADC correto
    adc_gpio_init(axis == 0 ? JOYSTICK_X_ADC_CHANNEL : JOYSTICK_Y_ADC_CHANNEL);

    // Leitura e filtragem do valor analógico
    while (true) {
        uint16_t sum = 0;
        for (int i = 0; i < 10; i++) { // Média móvel simples de 10 amostras
            adc_select_input(axis);
            sum += adc_read();
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        adcValue.val = sum / 10;
        xQueueSend(xQueueADC, &adcValue, portMAX_DELAY);
    }
}

void uart_task(void *p) {
    adc_t adcValue;
    char message[50];
    
    while (true) {
        if (xQueueReceive(xQueueADC, &adcValue, portMAX_DELAY)) {
            // Envia o dado do ADC pela UART
            snprintf(message, sizeof(message), "Axis %d: %d\n", adcValue.axis, adcValue.val);
            uart_puts(HC06_UART_ID, message);
        }
    }
}



void led_task(void *p) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    while (true) {
        gpio_put(LED_PIN, ledState ? 1 : 0); // Acende ou apaga o LED baseado no estado
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setup_buttons() {
    const int colorButtons[] = {BTN_COLOR_1_PIN, BTN_COLOR_2_PIN, BTN_COLOR_3_PIN, BTN_COLOR_4_PIN, BTN_COLOR_5_PIN};
    for (int i = 0; i < sizeof(colorButtons) / sizeof(colorButtons[0]); i++) {
        gpio_init(colorButtons[i]);
        gpio_set_dir(colorButtons[i], GPIO_IN);
        gpio_pull_up(colorButtons[i]);
        gpio_set_irq_enabled_with_callback(colorButtons[i], GPIO_IRQ_EDGE_FALL, true, &button_callback);
    }

    gpio_init(POWER_BUTTON_PIN);
    gpio_set_dir(POWER_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(POWER_BUTTON_PIN);
    gpio_set_irq_enabled_with_callback(POWER_BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &button_callback);
}

void hc06_task(void *p) {
    uart_init(HC06_UART_ID, HC06_BAUD_RATE);
    gpio_set_function(HC06_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(HC06_RX_PIN, GPIO_FUNC_UART);
    hc06_init("aps2_legal", "1234");

    while (true) {
        uart_puts(HC06_UART_ID, "OLAAA ");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Função principal
int main() {
    stdio_init_all();
    printf("Start RTOS\n");

    xQueueBTN = xQueueCreate(10, sizeof(ButtonPress));
    xQueueADC = xQueueCreate(10, sizeof(adc_t));

    adc_init();
    setup_buttons();

    xTaskCreate(adc_task, "ADC_Task X", 4096, (void*)0, 1, NULL); // Eixo X
    xTaskCreate(adc_task, "ADC_Task Y", 4096, (void*)1, 1, NULL); // Eixo Y
    xTaskCreate(uart_task, "UART_Task", 4096, NULL, 1, NULL);
    xTaskCreate(button_task, "Button Task", 256, NULL, 1, NULL);
    xTaskCreate(led_task, "LED Task", 256, NULL, 1, NULL);
    xTaskCreate(hc06_task, "HC06 Task", 4096, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true) { }
    return 0;
}
