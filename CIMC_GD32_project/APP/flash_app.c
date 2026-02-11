// 包含flash应用程序的头文件，里面声明了本文件用到的函数和数据结构
#include "flash_app.h"


/*======================================================================
 * 全局变量定义区域
 *======================================================================*/

// lfs_t是LittleFS文件系统的主要结构体，用来管理整个文件系统
// 类比：就像一个图书馆的管理系统，记录了所有书籍（文件）的信息
lfs_t lfs;

// lfs_config是文件系统的配置结构体，包含了如何读写Flash的具体方法
// 类比：就像图书馆的规章制度，规定了如何存放和取出书籍
struct lfs_config cfg;


/*======================================================================
 * 函数名：lfs_init
 * 功能：初始化LittleFS文件系统
 * 参数：无
 * 返回值：无
 *
 * 详细说明：
 * 1. 这个函数是文件系统的初始化入口，必须在使用文件系统之前调用
 * 2. 它会先初始化底层的SPI Flash硬件，然后配置文件系统参数
 * 3. 如果初始化失败，程序会停在这里（死循环），方便调试
 *======================================================================*/
void lfs_init(void)
{
  // 步骤1：初始化SPI Flash硬件
  // SPI Flash就像一个存储芯片，需要先"唤醒"它才能使用
  spi_flash_init();

  // 打印调试信息到串口，告诉我们正在初始化存储后端
  my_printf(&huart1, "LFS: Initializing storage backend...\r\n");

  // 步骤2：配置文件系统的底层存储参数（读写擦除函数等）
  // lfs_storage_init会设置cfg结构体中的各种参数
  // 如果返回值不等于LFS_ERR_OK，说明初始化失败
  if (lfs_storage_init(&cfg) != LFS_ERR_OK)
  {
    // 初始化失败，打印错误信息
    my_printf(&huart1, "LFS: Storage backend init FAILED! Halting.\r\n");

    // 进入死循环，让程序停在这里
    // 为什么要死循环？因为文件系统初始化失败后继续运行可能会出现更严重的错误
    // 停在这里方便我们发现问题并调试
    while (1);
  }

  // 初始化成功，打印成功信息
  my_printf(&huart1, "LFS: Storage backend init OK.\r\n");
}


/*======================================================================
 * 函数名：list_dir_recursive
 * 功能：递归列出目录中的所有文件和子目录（像树状图一样显示）
 * 参数：
 *   - path: 要遍历的目录路径，如 "/" 表示根目录，"/boot" 表示boot目录
 *   - level: 当前递归的层级（深度），用于缩进显示，根目录level=0
 *
 * 什么是递归？
 * 递归就是函数调用自己。就像俄罗斯套娃，打开一个娃娃里面还有娃娃。
 * 这个函数打开一个目录，如果里面有子目录，就再次调用自己去打开子目录。
 *
 * 执行流程举例：
 * 假设文件系统结构是：
 * /
 * ├── boot/
 * │   └── boot_cnt.txt
 * └── data/
 *     ├── file1.txt
 *     └── subfolder/
 *         └── file2.txt
 *
 * 调用list_dir_recursive("/", 0)会：
 * 1. 打开根目录 /
 * 2. 读到boot目录 -> 调用list_dir_recursive("/boot", 1)
 * 3. 读到data目录 -> 调用list_dir_recursive("/data", 1)
 * 4. 以此类推，直到所有目录都遍历完
 *======================================================================*/
