#include "sd_fatfs.h"
#include "fatfs.h"					//sdFATFS文件系统


void test_sd_fatfs(void)
{
    FRESULT res;           // FatFs操作结果码
    DIR dir;               // 目录对象
    FILINFO fno;           // 文件信息对象
    uint32_t byteswritten; // 写入字节计数
    uint32_t bytesread;    // 读取字节计数
    char ReadBuffer[256];  // 读取缓冲区
    char WriteBuffer[] = "米醋电子工作室 - GD32 SD卡FATFS测试数据，如果你能看到这条消息，说明SD卡文件系统工作正常！";
    UINT bw, br;                              // 读写字节数计数
    const char *TestFileName = "SD_TEST.TXT"; // 测试文件名

    my_printf(&huart1, "\r\n--- SD卡FATFS文件系统测试开始 ---\r\n");

    // 挂载文件系统
    if (f_mount(&SDFatFS, SDPath, 1) != FR_OK)
    {
        my_printf(&huart1, "SD卡挂载失败，请检查SD卡连接或是否初始化\r\n");
        return;
    }
    my_printf(&huart1, "SD卡挂载成功\r\n");

    // 清理可能存在的乱码目录（可选，仅首次运行时需要）
    // f_unlink("测试目录");  // 取消注释可删除旧的乱码目录

    // 创建测试目录（如果不存在）
    res = f_mkdir("TestDir");
    if (res == FR_OK)
    {
        my_printf(&huart1, "创建测试目录成功\r\n");
    }
    else if (res == FR_EXIST)
    {
        my_printf(&huart1, "测试目录已存在\r\n");
    }
    else
    {
        my_printf(&huart1, "创建目录失败，错误码: %d\r\n", res);
    }

    // 创建并写入测试文件
    my_printf(&huart1, "创建并写入测试文件...\r\n");
    res = f_open(&SDFile, TestFileName, FA_CREATE_ALWAYS | FA_WRITE);
    if (res == FR_OK)
    {
        // 写入数据
        res = f_write(&SDFile, WriteBuffer, strlen(WriteBuffer), &bw);
        if (res == FR_OK && bw == strlen(WriteBuffer))
        {
            my_printf(&huart1, "写入文件成功: %u 字节\r\n", bw);
        }
        else
        {
            my_printf(&huart1, "写入文件失败，错误码: %d\r\n", res);
        }

        // 关闭文件
        f_close(&SDFile);
    }
    else
    {
        my_printf(&huart1, "创建文件失败，错误码: %d\r\n", res);
    }

    // 读取测试文件
    my_printf(&huart1, "读取测试文件...\r\n");
    memset(ReadBuffer, 0, sizeof(ReadBuffer));
    res = f_open(&SDFile, TestFileName, FA_READ);
    if (res == FR_OK)
    {
        // 读取数据
        res = f_read(&SDFile, ReadBuffer, sizeof(ReadBuffer) - 1, &br);
        if (res == FR_OK)
        {
            ReadBuffer[br] = '\0'; // 确保字符串结束符
            my_printf(&huart1, "读取文件成功: %u 字节\r\n", br);
            my_printf(&huart1, "文件内容: %s\r\n", ReadBuffer);

            // 验证数据一致性
            if (strcmp(ReadBuffer, WriteBuffer) == 0)
            {
                my_printf(&huart1, "数据验证成功: 读写数据一致\r\n");
            }
            else
            {
                my_printf(&huart1, "数据验证失败: 读写数据不一致\r\n");
            }
        }
        else
        {
            my_printf(&huart1, "读取文件失败，错误码: %d\r\n", res);
        }

        // 关闭文件
        f_close(&SDFile);
    }
    else
    {
        my_printf(&huart1, "打开文件失败，错误码: %d\r\n", res);
    }

    // 列出根目录文件
    my_printf(&huart1, "\r\n根目录文件列表:\r\n");
    res = f_opendir(&dir, "/");
    if (res == FR_OK)
    {
        for (;;)
        {
            // 读取目录项
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0)
                break;

            // 显示文件/目录信息
            if (fno.fattrib & AM_DIR)
            {
                my_printf(&huart1, "[DIR] %s\r\n", fno.fname);
            }
            else
            {
                my_printf(&huart1, "[FILE] %s (%lu 字节)\r\n", fno.fname, (unsigned long)fno.fsize);
            }
        }
        f_closedir(&dir);
    }
    else
    {
        my_printf(&huart1, "打开目录失败，错误码: %d\r\n", res);
    }

    // 完成测试，卸载文件系统
    f_mount(NULL, SDPath, 0);
    my_printf(&huart1, "--- SD卡FATFS文件系统测试结束 ---\r\n");
}
