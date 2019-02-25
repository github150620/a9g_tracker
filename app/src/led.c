// Copyright © 2019 - WU PENG. All Rights Reserved.
//
#include "led.h"
#include "api_hal_gpio.h"
#include "api_os.h"

#define LED_BLINK_TASK_SIZE      (1024 * 1)
#define LED_BLINK_TASK_PRIORITY  64

GPIO_config_t config[2];

typedef struct{
    LED_INDEX index;
    float freq;
    float duty;
}LED_BLINK;

LED_BLINK LED_blink[2];

void LED_BlinkTask(VOID *pData) {
    LED_BLINK *p = pData;
    while (1) {
        if (p->freq==LED_BLINK_FREQ_0) {
            OS_Sleep(1000);
        } else if (p->duty==LED_BLINK_DUTY_FULL) {
            GPIO_SetLevel(config[p->index], GPIO_LEVEL_HIGH);
            OS_Sleep(1000);
        } else if (p->duty==LED_BLINK_DUTY_EMPTY) {
            GPIO_SetLevel(config[p->index], GPIO_LEVEL_LOW);
            OS_Sleep(1000);
        } else {
            GPIO_SetLevel(config[p->index], GPIO_LEVEL_HIGH);
            OS_Sleep( p->duty * 1000.0 / p->freq );
            GPIO_SetLevel(config[p->index], GPIO_LEVEL_LOW);
            OS_Sleep( ( 1 - p->duty ) * 1000.0 / p->freq );
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

    LED_blink[0].index = LED_LED1;
    LED_blink[0].freq = LED_BLINK_FREQ_0;
    LED_blink[0].duty = LED_BLINK_DUTY_HALF;
    OS_CreateTask(LED_BlinkTask, &LED_blink[0], NULL, LED_BLINK_TASK_SIZE, LED_BLINK_TASK_PRIORITY, 0, 0, "LED blink task1");

    LED_blink[1].index = LED_LED2;
    LED_blink[1].freq = LED_BLINK_FREQ_0;
    LED_blink[1].duty = LED_BLINK_DUTY_HALF;
    OS_CreateTask(LED_BlinkTask, &LED_blink[1], NULL, LED_BLINK_TASK_SIZE, LED_BLINK_TASK_PRIORITY+1, 0, 0, "LED blink task2");
}

void LED_SetBlink(LED_INDEX i, float freq, float duty) {
    LED_blink[i].freq = freq;
    LED_blink[i].duty = duty;
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
