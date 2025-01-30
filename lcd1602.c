#include "lcd1602.h"

/******************************************************************************\
* Private definitions
\******************************************************************************/
/* LCD functions */
#define LCD_CLEARDISPLAY 		0x01
#define LCD_SETDDRAMADDR 		0x80
#define LCD_FULLLINE				0x40
/* PCF8574 */
#define PCF8574_ADDRESS     0x27 
#define PCF8574A_ADDRESS    0x3f
/* PCF8574 connections to LCD */
//#define PCF8574_D7 0x80
//#define PCF8574_D6 0x40
//#define PCF8574_D5 0x20
//#define PCF8574_D4 0x10
#define PCF8574_BL 0x08 /* Backlight */
#define PCF8574_EN 0x04 /* Enable bit */
#define PCF8574_RW 0x02 /* Read/Write bit (0 = write) */
#define PCF8574_RS 0x01 /* Register select bit */
/******************************************************************************\
* Private memory declarations
\******************************************************************************/
static uint8_t lcd_backlight = 1;
static uint8_t pcf_address = PCF8574_ADDRESS;
/******************************************************************************\
* Private prototypes
\******************************************************************************/
void PCF8574_Write(uint8_t data);
void LCD1602_Write4(uint8_t data, uint8_t rs);
void LCD1602_Write8(uint8_t data, uint8_t rs);
void LCD1602_CheckAddress(void);
uint8_t itoa(int value, char *ptr);
char PL_CH_a_[] = {0x0,0x0,0xe,0x1,0xf,0x11,0xf,0x1};			// a with tail
char PL_CH_c_[] = {0x2,0x4,0xe,0x10,0x10,0x11,0xe,0x0};		// c with accent
char PL_CH_e_[] = {0x0,0x0,0xe,0x11,0x1f,0x10,0xe,0x2};		// e with tail
char PL_CH_l_[] = {0xc,0x4,0x6,0x4,0xc,0x4,0xe,0x0};			// l with slash
char PL_CH_n_[] = {0x2,0x4,0x16,0x19,0x11,0x11,0x11,0x0};	// n with accent
char PL_CH_o_[] = {0x2,0x4,0xe,0x11,0x11,0x11,0xe,0x0};		// o with accent
char PL_CH_s_[] = {0x2,0x4,0xe,0x10,0xe,0x1,0x1e,0x0};		// s with accent
char PL_CH_z_[] = {0x4,0x0,0x1f,0x2,0x4,0x8,0x1f,0x0};		// z with dot
char PL_CH_z__[] = {0x2,0x4,0x1f,0x2,0x4,0x8,0x1f,0x0};		// z with accent

void LCD1602_Init(void) {																																		

	I2C_Init();											/* via I2C communication */
	
	LCD1602_CheckAddress();					/* if any PCF connected check which one */
																											
	DELAY(200)												/* >15ms */
	
	LCD1602_Write8(0x33,0);					/* 4-bit interface */								
	LCD1602_Write8(0x32,0);					/* HD44780U datasheet Figure 24 */
	LCD1602_Write8(0x28,0);
	LCD1602_Write8(0x08,0);
	LCD1602_Write8(0x01,0);
	LCD1602_Write8(0x0C,0);					/* cursor off, blink off */
}
void LCD1602_ClearLine(uint8_t line) // funkcja dodatkowa czyszczaca pojedyncza linie
{
    LCD1602_SetCursor(0, line);
    LCD1602_Print("                ");  // 16 spacji
    LCD1602_SetCursor(0, line);  // Powrót kursora na pocz¹tek linii
}


void LCD1602_Blink_On(void) {
	LCD1602_Write8(0x0D,0);						/* cursor off, blink on */
}

void LCD1602_Blink_Off(void) {
	LCD1602_Write8(0x0C,0);						/* cursor off, blink off */
}

void LCD1602_Blink_Off_Cursor_On(void) {
	LCD1602_Write8(0x0E,0);						/* cursor on, blink off */
}

void LCD1602_SetCursor(uint8_t col, uint8_t row) {
	if (row>1) row = 1;   /* prevents from too many rows */
	if (col>39)	col = 39;	/* prevents from being over range */
	LCD1602_Write8((LCD_SETDDRAMADDR |(col+(LCD_FULLLINE*row))),0);		/* | prevents from incorrect instruction */
}

void LCD1602_ClearAll(void) {
	
	LCD1602_Write8(LCD_CLEARDISPLAY, 0);
}

void LCD1602_Print(char *str) {

  uint8_t str_len = 0;
	
  while (str[str_len] != '\0') /* until end of string */
	{             
    LCD1602_Write8(str[str_len], 1);
    ++str_len;
  }
}

void LCD1602_PrintNum(int number) {

  char str[12];
	uint8_t size;
	
	size = itoa(number, str);
	LCD1602_Print(str);
	if (size < 5) LCD1602_Print("      ");   /* change if display bigger values*/
}

void LCD1602_Backlight(uint8_t state) {
	
	lcd_backlight = state;				 /* update internal variable */
	PCF8574_Write(0x00);
}
/**
 * @brief Write byte to LCD including backlight info.
 *
 * @param Data to send.
 */
