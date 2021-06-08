/*
 * Code.c
 *
 * Created: 5/4/2021 9:23:59 AM
 * Author : Acer
 */ 
#define F_CPU  8000000UL															// Initializing CPU clock frequency

#include <avr/io.h>																	// Standard AVR header
#include <util/delay.h>																// Delay Header
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>															// Interrupt header


#define LCD_PRT PORTC																// LCD DATA PORT
#define LCD_DDR DDRC																// LCD DATA DDR
#define LCD_PIN PINC																// LCD DATA PIN
#define LCD_EN PORTC1																// LCD EN
#define LCD_RS PORTC0																// LCD RS
#define	KEY_PRT	PORTD																// keyboard	PORT
#define	KEY_DDR	DDRD																// keyboard	DDR
#define	KEY_PIN	PIND																// keyboard	PIN
#define RED_PORT PORTB4																// Red port bit for sensing RGB LED
#define GREEN_PORT PORTB5															// Green port bit for sensing RGB LED
#define BLUE_PORT PORTB0															// Blue port bit for sensing RGB LED

unsigned char keypad[4][3] = {{'1','2','3'},										// Declaring variables
							  {'4','5','6'},
							  {'7','8','9'},
							  {'*','0','#'}};
								  
char colors[3][3] = {"R ","G ","B "};
char keys[4];
int colorValues[3] ;
int numberOfDigits = 0;
int calibratedValues[4][3] ;
char calibratingColors [4][7] = {"Red ","Green ","Blue ","White "};
char mode_2 ;
int ADCvalue ;
int sensorSum ;
char snum[5] ;
int redHigh ;
int redLow ;
int greenHigh ;
int greenLow ;
int blueHigh ;
int blueLow ;
int isCalibrated ;
int sensorValues[3] ;
int sensorValues_2[3];

void calibrationCalculation(){														// Function to calculate maximum and minimum values possible for Red, Green, Blue analog input values
	redHigh = (calibratedValues[0][0] + calibratedValues[3][0])/2;
	redLow = (calibratedValues[1][0] + calibratedValues[2][0])/2;
	greenHigh = (calibratedValues[1][1] + calibratedValues[3][1])/2;
	greenLow = (calibratedValues[0][1] + calibratedValues[2][1])/2;
	blueHigh = (calibratedValues[2][2] + calibratedValues[3][2])/2;
	blueLow = (calibratedValues[0][2] + calibratedValues[1][2])/2;
}

void ADCStartConversion() {														    // Function to start ADC conversion in single conversion mode
	ADCSRA |= (1 << ADSC);
}

void ADCInit(){																		// Function to initialize registers for ADC
	ADMUX = (1 << REFS0 ) | (1 << MUX2) | (1 << MUX1);
	ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS0) | (1 << ADPS1) | (1 << ADPS2) ;
	
	sei();												                            // Enabling external interrupts
	
	ADCStartConversion();
}

void ADCStop(){                                                                     // Function to stop ADC
	ADMUX &= ~(1 << REFS0 ) & ~(1 << MUX2) & ~(1 << MUX1);
	ADCSRA &= ~(1 << ADEN) & ~(1 << ADIE) & ~(1 << ADPS0) & ~(1 << ADPS1) & ~(1 << ADPS2) ;
	
	cli ();																			// Disabling external interrupts
}

void sensorCalculation(){															// Calculating R,G,B values for surfaces
	if (sensorValues[0] > redLow){
		sensorValues_2[0] = ((sensorValues[0] - redLow)/(redHigh - redLow))*255 ;
	}
	else{
		sensorValues_2[0] = 0 ;
	}
	if (sensorValues[1] > greenLow){
		sensorValues_2[1] = ((sensorValues[1] - greenLow)/(greenHigh - greenLow))*255 ;
	}
	else{
		sensorValues_2[1] = 0 ;
	}
	if (sensorValues[2] > blueLow){
		sensorValues_2[2] = ((sensorValues[2] - blueLow)/(blueHigh - blueLow))*255 ;
	}
	else{
		sensorValues_2[2] = 0 ;
	}
	for (int i = 0 ; i < 3 ; i++){													// Displaying R,G,B values
		lcdInst(0x01,0);										
		_delay_us(2000);
		lcdPrint(colors[i]);
		lcdPrint("- ");
		lcdPrint(itoa(sensorValues_2[i],snum,10));
		_delay_ms(1000);
	}
}

