@echo off
chcp 65001 >nul
echo ==========================================
echo   内网穿透诊断工具
echo ==========================================
echo.

echo [1/5] 检查服务器运行状态...
netstat -ano | findstr ":8080" >nul
if %errorlevel% == 0 (
    echo ✓ 服务器正在运行 ^(端口8080已监听^)
    netstat -ano | findstr ":8080" | findstr "LISTENING"
) else (
    echo ✗ 服务器未运行 ^(端口8080未监听^)
    echo   请运行: node server.js
)
echo.

echo [2/5] 检查本地访问...
curl -s -I http://localhost:8080 | findstr "HTTP" >nul
if %errorlevel% == 0 (
    echo ✓ 本地访问正常
    curl -s -I http://localhost:8080 | findstr "HTTP"
) else (
    echo ✗ 本地访问失败
)
echo.

echo [3/5] 检查Node.js进程...
tasklist | findstr "node.exe" >nul
if %errorlevel% == 0 (
    echo ✓ Node.js进程运行中:
    tasklist | findstr "node.exe"
) else (
    echo ✗ 未找到Node.js进程
)
echo.

echo [4/5] 检查防火墙规则...
netsh advfirewall firewall show rule name=all | findstr "Node.js" >nul
if %errorlevel% == 0 (
    echo ✓ 防火墙规则已配置
) else (
    echo ! 警告: 未找到Node.js防火墙规则
    echo   建议运行^(管理员权限^):
    echo   netsh advfirewall firewall add rule name="Node.js Server" dir=in action=allow program="C:\Program Files\nodejs\node.exe" enable=yes
)
echo.

echo [5/5] 域名解析测试...
echo 正在解析 www.yanjin.xyz...
nslookup www.yanjin.xyz | findstr "Address"
echo.

echo ==========================================
echo   诊断完成
echo ==========================================
echo.
echo 下一步建议:
echo 1. 如果本地访问正常,请检查内网穿透配置
echo 2. 确保内网穿透类型为 HTTP ^(不是TCP^)
echo 3. 使用手机4G网络测试: http://www.yanjin.xyz
echo.
pause
