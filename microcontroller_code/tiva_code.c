#include "TM4C123GH6PM.h"
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

void PortA_Init(void);
void PortB_Init(void);
void PortD_Init(void);
void PortE_Init(void);
void PortF_Init(void);
void SysTick_Init(void);
void Delay_us(int time);
void Delay_ms(int ms);  
void Systick_Wait(unsigned long delay);
void turningright_circular_200ms(void);
void manual_boob(char command);

void UART_init(void);
char readChar(void);
void printChar(char c);
void printString(char * string);
char* readString(char delimiter);

void distance_front(void);
float distance_side(void);
float distance_filter(float distance_buffer[], float current_distance, int *index);

void Forward(int speed, int turn);
void move_boobbot(float speed, float turn);
void stop(void);

float PID_controller(float current_distance, float params[], float errors[]);

int main(void){

    PortA_Init();  
		PortB_Init();
		PortD_Init();
		PortE_Init();
		PortF_Init();
		UART_init();
		SysTick_Init();
		float side;
		float side_filtered;
		float side_distances[3];
		int side_index = 0;
		float control_output;
		char mode;
		bool mode_flag = 1;
    float params[4] = {1.0, 0.0, 0.5, 20.0}; // Kp, Ki, Kd, required_distance
    float errors[2] = {0, 0}; // previous_error, sum_of_errors
		
		while(1){
			mode = readChar();
			if(mode == 'a') mode_flag = 1;
			else if(mode == 'm') mode_flag = 0; 
			
			//AUTOMATIC
			while(mode_flag){

				//printString("automatic \n");
				mode = readChar();
				if(mode=='m'){
					mode_flag = 0;
					break;
				}
				side = distance_side();
				side_filtered = distance_filter(side_distances, side, &side_index);
				Delay_ms(50);
				
				control_output = PID_controller(side, params, errors);
				while((GPIOA->DATA & 1<<6)==0){
					stop();
					turningright_circular_200ms();
				}
				move_boobbot(60,control_output);
				if (side_filtered < 15) GPIOF->DATA = 0x02;
				else if (side_filtered > 25) GPIOF->DATA = 0x04;
				else GPIOF->DATA = 0x8;		
			}
			
			
			//MANUAL
			while(!mode_flag){
				GPIOF->DATA=0x8;
				//printString("manual \n");
				char command = readChar();
				if(command=='a'){
					mode_flag = 1;
					break;
				}
				manual_boob(command);
			}
	}
}


void PortA_Init(void){    
	  //port A initialise for trigger output
		SYSCTL->RCGC2 |= 0x1;      
    GPIOA->DIR = 1<<7 | 0x0000003F;        /* make PB7 an output pin */
    GPIOA->DEN |= 0xFF;       /* make PB6 as digital pin */   
}

void PortB_Init(void){
	//port B initialise for trigger output
		SYSCTL->RCGC2 |= 0x2;      
    GPIOB->DIR = 1<<7;        /* make PB7 an output pin */
    GPIOB->DEN |= 0xFF;       /* make PB6 as digital pin */
}

void PortE_Init(void){
	SYSCTL->RCGC2|=0x00000010;
	GPIOE->LOCK = 0x4C4F434B;
	GPIOE->CR = 0xFF;
	GPIOE->AMSEL = 0x00;
	GPIOE->PCTL = 0x00000000;
	GPIOE->DIR = 0xFF;
	GPIOE->AFSEL = 0x00000000;
	GPIOE->PUR = 0x00;
	GPIOE->DEN = 0xFF;
}

void PortD_Init(void){
	SYSCTL->RCGC2|=0x00000008;
	GPIOD->LOCK = 0x4C4F434B;
	GPIOD->CR = 0xFF;
	GPIOD->AMSEL = 0x00;
	GPIOD->PCTL = 0x00000000;
	GPIOD->DIR = 0xFF;
	GPIOD->AFSEL = 0x00000000;
	GPIOD->PUR = 0x00;
	GPIOD->DEN = 0xFF;
}

void PortF_Init(void){
	//port F initialise
		SYSCTL->RCGC2|=0x20;
		GPIOF->LOCK=0x4C4F434B;
		GPIOF->CR=0x1F;
		GPIOF->DIR=0x0E;
		GPIOF->DEN=0xFF;
}

void SysTick_Init(void){
	//systick initialise
		SysTick->CTRL=0;
		SysTick->LOAD= 500000;		//DOWN COUNTING BY 500000 ASSUMING 50 MHZ CLOCK
		SysTick->VAL=0;
		SysTick->CTRL= 0X05; 	
}

