# 媒体文件查看器 (Media Viewer)

一个高性能的原生桌面应用程序，用 C++ 和 Qt 开发，用于浏览文件夹和 ZIP 压缩包中的图片和视频。

![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-blue)
![License](https://img.shields.io/badge/license-MIT-green)

## 功能特性

- **网格布局展示** - 自适应响应式网格，自动调整列数
- **ZIP 文件支持** - 直接浏览 ZIP 压缩包中的图片，无需解压
- **双图模式** - 在 ZIP 浏览时支持双图对比显示
- **懒加载** - 分批加载媒体文件，优化内存使用
- **硬件加速** - 使用 OpenGL 加速渲染，流畅的动画效果
- **图片预览** - 点击图片全屏查看，支持键盘导航
- **视频播放** - 视频进入视口自动静音循环播放
- **悬停效果** - 鼠标悬停时卡片缩放和阴影效果
- **平滑动画** - 所有过渡效果使用 GPU 加速

## 支持格式

| 类型 | 格式 |
|------|------|
| 图片 | JPG, JPEG, PNG, GIF, BMP, WebP, SVG |
| 视频 | MP4, AVI, MKV, MOV, WMV, FLV, WebM |
| 压缩包 | ZIP (CBZ) |

## 下载安装

从 [Releases](https://github.com/aqssxlzc/pic_viewer/releases) 页面下载对应平台的版本：

- **Windows**: 下载 `MediaViewer-windows-x64.zip`，解压后运行 `MediaViewer.exe`
- **macOS**: 下载 `MediaViewer-mac-x64.zip`，解压后运行 `MediaViewer.app`
- **Linux**: 下载 `MediaViewer-linux-x64.tar.gz`，解压后运行 `MediaViewer`

## 键盘快捷键

### 文件夹浏览模式

| 快捷键 | 功能 |
|--------|------|
| `ESC` / `Q` | 关闭预览窗口 |
| `←` / `A` / `↑` / `W` | 上一张图片 |
| `→` / `D` / `↓` / `S` | 下一张图片 |
| `Home` | 跳转到第一张 |
| `End` | 跳转到最后一张 |
| `PageUp` | 向前跳转 10 张 |
| `PageDown` | 向后跳转 10 张 |
| `Space` | 视频暂停/播放 |

### ZIP 文件浏览模式

| 快捷键 | 功能 |
|--------|------|
| `ESC` / `Q` | 关闭预览窗口 |
| `←` / `↑` / `W` | 上一张图片 |
| `→` / `↓` / `S` | 下一张图片 |
| `A` | 切换到上一个 ZIP 文件 |
| `D` | 切换到下一个 ZIP 文件 |
| `E` | 切换双图显示模式 |
| `Home` | 跳转到第一张 |
| `End` | 跳转到最后一张 |
| `PageUp` | 向前跳转 10 张 |
| `PageDown` | 向后跳转 10 张 |
| `Space` | 从第一张开始预览 / 视频暂停播放 |

## 从源码编译

### 依赖

- Qt 6.x (Core, Gui, Widgets, Multimedia, MultimediaWidgets, OpenGL, OpenGLWidgets, Concurrent)
- libzip
- C++17 编译器
- CMake 3.16+

### Linux

```bash
# 安装依赖 (Ubuntu/Debian)
sudo apt install qt6-base-dev qt6-multimedia-dev libgl1-mesa-dev libzip-dev cmake g++ make

# 编译
mkdir build && cd build
cmake ..
make -j$(nproc)
./MediaViewer
```

### Windows

```bash
# 使用 vcpkg 安装依赖
vcpkg install libzip:x64-windows

# 编译
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg路径]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### macOS

```bash
# 安装依赖
brew install qt libzip cmake

# 编译
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
make -j$(sysctl -n hw.ncpu)
./MediaViewer
```

## 使用方法

1. 运行程序
2. 点击"选择文件夹"按钮或"打开 ZIP"按钮
3. 选择包含图片/视频的文件夹或 ZIP 文件
4. 媒体文件将自动加载并以网格形式展示
5. 点击图片进入全屏预览模式

## 性能优化

- **分批加载**: 首次加载 30 个项目，滚动时自动加载更多
- **懒加载机制**: 只有可见的媒体项才会加载内容
- **GPU 加速**: 使用 OpenGL 进行硬件加速渲染
- **图片缩放优化**: 大图片自动缩放到合适尺寸作为缩略图
- **后台预加载**: 智能预加载相邻图片，提升浏览体验
- **视频优化**: 视频静音循环播放，减少资源占用

## 系统要求

- Windows 10+ / macOS 11+ / Linux (推荐)
- OpenGL 3.3+ 支持
- 4GB+ RAM (处理大量媒体文件时推荐)

## 许可证

MIT License
