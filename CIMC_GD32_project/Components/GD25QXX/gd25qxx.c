#include "gd25qxx.h"
#include "stm32f4xx_hal.h"
#include <string.h>

#define WRITE 0x02 /* write to memory instruction */
#define WRSR 0x01  /* write status register instruction */
#define WREN 0x06  /* write enable instruction */

/*
	这里读取ID出错
	9FH 返回的是通用 JEDEC 标准 ID (3 字节)，包含了容量信息 (15h 代表 16Mbit)。

	90H 返回的是制造商特定 ID (2 字节)，包含了设备型号信息 (14h 代表 GD25Q16)。

	这两种命令访问的是芯片内部不同的信息区域，返回不同的、各自正确的值是完全正常
	且符合预期的。在实际应用中，使用 9FH 读取 JEDEC ID 是更常用、更标准的方法。
*/
// #define RDID 0x90 /* read identification */ // 注释掉或删除旧的
#define JEDEC_ID 0x9F /* read JEDEC identification */

#define READ 0x03 /* read from memory instruction */
#define RDSR 0x05 /* read status register instruction  */
//#define RDID 0x90 /* read identification */
#define SE 0x20   /* sector erase instruction */
#define BE 0xC7   /* bulk erase instruction */

#define WIP_FLAG 0x01 /* write in progress(wip)flag */
#define DUMMY_BYTE 0xA5

/* SPI handle */
extern SPI_HandleTypeDef hspi2;

/**
 * @brief Initializes the SPI Flash chip.
 * @note This function assumes that the SPI peripheral (e.g., hspi2) and CS GPIO
 *       have already been initialized by the STM32CubeMX generated code (e.g., MX_SPI2_Init(), MX_GPIO_Init()).
 *       It primarily ensures the CS pin is high (chip deselected) initially.
 *       You can add a Flash ID read here for an initial check if desired.
 */
void spi_flash_init(void)
{
    // Ensure CS pin is high (deselected) initially.
    // The GPIO for CS (PB12 in your macros) should be configured as output push-pull in MX_GPIO_Init.
    SPI_FLASH_CS_HIGH();
    // Optional: Add a small delay if needed after power-up or SPI init, before first command
    // HAL_Delay(1);

    // Optional: You could read the Flash ID here to verify communication
    // uint32_t id = spi_flash_read_id();
    // (Add code to check ID or print it for debugging)
}

