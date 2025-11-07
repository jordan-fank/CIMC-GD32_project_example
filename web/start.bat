@echo off
chcp 65001 >nul
cls

echo ==========================================
echo   智能终端管理系统 - 快速启动
echo ==========================================
echo.

:: 检查是否已安装依赖
if not exist "backend\node_modules\" (
    echo [警告] 未检测到 node_modules，正在安装依赖...
    cd backend
    call npm install
    cd ..
    echo.
)

:: 启动服务器
echo [启动] 正在启动服务器...
cd backend
call npm start

pause
