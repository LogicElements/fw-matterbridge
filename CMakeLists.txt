cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS ON)

if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -MMD -MP -w")

include("cmake/gcc-arm-none-eabi.cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

message("Build type: " ${CMAKE_BUILD_TYPE})

set(CMAKE_PROJECT_NAME Matter-Test)
project(${CMAKE_PROJECT_NAME})
enable_language(C CXX ASM)

add_executable(${CMAKE_PROJECT_NAME})

# Convert output to hex and binary
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.hex
)

# Convert to bin file -> add conditional check?
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}> ${CMAKE_PROJECT_NAME}.bin
)
add_subdirectory(Library)

file(GLOB SOURCE_FILES "Core/Src/*.c" "Core/Src/*.cpp")

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
		${SOURCE_FILES}

		"Core/Codegen/zap-generated/app/callback-stub.cpp"
		"Core/Codegen/zap-generated/app/cluster-init-callback.cpp"
		"Core/Codegen/zap-generated/IMClusterCommandHandler.cpp"
		"Library/Drivers/BSP/Components/s25fl128s/s25fl128s.c"

		startup_stm32wb55xx_cm4.s
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "Core/Inc")

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC "Core/Codegen" "Core/Codegen/zap-generated")

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
		USE_NUCLEO_64
		USE_HAL_DRIVER
		USE_STM32WBXX_NUCLEO
		STM32WB55xx
		$<$<CONFIG:Debug>:DEBUG>
		CHIP_DNSSD_DEFAULT_PLATFORM
		"OPENTHREAD_CONFIG_FILE=<openthread_api_config_matter.h>"
		"CHIP_PLATFORM_CONFIG_INCLUDE=<CHIPPlatformConfig.h>"
		"CHIP_PROJECT_CONFIG_INCLUDE=<CHIPProjectConfig.h>"
		"MBEDTLS_CONFIG_FILE=<matter_config.h>"
		"CHIP_ADDRESS_RESOLVE_IMPL_INCLUDE_HEADER=<lib/address_resolve/AddressResolve_DefaultImpl.h>"
		"CHIP_HAVE_CONFIG_H"
		"THREAD_WB"
		"NON_SPEC_COMPLIANT_OTA_ACTION_DELAY_FLOOR=-1"
		"CORE_CM4"
)