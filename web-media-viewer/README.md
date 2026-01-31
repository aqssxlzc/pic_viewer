# Web媒体文件查看器

这是原生Qt5 C++应用的Web版本，使用Go语言编写。提供相同的功能，可以在任何浏览器中访问。

## 功能特性

- 📂 **文件夹浏览** - 输入文件夹路径显示所有媒体文件
- 🖼️ **图片和视频支持** - 支持jpg、png、gif、webp、mp4、mkv等格式
- 📋 **网格视图** - 响应式网格布局，自动适应屏幕宽度
- ⏳ **懒加载** - 分批加载（每批30个文件），提高性能
- 🔄 **排序功能** - 按修改时间排序，支持升序/降序
- 💾 **会话保存** - 记住最后打开的文件夹、滚动位置、排序方式
- 🎮 **键盘导航** - 全屏预览支持方向键和WASD导航
- 📱 **响应式设计** - 支持各种屏幕尺寸

## 安装

### 前置条件
- Go 1.21 或更高版本
- Linux/macOS/Windows 系统

### 构建

```bash
cd web-media-viewer
go mod download
go build -o media-viewer
```

## 运行

```bash
./media-viewer
```

然后在浏览器中打开：`http://localhost:8080`

## 使用方式

### 打开文件夹
1. 在顶部输入框输入文件夹的完整路径
2. 点击"打开文件夹"按钮
3. 应用将加载该文件夹中的所有媒体文件

例如：
- Linux/macOS: `/home/user/Pictures`、`/tmp/media`
- Windows: `C:\Users\User\Pictures`

### 排序
- 点击"排序: 新到旧"按钮切换排序方向
- 排序基于文件修改时间
- 选择会被自动保存

### 查看媒体
1. 点击网格中的任何媒体项
2. 全屏预览将打开
3. 使用方向键或WASD在文件之间导航
4. 按ESC关闭预览

### 加载更多
- 当有更多文件时，底部会显示"加载更多"按钮
- 点击按钮加载下一批文件

## API端点

### 获取文件列表
```
GET /api/files?dir=/path/to/folder&sort=desc&offset=0&limit=30
```

参数：
- `dir` - 文件夹路径（必需）
- `sort` - 排序方式：`asc`（升序）或 `desc`（降序），默认 `desc`
- `offset` - 分页偏移，默认 0
- `limit` - 每页数量，默认 30

响应：
```json
{
  "files": [
    {
      "name": "photo.jpg",
      "path": "/full/path/to/photo.jpg",
      "type": "image",
      "modified": "2024-02-01T12:00:00Z",
      "size": 102400
    }
  ],
  "total": 150,
  "imageCount": 120,
  "videoCount": 30
}
```

### 获取文件内容
```
GET /api/file?path=/full/path/to/file
```

- 支持图片和视频的直接访问
- 支持HTTP Range请求（用于视频流）

## 存储说明

所有偏好设置存储在浏览器的LocalStorage中：
- `lastDirectory` - 最后打开的文件夹
- `scrollPosition` - 滚动位置
- `sortAscending` - 排序方向

## 性能优化

- **虚拟滚动** - 只渲染可见的项目
- **分批加载** - 每批30个文件，减少初始加载时间
- **Range请求支持** - 支持视频流的断点续传
- **懒加载缩略图** - 图片在需要时才加载

## 与Qt版本的区别

| 功能 | Qt版本 | Web版本 |
|------|-------|--------|
| 独立应用 | ✓ | ✗ |
| 网络访问 | ✗ | ✓ |
| 自动缩略图生成 | ✓ | ✗ |
| 全键盘导航 | ✓ | 部分支持 |
| 会话状态保存 | 本地QSettings | LocalStorage |

## 故障排除

### 文件夹路径无效
- 确保路径存在且应用有读取权限
- 使用绝对路径而非相对路径

### 文件未显示
- 检查文件格式是否支持（jpg、png、gif、webp、mp4等）
- 确保文件有正确的扩展名

### 视频播放缓慢
- 这取决于网络带宽和视频编码
- 浏览器支持H.264和VP8/VP9等常见编码

## 开发

### 添加新的图片格式
在 `main.go` 中修改 `imageExtensions`：

```go
var imageExtensions = map[string]bool{
    ".jpg": true, ".jpeg": true, ".png": true, // ...
    ".heic": true,  // 添加新格式
}
```

### 修改默认端口
修改 `main.go` 中的 `port` 变量：

```go
port := ":9000"  // 修改为其他端口
```

## 许可证

MIT License

## 相关项目

- [Qt版本](../README.md) - 原生桌面应用版本
