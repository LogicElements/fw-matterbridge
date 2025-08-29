cmake_minimum_required(VERSION 3.22)

set(UTILITIES_INCLUDE
		"lpm/tiny_lpm"
)

set(UTILITIES_SRC
		"lpm/tiny_lpm/stm32_lpm.c"
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${UTILITIES_SRC})

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${UTILITIES_INCLUDE})