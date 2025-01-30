#include "TPM.h"

void InCap_OutComp_Init()
{
    /************************************************************************
     * 1) Włączenie sygnału zegara dla portu B:
     *    Musimy odblokować port B (SCGC5) zanim skonfigurujemy jego piny.
     ************************************************************************/
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK; // Dołączenie sygnału zegara do portu B
    
    /************************************************************************
     * 2) Ustawienie funkcji (MUX) poszczególnych pinów portu B:
     *    - PTB8  -> TPM0_CH3 (początkowo wykorzystywany do PWM, np. LED R lub TRIG)
     *    - PTB9  -> TPM0_CH2 (początkowo PWM, LED G)
     *    - PTB10 -> TPM0_CH1 (początkowo PWM, LED B)
     *    - PTB0  -> TPM1 EXTRG_IN (zewnętrzny sygnał wyzwalający dla TPM1)
     *    - PTB13 -> TPM1_CH1 (Input Capture)
     *    - PTB11 -> GPIO (wyjście – w pierwotnym kodzie służył do „ręcznego” TRIG)
     ************************************************************************/
    PORTB->PCR[8]  |= PORT_PCR_MUX(2);  // PTB8: TPM0_CH3 (PWM)
    PORTB->PCR[9]  |= PORT_PCR_MUX(2);  // PTB9: TPM0_CH2 (PWM)
    PORTB->PCR[10] |= PORT_PCR_MUX(2);  // PTB10: TPM0_CH1 (PWM)
    PORTB->PCR[0]  |= PORT_PCR_MUX(2);  // PTB0: EXTRG_IN dla TPM1
    PORTB->PCR[13] |= PORT_PCR_MUX(2);  // PTB13: TPM1_CH1 (Input Capture)
    
    // PTB11 jako GPIO (w oryginalnym kodzie służył do TRIG „ręcznego”)
    PORTB->PCR[11] |= PORT_PCR_MUX(1);  
    PORTB->PCR[11] &= (~PORT_PCR_SRE_MASK); // SRE=0: minimalnie szybsze narastanie zboczy
    PTB->PDDR      |= (1 << 11);           // PTB11 jako wyjście
    PTB->PCOR       = (1 << 11);           // Ustaw na 0 stan początkowy (LED off / TRIG low)
    
    /************************************************************************
     * 3) Włączenie zegara dla modułu TPM1:
     *    Zanim skonfigurujemy rejestry TPM1, musimy je „odblokować”.
     ************************************************************************/
    SIM->SCGC6 |= SIM_SCGC6_TPM1_MASK; // Dołączenie zegara do TPM1
    
    /************************************************************************
     * 4) Ustawienie źródła taktowania TPMx na MCGFLLCLK (~42 MHz w KL05):
     ************************************************************************/
    SIM->SOPT2 |= SIM_SOPT2_TPMSRC(1); // Wybierz źródło taktowania TPMx: MCGFLLCLK=~42 MHz
    
    /************************************************************************
     * 5) Tryb pracy TPM1
     *    - CPWMS=0 => zliczanie "up counting"
     *    - PS=0    => preskaler = 1, czyli zegar wewnętrzny do TPM1 = ~42 MHz
     ************************************************************************/
    TPM1->SC &= (~TPM_SC_CPWMS_MASK); // TPM1 w trybie "up counting"
    TPM1->SC &= (~TPM_SC_PS_MASK);    // Dzielnik zegara = 1 (PS=0)
    
    /************************************************************************
     * 6) Konfiguracja rejestru CONF dla TPM1:
     *    - CSOT = 1 => licznik startuje przy wyzwoleniu zewnętrznym (EXTRG_IN)
     *    - CROU = 1 => licznik zerowany przy wyzwoleniu
     *    - CSOO = 1 => licznik zatrzymywany przy OVERFLOW
     *    - TRGSEL=0 => źródłem wyzwalania jest sygnał EXTRG_IN
     ************************************************************************/
    TPM1->CONF |=  (TPM_CONF_CSOT_MASK  |  // Start licznika po triggerze
                    TPM_CONF_CROT_MASK  |  // Zerowanie po triggerze
                    TPM_CONF_CSOO_MASK);   // Stop licznika przy OVERFLOW
    TPM1->CONF &= (~TPM_CONF_TRGSEL_MASK); // Wyzwalanie zewn. (EXTRG_IN), TRGSEL=0
    
    /************************************************************************
     * 7) Ustawienie kanału TPM1_CH1 (PTB13) w tryb Input Capture:
     *    - ELSB=1, ELSA=0 => reagujemy na opadające zbocze (falling edge)
     *    - CHIE=1         => włącz przerwanie od kanału (po wykryciu input capture)
     ************************************************************************/
    TPM1->CONTROLS[1].CnSC = (TPM_CnSC_ELSB_MASK | TPM_CnSC_CHIE_MASK);
    
    /************************************************************************
     * 8) Włączenie przerwań od OVERFLOW w TPM1:
     ************************************************************************/
    TPM1->SC |= TPM_SC_TOIE_MASK; // Przerwanie od przepełnienia
    
    // Wyczyszczenie i włączenie przerwania w NVIC (kontrolerze przerwań)
    NVIC_ClearPendingIRQ(TPM1_IRQn);
    NVIC_EnableIRQ(TPM1_IRQn);
    
    /************************************************************************
     * 9) Włączenie licznika TPM1 (CMOD=1), 
     *    jednak pamiętaj, że realne zliczanie nastąpi po sygnale wyzwalającym.
     ************************************************************************/
    TPM1->SC |= TPM_SC_CMOD(1); // Uruchom TPM1 (zegar wewnętrzny)
}
