#include "i2c.h"

/******************************************************************************\
* Private definitions
\******************************************************************************/
#define SCL   3
#define SDA   4
/******************************************************************************\
* Private prototypes
\******************************************************************************/
void i2c_m_start(void);
void i2c_m_stop(void);
void i2c_m_rstart(void);
void i2c_tran(void);
void i2c_rec(void);
void i2c_enable(void);
void i2c_disable(void);
void i2c_send(uint8_t);
uint8_t i2c_recv(void);
void i2c_wait(void);
void i2c_nack(void);
void i2c_ack(void);
void i2c_clr_IICIF(void);
/******************************************************************************\
* Private memory declarations
\******************************************************************************/
static uint8_t error;
static uint16_t timeout;
volatile uint8_t dummy;

void I2C_Init(void) {	
SIM->SCGC4 |= SIM_SCGC4_I2C0_MASK ;		/* clock for I2C0  */
SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;		/* clock for PORTB */
	
PORTB->PCR[SCL] |= PORT_PCR_MUX(2); 	/* I2C0 SCL	(PTB3) */
PORTB->PCR[SDA] |= PORT_PCR_MUX(2); 	/* I2C0 SDA	(PTB4) */
	
I2C0->C1 &= ~(I2C_C1_IICEN_MASK);			/* disable module during modyfications*/
I2C0->F  |= I2C_F_MULT(0x01);					/* MULT = 0,1,2 */
I2C0->F  |= I2C_F_ICR(0x01);					/* SCLdivider = Table 36-28. I2C divider and hold values. Reference Manual p.622 */
}

uint8_t I2C_Ping(uint8_t address) {
	
	error = 0x00;
	
	i2c_enable();
	i2c_tran();															/* set to transmit mode */
	i2c_m_start();													/* send start */
	i2c_send((uint8_t)(address << 1));  		/* send write address */
	i2c_wait();															/* wait for ack from slave */
	i2c_m_stop();														/* clear start mask */
	i2c_disable();
	
	return error;
}

uint8_t I2C_Write(uint8_t address, uint8_t data) {
	
	error = 0x00;
	
	i2c_enable();
	i2c_tran();															/* set to transmit mode */
	i2c_m_start();													/* send start */
	i2c_send((uint8_t)(address << 1));  		/* send write address */
	i2c_wait();															/* wait for ack from slave */
	i2c_send(data);													/* send data */
	i2c_wait();
	i2c_m_stop();														/* clear start mask */
	i2c_disable();
	
	return error;
}

uint8_t I2C_Read(uint8_t address, uint8_t* data) {
	
	error = 0x00;
	
	i2c_enable();
	i2c_tran();															/* set to transmit mode */
	i2c_m_start();													/* send start */
	i2c_send((uint8_t)(address << 1)|0x01); /* send read address */
	i2c_wait();															/* wait for ack from slave */
	i2c_rec();															/* set to receive mode */
	i2c_nack();
	dummy = i2c_recv();										  /* read data */
	i2c_wait();
	i2c_m_stop();														/* clear start mask */
	(*data) = i2c_recv();
	i2c_disable();
	
	return error;
}

uint8_t I2C_WriteReg(uint8_t address, uint8_t reg, uint8_t data) {
	
	error = 0x00;
	
	i2c_enable();
	i2c_tran();															/* set to transmit mode */
	i2c_m_start();													/* send start */
	i2c_send((uint8_t)(address << 1));  		/* send write address */
	i2c_wait();															/* wait for ack from slave */
	i2c_send(reg);													/* select register */
	i2c_wait();
	i2c_send(data);													/* send data */
	i2c_wait();
	i2c_m_stop();														/* clear start mask */
	i2c_disable();
	
	return error;
}

uint8_t I2C_ReadReg(uint8_t address, uint8_t reg, uint8_t* data) {
	
	error = 0x00;
	
	i2c_enable();
	i2c_clr_IICIF();
	i2c_tran();															/* set to transmit mode */
	i2c_m_start();													/* send start */
	i2c_send((uint8_t)(address << 1));      /* send write address */
	i2c_wait();															/* wait for ack from slave */
	i2c_send(reg);													/* select register */
	i2c_wait();
	
	i2c_m_rstart();

	i2c_send((uint8_t)(address << 1)|0x01); /* send read address */
	i2c_wait();
	
	i2c_rec();															/* set to receive mode */
	i2c_nack();															/* no acknowledge bit */
	dummy = i2c_recv();										  /* read data */
	i2c_wait();
	i2c_m_stop();														/* clear start mask */
	(*data) = i2c_recv();
	
	i2c_disable();
	
	return error;
}

