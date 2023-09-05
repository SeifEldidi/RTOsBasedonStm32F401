/*
 * Mcal_I2C.c
 *
 *  Created on: Aug 14, 2023
 *      Author: Seif pc
 */
#include "../../Inc/Mcal/Mcal_I2C.h"
/*-------------Static Functions Declaration----------------*/
static ERR xMcal_I2C_Slave_Init(__IO  I2C_TypeDef *Inst,uint8_t Flag ,uint16_t Add);
static ERR xMcal_I2C_Clock_Init(__IO  I2C_TypeDef *Inst);
static uint32_t xMcal_I2C_CR2_Init(__IO  I2C_TypeDef *Inst);
static ERR xMcal_I2C_Freq_Init(__IO  I2C_TypeDef *Inst,uint8_t F_S_M,uint8_t Duty);
static ERR xMcal_I2C_Trise_Init(__IO  I2C_TypeDef *Inst,uint8_t F_S_M);
static ERR xMcal_I2C_MSTRDuty_Init(__IO  I2C_TypeDef *Inst,uint8_t Duty,uint32_t Freq);
static void vMcal_I2C_PinInit(__IO  I2C_TypeDef *Inst);

#if I2C_INT_EN
static ERR xMcal_I2C_IntInit();
#if I2C1_INT_EN == EN
static void I2C_EV_IRQHandling( I2C_IRQ_handler * I2C);
static void I2C_ERR_IRQHandling(I2C_IRQ_handler * I2C);
#endif
#endif
/*-------------Static Variables DeFintions----------------*/
#if I2C_INT_EN
I2C_IRQ_handler I2C1_Handler;
I2C_IRQ_handler I2C2_Handler;
I2C_IRQ_handler I2C3_Handler;

I2C_Callback TX = NULL;
I2C_Callback RX = NULL;
#endif
/*-------------Software Interfaces Defintions----------------*/
ERR xMCAL_I2C_Init(I2C_cfg_t * I2C)
{
	ERR I2C_State = HAL_OK;
	if(I2C != NULL)
	{
		/*-------GPIO Init------------*/
		vMcal_I2C_PinInit(I2C->Instance);
		/*-------Enable Clock Access to I2C------*/
		xMcal_I2C_Clock_Init(I2C->Instance);
		/*-------Reset I2C--------------------*/
		I2C->Instance->CR1 |= SWRST_RST;
		I2C->Instance->CR1 &= ~SWRST_RST;

		while(I2C->Instance->CR1 & SWRST_RST);
		/*--------Check whether I2C is slave or master-----*/
		if( I2C->SL_MSTR_MODE == I2C_SLAVE_MODE)
		{
			I2C_State = xMcal_I2C_Slave_Init(I2C->Instance,I2C->SL_SIZE,I2C->I2C_SLAVE_ADD);
		}else {}
		I2C_State = xMcal_I2C_Freq_Init(I2C->Instance, I2C->SM_FM_SEL,I2C->FM_DUTY_SEL);
		I2C_State = xMcal_I2C_Trise_Init(I2C->Instance, I2C->SM_FM_SEL);
		/*-----Enable Interrupts---*/
#if I2C_INT_EN == EN
			I2C_State = xMcal_I2C_IntInit(I2C->Instance);
#endif
		/*-----Enable Perripheral---*/
		I2C->Instance->CR1 |= I2C_EN;
		I2C->Instance->CR1 |= ACK_EN;
	}else{
		I2C_State = HAL_ERR;
	}
	return I2C_State;
}

ERR  xMCAL_I2C_Finder(__IO I2C_TypeDef * I2C ,uint8_t *Data)
{
	uint16_t Dummy = 0x0000;
	uint8_t Pos =0;
	uint32_t I2CADD = 1;
	uint32_t TimeOut = 0;
	ERR I2C_State = HAL_OK;
	if (I2C != NULL && Data != NULL) {
		for(I2CADD = 1 ;I2CADD<= 127;I2CADD++)
		{
			CLEAR_BIT(I2C->SR1,AF);
			I2C_State = HAL_OK;
			TimeOut = 0 ;
			/*------Wait for Bus to be Empty-----*/
			while (I2C->SR2 & I2C_BUS_BUSY);
			/*--------Generate Start Bit------*/
			I2C->CR1 |= START_EN;
			/*--------Wait Untill Start Is Generated----*/
			while (!(I2C->SR1 & I2C_SB));
			/*-------Send Address-----------*/
			I2C->DR = (((I2CADD << 1)) | I2C_WRITE);
			while (!(I2C->SR1 & I2C_ADDR))
			{
				TimeOut++;
				if(TimeOut > 50000UL)
				{
					I2C_State = HAL_ERR;
					break;
				}
			}
			if(I2C_State == HAL_OK)
			{
				Dummy = I2C->SR2;
				I2C->CR1 |= STOP_EN;
				Data[Pos++] = I2CADD;
			}else{
				I2C->CR1 |= STOP_EN;
			}
		}
	}
	return I2C_State;
}

