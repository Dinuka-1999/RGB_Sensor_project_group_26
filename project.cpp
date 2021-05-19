#ifndef F_CPU
#define F_CPU 16000000UL  //set the processor speed to 16 MHz
#endif
#include <avr/io.h>
#include <util/delay.h>  
#include <stdlib.h>
#include <math.h>
#include <avr/interrupt.h>
#define LCD_PORT PORTC //define the connected data port
#define rs PB4
#define en PB5
// initialized values for some readings
long  int RED1,RED_HIGH=0,RED_LOW=0,RED_DUTY_CYCLE;
long  int GREEN1,GREEN_HIGH=0,GREEN_LOW=0,GREEN_DUTY_CYCLE;
long  int BLUE1,BLUE_HIGH=0,BLUE_LOW=0,BLUE_DUTY_CYCLE;

unsigned char col1, row1;
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
	LCD_PORT =(LCD_PORT & 0xF0)|((command & 0xF0)>>4);  //sending the upper nibble of the command
	PORTB &=~(1<<rs);  //disable the rs pin
	PORTB |=(1<<en);   // make the rs resistor high
	_delay_us(2);
	PORTB &=~(1<<en);
	_delay_us(200);
	LCD_PORT=(LCD_PORT & 0xF0)| (command & 0x0F);  //sending the lower nibble of the command
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
	LCD_PORT =(LCD_PORT & 0xF0)|((data & 0xF0)>>4);  //sending the upper nibble of the character
	PORTB|=(1<<rs);
	PORTB|=(1<<en);
	_delay_us(2);
	PORTB &=~(1<<en);
	_delay_us(200);
	LCD_PORT=(LCD_PORT & 0xF0)| (data & 0x0F); // sending the lower nibble of the character
	PORTB|=(1<<en);
	_delay_us(1);
	PORTB &=~(1<<en);
	_delay_ms(2);
}
/*function to print a string (a set of characters)*/
void LCD_STRING(char *string1){
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
		_delay_ms(20);
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
	ADC_Init();
	LCD_Clear();
	lcd_command(0x80); //display some useful messages
	LCD_STRING("Calibrating");
	lcd_command(0xC0);
	LCD_STRING("Red Surface");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Press # to start");
	// this calibration will not start until we press the '#' button
	while (true){
		char key2=keyfind();
		if (key2=='#'){
			LCD_Clear();
			lcd_command(0x80);
			LCD_STRING("Calibrating");
			PORTD |=(1<<3); //light the red color of the RGB LED
			PORTB &=~(1<<0);
			PORTC &=~(1<<4);
			for (int r=1;r<=10;r++){ //get the RED value from the surface(10 values 
				RED_HIGH+=ADC_Read('5'); //to ensure the accuracy 
				_delay_ms(100);
			}
			PORTD &=~(1<<3);
			PORTB |=(1<<0);
			PORTC &=~(1<<4);
			for (int r=1;r<=10;r++){
				GREEN_LOW+=ADC_Read('5'); //get 10 values for the Green color from red surface
				_delay_ms(100);
			}
			PORTD &=~(1<<3);
			PORTB &=~(1<<0);
			PORTC |=(1<<4);
			for (int r=1;r<=10;r++){
				BLUE_LOW+=ADC_Read('5'); //get ten readings for the Blue color from red surface
				_delay_ms(100);
			}
			break;
		}
	}
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Done!");
	PORTD &=~(1<<3);
	PORTB &=~(1<<0);
	PORTC &=~(1<<4);
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Calibrating");
	lcd_command(0xC0);
	LCD_STRING("GREEN surface");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Press # to start"); //same thing for the green surface as the red surface
	while (true){
		char key3=keyfind();
		if (key3=='#'){
			LCD_Clear();
			lcd_command(0x80);
			LCD_STRING("Calibrating");
			PORTD &=~(1<<3);
			PORTB |=(1<<0);
			PORTC &=~(1<<4);
			for (int r=1;r<=10;r++){
				GREEN_HIGH+=ADC_Read('5');
				_delay_ms(100);
			}
			PORTD |=(1<<3);
			PORTB &=~(1<<0);
			PORTC &=~(1<<4);
			for (int r=1;r<=10;r++){
				RED_LOW+=ADC_Read('5');
				_delay_ms(100);
			}
			PORTD &=~(1<<3);
			PORTB &=~(1<<0);
			PORTC |=(1<<4);
			for (int r=1;r<=10;r++){
				BLUE_LOW+=ADC_Read('5');
				_delay_ms(100);
			}
			break;
		}
	}
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Done!");
	PORTD &=~(1<<3);
	PORTB &=~(1<<0);
	PORTC &=~(1<<4);
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Calibrating");
	lcd_command(0xC0);
	LCD_STRING("BLUE surface");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Press # to start"); // same procedure for the Blue Surface
	while (true){
		char key4=keyfind();
		if (key4=='#'){
			LCD_Clear();
			lcd_command(0x80);
			LCD_STRING("Calibrating");
			PORTD &=~(1<<3);
			PORTB &=~(1<<0);
			PORTC |=(1<<4);
			for (int r=1;r<=10;r++){
				BLUE_HIGH+=ADC_Read('5');
				_delay_ms(100);
			}
			PORTD |=(1<<3);
			PORTB &=~(1<<0);
			PORTC &=~(1<<4);
			for (int r=1;r<=10;r++){
				RED_LOW+=ADC_Read('5');
				_delay_ms(100);
			}
			PORTD &=~(1<<3);
			PORTB |=(1<<0);
			PORTC &=~(1<<4);
			for (int r=1;r<=10;r++){
				GREEN_LOW+=ADC_Read('5');
				_delay_ms(100);
			}
			break;
		}
	}
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Done!");
	PORTD &=~(1<<3);
	PORTB &=~(1<<0);
	PORTC &=~(1<<4);
	_delay_ms(1000);
	LCD_Clear();
	LCD_print();
}
//define a function to do the sensing part
void SENSING_MODE(void){
	char REDs[3];
	char GREENs[3];
	char BLUEs[3];
	int RED=0,GREEN=0,BLUE=0;
	ADC_Init();
	LCD_Clear();
	lcd_command(0x83);
	LCD_STRING("work is in");
	lcd_command(0xC4);
	LCD_STRING("progress");
	PORTD |=(1<<3);
	PORTB &=~(1<<0);
	PORTC &=~(1<<4);
	for (int r=1;r<=10;r++){//sense the red color intensity of the given surface
		RED+=ADC_Read('5');
		_delay_ms(100);
	}
	PORTD &=~(1<<3);
	PORTB |=(1<<0);
	PORTC &=~(1<<4);
	for (int r=1;r<=10;r++){ // sense the green color intensity of the surface
		GREEN+=ADC_Read('5');
		_delay_ms(100);
	}
	PORTD &=~(1<<3);
	PORTB &=~(1<<0);
    PORTC |=(1<<4);
	for (int r=1;r<=10;r++){ //sense the green color intensity of the Blue surface
		 BLUE+=ADC_Read('5');
		 _delay_ms(100);
	 }
	PORTD &=~(1<<3);
	PORTB &=~(1<<0);
	PORTC &=~(1<<4);
	 //convert the measured values to a more reliable region
	 RED1=((RED/10)-(RED_LOW/20))*(255.)/(RED_HIGH/10-RED_LOW/20);
	 BLUE1=((BLUE/10)-(BLUE_LOW/20))*(255.)/(BLUE_HIGH/10-BLUE_LOW/20);
	 GREEN1=((GREEN/10)-(GREEN_LOW/20))*(255.)/(GREEN_HIGH/10-GREEN_LOW/20);
	 LCD_Clear();
	 lcd_command(0x80);
	 //display those values on the LCD screen
	 LCD_STRING("  R    G    B");
	 lcd_command(0xC1);
	 itoa(RED1,REDs,10);//convert integer value to a string
	 LCD_STRING(REDs);
	 lcd_command(0xC6);
	 itoa(GREEN1,GREENs,10);
	 LCD_STRING(GREENs);
	 lcd_command(0xCB);
	 itoa(BLUE1,BLUEs,10);
	 LCD_STRING(BLUEs);
	 _delay_ms(1000);
	 LCD_Clear();
	 LCD_print();
 }
 void BLUE_light(void){
	 DDRB|=(1<<3);
	 TCCR2A=(1<<COM2A1)|(1<<WGM20)|(1<<WGM21); //set the OC2A port for fast PMW non inverting method
	 TIMSK2=(1<<TOIE2);								
	 OCR2A=BLUE_DUTY_CYCLE; // set the duty cycle 
	 sei();
	 TCCR2B=(1<<CS22)|(1<<CS21)|(1<<CS20);
 }
 void GREEN_RED_light(void){
	 DDRB|=(1<<2)|(1<<1); //set the OC1A and OC1B for fast pwm mode
	 TCCR1A=(1<<COM1B1)|(1<<WGM12)|(1<<WGM10)|(1<<COM1A1);
	 TIMSK1 =(1<<TOIE1);
	 OCR1A=RED_DUTY_CYCLE;
	 OCR1B=GREEN_DUTY_CYCLE;
	 sei();
	 TCCR1B=(1<<CS10)|(1<<CS12);
 }
