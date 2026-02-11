#include "wououi_app.h"



/*-------------------------------------PID参数的范围及步进值-------------------------------------------*/
#define PID_PARAM_P_MIN 0    // P 参数最小值 (0.00)
#define PID_PARAM_P_MAX 1000 // P 参数最大值 (10.00)
#define PID_PARAM_P_STEP 10  // P 参数步进值 (0.10)

#define PID_PARAM_I_MIN 0    // I 参数最小值 (0.00)
#define PID_PARAM_I_MAX 1000 // I 参数最大值 (10.00)
#define PID_PARAM_I_STEP 10  // I 参数步进值 (0.10)

#define PID_PARAM_D_MIN 0   // D 参数最小值 (0.00)
#define PID_PARAM_D_MAX 100 // D 参数最大值 (1.00)
#define PID_PARAM_D_STEP 1  // D 参数步进值 (0.01)



/*-------------------------------------PID参数的界面和参数设定-------------------------------------------*/

// 页面对象
TitlePage main_menu;         // 第一级菜单（左右轮选择）
ListPage left_wheel_menu;    // 左轮的PID参数选择菜单
ListPage right_wheel_menu;   // 右轮的PID参数选择菜单
ValWin param_adjust_val_win; // 使用 ValWin 进行参数调整

// 左右轮PID参数数据
typedef struct
{
  float p;
  float i;
  float d;
} PIDParams;

//定义左右轮PID参数
PIDParams left_wheel_pid = {1.0, 0.1, 0.01};
PIDParams right_wheel_pid = {1.0, 0.1, 0.01};



/*------------------------------------------第一级菜单选项(左右轮选择)------------------------------------------------ */
#define MAIN_MENU_NUM 2

Option main_menu_options[MAIN_MENU_NUM] = {
    {.text = (char *)"+ Left Wheel", .content = (char *)"Left"},
    {.text = (char *)"+ Right Wheel", .content = (char *)"Right"}};

// 第一级菜单图标
Icon main_menu_icons[MAIN_MENU_NUM] = {
    [0] = {0xFC, 0xFE, 0xFF, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
           0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x07, 0x07, 0x0F, 0x1F, 0x3F, 0xFF, 0xFE, 0xFC, 0xFF, 0x01,
           0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xFC, 0xFC,
           0x00, 0x00, 0xFC, 0xFC, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFF, 0xFF, 0xF0, 0xC0, 0x00,
           0x00, 0x00, 0x03, 0x07, 0x0F, 0x1F, 0x3E, 0x3C, 0x3C, 0x3C, 0x1E, 0x1F, 0x0F, 0x03, 0x00, 0x00,
           0x1F, 0x3F, 0x3F, 0x1F, 0x00, 0x00, 0x00, 0xC0, 0xF0, 0xFF, 0xCF, 0xDF, 0xFF, 0xFF, 0xFE, 0xFC,
           0xF8, 0xF8, 0xF0, 0xF0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xF0, 0xF0,
           0xF8, 0xF8, 0xFC, 0xFE, 0xFF, 0xFF, 0xDF, 0xCF}, // logo
    [1] = {0xFC, 0xFE, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x83, 0x81, 0x01, 0x01, 0x81, 0xE1, 0xE1, 0xE1,
           0xE1, 0x81, 0x01, 0x81, 0x81, 0x83, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFE, 0xFC, 0xFF, 0x01,
           0x00, 0x00, 0x00, 0xE0, 0xE0, 0xF3, 0xFF, 0xFF, 0x3F, 0x0F, 0x07, 0x07, 0x03, 0x03, 0x07, 0x07,
           0x0F, 0x3F, 0xFF, 0xFF, 0xF7, 0xE0, 0xE0, 0x00, 0x00, 0x00, 0x01, 0xFF, 0xFF, 0xE0, 0x80, 0x00,
           0x00, 0x01, 0x01, 0x3B, 0x7F, 0x7F, 0x7F, 0x3C, 0x78, 0xF8, 0xF0, 0xF0, 0xF8, 0x78, 0x3C, 0x3F,
           0x7F, 0x7F, 0x33, 0x01, 0x01, 0x00, 0x00, 0x80, 0xE0, 0xFF, 0xCF, 0xDF, 0xFF, 0xFF, 0xFE, 0xFC,
           0xF8, 0xF0, 0xF0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE1, 0xE1, 0xE1, 0xE1, 0xE0, 0xE0, 0xE0, 0xE0, 0xF0,
           0xF0, 0xF8, 0xFC, 0xFC, 0xFF, 0xFF, 0xDF, 0xCF} // Setting
};