ERR  xMCAL_I2C_MSTRSend(__IO I2C_TypeDef * I2C , uint8_t SA,uint8_t Reg_Addr,uint8_t *Data,uint8_t Data_Len)
{
	uint16_t Dummy = 0x0000;
	uint32_t TimeOut = 0;
	ERR I2C_State = HAL_OK;
	CLEAR_BIT(I2C->SR1,AF);
	if(I2C != NULL && Data !=NULL)
	{
		uint32_t Byte_TX = 0;
		/*------Wait for Bus to be Empty-----*/
		while (I2C->SR2 & I2C_BUS_BUSY);
		/*--------Generate Start Bit------*/
		I2C->CR1 |= START_EN;
		/*--------Wait Untill Start Is Generated----*/
		while (!(I2C->SR1 & I2C_SB));
		/*-------Send Address-----------*/
		I2C->DR = (((SA << 1)) | I2C_WRITE);
		while (!(I2C->SR1 & I2C_ADDR))
		{
			TimeOut++;
			if (TimeOut > 50000UL)
			{
				I2C_State = HAL_ERR;
				break;
			}
		}
		if(I2C_State == HAL_OK)
		{
			Dummy = I2C->SR2;
			while (!(I2C->SR1 & TxE_SET));
			/*--------Send Data------*/
			I2C->DR = Reg_Addr;
			/*--------Wait for ack and TXE----*/
			for (Byte_TX = 0; Byte_TX <= Data_Len - 1; Byte_TX++) {
				while (!(I2C->SR1 & TxE_SET));
				I2C->DR = *(Data + Byte_TX);
			}
			while (!(I2C->SR1 & TxE_SET));
			while (!(I2C->SR1 & BTF_SET));
			I2C->CR1 |= STOP_EN;
		}else{
			I2C->CR1 |= STOP_EN;
		}
	}
	return I2C_State;
}

ERR  xMCAL_I2C_SLAVESend(__IO  I2C_TypeDef * I2C , uint8_t *Data,uint8_t Data_Len)
{
	uint16_t Dummy = 0x0000;
	uint32_t TimeOut = 0;
	int32_t Byte_TX = 0;
	ERR I2C_State = HAL_OK;
	if(I2C != NULL && Data !=NULL)
	{
		/*--------polling Method----------*/
		/*--------Wait For Address Reception-----*/
		while (!(I2C->SR1 & I2C_ADDR))
		{
			TimeOut++;
			if (TimeOut > 50000UL) {
				I2C_State = HAL_ERR;
				break;
			}
		}
		if(I2C_State == HAL_OK)
		{
			/*---Clear ADDR Flag----*/
			Dummy = I2C->SR2;
			for (Byte_TX = 0; Byte_TX <= Data_Len - 1; Byte_TX++) {
				while (!(I2C->SR1 & TxE_SET));
				I2C->DR = *(Data + Byte_TX);
			}
			while(!(I2C->SR1 & AF_SET));
			CLEAR_MSK(I2C->SR1,AF_SET);
		}
	}
	return I2C_State;
}
ERR  xMCAL_I2C_SLAVERecieve(__IO  I2C_TypeDef * I2C , uint8_t *Data,uint8_t Data_Len)
{
	uint16_t Dummy = 0x0000;
	uint32_t TimeOut = 0;
	uint32_t Byte_TX = 0;
	uint8_t Dummy_CR1 = I2C->CR1;
	ERR I2C_State = HAL_OK;
	I2C->CR1 |= ACK_EN;
	if (I2C != NULL && Data != NULL) {
		/*--------polling Method----------*/
		/*--------Wait For Address Reception-----*/
		while (!(I2C->SR1 & I2C_ADDR)) {
			TimeOut++;
			if (TimeOut > 50000UL) {
				I2C_State = HAL_ERR;
				break;
			}
		}
		if (I2C_State == HAL_OK) {
			/*---Clear ADDR Flag----*/
			Dummy = I2C->SR2;
			for (Byte_TX = 0; Byte_TX <= Data_Len - 1; Byte_TX++) {
				while (!(I2C->SR1 & RxNE_SET));
				*(Data + Byte_TX) = I2C->DR;
			}
			while (!(I2C->SR1 & I2C_STOPF));
			Dummy = I2C->SR1;
			I2C->CR1 = Dummy_CR1;
		}
	}
	return I2C_State;
}