void Systick_Wait (unsigned long delay){
	volatile unsigned long elapsedTime;      
	unsigned long startTime = SysTick->VAL;  
	do{
		elapsedTime = (startTime-SysTick->VAL);  
	}
	while (elapsedTime <= delay);    
}
void Delay_ms (int ms){  
    unsigned long i;
	for( i=0; i< ms; i++){    
		Systick_Wait(50000);      
	}
}

void Delay_us (int time){   
    unsigned long i;
	for( i=0; i < time; i++){    
		Systick_Wait(50);      
	}
}	
void distance_front(void){
		while((GPIOA->DATA & 1<<6)==0){
			turningright_circular_200ms();
		}
}


float distance_side(void){
		int previous_state = 0;
		int current_state = 0;
		unsigned long start_time = 0, end_time = 0, pulse_duration = 0;
    float distance_cm = 0.0;
		
		//trigger pulse 
		GPIOB->DATA &= ~(1<<7);
		Delay_us(10);
		GPIOB->DATA |= 1<<7;
		Delay_us(15);
		GPIOB->DATA &= ~(1<<7);

	
		while (1) {
			current_state = GPIOB->DATA & (1 << 6); // Read ECHO state
				
			if (current_state && !previous_state) { //posedge
				
					
				start_time= SysTick->VAL;
			}
			
			if (!current_state && previous_state) { //negedge

					

				end_time=SysTick->VAL;
				pulse_duration= (start_time-end_time) & 0xFFFFFF;
				pulse_duration= pulse_duration*0.02;
				distance_cm= (pulse_duration* 0.01715); 	//0.0343/2
				
			
					return distance_cm;
			}    
			previous_state = current_state; // Update the previous state
		}				
}
float distance_filter(float distance_buffer[], float current_distance, int *index) {
    // Add the new distance to the buffer
    distance_buffer[*index] = current_distance;
    
    // Update the index for the circular buffer
    *index = (*index + 1) % 3;

    // Calculate the average of the distances in the buffer
    float sum = 0.0;
    for (int i = 0; i < 3; i++) {
        sum += distance_buffer[i];
    }
    
    // Return the filtered distance (average of the buffer)
    return (sum / 3);
}


float PID_controller(float current_distance, float params[], float errors[]) {
    // Extract parameters from the input arrays
    float Kp = params[0];
    float Ki = params[1];
    float Kd = params[2];
    float required_distance = params[3];

    // Extract error values from the input errors array
    float error = current_distance - required_distance; // Calculate the current error
    float previous_error = errors[0]; // Previous error
    float sum_of_errors = errors[1]; // Sum of errors (integral term)

    // Update integral and derivative terms
    sum_of_errors += error; // Integral term
    float derivative = error - previous_error; // Derivative term

    // Calculate control output
    float control_output = Kp * error + Ki * sum_of_errors + Kd * derivative;

    // Update previous error for next loop
    errors[0] = error; // Update previous error
    errors[1] = sum_of_errors; // Update sum of errors

    return control_output; // Return the control output
}
void stop(void){
	GPIOA->DATA&=0x00000040;
	Delay_ms(10);
}
void move_boobbot(float speed, float turn){
	float modturn;
	int intspeed=speed;
		if(turn<=5 && turn>=-5){
			for(int i=0; i<30; i++){
			  GPIOA->DATA|=0x0000000A<<1;      //GPIOE 1234 pins for forward, 1,2 right, 3,4 left
				Delay_us(150*(speed));
			  GPIOA->DATA&=0x00000040;
				Delay_us(150*(100-speed));
			}
			printString("DIST 30.0 TURN 0");
		}
		else if(turn<-5){   //turning right
				if(turn<(-intspeed)){
					modturn=speed;
				}
				else{
					modturn = -turn;
				}
			//TURN RIGHT
			GPIOA->DATA|= 0x0000000C<<1; 
			Delay_ms(10*modturn);            
			GPIOA->DATA&= 0x00000040;		

			//Delay_ms(50);

			GPIOA->DATA|=0x0000000A<<1;
			Delay_ms(100);
			GPIOA->DATA&=0x00000040;
			printString("DIST 2.0 TURN 4");
		}

		else{       //turning left
				if(turn>(intspeed)){  
					modturn=speed;
				}
				else{
					modturn = turn;
				}
				//TURN LEFT
				GPIOA->DATA|= 0x24;
        Delay_ms(10*modturn);             
        GPIOA->DATA&= 0x00000040;

				//Delay_ms(50);

				GPIOA->DATA|=0x0000000A<<1;
				Delay_ms(100);
				GPIOA->DATA&=0x00000040;

				printString("DIST 2.0 TURN -4");
		}
	}
