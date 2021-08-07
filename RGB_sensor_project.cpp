#ifndef F_CPU
#define F_CPU 16000000UL  //set the processor speed to 16 MHz
#endif
#include <avr/io.h>
#include <util/delay.h>  
#include <avr/interrupt.h>
#include <stdlib.h>
#include <math.h>
#include <avr/eeprom.h>
#define LCD_PORT PORTC //define the connected data port
#define rs PB4
#define en PB5
#define red_high_memory 0x001
#define green_high_memory 0x002
#define blue_high_memory 0x003
#define red_low_memory 0x004
#define green_low_memory 0x005
#define blue_low_memory 0x006
// initialized values for some readings
long  int RED1,RED_HIGH=0,RED_LOW=0;
long  int GREEN1,GREEN_HIGH=0,GREEN_LOW=0;
long  int BLUE1,BLUE_HIGH=0,BLUE_LOW=0;
unsigned char col1, row1;
int GREEN_DUTY_CYCLE;int RED_DUTY_CYCLE;int BLUE_DUTY_CYCLE;
unsigned char keypad[4][3]={{'1','2','3'},
							{'4','5','6'},
							{'7','8','9'},
							{'*','0','#'}};  //array for the keypad
	
void lcd_command(unsigned char command);
void LCD_INIT(void);
void LCD_character(unsigned char data);
void LCD_STRING(char *string1);
void LCD_Clear(void);
void LCD_print(void);
char keyfind(void);
void ADC_Init(void);
int ADC_Read(char channel);
void CALIBRATION(void);
void SENSING_MODE(void);
void LIGHT_RGB_LED(void);

/*function to give commands*/
void lcd_command(unsigned char command){
	LCD_PORT =((command & 0xF0)>>4);  //sending the upper nibble of the command
	PORTB &=~(1<<rs);  //disable the rs pin
	PORTB |=(1<<en);   // make the rs resistor high
	_delay_us(2);
	PORTB &=~(1<<en);
	_delay_us(200);
	LCD_PORT=(command & 0x0F);  //sending the lower nibble of the command
	PORTB|=(1<<en);
	_delay_us(2);
	PORTB &=~(1<<en);
	_delay_us(2);
}
/*initialize the lcd display for 4 bit mode*/
void LCD_INIT(void){
	DDRC=0x0F;
	DDRB=0x30;  //make the PC 0,1,2,3 and PB 4,5 as output pins
	_delay_ms(20);
	lcd_command(0x02);  // initialize for 4 bit mode
	lcd_command(0x28);
	lcd_command(0x0C); // display on cursor off
	lcd_command(0x06); // shift cursor to right
	lcd_command(0x01); // clear display screen;
	_delay_ms(2);
}
/*function to print a character on the LCD display*/
void LCD_character(unsigned char data){
	LCD_PORT =((data & 0xF0)>>4);  //sending the upper nibble of the character
	PORTB|=(1<<rs);
	PORTB|=(1<<en);
	_delay_us(2);
	PORTB &=~(1<<en);
	_delay_us(200);
	LCD_PORT=(data & 0x0F); // sending the lower nibble of the character
	PORTB|=(1<<en);
	_delay_us(1);
	PORTB &=~(1<<en);
	_delay_ms(2);
}
/*function to print a string (a set of characters)*/