/*------------------------------------------第二级菜单选项(左右轮参数设定)------------------------------------------------ */
// 左轮PID菜单选项
Option left_wheel_options[] = {
    {.text = (char *)"- Left Wheel PID"},
    {.text = (char *)"$ P Parameter", .val = 0, .decimalNum = DecimalNum_2},
    {.text = (char *)"$ I Parameter", .val = 0, .decimalNum = DecimalNum_2},
    {.text = (char *)"$ D Parameter", .val = 0, .decimalNum = DecimalNum_2}};

// 右轮PID菜单选项
Option right_wheel_options[] = {
    {.text = (char *)"- Right Wheel PID"},
    {.text = (char *)"$ P Parameter", .val = 0, .decimalNum = DecimalNum_2},
    {.text = (char *)"$ I Parameter", .val = 0, .decimalNum = DecimalNum_2},
    {.text = (char *)"$ D Parameter", .val = 0, .decimalNum = DecimalNum_2}};




/*---------------------------------------------------主菜单回调函数------------------------------------------------- */
bool MainMenu_CallBack(const Page *cur_page_addr, InputMsg msg)
{
  //如果点击确认
  if (msg_click == msg)
  {
    //返回现在选择的选项
    Option *select_item = WouoUI_ListTitlePageGetSelectOpt(cur_page_addr);

    if (!strcmp(select_item->content, "Left"))
    {
      // 更新左轮菜单的参数值
      left_wheel_options[1].val = (int32_t)(left_wheel_pid.p * 100);
      left_wheel_options[2].val = (int32_t)(left_wheel_pid.i * 100);
      left_wheel_options[3].val = (int32_t)(left_wheel_pid.d * 100);

      //跳到左轮参数设置页面
      WouoUI_JumpToPage((PageAddr)cur_page_addr, &left_wheel_menu);
    }

    else if (!strcmp(select_item->content, "Right"))
    {
      // 更新右轮菜单的参数值
      right_wheel_options[1].val = (int32_t)(right_wheel_pid.p * 100);
      right_wheel_options[2].val = (int32_t)(right_wheel_pid.i * 100);
      right_wheel_options[3].val = (int32_t)(right_wheel_pid.d * 100);

      //跳到右轮设置页面
      WouoUI_JumpToPage((PageAddr)cur_page_addr, &right_wheel_menu);
    }
  }

  return false;
}



/*---------------------------------------------------左右轮菜单回调函数--------------------------------------------------*/

//对左轮菜单进行操作
bool LeftWheelMenu_CallBack(const Page *cur_page_addr, InputMsg msg)
{
  //点击确认
  if (msg_click == msg)
  {
    Option *select_item = WouoUI_ListTitlePageGetSelectOpt(cur_page_addr);
    if (select_item->order >= 1 && select_item->order <= 3)
    {
      // 根据选择的参数类型设置范围和步长
      int min_val, max_val, step;
      switch (select_item->order)
      {
      case 1: // P Parameter
        min_val = PID_PARAM_P_MIN;
        max_val = PID_PARAM_P_MAX;
        step = PID_PARAM_P_STEP;
        break;
      case 2: // I Parameter
        min_val = PID_PARAM_I_MIN;
        max_val = PID_PARAM_I_MAX;
        step = PID_PARAM_I_STEP;
        break;
      case 3: // D Parameter
        min_val = PID_PARAM_D_MIN;
        max_val = PID_PARAM_D_MAX;
        step = PID_PARAM_D_STEP;
        break;
      default: // 不应该发生
        return false;
      }

      // 设置 ValWin 参数并跳转
      WouoUI_ValWinPageSetMinStepMax(&param_adjust_val_win, min_val, step, max_val);

      WouoUI_JumpToPage((PageAddr)cur_page_addr, &param_adjust_val_win);
    }
  }
  return false;
}


