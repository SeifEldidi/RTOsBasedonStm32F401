/*
 * MCAL_SPI.c
 *
 *  Created on: Aug 27, 2023
 *      Author: Seif pc
 */
#include "../../Inc/MCAL/MCAL_SPI.h"

/*-------------Static Functions Declaration------*/
static SPI_ERRORS SPI_CLK_EN(__IO SPI_TypeDef *SPI);
static SPI_ERRORS SPI_GPIO_INIT(__IO SPI_TypeDef *SPI ,uint8_t Flag);
static SPI_ERRORS SPI_MSTR_SLAVE_Init(SPI_Config_t *SPI);
static void SPI_MSTRSEND_Helper(__IO SPI_TypeDef *SPI,uint8_t Data);
static void SPI_MSTRRec_Helper(__IO SPI_TypeDef *SPI,uint8_t *Data);
static void SPI_MSTRWAIT(__IO SPI_TypeDef *SPI);
static SPI_ERRORS SPI_GETErrors(__IO SPI_TypeDef *SPI);
static void SPI_InterruptsSet(__IO SPI_TypeDef *SPI);

#if SPI_INTERRUPTS_TX_RX_EN == EN
static SPI_IRQ_Handler SPI1_Handler;
static SPI_IRQ_Handler SPI2_Handler;
static SPI_IRQ_Handler SPI3_Handler;

Callback SPI1_TX;
Callback SPI1_RX;

Callback SPI2_TX;
Callback SPI2_RX;

Callback SPI3_TX;
Callback SPI3_RX;

static void SPI1_TX_SEND();
static void SPI2_TX_SEND();
static void SPI3_TX_SEND();

static void SPI1_RX_REC();
static void SPI2_RX_REC();
static void SPI3_RX_REC();
#endif

/*-------------Static Functions Definition------*/
static SPI_ERRORS SPI_CLK_EN(__IO SPI_TypeDef *SPI)
{
	SPI_ERRORS Err_State = SPI_E_OK;
	if (SPI != NULL) {
		/*---------Enable Clock To peripheral-----*/
		if(SPI == SPI1)
		{
			HAL_RCC_SPI1_EN();
		}else if(SPI == SPI2)
		{
			HAL_RCC_SPI2_EN();
		}else if(SPI == SPI3)
		{
			HAL_RCC_SPI3_EN();
		}else if(SPI == SPI4)
		{
			HAL_RCC_SPI4_EN();
		}else{
			Err_State = SPI_INCORRECT_REF;
		}
		/*------Disable SPI------*/
		CLEAR_MSK(SPI->CR1,MCAL_SPI_EN);
	} else {
		Err_State = SPI_NULLPTR;
	}
	return Err_State;
}

