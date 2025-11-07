@echo off
chcp 65001 >nul
title Claude Relay Meter 代理服务器

echo ========================================
echo   Claude Relay Meter ��理服务器
echo ========================================
echo.

echo [1/3] 检查 Node.js 环境...
node --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ 错误: 未找到 Node.js
    echo    请先安装 Node.js: https://nodejs.org/
    pause
    exit /b 1
)
echo ✅ Node.js 已安装

echo.
echo [2/3] 安装依赖包...
if not exist node_modules (
    npm install
    if %errorlevel% neq 0 (
        echo ❌ 依赖安装失败
        pause
        exit /b 1
    )
    echo ✅ 依赖安装完成
) else (
    echo ✅ 依赖已存在
)

echo.
echo [3/3] 启动代理服务器...
echo.
echo ========================================
echo   服务器启动中...
echo ========================================
echo.

npm start

pause