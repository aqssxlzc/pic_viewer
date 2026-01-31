# 📸 媒体文件查看器 - 完整项目

这是一个完整的媒体文件查看系统，包含两个版本：

1. **Qt版本** - 原生C++桌面应用
2. **Web版本** - Go后端 + JavaScript前端的Web应用

两个版本提供相同的核心功能，但采用不同的技术实现，适应不同的使用场景。

## 📁 项目结构

```
grok/
├── 📱 Qt版本（C++）
│   ├── main.cpp                 # 应用入口点
│   ├── mainwindow.h/cpp         # 主窗口和文件管理
│   ├── mediaitem.h/cpp          # 媒体卡片组件
│   ├── imageviewer.h/cpp        # 全屏图片/视频查看器
│   ├── MediaViewer.pro          # qmake配置
│   ├── CMakeLists.txt           # CMake配置
│   └── MediaViewer              # 编译后的可执行文件
│
├── 🌐 Web版本（Go）
│   ├── web-media-viewer/
│   │   ├── main.go              # Go服务器
│   │   ├── go.mod/go.sum        # Go依赖管理
│   │   ├── media-viewer         # 编译后的二进制文件
│   │   ├── build.sh             # 构建脚本
│   │   ├── static/
│   │   │   ├── app.js           # 前端JavaScript
│   │   │   └── style.css        # 前端样式
│   │   ├── templates/
│   │   │   └── index.html       # HTML模板
│   │   ├── README.md            # Web版文档
│   │   ├── QUICKSTART.md        # Web版快速开始
│   │
│   └── Dockerfile               # Docker配置
│
├── 📖 文档
│   ├── README.md                # 主文档
│   ├── COMPARISON.md            # 版本对比
│   ├── SCROLL_POSITION_TEST.md  # 滚动位置测试说明
│   ├── build.sh                 # Qt版本构建脚本
│   └── Makefile                 # Qt版本Makefile
│
```

## 🚀 快速开始

### 运行 Qt 版本

```bash
# 构建
make -j$(nproc)

# 运行
./MediaViewer
```

### 运行 Web 版本

```bash
# 构建
cd web-media-viewer
go build -o media-viewer main.go

# 运行
./media-viewer

# 访问: http://localhost:8081
```

## ✨ 功能特性

### 两个版本都支持

- 📂 **文件夹浏览** - 输入路径显示所有媒体文件
- 🖼️ **多格式支持** - 图片（jpg、png、gif、webp等）+ 视频（mp4、mkv、webm等）
- 📋 **网格视图** - 响应式布局，自动适应屏幕宽度
- ⏳ **懒加载** - 分批加载（每批30个），高效处理大量文件
- 🔄 **排序功能** - 按修改时间排序，支持升序/降序切换
- 💾 **会话保存** - 记住最后打开的文件夹、滚动位置、排序方式
- 🎮 **键盘导航** - 全屏预览支持方向键和WASD导航
- 📱 **响应式设计** - 支持各种屏幕尺寸

### 版本特定功能

**Qt版本**
- 原生性能优化
- 硬件加速（OpenGL）
- 后台线程图片解码
- 专业UI框架

**Web版本**
- 网络远程访问
- 跨设备支持
- 轻松部署
- 简单扩展

## 📊 版本对比

| 特性 | Qt | Web |
|------|:--:|:---:|
| 原生性能 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| 易于部署 | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| 网络访问 | ❌ | ✅ |
| 文件大小 | ~20MB | ~9MB |
| 学习曲线 | 陡峭 | 平缓 |
| 扩展能力 | 中等 | 优秀 |

详见 [完整版本对比](COMPARISON.md)

## 🛠️ 技术栈

### Qt版本
- **语言**：C++ (C++17)
- **框架**：Qt 5.15
- **编译器**：g++
- **构建**：qmake/CMake
- **优化**：O3 -march=native -flto
- **加速**：OpenGL 3.3+

### Web版本
- **后端**：Go 1.25+
- **前端**：HTML5 + CSS3 + Vanilla JavaScript
- **库**：gorilla/mux
- **构建**：go build
- **部署**：Docker可选

## 📋 系统要求

### Qt版本
- Linux/macOS/Windows
- Qt 5.15 或更高版本
- GCC 9+ 或 Clang 10+
- 2GB RAM（推荐4GB+）

### Web版本
- Linux/macOS/Windows
- Go 1.21 或更高版本
- 现代Web浏览器（Chrome, Firefox, Safari, Edge）
- 512MB RAM（推荐1GB+）

## 📦 支持的文件格式

