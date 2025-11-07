@echo off
chcp 65001 >nul
title Claude Relay Meter 代理服务器启动器

echo ========================================
echo   Claude Relay Meter 代理服务启动器
echo ========================================
echo.

echo [信息] 正在启动代理服务器...
echo.
echo 服务地址: http://localhost:3001
echo 目标API: https://code-next.akclau.de
echo.

echo [提示] 启动后请重新加载VSCode窗口以应用设置
echo        快捷键: Ctrl+Shift+P → "Developer: Reload Window"
echo.

echo ========================================
echo   服务器日志
echo ========================================

cd /d "%~dp0"
node server-simple.js

echo.
echo [信息] 服务器已停止
pause