static SPI_ERRORS SPI_GPIO_INIT(__IO SPI_TypeDef *SPI ,uint8_t Flag)
{
	SPI_ERRORS Err_State = SPI_E_OK;
	if (SPI != NULL) {
		if(SPI == SPI1 || SPI == SPI2 ||SPI == SPI3 ||SPI == SPI4)
		{
			__IO GPIO_Typedef * GPIO_PORT     =  NULL;
			GPIO_PORT = (SPI == SPI1 )? GPIOA : GPIOB;
			__IO uint32_t *GPIO_AFR = (SPI == SPI1 )? &(GPIOA->AFRL) :(SPI == SPI2 )?&(GPIOB->AFRH):&(GPIOB->AFRL);
			uint32_t GPIO_AFR_SET = (SPI == SPI1 )? SPI1_AFR_SET :(SPI == SPI2 )?SPI2_AFR_SET:SPI3_AFR_SET;
			uint8_t SPIX_NSS	= (SPI == SPI1 )? GPIO_PIN_4 :(SPI == SPI2 )?GPIO_PIN_12:GPIO_PIN_15;
			uint8_t SPIX_SCK	= (SPI == SPI1 )? GPIO_PIN_5 :(SPI == SPI2 )?GPIO_PIN_13:GPIO_PIN_3;
			uint8_t SPIX_MISO	= (SPI == SPI1 )? GPIO_PIN_6 :(SPI == SPI2 )?GPIO_PIN_14:GPIO_PIN_4;
			uint8_t SPIX_MOSI	= (SPI == SPI1 )? GPIO_PIN_7 :(SPI == SPI2 )?GPIO_PIN_15:GPIO_PIN_5;
			GPIO_InitStruct SPIX_GPIO ={
					.Pin = SPIX_SCK|SPIX_MOSI|SPIX_MISO,
					.Speed = GPIO_Speed_50MHz,
					.mode = GPIO_MODE_AF_PP,
					.pull = GPIO_NOPULL,
			};
			HAL_GPIO_Init(GPIO_PORT , &SPIX_GPIO);
			SPIX_GPIO.Pin = SPIX_NSS;
			if(Flag == MCAL_SPI_MASTER_MODE || Flag == MCAL_SPI_MASTER_CRC_MODE)
				SPIX_GPIO.mode = GPIO_MODE_OUTPUT_PP;
			else
				SPIX_GPIO.mode = GPIO_MODE_INPUT;
			if(SPI == SPI3)
				HAL_GPIO_Init(GPIOA , &SPIX_GPIO);
			else
				HAL_GPIO_Init(GPIO_PORT , &SPIX_GPIO);
			*GPIO_AFR |= GPIO_AFR_SET;
		} else {
			Err_State = SPI_INCORRECT_REF;
		}
	} else {
		Err_State = SPI_NULLPTR;
	}
	return Err_State;
}

static void SPI_InterruptsSet(__IO SPI_TypeDef *SPI)
{
	if(SPI == SPI1)
		NVIC_EnableIRQ(SPI1_IRQ);
	else if(SPI == SPI2)
		NVIC_EnableIRQ(SPI2_IRQ);
	else if(SPI == SPI3)
		NVIC_EnableIRQ(SPI3_IRQ);
}

static SPI_ERRORS SPI_MSTR_SLAVE_Init(SPI_Config_t *SPI)
{
	SPI_ERRORS Err_State = SPI_E_OK;
	if (SPI != NULL)
	{
		if(SPI->Instance == SPI1 || SPI->Instance == SPI2 ||SPI->Instance == SPI3 ||SPI->Instance == SPI4)
		{
			uint16_t REG_CR1_Cpy = 0x0000UL;
//			uint16_t REG_CR2_Cpy = 0x0000UL;
			/*------Set BR-----------*/
			if(SPI->SPI_Mode == MCAL_SPI_MASTER_MODE || SPI->SPI_Mode == MCAL_SPI_MASTER_CRC_MODE )
			{
				CLEAR_MSK(REG_CR1_Cpy,FCLK_256);
				SET_MSK(REG_CR1_Cpy,(SPI->SPI_DivFactor<<BR));
				SET_MSK(REG_CR1_Cpy,MCAL_SPI_MSTR_EN|NSS_SOFTWARE|SSI_SET);
			}
			/*-------Set CPOL/CPHA--------*/
			uint8_t  SPI_CPOL = (SPI->SPI_CPOL == MCAL_SPI_CLKPOL_LOW )?MCAL_CPOL_LOW:
					(SPI->SPI_CPOL == MCAL_SPI_CLKPOL_HIGH )?MCAL_CPOL_HIGH:MCAL_CPOL_LOW;
			uint8_t  SPI_CPHA = (SPI->SPI_CPOL == MCAL_SPI_CLKPHA_FIRSTEDG )?MCAL_CPHA_FIRSTEDG:
					(SPI->SPI_CPOL == MCAL_SPI_CLKPHA_SECEDG )?MCAL_CPHA_SECEDG:MCAL_CPHA_FIRSTEDG;
			SET_MSK(REG_CR1_Cpy,SPI_CPOL|SPI_CPHA);
			/*------Set DFF----------*/
			CLEAR_MSK(REG_CR1_Cpy,MCAL_SPI_DFF_16BIT);
			/*------Set CRC-----------*/
			if(SPI->SPI_Mode == MCAL_SPI_MASTER_CRC_MODE || SPI->SPI_Mode == MCAL_SPI_SLAVE_CRC_MODE )
				SET_MSK(REG_CR1_Cpy,MCAL_SPI_CRCEN_EN);
			/*------Set DataDirection----*/
			CLEAR_MSK(REG_CR1_Cpy,MCAL_SPI_LSBFIRST);
			SET_MSK(REG_CR1_Cpy,(SPI->SPI_DataOrd<<LSBFIRST));
			/*------Set Interrupts-----*/
#if SPI_INTERRUPTS_TX_RX_EN == EN
			SPI_InterruptsSet(SPI->Instance);
#endif
			/*------Enable SPI------*/
			SET_MSK(REG_CR1_Cpy,MCAL_SPI_EN);
//			SPI->Instance->CR2 = REG_CR2_Cpy;
			SPI->Instance->CR1 = REG_CR1_Cpy;
			SPI_CS_DIS(SPI->Instance);
		}else{
			Err_State = SPI_INCORRECT_REF;
		}
	} else {
		Err_State = SPI_NULLPTR;
	}
	return Err_State;
}

