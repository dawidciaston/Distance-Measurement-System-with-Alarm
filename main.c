#include "MKL05Z4.h"
#include "frdm_bsp.h"
#include "lcd1602.h"
#include "TPM.h"
#include "DAC.h"
#include "klaw.h"
#include "tsi.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define M_PI 3.14159265358979323846

// -------------------- [FLAGI DO OBSŁUGI PRZYCISKÓW] --------------------
volatile uint8_t S1_press = 0;
volatile uint8_t S2_press = 0;
volatile uint8_t S3_press = 0;
volatile uint8_t S4_press = 0;

// -------------------- [ZMIENNE GLOBALNE] --------------------
volatile uint32_t d = 0;            // Wartość dzielnika dla TPM
volatile float wynik = 0;           // Wynik pomiaru czasu
volatile float tick, tick_head;     // Tick do przelicznika czasu dla TPM
volatile float distance;            // Odległość obliczona na podstawie wyniku
volatile uint8_t unit = 0;          // Aktualnie wybrana jednostka (0: metry, 1: cm, 2: mm, 3: cale)
volatile uint8_t unitSelected = 0;  // Flaga informująca o wyborze jednostki
volatile uint8_t inMenu = 0;        // Flaga informująca, czy menu jest aktywne
volatile uint32_t delay_count;      // Licznik opóźnienia
volatile uint8_t tpm1InterruptFlag = 0; // Synchronizacja pętli głównej z TPM1

// -------------------- [ZMIENNE GLOBALNE - DAC/sinus] --------------------
#define DIV_CORE    8192
#define MASKA_10BIT 0x03FF
volatile uint16_t dac;         
volatile int16_t  Sinus[1024]; 
volatile uint16_t faza, mod;
volatile uint8_t  beepOn = 0; 

// Próg w cm, poniżej którego ma być dźwięk
volatile float DISTANCE_THRESHOLD_CM = 20.0f;


// Funkcja obsługująca przerwanie SysTick
void SysTick_Handler(void)
{
    // --- Kod odliczania 1ms (opóźnienia) ---
    if (delay_count > 0)
    {
        delay_count--;
    }

    // --- Kod generowania dźwięku ---
    if (beepOn)
    {
        // pobierz próbkę z tablicy i załaduj do DAC
        dac = ( (Sinus[faza] / 100) + 0x0800 );
        DAC_Load_Trig(dac);

        // zaktualizuj fazę
        faza += mod;
        faza &= MASKA_10BIT;
    }
}

// Funkcja realizująca opóźnienie w milisekundach przy użyciu SysTick
void SysTick_Delay(uint32_t delay_ms)
{
    delay_count = delay_ms;
    while (delay_count != 0)
    {
        // Czekaj na zakończenie odliczania
    }
}
// Regulacja odleglosci beep
void update_threshold_display()
{
    char display[16];
    LCD1602_ClearLine(0);
    LCD1602_SetCursor(0, 0);
    
    switch (unit)
    {
        case 0:
            sprintf(display, "Thresh=%5.1f m", DISTANCE_THRESHOLD_CM / 100.0f);
            break;
        case 1:
            sprintf(display, "Thresh=%5.1f cm", DISTANCE_THRESHOLD_CM);
            break;
        case 2:
            sprintf(display, "Thresh=%5.1f mm", DISTANCE_THRESHOLD_CM * 10.0f);
            break;
        case 3:
            sprintf(display, "Thresh=%5.1f in", DISTANCE_THRESHOLD_CM / 2.54f);
            break;
    }

    LCD1602_Print(display);
}

