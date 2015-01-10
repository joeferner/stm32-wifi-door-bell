
#include "button.h"
#include "config.h"
#include "network.h"
#include <stdio.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_exti.h>

int _button_eventButtonNumber;

void button_setup() {
  GPIO_InitTypeDef gpioInitStructure;
  EXTI_InitTypeDef extiInitStructure;
  NVIC_InitTypeDef nvicInitStructure;

  printf("BEGIN BUTTON Setup\n");

  _button_eventButtonNumber = BUTTON_NONE;

  if (BUTTON_RCC1) {
    RCC_APB1PeriphClockCmd(BUTTON_RCC1, ENABLE);
  }
  if (BUTTON_RCC2) {
    RCC_APB2PeriphClockCmd(BUTTON_RCC2, ENABLE);
  }

  // Button 0
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  gpioInitStructure.GPIO_Pin = BUTTON0_PIN;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(BUTTON0_PORT, &gpioInitStructure);

  GPIO_EXTILineConfig(BUTTON0_EXTI_PORT, BUTTON0_EXTI_PIN);

  EXTI_StructInit(&extiInitStructure);
  extiInitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  extiInitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  extiInitStructure.EXTI_LineCmd = ENABLE;
  extiInitStructure.EXTI_Line = BUTTON0_EXTI_LINE;
  EXTI_Init(&extiInitStructure);

  nvicInitStructure.NVIC_IRQChannel = BUTTON0_EXTI_CH;
  nvicInitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
  nvicInitStructure.NVIC_IRQChannelSubPriority = 0x0F;
  nvicInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvicInitStructure);

  // Button 1
  gpioInitStructure.GPIO_Pin = BUTTON1_PIN;
  gpioInitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  gpioInitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(BUTTON1_PORT, &gpioInitStructure);

  GPIO_EXTILineConfig(BUTTON1_EXTI_PORT, BUTTON1_EXTI_PIN);

  EXTI_StructInit(&extiInitStructure);
  extiInitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  extiInitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  extiInitStructure.EXTI_LineCmd = ENABLE;
  extiInitStructure.EXTI_Line = BUTTON1_EXTI_LINE;
  EXTI_Init(&extiInitStructure);

  nvicInitStructure.NVIC_IRQChannel = BUTTON1_EXTI_CH;
  nvicInitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
  nvicInitStructure.NVIC_IRQChannelSubPriority = 0x0F;
  nvicInitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvicInitStructure);

  printf("END Button Setup\n");
}

void _button_irq(int buttonNumber) {
  _button_eventButtonNumber = buttonNumber;
}

void button_loop() {
  ConfigButton* configButton;
  if (_button_eventButtonNumber == BUTTON_NONE) {
    return;
  }

  if (_button_eventButtonNumber == BUTTON0) {
    configButton = &config.button0;
  } else if (_button_eventButtonNumber == BUTTON1) {
    configButton = &config.button1;
  } else {
    printf("Unhandles button %d\n", _button_eventButtonNumber);
    return;
  }

  printf("button pressed '%s' (%d)\n", configButton->name, _button_eventButtonNumber);
  network_sendButtonEvent(_button_eventButtonNumber, configButton);
  _button_eventButtonNumber = BUTTON_NONE;
}


