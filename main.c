/*
 * RGB Sensor.c
 *
 * Created: 5/6/2021 11:24:49 AM
 * Author : SSV
 */ 

#ifndef F_CPU
#define F_CPU 8000000UL // 8 MHz clock speed
#endif
#define D4 eS_PORTB4
#define D5 eS_PORTB5
#define D6 eS_PORTB6
#define D7 eS_PORTB7
#define RS eS_PORTB0
#define EN eS_PORTB1

unsigned char keypad[4][3] = {	{'1','2','3'},
{'4','5','6'},
{'7','8','9'},
{'*','0','#'}};

unsigned char colloc, rowloc;

/* calibration values */
int R255;
int R0;
int G255;
int G0;
int B255;
int B0;

char msg1[38]="Enter R,G,B values to light the LED";

#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"      //lcd library
#include <string.h>
#include <stdlib.h>
#include <math.h>

char keyfind()
{
	while(1)
	{
		DDRD |= 0xF0;
		DDRC &= ~(0x07);
		PORTD |= 0xF0;           /* set port direction as input-output */
		PORTC |= 0x07;

		do
		{
			PORTD &= 0x0F;
			PORTC &= 0xFF;     /* mask PORT for column read only */
			asm("NOP");
			_delay_ms(40);
			colloc = (PINC & 0x07);/* read status of column */
		}while((colloc & 0x07) == 0x07);
		_delay_ms(10);
		PORTD &= ~(0b00010000);
		PORTD |= 0xE0;     /* check for pressed key in 1st row */
		asm("NOP");
		colloc = (PINC & 0x07);
		if((colloc & 0x07) != 0x07)
		{
			rowloc = 0;
			break;
		}

		PORTD &= ~(0b00100000);
		PORTD |= 0xD0;     /* check for pressed key in 2nd row */
		asm("NOP");
		colloc = (PINC & 0x07);
		if((colloc & 0x07) != 0x07)
		{
			rowloc = 1;
			break;
		}
			
		PORTD &= ~(0b01000000);
		PORTD |= 0xB0;     /* check for pressed key in 3rd row */
		asm("NOP");
		colloc = (PINC & 0x07);
		if((colloc & 0x07) != 0x07)
		{
			rowloc = 2;
			break;
		}

		PORTD &= ~(0b10000000);
		PORTD |= 0x70;    /* check for pressed key in 4th row */
		asm("NOP");
		colloc = (PINC & 0x07);
		if((colloc & 0x07) != 0x07)
		{
			rowloc = 3;
			break;
		}
	}

	if((colloc & 0x07) == 0x06){
		return(keypad[rowloc][0]);
	}
	else if((colloc & 0x07) == 0x05){
		return(keypad[rowloc][1]);
	}
	else{
		return(keypad[rowloc][2]);
	}

}

/* display the color & value */
void display_text(char *color,char *digit){
	Lcd_Clear();
	Lcd_Write_String("Enter ");
	Lcd_Write_String(color);
	Lcd_Set_Cursor(2,0);
	Lcd_Write_String(color);
	Lcd_Write_String(":");
	Lcd_Write_String(digit);
	_delay_ms(80);
}

/* take inputs from the key pad & display it */
int take_inputs(char *color){
	char val[4]="";
	int val_=0;
	display_text(color,"");
	for(int i=0;i<3;i++){
		val[i]=keyfind();
		display_text(color,val);
	}
	val_=atoi(val);
	if(val_ > 255){
		return 1000;
	}
	return val_;
}

/* sensing a surface */
int sensing(char *color){
	int val=0;
	Lcd_Clear();
	Lcd_Write_String(color);
	Lcd_Write_String("..Press 1 to");
	Lcd_Set_Cursor(2,0);
	Lcd_Write_String("take the value");
	char key = keyfind();
	if(key == '1'){
		_delay_ms(50);
		for(int i=0;i<20;i++){
			val += analogRead();
		}
		val = val/20;  // A value between 0-1023
		Lcd_Clear();
		char _val[5];
		Lcd_Write_String(color);
		Lcd_Write_String(":");
		Lcd_Write_String(itoa(val,_val,10));
		_delay_ms(1000);
	}
	return val;
}

/* output the r,g,b values of a surface */
void sensor_out(int red,int green,int blue){
	long int rr=0;
	long int gg=0;
	long int bb=0;
	char R_[4];
	char G_[4];
	char B_[4];
	
	/* Values between 0-255 */
	rr=(((red-R0)*(255.))/(R255-R0));
	gg=(((green-G0)*(255.))/(G255-G0));
	bb=(((blue-B0)*(255.))/(B255-B0));
	Lcd_Clear();
	Lcd_Write_String("R: ");
	Lcd_Write_String(itoa(rr,R_,10));
	Lcd_Write_String("  G: ");
	Lcd_Write_String(itoa(gg,G_,10));
	Lcd_Set_Cursor(2,0);
	Lcd_Write_String("  B: ");
	Lcd_Write_String(itoa(bb,B_,10));
}