ERR  xMCAL_I2C_MSTRRec (__IO I2C_TypeDef * I2C , uint8_t SA,uint8_t Reg_Addr,uint8_t *Data,uint8_t Data_Len)
{
	uint16_t Dummy = 0x0000;
	ERR I2C_State = HAL_OK;
	int32_t Byte_RX = 0;
	if (I2C != NULL && Data != NULL) {
		I2C->CR1 |= ACK_EN;
		/*------Wait for Bus to be Empty-----*/
		while (I2C->SR2 & I2C_BUS_BUSY);
		/*--------Generate Start Bit------*/
		I2C->CR1 |= START_EN;
		/*--------Wait Untill Start Is Generated----*/
		while (!(I2C->SR1 & I2C_SB));
		/*-------Send Address-----------*/
		I2C->DR = (((SA << 1)) | I2C_WRITE);
		while (!(I2C->SR1 & I2C_ADDR));
		Dummy = I2C->SR2;
		while (!(I2C->SR1 & TxE_SET));
		/*--------Send Register Address------*/
		I2C->DR = Reg_Addr;
		while (!(I2C->SR1 & TxE_SET));
		/*---After Acknowledgment Send Sr----*/
		/*--------Generate Repeated Start Bit------*/
		I2C->CR1 |= START_EN;
		while (!(I2C->SR1 & I2C_SB));
		/*-------Send Address-----------*/
		I2C->DR = (((SA << 1)) | I2C_READ);
		while (!(I2C->SR1 & I2C_ADDR));
		Dummy = I2C->SR2;
		/*------Recieve Data -------*/
		if(Data_Len == 1)
		{
			I2C->CR1 &= ACK_DIS;
			while (!(I2C->SR1 & RxNE_SET));
			*(Data + Byte_RX) = I2C->DR ;
			I2C1->CR1 |= STOP_EN;
		}else{
			for (Byte_RX = 0; Byte_RX <=Data_Len-1 ; Byte_RX++) {
				while (!(I2C->SR1 & RxNE_SET));
				if(Byte_RX == Data_Len-2)
					/*---No Acknowledgment---*/
					I2C->CR1 &= ACK_DIS;
				else if(Byte_RX == Data_Len-1)
					I2C1->CR1 |= STOP_EN;
				*(Data + Byte_RX) = I2C->DR ;
			}
		}
	} else {
		I2C_State = HAL_ERR;
	}
	return I2C_State;
}


#if I2C_INT_EN

ERR  xMCAL_I2C_MSTRIntSend(__IO I2C_TypeDef * I2C , uint8_t SA,uint8_t *Data,uint8_t Data_Len,I2C_Callback Func)
{
	ERR Err_State = HAL_OK;
	if(I2C !=NULL)
	{
		I2C_IRQ_handler *I2CX_IRQ_handler = (I2C == I2C1 )?&I2C1_Handler:(I2C == I2C2 )?&I2C2_Handler:&I2C3_Handler;
		/*------------------HalfDuplex I2C-------------*/
		if(I2CX_IRQ_handler->I2C_BUSY_FLAG != I2C_BUSY_TX && I2CX_IRQ_handler->I2C_BUSY_FLAG != I2C_BUSY_RX)
		{
			/*--------Send Data To handler-------*/
			I2CX_IRQ_handler->Instance    	 = I2C;
			I2CX_IRQ_handler->Data_buffer[TX_POS]    = Data;
			I2CX_IRQ_handler->I2C_ADD_BUFFER         = SA;
			I2CX_IRQ_handler->I2C_DATA_LEN[TX_POS]   =   Data_Len;
			I2CX_IRQ_handler->I2C_BUSY_FLAG  =   I2C_BUSY_TX;
			I2CX_IRQ_handler->I2C_DATA_POS[TX_POS]   =   0;
			TX = Func;
			/*--------Start I2C Transmission-----*/
			I2C->CR1 |= START_EN;
			/*--------Enable All Event interrupts----*/
			I2C->CR2 |= ITEVTEN_EN |ITBUFEN_EN  |ITERREN_EN;
		}
	}else{
		Err_State = HAL_ERR;
	}
	return Err_State;
}

ERR  xMCAL_I2C_MSTRIntRec(__IO  I2C_TypeDef * I2C , uint8_t SA,uint8_t *Data,uint8_t Data_Len,I2C_Callback Func)
{
	ERR Err_State = HAL_OK;
	if (I2C != NULL) {
		I2C_IRQ_handler *I2CX_IRQ_handler =(I2C == I2C1) ? &I2C1_Handler :(I2C == I2C2) ? &I2C2_Handler : &I2C3_Handler;
		/*------------------HalfDuplex I2C-------------*/
		if (I2CX_IRQ_handler->I2C_BUSY_FLAG != I2C_BUSY_TX && I2CX_IRQ_handler->I2C_BUSY_FLAG != I2C_BUSY_RX) {
			/*--------Send Data To handler-------*/
			I2C->CR1 |= ACK_EN;
			I2CX_IRQ_handler->Instance = I2C;
			I2CX_IRQ_handler->Data_buffer[RX_POS] = Data;
			I2CX_IRQ_handler->I2C_ADD_BUFFER = SA;
			I2CX_IRQ_handler->I2C_DATA_LEN[RX_POS] = Data_Len;
			I2CX_IRQ_handler->I2C_BUSY_FLAG = I2C_BUSY_RX;
			I2CX_IRQ_handler->I2C_DATA_POS[RX_POS] = 0;
			RX = Func;
			/*--------Start I2C Transmission-----*/
			I2C->CR1 |= START_EN;
			/*--------Enable All Event interrupts----*/
			I2C->CR2 |= ITEVTEN_EN|ITBUFEN_EN|ITERREN_EN;
		}
	}
	return Err_State;
}