void pwmInit(){																	    // Function to initialize registers for PWM
	DDRB = (1<< PORTB3) | (1<< PORTB2) | (1<< PORTB1) ;
	
	TCCR2A = (1 << COM2A1) |(1 << WGM21 ) | (1 << WGM20 )  ;
	TCCR1A = (1 << COM1A1) |(1 << COM1B1 ) | (1 << WGM10 )  ;
	TIMSK2 = (1 << TOIE2);
	TIMSK1 = (1 << TOIE1); 
	
	sei() ;																			// Enabling external interrupts
	
	TCCR2B = (1 << CS22) | (1 << CS20) ;
	TCCR1B = (1 << CS10 )| (1 << WGM12 ) | (1<<CS12) ;
	
}

void pwmStop(){																		// Stopping the PWM 
	TCCR2A &= ~(1 << COM2A1) & ~(1 << WGM21 ) & ~(1 << WGM20 )  ;
	TCCR1A &= ~(1 << COM1A1) & ~(1 << COM1B1 ) & ~(1 << WGM10 )  ;
	TCCR2B = 0;
	TCCR1B = 0 ;
	
	cli();																			// Disabling external interrupts
	
}	

char getKey(){                                                                      // Function to get input from keyboard
	
	unsigned char colloc, rowloc;
	

	KEY_DDR = 0xF0;	
	KEY_PRT = 0xFF;
	while (1)
	{
		do																			// Checking whether previously pressed keys are released
		{
			KEY_PRT &= 0x0F;
			colloc = (KEY_PIN & 0x0E); 
		} while (colloc != 0x0E);
		
		do                                                                          // Checking whether a key is pressed
		{	
			do 
			{
				_delay_ms(20);
				colloc = (KEY_PIN & 0x0E);
			}	while (colloc == 0x0E);
			_delay_ms(20);
			colloc = (KEY_PIN & 0x0E);
		}	while (colloc == 0x0E);
		
		while(1)
		{
			KEY_PRT = 0xEF;
			colloc = (KEY_PIN & 0x0E); 
			if (colloc != 0x0E)
			{
				rowloc = 0 ;
				break ;
			}
			
			KEY_PRT = 0xDF ;
			colloc = (KEY_PIN & 0x0E);
			
			if (colloc != 0x0E)
			{
				rowloc = 1 ;
				break ;
			}
			
			KEY_PRT = 0xBF ;
			colloc = (KEY_PIN & 0x0E);
			
			if (colloc != 0x0E)
			{
				rowloc = 2 ;
				break ;
			}
			
			KEY_PRT = 0x7F ;
			colloc = (KEY_PIN & 0x0E);
			rowloc = 3;
			break;
		}
		
		if (colloc == 0x0C)
		{
			return (keypad[rowloc][0]);
		}
		else if (colloc == 0x0A)
		{
			return (keypad[rowloc][1]);
		}
		else 
		{
			return (keypad[rowloc][2]);
		}
	}

};

void lcdInst( unsigned char dorc, unsigned int mode) {   // Function for LCD Commands
	LCD_PRT = (LCD_PRT & 0x03) | ((dorc>>2) & 0x3C) ;            // Setting the data or command's upper nibble to the data port
	if (mode == 0) {
		LCD_PRT &= ~(1<<LCD_RS);                            // RS = 0 for command
	}
	else{
		LCD_PRT |= (1<<LCD_RS);								// RS = 1 for data
	}
	
	LCD_PRT |= (1<<LCD_EN);									// EN = 1 for H-to-L
	_delay_us(1) ;											// wait  to make EN wider
	LCD_PRT &= ~(1<<LCD_EN);								// EN = 0 for H-to-L
	
	if (mode == 0) {
		_delay_us(100);										// wait
	}
	
	
	LCD_PRT = (LCD_PRT & 0x03) | (dorc << 2) ;				// Setting the data or command's lower nibble to the data port
	LCD_PRT |= (1<<LCD_EN);									// EN = 1 for H-to-L
	_delay_us(1) ;											// wait  to make EN wider
	LCD_PRT &= ~(1<<LCD_EN);								// EN = 0 for H-to-L
}

void lcdGoToXY(unsigned int x,unsigned int y)
{
	unsigned char firstCharAdr[] = {0x80,0xC0};             // 
	lcdInst(firstCharAdr[y-1] + x - 1,0);
	_delay_us(100);
}