static SPI_ERRORS SPI_GETErrors(__IO SPI_TypeDef *SPI)
{
	SPI_ERRORS Err_State = SPI_E_OK;
	if(SPI->SR & MODF_R)
		Err_State = SPI_MODEF_ERR;
	else if(SPI->SR & CRCERR_R)
		Err_State = SPI_CRC_ERR;
	return Err_State;
}

static void SPI_MSTRSEND_Helper(__IO SPI_TypeDef *SPI,uint8_t Data)
{
	while (!(SPI->SR & TXE_R));
	SPI->DR = Data;
}

static void SPI_MSTRWAIT(__IO SPI_TypeDef *SPI)
{
	uint16_t Temp = 0x0000;
	while (!(SPI->SR & TXE_R));
	while ((SPI->SR & BSY_R));
	Temp = SPI->DR;
	Temp = SPI->SR;
}

static void SPI_MSTRRec_Helper(__IO SPI_TypeDef *SPI,uint8_t *Data)
{
	while(!(SPI->SR & RXNE_R));
	*Data = SPI->DR;
}

#if SPI_INTERRUPTS_TX_RX_EN == EN
/*-----------SPI1 TX Callback--------*/
static void SPI1_TX_SEND()
{
	if(SPI1_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX] < SPI1_Handler.Message_Len[MCAL_SPI_HANDLER_TX])
	{
		SPI1->DR = *(SPI1_Handler.Buffer[MCAL_SPI_HANDLER_TX] + SPI1_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX]);
		SPI1_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX]++;
	}else if(SPI1_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX] == SPI1_Handler.Message_Len[MCAL_SPI_HANDLER_TX] )
	{
		SPI1_Handler.State[MCAL_SPI_HANDLER_TX] = SPI_JOB_DONE;
		/*-----Disable Interrupts----*/
		CLEAR_MSK(SPI1->CR2,TXIE_EN);
		SPI1_TX = NULL;
	}
}
/*-----------SPI2 TX Callback--------*/
static void SPI2_TX_SEND()
{
	if(SPI2_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX] < SPI2_Handler.Message_Len[MCAL_SPI_HANDLER_TX])
	{
		SPI2->DR = *(SPI2_Handler.Buffer[MCAL_SPI_HANDLER_TX] + SPI2_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX]);
		SPI2_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX]++;
	}else if(SPI2_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX] == SPI2_Handler.Message_Len[MCAL_SPI_HANDLER_TX] )
	{
		SPI2_Handler.State[MCAL_SPI_HANDLER_TX] = SPI_JOB_DONE;
		/*-----Disable Interrupts----*/
		CLEAR_MSK(SPI2->CR2,TXIE_EN);
		SPI2_TX = NULL;
	}
}
/*-----------SPI3 TX Callback--------*/
static void SPI3_TX_SEND()
{
	if(SPI3_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX] < SPI3_Handler.Message_Len[MCAL_SPI_HANDLER_TX])
	{
		SPI3->DR = *(SPI1_Handler.Buffer[MCAL_SPI_HANDLER_TX] + SPI3_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX]);
		SPI3_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX]++;
	}else if(SPI3_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX] == SPI3_Handler.Message_Len[MCAL_SPI_HANDLER_TX] )
	{
		SPI3_Handler.State[MCAL_SPI_HANDLER_TX] = SPI_JOB_DONE;
		/*-----Disable Interrupts----*/
		CLEAR_MSK(SPI3->CR2,TXIE_EN);
		SPI3_TX = NULL;
	}
}
/*-----------SPI1 RX Callback--------*/
static void SPI1_RX_REC()
{
	if(SPI1_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX] < SPI1_Handler.Message_Len[MCAL_SPI_HANDLER_RX])
	{
		*(SPI1_Handler.Buffer[MCAL_SPI_HANDLER_RX] + SPI1_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX]) = SPI1->DR;
		SPI1_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX]++;
	}else if(SPI1_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX] == SPI1_Handler.Message_Len[MCAL_SPI_HANDLER_RX] )
	{
		SPI1_Handler.State[MCAL_SPI_HANDLER_TX] = SPI_JOB_DONE;
		/*-----Disable Interrupts----*/
		CLEAR_MSK(SPI1->CR2,RXIE_EN);
		SPI1_RX = NULL;
	}
}

