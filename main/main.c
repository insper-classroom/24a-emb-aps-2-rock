#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>
#include "hardware/adc.h"
#include <math.h>

const int BTN_PIN_R = 19;
const int BTN_G = 16;
const int BTN_B = 21;
const int BTN_Y = 20;
const int BTN_O = 22;

const int UART_TX_PIN = 0;
const int UART_RX_PIN = 1;

QueueHandle_t xQueueBTN;
QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;

void x_task(void *p) {
    adc_init();
    adc_gpio_init(27);
    
    adc_t str;
    int A[6] = {0};
    while (1) {
        adc_select_input(1);

        A[0]=A[1];
        A[1]=A[2];
        A[2]=A[3];
        A[3]=A[4];
        A[4]=adc_read();

        str.axis=0;
        str.val=(((A[0]+A[1]+A[2]+A[3]+A[4])/5) - 2047)/10;
        if (abs(str.val)<=150) str.val=0;
        xQueueSend(xQueueAdc, &str, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) {
    adc_init();
    adc_gpio_init(26);
    
    adc_t str;
    int A[6] = {0};
    while (1) {
        adc_select_input(0);

        A[0]=A[1];
        A[1]=A[2];
        A[2]=A[3];
        A[3]=A[4];
        A[4]=adc_read();

        str.axis=1;
        str.val=(((A[0]+A[1]+A[2]+A[3]+A[4])/5) - 2047)/10;
        if (abs(str.val)<=150) str.val=0;
        xQueueSend(xQueueAdc, &str, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Estados iniciais dos botões
volatile uint8_t button_states[5] = {1, 1, 1, 1, 1}; // 1 indica botão não pressionado

typedef struct {
    int btnId;
} ButtonPress;

void update_button_states() {
    button_states[0] = gpio_get(BTN_PIN_R) == 0 ? 0 : 1;
    button_states[1] = gpio_get(BTN_G) == 0 ? 0 : 1;
    button_states[2] = gpio_get(BTN_B) == 0 ? 0 : 1;
    button_states[3] = gpio_get(BTN_Y) == 0 ? 0 : 1;
    button_states[4] = gpio_get(BTN_O) == 0 ? 0 : 1;
}

void send_button_states() {
    // Envia os estados dos botões como um único byte para o UART

    uint8_t data = 0;
    for (int i = 0; i < 5; i++) {
        if (button_states[i] == 0) { // Se botão pressionado
            data |= (1 << i);
        }
    }
    uart_putc_raw(uart0, 0xFF);
    uart_putc_raw(uart0, data);
    uart_putc_raw(uart0, -1);
}

void button_callback(uint gpio, uint32_t events) {
    static TickType_t lastDebounceTime = 0;
    const TickType_t debounceDelay = pdMS_TO_TICKS(50); // 50 ms debounce time
    
    TickType_t currentTime = xTaskGetTickCount();
    if (currentTime - lastDebounceTime > debounceDelay) {
        // Suficiente tempo desde a última interrupção passou, podemos considerar isso como um evento válido de botão
        ButtonPress btnPress;
        btnPress.btnId = gpio;
        xQueueSendFromISR(xQueueBTN, &btnPress, NULL);
        
        // Atualizar o último tempo de debouncing
        lastDebounceTime = currentTime;
    }
}

void button_task(void *p) {
    ButtonPress btnPress;
    TickType_t lastSendTime = xTaskGetTickCount();

    while (true) {
        if (xTaskGetTickCount() - lastSendTime >= pdMS_TO_TICKS(100)) {
            // Envie os estados dos botões a cada 100ms
            update_button_states(); // Atualiza os estados dos botões
            printf("Button States: [");
            for (int i = 0; i < 5; i++) {
                printf("%d ", button_states[i]);
            }
            printf("]\n");
            send_button_states(); // Envia os estados dos botões para UART
            lastSendTime = xTaskGetTickCount();
        }

        // Verifique se houve pressão de botão
        if (xQueueReceive(xQueueBTN, &btnPress, 0)) {
            update_button_states(); // Atualiza os estados dos botões
            printf("Button States: [");
            for (int i = 0; i < 5; i++) {
                printf("%d ", button_states[i]);
            }
            printf("]\n");
            send_button_states(); // Envia os estados dos botões para UART
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Pequena pausa para liberar o processador
    }
}
void write_package(adc_t data) {
    int val = data.val;
    int msb = val >> 8;
    int lsb = val & 0xFF ;

    uart_putc_raw(uart0, data.axis);
    uart_putc_raw(uart0, lsb);
    uart_putc_raw(uart0, msb);
    uart_putc_raw(uart0, -1);
}

void uart_task(void *p) {
    adc_t data;

    while (1) {
        if (xQueueReceive(xQueueAdc, &data, 1)) {
            printf("Data: %d %d\n", data);
            write_package(data); 
            vTaskDelay(pdMS_TO_TICKS(100));
        } else ;
        
    }
}


int main() {
    stdio_init_all();

    // Configuração da UART
    uart_init(uart0, 115200);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Configuração dos pinos dos botões
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    gpio_init(BTN_G);
    gpio_set_dir(BTN_G, GPIO_IN);
    gpio_pull_up(BTN_G);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    gpio_init(BTN_Y);
    gpio_set_dir(BTN_Y, GPIO_IN);
    gpio_pull_up(BTN_Y);

    gpio_init(BTN_O);
    gpio_set_dir(BTN_O, GPIO_IN);
    gpio_pull_up(BTN_O);

    xQueueBTN = xQueueCreate(10, sizeof(ButtonPress));
    xQueueAdc = xQueueCreate(32, sizeof(adc_t));


    // Configuração de interrupção para os botões
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BTN_G, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BTN_Y, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BTN_O, GPIO_IRQ_EDGE_FALL, true, &button_callback);

    xTaskCreate(uart_task, "UART_Task", 4096, NULL, 1, NULL);
    xTaskCreate(button_task, "Button Task", 256, NULL, 1, NULL);
    xTaskCreate(x_task, "X Task", 256, NULL, 1, NULL);
    xTaskCreate(y_task, "Y Task", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true) { }

    return 0;
}
