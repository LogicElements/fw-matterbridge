/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file         stm32wbxx_hal_msp.c
 * @brief        This file provides code for the MSP Initialization
 *               and de-Initialization codes.
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
/* USER CODE BEGIN Includes */
#include "../inc/app_conf.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN Define */

/* USER CODE END Define */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN Macro */

/* USER CODE END Macro */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
extern TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch3;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* External functions --------------------------------------------------------*/
/* USER CODE BEGIN ExternalFunctions */

/* USER CODE END ExternalFunctions */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
/**
 * Initializes the Global MSP.
 */
void HAL_MspInit(void)
{
	/* USER CODE BEGIN MspInit 0 */

	/* USER CODE END MspInit 0 */

	__HAL_RCC_HSEM_CLK_ENABLE();

	/* System interrupt init*/
	/* PendSV_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);

	/* Peripheral interrupt init */
	/* HSEM_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(HSEM_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(HSEM_IRQn);

	/* USER CODE BEGIN MspInit 1 */

	/* Peripheral interrupt init */
	/* PVD_PVM_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(PVD_PVM_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(PVD_PVM_IRQn);
	/* FLASH_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(FLASH_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(FLASH_IRQn);
	/* RCC_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(RCC_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(RCC_IRQn);
	/* C2SEV_PWR_C2H_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(C2SEV_PWR_C2H_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(C2SEV_PWR_C2H_IRQn);
	/* PWR_SOTF_BLEACT_802ACT_RFPHASE_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(PWR_SOTF_BLEACT_802ACT_RFPHASE_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(PWR_SOTF_BLEACT_802ACT_RFPHASE_IRQn);
	/* FPU_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(FPU_IRQn, 5, 0);
	HAL_NVIC_EnableIRQ(FPU_IRQn);

	/* USER CODE END MspInit 1 */
}

/**
 * @brief IPCC MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hipcc: IPCC handle pointer
 * @retval None
 */
void HAL_IPCC_MspInit(IPCC_HandleTypeDef* hipcc)
{
	if (hipcc->Instance == IPCC)
	{
		/* USER CODE BEGIN IPCC_MspInit 0 */

		/* USER CODE END IPCC_MspInit 0 */
		/* Peripheral clock enable */
		__HAL_RCC_IPCC_CLK_ENABLE();
		/* IPCC interrupt Init */
		HAL_NVIC_SetPriority(IPCC_C1_RX_IRQn, 5, 0);
		HAL_NVIC_EnableIRQ(IPCC_C1_RX_IRQn);
		HAL_NVIC_SetPriority(IPCC_C1_TX_IRQn, 5, 0);
		HAL_NVIC_EnableIRQ(IPCC_C1_TX_IRQn);
		/* USER CODE BEGIN IPCC_MspInit 1 */

		/* USER CODE END IPCC_MspInit 1 */
	}
}

/**
 * @brief IPCC MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hipcc: IPCC handle pointer
 * @retval None
 */
void HAL_IPCC_MspDeInit(IPCC_HandleTypeDef* hipcc)
{
	if (hipcc->Instance == IPCC)
	{
		/* USER CODE BEGIN IPCC_MspDeInit 0 */

		/* USER CODE END IPCC_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_IPCC_CLK_DISABLE();

		/* IPCC interrupt DeInit */
		HAL_NVIC_DisableIRQ(IPCC_C1_RX_IRQn);
		HAL_NVIC_DisableIRQ(IPCC_C1_TX_IRQn);
		/* USER CODE BEGIN IPCC_MspDeInit 1 */

		/* USER CODE END IPCC_MspDeInit 1 */
	}
}

/**
 * @brief RNG MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hrng: RNG handle pointer
 * @retval None
 */
void HAL_RNG_MspInit(RNG_HandleTypeDef* hrng)
{
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
	if (hrng->Instance == RNG)
	{
		/* USER CODE BEGIN RNG_MspInit 0 */

		/* USER CODE END RNG_MspInit 0 */

		/** Initializes the peripherals clock
		 */
		PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RNG;
		PeriphClkInitStruct.RngClockSelection = RCC_RNGCLKSOURCE_HSI48;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
		{
			Error_Handler();
		}

		/* Peripheral clock enable */
		__HAL_RCC_RNG_CLK_ENABLE();
		/* USER CODE BEGIN RNG_MspInit 1 */

		/* USER CODE END RNG_MspInit 1 */
	}
}

/**
 * @brief RNG MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hrng: RNG handle pointer
 * @retval None
 */
void HAL_RNG_MspDeInit(RNG_HandleTypeDef* hrng)
{
	if (hrng->Instance == RNG)
	{
		/* USER CODE BEGIN RNG_MspDeInit 0 */

		/* USER CODE END RNG_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_RNG_CLK_DISABLE();
		/* USER CODE BEGIN RNG_MspDeInit 1 */

		/* USER CODE END RNG_MspDeInit 1 */
	}
}

/**
 * @brief RTC MSP Initialization
 * This function configures the hardware resources used in this example
 * @param hrtc: RTC handle pointer
 * @retval None
 */
