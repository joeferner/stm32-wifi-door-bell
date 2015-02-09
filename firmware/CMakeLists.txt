PROJECT(stm32-wifi-door-bell)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
ENABLE_LANGUAGE(ASM)

IF(NOT STM32_DEVICE_DIR)
  SET(STM32_DEVICE_DIR "/opt/STM32Cube_FW_F0_V1.2.0")
  MESSAGE(STATUS "No STM32_DEVICE_DIR specified, using default: " ${STM32_DEVICE_DIR})
ENDIF()

SET(CMSIS_FIND_LIBS "cmsis_md")
SET(CMSIS_STARTUP_SOURCE "${STM32_DEVICE_DIR}/Drivers/CMSIS/Device/ST/STM32F0xx/Source/Templates/gcc/startup_stm32f072xb.s")
SET(STM32_CHIP_DEF "STM32F072xB")
SET(STACK_ADDRESS "0x20005000")
SET(FLASH_SIZE "128K")
SET(RAM_SIZE "20K")
SET(MIN_STACK_SIZE "0x200")
SET(MIN_HEAP_SIZE "0")
SET(EXT_RAM_SIZE "0K")
SET(FLASH_ORIGIN "0x08000000")
SET(RAM_ORIGIN "0x20000000")
SET(EXT_RAM_ORIGIN "0x60000000")

CONFIGURE_FILE(stm32_flash.in ${CMAKE_CURRENT_BINARY_DIR}/stm32_flash.md)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -DMDNS_ADVERTISE_HOST -DUSE_HAL_DRIVER -D${STM32_CHIP_DEF} -I../ -I../contiki/")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -DMDNS_ADVERTISE_HOST -DUSE_HAL_DRIVER -D${STM32_CHIP_DEF} -I../ -I../contiki/")
SET(CMAKE_EXE_LINKER_FLAGS "-T${CMAKE_CURRENT_BINARY_DIR}/stm32_flash.md ${CMAKE_EXE_LINKER_FLAGS}")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_STDPERIPH_DRIVER")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_STDPERIPH_DRIVER")

INCLUDE_DIRECTORIES(
  ${STM32_DEVICE_DIR}/Drivers/CMSIS/Device/ST/STM32F0xx/Include/
  ${STM32_DEVICE_DIR}/Drivers/STM32F0xx_HAL_Driver/Inc/
  ${STM32_DEVICE_DIR}/Drivers/CMSIS/Include/
  ${CMAKE_CURRENT_SOURCE_DIR}
)

SET(PROJECT_SOURCES
  ${STM32_DEVICE_DIR}/Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_gpio.c
  ${STM32_DEVICE_DIR}/Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_rcc.c
  ${STM32_DEVICE_DIR}/Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_usart.c
  ${STM32_DEVICE_DIR}/Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_spi.c
  ${STM32_DEVICE_DIR}/Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_tim.c
  ${STM32_DEVICE_DIR}/Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_wwdg.c
  ${STM32_DEVICE_DIR}/Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_pwr.c
  ${STM32_DEVICE_DIR}/Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_dma.c
  ${STM32_DEVICE_DIR}/Drivers/STM32F0xx_HAL_Driver/Src/stm32f0xx_hal_rtc.c
  system_stm32f0xx.c

  main.c
  interrupts.c
  #rtc.c
  #config.c
  #network.c
  #button.c
  #stm32lib/spi.c
  #stm32lib/delay.c
  #stm32lib/time.c
  stm32lib/debug.c
  #stm32lib/util.c
  #stm32lib/ntp.c
  #stm32lib/newlib.c
  #stm32lib/crc32.c
  #stm32lib/sdcard.c
  #stm32lib/sdcard-fat.c
  #stm32lib/cc3000.c
  #stm32lib/cc3000-host-driver/wlan.c
  #stm32lib/cc3000-host-driver/cc3000_common.c
  #stm32lib/cc3000-host-driver/socket.c
  #stm32lib/cc3000-host-driver/hci.c
  #stm32lib/cc3000-host-driver/evnt_handler.c
  #stm32lib/cc3000-host-driver/nvmem.c
  #stm32lib/cc3000-host-driver/netapp.c
)

ADD_EXECUTABLE(${CMAKE_PROJECT_NAME}.elf ${PROJECT_SOURCES} ${CMSIS_STARTUP_SOURCE})
TARGET_LINK_LIBRARIES(${CMAKE_PROJECT_NAME}.elf ${CMSIS_LIBRARIES} ${StdPeriphLib_LIBRARIES})

ADD_CUSTOM_TARGET(${CMAKE_PROJECT_NAME}.hex DEPENDS ${CMAKE_PROJECT_NAME}.elf COMMAND ${CMAKE_OBJCOPY} -Oihex ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.hex)
ADD_CUSTOM_TARGET(${CMAKE_PROJECT_NAME}.bin DEPENDS ${CMAKE_PROJECT_NAME}.elf COMMAND ${CMAKE_OBJCOPY} -Obinary ${CMAKE_PROJECT_NAME}.elf ${CMAKE_PROJECT_NAME}.bin)
ADD_CUSTOM_TARGET(${CMAKE_PROJECT_NAME}.list DEPENDS ${CMAKE_PROJECT_NAME}.elf COMMAND ${CMAKE_OBJDUMP} -x -S ${CMAKE_PROJECT_NAME}.elf > ${CMAKE_PROJECT_NAME}.list)

