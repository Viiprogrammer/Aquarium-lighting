#include "led.h"
//Массив значений для вывода
uint8_t indicators[6] = {11, 11, 11, 11, 11, 11};
//Массив точек для вывода
uint8_t indicators_dots[6] = {0, 0, 0, 0, 0, 0};
//Знаки
unsigned char segments [] = {
	0b00111111,//0
	0b00000110,//1
	0b01011011,//2
	0b01001111,//3
	0b01100110,//4
	0b01101101,//5
	0b01111101,//6
	0b00000111,//7
	0b01111111,//8
	0b01101111,//9
	0b01000000,// -
	0b00000000,//Clear
	0b10000000,//.
	0b01010100,//m
	0b01110001,//F
	0b00111110,//U
	0b00111000,//L
	0b00111111,//O	
	0b01100011, //SUNSET
	0b01011100//DAMN
};

void timer1_led_init(void)
{
	TCCR1B |= (1<<WGM12); // устанавливаем режим СТС (сброс по совпадению)
	TIMSK |= (1<<OCIE1A);	//устанавливаем бит разрешения прерывания 1ого счетчика по совпадению с OCR1A(H и L)
	OCR1AH = 0b00000101; //записываем в регистр число для сравнения
	OCR1AL = 0b11011100;
	TCCR1B |= (1<<CS11);//установим делитель.
}
//Установка символа
void setCharIndicator(unsigned char number,  unsigned char i){
	indicators[i] = number;
}
//Установка точки
void setDotIndicator(unsigned char i, unsigned char state){
	indicators_dots[i] = state;
}
//Очистка точек
void ClearALLDotsIndicator(){
	for (unsigned char i = 0; i <= 5; i++)
	{
		indicators_dots[i] = 0;
	}
}
//Очистка  всех индикаторов
void ClearALLCharIndicator(){
	for (unsigned char i = 0; i <= 5; i++)
	{
		indicators[i] = 11;
	}
}
void DisplayLED(unsigned char number){
	    unsigned char char_temp = 0;
	    SPDR = ~(0b00000001 << number);
        
		//Передача
	    while(!(SPSR & (1<<SPIF))){
		   //Разрешение вложенных прерываний 
	    };
	 //Запрет вложенных прерываний
		
		char_temp = segments[indicators[number]];
		
		//Установка точки
	    if(indicators_dots[number]){ char_temp = char_temp | segments[12];}
	    	
	    SPDR = char_temp;
		
		//Передача
	    while(!(SPSR & (1<<SPIF))){
		   
	    };
		
        //Строб защелки
	    PORTB |= (1 << 2);
	    PORTB &= ~(1 << 2);
}

//Какая-то хуйня, потом пригодится 
//unsigned int indicator = 0b00000001;
/*
    for(uint8_t i = 0; i <=5; i++){
		PORTD |= (1 << PD4);
		SPDR = ~(0b00000001 << i);

		 
		while(!(SPSR & (1<<SPIF))){
			sei();
		};//подождем пока данные передадутся
		cli();
		char_temp = segments[indicators[i]];
		
		if(indicators_dots[i]) char_temp = char_temp | segments[12];;
		
		SPDR = char_temp;
		while(!(SPSR & (1<<SPIF))){
			sei();
		};//подождем пока данные передадутся
		//сгенерируем отрицательный фронт для записи в STORAGE REGISTER
		PORTD |= (1 << PD7); //высокий уровень
		PORTD &= ~(1 << PD7); //низкий уровень
	}
*/
	
unsigned char dissplay = 0;
ISR (TIMER1_COMPA_vect)
{
    DisplayLED(dissplay);
    dissplay++;
    if (dissplay > 5 ){ 
		dissplay = 0;
	}
}