void LCD_STRING(char *string1,int val){
	lcd_command(val);
	for (int r=0;string1[r]!=0;r++){
		LCD_character(string1[r]);
	}
}
/*function to clear the LCD display*/
void LCD_Clear(void){
	lcd_command(0x01);
	_delay_ms(2);
	lcd_command(0x80); //set the cursor to home position
}
/*user defined function to print the initial data instructions on the display*/
char keyfind(void){
	
	while(1){
		DDRD = 0xF8;     // set the port B as input(column) and output(Row)
		PORTD =0xF7;	// set all the connected pins to high
		do
		{
			PORTD &= 0x07;      // mask PORT for column read only
			asm("NOP");
			_delay_ms(40);
			col1 = (PIND & 0x07);    // read status of column
		}while(col1 == 0x07);       // if any button is pressed then do the following
		_delay_ms(200);
		// now check for rows
		PORTD = 0xE7;            /* check for pressed key in 1st row */
		asm("NOP");
		col1 = (PIND & 0x0F);
		if(col1 != 0x07)
		{
			row1 = 0;
			break;
		}
		PORTD = 0xD7;		/* check for pressed key in 2nd row */
		asm("NOP");
		col1 = (PIND & 0x07);
		if(col1 != 0x07)
		{
			row1 = 1;
			break;
		}
		
		PORTD = 0xB7;		/* check for pressed key in 3rd row */
		asm("NOP");
		col1 = (PIND & 0x07);
		if(col1 != 0x07)
		{
			row1 = 2;
			break;
		}
		PORTD = 0x77;		/* check for pressed key in 4th row */
		asm("NOP");
		col1 = (PIND & 0x07);
		if(col1 != 0x0F7)
		{
			row1 = 3;
			break;
		}
	}
	if(col1 == 0x06){
		return(keypad[row1][0]);
	}
	else if(col1 == 0x05){
		return(keypad[row1][1]);
	}
	else if(col1 == 0x03){
		return(keypad[row1][2]);
	}
}
void ADC_Init()
{
	DDRC &=~(1<<5);			// Make ADC 5 port as input 
	ADCSRA = 0x87;			// Enable ADC, fr/128 
	ADMUX = 0x45;			// Vref: Avcc, ADC channel: 5
	
}

int ADC_Read(char channel)
{
	int Ain,AinLow;
	
	ADMUX|=(channel & 0x0F);	// Set input channel to read 

	ADCSRA |= (1<<ADSC);		// Start conversion 
	while((ADCSRA&(1<<ADIF))==0);	// Monitor end of conversion interrupt 
	
	_delay_us(10);
	AinLow = (int)ADCL;		//Read lower byte
	Ain = (int)ADCH*256;		// Read higher 2 bits and Multiply with weight 
	Ain = Ain + AinLow;				
	return(Ain);			// Return digital value
}