void lcdInit(){
	LCD_DDR = 0xFF;											// Setting PortB as output
	LCD_PRT &= ~(1<<LCD_EN)	;								// Setting LCD_EN = 0
	_delay_us(2000) ;										// wait for stable power
	lcdInst(0x33,0);                                        // First command to initialize 4-bit mode
	_delay_us(100);											
	lcdInst(0x32,0);                                        // Second command to initialize 4-bit mode
	_delay_us(100);											
	lcdInst(0x28,0);                                        // Third command to initialize 4-bit mode
	_delay_us(100);											
	lcdInst(0x0C,0); 										// display on, cursor off
	_delay_us(100);											
	lcdInst(0x01,0);										// Clear LCD
	_delay_us(2000);										
	lcdInst(0x06,0);										// Shift cursor right
	_delay_us(100);                                         
											
}
 void lcdPrint(char *str){                                  // Function to print given string
	 
	 unsigned char i = 0;
	 
	 while(str[i] != 0)
	 {
		 lcdInst(str[i],1);
		 i ++;
		 _delay_us(100);
	 }
 }
 
 void lightLED(){
	 lcdGoToXY(1,1);
	 lcdPrint("Enter the R,G,B");
	 lcdGoToXY(6,2);
	 lcdPrint("values");
	 _delay_ms(1000);
	 lcdInst(0x01,0);
	 _delay_us(2000);
	 lcdGoToXY(1,1);
	 lcdPrint("Press * to type");
	 lcdGoToXY(4,2);
	 lcdPrint("next value");
	 _delay_ms(1000);
	 for(int i = 0;i <= 2; i++){
		 lcdInst(0x01,0);
		 _delay_us(2000);
		 lcdGoToXY(1,1);
		 lcdPrint(colors[i]);
		 for(int j = 0; j <= 3 ; j++){
			 keys[j] = getKey();
			 if (keys[j] != '*'){
				 lcdInst(keys[j],1);
			 }
			 else{
				 numberOfDigits = j;
				 _delay_us(2000);
				 lcdInst(0x01,0);
				 _delay_us(2000);
				 break;
			 }
		 }
		 lcdInst(0x01,0);
		 _delay_us(2000);
		 char number[] = "";
		 for(int k = 0; k <= numberOfDigits - 1 ; k++){
			 strncat(number ,&keys[k] , 1) ;
		 }
		 colorValues[i] = atoi(number);
	 }
	 pwmInit();
	 _delay_ms(4000);
	 colorValues[0] = 0;
	 colorValues[1] = 0;
	 colorValues[2] = 0;
	 pwmStop();
	 
	 
 }
 
 void calibration(){
	 lcdGoToXY(3,1);
	 lcdPrint("Calibrate for");
	 lcdGoToXY(2,2);
	 lcdPrint("Red,Green,Blue");
	 _delay_ms(1000);
	 lcdInst(0x01,0);
	 _delay_us(2000);
	 lcdGoToXY(5,1);
	 lcdPrint("and White");
	 lcdGoToXY(3,2);
	 lcdPrint("consecutively");
	 _delay_ms(1000);
	 lcdInst(0x01,0);
	 _delay_us(2000);
	 DDRB |= (1<< DDB5) | (1<< DDB4) | (1<< DDB0);
	 for (int i = 0; i < 4; i ++){
		 lcdGoToXY(3,1);
		 lcdPrint("Calibrate for");
		 lcdGoToXY(5,2);
		 lcdPrint(calibratingColors[i]);
		 _delay_ms(1000);
		 lcdInst(0x01,0);
		 _delay_us(2000);
		 lcdGoToXY(1,1);
		 lcdPrint("Press * to start");
		 _delay_ms(1000);
		 lcdInst(0x01,0);
		 _delay_us(2000);
		 if ('*' == getKey()){
			 lcdPrint("Calibrating...");
			 ADCInit();
			 PORTB |= (1 << RED_PORT );
			 _delay_ms(100);
			 sensorSum = 0 ;
			 for (int k = 0 ; k < 20 ; k ++){
				 sensorSum += ADCvalue ;
				 _delay_ms(10);
			 }
			 calibratedValues[i][0] = sensorSum / 20 ;
			 PORTB &= ~(1 << RED_PORT );
			 PORTB |= (1 << GREEN_PORT );
			 _delay_ms(100);
			 sensorSum = 0 ;
			 for (int k = 0 ; k < 20 ; k ++){
				 sensorSum += ADCvalue ;
				 _delay_ms(10);
			 }
			 calibratedValues[i][1] = sensorSum / 20 ;
			 PORTB &= ~(1 << GREEN_PORT );
			 PORTB |= (1 << BLUE_PORT );
			 _delay_ms(100);
			 sensorSum = 0 ;
			 for (int k = 0 ; k < 20 ; k ++){
				 sensorSum += ADCvalue ;
				 _delay_ms(10);
			 }
			 calibratedValues[i][2] = sensorSum / 20 ;
			 PORTB &= ~(1 << BLUE_PORT );
			 lcdInst(0x01,0);
			 _delay_us(2000);
			 lcdGoToXY(1,1);
			 lcdPrint("Calibrated for");
			 lcdGoToXY(4,2);
			 lcdPrint(calibratingColors[i]);
			 _delay_ms(1000);
			 lcdInst(0x01,0);
			 _delay_us(2000);
		 }
	 }
	 lcdGoToXY(3,1);
	 lcdPrint("Successfully ");
	 lcdGoToXY(4,2);
	 lcdPrint("Calibrated ");
	 _delay_ms(1000);
	 lcdInst(0x01,0);
	 _delay_us(2000);
	 calibrationCalculation();
	 ADCStop();
	 
 }
 
