# 🚀 快速开始

## 编译和运行 Web Media Viewer

### 方式1：使用编译脚本

```bash
cd web-media-viewer
chmod +x build.sh
./build.sh
./media-viewer
```

### 方式2：手动编译

```bash
cd web-media-viewer
go mod tidy
go build -o media-viewer main.go
./media-viewer
```

### 方式3：直接运行（无需编译）

```bash
cd web-media-viewer
go run main.go
```

## 访问应用

编译完成后，打开浏览器访问：

```
http://localhost:8080
```

## 使用示例

### 查看文件夹中的媒体

1. **输入文件夹路径**
   - 在顶部输入框输入完整路径
   - 例如：`/home/user/Pictures` 或 `/tmp/media`

2. **点击"打开文件夹"**
   - 应用会加载该文件夹中的所有图片和视频
   - 首次加载显示30个文件

3. **浏览媒体**
   - 向下滚动加载更多文件
   - 点击"加载更多"按钮显示下一批

### 排序文件

- 点击"排序: 新到旧"按钮切换排序方向
- 按修改时间排序

### 全屏预览

1. **点击任何媒体项**开启全屏预览
2. **使用键盘导航**：
   - `←` 或 `A` / `a` - 上一个文件
   - `→` 或 `D` / `d` - 下一个文件
   - `ESC` - 关闭预览

3. **视频播放**
   - 使用视频播放器的控件
   - 点击右上角"关闭"按钮退出预览

## 项目结构

```
web-media-viewer/
├── main.go              # 主应用程序
├── go.mod              # Go模块配置
├── go.sum              # 依赖锁文件
├── media-viewer        # 编译后的可执行文件
├── build.sh            # 构建脚本
├── README.md           # 完整文档
├── static/
│   ├── app.js          # 前端JavaScript代码
│   └── style.css       # 前端样式
└── templates/
    └── index.html      # HTML模板
```

## API端点

### 获取文件列表

```bash
curl "http://localhost:8080/api/files?dir=/home&sort=desc&offset=0&limit=30"
```

### 获取媒体文件

```bash
curl "http://localhost:8080/api/file?path=/absolute/path/to/file"
```

## 常见问题

### Q: 文件夹无法访问
A: 确保路径存在且应用有读取权限。

### Q: 某些文件类型未显示
A: 支持的格式有：jpg、jpeg、png、gif、webp、bmp、svg、mp4、avi、mkv、mov、wmv、flv、webm

### Q: 如何更改端口
A: 编辑 `main.go` 第 293 行，修改 `port := ":8080"` 中的端口号。

### Q: 如何在远程机器上访问
A: 
1. 在远程机器上运行应用
2. 在本地浏览器中访问 `http://<remote-ip>:8080`
3. 可选：使用反向代理（nginx）来添加身份验证

## 与 Qt 版本对比

| 特性 | Qt版本 | Web版本 | 说明 |
|------|-------|--------|------|
| 跨平台 | ✓ | ✓ | 都支持 Linux/macOS/Windows |
| 独立运行 | ✓ | ✗ | Qt是独立应用，Web需要浏览器 |
| 网络访问 | ✗ | ✓ | Web可远程访问 |
| 缩略图 | ✓ | ✓ | 都支持图片缩略图 |
| 键盘快捷键 | ✓ | ✓ | 都支持 |
| 会话保存 | ✓ | ✓ | 都保存最后目录/滚动位置 |
| 文件大小 | ~20MB | ~9MB | Web版本更小 |

## 进阶配置

### 配置为系统服务（Linux/systemd）

创建 `/etc/systemd/system/media-viewer.service`：

```ini
[Unit]
Description=Web Media Viewer
After=network.target

[Service]
Type=simple
User=chen
WorkingDirectory=/run/media/chen/.../web-media-viewer
ExecStart=/run/media/chen/.../web-media-viewer/media-viewer
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

然后运行：

```bash
sudo systemctl enable media-viewer
sudo systemctl start media-viewer
sudo systemctl status media-viewer
```

### 使用Nginx反向代理

```nginx
server {
    listen 80;
    server_name media.example.com;

    location / {
        proxy_pass http://localhost:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

## 性能优化

### 对于大量文件（1000+）

- 应用使用分批加载（每批30个）
- 使用虚拟滚动减少DOM节点
- 考虑在专业服务器上运行

### 对于大文件（1GB+视频）

- 支持HTTP Range请求用于流媒体
- 建议使用专业视频播放器
- 考虑预先转码为H.264格式

## 故障排除

### 内存占用高

如果内存占用过高，可能是因为：
1. 一次性加载了太多大型图片
2. 有很多标签页打开

解决方案：
- 关闭不需要的标签页
- 限制同时加载的图片数量
- 分割大文件夹为更小的子文件夹

### 响应缓慢

可能的原因：
1. 网络连接不好
2. 服务器磁盘I/O繁忙
3. 浏览器缓存满

解决方案：
- 检查网络连接
- 重启应用
- 清理浏览器缓存

## 下一步

- [查看完整文档](README.md)
- [对比两个版本的代码](../README.md)
- [提交问题或功能请求](https://github.com)

## 许可证

MIT License
