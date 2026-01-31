#!/bin/bash

set -e

echo "=== Web Media Viewer 构建脚本 ==="
echo ""

# 检查Go是否安装
if ! command -v go &> /dev/null; then
    echo "错误: 未找到Go。请先安装Go 1.21或更高版本"
    exit 1
fi

GO_VERSION=$(go version | awk '{print $3}')
echo "Go版本: $GO_VERSION"

# 下载依赖
echo ""
echo "正在下载依赖..."
go mod download

# 构建
echo ""
echo "正在构建..."
CGO_ENABLED=0 go build -o media-viewer -ldflags="-s -w" main.go

echo ""
echo "✓ 构建完成！"
echo ""
echo "运行应用："
echo "  ./media-viewer"
echo ""
echo "然后在浏览器中打开："
echo "  http://localhost:8080"
echo ""