void list_dir_recursive(const char *path, int level)
{
    // ===== 变量声明区域 =====

    // 目录操作句柄（handle），就像打开文件夹的"钥匙"
    // 类比：dir就像你打开一个文件夹后得到的"窗口"，通过它可以看到里面的内容
    lfs_dir_t dir;

    // 文件/目录信息结构体，存储名称、大小、类型等信息
    // 类比：就像文件的"名片"，上面写着文件叫什么名字、多大、是文件还是文件夹
    struct lfs_info info;

    // 用于存储完整路径的缓冲区（最多128个字符）
    // 例如：如果当前在"/boot"，子目录是"data"，完整路径就是"/boot/data"
    char full_path[128];


    // ===== 打开目录 =====

    // 尝试打开path指定的目录，并把"钥匙"（句柄）存到dir中
    // 参数说明：
    //   &lfs - 文件系统对象
    //   &dir - 用于接收目录句柄的变量
    //   path - 要打开的目录路径
    // 返回值：LFS_ERR_OK表示成功，其他值表示失败
    if (lfs_dir_open(&lfs, &dir, path) != LFS_ERR_OK)
    {
        // 打开失败，打印错误信息
        my_printf(&huart1, "Failed to open directory: %s\r\n", path);

        // 直接返回，结束函数（这次递归调用结束）
        return;
    }


    // ===== 循环读取目录中的所有条目 =====

    // while(true)表示无限循环，直到遇到break才退出
    // 为什么用无限循环？因为我们不知道目录里有多少个文件/文件夹
    // 只能一个一个读，读完了就break退出
    while (true)
    {
        // 从dir句柄中读取下一个条目（文件或目录）的信息，存入info结构体
        // 就像翻书一样，每次翻一页，读取当前页的内容
        // 返回值：>0表示成功读取，<=0表示没有更多条目或出错
        int res = lfs_dir_read(&lfs, &dir, &info);

        if (res <= 0)
        {
            // 没有更多条目了，或者读取出错
            // 退出while循环
            break;
        }


        // ===== 过滤特殊目录 =====

        // 在Unix/Linux文件系统中：
        //   '.' 代表当前目录（自己）
        //   '..' 代表上级目录（父目录）
        // 这两个是系统自动生成的，我们不需要显示它们
        // strcmp()函数用于比较两个字符串是否相同，相同返回0
        if (strcmp(info.name, ".") == 0 || strcmp(info.name, "..") == 0)
        {
            // continue表示跳过本次循环，直接进入下一次循环
            // 也就是不处理'.'和'..'，继续读取下一个条目
            continue;
        }


        // ===== 打印缩进（树状结构的美化） =====

        // 根据递归层级level打印缩进
        // level=0（根目录）不缩进，level=1缩进4个空格，level=2缩进8个空格...
        // 这样可以形成树状结构的视觉效果
        for (int i = 0; i < level; i++)
        {
            // 每一层递归多打印4个空格
            my_printf(&huart1, "    ");
        }


        // ===== 判断是目录还是文件，并进行相应处理 =====

        // info.type存储了条目的类型
        if (info.type == LFS_TYPE_DIR)   // 如果是目录
        {
            // 打印目录名，[DIR]标签表示这是一个目录
            // +--是树状图的"树枝"符号
            my_printf(&huart1, "+-- [DIR] %s\r\n", info.name);


            // ===== 构造子目录的完整路径 =====

            // 为什么要分两种情况？
            // 因为根目录的路径是"/"，如果直接拼接会变成"//boot"（两个斜杠）
            // 所以要特殊处理根目录的情况

            if (strcmp(path, "/") == 0)  // 如果当前在根目录
            {
                // 直接用 "/子目录名" 的格式
                // 例如：info.name是"boot"，full_path就是"/boot"
                sprintf(full_path, "/%s", info.name);
            }
            else  // 如果不在根目录
            {
                // 用 "当前路径/子目录名" 的格式
                // 例如：path是"/boot"，info.name是"data"，full_path就是"/boot/data"
                sprintf(full_path, "%s/%s", path, info.name);
            }


            // ===== 递归调用，进入子目录 =====

            // ★★★ 这是递归的核心！★★★
            // 函数调用自己，进入刚才发现的子目录
            // level + 1 表示递归层级加1（深入一层）
            // 就像打开了一个文件夹，发现里面还有文件夹，继续打开...
            list_dir_recursive(full_path, level + 1);
        }
        else  // 否则是文件
        {
            // 打印文件名和文件大小
            // [FILE]标签表示这是一个文件
            // %lu表示无符号长整型（文件大小）
            my_printf(&huart1, "+-- [FILE] %s (%lu bytes)\r\n", info.name, (unsigned long)info.size);
        }
    } // while循环结束


    // ===== 关闭目录 =====

    // 读取完目录后，要关闭它，释放资源
    // 就像看完一个文件夹后要把它关上
    lfs_dir_close(&lfs, &dir);
}



