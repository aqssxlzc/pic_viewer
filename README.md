# 媒体文件查看器 (Media Viewer)

一个高性能的原生桌面应用程序，用C++和Qt开发，用于浏览文件夹中的图片和视频。

## 功能特性

✅ **网格布局展示** - 自适应响应式网格，自动调整列数  
✅ **懒加载** - 分批加载媒体文件（每批30个），优化内存使用  
✅ **硬件加速** - 使用OpenGL加速渲染，流畅的动画效果  
✅ **图片预览** - 点击图片全屏查看，支持键盘导航  
✅ **视频自动播放** - 视频进入视口自动静音循环播放  
✅ **悬停效果** - 鼠标悬停时卡片缩放和阴影效果  
✅ **平滑动画** - 所有过渡效果使用GPU加速

## 支持格式

- **图片**: JPG, JPEG, PNG, GIF, BMP, WebP, SVG
- **视频**: MP4, AVI, MKV, MOV, WMV, FLV, WebM

## 依赖

- Qt 6.x (Core, Gui, Widgets, Multimedia, MultimediaWidgets, OpenGL)
- C++17 编译器
- CMake 3.16+ 或 qmake

## 编译方法

### 使用 CMake (推荐)

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
./MediaViewer
```

### 使用 qmake

```bash
qmake MediaViewer.pro
make -j$(nproc)
./MediaViewer
```

### 快速编译脚本

```bash
# 编译并运行
./build.sh
```

## 使用方法

1. 运行程序
2. 点击"选择文件夹"按钮
3. 选择包含图片和视频的文件夹
4. 媒体文件将自动加载并以网格形式展示

## 键盘快捷键

在图片预览模式下：
- **ESC** 或 **Q** - 关闭预览
- **←** - 上一张图片
- **→** - 下一张图片

## 性能优化

- **分批加载**: 首次加载30个项目，滚动时自动加载更多
- **懒加载机制**: 只有可见的媒体项才会加载内容
- **GPU加速**: 使用OpenGL进行硬件加速渲染
- **图片缩放优化**: 大图片自动缩放到合适尺寸作为缩略图
- **视频优化**: 视频静音循环播放，减少资源占用

## 系统要求

- Linux (推荐) / Windows / macOS
- OpenGL 3.3+ 支持
- 4GB+ RAM (处理大量媒体文件时)

## 许可证

MIT License
