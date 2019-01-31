#ifndef __LED_H__
#define __LED_H__

typedef enum{
    LED_LED1 = 0,
    LED_LED2 = 1
}LED_INDEX;

typedef enum{
    LED_BLINK_FREQ_1HZ = 1,
    LED_BLINK_FREQ_2HZ = 2,
    LED_BLINK_FREQ_4HZ = 4,
    LED_BLINK_FREQ_8HZ = 8
}LED_BLINK_FREQ;

void LED_Init();
void LED_TurnOn(LED_INDEX i);
void LED_TurnOff(LED_INDEX i);
void LED_Reversal(LED_INDEX i);
void LED_Blink_L1(LED_BLINK_FREQ freq, int seconds);
void LED_Blink_L2(LED_BLINK_FREQ freq, int seconds);

#endif