/*======================================================================
 * 函数名：lfs_basic_test
 * 功能：LittleFS文件系统完整测试函数
 *
 * 测试流程：
 *   1. 挂载文件系统（如果失败则格式化后再挂载）
 *   2. 创建boot目录（如果不存在）
 *   3. 读取并更新启动计数器文件 boot/boot_cnt.txt
 *   4. 递归列出整个文件系统的目录结构
 *
 * 小白须知 - 什么是文件系统？
 * ----------------------------------------
 * Flash芯片本身只能做最基本的操作：擦除、读取、写入字节
 * 就像你有一块空白的纸，只能在上面写字、擦掉、重新写
 * 但是没有"文件"、"文件夹"这些概念
 *
 * LittleFS是一个轻量级的文件系统，就像给这张纸制定了规则：
 * - 这一块区域是目录信息
 * - 那一块区域是文件内容
 * - 文件名、大小、创建时间等信息存在哪里
 *
 * 有了文件系统，我们就可以像操作电脑硬盘一样操作Flash：
 * - 创建文件夹
 * - 创建文件
 * - 读写文件
 * - 删除文件
 * 而不用关心底层Flash的擦除、页对齐等复杂细节
 *
 * LittleFS的角色：翻译官
 * - 你说："我要创建一个文件"
 * - LittleFS说："好的，我来把它翻译成Flash的擦除、写入操作"
 *======================================================================*/
