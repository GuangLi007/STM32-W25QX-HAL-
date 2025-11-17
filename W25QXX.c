#include <stdio.h>

#include "soft_spi.h"
#include "w25qxx.h"

uint8_t write_buffer[256];
uint8_t read_buffer[256];


uint16_t W25QXX_TYPE = W25Q256;
#define ENABLE_W25Q258 1

// CS控制函数
static void W25QXX_CS_Set(uint8_t state)
{
    HAL_GPIO_WritePin(SOFT_SPI_CS_PORT, SOFT_SPI_CS_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// 重定义SPI读写函数
#define SPI1_ReadWriteByte(x) SOFT_SPI_ReadWriteByte(x)

// 软件复位W25qXX
void W25XX_Reset_With_Soft(void)
{
    W25QXX_CS_Set(0);                    // 使能器件
    SPI1_ReadWriteByte(W25X_Enable_Rest);
    W25QXX_CS_Set(1);
    W25QXX_CS_Set(0);
    SPI1_ReadWriteByte(W25X_Rest_Device);
    W25QXX_CS_Set(1);
}

#if ENABLE_W25Q258
// 进入4字节地址模式
void W25QXX_Enter_4Byte(void)
{
    W25QXX_CS_Set(0);
    SPI1_ReadWriteByte(W25X_Enter_4_Addres);
    W25QXX_CS_Set(1);
}

// 退出4字节地址模式
void W25QXX_Exit_4Byte(void)
{
    W25QXX_CS_Set(0);
    SPI1_ReadWriteByte(W25X_Exit_4_Addres);
    W25QXX_CS_Set(1);
}
#endif

void W25QXX_Init(void)
{
    // 初始化软件SPI（包括CS引脚）
    SOFT_SPI_Init();
    
    // 设置CS为高电平（不选中）
    W25QXX_CS_Set(1);
    
    // 进入4字节模式
    #if ENABLE_W25Q258
        W25QXX_Enter_4Byte();
    #endif
    
    W25QXX_TYPE = W25QXX_ReadID();
}

uint8_t W25QXX_ReadSR(void)
{
    uint8_t byte = 0;
    W25QXX_CS_Set(0);                         // 使能器件
    SPI1_ReadWriteByte(W25X_ReadStatusReg);   // 发送读取状态寄存器命令
    byte = SPI1_ReadWriteByte(0Xff);          // 读取一个字节
    W25QXX_CS_Set(1);                         // 取消片选
    return byte;
}

void W25QXX_Write_SR(uint8_t sr)
{
    W25QXX_CS_Set(0);                         // 使能器件
    SPI1_ReadWriteByte(W25X_WriteStatusReg);  // 发送写取状态寄存器命令
    SPI1_ReadWriteByte(sr);                   // 写入一个字节
    W25QXX_CS_Set(1);                         // 取消片选
}

void W25QXX_Write_Enable(void)
{
    W25QXX_CS_Set(0);                         // 使能器件
    SPI1_ReadWriteByte(W25X_WriteEnable);     // 发送写使能
    W25QXX_CS_Set(1);                         // 取消片选
}

void W25QXX_Write_Disable(void)
{
    W25QXX_CS_Set(0);                         // 使能器件
    SPI1_ReadWriteByte(W25X_WriteDisable);    // 发送写禁止指令
    W25QXX_CS_Set(1);                         // 取消片选
}

uint16_t W25QXX_ReadID(void)
{
    uint16_t Temp = 0;
    W25QXX_CS_Set(0);
    SPI1_ReadWriteByte(0x90);                 // 发送读取ID命令
    SPI1_ReadWriteByte(0x00);
    SPI1_ReadWriteByte(0x00);
    SPI1_ReadWriteByte(0x00);
    Temp |= SPI1_ReadWriteByte(0xFF) << 8;
    Temp |= SPI1_ReadWriteByte(0xFF);
    W25QXX_CS_Set(1);
    return Temp;
}

void W25QXX_Read(uint8_t* pBuffer, uint32_t ReadAddr, uint32_t NumByteToRead)
{
    uint32_t i;
    W25QXX_CS_Set(0);                         // 使能器件

    #if ENABLE_W25Q258
        SPI1_ReadWriteByte(W25X_ReadData);    // 发送4字节地址读取命令
        SPI1_ReadWriteByte((uint8_t)((ReadAddr) >> 24)); // 发送32bit地址
    #else
        SPI1_ReadWriteByte(W25X_ReadData);    // 发送普通读指令
    #endif
    
    SPI1_ReadWriteByte((uint8_t)((ReadAddr) >> 16));  // 发送24bit地址
    SPI1_ReadWriteByte((uint8_t)((ReadAddr) >> 8));
    SPI1_ReadWriteByte((uint8_t)ReadAddr);
    
    for(i = 0; i < NumByteToRead; i++)
    {
        pBuffer[i] = SPI1_ReadWriteByte(0XFF);   // 循环读数
    }
    W25QXX_CS_Set(1);
}

void W25QXX_Write_Page(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
    uint32_t i;
    W25QXX_Write_Enable();                    // SET WEL
    W25QXX_CS_Set(0);                         // 使能器件
    SPI1_ReadWriteByte(W25X_PageProgram);     // 发送写页命令
    
    #if ENABLE_W25Q258
        SPI1_ReadWriteByte((uint8_t)((WriteAddr) >> 24)); // 发送32bit地址
    #endif
    
    SPI1_ReadWriteByte((uint8_t)((WriteAddr) >> 16)); // 发送24bit地址
    SPI1_ReadWriteByte((uint8_t)((WriteAddr) >> 8));
    SPI1_ReadWriteByte((uint8_t)WriteAddr);
    
    for(i = 0; i < NumByteToWrite; i++)
    {
        SPI1_ReadWriteByte(pBuffer[i]);       // 循环写数
    }
    W25QXX_CS_Set(1);                         // 取消片选
    W25QXX_Wait_Busy();                       // 等待写入结束
}

void W25QXX_Write_NoCheck(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
    uint32_t pageremain;
    pageremain = 256 - WriteAddr % 256;       // 单页剩余的字节数
    if(NumByteToWrite <= pageremain)
        pageremain = NumByteToWrite;          // 不大于256个字节
    
    while(1)
    {
        W25QXX_Write_Page(pBuffer, WriteAddr, pageremain);
        if(NumByteToWrite == pageremain)
            break;                            // 写入结束了
        else                                  // NumByteToWrite > pageremain
        {
            pBuffer += pageremain;
            WriteAddr += pageremain;
            NumByteToWrite -= pageremain;     // 减去已经写入了的字节数
            if(NumByteToWrite > 256)
                pageremain = 256;             // 一次可以写入256个字节
            else 
                pageremain = NumByteToWrite;  // 不够256个字节了
        }
    }
}

uint8_t W25QXX_BUFFER[4096];

void W25QXX_Write(uint8_t* pBuffer, uint32_t WriteAddr, uint32_t NumByteToWrite)
{
    uint32_t secpos;
    uint32_t secoff;
    uint32_t secremain;
    uint32_t i;
    uint8_t * W25QXX_BUF;

    W25QXX_BUF = W25QXX_BUFFER;
    secpos = WriteAddr / 4096;                // 扇区地址
    secoff = WriteAddr % 4096;                // 在扇区内的偏移
    secremain = 4096 - secoff;                // 扇区剩余空间大小
    
    if(NumByteToWrite <= secremain)
        secremain = NumByteToWrite;           // 不大于4096个字节
    
    while(1)
    {
        W25QXX_Read(W25QXX_BUF, secpos * 4096, 4096); // 读出整个扇区的内容
        for(i = 0; i < secremain; i++)        // 校验数据
        {
            if(W25QXX_BUF[secoff + i] != 0XFF)
                break;                        // 需要擦除
        }
        
        if(i < secremain)                     // 需要擦除
        {
            W25QXX_Erase_Sector(secpos);      // 擦除这个扇区
            for(i = 0; i < secremain; i++)    // 复制
            {
                W25QXX_BUF[i + secoff] = pBuffer[i];
            }
            W25QXX_Write_NoCheck(W25QXX_BUF, secpos * 4096, 4096); // 写入整个扇区
        }
        else 
        {
            W25QXX_Write_NoCheck(pBuffer, WriteAddr, secremain); // 写已经擦除了的,直接写入扇区剩余区间
        }
        
        if(NumByteToWrite == secremain)
            break;                            // 写入结束了
        else                                  // 写入未结束
        {
            secpos++;                         // 扇区地址增1
            secoff = 0;                       // 偏移位置为0
            pBuffer += secremain;             // 指针偏移
            WriteAddr += secremain;           // 写地址偏移
            NumByteToWrite -= secremain;      // 字节数递减
            if(NumByteToWrite > 4096)
                secremain = 4096;             // 下一个扇区还是写不完
            else 
                secremain = NumByteToWrite;   // 下一个扇区可以写完了
        }
    }
}

void W25QXX_Erase_Chip(void)
{
    W25QXX_Write_Enable();                    // SET WEL
    W25QXX_Wait_Busy();
    W25QXX_CS_Set(0);                         // 使能器件
    SPI1_ReadWriteByte(W25X_ChipErase);       // 发送片擦除命令
    W25QXX_CS_Set(1);                         // 取消片选
    W25QXX_Wait_Busy();                       // 等待芯片擦除结束
}

void W25QXX_Erase_Sector(uint32_t Dst_Addr)
{
    Dst_Addr *= 4096;
    W25QXX_Write_Enable();                    // SET WEL
    W25QXX_Wait_Busy();
    W25QXX_CS_Set(0);                         // 使能器件
    SPI1_ReadWriteByte(W25X_SectorErase);     // 发送扇区擦除指令
    
    #if ENABLE_W25Q258
        SPI1_ReadWriteByte((uint8_t)((Dst_Addr) >> 24));
    #endif
    
    SPI1_ReadWriteByte((uint8_t)((Dst_Addr) >> 16));  // 发送24bit地址
    SPI1_ReadWriteByte((uint8_t)((Dst_Addr) >> 8));
    SPI1_ReadWriteByte((uint8_t)Dst_Addr);
    
    W25QXX_CS_Set(1);                         // 取消片选
    W25QXX_Wait_Busy();                       // 等待擦除完成
}

void W25QXX_Wait_Busy(void)
{
    while((W25QXX_ReadSR() & 0x01) == 0x01);  // 等待BUSY位清空
}

void W25QXX_PowerDown(void)
{
    uint32_t Time_i = 0xffff;
    W25QXX_CS_Set(0);                         // 使能器件
    SPI1_ReadWriteByte(W25X_PowerDown);       // 发送掉电命令
    W25QXX_CS_Set(1);                         // 取消片选
    while(Time_i--);                          // 等待TPD
}

void W25QXX_WAKEUP(void)
{
    uint32_t Time_i = 0xffff;
    W25QXX_CS_Set(0);                         // 使能器件
    SPI1_ReadWriteByte(W25X_ReleasePowerDown); // 发送唤醒命令
    W25QXX_CS_Set(1);                         // 取消片选
    while(Time_i--);                          // 等待TRES1
}

void W25QXX_Test(void)
{
    uint32_t i;
    uint32_t test_addr = 0x1000; // 测试地址

    printf("\r\n=== W25QXX Flash Test ===\r\n");

    // 1. 填充测试数据
    for(i = 0; i < 256; i++)
    {
        write_buffer[i] = i;
    }

    // 2. 擦除扇区
    printf("Erasing sector at 0x%08lX...\r\n", test_addr);
    W25QXX_Erase_Sector(test_addr / 4096);
    printf("Sector erase complete.\r\n");

    // 3. 写入数据
    printf("Writing 256 bytes to 0x%08lX...\r\n", test_addr);
    W25QXX_Write(write_buffer, test_addr, 256);
    printf("Write complete.\r\n");

    // 4. 读取数据
    printf("Reading 256 bytes from 0x%08lX...\r\n", test_addr);
    W25QXX_Read(read_buffer, test_addr, 256);
    printf("Read complete.\r\n");

    // 5. 验证数据
    uint8_t verify_ok = 1;
    for(i = 0; i < 256; i++)
    {
        if(read_buffer[i] != write_buffer[i])
        {
            verify_ok = 0;
            printf("Verify error at byte %lu: wrote 0x%02X, read 0x%02X\r\n",
                   i, write_buffer[i], read_buffer[i]);
            break;
        }
    }

    if(verify_ok)
    {
        printf("Data verification: PASS\r\n");
    }
    else
    {
        printf("Data verification: FAIL\r\n");
    }

    // 6. 测试读取状态寄存器
    uint8_t status = W25QXX_ReadSR();
    printf("Status Register: 0x%02X\r\n", status);

    // 7. 测试读取部分数据并显示
    printf("\r\nFirst 16 bytes read:\r\n");
    for(i = 0; i < 16; i++)
    {
        printf("%02X ", read_buffer[i]);
        if((i + 1) % 16 == 0) printf("\r\n");
    }
    printf("\r\n");

    printf("=== Test Complete ===\r\n");
}
