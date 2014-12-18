#ifndef PLATFORM_CONFIG_H_INCLUDED
#define PLATFORM_CONFIG_H_INCLUDED

#include <misc.h>
#include <stm32f10x_rcc.h>

#define MDNS_DEVICE_NAME "doorbell01"

#define SPI1_ENABLE
#define SPI2_ENABLE
#define SDCARD_ENABLE
#define CC3000_ENABLE

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

#endif // PLATFORM_CONFIG_H_INCLUDED