void lfs_basic_test(void)
{
    my_printf(&huart1, "\r\n--- LittleFS File System Test ---\r\n");


    // ========== 步骤1：挂载文件系统 ==========

    // 什么是"挂载"（mount）？
    // --------------------------------------------
    // 类比：就像把U盘插到电脑上，电脑识别后可以看到里面的文件
    // 挂载就是告诉程序："Flash上有一个文件系统，请把它加载进来，让我可以使用"
    //
    // 挂载过程：
    // 1. 读取Flash特定区域的文件系统元数据（metadata）
    // 2. 解析目录结构、文件信息
    // 3. 把这些信息加载到内存的lfs结构体中
    // 4. 之后就可以通过lfs来操作文件系统了

    // 尝试挂载文件系统
    // 参数：&lfs（文件系统对象）, &cfg（配置信息）
    // 返回值：0(LFS_ERR_OK)表示成功，非0表示失败
    int err = lfs_mount(&lfs, &cfg);

    // 检查挂载是否成功
    // 在C语言中，if(err)等价于if(err != 0)，即err不为0时条件成立
    if (err)  // 挂载失败
    {
        // 为什么会挂载失败？
        // 1. Flash是全新的，从来没有创建过文件系统（第一次使用）
        // 2. Flash数据损坏
        // 3. Flash被擦除过

        my_printf(&huart1, "LFS: Mount failed(%d), formatting...\n", err);

        // 解决方案：格式化Flash，创建一个新的文件系统
        // 类比：就像给新硬盘"分区格式化"，让它可以存储文件

        // lfs_format：格式化Flash，创建文件系统的基础结构
        // 然后再次尝试挂载
        // || 是"或"运算符，任意一个失败就进入if
        if (lfs_format(&lfs, &cfg) || (err = lfs_mount(&lfs, &cfg)))
        {
            // 格式化或挂载还是失败，说明硬件有问题
            my_printf(&huart1, "LFS: Format/Mount failed(%d)!\n", err);
            return;  // 直接返回，不再继续测试
        }

        // 格式化并挂载成功
        my_printf(&huart1, "LFS: Format & Mount OK.\n");
    }
    else  // 挂载成功（err == 0）
    {
        // 说明Flash上已经有文件系统，并且成功加载了
        my_printf(&huart1, "LFS: Mount successful.\n");
    }



    // ========== 步骤2：创建目录 ==========

    // 尝试创建一个名为"boot"的目录
    // lfs_mkdir类似于Windows的"新建文件夹"或Linux的mkdir命令
    err = lfs_mkdir(&lfs, "boot");

    // 处理创建结果
    // 情况1：创建失败，且不是因为目录已存在
    // 情况2：创建成功
    // 情况3：创建失败，但是因为目录已经存在（这种情况我们可以接受）

    // && 是"与"运算符，两个条件都成立才进入if
    // err != LFS_ERR_EXIST 表示错误不是"目录已存在"
    if (err && err != LFS_ERR_EXIST)
    {
        // 创建失败，且不是因为已存在，说明有其他问题
        my_printf(&huart1, "LFS: Failed to create 'boot' directory(%d)!\n", err);

        // goto语句：跳转到end_test标签处
        // 为什么用goto？因为后面还有清理工作（比如卸载文件系统）
        // goto可以让所有出错的地方都跳到同一个清理代码，避免重复写代码
        goto end_test;
    }

    if (err == LFS_ERR_OK)  // 创建成功（目录原本不存在，现在创建了）
    {
        my_printf(&huart1, "LFS: Directory 'boot' created successfully.\n");
    }
    // 注意：如果err == LFS_ERR_EXIST，说明目录已经存在，我们不打印任何信息
    // 这是正常情况（第二次及以后运行程序时会遇到）


    // ========== 步骤3：读写文件 - 启动计数器 ==========

    // 这个功能的目的：
    // 每次MCU重启时，读取文件中的计数，加1后再写回去
    // 这样就可以知道设备重启了多少次

    // 定义一个32位无符号整数，用于存储启动次数
    // uint32_t表示0到4,294,967,295的整数
    uint32_t boot_count = 0;

    // 定义文件操作句柄，类似于dir是目录的"钥匙"，file是文件的"钥匙"
    lfs_file_t file;

    // 定义文件路径：boot目录下的boot_cnt.txt文件
    // 注意：这是一个文本文件的名字，但我们存储的是二进制数据
    const char *filename = "boot/boot_cnt.txt";

    // ----- 3.1 打开文件 -----

    // 打开文件，如果不存在则创建
    // 标志位说明：
    //   LFS_O_RDWR: Read/Write 读写模式（既可以读也可以写）
    //   LFS_O_CREAT: Create 如果文件不存在，则创建它
    // | 是位或运算符，用于组合多个标志位
    err = lfs_file_open(&lfs, &file, filename, LFS_O_RDWR | LFS_O_CREAT);

    if (err)  // 打开失败
    {
        my_printf(&huart1, "LFS: Failed to open file '%s'(%d)!\n", filename, err);
        goto end_test;  // 跳转到清理代码
    }


    // ----- 3.2 读取文件内容 -----

    // 从文件中读取数据到boot_count变量
    // 参数说明：
    //   &lfs - 文件系统对象
    //   &file - 文件句柄
    //   &boot_count - 目标缓冲区（把数据读到这里）
    //   sizeof(boot_count) - 要读取的字节数（4字节，因为uint32_t是4字节）
    // 返回值：实际读取的字节数，负数表示出错
    lfs_ssize_t r_sz = lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // 处理读取结果
    if (r_sz < 0)  // 读取出错（返回负数）
    {
        my_printf(&huart1, "LFS: Failed to read file '%s'(%ld), initializing counter.\n", filename, (long)r_sz);
        boot_count = 0;  // 初始化为0
    }
    else if (r_sz != sizeof(boot_count))  // 读取的字节数不对
    {
        // 为什么会不对？
        // 1. 文件是空的（第一次创建）- r_sz会是0
        // 2. 文件数据损坏
        my_printf(&huart1, "LFS: Read %ld bytes from '%s' (expected %d), initializing counter.\n", (long)r_sz, filename, (int)sizeof(boot_count));
        boot_count = 0;  // 初始化为0
    }
    // 如果r_sz == sizeof(boot_count)，说明读取成功，boot_count已经有正确的值


    // ----- 3.3 更新计数器 -----

    // 计数加1
    boot_count++;

    // 打印当前的启动次数
    my_printf(&huart1, "LFS: File '%s' current boot count: %lu\n", filename, boot_count);


    // ----- 3.4 将文件指针移回开头 -----

    // 什么是文件指针？
    // 文件指针就像读书时的书签，标记当前读到哪里或要写到哪里
    // 刚才读完数据后，文件指针在文件末尾
    // 现在要写入数据，需要把指针移回开头，覆盖原来的内容

    // lfs_file_rewind: 将文件指针移到文件开头
    err = lfs_file_rewind(&lfs, &file);
    if (err)
    {
        my_printf(&huart1, "LFS: Failed to rewind file '%s'(%d)!\n", filename, err);
        lfs_file_close(&lfs, &file);  // 关闭文件
        goto end_test;
    }


    // ----- 3.5 写入更新后的计数 -----

    // 将更新后的boot_count写入文件
    // 参数和lfs_file_read类似，但方向相反（从内存写到文件）
    lfs_ssize_t w_sz = lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // 处理写入结果
    if (w_sz < 0)  // 写入出错
    {
        my_printf(&huart1, "LFS: Failed to write file '%s'(%ld)!\n", filename, (long)w_sz);
    }
    else if (w_sz != sizeof(boot_count))  // 部分写入（没写完整）
    {
        my_printf(&huart1, "LFS: Partial write to '%s' (%ld/%d bytes)!\n", filename, (long)w_sz, (int)sizeof(boot_count));
    }
    else  // 写入成功
    {
        my_printf(&huart1, "LFS: File '%s' updated successfully.\n", filename);
    }


    // ----- 3.6 关闭文件 -----

    // 关闭文件，释放资源
    // 类比：用完文件后要"关上"，就像看完书要合上书一样
    // lfs_file_close返回0表示成功，非0表示失败
    if (lfs_file_close(&lfs, &file))
    {
        my_printf(&huart1, "LFS: Failed to close file '%s'!\n", filename);
    }

    // ========== 步骤4：显示文件系统结构 ==========

    // 调用前面定义的递归函数，以树状图的形式显示整个文件系统
    my_printf(&huart1, "\r\n[File System Structure]\r\n");
    my_printf(&huart1, "/ (root directory)\r\n");

    // 从根目录"/"开始遍历，初始层级为0
    list_dir_recursive("/", 0);



    // ========== 测试结束标签 ==========

    // 这是一个标签（label），goto语句会跳转到这里
    // 所有的错误处理都会跳到这里，执行统一的清理工作
end_test:
    my_printf(&huart1, "--- LittleFS File System Test End ---\r\n");

    // 可选：卸载文件系统
    // 在嵌入式系统中，如果MCU会复位重启，通常不需要卸载
    // 但如果程序会继续运行，最好卸载以保证数据完整性
    // lfs_unmount(&lfs);
}











