#include "twi.h"

#define soft_reset()        \
do                          \
{                           \
	wdt_enable(WDTO_15MS);  \
	for(;;)                 \
	{                       \
	}                       \
} while(0)

#define TWI_WHILE             \
i2c_error_counter = 10;       \
while (!(TWCR & (1<<TWINT))){ \
	if(!i2c_error_counter){   \ 
		soft_reset();         \
	}                         \
}                             \

void I2C_Init (void)
{
	TWBR=0x20;//скорость передачи (при 8 мгц получается 100 кгц, что и необходимо для общения с ds1307)
}


void I2C_StartCondition(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
	TWI_WHILE;//подождем пока установится TWIN
}

void I2C_StopCondition(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
}

void I2C_SendByte(unsigned char c)
{
	TWDR = c;//запишем байт в регистр данных
	TWCR = (1<<TWINT)|(1<<TWEN);//включим передачу байта
	TWI_WHILE;//подождем пока установится TWIN
}

void I2C_SendByteByADDR(unsigned char c,unsigned char addr)
{
	I2C_StartCondition(); // Отправим условие START
	I2C_SendByte(addr); // Отправим в шину адрес устройства + бит чтения-записи
	I2C_SendByte(c);// Отправим байт данных
	I2C_StopCondition();// Отправим условие STOP
}

unsigned char I2C_ReadByte(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWEA);
	TWI_WHILE;//ожидание установки бита TWIN
	return TWDR;//читаем регистр данных
}

unsigned char I2C_ReadLastByte(void)
{
	TWCR = (1<<TWINT)|(1<<TWEN);
	TWI_WHILE;//ожидание установки бита TWIN
	return TWDR;//читаем регистр данных
}