void LIGHT_RGB_LED(void){
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Enter RGB values");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Enter R value");
	int val1=0;
	int power1=0;
	while (1){
		char key1=keyfind();
		/*if we have pressed a number then execute the following*/
		if (key1){
			val1+=(key1-48)*pow(10,2-power1);
			power1++;
			_delay_ms(60);
		}
		if (power1==3){
			/*if the entered value is greater than
			then display the following message to enter a new number*/
			if (val1>255){
				LCD_Clear();
				lcd_command(0x80);
				LCD_STRING("Invalid Value");
				lcd_command(0xC0);
				LCD_STRING("Enter a new val");
				_delay_ms(1000);
				LCD_Clear();
				lcd_command(0x80);
				LCD_STRING("Enter R value");
				val1=0;
				power1=0;
			}
			//if the entered number is in the 0-255 range then break the loop
			else{
				RED1=val1;
				break;
			}
		}
	}
	val1=0;
	power1=0;
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Done");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Enter G value");
	// same thing what we did for red
	while (1){
		char key1=keyfind();
		if (key1){
			val1+=(key1-48)*pow(10,2-power1);
			power1++;
			_delay_ms(60);
		}
		if (power1==3){
			if (val1>255){
				LCD_Clear();
				lcd_command(0x80);
				LCD_STRING("Invalid Value");
				lcd_command(0xC0);
				LCD_STRING("Enter a new val");
				_delay_ms(1000);
				LCD_Clear();
				lcd_command(0x80);
				LCD_STRING("Enter G value");
				val1=0;
				power1=0;
			}
			else{
				GREEN1=val1;
				break;
			}
		}
	}
	power1=0;
	val1=0;
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Done");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Enter B value");
	//same thing that we did for red
	while (1){
		char key1=keyfind();
		if (key1){
			val1+=(key1-48)*pow(10,2-power1);
			power1++;
			_delay_ms(60);
		}
		if (power1==3){
			if (val1>255){
				LCD_Clear();
				lcd_command(0x80);
				LCD_STRING("Invalid Value");
				lcd_command(0xC0);
				LCD_STRING("Enter a new val");
				_delay_ms(1000);
				LCD_Clear();
				lcd_command(0x80);
				LCD_STRING("Enter B value");
				val1=0;
				power1=0;
			}
			else{
				BLUE1=val1;
				break;
			}
		}
	}
	//write pwm values
	BLUE_light();
	GREEN_RED_light();
	RED_DUTY_CYCLE=RED1;
	GREEN_DUTY_CYCLE=GREEN1;
	BLUE_DUTY_CYCLE=BLUE1;
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Done");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Thanks for using");
	lcd_command(0xC0);
	LCD_STRING("the device");
	_delay_ms(5000);
	PORTD &=~(1<<3);
	PORTB &=~(1<<0);
	PORTC &=~(1<<4);
	LCD_Clear();
	LCD_print();
}
int main(void)
{	
	LCD_INIT();
	lcd_command(0x82);
	LCD_STRING("Hello world!");
	lcd_command(0xC4);
	LCD_STRING("Welcome!");
	_delay_ms(1000);
	LCD_print();
	while(1){
		char key=keyfind();
		if (key){
			if (key=='1'){
				CALIBRATION();
			}
			else if(key=='2'){
				SENSING_MODE();
			}
			else if(key=='3'){
				LIGHT_RGB_LED();
			}
			
		}
	}
}
//to control the overflow of pwm 
ISR(TIMER1_OVF_vect){
	OCR1B=GREEN_DUTY_CYCLE;
	OCR1A=RED_DUTY_CYCLE;
}
ISR(TIMER2_OVF_vect){
	OCR2A=BLUE_DUTY_CYCLE;
}
void LCD_print(void){
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Select the mode!");
	lcd_command(0xC0); //set the cursor at the starting position of the second line
	LCD_STRING("Select number");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("1:Calibration");
	lcd_command(0xC2);
	LCD_STRING("Mode");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("2:Sensing Mode");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("3:Light the RGB");
	lcd_command(0xC2); //set  the cursor at the 2nd row 3rd column
	LCD_STRING("LED");
}