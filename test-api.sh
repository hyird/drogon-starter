#!/bin/bash
# test_api.sh - API 快速测试脚本

BASE_URL="http://localhost:8080"
TOKEN=""

echo "=== 1. 登录获取 Token ==="
RESPONSE=$(curl -s -X POST "$BASE_URL/api/auth/login" \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "123456"}')

echo "$RESPONSE" | jq .

TOKEN=$(echo "$RESPONSE" | jq -r '.data.token')
echo "Token: ${TOKEN:0:50}..."

echo ""
echo "=== 2. 获取当前用户 ==="
curl -s -X GET "$BASE_URL/api/user/me" \
  -H "Authorization: Bearer $TOKEN" | jq .

echo ""
echo "=== 3. 获取用户列表 ==="
curl -s -X GET "$BASE_URL/api/user/list?page=1&pageSize=10" \
  -H "Authorization: Bearer $TOKEN" | jq .

echo ""
echo "=== 4. 注册新用户 ==="
curl -s -X POST "$BASE_URL/api/auth/register" \
  -H "Content-Type: application/json" \
  -d "{
    \"username\": \"admin\",
    \"password\": \"123456\",
    \"email\": \"admin@admin.com\"
  }" | jq .

echo ""
echo "=== 5. 刷新 Token ==="
curl -s -X POST "$BASE_URL/api/auth/refresh" \
  -H "Authorization: Bearer $TOKEN" | jq .

echo ""
echo "=== 测试完成 ==="