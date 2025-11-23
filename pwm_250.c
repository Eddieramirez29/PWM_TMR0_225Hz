#include <xc.h>

#define _XTAL_FREQ 8000000
#define RESOLUTION_ADC 0.00488
#define REFERENCE_VOLTAGE 5
#define FOSC 8000000

#define PERIOD_TIME 0.004444  // 4.444 ms para 225 Hz
#define MAX_TIME_4   0.000512
#define MAX_TIME_8   0.001024
#define MAX_TIME_16  0.002048
#define MAX_TIME_32  0.004096
#define MAX_TIME_64  0.008192


// CONFIG1
#pragma config FOSC  = INTRC_NOCLKOUT
#pragma config WDTE  = OFF
#pragma config PWRTE = OFF
#pragma config MCLRE = OFF
#pragma config CP    = OFF
#pragma config CPD   = OFF
#pragma config BOREN = OFF
#pragma config IESO  = ON
#pragma config FCMEN = ON
#pragma config LVP   = OFF

// CONFIG2
#pragma config BOR4V = BOR40V
#pragma config WRT   = OFF

unsigned int ADC_value;
float Vin;
float t_h;
float t_l;
unsigned char TMR0_LOW_VALUE;
unsigned char TMR0_HIGH_VALUE;
unsigned char prescaler_selector, prescaler;
unsigned char turn_pin = 1;

void setRegisters()
{
    OSCCON = 0b01110000;
    while (OSCCONbits.HTS == 0) { }

    TRISAbits.TRISA0 = 1;
    ANSELbits.ANS0 = 1;

    ADCON0bits.ADCS1 = 1;
    ADCON0bits.ADCS0 = 0;

    ADCON0bits.CHS3 = 0;
    ADCON0bits.CHS2 = 0;
    ADCON0bits.CHS1 = 0;
    ADCON0bits.CHS0 = 0;

    TRISCbits.TRISC0 = 0;

    ADCON1bits.ADFM = 1;
    ADCON1bits.VCFG1 = 0;
    ADCON1bits.VCFG0 = 0;

    OPTION_REGbits.T0CS = 0;
    OPTION_REGbits.T0SE = 0;
    OPTION_REGbits.PSA = 0;

    INTCONbits.T0IE = 1;
}

void initRegisters()
{
    INTCONbits.GIE = 1;
    INTCONbits.INTF = 0;
    PORTCbits.RC0 = 0;

    TMR0 = 0x00;

    OPTION_REGbits.PS2 = 0;
    OPTION_REGbits.PS1 = 1;
    OPTION_REGbits.PS0 = 0;
}

void get_voltage_value()
{
    __delay_us(20);
    ADCON0bits.GO_DONE = 1;
    while (ADCON0bits.GO_DONE);
    ADC_value = (ADRESH << 8) | ADRESL;
    Vin = ADC_value * RESOLUTION_ADC;
}

void get_high_time()
{
    t_h = (Vin * PERIOD_TIME) / REFERENCE_VOLTAGE;
}

void get_low_time()
{
    t_l = PERIOD_TIME - t_h;
}

void calculate_time_delay_high_low()
{
    if (t_h >= 0 && t_h < MAX_TIME_4)
    {
        prescaler_selector = 1;
        prescaler = 4;
    }
    else if (t_h >= MAX_TIME_4 && t_h < MAX_TIME_8)
    {
        prescaler_selector = 2;
        prescaler = 8;
    }
    else if (t_h >= MAX_TIME_8 && t_h < MAX_TIME_16)
    {
        prescaler_selector = 3;
        prescaler = 16;
    }
    else if (t_h >= MAX_TIME_16 && t_h < MAX_TIME_32)
    {
        prescaler_selector = 4;
        prescaler = 32;
    }
    else if (t_h >= MAX_TIME_32 && t_h < MAX_TIME_64)
    {
        prescaler_selector = 5;
        prescaler = 64;
    }

    switch (prescaler_selector)
    {
        case 1:
        {
            OPTION_REGbits.PS2 = 0;
            OPTION_REGbits.PS1 = 1;
            OPTION_REGbits.PS0 = 0;
        }
        break;

        case 2:
        {
            OPTION_REGbits.PS2 = 0;
            OPTION_REGbits.PS1 = 1;
            OPTION_REGbits.PS0 = 1;
        }
        break;

        case 3:
        {
            OPTION_REGbits.PS2 = 1;
            OPTION_REGbits.PS1 = 0;
            OPTION_REGbits.PS0 = 0;
        }
        break;

        case 4:
        {
            OPTION_REGbits.PS2 = 1;
            OPTION_REGbits.PS1 = 0;
            OPTION_REGbits.PS0 = 1;
        }
        break;

        case 5:
        {
            OPTION_REGbits.PS2 = 1;
            OPTION_REGbits.PS1 = 1;
            OPTION_REGbits.PS0 = 0;
        }
        break;
    }

    TMR0 = 0x00;
    TMR0_HIGH_VALUE = 256 - (unsigned char)(t_h / ((4.0 * prescaler) / FOSC));
    turn_pin = 0;
    TMR0_LOW_VALUE = 256 - (unsigned char)(t_l / ((4.0 * prescaler) / FOSC));
}

void __interrupt() ISR()
{
    if (INTCONbits.T0IF)
    {
        if (turn_pin == 0)
        {
            TMR0 = TMR0_LOW_VALUE;
            PORTCbits.RC0 = 0;
            turn_pin = 1;
        }
        else
        {
            TMR0 = TMR0_HIGH_VALUE;
            PORTCbits.RC0 = 1;
            turn_pin = 0;
        }

        INTCONbits.T0IF = 0;
    }
}

void main()
{
    setRegisters();
    initRegisters();
    ADCON0bits.ADON = 1;

    while (1)
    {
        get_voltage_value();
        get_high_time();
        get_low_time();
        calculate_time_delay_high_low();
    }
}