/*
 * MCP2515CAN.h
 *
 *  Created on: Aug 29, 2023
 *      Author: Seif pc
 */

#ifndef HAL_MCP2515CAN_H_
#define HAL_MCP2515CAN_H_
/*-------------Includes Section Start---------------*/
#include <stdint.h>
#include "../std_macros.h"
#include "../Mcal/MCAL_SPI.h"
/*-------------Includes Section End----------------*/

/*------------Macro Declarations Start-----------*/
/*-----------TX Registers--------*/
#define TXBnCTRL_0		0x30
#define TXBnCTRL_1		0x40
#define TXBnCTRL_2		0x50

#define TXBnSIDH_0		0x31
#define TXBnSIDH_1		0x41
#define TXBnSIDH_2		0x51

#define TXBnSIDL_0		0x32
#define TXBnSIDL_1		0x42
#define TXBnSIDL_2		0x52

#define TXBnEID8_0		0x33
#define TXBnEID8_1		0x43
#define TXBnEID8_2		0x53

#define TXBnEID0_0		0x34
#define TXBnEID0_1		0x44
#define TXBnEID0_2		0x54

#define TXBnDLC_0		0x35
#define TXBnDLC_1		0x45
#define TXBnDLC_2		0x55

#define TXBnDm_0        0x36
#define TXBnDm_1        0x46
#define TXBnDm_2        0x56
/*-----------RX Registers--------*/
#define RXBnCTRL_0		0x60
#define RXBnCTRL_1		0x70

#define RXBnSIDH_0		0x61
#define RXBnSIDH_1		0x71

#define RXBnSIDL_0		0x62
#define RXBnSIDL_1		0x72

#define RXBnEID8_0		0x63
#define RXBnEID8_1		0x73

#define RXBnEID0_0		0x64
#define RXBnEID0_1		0x74

#define RXBnDLC_0		0x65
#define RXBnDLC_1		0x75

#define RXBnDm_0        0x76
#define RXBnDm_1        0x76
/*-----------TX Status Register-------*/
#define ABTF 		6U
#define ABTF_SET	(1<<ABTF)
#define ABTF_CLEAR	(~(ABTF_SET))

#define MLOA		5U
#define MLOA_READ	(1<<MLOA)

#define TXERR		4U
#define TXERR_DET	(1<<TXERR)

#define TXREQ		3U
#define TXREQ_READ	(1<<TXREQ)
#define TXREQ_SET	(TXREQ_READ)

#define TXP			0U
#define TXP_HIGH	(3<<TXP)
#define TXP_MED		(2<<TXP)
#define TXP_LOW		(1<<TXP)
#define TXP_LOWEST	(0<<TXP)
/*-----------DLC Register-------*/
#define RTR			6U
#define RTR_REMOTE	(1<<RTR)

#define TX_DLC_0	0U
#define TX_DLC_1	1U
#define TX_DLC_2	2U
#define TX_DLC_3	3U
#define TX_DLC_4	4U
#define TX_DLC_5	5U
#define TX_DLC_6	6U
#define TX_DLC_7	7U
#define TX_DLC_8	8U

/*------------Macro Declarations End-----------*/

/*-----------Function Macro Start----------------*/

/*-----------Function Macro End----------------*/

/*----------Defined Data types start-------------------*/

/*----------Defined Data types End----------------*/

/*-----------Software Interfaces start--------------*/

/*-----------Software Interfaces End--------------*/

#endif /* HAL_MCP2515CAN_H_ */