// 对右轮菜单进行操作
bool RightWheelMenu_CallBack(const Page *cur_page_addr, InputMsg msg)
{
  if (msg_click == msg)
  {
    Option *select_item = WouoUI_ListTitlePageGetSelectOpt(cur_page_addr);
    if (select_item->order >= 1 && select_item->order <= 3)
    {
      // 根据选择的参数类型设置范围和步长
      int min_val, max_val, step;
      switch (select_item->order)
      {
      case 1: // P Parameter
        min_val = PID_PARAM_P_MIN;
        max_val = PID_PARAM_P_MAX;
        step = PID_PARAM_P_STEP;
        break;
      case 2: // I Parameter
        min_val = PID_PARAM_I_MIN;
        max_val = PID_PARAM_I_MAX;
        step = PID_PARAM_I_STEP;
        break;
      case 3: // D Parameter
        min_val = PID_PARAM_D_MIN;
        max_val = PID_PARAM_D_MAX;
        step = PID_PARAM_D_STEP;
        break;
      default: // 不应该发生
        return false;
      }

      // 设置 ValWin 参数并跳转
      WouoUI_ValWinPageSetMinStepMax(&param_adjust_val_win, min_val, step, max_val);
      WouoUI_JumpToPage((PageAddr)cur_page_addr, &param_adjust_val_win);
    }
  }
  return false;
}

/*---------------------------------------------------参数调整回调函数-------------------------------------------------------- */

// 参数调整弹窗回调函数 (修改为处理 ValWin)
bool ParamAdjust_CallBack(const Page *cur_page_addr, InputMsg msg)
{
  // 获取 ValWin 实例和其父页面 (菜单)
  ValWin *val_win = (ValWin *)cur_page_addr;
  Page *parent = (Page *)val_win->page.last_page;
  Option *select_opt = WouoUI_ListTitlePageGetSelectOpt(parent); // 获取来源菜单选中的选项

  // 根据输入消息调整数值
  if (msg == msg_up || msg == msg_right)
  { // 假设上/右是增加
    WouoUI_ValWinPageValIncrease(val_win);
    return true; // 消息已处理，阻止页面自动返回
  }
  else if (msg == msg_down || msg == msg_left)
  { // 假设下/左是减少
    WouoUI_ValWinPageValDecrease(val_win);
    return true; // 消息已处理，阻止页面自动返回
  }
  else if (msg == msg_click)
  { // 点击确认
    // 判断是左轮还是右轮菜单
    if (parent == (Page *)&left_wheel_menu)
    {
      // 更新左轮PID参数 (注意 val 是 int32_t，需转为 float)
      switch (select_opt->order)
      {
      case 1:
        left_wheel_pid.p = val_win->val / 100.0f;
        break;
      case 2:
        left_wheel_pid.i = val_win->val / 100.0f;
        break;
      case 3:
        left_wheel_pid.d = val_win->val / 100.0f;
        break;
      }
      // 更新菜单显示值 (ValWin 开启了 auto_set_bg_opt, 会自动更新，这里无需手动更新)
      // left_wheel_options[select_opt->order].val = val_win->val; // 理论上不需要
    }
    else if (parent == (Page *)&right_wheel_menu)
    {
      // 更新右轮PID参数
      switch (select_opt->order)
      {
      case 1:
        right_wheel_pid.p = val_win->val / 100.0f;
        break;
      case 2:
        right_wheel_pid.i = val_win->val / 100.0f;
        break;
      case 3:
        right_wheel_pid.d = val_win->val / 100.0f;
        break;
      }
      // 更新菜单显示值 (ValWin 开启了 auto_set_bg_opt, 会自动更新，这里无需手动更新)
      // right_wheel_options[select_opt->order].val = val_win->val; // 理论上不需要
    }
    // 确认后，不需要 return true，让页面自动返回
  }
  // 对于 msg_return 或其他未处理的消息，返回 false，让页面自动处理返回逻辑
  return false;
}


