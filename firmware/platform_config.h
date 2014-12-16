#ifndef PLATFORM_CONFIG_H_INCLUDED
#define PLATFORM_CONFIG_H_INCLUDED

#include <misc.h>
#include <stm32f10x_rcc.h>

#define SPI2_ENABLE
#define SDCARD_ENABLE

#define DEBUG_RCC1              0
#define DEBUG_RCC2              RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO | RCC_APB2Periph_USART1
#define DEBUG_USART             USART1
#define DEBUG_USART_BAUD        19200
#define DEBUG_USART_TX          GPIOA
#define DEBUG_USART_TX_PIN      GPIO_Pin_9
#define DEBUG_USART_RX          GPIOA
#define DEBUG_USART_RX_PIN      GPIO_Pin_10
#define DEBUG_USART_RX_DMA_CH   DMA1_Channel5
#define DEBUG_USART_DR_BASE     ((uint32_t)&USART1->DR)

#ifdef SDCARD_ENABLE
#ifndef SPI2_ENABLE
#  error "SPI2 is required for adc"
#endif
#define SDCARD_RCC1             RCC_APB1Periph_SPI2
#define SDCARD_RCC2             RCC_APB2Periph_GPIOA
#define SDCARD_CS               GPIOA
#define SDCARD_CS_PIN           GPIO_Pin_8
#define SDCARD_SPI              SPI2
#endif // SDCARD_ENABLE

#endif // PLATFORM_CONFIG_H_INCLUDED