/*-----------SPI2 RX Callback--------*/
static void SPI2_RX_REC()
{
	if(SPI2_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX] < SPI2_Handler.Message_Len[MCAL_SPI_HANDLER_RX])
	{
		*(SPI2_Handler.Buffer[MCAL_SPI_HANDLER_RX] + SPI2_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX]) = SPI2->DR;
		SPI2_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX]++;
	}else if(SPI2_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX] == SPI2_Handler.Message_Len[MCAL_SPI_HANDLER_RX] )
	{
		SPI2_Handler.State[MCAL_SPI_HANDLER_TX] = SPI_JOB_DONE;
		/*-----Disable Interrupts----*/
		CLEAR_MSK(SPI2->CR2,RXIE_EN);
		SPI2_RX = NULL;
	}
}

/*-----------SPI3 RX Callback--------*/
static void SPI3_RX_REC()
{
	if(SPI3_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX] < SPI3_Handler.Message_Len[MCAL_SPI_HANDLER_RX])
	{
		*(SPI3_Handler.Buffer[MCAL_SPI_HANDLER_RX] + SPI3_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX]) = SPI3->DR;
		SPI3_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX]++;
	}else if(SPI3_Handler.TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX] == SPI3_Handler.Message_Len[MCAL_SPI_HANDLER_RX] )
	{
		SPI3_Handler.State[MCAL_SPI_HANDLER_TX] = SPI_JOB_DONE;
		/*-----Disable Interrupts----*/
		CLEAR_MSK(SPI3->CR2,RXIE_EN);
		SPI3_RX = NULL;
	}
}

#endif
/*-------------Global Function Definition-------*/
void SPI_CS_EN(__IO SPI_TypeDef *SPI)
{
	if(SPI == SPI1)
	{
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_4,GPIO_PIN_RESET);
	}else if(SPI == SPI2)
	{
		HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);
	}else if(SPI == SPI3)
	{
		HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET);
	}else{

	}
}

void SPI_CS_DIS(__IO SPI_TypeDef *SPI)
{
	if (SPI == SPI1)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
	} else if (SPI == SPI2)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
	} else if (SPI == SPI3)
	{
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
	} else {

	}
}