// Funkcja przerwania TPM1 dla pomiaru czasu impulsu
void TPM1_IRQHandler(void)
{
    TPM1->SC = 0; // Zatrzymanie timera TPM1
    
    if ((TPM1->STATUS & TPM_STATUS_TOF_MASK)) // Sprawdzenie, czy doszło do przepełnienia
    {
        TPM1->SC = 0;
        wynik = 100000;
        d += 1;
        if (d == 8) d = 0;
    }
    if (TPM1->STATUS & TPM_STATUS_CH1F_MASK) // Sprawdzenie, czy wykryto zdarzenie na kanale 1
    {
        wynik = TPM1->CONTROLS[1].CnV; // Odczyt wyniku pomiaru
    }
    
    // Kasowanie flag przerwania
    TPM1->STATUS |= TPM_STATUS_CH1F_MASK;
    TPM1->STATUS |= TPM_STATUS_TOF_MASK;
    
    // Przywrócenie konfiguracji timera
    TPM1->SC = d;
    TPM1->SC |= TPM_SC_TOIE_MASK; // Odblokowanie przerwań od TOF
    TPM1->SC |= TPM_SC_CMOD(1);   // Ponowne uruchomienie timera

    // --- SYGNAŁ DO GŁÓWNEJ PĘTLI, ŻE POMIAR SIĘ ZAKOŃCZYŁ ---
    tpm1InterruptFlag = 1;
}

// Funkcja aktualizująca wyświetlanie odległości na ekranie LCD
void update_distance_display()
{
    char display[16];

    // Wyczyść linię, na której wyświetlana jest odległość
    LCD1602_ClearLine(1);

    // Ustaw kursor na początku linii
    LCD1602_SetCursor(0, 1);

    if (distance > 350.0f)
    {
        sprintf(display, "Zbyt daleko");
    }
    else
    {
        switch (unit)
        {
            case 0:
                sprintf(display, "dist=%6.2f m", distance / 100);
                break;
            case 1:
                sprintf(display, "dist=%6.2f cm", distance);
                break;
            case 2:
                sprintf(display, "dist=%6.2f mm", distance * 10);
                break;
            case 3:
                sprintf(display, "dist=%6.2f in", distance / 2.54);
                break;
        }
    }

    LCD1602_Print(display);
}

// Funkcja wyboru jednostki pomiaru (menu)
void select_unit()
{
    LCD1602_ClearAll();
    LCD1602_SetCursor(0, 0);
    LCD1602_Print("Czujnik dystansu");
    LCD1602_SetCursor(0, 1);
    LCD1602_Print("Menu:");
    SysTick_Delay(2000);

    LCD1602_ClearAll();
    LCD1602_SetCursor(0, 0);
    LCD1602_Print("1:metry 2:cm");
    LCD1602_SetCursor(0, 1);
    LCD1602_Print("3:mm 4:cale");

    unitSelected = 0;
    inMenu = 1;

    while (!unitSelected)
    {
        if (S1_press)
        {
            unit = 0;         
            unitSelected = 1;
            S1_press = 0;
        }
        else if (S2_press)
        {
            unit = 1;         
            unitSelected = 1;
            S2_press = 0;
        }
        else if (S3_press)
        {
            unit = 2;         
            unitSelected = 1;
            S3_press = 0;
        }
        else if (S4_press)
        {
            unit = 3;         
            unitSelected = 1;
            S4_press = 0;
        }
    }

    LCD1602_ClearAll();
    inMenu = 0;
}

// Funkcja obsługująca przerwania od przycisków (PORTA)
void PORTA_IRQHandler(void)
{
    uint32_t buf;

    buf = PORTA->ISFR & (S1_MASK | S2_MASK | S3_MASK | S4_MASK);

    switch(buf)
    {
        case S1_MASK:
            DELAY(100);
            if (!(PTA->PDIR & S1_MASK))
            {
                DELAY(100);
                if (!(PTA->PDIR & S1_MASK))
                {
                    S1_press = 1;
                }
            }
            break;

        case S2_MASK:
            DELAY(100);
            if (!(PTA->PDIR & S2_MASK))
            {
                DELAY(100);
                if (!(PTA->PDIR & S2_MASK))
                {
                    S2_press = 1;
                }
            }
            break;

        case S3_MASK:
            DELAY(100);
            if (!(PTA->PDIR & S3_MASK))
            {
                DELAY(100);
                if (!(PTA->PDIR & S3_MASK))
                {
                    S3_press = 1;
                }
            }
            break;

        case S4_MASK:
            DELAY(100);
            if (!(PTA->PDIR & S4_MASK))
            {
                DELAY(100);
                if (!(PTA->PDIR & S4_MASK))
                {
                    S4_press = 1;
                }
            }
            break;

        default:
            break;
    }

    PORTA->ISFR |= (S1_MASK | S2_MASK | S3_MASK | S4_MASK);
    NVIC_ClearPendingIRQ(PORTA_IRQn);
}


