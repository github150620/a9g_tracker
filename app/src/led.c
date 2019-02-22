#include "led.h"
#include "api_hal_gpio.h"
#include "api_os.h"

#define LED_BLINK_TASK_SIZE      (1024 * 1)
#define LED_BLINK_TASK_PRIORITY  64

GPIO_config_t config[2];

float LED_blinkFreq;
float LED_blinkDuty;

void LED_BlinkTask(VOID *pData) {
    while (1) {
        if (LED_blinkFreq==LED_BLINK_FREQ_0) {
            OS_Sleep(1000);
        } else if (LED_blinkDuty==LED_BLINK_DUTY_FULL) {
            GPIO_SetLevel(config[LED_LED1], GPIO_LEVEL_HIGH);
            OS_Sleep(1000);
        } else if (LED_blinkDuty==LED_BLINK_DUTY_EMPTY) {
            GPIO_SetLevel(config[LED_LED1], GPIO_LEVEL_LOW);
            OS_Sleep(1000);
        } else {
            GPIO_SetLevel(config[LED_LED1], GPIO_LEVEL_HIGH);
            OS_Sleep(LED_blinkDuty*1000.0/LED_blinkFreq);
            GPIO_SetLevel(config[LED_LED1], GPIO_LEVEL_LOW);
            OS_Sleep((1-LED_blinkDuty)*1000.0/LED_blinkFreq);
        }
    }
}

void LED_Init() {
	config[LED_LED1].mode         = GPIO_MODE_OUTPUT;
	config[LED_LED1].pin          = GPIO_PIN27;
	config[LED_LED1].defaultLevel = GPIO_LEVEL_LOW;
    GPIO_Init(config[LED_LED1]);
	
	config[LED_LED2].mode         = GPIO_MODE_OUTPUT;
	config[LED_LED2].pin          = GPIO_PIN28;
	config[LED_LED2].defaultLevel = GPIO_LEVEL_LOW;
    GPIO_Init(config[LED_LED2]);

    LED_blinkFreq = LED_BLINK_FREQ_0;
    LED_blinkDuty = LED_BLINK_DUTY_HALF;
    OS_CreateTask(LED_BlinkTask, NULL, NULL, LED_BLINK_TASK_SIZE, LED_BLINK_TASK_PRIORITY, 0, 0, "LED blink task");    
}

void LED_BlinkSet(float freq, float duty) {
    LED_blinkFreq = freq;
    LED_blinkDuty = duty;
}

void LED_TurnOn(LED_INDEX i) {
    GPIO_SetLevel(config[i], GPIO_LEVEL_HIGH);
}

void LED_TurnOff(LED_INDEX i) {
    GPIO_SetLevel(config[i], GPIO_LEVEL_LOW);
}

void LED_Reversal(LED_INDEX i) {
	GPIO_LEVEL level;
    GPIO_GetLevel(config[i], &level);
	GPIO_SetLevel(config[i], (level==GPIO_LEVEL_HIGH)?GPIO_LEVEL_LOW:GPIO_LEVEL_HIGH);
}