SPI_ERRORS MCAL_SPI_Init(SPI_Config_t *SPI)
{
	SPI_ERRORS Err_State = SPI_E_OK;
	if(SPI != NULL)
	{
		Err_State = SPI_GPIO_INIT(SPI->Instance,SPI->SPI_Mode);
		Err_State = SPI_CLK_EN(SPI->Instance);
		Err_State =	SPI_MSTR_SLAVE_Init(SPI);
		Err_State = SPI_GETErrors(SPI->Instance);
	}else{
		Err_State = SPI_NULLPTR;
	}
	return Err_State;
}

SPI_ERRORS MCAL_MstrSPI_Send(__IO SPI_TypeDef *SPI , uint8_t Data)
{
	SPI_ERRORS Err_State = SPI_E_OK;
	if (SPI != NULL) {
		SPI_CS_EN(SPI);
		SPI_MSTRSEND_Helper(SPI ,Data);
		/*------Wait Until SPI TXE is SET and BSY is Cleared---*/
		SPI_MSTRWAIT(SPI);
		/*-----Disable CS----*/
		SPI_CS_DIS(SPI);
	} else {
		Err_State = SPI_NULLPTR;
	}
	return Err_State;
}

SPI_ERRORS MCAL_MstrSPI_Receive(__IO SPI_TypeDef *SPI , uint8_t* Data)
{
	SPI_ERRORS Err_State = SPI_E_OK;
	if (SPI != NULL) {
		SPI_MSTRRec_Helper(SPI,Data);
	} else {
		Err_State = SPI_NULLPTR;
	}
	return Err_State;
}

SPI_ERRORS MCAL_MstrSPI_SendBuffer(__IO SPI_TypeDef *SPI , uint8_t* Data,uint8_t Buff_Len)
{
	SPI_ERRORS Err_State = SPI_E_OK;
	if (SPI != NULL) {
		uint8_t Byte_Tx = 0;
		/*-----Enable CS-----*/
		SPI_CS_EN(SPI);
		while(Byte_Tx < Buff_Len)
		{
			SPI_MSTRSEND_Helper(SPI,*(Data+Byte_Tx));
			Byte_Tx++;
		}
		SPI_MSTRWAIT(SPI);
		/*-----Disable CS----*/
		SPI_CS_DIS(SPI);
	} else {
		Err_State = SPI_NULLPTR;
	}
	return Err_State;
}

SPI_ERRORS MCAL_MstrSPI_ReceiveBuffer(__IO SPI_TypeDef *SPI , uint8_t* Data,uint8_t Buff_Len)
{
	SPI_ERRORS Err_State = SPI_E_OK;
	if (SPI != NULL) {
		/*-----May First Need to Send Device Address hence chip select will not be enabled ---*/
		uint8_t Byte_Tx = 0;
		while (Byte_Tx < Buff_Len) {
			SPI_MSTRRec_Helper(SPI, Data + Byte_Tx);
			Byte_Tx++;
		}
	} else {
		Err_State = SPI_NULLPTR;
	}
	return Err_State;
}

#if SPI_INTERRUPTS_TX_RX_EN == EN
void MCAL_MSTRSPITXCLR_Flag(__IO SPI_TypeDef *SPI)
{
	SPI_IRQ_Handler *SPIX_IRQ_handler = (SPI == SPI1)?&SPI1_Handler:(SPI == SPI2)?&SPI2_Handler:&SPI3_Handler;
	SPIX_IRQ_handler->Flag[MCAL_SPI_HANDLER_TX] = MCAL_SPI_TX_EMPTY;
}

void MCAL_MSTRSPIRXCLR_Flag(__IO SPI_TypeDef *SPI)
{
	SPI_IRQ_Handler *SPIX_IRQ_handler = (SPI == SPI1)?&SPI1_Handler:(SPI == SPI2)?&SPI2_Handler:&SPI3_Handler;
	SPIX_IRQ_handler->Flag[MCAL_SPI_HANDLER_RX] = MCAL_SPI_RX_EMPTY;
}

