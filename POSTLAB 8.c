/* 
 * File:   POSTLAB 8.c
 * Author: diego
 *
 * Created on 22 de abril de 2022, 09:36 PM
 */
// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT        // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF                   // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF                  // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF                  // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF                     // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF                    // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF                  // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF                   // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF                  // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF                    // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V               // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF                    // Flash Program Memory Self Write Enable bits (Write protection off)


#include <xc.h>
#include <stdint.h>

/*------------------------------------------------------------------------------
 * CONSTANTES 
 ------------------------------------------------------------------------------*/
#define _XTAL_FREQ 1000000

/*------------------------------------------------------------------------------
 * VARIABLES 
 ------------------------------------------------------------------------------*/
int voltaje[3] = {0, 0, 0}, cdu;          // arreglo para unidad,decena,centena
uint8_t  PORTF = 0;                           // puerto extra para contar los valores de potenciometro
uint8_t bandera = 0;           // Variable para cambio en 7seg multiplexados

/*------------------------------------------------------------------------------
 * PROTOTIPO DE FUNCIONES 
 ------------------------------------------------------------------------------*/
void setup(void);                           
void valores(int a, int b[]);   
void multiplexado(int a[]);  
uint8_t tabla(int a);  
/*------------------------------------------------------------------------------
 * INTERRUPCIONES 
 ------------------------------------------------------------------------------*/
void __interrupt() isr (void){
    if(PIR1bits.ADIF){                      // Verificación de interrupción del módulo ADC
        if(ADCON0bits.CHS == 0){            // Verificamos sea AN0 el canal seleccionado
            PORTB = ADRESH;                 // Mostrar el resgitro ADRESH en PORTC
        }
        else if (ADCON0bits.CHS == 1){      // Verificamos sea AN1 el canal seleccionado
            PORTF = ADRESH;                  
        }
        PIR1bits.ADIF = 0;                  // Limpieza de bandera 
    } 
    else if(INTCONbits.T0IF){               // Verificación de interrupción del TIMER0 
        multiplexado(voltaje);                   // ver displays
        bandera++;                          // Incremento de bandera
        if(bandera>2){                      // Reinicio de bandera
            bandera = 0;
        }        
        INTCONbits.T0IF = 0;                // Limpieza de bandera 
        TMR0 = 252;                         // reinicio tmr0 2ms
    }
    return;
}

/*------------------------------------------------------------------------------
 * CICLO PRINCIPAL
 ------------------------------------------------------------------------------*/
void main (void){
    setup();    
    while(1){    
        
        if(ADCON0bits.GO == 0){             // No hay proceso de conversion
            // *En caso de estar usando mas de un canal analógico
           
            if(ADCON0bits.CHS == 0b0000)    
                ADCON0bits.CHS = 0b0001;    // Cambio de canal
            else if(ADCON0bits.CHS == 0b0001)
                ADCON0bits.CHS = 0b0000;    // Cambio de canal
            __delay_us(40);                 // Tiempo de adquisición
            
            ADCON0bits.GO = 1;              // Iniciamos proceso de conversión
        } 
        cdu = (PORTF)*(1.96);          // contador de 0 a 500 debido a 8bits
        valores(cdu,voltaje);             // Llamado de función de separación de dígitos del número
    }
    return;
}

/*------------------------------------------------------------------------------
 * CONFIGURACION 
 ------------------------------------------------------------------------------*/
    void setup(void){
    ANSEL = 0b00000011; // AN0 como entrada analógica
    ANSELH = 0;         // I/O digitales)
    
    //ANSEL = 0b00000111; // AN0, AN1 y AN2 como entrada analógica
    
    TRISA = 0b00000011; // AN0 como entrada
    //TRISA = 0b00000111; // AN0, AN1 y AN2 como entrada
    PORTA = 0; 
    
    TRISC = 0;
    PORTC = 0;
    TRISD = 0;
    PORTD = 0;
    TRISB = 0;                   // Habilitación de PORTB como salida
    PORTB = 0;                   // Limpieza del PORTB
   
   // Configuración reloj interno
    OSCCONbits.IRCF = 0b0100;   // 1MHz
    OSCCONbits.SCS = 1;         // Oscilador interno
    
   // Configuración de TIMER0
    OPTION_REGbits.T0CS = 0;        // temporizador
    OPTION_REGbits.PSA = 0;         // TMR0
    OPTION_REGbits.PS2 = 1;
    OPTION_REGbits.PS1 = 1;
    OPTION_REGbits.PS0 = 0;         //prescales 128
    TMR0 = 252; //2ms

   // Configuración ADC
    ADCON0bits.ADCS = 0b01;     // Fosc/8
    ADCON1bits.VCFG0 = 0;       // VDD
    ADCON1bits.VCFG1 = 0;       // VSS
    ADCON0bits.CHS = 0b0000;    // Seleccionamos el AN0
    ADCON0bits.CHS = 0b0001;    // Seleccionamos el AN1
    ADCON1bits.ADFM = 0;        // Justificado a la izquierda
    ADCON0bits.ADON = 1;        // Habilitamos modulo ADC
    __delay_us(40);             // Sample time
    
  // Configuracion interrupciones
    PIR1bits.ADIF = 0;          // Limpiamos bandera de ADC
    PIE1bits.ADIE = 1;          // Habilitamos interrupcion de ADC
    INTCONbits.PEIE = 1;        // Habilitamos int. de perifericos
    INTCONbits.GIE = 1;         // Habilitamos int. globales
    INTCONbits.T0IE = 1;        // Habilitación de interrupciones del TIMER0
    INTCONbits.T0IF = 0;        // Limpieza bandera de interrupción del TIMER0
 
    
}

/*------------------------------------------------------------------------------
 * FUNCIONES 
 ------------------------------------------------------------------------------*/

void multiplexado(int a[]){
    PORTD = 0x00;
    PORTC = 0x00;
    switch(bandera){
        case 0:                                 // centenas
            PORTDbits.RD2 = 1;                  
            PORTC = tabla(a[1])+(0x80);         
            break;
        case 1:                                 // decenas
            PORTDbits.RD1 = 1;                  
            PORTC = tabla(a[2]);                 
            break;  
        case 2:                                 // unidades
            PORTDbits.RD0 = 1;                  
            PORTC = tabla(a[3]);                
            break;
        default:
            break;
    }
    return;
}

void valores(int a, int b[]){      
    b[1] = a/100;
    b[2] = (a-(100*(b[1])))/10;
    b[3] = (a-(100*(b[1])+10*(b[2])));
    return;
}

// tabla para encender los pines de 7 segmentos correctamente segun valores decimales
uint8_t tabla(int a){
    uint8_t pines = 0;

    switch(a){
        case 0:                 // 0
            pines = 0b10111111;    
            break;
        case 1:                 // 1
            pines = 0b10000110;      
            break;
        case 2:                 // 2
            pines = 0b11011011;       
            break;
        case 3:                 // 3
            pines = 0b11001111;       
            break;
        case 4:                 // 4
            pines = 0b11100110;       
            break;
        case 5:                 // 5
            pines = 0b11101101;       
            break;
        case 6:                 // 6
            pines = 0b11111101;       
            break;
        case 7:                 // 7
            pines = 0b10000111;       
            break;
        case 8:                 // 8
            pines = 0b11111111;       
            break;
        case 9:                 // 9
            pines = 0b11101111;       
            break;
    }
    return pines;               
}