// Inicjalizacja TPM0 do generowania krótkiego impulsu TRIG.
void TRIG_PWM_Init(void)
{
    // Włącz zegar na port B i TPM0
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK; 
    SIM->SCGC6 |= SIM_SCGC6_TPM0_MASK;

    // PTB8 -> MUX = 2 => TPM0_CH3
    PORTB->PCR[8] &= ~PORT_PCR_MUX_MASK;
    PORTB->PCR[8] |=  PORT_PCR_MUX(2);

    // Ustaw źródło taktowania TPM na MCGFLLCLK (42 MHz)
    SIM->SOPT2 |= SIM_SOPT2_TPMSRC(1);

    // Preskaler = 64 => zegar TPM0 będzie = 42 MHz / 64 = 656250 Hz
    TPM0->SC = 0;
    TPM0->SC |= TPM_SC_PS(6);     // ps=6 -> 64

    // Ustalenie okresu PWM
    TPM0->MOD = 32000;   // częstotliwość ~ 20.5 Hz

    // Ustawiamy Edge-aligned PWM
    TPM0->CONTROLS[3].CnSC = 0;
    TPM0->CONTROLS[3].CnSC |= TPM_CnSC_MSB_MASK | TPM_CnSC_ELSA_MASK;

    // Wypełnienie tak, żeby impuls wysoki trwał ~ 10-20 us
    TPM0->CONTROLS[3].CnV = 13;

    // Start licznika
    TPM0->SC |= TPM_SC_CMOD(1); 
}

// Funkcja główna programu
int main(void)
{
    char display[16];
    volatile uint32_t ps_value[] = {1, 2, 4, 8, 16, 32, 64, 128};
    tick_head = 1000.0 / SystemCoreClock;

    LCD1602_Init();
    LCD1602_Backlight(TRUE);
    LCD1602_ClearAll();
    
    // Input Capture do odczytu ECHA (TPM1_CH1):
    InCap_OutComp_Init();
    
    // Klawiatura
    Klaw_Init();
    Klaw_S1_4_Int();
		// Panel dotykowy
		TSI_Init();
    // Inicjalizacja DAC + sinus do beep
    DAC_Init();
    for(uint16_t i = 0; i < 1024; i++)
    {
        Sinus[i] = (int16_t)(sin((double)i * 2.0 * M_PI / 1024.0) * 2047.0);
    }
    faza = 0;
   mod = 256; 

    // Uruchomienie SysTick co 1 ms
    SysTick_Config(SystemCoreClock / 1000);

    // Generowanie impulsu
    TRIG_PWM_Init();

    // Menu wyboru jednostki
    select_unit();
		// Zmiana odleglosci beep
		update_threshold_display();

    while (1)
    {		
				if (S1_press) { unit = 0; S1_press = 0; update_threshold_display(); } // metry
				if (S2_press) { unit = 1; S2_press = 0; update_threshold_display(); } // cm
				if (S3_press) { unit = 2; S3_press = 0; update_threshold_display(); } // mm
				if (S4_press) { unit = 3; S4_press = 0; update_threshold_display(); } // cale

        while (!tpm1InterruptFlag)
        {
        }
        tpm1InterruptFlag = 0;

        tick = tick_head * ps_value[d];
        if (wynik != 100000)
        {
					wynik *= tick;
					float new_distance = (wynik / 58.0f) * 1000.0f;
					
					update_distance_display();
        }

        uint8_t slider_value = TSI_ReadSlider();
        if (slider_value != 0)
        {
            DISTANCE_THRESHOLD_CM = (float)slider_value / 2.0f;
            update_threshold_display();
        }

        if (distance < DISTANCE_THRESHOLD_CM)
            beepOn = 1;
        else
            beepOn = 0;

        SysTick_Delay(50);
    }
}