void PIDMenu_Init(void)
{
  // 选择默认UI
  WouoUI_SelectDefaultUI();

  // 清空缓存并刷新屏幕
  WouoUI_BuffClear();
  WouoUI_BuffSend();
  WouoUI_GraphSetPenColor(1);

  // 初始化菜单选项的值
  left_wheel_options[1].val = (int32_t)(left_wheel_pid.p * 100);
  left_wheel_options[2].val = (int32_t)(left_wheel_pid.i * 100);
  left_wheel_options[3].val = (int32_t)(left_wheel_pid.d * 100);

  right_wheel_options[1].val = (int32_t)(right_wheel_pid.p * 100);
  right_wheel_options[2].val = (int32_t)(right_wheel_pid.i * 100);
  right_wheel_options[3].val = (int32_t)(right_wheel_pid.d * 100);

  // 初始化页面对象
  WouoUI_TitlePageInit(&main_menu, MAIN_MENU_NUM, main_menu_options, main_menu_icons, MainMenu_CallBack);
  WouoUI_ListPageInit(&left_wheel_menu, sizeof(left_wheel_options) / sizeof(Option), left_wheel_options, Setting_none, LeftWheelMenu_CallBack);
  WouoUI_ListPageInit(&right_wheel_menu, sizeof(right_wheel_options) / sizeof(Option), right_wheel_options, Setting_none, RightWheelMenu_CallBack);
  // WouoUI_SpinWinPageInit(&param_adjust_win, NULL, 0, DecimalNum_2, 0, 1000, true, true, ParamAdjust_CallBack); // 移除 SpinWin 初始化
  // 初始化 ValWin 页面
  // text: NULL (将由 auto_get_bg_opt 自动设置)
  // init_val: 0 (将由 auto_get_bg_opt 自动设置)
  // min, max, step: 将在跳转前由 WouoUI_ValWinPageSetMinStepMax 设置
  // auto_get_bg_opt: true (自动获取父页面选项的 text 和 val)
  // auto_set_bg_opt: true (确认时自动将 val 写回父页面选项)
  // cb: ParamAdjust_CallBack (使用同一个回调函数)
  WouoUI_ValWinPageInit(&param_adjust_val_win, NULL, 0, 0, 1000, 10, true, true, ParamAdjust_CallBack);
}



void wououi_init(void)
{

  // ... 系统初始化 ...
//  OLED_Init(); // 假设调用基础驱动的初始化
  // 或者 u8g2 初始化序列...

  // --- WouoUI 初始化 ---
  // 1. 选择默认UI (如果 WouoUI_user.c 中定义了多个 UI 实例)
  WouoUI_SelectDefaultUI(); 

  // 2. 绑定缓存刷新函数 (关键步骤)
  //    将你实现的 OLED_SendBuff 函数地址传递给 WouoUI 框架
  WouoUI_AttachSendBuffFun(OLED_SendBuff); 

  // 3. 初始化用户菜单 (执行 WouoUI_user.c/h 中定义的菜单结构初始化)
  PIDMenu_Init(); // 函数名取决于用户文件，通常是初始化页面、列表项等
  // --- WouoUI 初始化完成 ---
}


void wououi_task(void)
{
	WouoUI_Proc(10);
}
	
