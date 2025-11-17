#include "soft_spi.h"

// 简单的延时函数，用于控制SCK速度
static void SOFT_SPI_Delay(void)
{
    volatile uint32_t i = 8;  // 根据实际时钟调整
    while(i--);
}

void SOFT_SPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 使能GPIO时钟
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();

    // SCK (PB2) 推挽输出
    GPIO_InitStruct.Pin = SOFT_SPI_SCK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SOFT_SPI_SCK_PORT, &GPIO_InitStruct);

    // CS (PB6) 推挽输出
    GPIO_InitStruct.Pin = SOFT_SPI_CS_PIN;
    HAL_GPIO_Init(SOFT_SPI_CS_PORT, &GPIO_InitStruct);

    // MOSI (PF8) 推挽输出
    GPIO_InitStruct.Pin = SOFT_SPI_MOSI_PIN;
    HAL_GPIO_Init(SOFT_SPI_MOSI_PORT, &GPIO_InitStruct);

    // MISO (PF9) 输入
    GPIO_InitStruct.Pin = SOFT_SPI_MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(SOFT_SPI_MISO_PORT, &GPIO_InitStruct);

    // 初始状态
    HAL_GPIO_WritePin(SOFT_SPI_SCK_PORT, SOFT_SPI_SCK_PIN, GPIO_PIN_SET);   // SCK 高电平
    HAL_GPIO_WritePin(SOFT_SPI_CS_PORT, SOFT_SPI_CS_PIN, GPIO_PIN_SET);     // CS 高电平（不选中）
    HAL_GPIO_WritePin(SOFT_SPI_MOSI_PORT, SOFT_SPI_MOSI_PIN, GPIO_PIN_RESET); // MOSI 低电平
}

void SOFT_SPI_SendByte(uint8_t byte)
{
    for (int i = 0; i < 8; i++)
    {
        // 设置 MOSI
        if (byte & 0x80)
            HAL_GPIO_WritePin(SOFT_SPI_MOSI_PORT, SOFT_SPI_MOSI_PIN, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(SOFT_SPI_MOSI_PORT, SOFT_SPI_MOSI_PIN, GPIO_PIN_RESET);

        // 产生 SCK 下降沿
        HAL_GPIO_WritePin(SOFT_SPI_SCK_PORT, SOFT_SPI_SCK_PIN, GPIO_PIN_RESET);
        SOFT_SPI_Delay();
        
        // 产生 SCK 上升沿，芯片在上升沿采样数据
        HAL_GPIO_WritePin(SOFT_SPI_SCK_PORT, SOFT_SPI_SCK_PIN, GPIO_PIN_SET);
        SOFT_SPI_Delay();

        byte <<= 1;
    }
}

uint8_t SOFT_SPI_ReceiveByte(void)
{
    uint8_t byte = 0;

    for (int i = 0; i < 8; i++)
    {
        byte <<= 1;

        // 产生 SCK 下降沿
        HAL_GPIO_WritePin(SOFT_SPI_SCK_PORT, SOFT_SPI_SCK_PIN, GPIO_PIN_RESET);
        SOFT_SPI_Delay();

        // 读取 MISO
        if (HAL_GPIO_ReadPin(SOFT_SPI_MISO_PORT, SOFT_SPI_MISO_PIN) == GPIO_PIN_SET)
            byte |= 0x01;

        // 产生 SCK 上升沿
        HAL_GPIO_WritePin(SOFT_SPI_SCK_PORT, SOFT_SPI_SCK_PIN, GPIO_PIN_SET);
        SOFT_SPI_Delay();
    }

    return byte;
}

// 同时发送和接收（全双工）
uint8_t SOFT_SPI_ReadWriteByte(uint8_t byte)
{
    uint8_t receive_data = 0;

    for (int i = 0; i < 8; i++)
    {
        receive_data <<= 1;

        // 设置 MOSI
        if (byte & 0x80)
            HAL_GPIO_WritePin(SOFT_SPI_MOSI_PORT, SOFT_SPI_MOSI_PIN, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(SOFT_SPI_MOSI_PORT, SOFT_SPI_MOSI_PIN, GPIO_PIN_RESET);

        // 产生 SCK 下降沿
        HAL_GPIO_WritePin(SOFT_SPI_SCK_PORT, SOFT_SPI_SCK_PIN, GPIO_PIN_RESET);
        SOFT_SPI_Delay();

        // 读取 MISO
        if (HAL_GPIO_ReadPin(SOFT_SPI_MISO_PORT, SOFT_SPI_MISO_PIN) == GPIO_PIN_SET)
            receive_data |= 0x01;

        // 产生 SCK 上升沿
        HAL_GPIO_WritePin(SOFT_SPI_SCK_PORT, SOFT_SPI_SCK_PIN, GPIO_PIN_SET);
        SOFT_SPI_Delay();

        byte <<= 1;
    }

    return receive_data;
}