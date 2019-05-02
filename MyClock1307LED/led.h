#ifndef LED_H_
#define LED_H_

#include "main.h"

//--------------------------------------------
void LED_print(unsigned int number);
void timer1_led_init(void);
void setCharIndicator(unsigned char number,  unsigned char i);
void setDotIndicator(unsigned char i, unsigned char state);
void ClearALLDotsIndicator();
void ClearALLCharIndicator();
void DisplayLED(unsigned char number);
uint8_t indicators_dots[6];
uint8_t indicators[6];

#endif /* LED_H_ */