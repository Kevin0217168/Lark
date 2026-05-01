#!/usr/bin/env bash
# ============================================================================
# upload_firmware.sh — 编译后自动上传固件到 Lark 后端服务器
#
# 用法:
#   ./scripts/upload_firmware.sh -u <username> -p <password> [选项]
#
# 必选参数:
#   -u, --username    后端 root 用户名
#   -p, --password    后端 root 用户密码
#
# 可选参数:
#   -s, --server      服务器地址 (默认: http://192.168.1.200:8080/api)
#   -d, --device-type 设备类型   (默认: ESP32-CAM)
#   -n, --name        固件名称   (默认: lark_firmware)
#   -f, --firmware    固件文件路径 (默认: build/lark.bin)
#   -h, --help        显示帮助
#
# 示例:
#   # 上传到本地调试服务器
#   ./scripts/upload_firmware.sh -u root -p 123456
#
#   # 上传到生产服务器
#   ./scripts/upload_firmware.sh -u root -p 123456 -s https://lark.mintlab.top/api
# ============================================================================

set -euo pipefail

# ── 颜色定义 ──
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# ── 脚本所在目录 → 项目根目录 ──
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# ── 默认值 ──
SERVER_URL="http://192.168.1.200:8080/api"
DEVICE_TYPE="ESP32-CAM"
FW_NAME="lark_firmware"
FW_FILE="$PROJECT_DIR/build/lark.bin"
USERNAME=""
PASSWORD=""

# ── 帮助信息 ──
usage() {
    echo -e "${CYAN}Lark 固件上传脚本${NC}"
    echo ""
    echo "用法: $0 -u <username> -p <password> [选项]"
    echo ""
    echo "必选参数:"
    echo "  -u, --username      后端 root 用户名"
    echo "  -p, --password      后端 root 用户密码"
    echo ""
    echo "可选参数:"
    echo "  -s, --server        服务器地址 (默认: $SERVER_URL)"
    echo "  -d, --device-type   设备类型   (默认: $DEVICE_TYPE)"
    echo "  -n, --name          固件名称   (默认: $FW_NAME)"
    echo "  -f, --firmware      固件文件   (默认: build/lark.bin)"
    echo "  -h, --help          显示本帮助"
    exit 0
}

# ── 解析参数 ──
while [[ $# -gt 0 ]]; do
    case "$1" in
        -u|--username)    USERNAME="$2";    shift 2 ;;
        -p|--password)    PASSWORD="$2";    shift 2 ;;
        -s|--server)      SERVER_URL="$2";  shift 2 ;;
        -d|--device-type) DEVICE_TYPE="$2"; shift 2 ;;
        -n|--name)        FW_NAME="$2";     shift 2 ;;
        -f|--firmware)    FW_FILE="$2";     shift 2 ;;
        -h|--help)        usage ;;
        *) echo -e "${RED}未知参数: $1${NC}"; usage ;;
    esac
done

# ── 参数校验 ──
if [[ -z "$USERNAME" || -z "$PASSWORD" ]]; then
    echo -e "${RED}错误: 必须提供 -u/--username 和 -p/--password${NC}"
    echo ""
    usage
fi

# ── 从 CMakeLists.txt 提取版本号 ──
CMAKE_FILE="$PROJECT_DIR/CMakeLists.txt"
if [[ ! -f "$CMAKE_FILE" ]]; then
    echo -e "${RED}错误: 找不到 $CMAKE_FILE${NC}"
    exit 1
fi

VERSION=$(grep -oP 'set\(PROJECT_VER\s+"?\K[0-9]+\.[0-9]+\.[0-9]+[^"]*' "$CMAKE_FILE" || true)
if [[ -z "$VERSION" ]]; then
    echo -e "${RED}错误: 无法从 CMakeLists.txt 提取 PROJECT_VER${NC}"
    exit 1
fi

# ── 检查固件文件 ──
if [[ ! -f "$FW_FILE" ]]; then
    echo -e "${RED}错误: 固件文件不存在: $FW_FILE${NC}"
    echo -e "${YELLOW}提示: 请先运行 'idf.py build' 编译项目${NC}"
    exit 1
