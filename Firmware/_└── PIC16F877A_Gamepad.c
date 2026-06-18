```c
/******************************************************************************
 * Project Name : Gamepad Controller System
 *
 * Course       : Electrical and Electronics Engineering (EEE)
 *
 * Students:
 *   Mustafa Haki Karaca
 *   Student ID : 61230001
 *   Department : EEE
 *
 *   Kerim Güler
 *   Student ID : 61230005
 *   Department : EEE
 *
 * Description:
 * This project reads joystick, trigger, button, and MPU6050 sensor data,
 * sends the data through UART, and controls a vibration motor.
 ******************************************************************************/

#include <xc.h>
#include <stdio.h>
#include <stdint.h>

#define _XTAL_FREQ 20000000

#pragma config FOSC = HS
#pragma config WDTE = OFF
#pragma config PWRTE = ON
#pragma config BOREN = ON
#pragma config LVP = OFF
#pragma config CPD = OFF
#pragma config WRT = OFF
#pragma config CP = OFF

#define MOTOR_PIN PORTDbits.RD3  // Vibration motor output pin

// Send a single character over UART
void UART_Write(char data) {
    while(!TRMT);
    TXREG = data;
}

// Send a string over UART
void UART_Write_String(const char *text) {
    for(int i = 0; text[i] != '\0'; i++) {
        UART_Write(text[i]);
    }
}

// Read data from the selected ADC channel
uint16_t ADC_Read(uint8_t channel) {
    ADCON0 &= 0xC5;
    ADCON0 |= (channel << 3);
    __delay_ms(2);
    GO_nDONE = 1;
    while(GO_nDONE);
    return ((ADRESH << 8) + ADRESL);
}

// Initialize I2C module
void I2C_Init(const unsigned long clock) {
    SSPCON = 0x28;
    SSPCON2 = 0x00;
    SSPADD = (_XTAL_FREQ / (4 * clock)) - 1;
    SSPSTAT = 0x00;
    TRISCbits.TRISC3 = 1;
    TRISCbits.TRISC4 = 1;
}

// Wait until I2C bus is free
void I2C_Wait() {
    uint16_t timeout = 0;
    while (((SSPCON2 & 0x1F) || (SSPSTAT & 0x04)) && (timeout < 1000)) {
        timeout++;
        __delay_us(1);
    }
}

// Start I2C communication
void I2C_Start() {
    I2C_Wait();
    SEN = 1;
}

// Generate repeated start
void I2C_RepeatedStart() {
    I2C_Wait();
    RSEN = 1;
}

// Stop I2C communication
void I2C_Stop() {
    I2C_Wait();
    PEN = 1;
}

// Send one byte over I2C
void I2C_Write(uint8_t data) {
    I2C_Wait();
    SSPBUF = data;
}

// Read one byte from I2C
uint8_t I2C_Read(uint8_t ack) {
    uint8_t temp;
    I2C_Wait();
    RCEN = 1;
    I2C_Wait();
    temp = SSPBUF;
    I2C_Wait();
    ACKDT = (ack) ? 0 : 1;
    ACKEN = 1;
    return temp;
}

// Initialize MPU6050 sensor
void MPU6050_Init() {
    I2C_Start();
    I2C_Write(0xD0);
    I2C_Write(0x6B);
    I2C_Write(0x00);
    I2C_Stop();
}

void main(void) {

    // Configure I/O directions
    TRISA = 0x0F;
    TRISE = 0x03;
    TRISB = 0x3F;
    TRISD = 0xF3;

    // Turn motor off at startup
    MOTOR_PIN = 0;

    // Configure ADC
    ADCON1 = 0x80;
    ADCON0 = 0x41;

    // Configure UART
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC7 = 1;
    SPBRG = 129;
    TXSTA = 0x24;
    RCSTA = 0x90;

    // Initialize I2C and MPU6050
    I2C_Init(10000);
    __delay_ms(100);
    MPU6050_Init();

    char buffer[40];
    uint16_t j1_x, j1_y, j2_x, j2_y, l2_trig, r2_trig;
    int16_t ax, ay, az;

    while(1) {

        // Read joystick and trigger values
        j1_x    = ADC_Read(2);
        j1_y    = ADC_Read(3);
        j2_x    = ADC_Read(0);
        j2_y    = ADC_Read(1);
        l2_trig = ADC_Read(6);
        r2_trig = ADC_Read(5);

        // Read accelerometer data from MPU6050
        I2C_Start();
        I2C_Write(0xD0);
        I2C_Write(0x3B);
        I2C_RepeatedStart();
        I2C_Write(0xD1);
        ax = (I2C_Read(1) << 8) | I2C_Read(1);
        ay = (I2C_Read(1) << 8) | I2C_Read(1);
        az = (I2C_Read(1) << 8) | I2C_Read(0);
        I2C_Stop();

        // Send joystick values
        sprintf(buffer, "J1:%u,%u J2:%u,%u ", j1_x, j1_y, j2_x, j2_y);
        UART_Write_String(buffer);

        // Send trigger values
        sprintf(buffer, "L2:%u R2:%u ", l2_trig, r2_trig);
        UART_Write_String(buffer);

        // Send accelerometer values
        sprintf(buffer, "A:%d,%d,%d T:", ax, ay, az);
        UART_Write_String(buffer);

        // Check pressed buttons
        if(PORTBbits.RB0 == 0) UART_Write_String("Sag ");
        if(PORTBbits.RB1 == 0) UART_Write_String("RB ");
        if(PORTBbits.RB2 == 0) UART_Write_String("X ");
        if(PORTBbits.RB3 == 0) UART_Write_String("B ");
        if(PORTBbits.RB4 == 0) UART_Write_String("A ");
        if(PORTBbits.RB5 == 0) UART_Write_String("Y ");
        if(PORTDbits.RD4 == 0) UART_Write_String("Sol ");
        if(PORTDbits.RD5 == 0) UART_Write_String("Yukari ");
        if(PORTDbits.RD6 == 0) UART_Write_String("Asagi ");
        if(PORTDbits.RD7 == 0) UART_Write_String("LB ");

        // End current message
        UART_Write_String("\r\n");

        // Receive motor control command
        if(RCIF) {
            char rx_data = RCREG;
            if(rx_data == 'M') MOTOR_PIN = 1;
            else if (rx_data == 'S') MOTOR_PIN = 0;
        }

        // Small update delay
        __delay_ms(50);
    }
}
```
