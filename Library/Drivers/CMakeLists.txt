cmake_minimum_required(VERSION 3.22)

set(DRIVERS_INCLUDE
		STM32WBxx_HAL_Driver/Inc
		STM32WBxx_HAL_Driver/Inc/Legacy
		BSP/P-NUCLEO-WB55.Nucleo
		CMSIS/Device/ST/STM32WBxx/Include
		CMSIS/Include
)

set(DRIVERS_SRC
		"BSP/P-NUCLEO-WB55.Nucleo/stm32wbxx_nucleo.c"

		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_cortex.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_dma.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_dma_ex.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_exti.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_flash.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_flash_ex.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_gpio.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_hsem.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_i2c.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_i2c_ex.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_ipcc.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_iwdg.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_pwr.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_pwr_ex.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_qspi.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rcc.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rcc_ex.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rng.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rtc.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_rtc_ex.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_spi.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_spi_ex.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_tim.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_tim_ex.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_uart.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_hal_uart_ex.c"
		"STM32WBxx_HAL_Driver/Src/stm32wbxx_ll_rcc.c"
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${DRIVERS_SRC})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${DRIVERS_INCLUDE})