### 图片格式
- JPEG (.jpg, .jpeg)
- PNG (.png)
- GIF (.gif)
- WebP (.webp)
- BMP (.bmp)
- SVG (.svg)

### 视频格式
- MP4 (.mp4)
- WebM (.webm)
- Matroska (.mkv)
- AVI (.avi)
- MOV (.mov)
- WMV (.wmv)
- FLV (.flv)

## 🎯 使用场景

### Qt版本适用于
- 📸 本地摄影工作流
- 🎬 专业视频审看
- 💼 企业内部系统
- 🔐 安全敏感环境

### Web版本适用于
- 🏠 家庭NAS媒体服务
- 🤝 团队文件共享
- 🌍 远程访问
- ☁️ 云服务部署

## 🔧 配置和优化

### Qt版本

**配置文件位置**
```
Linux/macOS: ~/.config/MediaViewer/
Windows: %AppData%/MediaViewer/
```

**配置项**
```ini
[General]
lastDirectory=/path/to/folder
scrollPosition=3600
sortAscending=false
```

**优化建议**
- 启用硬件加速（默认启用）
- 为大文件夹增加虚拟内存
- 定期清理缓存

### Web版本

**环境变量**
```bash
# 修改监听端口
PORT=8081

# 修改最大上传大小
MAX_FILE_SIZE=1GB
```

**性能优化**
- 使用CDN分发静态资源
- 启用Gzip压缩
- 配置反向代理缓存

## 📚 文档

- [Qt版本完整文档](README-QT.md)
- [Web版本完整文档](web-media-viewer/README.md)
- [Web版本快速开始](web-media-viewer/QUICKSTART.md)
- [版本详细对比](COMPARISON.md)
- [API文档](web-media-viewer/README.md#api端点)

## 🐛 故障排除

### 常见问题

#### Q: 文件无法显示
A: 检查文件格式是否支持，文件权限是否正确

#### Q: 应用启动缓慢
A: 
- Qt版本：检查磁盘I/O或增加虚拟内存
- Web版本：检查网络连接和浏览器缓存

#### Q: 内存占用高
A: 减少同时加载的文件数量，分割大文件夹

### 获取帮助

1. 检查相应的README文档
2. 查看控制台输出的错误信息
3. 运行调试模式

```bash
# Qt版本
./MediaViewer 2>&1 | grep -i error

# Web版本
./media-viewer -v
```

## 🎓 开发指南

### 添加新的图片格式

**Qt版本** - 编辑 `mainwindow.h`
```cpp
imageExtensions << "heic" << "heif";
```

**Web版本** - 编辑 `main.go`
```go
var imageExtensions = map[string]bool{
    ".heic": true,
    ".heif": true,
}
```

### 修改UI样式

**Qt版本** - 在 `mainwindow.cpp` 中修改StyleSheet

**Web版本** - 编辑 `static/style.css`

### 添加新的API端点

**Web版本** - 在 `main.go` 中添加路由
```go
router.HandleFunc("/api/newfeature", handleNewFeature).Methods("GET", "POST")
```

## 📈 性能基准

### 初始化时间
- Qt版本: 1-2秒
- Web版本: 首次3-5秒，后续 < 1秒

### 显示30个文件的时间
- Qt版本: 200-500ms
- Web版本: 500-1000ms

### 内存占用（加载100个文件）
- Qt版本: 200-300MB
- Web版本: 服务器 50-100MB + 浏览器 300-400MB

## 🔄 版本历史

### Qt版本
- v1.0 - 基础网格视图和全屏预览
- v1.1 - 添加排序和会话保存
- v1.2 - 优化性能，添加键盘导航
- v1.3 - 改进滚动位置记忆 ✨ 当前版本

### Web版本
- v1.0 - 初始发布 ✨ 当前版本

## 🤝 贡献

欢迎贡献改进！

### 改进方向
- [ ] 添加更多视频格式支持
- [ ] 实现缩略图预生成
- [ ] 添加搜索和过滤功能
- [ ] 支持文件分类和标签
- [ ] 实现相册功能

## 📄 许可证

MIT License

## 👨‍💻 作者

开发于 2024-2026

## 🙏 致谢

- Qt Framework 开发团队
- Go 语言社区
- 所有贡献者

## 📞 联系方式

- 问题报告：[提交issue]
- 功能请求：[创建讨论]
- 邮件：[contact@example.com]

---

**提示**：选择最适合您使用场景的版本！
- 需要**本地独立应用**？→ 使用 **Qt版本**
- 需要**网络远程访问**？→ 使用 **Web版本**
- 不确定？→ 阅读 [完整版本对比](COMPARISON.md)
