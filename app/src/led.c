
#include "led.h"
#include "api_hal_gpio.h"
#include "api_os.h"

GPIO_config_t config[2];

void LED_Init() {
	config[LED_LED1].mode         = GPIO_MODE_OUTPUT;
	config[LED_LED1].pin          = GPIO_PIN27;
	config[LED_LED1].defaultLevel = GPIO_LEVEL_LOW;
    GPIO_Init(config[LED_LED1]);
	
	config[LED_LED2].mode         = GPIO_MODE_OUTPUT;
	config[LED_LED2].pin          = GPIO_PIN28;
	config[LED_LED2].defaultLevel = GPIO_LEVEL_LOW;
    GPIO_Init(config[LED_LED2]);
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

void LED_Blink_L1(LED_BLINK_FREQ freq, int seconds) {
    bool b = false;
    for (int i=0;i<2*freq*seconds;i++) {
        b = (b ? false : true);
        b ? GPIO_SetLevel(config[LED_LED1], GPIO_LEVEL_HIGH):GPIO_SetLevel(config[LED_LED1], GPIO_LEVEL_LOW);
        OS_Sleep(500/freq);
    }
}

void LED_Blink_L2(LED_BLINK_FREQ freq, int seconds) {
    bool b = false;
    for (int i=0;i<2*freq*seconds;i++) {
        b = (b ? false : true);
        b ? GPIO_SetLevel(config[LED_LED2], GPIO_LEVEL_HIGH):GPIO_SetLevel(config[LED_LED2], GPIO_LEVEL_LOW);
        OS_Sleep(500/freq);
    }
}