ERR  xMCAL_I2C_SLAVEIntSend(__IO  I2C_TypeDef * I2C , uint8_t *Data,uint8_t Data_Len,I2C_Callback Func)
{
	ERR Err_State = HAL_OK;
	if (I2C != NULL) {
		I2C_IRQ_handler *I2CX_IRQ_handler =(I2C == I2C1) ? &I2C1_Handler :(I2C == I2C2) ? &I2C2_Handler : &I2C3_Handler;
		/*------------------HalfDuplex I2C-------------*/
		if (I2CX_IRQ_handler->I2C_BUSY_FLAG != I2C_BUSY_TX && I2CX_IRQ_handler->I2C_BUSY_FLAG != I2C_BUSY_RX) {
			/*--------Send Data To handler-------*/
			I2C->CR1 |= ACK_EN;
			I2CX_IRQ_handler->Instance = I2C;
			I2CX_IRQ_handler->Data_buffer[TX_POS] = Data;
			I2CX_IRQ_handler->I2C_DATA_LEN[TX_POS] = Data_Len;
			I2CX_IRQ_handler->I2C_BUSY_FLAG = I2C_BUSY_TX;
			I2CX_IRQ_handler->I2C_DATA_POS[TX_POS] = 0;
			TX = Func;
			/*--------Enable All Event interrupts----*/
			I2C->CR2 |= ITEVTEN_EN | ITBUFEN_EN | ITERREN_EN;
		}
	}
	return Err_State;
}

ERR  xMCAL_I2C_SLAVEIntRecieve(__IO  I2C_TypeDef * I2C , uint8_t *Data,uint8_t Data_Len,I2C_Callback Func)
{
	ERR Err_State = HAL_OK;
	if (I2C != NULL) {
		I2C_IRQ_handler *I2CX_IRQ_handler =(I2C == I2C1) ? &I2C1_Handler :(I2C == I2C2) ? &I2C2_Handler : &I2C3_Handler;
		/*------------------HalfDuplex I2C-------------*/
		if (I2CX_IRQ_handler->I2C_BUSY_FLAG != I2C_BUSY_TX && I2CX_IRQ_handler->I2C_BUSY_FLAG != I2C_BUSY_RX) {
			/*--------Send Data To handler-------*/
			I2C->CR1 |= ACK_EN;
			I2CX_IRQ_handler->Instance = I2C;
			I2CX_IRQ_handler->Data_buffer[RX_POS] = Data;
			I2CX_IRQ_handler->I2C_DATA_LEN[RX_POS] = Data_Len;
			I2CX_IRQ_handler->I2C_BUSY_FLAG = I2C_BUSY_RX;
			I2CX_IRQ_handler->I2C_DATA_POS[RX_POS] = 0;
			RX = Func;
			/*--------Enable All Event interrupts----*/
			I2C->CR2 |= ITEVTEN_EN | ITBUFEN_EN | ITERREN_EN;
		}
	}
	return Err_State;
}

#endif


/*-------------Static Functions Defintions----------------*/
#if I2C1_INT_EN == EN

static void I2C_HANDLERRXEvent(I2C_IRQ_handler * I2C)
{
	if (I2C->I2C_BUSY_FLAG == I2C_BUSY_RX) {
		if (I2C->I2C_DATA_LEN[RX_POS] == 1) {
			if (I2C->I2C_DATA_POS[RX_POS] < I2C->I2C_DATA_LEN[RX_POS])
			{
				*(I2C->Data_buffer[RX_POS] + I2C->I2C_DATA_POS[RX_POS]) =I2C->Instance->DR;
				I2C->I2C_DATA_POS[RX_POS]++;
				I2C->Instance->CR1 |= STOP_EN;
			}
		} else if (I2C->I2C_DATA_LEN[RX_POS] > 1) {

			if (I2C->I2C_DATA_POS[RX_POS] < I2C->I2C_DATA_LEN[RX_POS] - 1) {
				/*-------Set Ack and Stop En-----*/
				if (I2C->I2C_DATA_POS[RX_POS] == I2C->I2C_DATA_LEN[RX_POS] - 2)
					I2C->Instance->CR1 &= ACK_DIS;
				/*----Read Data-----*/
				*(I2C->Data_buffer[RX_POS] + I2C->I2C_DATA_POS[RX_POS]) =
						I2C->Instance->DR;
				I2C->I2C_DATA_POS[RX_POS]++;
			} else if (I2C->I2C_DATA_POS[RX_POS] < I2C->I2C_DATA_LEN[RX_POS]) {
				*(I2C->Data_buffer[RX_POS] + I2C->I2C_DATA_POS[RX_POS]) =I2C->Instance->DR;
				I2C->I2C_DATA_POS[RX_POS]++;
				I2C->Instance->CR1 |= STOP_EN;
				CLEAR_MSK(I2C->Instance->CR2 , ITEVTEN_EN|ITBUFEN_EN|ITERREN_EN);
				/*---Reset Handler----*/
				I2C->Data_buffer[RX_POS] = NULL;
				I2C->I2C_ADD_BUFFER = 0x00;
				I2C->I2C_DATA_LEN[RX_POS] = 0x00;
				I2C->I2C_BUSY_FLAG = I2C_EMPTY_RX;
				/*---------Callback to UpperLayer-----*/
				if(RX != NULL)
					RX(I2C_EVENT_RX_CMPLT);
			}
		}
	}
}