void HAL_RTC_MspInit(RTC_HandleTypeDef* hrtc)
{
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
	if (hrtc->Instance == RTC)
	{
		/* USER CODE BEGIN RTC_MspInit 0 */
		HAL_PWR_EnableBkUpAccess(); /**< Enable access to the RTC registers */

		/**
		 *  Write twice the value to flush the APB-AHB bridge
		 *  This bit shall be written in the register before writing the next one
		 */
		HAL_PWR_EnableBkUpAccess();

		__HAL_RCC_RTC_CONFIG(RCC_RTCCLKSOURCE_LSE); /**< Select LSE as RTC Input */
		/* USER CODE END RTC_MspInit 0 */

		/** Initializes the peripherals clock
		 */
		PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
		PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
		{
			Error_Handler();
		}

		/* Peripheral clock enable */
		__HAL_RCC_RTC_ENABLE();
		__HAL_RCC_RTCAPB_CLK_ENABLE();
		/* RTC interrupt Init */
		HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 5, 0);
		HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
		/* USER CODE BEGIN RTC_MspInit 1 */

		MODIFY_REG(RTC->CR, RTC_CR_WUCKSEL, CFG_RTC_WUCKSEL_DIVIDER);

		/* USER CODE END RTC_MspInit 1 */
	}
}

/**
 * @brief RTC MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param hrtc: RTC handle pointer
 * @retval None
 */
void HAL_RTC_MspDeInit(RTC_HandleTypeDef* hrtc)
{
	if (hrtc->Instance == RTC)
	{
		/* USER CODE BEGIN RTC_MspDeInit 0 */

		/* USER CODE END RTC_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_RTC_DISABLE();
		__HAL_RCC_RTCAPB_CLK_DISABLE();

		/* RTC interrupt DeInit */
		HAL_NVIC_DisableIRQ(RTC_WKUP_IRQn);
		/* USER CODE BEGIN RTC_MspDeInit 1 */

		/* USER CODE END RTC_MspDeInit 1 */
	}
}

/* USER CODE BEGIN 1 */
/**
 * @brief TIM_Base MSP Initialization
 * This function configures the hardware resources used in this example
 * @param htim_base: TIM_Base handle pointer
 * @retval None
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef* htim_base)
{
	if (htim_base->Instance == TIM1)
	{
		/* USER CODE BEGIN TIM1_MspInit 0 */

		/* USER CODE END TIM1_MspInit 0 */
		/* Peripheral clock enable */
		__HAL_RCC_TIM1_CLK_ENABLE();

		/* TIM1 DMA Init */
		/* TIM1_CH3 Init */
		hdma_tim1_ch3.Instance = DMA1_Channel1;
		hdma_tim1_ch3.Init.Request = DMA_REQUEST_TIM1_CH3;
		hdma_tim1_ch3.Init.Direction = DMA_MEMORY_TO_PERIPH;
		hdma_tim1_ch3.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_tim1_ch3.Init.MemInc = DMA_MINC_ENABLE;
		hdma_tim1_ch3.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
		hdma_tim1_ch3.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
		hdma_tim1_ch3.Init.Mode = DMA_NORMAL;
		hdma_tim1_ch3.Init.Priority = DMA_PRIORITY_LOW;
		if (HAL_DMA_Init(&hdma_tim1_ch3) != HAL_OK)
		{
			Error_Handler();
		}

		__HAL_LINKDMA(htim_base, hdma[TIM_DMA_ID_CC3], hdma_tim1_ch3);

		/* USER CODE BEGIN TIM1_MspInit 1 */

		/* USER CODE END TIM1_MspInit 1 */
	}
}

void HAL_TIM_MspPostInit(TIM_HandleTypeDef* htim)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if (htim->Instance == TIM1)
	{
		/* USER CODE BEGIN TIM1_MspPostInit 0 */

		/* USER CODE END TIM1_MspPostInit 0 */

		__HAL_RCC_GPIOA_CLK_ENABLE();
		/**TIM1 GPIO Configuration
		PA10     ------> TIM1_CH3
		*/
		GPIO_InitStruct.Pin = RGB_LED_Pin;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
		HAL_GPIO_Init(RGB_LED_GPIO_Port, &GPIO_InitStruct);

		/* USER CODE BEGIN TIM1_MspPostInit 1 */

		/* USER CODE END TIM1_MspPostInit 1 */
	}
}

/**
 * @brief TIM_Base MSP De-Initialization
 * This function freeze the hardware resources used in this example
 * @param htim_base: TIM_Base handle pointer
 * @retval None
 */
void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef* htim_base)
{
	if (htim_base->Instance == TIM1)
	{
		/* USER CODE BEGIN TIM1_MspDeInit 0 */

		/* USER CODE END TIM1_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_TIM1_CLK_DISABLE();

		/* TIM1 DMA DeInit */
		HAL_DMA_DeInit(htim_base->hdma[TIM_DMA_ID_CC3]);
		/* USER CODE BEGIN TIM1_MspDeInit 1 */

		/* USER CODE END TIM1_MspDeInit 1 */
	}
}

/* USER CODE END 1 */
