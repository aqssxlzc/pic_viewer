package main

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"net"
	"net/http"
	"os"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"time"

	"github.com/gorilla/mux"
)

type MediaFile struct {
	Name     string    `json:"name"`
	Path     string    `json:"path"`
	Type     string    `json:"type"` // "image" or "video"
	Modified time.Time `json:"modified"`
	Size     int64     `json:"size"`
}

type MediaResponse struct {
	Files      []MediaFile `json:"files"`
	Total      int         `json:"total"`
	ImageCount int         `json:"imageCount"`
	VideoCount int         `json:"videoCount"`
}

var (
	imageExtensions = map[string]bool{
		".jpg": true, ".jpeg": true, ".png": true, ".gif": true,
		".bmp": true, ".webp": true, ".svg": true,
	}
	videoExtensions = map[string]bool{
		".mp4": true, ".avi": true, ".mkv": true, ".mov": true,
		".wmv": true, ".flv": true, ".webm": true,
	}
)

// 获取文件夹中的媒体文件
func handleListFiles(w http.ResponseWriter, r *http.Request) {
	directory := r.URL.Query().Get("dir")
	sortBy := r.URL.Query().Get("sort") // "asc" or "desc"
	offset := 0
	limit := 30

	if dir := r.URL.Query().Get("offset"); dir != "" {
		if o, err := strconv.Atoi(dir); err == nil {
			offset = o
		}
	}

	if dir := r.URL.Query().Get("limit"); dir != "" {
		if l, err := strconv.Atoi(dir); err == nil && l > 0 {
			limit = l
		}
	}

	if directory == "" {
		http.Error(w, "directory parameter required", http.StatusBadRequest)
		return
	}

	// 验证目录存在
	info, err := os.Stat(directory)
	if err != nil || !info.IsDir() {
		http.Error(w, "invalid directory", http.StatusBadRequest)
		return
	}

	files, err := os.ReadDir(directory)
	if err != nil {
		http.Error(w, "failed to read directory", http.StatusInternalServerError)
		return
	}

	var mediaFiles []MediaFile
	imageCount := 0
	videoCount := 0

	for _, file := range files {
		if file.IsDir() {
			continue
		}

		ext := strings.ToLower(filepath.Ext(file.Name()))
		var fileType string

		if imageExtensions[ext] {
			fileType = "image"
			imageCount++
		} else if videoExtensions[ext] {
			fileType = "video"
			videoCount++
		} else {
			continue
		}

		fileInfo, err := file.Info()
		if err != nil {
			continue
		}

		mediaFiles = append(mediaFiles, MediaFile{
			Name:     file.Name(),
			Path:     filepath.Join(directory, file.Name()),
			Type:     fileType,
			Modified: fileInfo.ModTime(),
			Size:     fileInfo.Size(),
		})
	}

	// 排序
	sort.Slice(mediaFiles, func(i, j int) bool {
		if sortBy == "asc" {
			return mediaFiles[i].Modified.Before(mediaFiles[j].Modified)
		}
		return mediaFiles[i].Modified.After(mediaFiles[j].Modified)
	})

	// 分页
	total := len(mediaFiles)
	if offset >= total {
		offset = 0
	}
	end := offset + limit
	if end > total {
		end = total
	}

	paginatedFiles := mediaFiles[offset:end]

	response := MediaResponse{
		Files:      paginatedFiles,
		Total:      total,
		ImageCount: imageCount,
		VideoCount: videoCount,
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(response)
}

// 提供文件服务（图片、视频）
func handleFileProxy(w http.ResponseWriter, r *http.Request) {
	filePath := r.URL.Query().Get("path")
	if filePath == "" {
		http.Error(w, "path parameter required", http.StatusBadRequest)
		return
	}

	// 安全检查：防止目录遍历
	if strings.Contains(filePath, "..") {
		http.Error(w, "invalid path", http.StatusBadRequest)
		return
	}

	file, err := os.Open(filePath)
	if err != nil {
		http.Error(w, "file not found", http.StatusNotFound)
		return
	}
	defer file.Close()

	// 设置正确的Content-Type
	ext := strings.ToLower(filepath.Ext(filePath))
	contentType := getContentType(ext)
	if contentType != "" {
		w.Header().Set("Content-Type", contentType)
	}

	// 支持Range请求（用于视频流）
	w.Header().Set("Accept-Ranges", "bytes")

	// 获取文件大小以用于Range请求
	fileInfo, _ := file.Stat()
	fileSize := fileInfo.Size()

	// 处理Range请求
	rangeHeader := r.Header.Get("Range")
	if rangeHeader != "" {
		// 简单的Range支持
		if strings.HasPrefix(rangeHeader, "bytes=") {
			ranges := strings.TrimPrefix(rangeHeader, "bytes=")
			parts := strings.Split(ranges, "-")
			if len(parts) == 2 {
				start, _ := strconv.ParseInt(parts[0], 10, 64)
				end, _ := strconv.ParseInt(parts[1], 10, 64)
				if end == 0 {
					end = fileSize - 1
				}
				length := end - start + 1

				w.Header().Set("Content-Range", fmt.Sprintf("bytes %d-%d/%d", start, end, fileSize))
				w.Header().Set("Content-Length", strconv.FormatInt(length, 10))
				w.WriteHeader(http.StatusPartialContent)

				file.Seek(start, 0)
				io.CopyN(w, file, length)
				return
			}
		}
	}

	w.Header().Set("Content-Length", strconv.FormatInt(fileSize, 10))
	io.Copy(w, file)
}

// 获取Content-Type
func getContentType(ext string) string {
	switch ext {
	case ".jpg", ".jpeg":
		return "image/jpeg"
	case ".png":
		return "image/png"
	case ".gif":
		return "image/gif"
	case ".webp":
		return "image/webp"
	case ".svg":
		return "image/svg+xml"
	case ".mp4":
		return "video/mp4"
	case ".avi":
		return "video/x-msvideo"
	case ".mkv":
		return "video/x-matroska"
	case ".mov":
		return "video/quicktime"
	case ".wmv":
		return "video/x-ms-wmv"
	case ".webm":
		return "video/webm"
	case ".flv":
		return "video/x-flv"
	default:
		return "application/octet-stream"
	}
}

// 获取会话状态
func handleGetSession(w http.ResponseWriter, r *http.Request) {
	// 从cookie中读取或创建新的会话
	sessionData := map[string]interface{}{}

	// 这里可以从数据库或文件读取会话数据
	// 为简化起见，这里只是返回空会话

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(sessionData)
}

// 保存会话状态
func handleSaveSession(w http.ResponseWriter, r *http.Request) {
	var sessionData map[string]interface{}
	if err := json.NewDecoder(r.Body).Decode(&sessionData); err != nil {
		http.Error(w, "invalid request", http.StatusBadRequest)
		return
	}

	// 这里可以将会话数据保存到数据库或文件
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{"status": "ok"})
}

