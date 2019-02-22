#ifndef __LED_H__
#define __LED_H__

#define LED_BLINK_FREQ_0   0
#define LED_BLINK_FREQ_0_5 0.5
#define LED_BLINK_FREQ_1HZ 1.0
#define LED_BLINK_FREQ_2HZ 2.0
#define LED_BLINK_FREQ_4HZ 4.0
#define LED_BLINK_FREQ_8HZ 8.0

#define LED_BLINK_DUTY_FULL    1.0
#define LED_BLINK_DUTY_HALF    0.5
#define LED_BLINK_DUTY_QUARTER 0.25
#define LED_BLINK_DUTY_EIGHTH  0.125
#define LED_BLINK_DUTY_EMPTY   0.0

typedef enum{
    LED_LED1 = 0,
    LED_LED2 = 1
}LED_INDEX;

void LED_Init();
void LED_BlinkSet(float freq, float duty);
void LED_TurnOn(LED_INDEX i);
void LED_TurnOff(LED_INDEX i);
void LED_Reversal(LED_INDEX i);

#endif
