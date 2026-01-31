#!/bin/bash

cd /run/media/chen/c293c7a5-2aa3-4228-8a1a-0a2a4f828a3c/home/chen/grok

echo "==== 滚动位置记忆功能测试 ===="
echo ""
echo "步骤 1: 启动应用（第一次）"
echo "- 应用将打开最后一个目录（如果有的话）"
echo "- 本次示例中应该没有最后的目录记录"
echo ""
echo "请执行以下命令手动测试:"
echo ""
echo "  1. 启动应用:"
echo "     ./MediaViewer"
echo ""
echo "  2. 打开一个包含多个文件的文件夹（菜单: 选择文件夹）"
echo ""
echo "  3. 向下滚动到中间或下方位置"
echo ""
echo "  4. 关闭应用"
echo ""
echo "  5. 检查保存的配置:"
echo "     cat ~/.config/MediaViewer/MediaViewer.conf"
echo ""
echo "  6. 重新启动应用:"
echo "     ./MediaViewer"
echo ""
echo "  7. 验证："
echo "     - 应该自动打开上次的文件夹"
echo "     - 滚动位置应该恢复到之前的位置"
echo "     - 排序方向应该恢复（如果改过的话）"
echo ""
echo "==== 调试输出 ===="
./MediaViewer &
APP_PID=$!
sleep 2
echo "应用 PID: $APP_PID"
echo "启动成功。请按照上面的步骤进行手动测试。"
echo ""
echo "应用运行中... 按 Ctrl+C 关闭此脚本（不会关闭应用）"
wait