fi

FW_SIZE=$(stat -c%s "$FW_FILE" 2>/dev/null || stat -f%z "$FW_FILE" 2>/dev/null)

echo -e "${CYAN}============================================${NC}"
echo -e "${CYAN}       Lark 固件上传${NC}"
echo -e "${CYAN}============================================${NC}"
echo -e "  服务器:     ${GREEN}$SERVER_URL${NC}"
echo -e "  设备类型:   ${GREEN}$DEVICE_TYPE${NC}"
echo -e "  固件名称:   ${GREEN}$FW_NAME${NC}"
echo -e "  版本号:     ${GREEN}$VERSION${NC}"
echo -e "  固件文件:   ${GREEN}$FW_FILE${NC}"
echo -e "  文件大小:   ${GREEN}${FW_SIZE} bytes${NC}"
echo -e "${CYAN}============================================${NC}"
echo ""

# ── Step 1: 登录获取 Token ──
echo -e "${YELLOW}[1/2] 登录获取 Token ...${NC}"

LOGIN_RESP=$(curl -s -w "\n%{http_code}" \
    -X POST "${SERVER_URL}/login" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "username=${USERNAME}&password=${PASSWORD}")

HTTP_CODE=$(echo "$LOGIN_RESP" | tail -1)
LOGIN_BODY=$(echo "$LOGIN_RESP" | sed '$d')

if [[ "$HTTP_CODE" != "200" ]]; then
    echo -e "${RED}登录失败! HTTP $HTTP_CODE${NC}"
    echo -e "${RED}响应: $LOGIN_BODY${NC}"
    exit 1
fi

# 从 JSON 响应提取 access_token
TOKEN=$(echo "$LOGIN_BODY" | python3 -c "import sys,json; print(json.load(sys.stdin)['access_token'])" 2>/dev/null || true)

if [[ -z "$TOKEN" ]]; then
    echo -e "${RED}登录失败: 无法从响应中提取 token${NC}"
    echo -e "${RED}响应: $LOGIN_BODY${NC}"
    exit 1
fi

echo -e "${GREEN}登录成功! Token: ${TOKEN:0:20}...${NC}"
echo ""

# ── Step 2: 上传固件 ──
echo -e "${YELLOW}[2/2] 上传固件 ...${NC}"

UPLOAD_RESP=$(curl -s -w "\n%{http_code}" \
    -X POST "${SERVER_URL}/firmwares/upload" \
    -H "Authorization: Bearer ${TOKEN}" \
    -F "device_type=${DEVICE_TYPE}" \
    -F "name=${FW_NAME}" \
    -F "version=${VERSION}" \
    -F "file=@${FW_FILE}")

HTTP_CODE=$(echo "$UPLOAD_RESP" | tail -1)
UPLOAD_BODY=$(echo "$UPLOAD_RESP" | sed '$d')

if [[ "$HTTP_CODE" == "200" ]]; then
    echo -e "${GREEN}============================================${NC}"
    echo -e "${GREEN}  固件上传成功!${NC}"
    echo -e "${GREEN}============================================${NC}"
    echo -e "  响应: $UPLOAD_BODY"
elif [[ "$HTTP_CODE" == "400" ]]; then
    echo -e "${YELLOW}============================================${NC}"
    echo -e "${YELLOW}  固件已存在 (相同设备类型+版本)${NC}"
    echo -e "${YELLOW}============================================${NC}"
    echo -e "  响应: $UPLOAD_BODY"
    echo -e "${YELLOW}提示: 若要更新，请先在后端删除旧版本，或修改 CMakeLists.txt 中的 PROJECT_VER${NC}"
    exit 1
elif [[ "$HTTP_CODE" == "403" ]]; then
    echo -e "${RED}权限不足: 用户不是 root 角色${NC}"
    echo -e "  响应: $UPLOAD_BODY"
    exit 1
else
    echo -e "${RED}上传失败! HTTP $HTTP_CODE${NC}"
    echo -e "  响应: $UPLOAD_BODY"
    exit 1
fi
