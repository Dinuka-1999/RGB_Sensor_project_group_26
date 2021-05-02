#ifndef F_CPU
#define F_CPU 16000000UL  //set the processor speed to 16 MHz
#endif
#include <avr/io.h>
#include <util/delay.h>  
#define LCD_PORT PORTC //define the connected data port
#define rs PB4
#define en PB5

long  int RED1,RED_HIGH=0,RED_LOW=0;
long  int GREEN1,GREEN_HIGH=0,GREEN_LOW=0;
long  int BLUE1,BLUE_HIGH=0,BLUE_LOW=0;

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
void LCD_print(void){
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Select the mode!");
	lcd_command(0xC0); //set the cursor at the starting position of the second line
	LCD_STRING("Select number");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("1:Calibration Mode");
	lcd_command(0xC0);
	LCD_STRING("2:Sensing Mode");
	for (int r=1;r<4;r++){
		_delay_ms(1000);
		lcd_command(0x18); //shift the code to left
	}
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("3:Light the RGB");
	lcd_command(0xC2); //set  the cursor at the 2nd row 3rd column
	LCD_STRING("LED");
	_delay_ms(1000);
}
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
	DDRC &=~(1<<5);			/* Make ADC 5 port as input */
	ADCSRA = 0x87;			/* Enable ADC, fr/128  */
	ADMUX = 0x45;			/* Vref: Avcc, ADC channel: 5 */
	
}

int ADC_Read(char channel)
{
	int Ain,AinLow;
	
	ADMUX=ADMUX|(channel & 0x0f);	/* Set input channel to read */

	ADCSRA |= (1<<ADSC);		/* Start conversion */
	while((ADCSRA&(1<<ADIF))==0);	/* Monitor end of conversion interrupt */
	
	_delay_us(10);
	AinLow = (int)ADCL;		/* Read lower byte*/
	Ain = (int)ADCH*256;		/* Read higher 2 bits and 
					Multiply with weight */
	Ain = Ain + AinLow;				
	return(Ain);			/* Return digital value*/
}

/*function for the calibration mode*/
void CALIBRATION(void){
	ADC_Init();
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Calibrating");
	lcd_command(0xC0);
	LCD_STRING("Red Surface");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Press # to start");
	while (true){
		char key2=keyfind();
		if (key2=='#'){
			DDRB|=(1<<0);
			DDRC|=(1<<4);
			LCD_Clear();
			lcd_command(0x80);
			LCD_STRING("Calibrating");
			PORTD |=(1<<3);
			PORTB &=~(1<<0);
			PORTC &=~(1<<4);
			for (int r=1;r<=10;r++){
				RED_HIGH+=ADC_Read('5');
				_delay_ms(100);
			}
			PORTD &=~(1<<3);
			PORTB |=(1<<0);
			PORTC &=~(1<<4);
			for (int r=1;r<=10;r++){
				GREEN_LOW+=ADC_Read('5');
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
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Calibrating");
	lcd_command(0xC0);
	LCD_STRING("GREEN surface");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Press # to start");
	while (true){
		char key3=keyfind();
		if (key3=='#'){
			DDRB|=(1<<0);
			DDRC|=(1<<4);
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
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Calibrating");
	lcd_command(0xC0);
	LCD_STRING("BLUE surface");
	_delay_ms(1000);
	LCD_Clear();
	lcd_command(0x80);
	LCD_STRING("Press # to start");
	while (true){
		char key4=keyfind();
		if (key4=='#'){
			DDRB|=(1<<0);
			DDRC|=(1<<4);
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
	_delay_ms(1000);
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
	while(true){
		char key=keyfind();
		if (key){
			if (key=='1'){
				CALIBRATION();
			}
		}
	}
}