/* calibration */
int calib(char *color_card,char *color){
	int val=0;
	Lcd_Clear();
	Lcd_Write_String(color_card);
	_delay_ms(1500);
	Lcd_Clear();
	Lcd_Write_String("Press 1 to");
	Lcd_Set_Cursor(2,0);
	Lcd_Write_String("take the value");
	char key = keyfind();
	if(key == '1'){
		_delay_ms(50);
		for(int i=0;i<20;i++){
			val += analogRead();
		}
		val = val/20; // A value between 0-1023
		Lcd_Clear();
		char _val[5];
		Lcd_Write_String(color);
		Lcd_Write_String(":");
		Lcd_Write_String(itoa(val,_val,10));
		_delay_ms(1000);
	}
	return val;
}

/* analog read of pin A3 */
int analogRead(void){
	int val;
	ADMUX = 0x43;
	ADCSRA = 0x87;
	ADCSRB =0x00;
	ADCSRA |= (1 << ADSC);
	while ((ADCSRA & (1 << ADSC))){}
	val = (int)ADCL;
	val += (int)(ADCH << 8);
	return val;   // A value between 0-1023
}

/* RED led in fast PWM mode */
void red_led(int duty){
	DDRB |= 1<<2;
	TCCR1A |= (1<<COM1B1)|(1<<WGM12)|(1<<WGM10);
	OCR1B = duty;
	TCCR1B=(1<<CS10)|(1<<CS12);
}

/* GREEN led in fast PWM mode */
void green_led(int duty){
	DDRB |= 1<<3;
	TCCR2A |= (1<<COM2A1)|(1<<WGM21)|(1<<WGM20);
	OCR2A = duty;
	TCCR2B =(1<<CS22)|(1<<CS21)|(1<<CS20);
}

/* BLUE led in fast PWM mode */
void blue_led(int duty){
	DDRD |= 1<<3;
	TCCR2A |= (1<<COM2B1)|(1<<WGM21)|(1<<WGM20);
	OCR2B = duty;
	TCCR2B =(1<<CS22)|(1<<CS21)|(1<<CS20);
}



int main(void){
	DDRB |= 0xF3;
	Lcd_Init();
	Lcd_Write_String("Welcome!!");
	Lcd_Set_Cursor(2,0);
	Lcd_Write_String("Select a mode");
	_delay_ms(2000);
	while(1){
		Lcd_Clear();
		Lcd_Write_String("1:Calibration");
		Lcd_Set_Cursor(2,0);
		Lcd_Write_String("2:Sensing");
		_delay_ms(2000);
		Lcd_Clear();
		Lcd_Write_String("3:Lighting");
		char key = keyfind();
		
		/* Calibration mode : White & Black color card are used for calibration. Light up the RED LED and take the values for White & Black color cards. Same procedure happens with GREEN & BLUE LEDs sequentially.  */
		
		if (key == '1'){
			Lcd_Clear();
			Lcd_Write_String("Use following color cards for calibration");
			for(int i=0;i<26;i++){
				_delay_ms(200);
				Lcd_Shift_Left();
			}
			red_led(255);
			R255=calib("White color card","Red");
			R0=calib("Black color card","Red");
			red_led(0);
			green_led(255);
			G255=calib("White color card","Green");
			G0=calib("Black color card","Green");
			green_led(0);
			blue_led(255);
			B255=calib("White color card","Blue");
			B0=calib("Black color card","Blue");
			blue_led(0);
		}
		
		/* Sensing mode : Take the values of given surface with RED,GREEN & BLUE LEDs sequentially. By using those values & calibration values, Generate the R,G,B values of the given surface  */
		
		else if(key == '2'){
			int R=0;
			int G=0;
			int B=0;
			_delay_ms(100);
			red_led(255);
			R=sensing("Red");
			red_led(0);
			green_led(255);
			G=sensing("Green");
			green_led(0);
			blue_led(255);
			B=sensing("Blue");
			blue_led(0);
			sensor_out(R,G,B);
			_delay_ms(5000);
			Lcd_Clear();
			Lcd_Write_String("Press 1 to exit");
			_delay_ms(1500);
			sensor_out(R,G,B);
			char key=keyfind();
			if(key == '1'){
				_delay_ms(20);
				continue;
			}
			
		}
		
		/* Lighting mode : Take r,g,b values as inputs & light up the RGB LED according to those values */
		
		else if(key == '3'){
			Lcd_Clear();
			Lcd_Write_String(msg1);
			for(int i=0;i<21;i++){
				_delay_ms(200);
				Lcd_Shift_Left();
			}
			Lcd_Clear();
			Lcd_Write_String("Ex:add 25 as 025");
			Lcd_Set_Cursor(2,1);
			Lcd_Write_String("with leading 0s");
			_delay_ms(3000);
			int red =take_inputs("Red");
			if(red == 1000){
				Lcd_Clear();
				Lcd_Write_String("Invalid Input");
				_delay_ms(1500);
				continue;
			}
			int green = take_inputs("Green");
			if(green == 1000){
				Lcd_Clear();
				Lcd_Write_String("Invalid Input");
				_delay_ms(1500);
				continue;
			}
			int blue = take_inputs("Blue");
			if(blue == 1000){
				Lcd_Clear();
				Lcd_Write_String("Invalid Input");
				_delay_ms(1500);
				continue;
			}
			red_led(red);
			green_led(green);
			blue_led(blue);
			Lcd_Clear();
			Lcd_Write_String("Press 1 exit");
			char key = keyfind();
			if(key == '1'){
				_delay_ms(300);
				red_led(0);
				green_led(0);
				blue_led(0);
				continue;
			}
		}
	}	
}

