# 滚动位置记忆功能测试说明

## 功能说明
应用现在支持以下功能：
1. **记住最后打开的文件夹** - 下次启动时自动打开
2. **记住滚动位置** - 保存滚动条的位置并在启动时恢复
3. **记住排序方向** - 保存排序选择（新到旧 或 旧到新）

## 配置存储位置
配置文件保存在：`~/.config/MediaViewer/MediaViewer.conf`

内容示例：
```
[General]
lastDirectory=/path/to/your/media/folder
scrollPosition=3600
sortAscending=false
```

## 测试步骤

### 第一次运行（没有之前的配置）
```bash
# 清除配置（可选）
rm ~/.config/MediaViewer/MediaViewer.conf

# 启动应用
./MediaViewer
```

### 功能测试

#### 1. 测试目录记忆
- ✓ 启动应用，点击"选择文件夹"
- ✓ 选择一个包含多个媒体文件的文件夹
- ✓ 关闭应用
- ✓ 重新启动应用，应该自动打开上次的文件夹

#### 2. 测试滚动位置记忆
- ✓ 打开一个文件夹（包含至少30个以上的文件）
- ✓ 向下滚动到中间或下方位置（例如滚动到第三屏）
- ✓ **关闭应用**（注意：必须完全关闭窗口）
- ✓ 检查配置文件：`cat ~/.config/MediaViewer/MediaViewer.conf`
  - 应该看到 `scrollPosition=xxxx`（数字应该 > 0）
- ✓ 重新启动应用
- ✓ **验证**：滚动条应该恢复到之前的位置

#### 3. 测试排序方向记忆
- ✓ 点击灰色按钮"排序: 新到旧"来切换排序方向
  - 按钮会变为"排序: 旧到新"（反向）
- ✓ 关闭应用
- ✓ 重新启动应用
- ✓ **验证**：按钮状态应该恢复（排序顺序应该是记住的）

## 调试输出

要查看详细的调试信息（包括滚动位置的恢复过程），在终端运行：

```bash
./MediaViewer 2>&1 | grep -i "scroll\|restored\|scroll position"
```

期望看到类似的输出：
```
Restored settings - Directory: "/path/to/folder" ScrollPos: 3600 SortAsc: false
Restoring scroll position: 3600
Scroll restored to: 3600 max: 8000
```

## 故障排除

### 问题：滚动位置没有被恢复
**可能原因：**
1. 应用可能没有被完全关闭（使用 `killall MediaViewer` 来确保）
2. 文件夹包含的文件太少（需要至少30个文件）
3. 配置文件没有被正确保存

**解决方案：**
- 确保应用窗口完全关闭
- 检查配置文件内容：`cat ~/.config/MediaViewer/MediaViewer.conf`
- 查看调试输出：`./MediaViewer 2>&1 | grep -i scroll`

### 问题：滚动位置被重置为顶部
**可能原因：**
- 上次关闭时滚动位置为0（顶部）
- 或者文件列表发生了变化（添加/删除文件）

**验证方法：**
检查 `scrollPosition` 值是否大于 0

## 性能提示

- 如果文件夹包含几千个文件，首次加载可能需要几秒钟
- 滚动位置的恢复延迟设置为150ms，这是为了确保布局计算完成
- 应用使用虚拟滚动，只加载可见的项目（每批30个）

## 相关代码修改

主要改动：
1. `mainwindow.h`: 添加了 `lastScrollPosition` 成员和 `loadMediaFiles(const QString &, bool)` 重载
2. `mainwindow.cpp`: 
   - 构造函数中从 QSettings 读取保存的值
   - `closeEvent()` 中保存滚动位置
   - `loadMediaFiles()` 中添加滚动位置恢复逻辑
   - `toggleSortOrder()` 中保存排序方向