void spi_flash_sector_erase(uint32_t sector_addr)
{
    spi_flash_write_enable();

    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(SE);
    spi_flash_send_byte((sector_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((sector_addr & 0xFF00) >> 8);
    spi_flash_send_byte(sector_addr & 0xFF);
    SPI_FLASH_CS_HIGH();

    spi_flash_wait_for_write_end();
}

void spi_flash_bulk_erase(void)
{
    spi_flash_write_enable();

    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(BE);
    SPI_FLASH_CS_HIGH();

    spi_flash_wait_for_write_end();
}

void spi_flash_page_write(uint8_t *pbuffer, uint32_t write_addr, uint16_t num_byte_to_write)
{
    spi_flash_write_enable();

    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(WRITE);
    spi_flash_send_byte((write_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((write_addr & 0xFF00) >> 8);
    spi_flash_send_byte(write_addr & 0xFF);

    while (num_byte_to_write--)
    {
        spi_flash_send_byte(*pbuffer);
        pbuffer++;
    }

    SPI_FLASH_CS_HIGH();
    spi_flash_wait_for_write_end();
}

void spi_flash_buffer_write(uint8_t *pbuffer, uint32_t write_addr, uint16_t num_byte_to_write)
{
    uint8_t num_of_page = 0, num_of_single = 0, addr = 0, count = 0, temp = 0;

    addr = write_addr % SPI_FLASH_PAGE_SIZE;
    count = SPI_FLASH_PAGE_SIZE - addr;
    num_of_page = num_byte_to_write / SPI_FLASH_PAGE_SIZE;
    num_of_single = num_byte_to_write % SPI_FLASH_PAGE_SIZE;

    if (0 == addr)
    {
        if (0 == num_of_page)
        {
            spi_flash_page_write(pbuffer, write_addr, num_byte_to_write);
        }
        else
        {
            while (num_of_page--)
            {
                spi_flash_page_write(pbuffer, write_addr, SPI_FLASH_PAGE_SIZE);
                write_addr += SPI_FLASH_PAGE_SIZE;
                pbuffer += SPI_FLASH_PAGE_SIZE;
            }
            spi_flash_page_write(pbuffer, write_addr, num_of_single);
        }
    }
    else
    {
        if (0 == num_of_page)
        {
            if (num_of_single > count)
            {
                temp = num_of_single - count;
                spi_flash_page_write(pbuffer, write_addr, count);
                write_addr += count;
                pbuffer += count;
                spi_flash_page_write(pbuffer, write_addr, temp);
            }
            else
            {
                spi_flash_page_write(pbuffer, write_addr, num_byte_to_write);
            }
        }
        else
        {
            num_byte_to_write -= count;
            num_of_page = num_byte_to_write / SPI_FLASH_PAGE_SIZE;
            num_of_single = num_byte_to_write % SPI_FLASH_PAGE_SIZE;

            spi_flash_page_write(pbuffer, write_addr, count);
            write_addr += count;
            pbuffer += count;

            while (num_of_page--)
            {
                spi_flash_page_write(pbuffer, write_addr, SPI_FLASH_PAGE_SIZE);
                write_addr += SPI_FLASH_PAGE_SIZE;
                pbuffer += SPI_FLASH_PAGE_SIZE;
            }

            if (0 != num_of_single)
            {
                spi_flash_page_write(pbuffer, write_addr, num_of_single);
            }
        }
    }
}

void spi_flash_buffer_read(uint8_t *pbuffer, uint32_t read_addr, uint16_t num_byte_to_read)
{
    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(READ);
    spi_flash_send_byte((read_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((read_addr & 0xFF00) >> 8);
    spi_flash_send_byte(read_addr & 0xFF);

    while (num_byte_to_read--)
    {
        *pbuffer = spi_flash_send_byte(DUMMY_BYTE);
        pbuffer++;
    }

    SPI_FLASH_CS_HIGH();
}

/**
 * @brief Reads the JEDEC Manufacturer and Device ID (using 9FH command).
	修改后的读取ID值，读取标准通用的ID值
 * @return uint32_t: (Manufacturer ID << 16) | (Memory Type << 8) | Capacity
 * For GD25Q16C, expected value is typically 0xC84015
 */
uint32_t spi_flash_read_id(void)
{
    uint8_t temp0 = 0, temp1 = 0, temp2 = 0;

    SPI_FLASH_CS_LOW();

    // 1. 发送 JEDEC ID 命令
    spi_flash_send_byte(JEDEC_ID); 

    // 2. 连续读取 3 个字节的 ID 数据
    //    在读取时，需要发送无效数据 (Dummy) 来产生时钟让 Flash 输出
    temp0 = spi_flash_send_byte(DUMMY_BYTE); // 读取 Manufacturer ID
    temp1 = spi_flash_send_byte(DUMMY_BYTE); // 读取 Memory Type
    temp2 = spi_flash_send_byte(DUMMY_BYTE); // 读取 Capacity

    SPI_FLASH_CS_HIGH();

    // 组合成 32 位值返回 (高位补零)
    return (temp0 << 16) | (temp1 << 8) | temp2;
}

void spi_flash_start_read_sequence(uint32_t read_addr)
{
    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(READ);
    spi_flash_send_byte((read_addr & 0xFF0000) >> 16);
    spi_flash_send_byte((read_addr & 0xFF00) >> 8);
    spi_flash_send_byte(read_addr & 0xFF);
}

uint8_t spi_flash_read_byte(void)
{
    return (spi_flash_send_byte(DUMMY_BYTE));
}

uint8_t spi_flash_send_byte(uint8_t byte)
{
    uint8_t rx_data;
    HAL_SPI_TransmitReceive(&hspi2, &byte, &rx_data, 1, 1000);
    return rx_data;
}

uint16_t spi_flash_send_halfword(uint16_t half_word)
{
    uint16_t rx_data;
    HAL_SPI_TransmitReceive(&hspi2, (uint8_t *)&half_word, (uint8_t *)&rx_data, 2, 1000);
    return rx_data;
}

void spi_flash_write_enable(void)
{
    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(WREN);
    SPI_FLASH_CS_HIGH();
}

void spi_flash_wait_for_write_end(void)
{
    uint8_t flash_status = 0;

    SPI_FLASH_CS_LOW();
    spi_flash_send_byte(RDSR);

    do
    {
        flash_status = spi_flash_send_byte(DUMMY_BYTE);
    } while ((flash_status & WIP_FLAG) == 0x01);

    SPI_FLASH_CS_HIGH();
}


