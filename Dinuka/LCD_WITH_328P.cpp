#ifndef F_CPU
#define F_CPU 16000000UL  //set the processor speed to 16 MHz
#endif
#include <avr/io.h>
#include <util/delay.h>
#define LCD_PORT PORTB
#define rs PC0
#define en PC1

void lcd_command(unsigned char command){
	LCD_PORT =(LCD_PORT & 0x0F)|(command & 0xF0);
	PORTC &=~(1<<rs);
	PORTC |=(1<<en);
	_delay_us(2);
	PORTC &=~(1<<en);
	_delay_us(200);
	LCD_PORT=(LCD_PORT & 0x0F)| (command<<4);
	PORTC|=(1<<en);
	_delay_us(2);
	PORTC &=~(1<<en);
	_delay_us(2);
}
void LCD_INIT(void){
	DDRB=0xF0;
	DDRC=0x03;
	_delay_ms(20);
	lcd_command(0x02);
	lcd_command(0x28);
	lcd_command(0x0C);
	lcd_command(0x06);
	lcd_command(0x01);
	_delay_ms(2);
}
void LCD_character(unsigned char data){
	LCD_PORT =(LCD_PORT & 0x0F)|(data & 0xF0);
	PORTC|=(1<<rs);
	PORTC|=(1<<en);
	_delay_us(2);
	PORTC &=~(1<<en);
	_delay_us(200);
	LCD_PORT=(LCD_PORT & 0x0F)| (data<<4);
	PORTC|=(1<<en);
	_delay_us(1);
	PORTC &=~(1<<en);
	_delay_ms(2);
}
void LCD_STRING(char *string1){
	for (int r=0;string1[r]!=0;r++){
		LCD_character(string1[r]);
	}
}

void LCD_Clear(){
	lcd_command(0x01);
	_delay_ms(2);
	lcd_command(0x80);
}
void LCD_print(void){
	LCD_Clear();
	LCD_STRING("Select the mode!");
	lcd_command(0xC0);
	LCD_STRING("Select number");
	_delay_ms(100);
	LCD_Clear();
	LCD_STRING("1:Calibration M");
	lcd_command(0xC0);
	LCD_STRING("2:Sensing Mode");
	_delay_ms(100);
	LCD_Clear();
	LCD_STRING("3:Light RGB LED");
}

int main(void)
{
	LCD_INIT();
	LCD_STRING("Hello world !");
	lcd_command(0xC0);
	LCD_STRING("Welcome !");
	_delay_ms(100);
	LCD_print();
}

