
#ifndef PLATFORM_CONFIG_H_INCLUDED
#define PLATFORM_CONFIG_H_INCLUDED

//#define SPI1_ENABLE
#define SPI2_ENABLE
#define SDCARD_ENABLE
//#define CC3000_ENABLE
//#define BUTTON_ENABLE

#define DEBUG_USART   USART1
#define DEBUG_BAUD    9600
#define DEBUG_TX_PORT GPIOA
#define DEBUG_TX_PIN  GPIO_Pin_9
#define DEBUG_RX_PORT GPIOA
#define DEBUG_RX_PIN  GPIO_Pin_10

#ifdef SDCARD_ENABLE
#ifndef SPI2_ENABLE
#  error "SPI2 is required for adc"
#endif
#define NEWLIB_FAT     fat
#define SDCARD_SPI     SPI2
#define SDCARD_CS_PORT GPIOA
#define SDCARD_CS_PIN  GPIO_Pin_8
#endif // SDCARD_ENABLE

#ifdef CC3000_ENABLE
#ifndef SPI1_ENABLE
#  error "SPI1 is required for adc"
#endif
#define CC3000_RCC1             0
#define CC3000_RCC2             RCC_APB2Periph_SPI1 | RCC_APB2Periph_GPIOB
#define CC3000_CS               GPIOB
#define CC3000_CS_PIN           GPIO_Pin_0
#define CC3000_IRQ              GPIOB
#define CC3000_IRQ_PIN          GPIO_Pin_1
#define CC3000_IRQ_EXTI_LINE    EXTI_Line1
#define CC3000_IRQ_EXTI_PORT    GPIO_PortSourceGPIOB
#define CC3000_IRQ_EXTI_PIN     GPIO_PinSource1
#define CC3000_IRQ_EXTI_CH      EXTI1_IRQn
#define CC3000_EN               GPIOB
#define CC3000_EN_PIN           GPIO_Pin_2
#define CC3000_SPI              SPI1
#endif // SDCARD_ENABLE

#ifdef BUTTON_ENABLE
#define BUTTON_RCC1             0
#define BUTTON_RCC2             RCC_APB2Periph_GPIOA

#define BUTTON0_PORT            GPIOA
#define BUTTON0_PIN             GPIO_Pin_0
#define BUTTON0_EXTI_LINE       EXTI_Line0
#define BUTTON0_EXTI_PORT       GPIO_PortSourceGPIOA
#define BUTTON0_EXTI_PIN        GPIO_PinSource0
#define BUTTON0_EXTI_CH         EXTI0_IRQn

#define BUTTON1_PORT            GPIOA
#define BUTTON1_PIN             GPIO_Pin_2
#define BUTTON1_EXTI_LINE       EXTI_Line2
#define BUTTON1_EXTI_PORT       GPIO_PortSourceGPIOA
#define BUTTON1_EXTI_PIN        GPIO_PinSource2
#define BUTTON1_EXTI_CH         EXTI2_IRQn
#endif

#include <stm32lib/utils.h>
#include <stm32lib/hal/rcc.h>
#include <stm32lib/hal/gpio.h>
#include <stm32lib/debug.h>
#include <stm32lib/rcc.h>
#include <stm32lib/time.h>
#include <stm32lib/spi.h>
#include <stm32lib/device/sdcard/sdcard.h>
#include <stm32lib/device/sdcard/fat.h>

#endif // PLATFORM_CONFIG_H_INCLUDED