/*function for the calibration mode*/
void CALIBRATION(void){
	DDRB|=(1<<0); // make the required connected pins as output pins
	DDRC|=(1<<4);
	DDRD|=(1<<3);
	/*eeprom_write_byte((uint8_t*)red_high_memory,0);
	eeprom_write_byte((uint8_t*)green_high_memory,0);
	eeprom_write_byte((uint8_t*)blue_high_memory,0);
	eeprom_write_byte((uint8_t*)red_low_memory,0);
	eeprom_write_byte((uint8_t*)green_low_memory,0);
	eeprom_write_byte((uint8_t*)blue_low_memory,0);*/
	ADC_Init();
	LCD_Clear();
 //display some useful messages
	LCD_STRING("Calibrating",0x82);
	LCD_STRING("Black Surface",0xC2);
	_delay_ms(1000);
	LCD_Clear();
	LCD_STRING("Press # to start",0x80);
	// this calibration will not start until we press the '#' button
	char key2=keyfind();
	if (key2=='#'){
		LCD_Clear();
		LCD_STRING("Calibrating",0x82);
		PORTD |=(1<<3); //light the red color of the RGB LED
		PORTB &=~(1<<0);
		PORTC &=~(1<<4);
		_delay_ms(2000);
		for (int r=1;r<=10;r++){ //get the RED value from the surface(10 values 
			RED_LOW+=ADC_Read('5'); //to ensure the accuracy 
			_delay_ms(10);
		}
		PORTD &=~(1<<3);
		PORTB |=(1<<0);
		PORTC &=~(1<<4);
		_delay_ms(2000);
		for (int r=1;r<=10;r++){
			GREEN_LOW+=ADC_Read('5'); //get 10 values for the Green color from red surface
			_delay_ms(10);
		}
		PORTD &=~(1<<3);
		PORTB &=~(1<<0);
		PORTC |=(1<<4);
		_delay_ms(2000);
		for (int r=1;r<=10;r++){
			BLUE_LOW+=ADC_Read('5'); //get ten readings for the Blue color from red surface
			_delay_ms(10);
		}
	}
	LCD_Clear();
	LCD_STRING("Done!",0x80);
	PORTD &=~(1<<3);
	PORTB &=~(1<<0);
	PORTC &=~(1<<4);
	_delay_ms(1000);
	LCD_Clear();
	LCD_STRING("Calibrating",0x82);
	LCD_STRING("White surface",0xC1);
	_delay_ms(1000);
	LCD_Clear();
	LCD_STRING("Press # to start",0x80); //same thing for the green surface as the red surface
		char key3=keyfind();
		if (key3=='#'){
			LCD_Clear();
			LCD_STRING("Calibrating",0x82);
			PORTD &=~(1<<3);
			PORTB |=(1<<0);
			PORTC &=~(1<<4);
			_delay_ms(2000);
			for (int r=1;r<=10;r++){
				GREEN_HIGH+=ADC_Read('5');
				_delay_ms(10);
			}
			PORTD |=(1<<3);
			PORTB &=~(1<<0);
			PORTC &=~(1<<4);
			_delay_ms(2000);
			for (int r=1;r<=10;r++){
				RED_HIGH+=ADC_Read('5');
				_delay_ms(10);
			}
			PORTD &=~(1<<3);
			PORTB &=~(1<<0);
			PORTC |=(1<<4);
			_delay_ms(2000);
			for (int r=1;r<=10;r++){
				BLUE_HIGH+=ADC_Read('5');
				_delay_ms(10);
			}
		}
	LCD_Clear();
	LCD_STRING("Done!",0x80);
	PORTD &=~(1<<3);
	PORTB &=~(1<<0);
	PORTC &=~(1<<4);
	RED_HIGH=((RED_HIGH/10)*255.)/1023;
	GREEN_HIGH=((GREEN_HIGH/10)*255.)/1023;
	BLUE_HIGH=((BLUE_HIGH/10)*255.)/1023;
	RED_LOW=((RED_LOW/20)*255.)/1023;
	GREEN_LOW=((GREEN_LOW/20)*255.)/1023;
	BLUE_LOW=((BLUE_LOW/20)*255.)/1023;
	eeprom_update_byte((uint8_t*)red_high_memory,RED_HIGH);
	eeprom_update_byte((uint8_t*)green_high_memory,GREEN_HIGH);
	eeprom_update_byte((uint8_t*)blue_high_memory,BLUE_HIGH);
	eeprom_update_byte((uint8_t*)red_low_memory,RED_LOW);
	eeprom_update_byte((uint8_t*)green_low_memory,GREEN_LOW);
	eeprom_update_byte((uint8_t*)blue_low_memory,BLUE_LOW);
	eeprom_update_byte((uint8_t*)0x007,1);
	_delay_ms(1000);
	LCD_Clear();
	LCD_print();
}
//define a function to do the sensing part
void SENSING_MODE(void){
	DDRB|=(1<<0); // make the required connected pins as output pins
	DDRC|=(1<<4);
	DDRD|=(1<<3);
	RED1=0;BLUE1=0;GREEN1=0;
	char REDs[3];
	char GREENs[3];
	char BLUEs[3];
	unsigned int RED=0,GREEN=0,BLUE=0;
	ADC_Init();
	LCD_Clear();
	LCD_STRING("work is in",0x83);
	LCD_STRING("progress",0xC4);
	PORTD |=(1<<3);
	PORTB &=~(1<<0);
	PORTC &=~(1<<4);
	_delay_ms(100);
	for (int r=1;r<=10;r++){//sense the red color intensity of the given surface
		RED+=ADC_Read('5');
		_delay_ms(100);
	}
	PORTD &=~(1<<3);
	PORTB |=(1<<0);
	PORTC &=~(1<<4);
	_delay_ms(100);
	for (int r=1;r<=10;r++){ // sense the green color intensity of the surface
		GREEN+=ADC_Read('5');
		_delay_ms(100);
	}
	PORTD &=~(1<<3);
	PORTB &=~(1<<0);
    PORTC |=(1<<4);
	_delay_ms(100);
	for (int r=1;r<=10;r++){ //sense the green color intensity of the Blue surface
		 BLUE+=ADC_Read('5');
		 _delay_ms(100);
	 }
	PORTD &=~(1<<3);
	PORTB &=~(1<<0);
	PORTC &=~(1<<4);
	 //convert the measured values to a more reliable region
	uint8_t red_high=eeprom_read_byte((uint8_t*)red_high_memory);
	uint8_t green_high=eeprom_read_byte((uint8_t*)green_high_memory);
	uint8_t blue_high=eeprom_read_byte((uint8_t*)blue_high_memory);
	uint8_t red_low=eeprom_read_byte((uint8_t*)red_low_memory);
	uint8_t green_low=eeprom_read_byte((uint8_t*)green_low_memory);
	uint8_t blue_low=eeprom_read_byte((uint8_t*)blue_low_memory);
	unsigned int red_high1=(red_high*1023.)/255;
	unsigned int green_high1=((green_high)*1023.)/255;
	unsigned int blue_high1=((blue_high)*1023.)/255;
	unsigned int red_low1=red_low*1023./255;
	unsigned int green_low1=(green_low*1023.)/255;
	unsigned int blue_low1=blue_low*1023./255;
	RED1=(RED/10-red_low1)*255./(red_high1-red_low1);
	BLUE1=(BLUE/10-blue_low1)*255./(blue_high1-blue_low1);
	GREEN1=(GREEN/10-green_low1)*255./(green_high1-green_low1);
	 LCD_Clear();
	 //display those values on the LCD screen
	 LCD_STRING("  R    G    B",0x80);
	 itoa(RED1,REDs,10);//convert integer value to a string
	 LCD_STRING(REDs,0xC1);
	 itoa(GREEN1,GREENs,10);
	 LCD_STRING(GREENs,0xC6);
	 itoa(BLUE1,BLUEs,10);
	 LCD_STRING(BLUEs,0xCB);
	 _delay_ms(1000);
	 LCD_Clear();
	 LCD_print();
 }
 void pwmStart(){
	 DDRB|=(1<<3)|(1<<2)|(1<<1);
	 TCCR2A=(1<<COM2A1)|(1<<WGM20)|(1<<WGM21); //set the OC2A port for fast PMW non inverting method
	 TCCR1A=(1<<COM1B1)|(1<<WGM10)|(1<<COM1A1)|(1<<WGM12);
	 TIMSK1 =(1<<TOIE1);
	 TIMSK2=(1<<TOIE2);								
	 OCR2A=BLUE_DUTY_CYCLE; // set the duty cycle 
	 OCR1B=GREEN_DUTY_CYCLE;
	 OCR1A=RED_DUTY_CYCLE;
	 sei();
	 TCCR2B=(1<<CS22)|(1<<CS21)|(1<<CS20);
	 TCCR1B=(1<<CS10)|(1<<CS12);
 }
 void pwmStop(){
	  TCCR2A &=~(1<<COM2A1)& ~(1<<WGM20)& ~(1<<WGM21);
	  TCCR1A &=~(1<<COM1B1)& ~(1<<WGM10) & ~(1<<COM1A1)& ~(1<<WGM12);
	  cli();
	  TCCR2B&=~(1<<CS22)&~(1<<CS21)&~(1<<CS20);
	  TCCR1B&=~(1<<CS10)&~(1<<CS12);
 }