SPI_JOB_State MCAL_MstrSPI_SendInterrupts(__IO SPI_TypeDef *SPI , uint8_t Data)
{
	SPI_JOB_State SPIX_Status = SPI_JOB_ERR;
	if (SPI != NULL) {
		/*-----Select Handler----*/
		SPI_IRQ_Handler *SPIX_IRQ_handler = (SPI == SPI1)?&SPI1_Handler:(SPI == SPI2)?&SPI2_Handler:&SPI3_Handler;
		if(SPIX_IRQ_handler->Flag[MCAL_SPI_HANDLER_TX] == MCAL_SPI_TX_EMPTY)
		{
			Callback   SPIX_Function = (SPI == SPI1)?&SPI1_TX_SEND:(SPI == SPI2)?&SPI2_TX_SEND:&SPI3_TX_SEND;
			Callback * SPIX_Callback = (SPI == SPI1)?&SPI1_TX:(SPI == SPI2)?&SPI2_TX:&SPI3_TX;
			/*-------En ChipSelect---*/
			SPI_CS_EN(SPI);
			/*-------Set SPI Channel to be Busy-----*/
			SPIX_IRQ_handler->Flag[MCAL_SPI_HANDLER_TX]           = MCAL_SPI_TX_BUSY; /*---Set Flag to be Busy----*/
			SPIX_IRQ_handler->State[MCAL_SPI_HANDLER_TX]          = SPI_JOB_PROCESSING;
			SPIX_IRQ_handler->Buffer[MCAL_SPI_HANDLER_TX]         =  &Data;
			SPIX_IRQ_handler->Message_Len[MCAL_SPI_HANDLER_TX]    = 1;
			SPIX_IRQ_handler->TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX] = 0;
			/*-------Start Communication Enable Interrupts-----*/
			*SPIX_Callback = SPIX_Function;
			SPI->CR2 |= TXIE_EN;
		}else{
			SPIX_Status = SPIX_IRQ_handler->State[MCAL_SPI_HANDLER_TX];
		}
	} else {
		SPIX_Status = SPI_JOB_ERR;
	}
	return SPIX_Status;
}
/* Name   : SPI_JOB_State MCAL_MstrSPI_ReceiveInterrupts(__IO SPI_TypeDef *SPI , uint8_t* Data)
 * brief  : Software API to Recieve Data Over SPI By Interrupts
 *          the configuration FILE  macro must be enabled SPI_INTERRUPTS_TX_RX_EN
 *          Function Description : Recieve Byte thorugh SPI then Clear Flag through MCAL_MSTRSPIRXCLR_Flag() to allow function to receive again.
 * param  : takes pointer to SPI Instance
 * param  : pointer to data
 * return : Status of the JOB
 * 			SPI_JOB_DONE Data was sent/Recieved Succesfully
 * 			SPI_JOB_PROCESSING  Data is still being processed
 * 			SPI_JOB_ERR   an Err has occured
 */