static void I2C_HANDLERTXEvent(I2C_IRQ_handler * I2C)
{
	if (I2C->I2C_BUSY_FLAG == I2C_BUSY_TX)
	{
		if (I2C->I2C_DATA_LEN[TX_POS] > 0)
		{
			I2C->Instance->DR = *(I2C->Data_buffer[TX_POS]+ I2C->I2C_DATA_POS[TX_POS]);
			I2C->I2C_DATA_POS[TX_POS]++;
			I2C->I2C_DATA_LEN[TX_POS]--;
		}
	}
}

static void I2C_TXEeventClose(I2C_IRQ_handler * I2C)
{
	CLEAR_MSK(I2C->Instance->CR2 , ITEVTEN|ITBUFEN_EN|ITERREN_EN);
	/*---Reset Handler----*/
	I2C->Data_buffer[TX_POS] = NULL;
	I2C->I2C_ADD_BUFFER = 0x00;
	I2C->I2C_DATA_LEN[TX_POS] = 0x00;
	I2C->I2C_BUSY_FLAG = I2C_EMPTY_TX;
	/*---Callback to Upper Layer For Notification---*/
	if (TX != NULL)
		TX(I2C_EVENT_TX_CMPLT);
}

/*----------Handler for Interrupts------*/
static void I2C_EV_IRQHandling( I2C_IRQ_handler * I2C)
{
	//Interrupt handling for both master and slave mode of a device
	uint16_t Temp1 = I2C->Instance->CR2 & ITEVTEN_EN;
	uint16_t Temp2 = I2C->Instance->CR2 & ITBUFEN_EN;
	uint16_t Temp3 = 0x0000;
	//1. Handle For interrupt generated by SB event
	//	Note : SB flag is only applicable in Master mode
	Temp3 = I2C->Instance->SR1 & I2C_SB;
	if(Temp1 && Temp3)
	{
		/*----Send Address as MSTR-----*/
		if(I2C->Instance->SR2 & MSTR_MODE)
		{
			/*-----Send Address with R/W bit Cleared---*/
			if(I2C->I2C_BUSY_FLAG == I2C_BUSY_TX)
			{
				TX(I2C_EVENT_TX_PROCESSING);
				I2C->Instance->DR = ((I2C->I2C_ADD_BUFFER<<1)| I2C_WRITE);
			}
			else if(I2C->I2C_BUSY_FLAG == I2C_BUSY_RX)
			{
				RX(I2C_EVENT_RX_PROCESSING);
				I2C->Instance->DR = ((I2C->I2C_ADD_BUFFER<<1)| I2C_READ);
			}
		}
	}
	//2. Handle For interrupt generated by ADDR event
	//Note : When master mode : Address is sent
	//		 When Slave mode   : Address matched with own address
	Temp3 = I2C->Instance->SR1 & I2C_ADDR;
	if(Temp1 && Temp3)
	{
		if(I2C->Instance->SR2 & MSTR_MODE)
		{
			if(I2C->I2C_BUSY_FLAG == I2C_BUSY_RX)
			{
				if(I2C->I2C_DATA_LEN[RX_POS] == 1)
					I2C->Instance->CR1 &= ACK_DIS;
			}
		}
		uint16_t Temp = I2C->Instance->SR1;
		Temp = I2C->Instance->SR2;
		(void) Temp;
	}
	//3. Handle For interrupt generated by BTF(Byte Transfer Finished) event
	Temp3 = I2C->Instance->SR1 & BTF_SET;
	if (Temp1 && Temp3)
	{
		if(I2C->Instance->SR2 & MSTR_MODE)
		{
			if(I2C->I2C_BUSY_FLAG ==I2C_BUSY_TX )
			{
				if(I2C->Instance->SR1 & TxE_SET )
				{
					if(I2C->I2C_DATA_LEN[TX_POS] == 0)
					{
						/*---Generate Stop Bit----*/
						I2C->Instance->CR1 |= STOP_EN;
						/*---Reset Handler----*/
						I2C_TXEeventClose(I2C);
					}
				}
			}else if (I2C->I2C_BUSY_FLAG == I2C_BUSY_RX)
			{

			}
		}
	}
	//4. Handle For interrupt generated by STOPF event
	// Note : Stop detection flag is applicable only slave mode . For master this flag will never be set
	Temp3 = I2C->Instance->SR1 & I2C_STOPF;
	if (Temp1 && Temp3)
	{
		if(!(I2C->Instance->SR2 & MSTR_MODE))
		{
			if(I2C->I2C_BUSY_FLAG == I2C_BUSY_RX)
			{
				uint16_t CR1_DUM = I2C->Instance->CR1;
				/*-----Clear STOPF and ADDR------*/
				uint16_t Temp = I2C->Instance->SR1;
				Temp = I2C->Instance->SR2;
				(void) Temp;
				uint16_t Dummy = I2C->Instance->SR1;
				I2C->Instance->CR1 = CR1_DUM;
				CLEAR_MSK(I2C->Instance->CR2 , ITEVTEN_EN|ITBUFEN_EN|ITERREN_EN);
				/*------Reset handler-----*/
				I2C->Data_buffer[TX_POS] = NULL;
				I2C->I2C_DATA_LEN[TX_POS] = 0x00;
				I2C->I2C_DATA_POS[TX_POS] = 0x00;
				I2C->I2C_BUSY_FLAG = I2C_EMPTY_RX;
				/*---Callback to Upper Layer For Notification Slave---*/
				RX(I2C_EVENT_SLAVE_RX_CMPLT);
			}else{}
		}else{}
	}
	//5. Handle For interrupt generated by TXE event
	Temp3 = I2C->Instance->SR1 & TxE_SET;
	if (Temp1 &&Temp2 &&Temp3)
	{
		if(I2C->Instance->SR2 & MSTR_MODE)
		{
			I2C_HANDLERTXEvent(I2C);
		}else{
			if(I2C->I2C_BUSY_FLAG == I2C_BUSY_TX)
				I2C_HANDLERTXEvent(I2C);
		}
	}
	//6. Handle For interrupt generated by RXNE event
	Temp3 = I2C->Instance->SR1 & RxNE_SET;
	if (Temp1 &&Temp2 &&Temp3)
	{
		if(I2C->Instance->SR2 & MSTR_MODE)
		{
			I2C_HANDLERRXEvent(I2C);
		}else{
			if(I2C->I2C_BUSY_FLAG == I2C_BUSY_RX)
			{
				*(I2C->Data_buffer[RX_POS] + I2C->I2C_DATA_POS[RX_POS]) = I2C->Instance->DR;
				I2C->I2C_DATA_POS[RX_POS]++;
			}
		}
	}
}

