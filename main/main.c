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
#include "hc05.h"
#include "hardware/pwm.h"


const int BTN_PIN_R = 22;
const int BTN_G = 20;
const int BTN_B = 16;
const int BTN_Y = 19;
const int BTN_O = 15;
const int BTN_SELECT = 14;

const int UART_TX_PIN = 0;
const int UART_RX_PIN = 1;

#define PWM_0_GP 10
#define PWM_1_GP 11
#define PWM_2_GP 12




QueueHandle_t xQueueBTN;
QueueHandle_t xQueueAdc;
QueueHandle_t xQueueRGB;
QueueHandle_t xQueueS;

volatile int select_state = 1; // 1 indica botão não pressionado
volatile uint8_t button_states[5] = {1, 1, 1, 1, 1}; // 1 indica botão não pressionado

typedef struct rgb {
    int r;
    int g;
    int b;
} rgb_t;

typedef struct adc {
    int val_x;
    int val_y;
} adc_t;

typedef struct {
    int btnId;
} ButtonPress;


volatile adc_t data_joy;


double absoluto(double x) {
    if (x < 0) {
        return -x;
    }
    return x;
}

void init_pwm(int pwm_pin_gp, uint resolution, uint *slice_num, uint *chan_num) {
    gpio_set_function(pwm_pin_gp, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pwm_pin_gp);
    uint chan = pwm_gpio_to_channel(pwm_pin_gp);
    pwm_set_clkdiv(slice, 125); // pwm clock should now be running at 1MHz
    pwm_set_wrap(slice, resolution);
    pwm_set_chan_level(slice, PWM_CHAN_A, 0);
    pwm_set_enabled(slice, true);

    *slice_num = slice;
    *chan_num = chan;
}

void rgb_task(void *p) {
    int pwm_r_slice, pwm_r_chan;
    int pwm_g_slice, pwm_g_chan;
    int pwm_b_slice, pwm_b_chan;
    init_pwm(PWM_0_GP, 256, &pwm_r_slice, &pwm_r_chan); // RGB RED NO PIN 10
    init_pwm(PWM_1_GP, 256, &pwm_g_slice, &pwm_g_chan); // RGB GREEN NO PIN 11
    init_pwm(PWM_2_GP, 256, &pwm_b_slice, &pwm_b_chan); // RGB BLUE NO PIN 12

    rgb_t rgb;
    while (true) {

        if (xQueueReceive(xQueueRGB, &rgb, pdMS_TO_TICKS(100))) {
            //printf("r: %d, g: %d, b: %d\n", rgb.r, rgb.g, rgb.b);
            pwm_set_chan_level(pwm_r_slice, pwm_r_chan, rgb.r);
            pwm_set_chan_level(pwm_g_slice, pwm_g_chan, rgb.g);
            pwm_set_chan_level(pwm_b_slice, pwm_b_chan, rgb.b);
        }
    }
}

void select_task(void *p) {
    ButtonPress btnPress;
    while (true) {
        if (xQueueReceive(xQueueS, &btnPress, pdMS_TO_TICKS(100))) {
            //printf("Select Pressed\n");
            //printf("btn ");
            select_state = 0;
        }
        else {
            select_state = 1;
        }
    }
}