SPI_JOB_State MCAL_MstrSPI_ReceiveInterrupts(__IO SPI_TypeDef *SPI , uint8_t* Data)
{
	SPI_JOB_State SPIX_Status = SPI_JOB_ERR;
	if (SPI != NULL) {
		/*-----Select Handler----*/
		SPI_IRQ_Handler *SPIX_IRQ_handler =	(SPI == SPI1) ? &SPI1_Handler :(SPI == SPI2) ? &SPI2_Handler : &SPI3_Handler;
		if (SPIX_IRQ_handler->Flag[MCAL_SPI_HANDLER_RX] == MCAL_SPI_RX_EMPTY) {
			/*-----Select Correct Callback----*/
			Callback SPIX_Function =(SPI == SPI1) ? &SPI1_RX_REC :(SPI == SPI2) ? &SPI2_RX_REC : &SPI3_RX_REC;
			Callback *SPIX_Callback = (SPI == SPI1) ? &SPI1_RX :(SPI == SPI2) ? &SPI2_RX : &SPI3_RX;
			/*-------Set SPI Channel to be Busy-----*/
			SPIX_IRQ_handler->State[MCAL_SPI_HANDLER_RX] = SPI_JOB_PROCESSING;
			SPIX_IRQ_handler->Buffer[MCAL_SPI_HANDLER_RX] = Data;
			SPIX_IRQ_handler->Message_Len[MCAL_SPI_HANDLER_RX] = 1;
			SPIX_IRQ_handler->TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX] = 0;
			/*-------Start Communication Enable Interrupts-----*/
			*SPIX_Callback = SPIX_Function;
			SPI->CR2 |= RXIE_EN;
		} else {
			SPIX_Status = SPIX_IRQ_handler->State[MCAL_SPI_HANDLER_TX];
		}
	} else {
		SPIX_Status = SPI_JOB_ERR;
	}
	return SPIX_Status;
}
/* Name   : SPI_JOB_State MCAL_MstrSPI_SendBufferInterrupts(__IO SPI_TypeDef *SPI , uint8_t* Data,uint8_t Buff_Len)
 * brief  : Software API to Send Data Over SPI By Interrupts
 *          the configuration FILE  macro must be enabled SPI_INTERRUPTS_TX_RX_EN
 *          to Send Data one must clear the flag through API MCAL_MSTRSPITXCLR_Flag() after the job is done .
 * param  : takes pointer to SPI Instance ,Data,buffer Len
 * return : Status of the JOB
 * 			SPI_JOB_DONE Data was sent/Recieved Succesfully
 * 			SPI_JOB_PROCESSING  Data is still being processed
 * 			SPI_JOB_ERR   an Err has occured
 */
SPI_JOB_State MCAL_MstrSPI_SendBufferInterrupts(__IO SPI_TypeDef *SPI , uint8_t* Data,uint8_t Buff_Len)
{
	SPI_JOB_State SPIX_Status = SPI_JOB_ERR;
	if (SPI != NULL)
	{
		/*-----Select Handler----*/
		SPI_IRQ_Handler *SPIX_IRQ_handler =(SPI == SPI1) ? &SPI1_Handler :(SPI == SPI2) ? &SPI2_Handler : &SPI3_Handler;
		if (SPIX_IRQ_handler->Flag[MCAL_SPI_HANDLER_TX] == MCAL_SPI_TX_EMPTY)
		{
			/*-----Select Correct Callback----*/
			Callback SPIX_Function =(SPI == SPI1) ? &SPI1_TX_SEND :(SPI == SPI2) ? &SPI2_TX_SEND : &SPI3_TX_SEND;
			Callback *SPIX_Callback = (SPI == SPI1) ? &SPI1_TX :(SPI == SPI2) ? &SPI2_TX : &SPI3_TX;
			/*--------Select SPI Chip-----*/
			SPI_CS_EN(SPI);
			/*-------Set SPI Channel to be Busy-----*/
			SPIX_IRQ_handler->Flag[MCAL_SPI_HANDLER_TX] = MCAL_SPI_TX_BUSY; /*---Set Flag to be Busy----*/
			SPIX_IRQ_handler->State[MCAL_SPI_HANDLER_TX] = SPI_JOB_PROCESSING;
			SPIX_IRQ_handler->Buffer[MCAL_SPI_HANDLER_TX] = Data;
			SPIX_IRQ_handler->Message_Len[MCAL_SPI_HANDLER_TX] = Buff_Len;
			SPIX_IRQ_handler->TX_RX_Curr_POS[MCAL_SPI_HANDLER_TX] = 0;
			/*-------Start Communication Enable Interrupts-----*/
			SPIX_Status = SPI_JOB_PROCESSING;
			*SPIX_Callback = SPIX_Function;
			SPI->CR2 |= TXIE_EN;
		} else
		{
			SPIX_Status = SPIX_IRQ_handler->State[MCAL_SPI_HANDLER_TX];
		}
	} else {
		SPIX_Status = SPI_JOB_ERR;
	}
	return SPIX_Status;
}
/* Name   : SPI_JOB_State MCAL_MstrSPI_ReceiveBufferInterrupts(__IO SPI_TypeDef *SPI , uint8_t* Data,uint8_t Buff_Len)
 * brief  : Software API to Recieve Data Over SPI By Interrupts
 *          the configuration FILE  macro must be enabled SPI_INTERRUPTS_TX_RX_EN
 *          Function Description : at First the SPI Data is recieved by interrupt processing untill the last byte is recieved in which the flag is not
 *          cleared and is left to be cleared by the user thorugh MCAL_MSTRSPIRXCLR_Flag(SPI); to allow data recption by interrupts again
 * param  : takes pointer to SPI Instance ,Data,buffer Len
 * return : Status of the JOB
 * 			SPI_JOB_DONE Data was sent/Recieved Succesfully
 * 			SPI_JOB_PROCESSING  Data is still being processed
 * 			SPI_JOB_ERR   an Err has occured
 */
