#include "DAC.h"
#include "frdm_bsp.h"

void DAC_Init(void)
{
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
	SIM->SCGC6 |= SIM_SCGC6_DAC0_MASK;          // Dołączenie sygnału zegara do DAC0
	DAC0->C1 |= DAC_C1_DACBFEN_MASK;						// Włączenie bufora 2x16 bit
	DAC0->C0 |= (DAC_C0_DACEN_MASK | DAC_C0_DACTRGSEL_MASK);	// Włączenie DAC0, wyzwalanie programowe
}

uint8_t DAC_Load_Trig(uint16_t load)
{
	uint8_t load_temp, pos=0;
	if(load>0xFFF)	return (1);		// Sprawdzenie zakresu danej wejściowej
	load_temp=load&0x0FF;
	pos=(DAC0->C2^0x11)>>4;
	DAC0->DAT[pos].DATL = load_temp;	// Załadowanie młodszego bajtu przetwornika C/A
	load_temp=(load>>8);
	DAC0->DAT[pos].DATH = load_temp;	// Załadowanie starszego bajtu przetwornika C/A
	DAC0->C0 |= DAC_C0_DACSWTRG_MASK;		// Przełączenie na następną daną z bufora (przed chwilą załadowaną)
	return (0);
}
