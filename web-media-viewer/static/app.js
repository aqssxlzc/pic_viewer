let currentDirectory = '/home';
let sortAscending = false;
let allFiles = [];
let currentOffset = 0;
const batchSize = 30;
let currentViewerIndex = -1;
let isLoading = false;
let hasMoreFiles = true;
let totalFiles = 0;

// 初始化
window.addEventListener('load', () => {
    const savedDir = localStorage.getItem('lastDirectory');
    if (savedDir) {
        currentDirectory = savedDir;
        document.getElementById('dirInput').value = savedDir;
    }
    
    const savedSort = localStorage.getItem('sortAscending');
    if (savedSort) {
        sortAscending = savedSort === 'true';
        updateSortButton();
    }

    const savedScrollPos = localStorage.getItem('scrollPosition');
    loadFiles();

    if (savedScrollPos) {
        setTimeout(() => {
            document.querySelector('.main-content').scrollTop = parseInt(savedScrollPos);
        }, 100);
    }
});

// 保存滚动位置和检测自动加载
document.addEventListener('DOMContentLoaded', () => {
    const mainContent = document.querySelector('.main-content');
    if (mainContent) {
        mainContent.addEventListener('scroll', () => {
            localStorage.setItem('scrollPosition', mainContent.scrollTop);
            
            // 检测是否滚动到底部（距离底部500px时自动加载）
            const scrollTop = mainContent.scrollTop;
            const scrollHeight = mainContent.scrollHeight;
            const clientHeight = mainContent.clientHeight;
            const distanceFromBottom = scrollHeight - (scrollTop + clientHeight);
            
            // 当距离底部不足500px且还有更多文件时，自动加载
            if (distanceFromBottom < 500 && hasMoreFiles && !isLoading) {
                loadFiles();
            }
        });
    }
});

function changeDirectory() {
    const dir = document.getElementById('dirInput').value.trim() || '/home';
    currentDirectory = dir;
    localStorage.setItem('lastDirectory', currentDirectory);
    currentOffset = 0;
    allFiles = [];
    isLoading = false;
    hasMoreFiles = true;
    loadFiles();
}

function toggleSort() {
    sortAscending = !sortAscending;
    localStorage.setItem('sortAscending', sortAscending);
    updateSortButton();
    currentOffset = 0;
    allFiles = [];
    loadFiles();
}

function updateSortButton() {
    const btn = document.getElementById('sortBtn');
    btn.textContent = sortAscending ? '排序: 旧到新' : '排序: 新到旧';
}

async function loadFiles() {
    const grid = document.getElementById('grid');
    
    // 防止重复加载
    if (isLoading) return;
    isLoading = true;
    
    if (currentOffset === 0) {
        grid.innerHTML = '<div class="loading">加载文件中...</div>';
    }

    try {
        const params = new URLSearchParams({
            dir: currentDirectory,
            sort: sortAscending ? 'asc' : 'desc',
            offset: currentOffset,
            limit: batchSize
        });

        const response = await fetch(`/api/files?${params}`);

        if (!response.ok) {
            throw new Error('Failed to load files');
        }

        const data = await response.json();
        
        if (currentOffset === 0) {
            allFiles = data.files || [];
            grid.innerHTML = '';
            totalFiles = data.total || 0;
            hasMoreFiles = (currentOffset + batchSize) < totalFiles;
        } else {
            allFiles.push(...(data.files || []));
            totalFiles = data.total || 0;
            hasMoreFiles = (currentOffset + batchSize) < totalFiles;
        }

        // 渲染文件
        if (currentOffset === 0) {
            grid.innerHTML = '';
        }
        
        const filesCount = data.files ? data.files.length : 0;
        for (let i = 0; i < filesCount; i++) {
            const file = data.files[i];
            const index = allFiles.length - filesCount + i;
            const item = createMediaItem(file, index);
            grid.appendChild(item);
        }

        // 更新统计信息
        const stats = document.getElementById('stats');
        stats.textContent = `共 ${data.total || 0} 个文件 | ${data.imageCount || 0} 张图片 | ${data.videoCount || 0} 个视频`;

        // 隐藏"加载更多"按钮（因为现在是自动加载）
        const loadMoreBtn = document.getElementById('loadMoreBtn');
        if (hasMoreFiles) {
            loadMoreBtn.style.display = 'none';  // 自动加载时隐藏按钮
        } else {
            loadMoreBtn.style.display = 'none';
        }

        currentOffset += batchSize;
    } catch (error) {
        console.error('Error loading files:', error);
        const grid = document.getElementById('grid');
        if (currentOffset === 0) {
            grid.innerHTML = '<div class="loading">加载失败，请检查文件夹路径</div>';
        }
    } finally {
        isLoading = false;
    }
}