uint8_t I2C_ReadRegBlock(uint8_t address, uint8_t reg, uint8_t size, uint8_t* data) {
	
	error = 0x00;
	uint8_t cnt = 0;
	
	i2c_enable();
	i2c_clr_IICIF();
	i2c_tran();															/* set to transmit mode */
	i2c_m_start();													/* send start */
	i2c_send((uint8_t)(address << 1));      /* send write address */
	i2c_wait();															/* wait for ack from slave */
	i2c_send(reg);													/* select register */
	i2c_wait();
	
	i2c_m_rstart();

	i2c_send((uint8_t)(address << 1)|0x01); /* send read address */
	i2c_wait();
	
	i2c_rec();															/* set to receive mode */
	
	i2c_ack();														/* transmit acknowledge bit */
	dummy = i2c_recv();										/* read data */
	
	while(cnt < (size-2)) {
		i2c_wait();	
		data[cnt++] = i2c_recv();
	}
	i2c_nack();	/* no acknowledge bit */
	i2c_wait();
	data[cnt++] = i2c_recv();
															
	i2c_wait();
	i2c_m_stop();														/* set start mask off */	
	data[cnt] = i2c_recv();

	i2c_disable();
	
	return error;
}
/**
 * @brief I2C master start.
 */
void i2c_m_start(void) {
  I2C0->C1 |= I2C_C1_MST_MASK;
}
/**
 * @brief I2C master stop.
 */
void i2c_m_stop(void) {
  I2C0->C1 &= ~I2C_C1_MST_MASK;
}
/**
 * @brief I2C master restart. (errata for Mask 2N96F)
 */
void i2c_m_rstart(void) {
	// WARNING! According to procesor errara, F register must be cleard
	// if MULT!=0 to perform restart correctly
	uint8_t tempF = I2C0->F;								/* save F register */
	I2C0->F   = 0;													/* clear F register */
  I2C0->C1 |= I2C_C1_RSTA_MASK;
	I2C0->F   = tempF;											/* restore F register */
}
/**
 * @brief I2C transmit.
 */
void i2c_tran(void) {
  I2C0->C1 |= I2C_C1_TX_MASK;
}
/**
 * @brief I2C receive.
 */
void i2c_rec(void) {
  I2C0->C1 &= ~I2C_C1_TX_MASK;
}
/**
 * @brief I2C enable.
 */
void i2c_enable(void) {
  I2C0->C1 |= I2C_C1_IICEN_MASK;
}
/**
 * @brief I2C disable.
 */
void i2c_disable(void) {
  I2C0->C1 &= ~I2C_C1_IICEN_MASK;
}
/**
 * @brief I2C send data.
 */
void i2c_send(uint8_t data) {
	I2C0->D = data;
}
/**
 * @brief I2C receive data.
 */
uint8_t i2c_recv(void) {
	return I2C0->D;
}
/**
 * @brief I2C wait.
 */
void i2c_wait(void) {
  timeout = 0;
	
  while ((((I2C0->S & I2C_S_IICIF_MASK)==0)||((I2C0->S & I2C_S_TCF_MASK)==0)) && (timeout<10000)) timeout++;
  if (timeout >= 10000) 						  error |= I2C_ERR_TIMEOUT; 
	if ((I2C0->S & I2C_S_RXAK_MASK)==1) error |= I2C_ERR_NOACK;
	i2c_clr_IICIF();
}
/**
 * @brief I2C transmit no acknowledge bit.
 */
void i2c_nack(void) {
  I2C0->C1 |= I2C_C1_TXAK_MASK;
}
/**
 * @brief I2C transmit acknowledge bit.
 */
void i2c_ack(void) {
  I2C0->C1 &= ~I2C_C1_TXAK_MASK;
}
void i2c_clr_IICIF(void) {
  I2C0->S |= I2C_S_IICIF_MASK;
}

