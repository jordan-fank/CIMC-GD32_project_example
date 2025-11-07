/**
 * 代理服务器测试脚本
 */

const http = require('http');

const PROXY_HOST = 'localhost';
const PROXY_PORT = 3001;

function makeRequest(path, method = 'POST', data = null) {
    return new Promise((resolve, reject) => {
        const options = {
            hostname: PROXY_HOST,
            port: PROXY_PORT,
            path: path,
            method: method,
            headers: {
                'Content-Type': 'application/json',
                'User-Agent': 'Claude-Relay-Meter-Test/1.0'
            }
        };

        if (data) {
            const postData = JSON.stringify(data);
            options.headers['Content-Length'] = Buffer.byteLength(postData);
        }

        const req = http.request(options, (res) => {
            let body = '';
            res.on('data', (chunk) => {
                body += chunk;
            });
            res.on('end', () => {
                try {
                    const response = JSON.parse(body);
                    resolve({
                        statusCode: res.statusCode,
                        headers: res.headers,
                        data: response
                    });
                } catch (error) {
                    reject(new Error(`JSON解析失败: ${error.message}`));
                }
            });
        });

        req.on('error', (error) => {
            reject(error);
        });

        if (data) {
            req.write(JSON.stringify(data));
        }
        req.end();
    });
}

async function runTests() {
    console.log('========================================');
    console.log('  Claude Relay Meter 代理测试');
    console.log('========================================');
    console.log('');

    try {
        // 测试1: 健康检查
        console.log('[测试1] 健康检查...');
        const healthResponse = await makeRequest('/health', 'GET');
        if (healthResponse.statusCode === 200) {
            console.log('✅ 健康检查通过:', healthResponse.data.status);
        } else {
            console.log('❌ 健康检查失败:', healthResponse.statusCode);
        }
        console.log('');

        // 测试2: 获取API ID
        console.log('[测试2] 获取API ID...');
        const apiIdResponse = await makeRequest('/apiStats/api/get-key-id', 'POST', {});
        if (apiIdResponse.statusCode === 200) {
            console.log('✅ API ID获取成功:', apiIdResponse.data.apiId);
        } else {
            console.log('❌ API ID获取失败:', apiIdResponse.statusCode);
            console.log('   错误信息:', apiIdResponse.data);
        }
        console.log('');

        // 测试3: 获取使用量统计
        console.log('[测试3] 获取使用量统计...');
        const usageResponse = await makeRequest('/apiStats/api/get-usage-stats', 'POST', {});
        if (usageResponse.statusCode === 200) {
            console.log('✅ 使用量统计获取成功:');
            console.log('   总使用量:', usageResponse.data.usage?.total_usage || 'N/A');
            console.log('   输入Token:', usageResponse.data.tokens?.input || 'N/A');
            console.log('   输出Token:', usageResponse.data.tokens?.output || 'N/A');
            console.log('   请求数:', usageResponse.data.requests || 'N/A');
        } else {
            console.log('❌ 使用量统计获取失败:', usageResponse.statusCode);
            console.log('   错误信息:', usageResponse.data);
        }
        console.log('');

        // 测试4: 未知端点
        console.log('[测试4] 未知端点测试...');
        const unknownResponse = await makeRequest('/unknown-endpoint', 'GET');
        if (unknownResponse.statusCode === 404) {
            console.log('✅ 404错误处理正常');
        } else {
            console.log('❌ 404错误处理异常:', unknownResponse.statusCode);
        }
        console.log('');

        console.log('========================================');
        console.log('  测试完成！');
        console.log('========================================');
        console.log('');
        console.log('如果所有测试都通过，请更新VSCode设置:');
        console.log('"relayMeter.apiUrl": "http://localhost:3001"');
        console.log('"relayMeter.apiKey": "proxy-key"');

    } catch (error) {
        console.error('❌ 测试失败:', error.message);
        console.log('');
        console.log('请确保代理服务器正在运行:');
        console.log('cd claude-relay-proxy && npm start');
    }
}

// 运行测试
runTests();