/*======================================================================
 * SPI Flash硬件配置说明（使用STM32CubeMX配置）
 *======================================================================
 *
 * 什么是SPI？
 * ----------------------------------------
 * SPI (Serial Peripheral Interface) 串行外设接口
 * 是一种同步串行通信协议，用于MCU和外部设备（如Flash芯片）之间的通信
 *
 * SPI通信需要4根线：
 *   1. SCK (Serial Clock)    - 时钟线，由主设备（MCU）提供
 *   2. MOSI (Master Out Slave In) - 主设备发送数据线
 *   3. MISO (Master In Slave Out) - 主设备接收数据线
 *   4. CS/NSS (Chip Select)  - 片选线，选中要通信的设备
 *
 * CubeMX配置参数：
 * ----------------------------------------
 * 1. 模式：全双工（Full Duplex）
 *    - 可以同时发送和接收数据
 *
 * 2. 帧格式：Motorola
 *    - SPI有多种帧格式，Motorola是最常用的一种
 *
 * 3. 数据大小：8位
 *    - 每次传输8个比特（1个字节）
 *    - 根据Flash芯片的寄存器大小决定
 *
 * 4. 位顺序：MSB First（高位先行）
 *    - 一个字节的最高位（bit7）先传输
 *    - 查看Flash芯片数据手册确定
 *
 * 5. 时钟频率：
 *    - 根据Flash芯片支持的最大频率配置
 *    - 不能超过芯片的最大SPI时钟频率
 *
 * 6. SPI模式：
 *    - SPI有4种模式（Mode 0-3），由CPOL和CPHA两个参数决定
 *    - CPOL: 时钟极性（空闲时时钟是高电平还是低电平）
 *    - CPHA: 时钟相位（在第几个时钟沿采样数据）
 *    - 查看Flash芯片手册，确定支持哪种模式（通常是Mode 0或Mode 3）
 *
 * 7. 片选管理：软件方式
 *    - 片选信号由软件控制（GPIO输出）
 *    - 配置片选引脚为推挽输出模式
 *
 * 8. CRC校验：禁止
 *    - 不使用硬件CRC校验功能
 *
 * 注意事项：
 * - 修改驱动文件中的片选引脚定义（通常在.h文件中）
 * - 确保SPI外设和GPIO已通过CubeMX正确初始化
 *======================================================================*/

// 假设SPI2和相关GPIO已通过CubeMX初始化
// extern SPI_HandleTypeDef hspi2; (确保在main.c或对应位置有定义)



/*======================================================================
 * 函数名：test_spi_flash
 * 功能：测试SPI Flash芯片的基本读写功能
 *
 * 测试步骤：
 *   1. 初始化SPI Flash
 *   2. 读取Flash ID（验证通信是否正常）
 *   3. 擦除一个扇区
 *   4. 校验擦除（检查是否全为0xFF）
 *   5. 写入测试数据
 *   6. 读取数据
 *   7. 校验数据（比较写入和读取的数据是否一致）
 *
 * 小白须知 - Flash的特性：
 * ----------------------------------------
 * 1. Flash只能从1写成0，不能从0写成1
 *    所以写入之前必须先擦除（擦除后所有位都是1，即0xFF）
 *
 * 2. 擦除的最小单位是扇区（Sector）
 *    - 一个扇区通常是4KB（4096字节）
 *    - 也可以擦除更大的块（Block）或整个芯片
 *
 * 3. 写入的最小单位是页（Page）
 *    - 一个页通常是256字节
 *    - 写入操作不能跨页
 *
 * 4. 地址对齐
 *    - 虽然可以从任意地址写入，但从页起始地址开始更规范
 *    - 页地址：0x000000, 0x000100, 0x000200...（每256字节递增）
 *======================================================================*/