void sensingMode()	{
	sensorValues[0] = 0;
	sensorValues[1] = 0;
	sensorValues[2] = 0;
	lcdInst(0x01,0);
	_delay_us(2000);
	lcdGoToXY(1,1);
	lcdPrint("Press * to start");
	_delay_ms(1000);
	lcdInst(0x01,0);
	_delay_us(2000);
	if ('*' == getKey()){
		DDRB |= (1<< DDB5) | (1<< DDB4) | (1<< DDB0);
		ADCStartConversion();
		lcdGoToXY(6,1);
		lcdPrint("Sensing");
		PORTB |= (1 << RED_PORT );
		_delay_ms(100);
		for (int k = 0 ; k < 20 ; k ++){
			sensorValues[0] += ADCvalue ;
			_delay_ms(10);
		}
		PORTB &= ~(1 << RED_PORT );
		PORTB |= (1 << GREEN_PORT );
		_delay_ms(100);
		for (int k = 0 ; k < 20 ; k ++){
			sensorValues[1] += ADCvalue ;
			_delay_ms(10);
		}
		PORTB &= ~(1 << GREEN_PORT );
		PORTB |= (1 << BLUE_PORT );
		_delay_ms(100);
		for (int k = 0 ; k < 20 ; k ++){
			sensorValues[2] += ADCvalue ;
			_delay_ms(10);
		}
		PORTB &= ~(1 << BLUE_PORT );
		lcdInst(0x01,0);
		_delay_us(2000);
		sensorValues[0] /= 20 ;
		sensorValues[1] /= 20 ;
		sensorValues[2] /= 20 ;
		sensorCalculation();
	}
 }

int main(void)
{
	
    lcdInit();
	lcdGoToXY(17,1);
	_delay_us(100);
	lcdPrint("WELCOME");
	for (int i = 1;i < 12;i++)
	{
		lcdInst(0x18,0);
		_delay_ms(200);
	}
	_delay_ms(1500);
	for (int i = 1;i < 13;i++)
	{
		lcdInst(0x18,0);
		_delay_ms(200);
	}
    while (1) 
    {
		lcdInst(0x01,0);
		_delay_us(2000);
		lcdGoToXY(1,1);
		lcdPrint("Press 1 to light");
		lcdGoToXY(3,2);
		lcdPrint("the RGB LED");
		_delay_ms(1000);
		lcdInst(0x01,0);
		_delay_us(2000);
		lcdGoToXY(1,1);
		lcdPrint("Press 2 to enter");
		lcdGoToXY(1,2);
		lcdPrint("calibration mode");
		_delay_ms(1000);
		lcdInst(0x01,0);
		_delay_us(2000);
		lcdGoToXY(1,1);
		lcdPrint("Press 3 to enter");
		lcdGoToXY(1,2);
		lcdPrint("the sensing mode");
		_delay_ms(1000);
		lcdInst(0x01,0);
		_delay_us(2000);
		char mode = getKey();
		if (mode == '1')
		{
			lightLED();
		}
		else if (mode == '2')
		{
			calibration();
		}
		else if (mode == '3')
		{
			sensingMode();
		}
		lcdInst(0x01,0);
		_delay_us(2000);
		lcdGoToXY(1,1);
		lcdPrint("Press # to reuse");
		lcdGoToXY(4,2);
		lcdPrint("the device");
		_delay_ms(1000);
		lcdInst(0x01,0);
		_delay_us(2000);
		lcdGoToXY(1,1);
		lcdPrint("Press * to exit");
		_delay_ms(1000);
		lcdInst(0x01,0);
		_delay_us(2000);
		mode_2 = getKey();
		if ('*' == mode_2){
			lcdGoToXY(4,1);
			lcdPrint("Thank You");
			_delay_ms(1000);
			lcdInst(0x01,0);
			_delay_us(2000);
			break;
		}
		else if ('#' == mode_2){
			continue;
		}
		
    }
	return 0 ;
}

ISR(TIMER2_OVF_vect){                                  // External Interrupts for lighting
	OCR2A = colorValues[2];
}

ISR(TIMER1_OVF_vect){
 	OCR1A = colorValues[0];
 	OCR1B = colorValues[1];
}

ISR(ADC_vect){
	ADCvalue = ADC;
}

