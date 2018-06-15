#include <msp430x14x.h>
#include <stdbool.h>
#include <in430.h>
#include <intrinsics.h>
#include <msp430f1611.h>
#include <stdint.h>

int state = 0;
char TEXT[100];
char koniec = '\r';

int main( void )
{
 	WDTCTL = WDTPW + WDTHOLD;

//UART---------------------------------------------------------------------------

	DCOCTL = DCO0;  //ustawienie zegara SMCLK
	BCSCTL1 |= RSEL2 + RSEL0;  //SMCLK

	U0CTL |= SWRST; // Wlaczenie software reset

	U0CTL |= CHAR; // 8-bit character
	U0TCTL |= SSEL1; // Source clock SMCLK

	ME1 |= UTXE0 | URXE0; // Enabled USART0 TXD RXD

	// Ustawienia dla 115200
	U0BR0 = 0x09;
	U0BR1 = 0x00;
	U0MCTL = 0x08;

	U0CTL &= ~SWRST; // Wylaczenie software reset

	IE1 |= UTXIE0;
	// DMA1 recv
	DMACTL0 |= DMA1TSEL_3; // recv trigger recv
	DMA1CTL |= DMADSTINCR_3 + DMASRCBYTE + DMADSTBYTE + DMALEVEL + DMAIE; // increment source, byte in, byte out, level triggered

	TA0CCTL0 |= CAP | CM_0;
	TA0CTL |= TASSEL_1 | ID_3 | TAIE;
	TA0CCR0 = 500;

	TB0CCTL0 |= CAP | CM_0;
	TB0CTL |= TASSEL_1 | ID_3 | TAIE;
	TB0CCR0 = 20000;


//UART koniec---------------------------------------------------------------------

	P3SEL = BIT4 + BIT5;
	P3DIR = BIT4;
	_EINT();
 	DMA1SA = (unsigned int)&U0RXBUF; // Source block address
	while(true)
	{
		if(state == 0){
			getready4file();
		}
		if(state == 1){
			checkiffilesent();
		}
		if (state == 2){
			fixtext();
		}
	}
}


inline void checkiffilesent(){
	int i =0;
	DMA1CTL &= ~DMAEN;
	_DINT();
	while (i<50){
		if (TEXT[i] == koniec){
			state = 2;
			TACTL &= ~MC_1;
			TACTL |= TACLR;
			TBCTL &= ~MC_1;
			TBCTL |= TBCLR;
			break;
		}
		i++;
	}
	_EINT();
	if (state != 2)  __bis_SR_register(LPM1_bits | GIE);
}

inline void fixtext(){
	char a = TEXT[0];
	if (a == koniec) {
		U0TXBUF = '\n';
		__bis_SR_register(LPM1_bits | GIE);
		state = 0;
		return;
	}
	a = (a>96 && a<123) ? a-32: a;
	U0TXBUF = a;
	char prev = (a>64 && a<91) ? a+32: a;
	a = TEXT[1];
	int i;
	for(i=1; a!=koniec;++i){
		a = (a>64 && a<91) ? a+32: a;
		if (a != prev)
			if(!(a == ' ' && (TEXT[i+1] == '.' || TEXT[i+1] == ',' || TEXT[i+1] == '!' || TEXT[i+1] == '?')))
			{
				U0TXBUF = a;
				__bis_SR_register(LPM1_bits | GIE);
				prev = a;
			}
		a = TEXT[i+1];
	}
	state = 0;
	TEXT[i] = 'z';
	U0TXBUF = '\n';
	__bis_SR_register(LPM1_bits | GIE);
}

inline void getready4file(){
	DMA1SZ = 50;
	DMA1CTL |= DMAEN;
	DMA1DA = TEXT+1;
	IE1 |= URXIE0;
	__bis_SR_register(LPM1_bits | GIE);
}



#pragma vector = USART0TX_VECTOR // TRANSMITER
__interrupt void usart0_tx (void)
{
	_BIC_SR_IRQ(LPM1_bits);
}
#pragma vector = USART0RX_VECTOR // TRANSMITER
__interrupt void usart0_rx (void)
{
	TEXT[0] = U0RXBUF;
	TACTL |= MC_1;
	TBCTL |= MC_1;
	IE1 &= ~URXIE0;
}

#pragma vector=DACDMA_VECTOR
__interrupt void dmaisr(void)
{
	state = 1;
	TACTL &= ~MC_1;
	TACTL |= TACLR;
	TBCTL &= ~MC_1;
	TBCTL |= TBCLR;
	DMA1CTL &= ~DMAIFG;
	_BIC_SR_IRQ(LPM1_bits);
}

#pragma vector=TIMER0_A1_VECTOR
__interrupt void ISR_timerA (void)
{

	state = 1;
	TACTL &= ~(TAIFG);
    _BIC_SR_IRQ(LPM1_bits);
}
#pragma vector=TIMER0_B1_VECTOR
__interrupt void ISR_timerB (void)
{

	state = 0;
	TBCTL &= ~(TBIFG);
	TACTL &= ~MC_1;
	TACTL |= TACLR;
	TBCTL &= ~MC_1;
	TBCTL |= TBCLR;
	U0TXBUF = 'x';
    _BIC_SR_IRQ(LPM1_bits);
}



