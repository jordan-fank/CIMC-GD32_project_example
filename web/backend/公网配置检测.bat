@echo off
chcp 65001 >nul
title WebSocket终端管理系统 - 公网配置助手

echo ==========================================
echo   WebSocket终端管理系统 - 公网配置助手
echo ==========================================
echo.

echo [1/5] 检查服���器运行状态...
netstat -ano | grep ":8080" | grep "LISTENING" >nul
if %errorlevel% == 0 (
    echo ✓ 服务器正在运行 (端口8080)

    echo.
    echo [2/5] 测试本地HTTP访问...
    curl -s -I http://localhost:8080 | grep "HTTP/1.1 200" >nul
    if %errorlevel% == 0 (
        echo ✓ HTTP服务正常

        echo.
        echo [3/5] 测试WebSocket连接...
        powershell -Command "try { $ws = New-Object System.Net.WebSockets.ClientWebSocket; $cts = New-Object System.Threading.CancellationTokenSource(5000); $ws.ConnectAsync('ws://localhost:8080', $cts.Token).Wait(); $ws.CloseAsync([System.Net.WebSockets.WebSocketCloseStatus]::NormalClosure, 'Test', $cts.Token).Wait(); Write-Host '✓ WebSocket连接正常' } catch { Write-Host '✗ WebSocket连接失败' }"

        echo.
        echo [4/5] 检查域名解析...
        nslookup www.yanjin.xyz | findstr "Address" >nul
        if %errorlevel% == 0 (
            echo ✓ 域名解析正常

            echo.
            echo [5/5] 测试公网访问...
            echo 正在测试 http://www.yanjin.xyz ...
            curl -s -I http://www.yanjin.xyz --max-time 10 | grep "HTTP/1.1 200" >nul
            if %errorlevel% == 0 (
                echo ✓ 公网访问成功！
                echo.
                echo ==========================================
                echo   配置完成！
                echo ==========================================
                echo.
                echo ✓ 别人现在可以通过以下方式访问:
                echo   - 浏览器: http://www.yanjin.xyz
                echo   - WebSocket: ws://www.yanjin.xyz
                echo.
                echo ✓ 使用说明:
                echo   1. 任何人打开 http://www.yanjin.xyz
                echo   2. 页面会自动配置连接参数
                echo   3. 点击"连接服务器"即可通信
                echo.
            ) else (
                echo ✗ 公网访问失败
                echo.
                echo   可能的原因:
                echo   1. FRP未配置HTTP映射
                echo   2. FRP服务未运行
                echo   3. 防火墙阻止连接
                echo.
                echo   解决方案:
                echo   1. 检查FRP配置，确保使用 type = http
                echo   2. 重启FRP客户端
                echo   3. 查看[公网访问配置指南.md]
            )
        ) else (
            echo ✗ 域名解析失败
            echo   请检查域名 www.yanjin.xyz 是否正确配置
        )
    ) else (
        echo ✗ HTTP服务异常
        echo   请检查服务器是否正确启动
    )
) else (
    echo ✗ 服务器未运行
    echo.
    echo   启动服务器:
    echo   cd C:\Users\HP\Desktop\CIMC_xi\web\backend
    echo   node server.js
)

echo.
echo ==========================================
echo   诊断完成
echo ==========================================
echo.
pause