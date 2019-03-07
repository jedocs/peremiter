// 11,28ms
// Use high voltage MCLR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!4
#include <p24Fxxxx.h>
#include<timer.h>

// PIC24F16KA102 Configuration Bit Settings

int FBS __attribute__((space(prog), address(0xF80000))) = 0xF;
//_FBS(
//    BWRP_OFF &           // Table Write Protect Boot (Boot segment may be written)
//    BSS_OFF              // Boot segment Protect (No boot program Flash segment)
//);
int FGS __attribute__((space(prog), address(0xF80004))) = 0x3;
//_FGS(
//    GWRP_OFF &           // General Segment Code Flash Write Protection bit (General segment may be written)
//    GCP_OFF              // General Segment Code Flash Code Protection bit (No protection)
//);
int FOSCSEL __attribute__((space(prog), address(0xF80006))) = 0x81;

int FOSC __attribute__((space(prog), address(0xF80008))) = 0xDB;
//_FOSC(
//    POSCMOD_NONE &       // Primary Oscillator Configuration bits (Primary oscillator disabled)
//    OSCIOFNC_ON &        // CLKO Enable Configuration bit (CLKO output disabled; pin functions as port I/O)
//    POSCFREQ_HS &        // Primary Oscillator Frequency Range Configuration bits (Primary oscillator/external clock input frequency greater than 8 MHz)
//    SOSCSEL_SOSCLP &     // SOSC Power Selection Configuration bits (Secondary oscillator configured for low-power operation)
//    FCKSM_CSDCMD         // Clock Switching and Monitor Selection (Both Clock Switching and Fail-safe Clock Monitor are disabled)
//);
int FWDT __attribute__((space(prog), address(0xF8000A))) = 0x5F;
//_FWDT(
//    WDTPS_PS32768 &      // Watchdog Timer Postscale Select bits (1:32,768)
//    FWPSA_PR128 &        // WDT Prescaler (WDT prescaler ratio of 1:128)
//    WINDIS_OFF &         // Windowed Watchdog Timer Disable bit (Standard WDT selected; windowed WDT disabled)
//    FWDTEN_OFF           // Watchdog Timer Enable bit (WDT disabled (control is placed on the SWDTEN bit))
//);
int FPOR __attribute__((space(prog), address(0xF8000C))) = 0xFB;    //0x7B
//_FPOR(
//    BOREN_BOR3 &         // Brown-out Reset Enable bits (Brown-out Reset enabled in hardware; SBOREN bit disabled)
//    PWRTEN_ON &          // Power-up Timer Enable bit (PWRT enabled)
//    I2C1SEL_PRI &        // Alternate I2C1 Pin Mapping bit (Default location for SCL1/SDA1 pins)
//    BORV_V18 &           // Brown-out Reset Voltage bits (Brown-out Reset set to lowest voltage (1.8V))
//    MCLRE_On            // MCLR Pin Enable bit 
//);
int FICD __attribute__((space(prog), address(0xF8000E))) = 0xC3;
//_FICD(
//    ICS_PGx1             // ICD Pin Placement Select bits (PGC1/PGD1 are used for programming and debugging the device)
//);
int FDS __attribute__((space(prog), address(0xF80010))) = 0xFF;
//_FDS(
//    DSWDTPS_DSWDTPSF &   // Deep Sleep Watchdog Timer Postscale Select bits (1:2,147,483,648 (25.7 Days))
//    DSWDTOSC_LPRC &      // DSWDT Reference Clock Select bit (DSWDT uses LPRC as reference clock)
//    RTCOSC_SOSC &        // RTCC Reference Clock Select bit (RTCC uses SOSC as reference clock)
//    DSBOREN_ON &         // Deep Sleep Zero-Power BOR Enable bit (Deep Sleep BOR enabled in Deep Sleep)
//    DSWDTEN_ON           // Deep Sleep Watchdog Timer Enable bit (DSWDT enabled)
//);

#include "define.h"
extern volatile char rxstate;
extern volatile unsigned int rf12_crc;
extern volatile unsigned char TX_len;
extern volatile unsigned char rf12_buf[];
int i;

volatile unsigned int counter = 3527;
volatile unsigned char count = 0;
volatile unsigned int count1 = 0;
// rf input/output buffers:
extern unsigned char RxPacket[]; // Receive data puffer (payload only)
extern unsigned char TxPacket[]; // Transmit data puffer
extern unsigned char RxPacketLen;

extern BOOL hasPacket;
extern WORD rcrc, ccrc;
BOOL kuld;
//*************************************************************************************
//
//		Init
//
//*************************************************************************************

void init(void) {

    CLKDIV = 0;
    AD1PCFG = 0b1111111111111111;
    IPC0 = 0b0110000000000111;

    // RF =====================

    TRISA = 0b0000000001110011;
    TRISB = 0b0101011111111011;

    TRISBbits.TRISB2 = 0; // NSEL        	PORTBbits.RB2                              // chip select, active low output
    TRISAbits.TRISA7 = 1; //0; // NFFS        	PORTAbits.RA7                              // rx fifo select, active low output
    TRISAbits.TRISA6 = 1; // NIRQ			PORTAbits.RA6

    NSEL = 1; // nSEL inactive
    //NFFS = 1; // nFFS inactive

    CNPD1 = 0; // pulldowns disable
    CNPD2 = 0; // pulldowns disable

    INTCON2bits.INT2EP = 1; // INT2 lefutó él

    //================= SPI setup ======================================

    SPI1CON1 = 0b0000000100111111; // SMP:0, CKP:0, CKE:1, 8bit mode
    SPI1CON2 = 0b0000000000000000; //enhanced buffer mode disable
    SPI1STATbits.SPIEN = 1; /* Enable/Disable the spi module */

    REFOCON = 0b1010100100000000;

    LED = 0; //LED
    LED_TRIS = 0;
    LED1 = 0;
    LED1_TRIS = 0;
    RXEN = 0;
    TXEN = 0;
    RXEN_TRIS = 0;
    TXEN_TRIS = 0;

    PR1 = 59; //562; //454;
    T1CON = 0b0000000000010000;
    INTCON2bits.INT0EP = 1; // INT2 lefutó él
    IFS0bits.INT0IF = 0;
    IEC0bits.INT0IE = 1;

    IFS0bits.T1IF = 0;
    IEC0bits.T1IE = 1;
    T1CONbits.TON = 1;
}

void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void) {

    count++;
    count1++;

    if (count == 4) count = 0;

    if (count == 0) {
        NFFS = 0;
        LED = 0;
    }
    if (count == 1) {
        NFFS = 0;
        LED = 1;
    }
    if (count == 2) {
        NFFS = 1;
        LED = 0;
    }
    if (count == 3) {
        NFFS = 1;
        LED = 1;
    }

    if (count1 == 8500) {
        count1 = 0;
        LED1 = !LED1;
    }

    IFS0bits.T1IF = 0;
}

//*************************************************************************
//
// Int on change interrupt
//
//*************************************************************************

void __attribute__((interrupt, shadow, auto_psv)) _CNInterrupt(void) {
    IFS1bits.CNIF = 0;
}

//************************************************************************
//
//	Main
//
//************************************************************************

int main(void) {

    init();

    RF_init(SERVER_MBED_NODE, RF12_868MHZ, NETWORD_ID, 0, 0x08);
    TRISAbits.TRISA7 = 0; // NFFS !
    TXEN = 1;
    while (1) {

    }
}