static void I2C_ERR_IRQHandling(I2C_IRQ_handler * I2C)
{
	uint32_t temp1, temp2;

	//Know the status of  ITERREN control bit in the CR2
	temp2 = (I2C->Instance->CR2) & ITERREN_EN;
	/***********************Check for Bus error************************************/
	temp1 = (I2C->Instance->SR1) & I2C_BERR;
	if (temp1 && temp2)
	{
		//Implement the code to clear the buss error flag
		CLEAR_MSK(I2C->Instance->SR1,I2C_BERR);
		//Implement the code to notify the application about the error
		if(I2C->I2C_BUSY_FLAG == I2C_BUSY_TX)
		{
			/*-------Check if MSTR Sends the Data---*/
			if(I2C->Instance->SR2 & MSTR_MODE)
			{
				/*----Abort Transfer----*/
				/*---Reset Handler----*/
				I2C->Instance = NULL;
				I2C->Data_buffer[TX_POS] = NULL;
				I2C->I2C_ADD_BUFFER = 0x00;
				I2C->I2C_DATA_LEN[TX_POS] = 0x00;
				I2C->I2C_BUSY_FLAG = I2C_EMPTY_TX;
				/*---Gen Stop Bit----*/
				I2C->Instance->CR1 |= STOP_EN;
			}
		}else if(I2C->I2C_BUSY_FLAG == I2C_BUSY_RX)
		{
			if (I2C->Instance->SR2 & MSTR_MODE) {
				/*----Abort Transfer----*/
				/*---Reset Handler----*/
				I2C->Instance = NULL;
				I2C->Data_buffer[RX_POS] = NULL;
				I2C->I2C_ADD_BUFFER = 0x00;
				I2C->I2C_DATA_LEN[RX_POS] = 0x00;
				I2C->I2C_BUSY_FLAG = I2C_EMPTY_RX;
				/*---Gen Stop Bit----*/
				I2C->Instance->CR1 |= STOP_EN;
			}
		}
	}
	/***********************Check for arbitration lost error************************************/
	temp1 = (I2C->Instance->SR1) & OVR_SET;
	if (temp1 && temp2) {
		//This is arbitration lost error
		//Implement the code to clear the arbitration lost error flag
		CLEAR_MSK(I2C->Instance->SR1,I2C_ARLO);
		//Implement the code to notify the application about the error
	}
	/***********************Check for ACK failure  error************************************/
	temp1 = (I2C->Instance->SR1) & AF_SET;
	if (temp1 && temp2) {
		CLEAR_MSK(I2C->Instance->SR1,AF_SET);
		if (I2C->Instance->SR2 & MSTR_MODE) {
			/*----Abort Transfer----*/
			if(I2C->I2C_BUSY_FLAG == I2C_BUSY_TX)
			{
				/*---Reset Handler----*/
				I2C->Instance = NULL;
				I2C->Data_buffer[TX_POS] = NULL;
				I2C->I2C_ADD_BUFFER = 0x00;
				I2C->I2C_DATA_LEN[TX_POS] = 0x00;
				I2C->I2C_BUSY_FLAG = I2C_EMPTY_TX;
				/*---Gen Stop Bit----*/
				I2C->Instance->CR1 |= STOP_EN;
			}else if(I2C->I2C_BUSY_FLAG == I2C_BUSY_RX)
			{
				/*---Reset Handler----*/
				I2C->Instance = NULL;
				I2C->Data_buffer[RX_POS] = NULL;
				I2C->I2C_ADD_BUFFER = 0x00;
				I2C->I2C_DATA_LEN[RX_POS] = 0x00;
				I2C->I2C_BUSY_FLAG = I2C_EMPTY_RX;
				/*---Gen Stop Bit----*/
				I2C->Instance->CR1 |= STOP_EN;
			}
		}else {
			if(!(I2C->Instance->SR2 & MSTR_MODE))
			{
				if(I2C->I2C_BUSY_FLAG == I2C_BUSY_TX)
				{
					/*----Slave Lines released by hardware----*/
					CLEAR_MSK(I2C->Instance->CR2 , ITEVTEN_EN|ITBUFEN_EN|ITERREN_EN);
					/*----Call Calback----*/
					I2C->Data_buffer[RX_POS] = NULL;
					I2C->I2C_DATA_LEN[RX_POS] = 0x00;
					I2C->I2C_DATA_POS[RX_POS] = 0x00;
					I2C->I2C_BUSY_FLAG = I2C_EMPTY_TX;
					TX(I2C_EVENT_SLAVE_TX_CMPLT);
				}
			}
		}
	}
	/***********************Check for Overrun/underrun error************************************/
	temp1 = (I2C->Instance->SR1) & OVR_SET;
	if (temp1 && temp2) {
		//This is Overrun/underrun
		CLEAR_MSK(I2C->Instance->SR1,OVR_SET);
		//Implement the code to clear the Overrun/underrun error flag
		if (I2C->Instance->SR2 & MSTR_MODE) {
			if(I2C->I2C_BUSY_FLAG == I2C_BUSY_RX)
			{
				CLEAR_MSK(I2C->Instance->SR1,RxNE_SET);
			}
		}else{

		}
	}
	/***********************Check for Time out erro SMBUS************************************/
	temp1 = (I2C->Instance->SR1) & I2C_TIMEOUT_;
	if (temp1 && temp2) {

	}
}