void LIGHT_RGB_LED(void){
	LCD_Clear();
	LCD_STRING("Enter RGB values",0x80);
	LCD_STRING("between 0-255",0xC1);
	_delay_ms(1000);
	LCD_Clear();
	LCD_STRING("Press # to enter",0x80);
	LCD_STRING("& * to backspace",0xC0);
	_delay_ms(1000);
	LCD_Clear();
	LCD_STRING("Enter R value",0x80);
	int val1=0;
	int pos=0;
	while (1){
		char key1=keyfind();
		/*if we have pressed a number then execute the following*/
		if (key1!='#' && key1!='*'){
			val1=(key1-48)+(val1*10);
			lcd_command(0xC0|pos);
			pos++;
			LCD_character(key1);
			_delay_ms(60);
		}
		else if (key1=='#'){
			_delay_ms(20);
			/*if the entered value is greater than
			then display the following message to enter a new number*/
			if (val1>255){
				LCD_Clear();
				LCD_STRING("Invalid Value",0x80);
				_delay_ms(1500);
				LCD_Clear();
				LCD_STRING("Re-enter R value",0x80);
				val1=0;
				pos=0;
			}
			//if the entered number is in the 0-255 range then break the loop
			else{
				RED1=val1;
				break;
			}
		}
		else if (key1=='*'){
			if (pos!=0){
				lcd_command(0xC0|(pos-1));
				LCD_character(' ');
				_delay_ms(60);
				val1=val1/10;
				pos--;
			}
		}
	}
	val1=0;
	pos=0;
	LCD_Clear();
	LCD_STRING("Enter G value",0x80);
	// same thing what we did for red
	while (1){
		char key1=keyfind();
		if (key1!='#' && key1!='*'){
			val1=(key1-48)+(val1*10);
			lcd_command(0xC0|pos);
			pos++;
			LCD_character(key1);
			_delay_ms(60);
		}
		else if (key1=='#'){
			_delay_ms(20);
			/*if the entered value is greater than
			then display the following message to enter a new number*/
			if (val1>255){
				LCD_Clear();
				LCD_STRING("Invalid Value",0x80);
				_delay_ms(1500);
				LCD_Clear();
				LCD_STRING("Re-enter G value",0x80);
				val1=0;
				pos=0;
			}
			//if the entered number is in the 0-255 range then break the loop
			else{
				GREEN1=val1;
				break;
			}
		}
		else if (key1=='*'){
			if (pos!=0){
				lcd_command(0xC0|(pos-1));
				LCD_character(' ');
				_delay_ms(60);
				val1=val1/10;
				pos--;
			}
		}
	}
	pos=0;
	val1=0;;
	LCD_Clear();
	LCD_STRING("Enter B value",0x80);
	//same thing that we did for red
	while (1){
		char key1=keyfind();
		if (key1!='#' && key1!='*'){
			val1=(key1-48)+(val1*10);
			lcd_command(0xC0|pos);
			pos++;
			LCD_character(key1);
			_delay_ms(60);
		}
		else if (key1=='#'){
			_delay_ms(20);
			/*if the entered value is greater than
			then display the following message to enter a new number*/
			if (val1>255){
				LCD_Clear();
				LCD_STRING("Invalid Value",0x80);
				_delay_ms(1500);
				LCD_Clear();
				LCD_STRING("Re-enter B value",0x80);
				val1=0;
				pos=0;
			}
			//if the entered number is in the 0-255 range then break the loop
			else{
				BLUE1=val1;
				break;
			}
		}
		else if (key1=='*'){
			if (pos!=0){
				lcd_command(0xC0|(pos-1));
				LCD_character(' ');
				_delay_ms(60);
				val1=val1/10;
				pos--;
			}
		}
	}
	//write pwm values
	pwmStart();
	RED_DUTY_CYCLE=RED1;
	GREEN_DUTY_CYCLE=GREEN1;
	BLUE_DUTY_CYCLE=BLUE1;
	LCD_Clear();
	LCD_STRING("Thanks for using",0x80);
	LCD_STRING("the device",0xC0);
	_delay_ms(1500);
    LCD_Clear();
    LCD_STRING("Press # to exit",0x80);
    char key_1=keyfind();
	if (key_1=='#'){
		pwmStop();
		LCD_Clear();
		LCD_print();
	}
}
int main(void)
{
	LCD_INIT();
	LCD_STRING("Hello world!",0x82);
	LCD_STRING("Welcome!",0xC4);
	_delay_ms(1000);
	LCD_print();
	while(1){
		char key=keyfind();
		if (key){
			if (key=='1'){
				CALIBRATION();
			}
			else if(key=='2'){
				uint8_t num=eeprom_read_byte((uint8_t*)0x007);
				if (num!=1){
					LCD_Clear();
					LCD_STRING("Calibrate first",0x80);
					LCD_STRING("Press 1",0xC4);
				}
				else{
					SENSING_MODE();
				}
			}
			else if(key=='3'){
				LIGHT_RGB_LED();
			}
			
		}
	}
	return 0;
}
//to control the overflow of pwm 
void LCD_print(void){
	LCD_Clear();
	LCD_STRING("Select the mode!",0x80);
 //set the cursor at the starting position of the second line
	LCD_STRING("Select number",0xC0);
	_delay_ms(1000);
	LCD_Clear();
	LCD_STRING("1:Calibration",0x80);
	LCD_STRING("Mode",0xC6);
	_delay_ms(1000);
	LCD_Clear();
	LCD_STRING("2:Sensing Mode",0x80);
	_delay_ms(1000);
	LCD_Clear();
	LCD_STRING("3:Light the RGB",0x80);
 //set  the cursor at the 2nd row 3rd column
	LCD_STRING("LED",0xC5);
}
ISR(TIMER1_OVF_vect){
	OCR1A=RED_DUTY_CYCLE;
	OCR1B=GREEN_DUTY_CYCLE;
}
ISR(TIMER2_OVF_vect){
	OCR2A=BLUE_DUTY_CYCLE;
}