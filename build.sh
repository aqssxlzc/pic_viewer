#!/bin/bash

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}媒体文件查看器 - 编译脚本${NC}"
echo "================================"

# Check if Qt is installed
if ! command -v qmake &> /dev/null && ! command -v qmake6 &> /dev/null; then
    echo -e "${RED}错误: 未找到 Qt (qmake)${NC}"
    echo "请安装 Qt 6:"
    echo "  Ubuntu/Debian: sudo apt install qt6-base-dev qt6-multimedia-dev"
    echo "  Fedora: sudo dnf install qt6-qtbase-devel qt6-qtmultimedia-devel"
    echo "  Arch: sudo pacman -S qt6-base qt6-multimedia"
    exit 1
fi

# Choose build system
if [ "$1" == "cmake" ]; then
    echo -e "${YELLOW}使用 CMake 构建...${NC}"
    mkdir -p build
    cd build
    cmake ..
    make -j$(nproc)
    BUILD_SUCCESS=$?
    cd ..
    EXECUTABLE="./build/MediaViewer"
else
    echo -e "${YELLOW}使用 qmake 构建...${NC}"
    
    # Try qmake6 first, then qmake
    if command -v qmake6 &> /dev/null; then
        QMAKE=qmake6
    else
        QMAKE=qmake
    fi
    
    $QMAKE MediaViewer.pro
    make -j$(nproc)
    BUILD_SUCCESS=$?
    EXECUTABLE="./MediaViewer"
fi

if [ $BUILD_SUCCESS -eq 0 ]; then
    echo -e "${GREEN}编译成功!${NC}"
    echo ""
    echo "运行程序:"
    echo "  $EXECUTABLE"
    echo ""
    read -p "是否立即运行? (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        $EXECUTABLE
    fi
else
    echo -e "${RED}编译失败!${NC}"
    exit 1
fi
