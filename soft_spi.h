#ifndef __SOFT_SPI_H
#define __SOFT_SPI_H

#include "main.h"

// 定义软件SPI引脚 - 使用QSPI引脚
#define SOFT_SPI_SCK_PIN    GPIO_PIN_2
#define SOFT_SPI_SCK_PORT   GPIOB

#define SOFT_SPI_CS_PIN     GPIO_PIN_6
#define SOFT_SPI_CS_PORT    GPIOB

#define SOFT_SPI_MOSI_PIN   GPIO_PIN_8
#define SOFT_SPI_MOSI_PORT  GPIOF

#define SOFT_SPI_MISO_PIN   GPIO_PIN_9
#define SOFT_SPI_MISO_PORT  GPIOF

// 函数声明
void SOFT_SPI_Init(void);
void SOFT_SPI_SendByte(uint8_t byte);
uint8_t SOFT_SPI_ReceiveByte(void);
uint8_t SOFT_SPI_ReadWriteByte(uint8_t byte);

#endif