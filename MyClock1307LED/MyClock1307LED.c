#include "main.h"

#define SHIFT_REG_DDR DDRB
#define SHIFT_REG_PORT PORTB
#define DATA 3
#define SCL  5
#define LATCH  2

//ШИМ настройки
#define R 5
#define G 6
#define B 7
#define W  0

#define DDR_R DDRD
#define DDR_G DDRD
#define DDR_B DDRD
#define DDR_W  DDRB

#define PORT_R PORTD
#define PORT_G PORTD
#define PORT_B PORTD
#define PORT_W  PORTB

#define RGBW 666 //Welcom to HELL

#define BUTTON_SPROB_LED_PORT PORTB
#define BUTTON_SPROB_LED_DDR DDRB
#define BUTTON_SPROB_LED_PIN PB6

//Режимы меню
#define MODE_NONE 0
#define MODE_EDIT_HOURS 1
#define MODE_EDIT_MINUTES 2
#define MODE_EDIT_DAY 3
#define MODE_EDIT_MOUTH 4
#define MODE_EDIT_YEAR 5
#define MODE_EDIT_WEEK_DAY 6

//Меню настройки рассветов и закатов
#define MODE_EDIT_START_TIME_DAWN_HOURS 7
#define MODE_EDIT_START_TIME_DAWN_MINS 8
#define MODE_EDIT_INTERVAL_DAWN 9

#define MODE_EDIT_START_TIME_SUNSET_HOURS 10
#define MODE_EDIT_START_TIME_SUNSET_MINS 11
#define MODE_EDIT_INTERVAL_SUNSET 12

//ID инкрементов
#define MODE_INC 1
#define MODE_DISINC 2
#define MODE_NONE_INC 0

#define LIGHT_DAWM 1
#define LIGHT_SUNSET 2
#define LIGHT_FULL 3
#define LIGHT_OFF 4

//Переменные Времени и Даты
unsigned char sec = 0, min = 0, hour = 0, day = 0, date = 0, month = 0, year = 0;

//Временные переменные 
unsigned char sec_, min_, hour_, day_, date_, month_, year_, hour_, min_;
unsigned char temp_mins_dawm = 0, temp_hours_dawm = 0, temp_mins_sunset = 0, temp_hours_sunset = 0;;
unsigned int excess_mins_dawm = 0, excess_mins_sunset = 0;

//Переменные рассвета и заката
uint16_t led_menu_dawn_hours = 0, led_menu_dawn_mins = 0, led_menu_dawn_interval_mins= 0, led_menu_sunset_hours = 0, led_menu_sunset_mins = 0, led_menu_sunset_interval_mins = 0;

//Кнопки
uint8_t button, button_code;

//Меню
uint8_t now_mode = 0, increment_mode = 0;
uint16_t now_mode_led_menu = 0, increment_mode_led_menu = 0, now_mode_light = 0, prev_now_mode_light = 0;

//EEPORM
uint16_t ee_led_menu_dawn_hours EEMEM = 0, ee_led_menu_dawn_mins EEMEM = 0, ee_led_menu_dawn_interval_mins EEMEM = 0, ee_led_menu_sunset_hours EEMEM = 0, ee_led_menu_sunset_mins EEMEM = 0, ee_led_menu_sunset_interval_mins EEMEM = 0;


//ШИМ каналы RGBW
unsigned char red, green = 0, blue = 0, white = 0;       //Переменные скважности ШИМ
unsigned char red_b  = 0, green_b = 0, blue_b = 0, white_b = 0;     //Буферизация значений скважности ШИМ
unsigned char count = 0, i2c_time_disable = 0;                           //Счетчик вызовов обработчика прерываний

volatile char twoHZ = 255;

volatile char twoHZ_status = 0;
 
//Прерывание моргалки
ISR(TIMER2_COMP_vect)
{
	 twoHZ--;
	 if(!twoHZ){
	    twoHZ_status = !twoHZ_status;
		twoHZ = 255;
	}
}

//Прерывание ШИМ
ISR (TIMER0_OVF_vect)
{
	count++;
	if (count == 0){//Переполнение
		
		//Сохранием значения в буфер
		red_b   = red; 
		green_b = green;
		blue_b  = blue;
		white_b  = white;
		
       //Высокий уровень
	   PORT_R |= (1<<R);
	   PORT_G |= (1<<G);
	   PORT_B |= (1<<B);
	   PORT_W |= (1<<W);
		
	}
	
	//Если достигли ширины импульса, низкий уровень
	if (red_b   == count) { PORT_R &=~ (1<<R);}
	if (green_b == count) { PORT_G &=~ (1<<G);}
	if (blue_b  == count) {  PORT_B &=~ (1<<B);}
	if (white_b  == count) { 	PORT_W &= ~(1<<W);}
}