void hc05_task(void *p) {
    uart_init(hc05_UART_ID, hc05_BAUD_RATE);
    gpio_set_function(hc05_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(hc05_RX_PIN, GPIO_FUNC_UART);
    hc05_init("clona_cartao", "4242");

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    int status = 0; // COLOCAR O STATE DO BLUETOOTH
    uint8_t r, g, b;
    wheel(status, &r, &g, &b);
    rgb_t rgb = {r, g, b};
    xQueueSend(xQueueRGB, &rgb, 0);
}

void wheel(int WheelPos, uint8_t *r, uint8_t *g, uint8_t *b) {
    WheelPos = 255 - WheelPos;

    if (WheelPos == 0) {
        *r = 255;
        *g = 0;
        *b = 0;
    } else if (WheelPos ==1) {
        *r = 0;
        *g = 0;
        *b = 255;
    } else {
        *r = 0;
        *g = 255;
        *b = 0;
    }
}

void write_package() {
    
    uint8_t data_bt = 0;
    for (int i = 0; i < 5; i++) {
        if (button_states[i] == 0) { // Se botão pressionado
            data_bt |= (1 << i);
        }
    }

    uart_putc_raw(uart1, 255);
    uart_putc_raw(uart1, data_joy.val_x);
    uart_putc_raw(uart1, data_joy.val_y);
    uart_putc_raw(uart1, data_bt);
    uart_putc_raw(uart1, select_state);
    //printf("Data: %d %d %d\n", data.axis, valor_pot, status_botoes);
}


void x_task(void *p) {
    adc_init();
    adc_gpio_init(27);
    
    int A[6] = {0};
    while (1) {
        adc_select_input(1);
        data_joy.val_x = (100*adc_read())/4095;
        //xQueueSend(xQueueAdc, &str, 1);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void y_task(void *p) {
    adc_init();
    adc_gpio_init(28);
    
    adc_t str;
    int A[6] = {0};
    while (1) {
        adc_select_input(2);
        data_joy.val_y = (100*adc_read())/4095;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Estados iniciais dos botões


void update_button_states() {
    button_states[0] = gpio_get(BTN_PIN_R) == 0 ? 0 : 1;
    button_states[1] = gpio_get(BTN_G) == 0 ? 0 : 1;
    button_states[2] = gpio_get(BTN_B) == 0 ? 0 : 1;
    button_states[3] = gpio_get(BTN_Y) == 0 ? 0 : 1;
    button_states[4] = gpio_get(BTN_O) == 0 ? 0 : 1;
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

void select_callback(uint gpio, uint32_t events) {
    static TickType_t lastDebounceTime = 0;
    const TickType_t debounceDelay = pdMS_TO_TICKS(50); // 50 ms debounce time
    
    TickType_t currentTime = xTaskGetTickCount();
    if (currentTime - lastDebounceTime > debounceDelay) {
        // Suficiente tempo desde a última interrupção passou, podemos considerar isso como um evento válido de botão
        ButtonPress btnPress;
        btnPress.btnId = gpio;
        xQueueSendFromISR(xQueueS, &btnPress, NULL);
        
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
            //printf("[");
            for (int i = 0; i < 5; i++) {
                //printf("%d ", button_states[i]);
            }
            //printf("]\n");
            lastSendTime = xTaskGetTickCount();
        }

        // Verifique se houve pressão de botão
        if (xQueueReceive(xQueueBTN, &btnPress, 0)) {
            update_button_states(); // Atualiza os estados dos botões
            //printf("Button States: [");
            for (int i = 0; i < 5; i++) {
                //printf("%d ", button_states[i]);
            }
            //printf("]\n");
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // Pequena pausa para liberar o processador
    }
}





void uart_task(void *p) {
    adc_t data_joy;

    

    while (1){
        write_package();
        vTaskDelay(pdMS_TO_TICKS(100)); // Pequena pausa para liberar o processador
    }
    
    
        
}



int main() {
    stdio_init_all();

    // Configuração da UART
    // uart_init(uart0, 115200);
    // gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    // gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

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

    gpio_init(BTN_SELECT);
    gpio_set_dir(BTN_SELECT, GPIO_IN);
    gpio_pull_up(BTN_SELECT);

    xQueueBTN = xQueueCreate(10, sizeof(ButtonPress));
    xQueueAdc = xQueueCreate(2, sizeof(adc_t));
    xQueueRGB = xQueueCreate(32, sizeof(rgb_t));
    xQueueS = xQueueCreate(2, sizeof(ButtonPress));



    // Configuração de interrupção para os botões
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BTN_G, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BTN_Y, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BTN_O, GPIO_IRQ_EDGE_FALL, true, &button_callback);
    gpio_set_irq_enabled_with_callback(BTN_SELECT, GPIO_IRQ_EDGE_FALL, true, &select_callback);

    xTaskCreate(hc05_task, "hc05_task", 4096, NULL, 1, NULL);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 1, NULL);
    xTaskCreate(button_task, "Button_Task", 256, NULL, 1, NULL);
    xTaskCreate(x_task, "X_task", 256, NULL, 1, NULL);
    xTaskCreate(y_task, "Y_task", 256, NULL, 1, NULL);
    xTaskCreate(rgb_task, "RGB_task", 256, NULL, 1, NULL);
    xTaskCreate(select_task, "Select_Task", 256, NULL, 1, NULL);


    vTaskStartScheduler();

    while (true) { }

    return 0;
}