SPI_JOB_State MCAL_MstrSPI_ReceiveBufferInterrupts(__IO SPI_TypeDef *SPI , uint8_t* Data,uint8_t Buff_Len)
{
	SPI_JOB_State SPIX_Status = SPI_JOB_ERR;
	if (SPI != NULL) {
		/*-----Select Handler----*/
		SPI_IRQ_Handler *SPIX_IRQ_handler =(SPI == SPI1) ? &SPI1_Handler :(SPI == SPI2) ? &SPI2_Handler : &SPI3_Handler;
		if (SPIX_IRQ_handler->Flag[MCAL_SPI_HANDLER_RX] == MCAL_SPI_RX_EMPTY) {
			/*-----Select Correct Callback----*/
			Callback SPIX_Function =(SPI == SPI1) ? &SPI1_RX_REC :(SPI == SPI2) ? &SPI2_RX_REC : &SPI3_RX_REC;
			Callback *SPIX_Callback = (SPI == SPI1) ? &SPI1_RX :(SPI == SPI2) ? &SPI2_RX : &SPI3_RX;
			/*-------Set SPI Channel to be Busy-----*/
			SPIX_IRQ_handler->State[MCAL_SPI_HANDLER_RX] = SPI_JOB_PROCESSING;
			SPIX_IRQ_handler->Buffer[MCAL_SPI_HANDLER_RX] = Data;
			SPIX_IRQ_handler->Message_Len[MCAL_SPI_HANDLER_RX] = Buff_Len;
			SPIX_IRQ_handler->TX_RX_Curr_POS[MCAL_SPI_HANDLER_RX] = 0;
			/*-------Start Communication Enable Interrupts-----*/
			*SPIX_Callback = SPIX_Function;
			SPI->CR2 |= RXIE_EN;
		} else {
			SPIX_Status = SPIX_IRQ_handler->State[MCAL_SPI_HANDLER_TX];
		}
	} else {
		SPIX_Status = SPI_JOB_ERR;
	}
	return SPIX_Status;
}
#endif


#if SPI_INTERRUPTS_TX_RX_EN == EN

void SPI1_IRQHandler()
{
	if(SPI1->SR & TXE_R)
	{
		if(SPI1_TX != NULL)
			SPI1_TX();
	}
	if(SPI1->SR & RXNE_R)
	{
		if(SPI1_RX != NULL)
			SPI1_RX();
	}
}

void SPI2_IRQHandler()
{
	if (SPI2->SR & TXE_R)
	{
		if (SPI2_TX != NULL)
			SPI2_TX();
	}
	if (SPI2->SR & RXNE_R)
	{
		if (SPI2_RX != NULL)
			SPI2_RX();
	}
}

void SPI3_IRQHandler()
{
	if (SPI3->SR & TXE_R)
	{
		if (SPI3_TX != NULL)
			SPI3_TX();
	}
	if (SPI3->SR & RXNE_R) {
		if (SPI3_RX != NULL)
			SPI3_RX();
	}
}


#endif