void test_spi_flash(void)
{
    // ===== 变量声明 =====

    // 用于存储Flash芯片的ID
    // Flash ID是芯片的"身份证"，可以用来识别芯片型号
    uint32_t flash_id;

    // 写缓冲区，用于存储要写入Flash的数据
    // SPI_FLASH_PAGE_SIZE通常是256字节
    uint8_t write_buffer[SPI_FLASH_PAGE_SIZE];

    // 读缓冲区，用于存储从Flash读取的数据
    uint8_t read_buffer[SPI_FLASH_PAGE_SIZE];


    // 测试地址
    // 选择Flash的起始地址进行测试
    // 0x000000 = 0（十进制），是第一个扇区的起始地址
    //
    // 地址选择的讲究：
    // 虽然Flash芯片能处理页内任意地址的写入，
    // 但从页的起始地址开始写入更规范，也方便理解数据组织
    // 页起始地址：0x000000, 0x000100(256), 0x000200(512), 0x000300(768)...
    // 每256字节（0x100）递增一个页
    uint32_t test_addr = 0x000000;


    my_printf(&huart1,"SPI FLASH Test Start\r\n");


    // ========== 步骤1：初始化SPI Flash ==========

    // 初始化SPI Flash驱动
    // 主要工作：设置片选(CS)引脚为高电平（未选中状态）
    // 为什么CS默认要高电平？
    // - CS低电平表示选中芯片，芯片会响应SPI命令
    // - CS高电平表示未选中，芯片进入空闲状态
    // - 初始化时应该让芯片处于未选中状态
    spi_flash_init();
    my_printf(&huart1,"SPI Flash Initialized.\r\n");



    // ========== 步骤2：读取Flash ID（验证通信） ==========

    // 读取Flash芯片的ID
    // Flash ID的作用：
    // 1. 验证SPI通信是否正常
    // 2. 识别Flash芯片的型号和容量
    // 3. 检测硬件连接是否正确
    //
    // Flash ID的两种读取命令：
    // ----------------------------------------
    // 命令0x9F (JEDEC ID):
    //   - 返回3字节的标准ID
    //   - 格式：[厂商ID][设备ID高字节][设备ID低字节]
    //   - 包含容量信息（例如：0x15代表16Mbit）
    //   - 这是更标准、更通用的方法 ★推荐使用★
    //
    // 命令0x90 (Manufacturer/Device ID):
    //   - 返回2字节的ID
    //   - 格式：[厂商ID][设备型号]
    //   - 包含具体型号信息（例如：0x14代表GD25Q16）
    //
    // 这两个命令访问芯片内部不同的信息区域，
    // 返回不同的值是正常的，各有各的用途
    //
    // 本代码使用0x9F命令读取JEDEC ID
    flash_id = spi_flash_read_id();
    my_printf(&huart1,"Flash ID: 0x%lX\r\n", flash_id);

    // 如何判断ID是否正确？
    // 查看Flash芯片的数据手册（Datasheet）
    // 例如：
    //   GD25Q64: ID可能是 0xC84017
    //   GD25Q16: ID可能是 0xC84015
    //   W25Q64:  ID可能是 0xEF4017
    // 如果读到的ID是0xFFFFFF或0x000000，说明通信有问题



    // ========== 步骤3：擦除扇区 ==========

    // Flash写入前必须先擦除！
    // ----------------------------------------
    // 为什么要擦除？
    // - Flash的特性：只能把1改成0，不能把0改成1
    // - 擦除操作会把所有位设置为1（即0xFF）
    // - 然后写入时可以把需要的位改成0
    //
    // 擦除单位：
    // - 扇区（Sector）：通常4KB（4096字节）★最常用★
    // - 块（Block）：通常64KB
    // - 整片擦除（Chip Erase）：擦除整个Flash
    //
    // 注意：擦除操作比较慢！
    // - 扇区擦除：几十到几百毫秒
    // - 整片擦除：可能需要几秒到几十秒

    my_printf(&huart1,"Erasing sector at address 0x%lX...\r\n", test_addr);

    // 擦除test_addr所在的扇区（4KB）
    // 例如：test_addr=0x000000，擦除0x000000-0x000FFF这4096个字节
    spi_flash_sector_erase(test_addr);

    my_printf(&huart1,"Sector erased.\r\n");



    // ========== 步骤4：校验擦除（可选但推荐） ==========

    // 读取刚才擦除的区域，检查是否真的擦除成功
    // 擦除成功的标志：所有字节都是0xFF

    // 从test_addr读取一页数据到read_buffer
    spi_flash_buffer_read(read_buffer, test_addr, SPI_FLASH_PAGE_SIZE);

    // 设置一个标志变量，假设擦除检查通过
    int erased_check_ok = 1;

    // 遍历读取的数据，检查是否全为0xFF
    for (int i = 0; i < SPI_FLASH_PAGE_SIZE; i++) {
        if (read_buffer[i] != 0xFF) {
            // 发现有不是0xFF的字节，说明擦除失败
            erased_check_ok = 0;
            break;  // 退出循环，不用再检查了
        }
    }

    // 打印检查结果
    if (erased_check_ok) {
        my_printf(&huart1,"Erase check PASSED. Sector is all 0xFF.\r\n");
    } else {
        my_printf(&huart1,"Erase check FAILED.\r\n");
    }



    // ========== 步骤5：准备并写入测试数据 ==========

    // ----- 5.1 准备数据 -----

    // 定义要写入的测试字符串
    const char* message = "Hello from STM32 to SPI FLASH! Microunion Studio Test - 12345.";

    // 计算字符串长度（不包括结尾的'\0'）
    uint16_t data_len = strlen(message);

    // 检查长度是否超过页大小
    if (data_len >= SPI_FLASH_PAGE_SIZE) {
        // 如果太长，截断到页大小-1（留一个字节给'\0'）
        data_len = SPI_FLASH_PAGE_SIZE - 1;
    }

    // 将write_buffer清零（填充0x00）
    // memset(目标缓冲区, 填充值, 字节数)
    memset(write_buffer, 0, SPI_FLASH_PAGE_SIZE);

    // 将message复制到write_buffer
    // memcpy(目标, 源, 字节数)
    memcpy(write_buffer, message, data_len);

    // 在字符串末尾添加结束符'\0'
    // 为什么要加？因为C语言的字符串必须以'\0'结尾
    write_buffer[data_len] = '\0';


    // ----- 5.2 写入数据 -----

    my_printf(&huart1,"Writing data to address 0x%lX: \"%s\"\r\n", test_addr, write_buffer);

    // 写入数据到Flash
    // 有两种写入函数可选：
    //
    // 1. spi_flash_buffer_write - 智能写入（推荐）
    //    - 可以处理跨页写入（自动分页）
    //    - 写入任意长度的数据
    //    - 本例使用这个函数
    //
    // 2. spi_flash_page_write - 页写入
    //    - 只能在一页内写入
    //    - 不能跨页（如果跨页会出错）
    //    - 适合确定数据在一页内的情况

    // 使用buffer_write写入整页数据（包括填充的0）
    spi_flash_buffer_write(write_buffer, test_addr, SPI_FLASH_PAGE_SIZE);

    // 如果只想写入有效数据（不包括填充的0），可以用：
    // spi_flash_page_write(write_buffer, test_addr, data_len + 1);

    my_printf(&huart1,"Data written.\r\n");

    // ========== 步骤6：读取数据 ==========

    my_printf(&huart1,"Reading data from address 0x%lX...\r\n", test_addr);

    // 先清空读缓冲区
    // 为什么要清空？
    // 确保读到的数据是从Flash来的，而不是缓冲区里的残留数据
    memset(read_buffer, 0, SPI_FLASH_PAGE_SIZE);

    // 从Flash读取数据到read_buffer
    // 参数：
    //   read_buffer - 接收数据的缓冲区
    //   test_addr - 读取的起始地址
    //   SPI_FLASH_PAGE_SIZE - 要读取的字节数
    spi_flash_buffer_read(read_buffer, test_addr, SPI_FLASH_PAGE_SIZE);

    // 打印读取的数据（当作字符串打印）
    my_printf(&huart1,"Data read: \"%s\"\r\n", read_buffer);



    // ========== 步骤7：校验数据 ==========

    // 比较写入的数据和读取的数据是否完全一致
    // memcmp(缓冲区1, 缓冲区2, 比较字节数)
    // 返回值：
    //   0 - 两个缓冲区完全相同
    //   非0 - 两个缓冲区不同
    if (memcmp(write_buffer, read_buffer, SPI_FLASH_PAGE_SIZE) == 0) {
        // 数据一致，测试成功！
        my_printf(&huart1,"Data VERIFIED! Write and Read successful.\r\n");
    } else {
        // 数据不一致，测试失败
        // 可能的原因：
        // 1. Flash硬件故障
        // 2. SPI通信不稳定
        // 3. 擦除不完全
        // 4. 地址计算错误
        my_printf(&huart1,"Data VERIFICATION FAILED!\r\n");
    }


    // ========== 测试结束 ==========

    my_printf(&huart1,"SPI FLASH Test End\r\n");

    // 测试完成！
    // 如果看到"Data VERIFIED!"，说明Flash读写功能正常
    // 可以放心在项目中使用Flash存储数据了
}


