/*
 * MCAL_SPI.h
 *
 *  Created on: Aug 27, 2023
 *      Author: Seif pc
 */

#ifndef MCAL_MCAL_SPI_H_
#define MCAL_MCAL_SPI_H_

/*-------------Includes Section Start---------------*/
#include <stdint.h>
#include "../std_macros.h"
#include "../HAL/Hal_RCC.h"
#include "../HAL/Hal_GPIO.h"
#include "../core/CortexM4_core_NVIC.h"
#include "Mcal_Spi_cfg.h"
/*-------------Includes Section End----------------*/
#define SPI1_OFFSET					0x00003000UL
#define SPI2_OFFSET					0x00003800UL
#define SPI3_OFFSET					0x00003C00UL
#define SPI4_OFFSET					0x00003400UL

#define SPI1						((__IO SPI_TypeDef*)(APB2_PERIPHBASE + SPI1_OFFSET))
#define SPI2						((__IO SPI_TypeDef*)(APB2_PERIPHBASE + SPI2_OFFSET))
#define SPI3						((__IO SPI_TypeDef*)(APB1_PERIPHBASE + SPI2_OFFSET))
#define SPI4						((__IO SPI_TypeDef*)(APB1_PERIPHBASE + SPI2_OFFSET))

/*------------Configuration Enums------*/
#define MCAL_SPI_MASTER_MODE			0x00U
#define MCAL_SPI_SLAVE_MODE				0x01U
#define MCAL_SPI_MASTER_CRC_MODE		0x02U
#define MCAL_SPI_SLAVE_CRC_MODE			0x03U

#define MCAL_SPI_CLKPOL_LOW				0x00U
#define MCAL_SPI_CLKPOL_HIGH			0x01U

#define MCAL_SPI_CLKPHA_FIRSTEDG		0x00U
#define MCAL_SPI_CLKPHA_SECEDG			0x01U

#define MCAL_SPI_BAUDRATE_2				0x00U
#define MCAL_SPI_BAUDRATE_4				0x01U
#define MCAL_SPI_BAUDRATE_8				0x02U
#define MCAL_SPI_BAUDRATE_16			0x03U
#define MCAL_SPI_BAUDRATE_32			0x04U
#define MCAL_SPI_BAUDRATE_64			0x05U
#define MCAL_SPI_BAUDRATE_128			0x06U
#define MCAL_SPI_BAUDRATE_256			0x07U

#define MCAL_SPI_DATA_DIR_LSB_FIRST		0x01U
#define MCAL_SPI_DATA_DIR_MSB_FIRST		0x00U

#define MCAL_SPI_HANDLER_TX				0x00U
#define MCAL_SPI_HANDLER_RX				0x01U

#define MCAL_SPI_TX_BUSY				0x01U
#define MCAL_SPI_TX_EMPTY				0x00U

#define MCAL_SPI_RX_BUSY				0x01U
#define MCAL_SPI_RX_EMPTY				0x00U
/*---------SPI_BIT_POS-----------*/
/*******************  Bit definition for SPI_CR1 register  ********************/
#define CRCEN							13U
#define MCAL_SPI_CRCEN_EN				(1<<CRCEN)
#define MCAL_SPI_CRCEN_DIS				(~(MCAL_SPI_CRCEN_EN))

#define CRCNEXT							12U
#define MCAL_SPI_CRCNEXT_EN				(1<<CRCNEXT)
#define MCAL_SPI_CRCNEXT_DIS			(~(MCAL_SPI_CRCNEXT_EN))

#define DFF								11U
#define MCAL_SPI_DFF_16BIT				(1<<DFF)
#define MCAL_SPI_DFF_8BIT				(~(MCAL_SPI_DFF_16BIT))

#define SSM								9U
#define NSS_SOFTWARE					(1<<SSM)

#define SSI								8U
#define SSI_SET							(1<<8U)

#define LSBFIRST						7U
#define MCAL_SPI_LSBFIRST				(1<<LSBFIRST)
#define MCAL_SPI_MSBFIRST				(~(MCAL_SPI_LSBFIRST))

#define SPE								6U
#define MCAL_SPI_EN						(1<<SPE)
#define MCAL_SPI_DIS					(~(MCAL_SPI_EN))

#define BR								3U
#define FCLK_2							(0<<BR)
#define FCLK_4							(1<<BR)
#define FCLK_8							(2<<BR)
#define FCLK_16							(3<<BR)
#define FCLK_32							(4<<BR)
#define FCLK_64							(5<<BR)
#define FCLK_128						(6<<BR)
#define FCLK_256						(7<<BR)

#define MSTR							2U
#define MCAL_SPI_MSTR_EN				(1<<MSTR)

#define SPI_CPOL_POS					1U
#define MCAL_CPOL_LOW					(0<<SPI_CPOL_POS)
#define MCAL_CPOL_HIGH					(1<<SPI_CPOL_POS)

#define SPI_CPHA_POS					0U
#define MCAL_CPHA_FIRSTEDG				(0<<SPI_CPHA_POS)
#define MCAL_CPHA_SECEDG				(1<<SPI_CPHA_POS)
/*******************  Bit definition for SPI_CR2 register  ********************/
#define TXIE							7U
#define TXIE_EN							(1<<TXIE)

#define RXIE							6U
#define RXIE_EN							(1<<RXIE)

#define SPI_ERRIE						5U
#define ERRIE_EN						(1<<SPI_ERRIE)

#define SSOE							2U
#define SSOE_EN							(1<<SSOE)

#define TXDMAEN							1U
#define TXDMAEN_EN						(1<<TXDMAEN)