void turningright_circular_200ms(void){
	GPIOA->DATA|= 0x0000000C<<1;  // Adjust GPIO values for right turn
  Delay_ms(200);             // Adjust delay as needed
  GPIOA->DATA= 0x0;
	printString("DIST 0.0 TURN 15");
}

void UART_init(void){
	    // Enable the UART1 module and Port B for UART1 TX/RX pins
    SYSCTL->RCGCUART |= (1 << 1);  // Enable UART1 clock
    SYSCTL->RCGC2 |= (1 << 1);     // Enable clock for Port B (PB0, PB1)

    // Set alternate function and configure pins for UART (PB0 for RX, PB1 for TX)
    GPIOB->AFSEL = (1 << 1) | (1 << 0);  // Enable alternate functions on PB0, PB1
    GPIOB->PCTL = (1 << 0) | (1 << 4);   // Assign UART1 to PB0 and PB1
    GPIOB->DEN |= (1 << 0) | (1 << 1);    // Enable digital functionality for PB0, PB1

    // Disable UART1 for configuration
    UART1->CTL &= ~(1 << 0);  // Clear UARTEN bit to disable UART1

    // Set baud rate for 9600 baud (IBRD = 325, FBRD = 34)
    UART1->IBRD = 325;  // Set integer baud rate divisor
    UART1->FBRD = 34;   // Set fractional baud rate divisor

    // Configure UART for 8-bit data, 1 stop bit, no parity
    UART1->LCRH = (0x3 << 5) | (1 << 4);  // 8-bit, no parity, 1 stop bit

    // Set clock source for UART1 to system clock
    UART1->CC = 0x0;  // Use system clock for UART

    // Enable UART1 for transmission and reception
    UART1->CTL = (1 << 0) | (1 << 8) | (1 << 9);  // Enable UART1, TX, and RX
}
char readChar(void)  {
    // Wait until the UART receive FIFO is not empty
    if((UART1->FR & (1 << 4)) != 0) return '\0';
    else return UART1->DR;                   
}
void printChar(char c)  {
    // Wait until the UART transmit FIFO is not full
    while((UART1->FR & (1 << 5)) != 0);
    
    // Write the character to the UART data register
    UART1->DR = c;           
}
void printString(char * string){
    // Loop through each character in the string and send it
    while(*string)
    {
        printChar(*(string++));
    }
}
char* readString(char delimiter){
    int stringSize = 0;
    char* string = (char*)calloc(10, sizeof(char));  // Allocate initial memory for the string
    char c = readChar();  // Read the first character
    printChar(c);  // Echo the character

    // Loop until the delimiter character is received
    while(c != delimiter)
    {
        // Store the received character in the string
        *(string + stringSize) = c;
        stringSize++;

        // If string length exceeds the current allocated size, reallocate more memory
        if((stringSize % 10) == 0)
        {
            string = (char*)realloc(string, (stringSize + 10) * sizeof(char));
        }

        // Read and echo the next character
        c = readChar();
        printChar(c);
    }

    // Return NULL if no characters were received
    if(stringSize == 0)
    {
        return '\0';
    }
    return string;
}

void manual_boob(char command){
        switch (command) {
            case 'u':  // Move forward
                move_boobbot(70, 0);
                break;
            case 'd':  // Move backward
                GPIOA->DATA|= 0x28;  // Adjust GPIO values for backward movement
                Delay_ms(400);             // Adjust delay as needed
                GPIOA->DATA= 0x0;
								printString("DIST -30.0 TURN 0");
                break;
            case 'l':  // Turn left
                GPIOA->DATA|= 0x24;  // Adjust GPIO values for left turn
                Delay_ms(400);             // Adjust delay as needed
                GPIOA->DATA= 0x0;
								printString("DIST 0.0 TURN -30");
                break;
            case 'r':  // Turn right
                GPIOA->DATA|= 0x0000000C<<1;  // Adjust GPIO values for right turn
                Delay_ms(400);             // Adjust delay as needed
                GPIOA->DATA= 0x0;
								printString("DIST 0.0 TURN 30");
                break;
            default:
                stop();  // Stop if the command is not recognized
                break;
        }			
}