#endif

static void vMcal_I2C_PinInit(__IO I2C_TypeDef *Inst)
{
	__IO GPIO_Typedef *GPIO_Port = NULL;
	GPIO_InitStruct GPIO;
	__IO uint32_t *AFR_Reg = NULL;

	uint8_t Pin_Number_SDA = (Inst ==I2C1)?GPIO_PIN_7:(Inst ==I2C2)?GPIO_PIN_11:GPIO_PIN_4;
	uint8_t Pin_Number_SCL = (Inst ==I2C1)?GPIO_PIN_6:(Inst ==I2C2)?GPIO_PIN_10:GPIO_PIN_8;
	AFR_Reg = (Inst == I2C1)?&(GPIOB->AFRL):(Inst ==I2C2)?&(GPIOB->AFRH):&(GPIOB->AFRL);

	if(Inst == I2C3)
	{
		GPIO_Port = GPIOB;
		GPIO.Pin = Pin_Number_SDA ;
		GPIO.Speed = GPIO_Speed_50MHz;
		GPIO.mode = GPIO_MODE_AF_OD;
		GPIO.pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIO_Port,&GPIO);
		GPIO_Port = GPIOA;
		GPIO.Pin = Pin_Number_SCL ;
		HAL_GPIO_Init(GPIO_Port,&GPIO);
	}else{
		GPIO_Port = GPIOB;
		GPIO.Pin = Pin_Number_SDA | Pin_Number_SCL;
		GPIO.Speed = GPIO_Speed_50MHz;
		GPIO.mode = GPIO_MODE_AF_OD;
		GPIO.pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIO_Port,&GPIO);
	}

	if(Inst == I2C3)
	{
		 AFR_Reg  = &(GPIOA->AFRH);
		*AFR_Reg &= ~(0xF<<0);
		*AFR_Reg |= (9<<0);

		 AFR_Reg  = &(GPIOB->AFRL);
		*AFR_Reg &= ~(0xF<<16);
		*AFR_Reg |= (4<<16);
	}else if(Inst == I2C1)
	{
		*AFR_Reg &=~(0xF<<28);
		*AFR_Reg &=~(0xF<<24);
		*AFR_Reg |=(4<<28);
		*AFR_Reg |=(4<<24);
	}else if(Inst == I2C2)
	{
		*AFR_Reg &=~(0xF<<8);
		*AFR_Reg &=~(0xF<<12);
		*AFR_Reg |=(4<<8);
		*AFR_Reg |=(4<<12);
	}

}

static ERR xMcal_I2C_Slave_Init(__IO I2C_TypeDef *Inst,uint8_t Flag ,uint16_t Add)
{
	ERR I2C_State = HAL_OK;
	switch (Flag) {
		case I2C_SLAVE_ADDR_7BIT:
			Inst->OAR1 &= ADD_7BIT;
			Inst->OAR1 |= (((uint8_t)(Add & 0x7F))<<1);
			break;
		case I2C_SLAVE_ADDR_10BIT:
			Inst->OAR1 |= ADD_10BIT;
			Inst->OAR1 |= Add & 0x03FF;
			break;
		default:
			I2C_State = HAL_ERR;
			break;
	}
	return I2C_State;
}