function createMediaItem(file, index) {
    const item = document.createElement('div');
    item.className = 'media-item';
    
    const imageUrl = `/api/file?path=${encodeURIComponent(file.path)}`;
    
    if (file.type === 'image') {
        const img = document.createElement('img');
        img.src = imageUrl;
        img.alt = file.name;
        item.appendChild(img);
    } else {
        const video = document.createElement('video');
        video.src = imageUrl;
        item.appendChild(video);
        
        const icon = document.createElement('div');
        icon.className = 'video-icon';
        item.appendChild(icon);
    }
    
    const overlay = document.createElement('div');
    overlay.className = 'media-item-overlay';
    overlay.textContent = file.name;
    item.appendChild(overlay);
    
    item.addEventListener('click', () => {
        currentViewerIndex = index;
        showViewer(file);
    });
    
    return item;
}

function loadMore() {
    loadFiles();
}

function showViewer(file) {
    const viewer = document.getElementById('viewer');
    const content = document.getElementById('viewerContent');
    
    const imageUrl = `/api/file?path=${encodeURIComponent(file.path)}`;
    
    content.innerHTML = '';
    if (file.type === 'image') {
        const img = document.createElement('img');
        img.src = imageUrl;
        img.alt = file.name;
        content.appendChild(img);
    } else {
        const video = document.createElement('video');
        video.src = imageUrl;
        video.controls = true;
        video.autoplay = true;
        video.style.maxWidth = '100%';
        video.style.maxHeight = '100%';
        content.appendChild(video);
    }
    
    viewer.classList.add('active');
}

function closeViewer() {
    document.getElementById('viewer').classList.remove('active');
}

function nextFile() {
    if (currentViewerIndex < allFiles.length - 1) {
        currentViewerIndex++;
        showViewer(allFiles[currentViewerIndex]);
    }
}

function previousFile() {
    if (currentViewerIndex > 0) {
        currentViewerIndex--;
        showViewer(allFiles[currentViewerIndex]);
    }
}

// 键盘快捷键
document.addEventListener('keydown', (e) => {
    const viewer = document.getElementById('viewer');
    if (!viewer || !viewer.classList.contains('active')) return;
    
    switch(e.key) {
        case 'Escape':
            closeViewer();
            break;
        case 'ArrowRight':
        case 'd':
        case 'D':
            nextFile();
            break;
        case 'ArrowLeft':
        case 'a':
        case 'A':
            previousFile();
            break;
    }
});

// 触摸滑动手势
let touchStartX = 0;
let touchEndX = 0;

document.addEventListener('touchstart', (e) => {
    const viewer = document.getElementById('viewer');
    if (!viewer || !viewer.classList.contains('active')) return;
    touchStartX = e.changedTouches[0].screenX;
}, false);

document.addEventListener('touchend', (e) => {
    const viewer = document.getElementById('viewer');
    if (!viewer || !viewer.classList.contains('active')) return;
    touchEndX = e.changedTouches[0].screenX;
    handleSwipe();
}, false);

function handleSwipe() {
    const swipeThreshold = 50; // 最小滑动距离（像素）
    const diff = touchStartX - touchEndX;
    
    // 右滑（上一个）：touchStartX > touchEndX
    if (Math.abs(diff) > swipeThreshold) {
        if (diff > 0) {
            // 向左滑动 → 下一个
            nextFile();
        } else {
            // 向右滑动 → 上一个
            previousFile();
        }
    }
}
