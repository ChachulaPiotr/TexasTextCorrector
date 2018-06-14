#include <msp430x14x.h>
#include <stdbool.h>
#include <in430.h>
#include <intrinsics.h>
#include <msp430f1611.h>
#include <stdint.h>

#define PROGRAM_RESET_VECTOR_ADDRESS 0x20FE

int iterator = 0;
int stopping = 0;
int state = 0;
char TEXT[100];
char* START = "\n\rSTART\n";
char koniec = '\r';
uint16_t value_DMASZ;

void runprogram();
void runReceivedProgram();


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
	// DMA0 transmiting
	DMACTL0 |= DMA0TSEL_4; // transmit trigger transmit
	DMA0CTL |= DMASRCINCR_3 + DMASRCBYTE + DMADSTBYTE + DMALEVEL + DMAIE; // increment source, byte in, byte out, level triggered

	// DMA1 recv
	DMACTL0 |= DMA1TSEL_3; // recv trigger recv
	DMA1CTL |= DMADSTINCR_3 + DMASRCBYTE + DMADSTBYTE + DMALEVEL + DMAIE; // increment source, byte in, byte out, level triggered

	TA0CCTL0 |= CAP | CM_0;
	TA0CTL |= TASSEL_1 | ID_3 | TAIE;
	TA0CCR0 = 5000;


//UART koniec---------------------------------------------------------------------

	P3SEL = BIT4 + BIT5;
	P3DIR = BIT4;
	_EINT();
 	DMA1SA = (unsigned int)&U0RXBUF; // Source block address
	//DMA1DA = 0x1100; // Dest single address
	//DMA1SZ = 1; // Block size
	//DMA1CTL |= DMAEN;
	//drukuj (START);
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
		//__bis_SR_register(LPM1_bits | GIE);
	}
}


inline void checkiffilesent(){
	int i =0;
	while (i<50){
		if (TEXT[i] == koniec){
			state = 2;
			TACTL |= MC_0 | TACLR;
			DMA1CTL &= ~DMAEN;
			break;
		}
		i++;
	}
	if (state != 2) __bis_SR_register(LPM1_bits | GIE);
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
	//DMA1SZ = value_DMASZ;
	DMA1SZ = 50;
	DMA1CTL |= DMAEN;
	DMA1DA = TEXT;
	TACTL |= MC_1;
	__bis_SR_register(LPM1_bits | GIE);
}


inline void drukuj (char * napis){
	iterator = 0;
	while (1){
		if (napis[iterator]=='\0'){
			iterator = 0;
			break;
		}
		else{
			stopping = 1;
			U0TXBUF = napis[iterator];
			while (stopping){}

		}
	}
}


#pragma vector = USART0TX_VECTOR // TRANSMITER
__interrupt void usart0_tx (void)
{
	_BIC_SR_IRQ(LPM1_bits);
}
#pragma vector = USART0RX_VECTOR // TRANSMITER
__interrupt void usart0_rx (void)
{
	TACTL |= MC_1;
	IE1 &= ~URXIE0;
}

#pragma vector=DACDMA_VECTOR
__interrupt void dmaisr(void)
{
	state = 2;
	TACTL |= MC_0 | TACLR;
	DMA1CTL &= ~DMAIFG;
	_BIC_SR_IRQ(LPM1_bits);
}

#pragma vector=TIMER0_A1_VECTOR
__interrupt void ISR_timer (void)
{

	state = 1;
	TACTL &= ~(TAIFG);
    _BIC_SR_IRQ(LPM1_bits);
}