//Преобразование диапазона
long map(long x, long in_min, long in_max, long out_min, long out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

//Строб светодиода кнопки
void led_strob(){
	BUTTON_SPROB_LED_PORT |= (1 << BUTTON_SPROB_LED_PIN);
	_delay_ms(100);
	BUTTON_SPROB_LED_PORT &= ~(1 << BUTTON_SPROB_LED_PIN);
}

//SPI инициализация 
void SPI_init(void)
{
	SHIFT_REG_DDR |= ((1<<DATA)|(1<<SCL));
	SHIFT_REG_PORT &= ~((1<<DATA)|(1<<SCL));
	SHIFT_REG_DDR |= (1<<LATCH) ;
	SHIFT_REG_PORT &=~ (1 << LATCH);
	SPCR = ((1<<SPE)|(1<<MSTR));//Включение, режим MASTER
}

//PWM инициализация 
void PWM_init(){
	DDR_R |= (1<<R);
	DDR_G |= (1<<G);
	DDR_B |= (1<<B);
	DDR_W |= (1<<W);
	
	PORT_R &= ~(1<<R);
	PORT_G &= ~(1<<G);
	PORT_B &= ~(1<<B);
	PORT_W &= ~(1<<W);

	DDRB &= ~(1<<PB7);
    TCCR0 |= (1 << CS10);
    TCNT0 = 0x00;
    TIMSK |= (1 << TOIE0);
}

//Установка скважности
void PWM_set(unsigned int channel, unsigned char val){
	if(channel == R){
		red = val;
	}else if(channel == G){
	    green = val;
    }else if(channel == B){
	  blue = val;
    }else if(channel == W){
      white = val;
    }else if(channel == RGBW){
      red = val;
	  green = val;
	  blue = val;
	  white = val;
    }
}

//Настройки времени
void ModifyRTC(void)
{
	I2C_StartCondition();
	I2C_SendByte(0b11010000);
	switch(now_mode){
		case MODE_EDIT_HOURS: //часы
			I2C_SendByte(2);//Переходим на 0x02 - байт часов
			if(increment_mode == MODE_INC){
			  if(hour<23){ 
				  I2C_SendByte(RTC_ConvertFromBinDec(hour+1));
				  hour++;
			  }else{ 
				  I2C_SendByte(RTC_ConvertFromBinDec(0));
				  hour = 0;
			  }
			}else if(increment_mode == MODE_DISINC){
			  if(hour>0){
			    I2C_SendByte(RTC_ConvertFromBinDec(hour-1));
				hour--;
			  }else{ 
				I2C_SendByte(RTC_ConvertFromBinDec(23));
				hour = 23;
			  }
			}
			
		break;
		
		case MODE_EDIT_MINUTES: // минуты
			I2C_SendByte(1);//Переходим на 0x01 - байт минут
			if(increment_mode == MODE_INC){
             if(min<59){ 
				 I2C_SendByte(RTC_ConvertFromBinDec(min+1));
				 min++;
             }else{ 
				 I2C_SendByte(RTC_ConvertFromBinDec(0));
				 min = 0;
			 }
		    }else if(increment_mode == MODE_DISINC){
				if(min>0){ 
					I2C_SendByte(RTC_ConvertFromBinDec(min-1));
					min--;
				}else{ 
					I2C_SendByte(RTC_ConvertFromBinDec(59));
					min = 59;
				}
			}
		break;
		
		
		case MODE_EDIT_DAY: // дата
		 	I2C_SendByte(4);//Переходим на 0x04 - байт числа даты
			if (month==2) //февраль
			{
				if(year%4==0) { //Если високосный год
					 if(increment_mode == MODE_INC){
						 if(date<29){ 
							 I2C_SendByte(RTC_ConvertFromBinDec(date+1));
							 date++;
						 }else{ 
							 I2C_SendByte(RTC_ConvertFromBinDec(1));
							 date = 1;
						 }
				     }else if(increment_mode == MODE_DISINC){
						 if(date>1){ 
							 I2C_SendByte(RTC_ConvertFromBinDec(date-1));
							 date--;
						 }else{ 
							 I2C_SendByte(RTC_ConvertFromBinDec(29));
							 date = 29;
						 }
			         }
				}else{
					if(increment_mode == MODE_INC){
						if(date<28){
							I2C_SendByte(RTC_ConvertFromBinDec(date+1));
							date++;
						}else{ 
							I2C_SendByte(RTC_ConvertFromBinDec(1));
							date = 1;
						}
					}else if(increment_mode == MODE_DISINC){
						 if(date>1){ 
							 I2C_SendByte(RTC_ConvertFromBinDec(date-1));
							 date--;
						 }else{ 
							 I2C_SendByte(RTC_ConvertFromBinDec(28));
							 date = 28;
						 }
					}				
				}
			}else if ((month==4)|(month==6)|(month==9)|(month==11)){
				if(increment_mode == MODE_INC){
					if(date<30){ 
						I2C_SendByte(RTC_ConvertFromBinDec(date+1));
						date++;
					}else{ 
						I2C_SendByte(RTC_ConvertFromBinDec(1));
						date = 1;
					}
				}else if(increment_mode == MODE_DISINC){
					if(date>1){ 
						I2C_SendByte(RTC_ConvertFromBinDec(date-1));
						date--;
					}else{ 
						I2C_SendByte(RTC_ConvertFromBinDec(30));
						date = 30;
					}
				}			
			}else{
				if(increment_mode == MODE_INC){
					if(date<31){ 
						I2C_SendByte(RTC_ConvertFromBinDec(date+1));
						date++;
					}else{ 
						I2C_SendByte(RTC_ConvertFromBinDec(1));
						date = 1;
					}
				}else if(increment_mode == MODE_DISINC){
					if(date>1){ 
						I2C_SendByte(RTC_ConvertFromBinDec(date-1));
						date--;
					}else{ 
						I2C_SendByte(RTC_ConvertFromBinDec(31));
						date = 31;
					}
				}
			}
		break;
		
		case MODE_EDIT_MOUTH: // месяц
		 	I2C_SendByte(5);//Переходим на 0x05 - байт месяца
			
			if(increment_mode == MODE_INC){
			 if(month<12){ 
				 I2C_SendByte(RTC_ConvertFromBinDec(month+1));
				 month++;
			 }else{ 
				 I2C_SendByte(RTC_ConvertFromBinDec(1));
				 month = 1;
			 }
			}else if(increment_mode == MODE_DISINC){
			 if(month > 1){ 
				 I2C_SendByte(RTC_ConvertFromBinDec(month-1));
				 month--;
			 }else{ 
				 I2C_SendByte(RTC_ConvertFromBinDec(12));
				 month = 12;
			 }
			}		
		break;
		
		
		case MODE_EDIT_YEAR: // год
		 	I2C_SendByte(6);//Переходим на 0x06 - байт года
			if(increment_mode == MODE_INC){
			 if(year<99){ 
				 I2C_SendByte(RTC_ConvertFromBinDec(year+1));
				 year++;
			 }else{ 
				 I2C_SendByte(RTC_ConvertFromBinDec(1));
				 year = 1;
			 }
			}else if(increment_mode == MODE_DISINC){
			 if(year>1){
				  I2C_SendByte(RTC_ConvertFromBinDec(year-1));
				  year--;
			 }else{ 
				  I2C_SendByte(RTC_ConvertFromBinDec(99));
				  year = 99;
			 }
			}
		break;
		
		
		case MODE_EDIT_WEEK_DAY: // день недели
		 	I2C_SendByte(3);//Переходим на 0x03 - байт дня недели
			if(increment_mode == MODE_INC){
			    if(day<7){ 
					I2C_SendByte(RTC_ConvertFromBinDec(day+1));
					day++;
			    }else{ 
					I2C_SendByte(RTC_ConvertFromBinDec(1));
					day = 1;
				}
			}else if(increment_mode == MODE_DISINC){
				if(day>1){ 
					I2C_SendByte(RTC_ConvertFromBinDec(day-1));
					day--;
				}else{ 
					I2C_SendByte(RTC_ConvertFromBinDec(7));
					day = 7;
				}
			}
	    break;

	}
	I2C_StopCondition();
}

//Настройка рассвета/заката
void ModifyLEDMenu(void)
{
	switch(now_mode_led_menu){
		    case MODE_EDIT_START_TIME_DAWN_HOURS: // день недели
			    if(increment_mode_led_menu == MODE_INC){
					if(led_menu_dawn_hours<10) led_menu_dawn_hours++;
					else led_menu_dawn_hours = 3;
				}else if(increment_mode_led_menu == MODE_DISINC){
					if(led_menu_dawn_hours>4) led_menu_dawn_hours--;
					else led_menu_dawn_hours = 10;
			    }
			break;
			
		    case MODE_EDIT_START_TIME_DAWN_MINS: // день недели
				 if(increment_mode_led_menu == MODE_INC){
					 if(led_menu_dawn_mins<59) led_menu_dawn_mins++;
					 else led_menu_dawn_mins = 0;
				 }else if(increment_mode_led_menu == MODE_DISINC){
					 if(led_menu_dawn_mins>0) led_menu_dawn_mins--;
					 else led_menu_dawn_mins = 59;
				 }
		    break;
			
			case MODE_EDIT_INTERVAL_DAWN: // день недели
				 if(increment_mode_led_menu == MODE_INC){
					 if(led_menu_dawn_interval_mins<120) led_menu_dawn_interval_mins += 15;
					 else led_menu_dawn_interval_mins = 15;
					 }else if(increment_mode_led_menu == MODE_DISINC){
					 if(led_menu_dawn_interval_mins>15) led_menu_dawn_interval_mins -= 15;
					 else led_menu_dawn_interval_mins = 120;
				 }
		    break;
			
		    case MODE_EDIT_START_TIME_SUNSET_HOURS: // день недели
		    if(increment_mode_led_menu == MODE_INC){
			    if(led_menu_sunset_hours<21) led_menu_sunset_hours++;
			    else led_menu_sunset_hours = 13;
			    }else if(increment_mode_led_menu == MODE_DISINC){
			    if(led_menu_sunset_hours>13) led_menu_sunset_hours--;
			    else led_menu_sunset_hours = 21;
		    }
		    break;
		    
		    case MODE_EDIT_START_TIME_SUNSET_MINS: // день недели
		    if(increment_mode_led_menu == MODE_INC){
			    if(led_menu_sunset_mins<59) led_menu_sunset_mins++;
			    else led_menu_sunset_mins = 0;
			    }else if(increment_mode_led_menu == MODE_DISINC){
			    if(led_menu_sunset_mins>0) led_menu_sunset_mins--;
			    else led_menu_sunset_mins = 59;
		    }
		    break;
		    
		    case MODE_EDIT_INTERVAL_SUNSET: // день недели
		    if(increment_mode_led_menu == MODE_INC){
			    if(led_menu_sunset_interval_mins<120) led_menu_sunset_interval_mins += 15;
			    else led_menu_sunset_interval_mins = 15;
			    }else if(increment_mode_led_menu == MODE_DISINC){
			    if(led_menu_sunset_interval_mins>15) led_menu_sunset_interval_mins -= 15;
			    else led_menu_sunset_interval_mins = 120;
		    }
		    break;
	}

}

void ShowTime(unsigned char hour, unsigned char min){
	setCharIndicator(hour/10, 1);
	setCharIndicator(hour%10, 2);
	
	setCharIndicator(min/10, 3);
	setCharIndicator(min%10, 4);
}

void ShowDate(unsigned char date, unsigned char month, unsigned char year){
	setCharIndicator(date/10, 0);
	setCharIndicator(date%10, 1);
	setDotIndicator(1, 1);
	
	setCharIndicator(month/10, 2);
	setCharIndicator(month%10, 3);
	setDotIndicator(3, 1);
	
	setCharIndicator(year/10, 4);
	setCharIndicator(year%10, 5);
}

int main(void)
{
	
	//Инициализация перефирии
	I2C_Init();
	PWM_init();
	BUT_Init();
	BUTTON_SPROB_LED_DDR |= (1 << BUTTON_SPROB_LED_PIN);
	
	SPI_init();
	timer1_led_init();
	
	//на случай если зависнет мк, то при подаче питания свет будет включен
	PORT_R |= (1<<R);
	PORT_G |= (1<<G);
	PORT_B |= (1<<B);
	PORT_W |= (1<<W);
	
    sei(); 
	
	//1Hz generator
	DDRC &= ~(1 << PC3);
	I2C_StartCondition();
	I2C_SendByte(0b11010000);
	I2C_SendByte(7);//Переходим на 0x07
	I2C_SendByte(0b00010000); //включим SQWE
	I2C_StopCondition();
	

    OCR2 = 255;

    TCCR2 |= (1 << WGM21);
    // Set to CTC Mode

    TIMSK |= (1 << OCIE2);
    //Set interrupt on compare match

    TCCR2 |= ((1 << CS21) | (1 << CS20) | (1 << CS20));
    // set prescaler to 64 and starts PWM
	
	
	//Восстанавливаем значения из EEPOM
	led_menu_dawn_hours = eeprom_read_word(&ee_led_menu_dawn_hours);
	led_menu_dawn_mins = eeprom_read_word(&ee_led_menu_dawn_mins);
	led_menu_dawn_interval_mins = eeprom_read_word(&ee_led_menu_dawn_interval_mins);
	led_menu_sunset_hours = eeprom_read_word(&ee_led_menu_sunset_hours);
	led_menu_sunset_mins = eeprom_read_word(&ee_led_menu_sunset_mins);
    led_menu_sunset_interval_mins = eeprom_read_word(&ee_led_menu_sunset_interval_mins);
		
	while(1)
	{   	  //Принудительное включение 
				if(now_mode_led_menu == MODE_NONE && now_mode == MODE_NONE){
				if((PINB & (1 << PB7))){	
				//Супер дупер крутой рассвет и закат
				if((hour >= led_menu_dawn_hours && min > led_menu_dawn_mins) || (hour > led_menu_dawn_hours && min < led_menu_dawn_mins)){
					if(hour >= led_menu_dawn_hours && min > led_menu_dawn_mins){
				
						temp_hours_dawm = hour-led_menu_dawn_hours;
						temp_mins_dawm = min-led_menu_dawn_mins;
						excess_mins_dawm = temp_hours_dawm*60 + temp_mins_dawm;
					}
					if(hour > led_menu_dawn_hours && min < led_menu_dawn_mins){
						temp_hours_dawm = hour-led_menu_dawn_hours;
						temp_mins_dawm = led_menu_dawn_mins-min;
						excess_mins_dawm = temp_hours_dawm*60 - temp_mins_dawm;
					}
			
					if(excess_mins_dawm < led_menu_dawn_interval_mins){
						PWM_set(RGBW, map(excess_mins_dawm, 1, led_menu_dawn_interval_mins, 0, 255));
						now_mode_light = LIGHT_DAWM;
						}else{
						if((hour >= led_menu_sunset_hours && min > led_menu_sunset_mins) || (hour > led_menu_sunset_hours && min < led_menu_sunset_mins)){
							if(hour >= led_menu_sunset_hours && min > led_menu_sunset_mins){
						
								temp_hours_sunset = hour-led_menu_sunset_hours;
								temp_mins_sunset = min-led_menu_sunset_mins;
								excess_mins_sunset = temp_hours_sunset*60 + temp_mins_sunset;
							}
							if(hour > led_menu_sunset_hours && min < led_menu_sunset_mins){
								temp_hours_sunset = hour-led_menu_sunset_hours;
								temp_mins_sunset = led_menu_sunset_mins-min;
								excess_mins_sunset = temp_hours_sunset*60 - temp_mins_sunset;
							}
							if(excess_mins_sunset < led_menu_sunset_interval_mins){
								PWM_set(RGBW, map(excess_mins_sunset, 1, led_menu_sunset_interval_mins, 255, 0));
								now_mode_light = LIGHT_SUNSET;
								}else{ PWM_set(RGBW, 0x00); now_mode_light = LIGHT_OFF;}
								}else{ PWM_set(RGBW, 0xFF); now_mode_light = LIGHT_FULL;}
							}
							}else{  
								PWM_set(RGBW, 0x00); now_mode_light = LIGHT_OFF;
							}			
					
							if(prev_now_mode_light != now_mode_light ){
								ClearALLCharIndicator();
							}
					
							prev_now_mode_light = now_mode_light;
							TIMSK |= (1 << TOIE0);
					}else{
						//Подаем + на драйвера если тумблер включен
						TIMSK &=~ (1 << TOIE0);
						PORT_R |= (1<<R);
						PORT_G |= (1<<G);
						PORT_B |= (1<<B);
						PORT_W |= (1<<W);
					}
					
					//Читаем время
						
					I2C_SendByteByADDR(0,0b11010000);	//Установка адреса в 0
					I2C_StartCondition(); //Отправим условие START
					I2C_SendByte(0b11010001); //Бит на чтение
					sec_ = I2C_ReadByte();
					min_ = I2C_ReadByte();
					hour_ = I2C_ReadByte();
					day_ = I2C_ReadByte();
					date_ = I2C_ReadByte();
					month_ = I2C_ReadByte();
					year_ = I2C_ReadLastByte();
					I2C_StopCondition(); //Отправим условие STOP
					sec = RTC_ConvertFromDec(sec_); //Преобразуем в десятичный формат
					min = RTC_ConvertFromDec(min_); //Преобразуем в десятичный формат
					hour = RTC_ConvertFromDec(hour_); //Преобразуем в десятичный формат
					day = RTC_ConvertFromDec(day_); //Преобразуем в десятичный формат
					year = RTC_ConvertFromDec(year_); //Преобразуем в десятичный формат
					month = RTC_ConvertFromDec(month_); //Преобразуем в десятичный формат
					date = RTC_ConvertFromDec(date_); //Преобразуем в десятичный формат
					if((PINB & (1 << PB7))){
					  TIMSK |= (1 << TOIE0);
					}
		}else{
		    PORT_R &= ~(1<<R);
			PORT_G &= ~(1<<G);
			PORT_B &= ~(1<<B);
			PORT_W &= ~(1<<W);
			TIMSK &=~ (1 << TOIE0);
		}
		
		//Модификация данных если выключены настроки и изменяется значение
	    if((now_mode != MODE_NONE) && ((increment_mode == MODE_INC) || (increment_mode == MODE_DISINC)))
		{
				ModifyRTC();
				increment_mode = MODE_NONE_INC;
	    }
		
	    if((now_mode_led_menu != MODE_NONE) && ((increment_mode_led_menu == MODE_INC) || (increment_mode_led_menu == MODE_DISINC)))
	    {
				    ModifyLEDMenu();
				    increment_mode_led_menu = MODE_NONE_INC;
	    }
				
		//Опрашиваем кнопки				
		BUT_Poll();
		button = BUT_GetBut(); 
		if(button){
		    button_code = BUT_GetBut();  
			switch(button){
				//Plus
				case 1:   
				 if(button_code == BUT_RELEASED_EN){
				  led_strob();
				  if(now_mode != MODE_NONE){
				   increment_mode = MODE_INC; 
				  }else if(now_mode_led_menu != MODE_NONE){
				   increment_mode_led_menu = MODE_INC;
				  }
				 }
				break;
			
				//Minus
				case 2: 
				 if(button_code == BUT_RELEASED_EN){
				  led_strob();
				  if(now_mode != MODE_NONE){
				   increment_mode = MODE_DISINC;
				  }else if(now_mode_led_menu != MODE_NONE){
				   increment_mode_led_menu = MODE_DISINC;
				  }
				 }
				break;
			
				//Okey
				case 3: 
				  if(now_mode_led_menu == MODE_NONE && ((button_code == BUT_RELEASED_LONG_CODE && now_mode == MODE_NONE) || (now_mode != MODE_NONE && button_code == BUT_RELEASED_EN))){ 
				   led_strob();
				   if(now_mode == MODE_NONE){
					//Редактируем часы
					now_mode = MODE_EDIT_HOURS;
                    ClearALLCharIndicator();
                    ClearALLDotsIndicator();
				   }else if(now_mode == MODE_EDIT_HOURS){
					//Минуты
					now_mode = MODE_EDIT_MINUTES; 
				   }else if(now_mode == MODE_EDIT_MINUTES){
					//Число
					now_mode = MODE_EDIT_DAY;
					ClearALLCharIndicator();
				   }else if(now_mode == MODE_EDIT_DAY){
					//Месяц
					now_mode = MODE_EDIT_MOUTH;
				   }else if(now_mode == MODE_EDIT_MOUTH){
					//Год
					now_mode = MODE_EDIT_YEAR;
				   }else if(now_mode == MODE_EDIT_YEAR){
					ClearALLCharIndicator();
					ClearALLDotsIndicator();
					//День
					now_mode = MODE_EDIT_WEEK_DAY;
				   }else if(now_mode == MODE_EDIT_WEEK_DAY){
					I2C_StartCondition();
					I2C_SendByte(0b11010000);
					I2C_SendByte(0);
					I2C_SendByte(RTC_ConvertFromBinDec(1));
					I2C_StopCondition();
					ClearALLCharIndicator();
					now_mode = MODE_NONE;
					increment_mode = MODE_NONE_INC;
				   }
				  }
				break;
			
				case 4: 
				  if(now_mode == MODE_NONE && ((button_code == BUT_RELEASED_LONG_CODE && now_mode_led_menu == MODE_NONE) || (now_mode_led_menu != MODE_NONE && button_code == BUT_RELEASED_EN))){
						  led_strob();
						  if(now_mode_led_menu == MODE_NONE){
							//Часы начала рассвета
							ClearALLCharIndicator();
							now_mode_led_menu = MODE_EDIT_START_TIME_DAWN_HOURS;
						  }else if(now_mode_led_menu == MODE_EDIT_START_TIME_DAWN_HOURS){
							eeprom_write_word(&ee_led_menu_dawn_hours , led_menu_dawn_hours);
							//Минуты начала рассвета
							now_mode_led_menu = MODE_EDIT_START_TIME_DAWN_MINS;
						  }else if(now_mode_led_menu == MODE_EDIT_START_TIME_DAWN_MINS){
							eeprom_write_word(&ee_led_menu_dawn_mins, led_menu_dawn_mins);
							//Длительность рассвета
							ClearALLCharIndicator();
							now_mode_led_menu = MODE_EDIT_INTERVAL_DAWN;
						  }else if(now_mode_led_menu == MODE_EDIT_INTERVAL_DAWN){
							  eeprom_write_word(&ee_led_menu_dawn_interval_mins, led_menu_dawn_interval_mins);
							//Часы начала заката
							ClearALLCharIndicator();
							now_mode_led_menu = MODE_EDIT_START_TIME_SUNSET_HOURS;
						  }else if(now_mode_led_menu == MODE_EDIT_START_TIME_SUNSET_HOURS){
							 eeprom_write_word(&ee_led_menu_sunset_hours, led_menu_sunset_hours);
							 //Минуты начала заката
							 now_mode_led_menu = MODE_EDIT_START_TIME_SUNSET_MINS;
						  }else if(now_mode_led_menu == MODE_EDIT_START_TIME_SUNSET_MINS){
							 eeprom_write_word(&ee_led_menu_sunset_mins, led_menu_sunset_mins);
							 //Длительность заката
							 ClearALLCharIndicator();
							 now_mode_led_menu = MODE_EDIT_INTERVAL_SUNSET;
						  }else if(now_mode_led_menu == MODE_EDIT_INTERVAL_SUNSET){
							eeprom_write_word(&ee_led_menu_sunset_interval_mins, led_menu_sunset_interval_mins);
							ClearALLCharIndicator();
							now_mode_led_menu = MODE_NONE;
							increment_mode_led_menu = MODE_NONE_INC;
						  }
					}
				break;
			
			}
		}

        //Вывод текущего режима, если мы не в меню
		
        if(now_mode_led_menu == MODE_NONE && now_mode == MODE_NONE){
			ShowTime(hour, min);
			setDotIndicator(2, (PINC & (1 << PC3)));
			if(now_mode_light == LIGHT_DAWM){
				setCharIndicator(18, 0);
				setCharIndicator(18, 5);
			}else if(now_mode_light == LIGHT_SUNSET){
				setCharIndicator(19, 0);
				setCharIndicator(19, 5);
			}
			/*
			if(now_mode_light == LIGHT_FULL){
				setCharIndicator(14, 1);
				setCharIndicator(15, 2);
				setCharIndicator(16, 3);	
				setCharIndicator(16, 4);
			}else if(now_mode_light == LIGHT_OFF){
				setCharIndicator(17, 1);
				setCharIndicator(14, 2);
				setCharIndicator(14, 3);
		   }else if(now_mode_light == LIGHT_DAWM){
				setCharIndicator(18, 0);
				setCharIndicator(18, 1);
				setCharIndicator(18, 2);
				setCharIndicator(18, 3);
				setCharIndicator(18, 4);
				setCharIndicator(18, 5);
           }else if(now_mode_light == LIGHT_SUNSET){
			   setCharIndicator(19, 0);
			   setCharIndicator(19, 1);
			   setCharIndicator(19, 2);
			   setCharIndicator(19, 3);
			   setCharIndicator(19, 4);
			   setCharIndicator(19, 5);
           }
		   */
        }
		
		if(now_mode_led_menu){
			//Меню настроки заката/рассвета
			switch(now_mode_led_menu){
				case MODE_EDIT_START_TIME_DAWN_HOURS: 
				  if(!twoHZ_status){
					   setCharIndicator(led_menu_dawn_hours/10, 1);
					   setCharIndicator(led_menu_dawn_hours%10, 2);
					   }else{
					   setCharIndicator(11, 1);
					   setCharIndicator(11, 2);
				   }
				   setCharIndicator(led_menu_dawn_mins/10, 3);
				   setCharIndicator(led_menu_dawn_mins%10, 4);				
				break;

				case MODE_EDIT_START_TIME_DAWN_MINS: 
				  setCharIndicator(led_menu_dawn_hours/10, 1);
				  setCharIndicator(led_menu_dawn_hours%10, 2);
				  if(!twoHZ_status){
					   setCharIndicator(led_menu_dawn_mins/10, 3);
					   setCharIndicator(led_menu_dawn_mins%10, 4);
				  }else{
					   setCharIndicator(11, 3);
					   setCharIndicator(11, 4);
				  }
				break;
			
			
				case MODE_EDIT_INTERVAL_DAWN: 
				  if(!twoHZ_status){
					  setCharIndicator(led_menu_dawn_interval_mins/100, 1);
					  setCharIndicator((led_menu_dawn_interval_mins%100)/10, 2);
					  setCharIndicator(led_menu_dawn_interval_mins%100%10, 3);
					  setCharIndicator(13, 4);
				  }else{
					  setCharIndicator(11, 1);
					  setCharIndicator(11, 2);
					  setCharIndicator(11, 3);
					  setCharIndicator(13, 4);
				  }

				break;
			
				case MODE_EDIT_START_TIME_SUNSET_HOURS:
				  if(!twoHZ_status){
					  setCharIndicator(led_menu_sunset_hours/10, 1);
					  setCharIndicator(led_menu_sunset_hours%10, 2);
					  }else{
					  setCharIndicator(11, 1);
					  setCharIndicator(11, 2);
				  }
				  setCharIndicator(led_menu_sunset_mins/10, 3);
				  setCharIndicator(led_menu_sunset_mins%10, 4);				
				break;
			
			
				case MODE_EDIT_START_TIME_SUNSET_MINS: 
				  setCharIndicator(led_menu_sunset_hours/10, 1);
				  setCharIndicator(led_menu_sunset_hours%10, 2);
				  if(!twoHZ_status){
					  setCharIndicator(led_menu_sunset_mins/10, 3);
					  setCharIndicator(led_menu_sunset_mins%10, 4);
					  }else{
					  setCharIndicator(11, 3);
					  setCharIndicator(11, 4);
				  }
				break;
			
			
				case MODE_EDIT_INTERVAL_SUNSET: 		  
				  if(!twoHZ_status){
					  setCharIndicator(led_menu_sunset_interval_mins/100, 1);
					  setCharIndicator((led_menu_sunset_interval_mins%100)/10, 2);
					  setCharIndicator(led_menu_sunset_interval_mins%100%10, 3);
					  setCharIndicator(13, 4);
					  }else{
					  setCharIndicator(11, 1);
					  setCharIndicator(11, 2);
					  setCharIndicator(11, 3);
					  setCharIndicator(13, 4);
				  }				break;
			
			}
		}
		if(now_mode){
			//Меню настроки часов
			switch(now_mode){
				case MODE_EDIT_HOURS: //часы
				   if(!twoHZ_status){
				    setCharIndicator(hour/10, 1);
				    setCharIndicator(hour%10, 2);
				   }else{
					setCharIndicator(11, 1);
					setCharIndicator(11, 2);				   
				   }
				   setCharIndicator(min/10, 3);
				   setCharIndicator(min%10, 4);
				break;

				case MODE_EDIT_MINUTES: // минуты
				   setCharIndicator(hour/10, 1);
				   setCharIndicator(hour%10, 2);
				   if(!twoHZ_status){
					  setCharIndicator(min/10, 3);
					  setCharIndicator(min%10, 4);
				   }else{
					  setCharIndicator(11, 3);
					  setCharIndicator(11, 4);
				   }
				break;
			
			
				case MODE_EDIT_DAY: // дата
				   ShowDate(date, month, year);
				break;
				
				case MODE_EDIT_MOUTH: // месяц
				   ShowDate(date, month, year);
				break;
				
				
				case MODE_EDIT_YEAR: // год
				   ShowDate(date, month, year);
				break;
				
				
				case MODE_EDIT_WEEK_DAY: // день недели
					setCharIndicator(10, 0);
					setCharIndicator(10, 1);
			   		setCharIndicator(0, 2);
			   		setCharIndicator(day, 3);
					setCharIndicator(10, 4);
					setCharIndicator(10, 5);
				break;
			
			}
		
		}
		
	}
}