void PCF8574_Write(uint8_t data) {
	
	I2C_Write(pcf_address, data | (lcd_backlight?PCF8574_BL:0x00));
}
/**
 * @brief Write 4 bits to LCD.
 *
 * @param A lower nibble of the byte.
 * @param Register select.
 */
void LCD1602_Write4(uint8_t data, uint8_t rs) {
	
	PCF8574_Write(((data << 4)&0xF0) | (rs?PCF8574_RS:0x00) | PCF8574_EN);
	PCF8574_Write(((data << 4)&0xF0) | (rs?PCF8574_RS:0x00));
	//DELAY(27)
}
/**
 * @brief Write byte to LCD.
 *
 * @param Data to send.
 * @param Register select
 */
void LCD1602_Write8(uint8_t data, uint8_t rs) {
	uint8_t temp;
	while(LCD1602_BF_AC(&temp));
	LCD1602_Write4(((data >> 4)&0x0F), rs);
	LCD1602_Write4(( data      &0x0F), rs);
}
/**
 * @brief Check which version of PCF is connected.
 *
 */
void LCD1602_CheckAddress(void) {
	
	if ((I2C_Write(PCF8574_ADDRESS, 0x00) & I2C_ERR_NOACK)==0) 
		pcf_address = PCF8574_ADDRESS;
	if ((I2C_Write(PCF8574A_ADDRESS, 0x00) & I2C_ERR_NOACK)==0) 
		pcf_address = PCF8574A_ADDRESS;
}
/**
 * @brief Convert integer to string.
 *
 * @param Value to convert.
 * @param String to return.
 * @return Size of string.
 */
uint8_t itoa(int value, char *ptr) {
	
	uint8_t count=0;
	int temp;
	
	if(value==0) {   
		*ptr++='0';
		*ptr='\0';
		return 1;
	}
	if(value<0) {
		value*=(-1);    
		*ptr++='-';
		count++;
	}
	for(temp=value;temp>0;ptr++) {
		temp/=10;
	}
	*ptr='\0';
	for(temp=value;temp>0;temp/=10) {
		*--ptr=temp%10+'0';
		count++;
	}
	return count;
}

uint8_t LCD1602_BF_AC(uint8_t *ptr)
{
	uint8_t buf,bf_flag;
	uint8_t temp_dh, temp_dl;
	buf=0xf0;
	
	buf |=((lcd_backlight?PCF8574_BL:0x00) | PCF8574_RW);
	I2C_Write(pcf_address, buf);
	buf |= PCF8574_EN;
	I2C_Write(pcf_address, buf);
	I2C_Read(pcf_address, &temp_dh);
	buf &= (~PCF8574_EN);
	I2C_Write(pcf_address, buf);
	buf |= PCF8574_EN;
	I2C_Write(pcf_address, buf);
	I2C_Read(pcf_address, &temp_dl);
	buf &= (~(PCF8574_EN | PCF8574_RW));
	I2C_Write(pcf_address, buf);
	buf = ((temp_dh & 0xf0) | (temp_dl>>4));
	*ptr = buf;
	bf_flag = buf & 0x80;
	return bf_flag;	
}

void LCD1602_PL_CH(void)
{
	uint8_t i;
	LCD1602_Write8(0x40,0);		// Set CGRAM address = 0
	for(i=0;i<8;i++)
	{
		LCD1602_Write8(PL_CH_a_[i],1);
	}
	for(i=0;i<8;i++)
	{
		LCD1602_Write8(PL_CH_c_[i],1);
	}
	for(i=0;i<8;i++)
	{
		LCD1602_Write8(PL_CH_e_[i],1);
	}
	for(i=0;i<8;i++)
	{
		LCD1602_Write8(PL_CH_l_[i],1);
	}
	for(i=0;i<8;i++)
	{
		LCD1602_Write8(PL_CH_n_[i],1);
	}
	for(i=0;i<8;i++)
	{
		LCD1602_Write8(PL_CH_o_[i],1);
	}
	for(i=0;i<8;i++)
	{
		LCD1602_Write8(PL_CH_s_[i],1);
	}for(i=0;i<8;i++)
	{
		LCD1602_Write8(PL_CH_z_[i],1);
	}
	LCD1602_Write8(0x80,0);		// Set DDRAM address = 0
}

void LCD1602_PrintPL(char *str)
{

  uint8_t str_len = 0;
	char buf;
	
  while (str[str_len] != '\0') 		/* until end of string */
	{
		buf = str[str_len];
		if(buf=='/')
		{
			if((str[str_len+1] != '\0') && (str[str_len+1]=='/'))
			{
				if(str[str_len+2] != '\0')
				{
					str_len+=2;
					switch (str[str_len])
					{
						case 'a': buf=0x0;
											break;
						case 'c': buf=0x01;
											break;
						case 'e': buf=0x02;
											break;
						case 'l': buf=0x03;
											break;
						case 'n': buf=0x04;
											break;
						case 'o': buf=0x05;
											break;
						case 's': buf=0x06;
											break;
						case 'z': buf=0x07;
											break;
						default:	str_len-=2;
											break;
					}
				}
			}
		}		
    LCD1602_Write8(buf, 1);
    ++str_len;
  }
	
}