#define RXDMAEN							0U
#define RXDMAEN_EN						(1<<RXDMAEN)
/********************  Bit definition for SPI_SR register  ********************/
#define FRE								8U
#define FRE_R							(1<<FRE)

#define SPI_BSY							7U
#define BSY_R							(1<<SPI_BSY)

#define SPI_OVR							6U
#define OVR_R							(1<<SPI_OVR)

#define MODF							5U
#define MODF_R							(1<<MODF)

#define CRCERR							4U
#define CRCERR_R						(1<<CRCERR)

#define UDR								3U
#define UDR_R							(1<<UDR)

#define CHSIDE							2U
#define CHSIDE_R						(1<<CHSIDE)

#define SPI_TXE							1U
#define TXE_R							(1<<SPI_TXE)

#define SPI_RXNE						0U
#define RXNE_R							(1<<SPI_RXNE)

#define SPI1_AFR_SET					(0x05<<20|0x05<<24|0x05<<28)
#define SPI2_AFR_SET					(0x05<<20|0x05<<24|0x05<<28)
#define SPI3_AFR_SET				    (0x06<<16|0x06<<20|0x06<<12)
/*------------Macro Declarations Start-----------*/

/*------------Macro Declarations End-----------*/

/*-----------Function Macro Start----------------*/

/*-----------Function Macro End----------------*/

/*----------Defined Data types start-------------------*/
typedef struct
{
  __IO uint32_t CR1;        /*!< SPI control register 1 (not used in I2S mode),      Address offset: 0x00 */
  __IO uint32_t CR2;        /*!< SPI control register 2,                             Address offset: 0x04 */
  __IO uint32_t SR;         /*!< SPI status register,                                Address offset: 0x08 */
  __IO uint32_t DR;         /*!< SPI data register,                                  Address offset: 0x0C */
  __IO uint32_t CRCPR;      /*!< SPI CRC polynomial register (not used in I2S mode), Address offset: 0x10 */
  __IO uint32_t RXCRCR;     /*!< SPI RX CRC register (not used in I2S mode),         Address offset: 0x14 */
  __IO uint32_t TXCRCR;     /*!< SPI TX CRC register (not used in I2S mode),         Address offset: 0x18 */
  __IO uint32_t I2SCFGR;    /*!< SPI_I2S configuration register,                     Address offset: 0x1C */
  __IO uint32_t I2SPR;      /*!< SPI_I2S prescaler register,                         Address offset: 0x20 */
} SPI_TypeDef;

typedef void(*Callback)(void);

typedef struct
{
	__IO SPI_TypeDef * Instance;
	uint8_t SPI_Mode;
	uint8_t SPI_CPOL:1;
	uint8_t	SPI_CPHA:1;
	uint8_t SPI_DivFactor:3;
	uint8_t SPI_DataOrd:1;
	uint8_t SPI_Res:2;
}SPI_Config_t;

typedef enum
{
	SPI_JOB_IDLE,
	SPI_JOB_DONE,
	SPI_JOB_PROCESSING,
	SPI_JOB_ERR,
}SPI_JOB_State;

typedef struct
{
	uint8_t *Buffer[2];
	uint8_t  Message_Len[2];
	uint8_t  TX_RX_Curr_POS[2];
	SPI_JOB_State State[2];
	uint8_t  Flag[2];
}SPI_IRQ_Handler;

typedef enum
{
	SPI_NULLPTR,
	SPI_INCORRECT_REF,
	SPI_MODEF_ERR,
	SPI_CRC_ERR,
	SPI_E_OK,
}SPI_ERRORS;

/*----------Defined Data types End----------------*/
/*-----------Software Interfaces start--------------*/
void SPI_CS_EN(__IO SPI_TypeDef *SPI);
void SPI_CS_DIS(__IO SPI_TypeDef *SPI);
SPI_ERRORS MCAL_SPI_Init(SPI_Config_t *SPI);
SPI_ERRORS MCAL_MstrSPI_Send(__IO SPI_TypeDef *SPI , uint8_t Data);
SPI_ERRORS MCAL_MstrSPI_Receive(__IO SPI_TypeDef *SPI , uint8_t* Data);
SPI_ERRORS MCAL_MstrSPI_SendBuffer(__IO SPI_TypeDef *SPI , uint8_t* Data,uint8_t Buff_Len);
SPI_ERRORS MCAL_MstrSPI_ReceiveBuffer(__IO SPI_TypeDef *SPI , uint8_t* Data,uint8_t Buff_Len);
#if SPI_INTERRUPTS_TX_RX_EN == EN
void MCAL_MSTRSPITXCLR_Flag(__IO SPI_TypeDef *SPI);
void MCAL_MSTRSPIRXCLR_Flag(__IO SPI_TypeDef *SPI);
SPI_JOB_State MCAL_MstrSPI_SendInterrupts(__IO SPI_TypeDef *SPI , uint8_t Data);
SPI_JOB_State MCAL_MstrSPI_ReceiveInterrupts(__IO SPI_TypeDef *SPI , uint8_t* Data);
SPI_JOB_State MCAL_MstrSPI_SendBufferInterrupts(__IO SPI_TypeDef *SPI , uint8_t* Data,uint8_t Buff_Len);
SPI_JOB_State MCAL_MstrSPI_ReceiveBufferInterrupts(__IO SPI_TypeDef *SPI , uint8_t* Data,uint8_t Buff_Len);
#endif
/*-----------Software Interfaces End--------------*/


#endif /* MCAL_MCAL_SPI_H_ */