func main() {
	router := mux.NewRouter()

	// API路由
	router.HandleFunc("/api/files", handleListFiles).Methods("GET")
	router.HandleFunc("/api/file", handleFileProxy).Methods("GET")
	router.HandleFunc("/api/session", handleGetSession).Methods("GET")
	router.HandleFunc("/api/session", handleSaveSession).Methods("POST")

	// 静态文件
	router.PathPrefix("/static/").Handler(http.StripPrefix("/static/", http.FileServer(http.Dir("static"))))

	// 首页
	router.HandleFunc("/", serveHome).Methods("GET")

	// 启动服务器（IPv6/IPv4 双栈）
	addr := "[::]:8081"
	listener, err := net.Listen("tcp", addr)
	if err != nil {
		log.Fatalf("Failed to listen on %s: %v", addr, err)
	}
	log.Printf("Server starting on http://localhost:8081 (listening on %s)", addr)
	log.Fatal(http.Serve(listener, router))
}

func serveHome(w http.ResponseWriter, r *http.Request) {
	// 尝试从templates目录加载index.html
	content, err := os.ReadFile("templates/index.html")
	if err != nil {
		// 如果文件不存在，返回内联HTML
		w.Header().Set("Content-Type", "text/html; charset=utf-8")
		fmt.Fprint(w, getDefaultHTML())
		return
	}
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	w.Write(content)
}

func getDefaultHTML() string {
	return `<!DOCTYPE html>
<html lang="zh-CN">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>媒体文件查看器</title>
    <link rel="stylesheet" href="/static/style.css">
</head>
<body>
    <div class="header">
        <h1>📸 媒体文件查看器</h1>
        <div class="controls">
            <input type="text" id="dirInput" class="directory-input" placeholder="输入文件夹路径或使用默认值" value="/home">
            <button onclick="changeDirectory()">打开文件夹</button>
            <button class="sort-btn" id="sortBtn" onclick="toggleSort()">排序: 新到旧</button>
        </div>
        <div class="stats">
            <span id="stats">加载中...</span>
        </div>
    </div>

    <div class="main-content">
        <div class="grid" id="grid">
            <div class="loading">加载文件中...</div>
        </div>
        <button class="load-more-btn" id="loadMoreBtn" onclick="loadMore()" style="display:none;">加载更多</button>
    </div>

    <div class="viewer-modal" id="viewer">
        <div class="viewer-controls">
            <div>
                <button onclick="previousFile()">← 上一个</button>
                <button onclick="nextFile()">下一个 →</button>
            </div>
            <button class="close-btn" onclick="closeViewer()">关闭 (ESC)</button>
        </div>
        <div class="viewer-content" id="viewerContent">
            <div class="loading">加载中...</div>
        </div>
    </div>

    <script src="/static/app.js"></script>
</body>
</html>`
}