#if I2C_INT_EN
static ERR xMcal_I2C_IntInit(I2C_TypeDef *Inst)
{
#if I2C1_INT_EN == EN
	if( Inst == I2C1)
	{
		NVIC_EnableIRQ(I2C1_EV_IRQ);
		NVIC_EnableIRQ(I2C1_ER_IRQ);
	}
#endif
#if I2C2_INT_EN == EN
	if(Inst == I2C2)
	{
		NVIC_EnableIRQ(I2C2_EV_IRQ);
		NVIC_EnableIRQ(I2C2_ER_IRQ);
	}
#endif
#if I2C3_INT_EN == EN
	if(Inst == I2C3)
	{
		NVIC_EnableIRQ(I2C3_EV_IRQ);
		NVIC_EnableIRQ(I2C3_ER_IRQ);
	}
#endif
}
#endif

static uint32_t xMcal_I2C_CR2_Init(__IO  I2C_TypeDef *Inst)
{
	uint32_t I2C_FREQ = HAL_RCC_GET_APB1FREQ();
	uint8_t I2C_CR2_FREQ = I2C_FREQ / I2C_PRESCALER;
	/*------SetUP I2C_CR2 Register-----*/
	Inst->CR2 |= I2C_CR2_FREQ;
	/*----Return----*/
	return I2C_FREQ;
}

static ERR xMcal_I2C_Freq_Init(__IO  I2C_TypeDef *Inst,uint8_t F_S_M,uint8_t Duty)
{
	ERR I2C_State = HAL_OK;
	uint32_t I2C_FREQ = 0;
	uint32_t CCR_Val;
	/*------SetUP I2C_CR2 Register-----*/
	I2C_FREQ = xMcal_I2C_CR2_Init(Inst);
	/*------Configure Clock Control Register-----*/
	switch (F_S_M) {
		case I2C_SM_MODE:
			Inst->CCR &= SM_MODE;
			CCR_Val = (I2C_FREQ / (2 * I2C_STANDARD_MODE));
			Inst->CCR |= (uint16_t)(CCR_Val & 0x7FF);
			break;
		case I2C_FM_MODE:
			Inst->CCR |= FM_MODE;
			xMcal_I2C_MSTRDuty_Init(Inst, Duty, I2C_FREQ);
			break;
		default:
			I2C_State = HAL_ERR;
			break;
	}
	return I2C_State;
}

static ERR xMcal_I2C_Trise_Init(__IO I2C_TypeDef *Inst,uint8_t F_S_M)
{
	ERR I2C_State = HAL_OK;
	uint32_t I2C_FREQ = HAL_RCC_GET_APB1FREQ();
	uint32_t Trise = 0;
	switch (F_S_M) {
		case I2C_SM_MODE:
			Trise = ((I2C_FREQ /I2C_PRESCALER) + 1);
			break;
		case I2C_FM_MODE:
			Trise = (((I2C_FREQ *3) /(I2C_PRESCALER*10)) + 1);
			break;
		default:
			I2C_State = HAL_ERR;
			break;
	}
	Inst->TRISE |= (Trise & 0x3F);
	return I2C_State;
}

static ERR xMcal_I2C_MSTRDuty_Init(__IO I2C_TypeDef *Inst,uint8_t Duty,uint32_t Freq)
{
	ERR I2C_State = HAL_OK;
	uint32_t CCR_Val = 0;
	switch(Duty)
	{
		case I2C_FM_DUTY_2:
			Inst->CCR &= DUTY_2;
			CCR_Val = (Freq/(3*I2C_FAST_MODE));
			break;
		case I2C_FM_DUTY_16_9:
			Inst->CCR |= DUTY_16_9;
			CCR_Val = (Freq/(25*I2C_FAST_MODE));
			break;
		default:
			I2C_State = HAL_ERR;
			break;
	}
	Inst->CCR |= (uint16_t)(CCR_Val & 0x7FF);
	return I2C_State;
}

static ERR xMcal_I2C_Clock_Init(__IO I2C_TypeDef *Inst)
{
	if(Inst == I2C1)
	{
		HAL_RCC_I2C1_EN();
	}else if(Inst == I2C2)
	{
		HAL_RCC_I2C2_EN();
	}else if(Inst == I2C3)
	{
		HAL_RCC_I2C3_EN();
	}else{
		return HAL_ERR;
	}
	return HAL_OK;
}

#if I2C1_INT_EN == EN
void I2C1_EV_IRQHandler(void)
{
	I2C_EV_IRQHandling(&I2C1_Handler);
}

void I2C1_ER_IRQHandler(void)
{
	I2C_ERR_IRQHandling(&I2C1_Handler);
}

#endif

#if I2C2_INT_EN == EN
void I2C2_EV_IRQHandler(void)
{
	I2C_EV_IRQHandling(&I2C12_Handler);
}

void I2C2_ER_IRQHandler(void)
{
	I2C_ERR_IRQHandling(&I2C2_Handler);
}
#endif

#if I2C3_INT_EN == EN
void I2C3_EV_IRQHandler(void)
{
	I2C_EV_IRQHandling(&I2C3_Handler);
}

void I2C3_ER_IRQHandler(void)
{
	I2C_ERR_IRQHandling(&I2C3_Handler);
}
#endif
