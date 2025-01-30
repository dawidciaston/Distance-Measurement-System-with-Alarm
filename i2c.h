
#ifndef I2C_H
#define I2C_H

#include "frdm_bsp.h"

/******************************************************************************\
* Global definitions
\******************************************************************************/
#define I2C_ERR_TIMEOUT		0x01 		/* error = timeout */
#define I2C_ERR_NOACK			0x02 		/* error = no ACK from slave  */
/**
 * @brief I2C initialization.
 */
void I2C_Init(void);
/**
 * @brief Send via I2C only device address (write). In response check error type.
 *
 * @param Address of slave.
 * @return Errors.
 */
uint8_t I2C_Ping(uint8_t address);
/**
 * @brief I2C basic write operation. Write 8 bits to specified device address.
 * 				Works best with I/O expanders.
 *
 * @param Address of slave.
 * @param Data to write.
 * @return Errors.
 */
uint8_t I2C_Write(uint8_t address, uint8_t data);
/**
 * @brief I2C basic read operation. Read 8 bits from specified device address.
 *				Works best with I/O expanders.
 *
 * @param Address of slave.
 * @param Data from slave.
 * @return Errors.
 */
uint8_t I2C_Read(uint8_t address, uint8_t* data);
/**
 * @brief I2C write to register.
 *
 * @param Address of slave.
 * @param Register.
 * @param Data to write.
 * @return Errors.
 */
uint8_t I2C_WriteReg(uint8_t address, uint8_t reg, uint8_t data);
/**
 * @brief I2C read from register.
 *
 * @param Address of slave.
 * @param Register.
 * @param Data from slave device.
 * @return Errors.
 */
uint8_t I2C_ReadReg(uint8_t address, uint8_t reg, uint8_t* data);
/**
 * @brief I2C read from block of registers (autoincrementation).
 *
 * @param Address of slave.
 * @param Start register.
 * @param Count of registers to read.
 * @param Data from slave device.
 * @return Errors.
 */
uint8_t I2C_ReadRegBlock(uint8_t address, uint8_t reg, uint8_t size, uint8_t* data);

#endif /* I2C_H */