/*======================================================================
 * 文件总结 - 给小白的学习指南
 *======================================================================
 *
 * 这个文件包含了两个主要功能模块：
 *
 * 【模块1：LittleFS文件系统】
 * ----------------------------------------
 * 函数：lfs_init(), lfs_basic_test(), list_dir_recursive()
 *
 * 核心概念：
 * 1. Flash本身只是一块存储芯片，没有"文件"、"目录"的概念
 * 2. LittleFS是一个轻量级文件系统，让我们可以像操作电脑硬盘一样操作Flash
 * 3. 文件系统的作用：把底层的Flash读写操作，翻译成高级的文件操作
 *
 * 主要操作：
 * - 挂载/格式化文件系统
 * - 创建目录
 * - 读写文件
 * - 遍历目录结构（递归）
 *
 * 类比理解：
 * Flash芯片 = 一块空白硬盘
 * LittleFS = Windows/Linux操作系统的文件管理功能
 * lfs_mount = 把U盘插到电脑上
 * lfs_mkdir = 新建文件夹
 * lfs_file_open/read/write = 打开/读取/写入文件
 *
 *
 * 【模块2：SPI Flash底层测试】
 * ----------------------------------------
 * 函数：test_spi_flash()
 *
 * 核心概念：
 * 1. SPI是一种串行通信协议，用于MCU和Flash芯片之间的通信
 * 2. Flash的基本操作：读取ID、擦除、写入、读取
 * 3. Flash的特性：写之前必须擦除，擦除后所有位都是1（0xFF）
 *
 * 主要操作：
 * - 读取Flash ID（验证通信）
 * - 擦除扇区（4KB）
 * - 写入数据（按页写入，256字节）
 * - 读取数据
 * - 校验数据
 *
 * 类比理解：
 * SPI通信 = 你和朋友的对讲机
 * Flash ID = 芯片的"身份证"
 * 擦除 = 用橡皮擦掉铅笔字
 * 写入 = 用铅笔写字
 * 读取 = 看写的是什么
 *
 *
 * 【两个模块的关系】
 * ----------------------------------------
 * test_spi_flash() - 底层硬件测试
 *   ↓
 * 验证Flash芯片硬件工作正常
 *   ↓
 * lfs_basic_test() - 文件系统测试
 *   ↓
 * 在Flash上建立文件系统，实现高级文件操作
 *
 * 就像：
 * 先检查硬盘能不能用（test_spi_flash）
 * 再给硬盘安装文件系统（lfs_basic_test）
 *
 *
 * 【学习建议】
 * ----------------------------------------
 * 1. 先理解test_spi_flash()，掌握Flash的基本读写
 * 2. 再学习lfs_basic_test()，理解文件系统的概念
 * 3. 重点理解递归函数list_dir_recursive()，这是编程中的重要概念
 * 4. 动手实验：修改测试数据，观察串口输出，加深理解
 *
 * 【常见问题】
 * ----------------------------------------
 * Q: 为什么Flash写之前要擦除？
 * A: Flash的物理特性决定的，只能把1改成0，不能把0改成1
 *    擦除操作把所有位设为1，写入时再把需要的位改成0
 *
 * Q: 为什么要用文件系统？直接读写Flash不行吗？
 * A: 可以，但文件系统提供了更方便的接口：
 *    - 不用手动管理地址
 *    - 支持文件名、目录
 *    - 自动处理磨损均衡
 *    - 提供断电保护
 *
 * Q: 递归函数是什么？
 * A: 函数调用自己。就像俄罗斯套娃，打开一个还有一个
 *    list_dir_recursive遍历目录时，遇到子目录就调用自己去遍历
 *
 * Q: 为什么要校验数据？
 * A: 确保写入和读取的数据一致，验证Flash工作正常
 *    在实际项目中，数据校验非常重要，可以发现硬件故障
 *
 *======================================================================*/




