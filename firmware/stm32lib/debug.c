
#include "debug.h"
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>
#ifdef DEBUG_USART_DMA_ENABLE
#include <stm32f10x_dma.h>
#include "ring_buffer.h"
#endif

#ifndef DEBUG_USART_DMA_RX_BUFFER_SIZE
#define DEBUG_USART_DMA_RX_BUFFER_SIZE 100
#endif

#ifdef DEBUG_USART_DMA_ENABLE
uint8_t g_debugUsartDmaRxBuffer[DEBUG_USART_DMA_RX_BUFFER_SIZE];
dma_ring_buffer g_debugUsartDmaRxBufferRingBuffer;
#endif

void debug_setup() {
  USART_InitTypeDef usartInitStructure;
  GPIO_InitTypeDef gpioInitStructure;
#ifdef DEBUG_USART_DMA_ENABLE
  DMA_InitTypeDef dmaInitStructure;
#endif

  GPIO_StructInit(&gpioInitStructure);

#ifdef DEBUG_USART_DMA_ENABLE
  dma_ring_buffer_init(&g_debugUsartDmaRxBufferRingBuffer, DEBUG_USART_DMA_RX_CH, g_debugUsartDmaRxBuffer, DEBUG_USART_DMA_RX_BUFFER_SIZE);

  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
#endif

#ifdef DEBUG_LED_ENABLE
  RCC_APB2PeriphClockCmd(DEBUG_LED_RCC, ENABLE);
  gpioInitStructure.GPIO_Pin = DEBUG_LED_PIN;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(DEBUG_LED_PORT, &gpioInitStructure);
#endif

  if (DEBUG_RCC1) {
    RCC_APB1PeriphClockCmd(DEBUG_RCC1, ENABLE);
  }
  if (DEBUG_RCC2) {
    RCC_APB2PeriphClockCmd(DEBUG_RCC2, ENABLE);
  }

  // Configure USART Tx as alternate function push-pull
  gpioInitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
  gpioInitStructure.GPIO_Pin = DEBUG_USART_TX_PIN;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(DEBUG_USART_TX, &gpioInitStructure);

  // Configure USART Rx as input floating
  gpioInitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  gpioInitStructure.GPIO_Pin = DEBUG_USART_RX_PIN;
  GPIO_Init(DEBUG_USART_RX, &gpioInitStructure);

#ifdef DEBUG_USART_DMA_ENABLE
  // Configure DMA - RX
  DMA_DeInit(DEBUG_USART_RX_DMA_CH);
  DMA_StructInit(&dmaInitStructure);
  dmaInitStructure.DMA_PeripheralBaseAddr = DEBUG_USART_DR_BASE;
  dmaInitStructure.DMA_MemoryBaseAddr = (uint32_t)g_debugUsartDmaRxBuffer;
  dmaInitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  dmaInitStructure.DMA_BufferSize = DEBUG_USART_DMA_RX_BUFFER_SIZE;
  dmaInitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  dmaInitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  dmaInitStructure.DMA_Mode = DMA_Mode_Circular;
  dmaInitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
  dmaInitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  dmaInitStructure.DMA_Priority = DMA_Priority_Medium;
  dmaInitStructure.DMA_M2M = DMA_M2M_Disable;
  DMA_Init(DEBUG_USART_RX_DMA_CH, &dmaInitStructure);
#endif

  // USART configuration
  USART_StructInit(&usartInitStructure);
  usartInitStructure.USART_BaudRate = DEBUG_USART_BAUD;
  usartInitStructure.USART_WordLength = USART_WordLength_8b;
  usartInitStructure.USART_Parity = USART_Parity_No;
  usartInitStructure.USART_StopBits = USART_StopBits_1;
  usartInitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  usartInitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(DEBUG_USART, &usartInitStructure);

#ifdef DEBUG_USART_DMA_ENABLE
  // Enable DMA
  USART_DMACmd(DEBUG_USART, USART_DMAReq_Rx, ENABLE);
  DMA_Cmd(DEBUG_USART_RX_DMA_CH, ENABLE);
#endif

  // Enable USART
  USART_Cmd(DEBUG_USART, ENABLE);

  printf("END Debug\n");
}

#ifdef DEBUG_LED_ENABLE
void debug_led_set(int v) {
  if (v) {
    GPIO_SetBits(DEBUG_LED_PORT, DEBUG_LED_PIN);
  } else {
    GPIO_ResetBits(DEBUG_LED_PORT, DEBUG_LED_PIN);
  }
}
#endif

uint8_t debug_read() {
#ifdef DEBUG_USART_DMA_ENABLE
  uint8_t buffer[1];
  while (dma_ring_buffer_read(g_debugUsartDmaRxBufferRingBuffer, buffer, 1) == 0);
  return buffer[0];
#else
  while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_RXNE) == RESET);
  return (uint8_t)USART_ReceiveData(DEBUG_USART);
#endif
}

void debug_write(uint8_t b) {
  USART_SendData(DEBUG_USART, b);
  while (USART_GetFlagStatus(DEBUG_USART, USART_FLAG_TXE) == RESET);
}

int _read(int file, char* ptr, int len) {
  int n;
  int num = 0;
  switch (file) {
    case STDIN_FILENO:
      for (n = 0; n < len; n++) {
        *ptr++ = (char)debug_read();
        num++;
      }
      break;
    default:
      errno = EBADF;
      return -1;
  }
  return num;
}

int _write(int file, char* ptr, int len) {
  int n;
  switch (file) {
    case STDOUT_FILENO: /*stdout*/
      for (n = 0; n < len; n++) {
        debug_write(*ptr++);
      }
      break;
    case STDERR_FILENO: /* stderr */
      for (n = 0; n < len; n++) {
        debug_write(*ptr++);
      }
      break;
    default:
      errno = EBADF;
      return -1;
  }
  return len;
}

