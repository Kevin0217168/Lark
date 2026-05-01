<template>
  <div class="birds-page">
    <!-- 背景装饰 -->
    <div class="background-decorations">
      <div class="decoration decoration-1"></div>
      <div class="decoration decoration-2"></div>
      <div class="decoration decoration-3"></div>
    </div>
    
    <div class="page-content">
      <h1 class="page-title" :class="{ 'fade-in': isLoaded }">我的雏鸟</h1>
      
      <!-- 未认领状态 -->
      <div v-if="!hasAdoptedBird && !loading">
        <!-- 提示信息 -->
        <div class="empty-section" :class="{ 'fade-in': isLoaded }">
          <div class="empty-card">
            <div class="empty-icon">🐦</div>
            <h3 class="empty-title">还没有认领雏鸟</h3>
            <p class="empty-text">点击下方按钮去认领一只可爱的雏鸟吧！</p>
          </div>
        </div>
        
        <!-- 操作按钮 -->
        <div class="action-section" :class="{ 'fade-in': isLoaded }">
          <button class="adopt-btn" @click="goToAdoptBirds">
            <span class="btn-icon">🏷️</span>
            <span class="btn-text">去认领雏鸟</span>
          </button>
        </div>
      </div>
      
      <!-- 已认领状态 - 全屏沉浸式设计 -->
      <div v-else-if="hasAdoptedBird && !loading" class="adopted-bird-section" :class="{ 'fade-in': isLoaded }">
        <!-- 雏鸟头部信息 -->
        <div class="bird-header">
          <h2 class="bird-name">{{ adoptedBird?.bird_name }}</h2>
          <div class="status-badge">已认领</div>
        </div>
        
        <!-- 雏鸟形象展示区 -->
        <div class="bird-hero-section">
          <!-- 监控控制栏 -->
          <div class="monitoring-controls">
            <div class="control-group">
              <button 
                class="control-btn" 
                @click="reconnectWebSocket"
              >
                <span class="btn-icon">🔄</span>
                <span class="btn-text">重连</span>
              </button>
              <button 
                class="control-btn" 
                :class="{ 'active': flipHorizontal }"
                @click="toggleHorizontalFlip"
              >
                <span class="btn-icon">🔄</span>
                <span class="btn-text">左右</span>
              </button>
              <button 
                class="control-btn" 
                :class="{ 'active': flipVertical }"
                @click="toggleVerticalFlip"
              >
                <span class="btn-icon">🔄</span>
                <span class="btn-text">上下</span>
              </button>
            </div>
            
            <!-- 视频质量和尺寸控制 -->
            <div class="video-settings">
              <div class="setting-item">
                <span class="setting-label">画质</span>
                <input 
                  type="range" 
                  v-model="imageQuality" 
                  min="1" 
                  max="63" 
                  step="1"
                  class="quality-slider"
                />
                <span class="setting-value">{{ imageQuality }}</span>
              </div>
              <div class="setting-item">
                <span class="setting-label">分辨率</span>
                <select v-model="frameSize" class="resolution-select">
                  <option value="FRAMESIZE_128X128">128x128</option>
                  <option value="FRAMESIZE_240X240">240x240</option>
                  <option value="FRAMESIZE_320X320">320x320</option>
                  <option value="FRAMESIZE_VGA">VGA</option>
                  <option value="FRAMESIZE_SVGA">SVGA</option>
                  <option value="FRAMESIZE_HD">HD</option>
                  <option value="FRAMESIZE_FHD">FHD</option>
                </select>
              </div>
            </div>
          </div>
          
          <div class="bird-image-container">
            <div v-if="currentFrameImage" class="video-container">
              <img 
                :src="currentFrameImage" 
                class="video-stream"
                :class="{ 'disconnected': isStreamDisconnected, 'flip-horizontal': flipHorizontal, 'flip-vertical': flipVertical }"
                alt="雏鸟实时画面"
              />
              <div class="frame-info">
                <span>更新时间: {{ lastFrameTime }}</span>
                <span class="fps-info">帧率: {{ currentFps }} FPS</span>
              </div>
              <div class="stream-disconnected-overlay" v-if="isStreamDisconnected">
                <div class="disconnected-content">
                  <div class="disconnected-icon">⚠️</div>
                  <div class="disconnected-message">视频流已断联</div>
                </div>
              </div>
            </div>
            <div v-else-if="connectionError" class="video-error">
              <div class="error-content">
                <div class="error-icon">❌</div>
                <div class="error-message">{{ connectionError }}</div>
                <button v-if="camDeviceId" class="retry-btn" @click="reconnectWebSocket">
                  重试连接
                </button>
              </div>
            </div>
            <div v-else class="image-placeholder">
              <div class="placeholder-icon">🐣</div>
              <p class="placeholder-text">雏鸟画面显示区域</p>
              <button v-if="camDeviceId" class="start-stream-btn" @click="startRealtimeMonitoring">
                开始监控
              </button>
            </div>
          </div>
          
          <!-- 关键状态信息 -->
          <div class="bird-status-info">
            <div class="status-item">
              <div class="status-value">{{ calculateAge(adoptedBird?.birth_date) }}</div>
              <div class="status-label">日龄</div>
            </div>
            <div class="status-divider"></div>
            <div class="status-item">
              <div class="status-value">{{ adoptedBird?.bird_species }}</div>
              <div class="status-label">品种</div>
            </div>
            <div class="status-divider"></div>
            <div class="status-item">
              <div class="status-value">{{ adoptedBird?.birdcage?.label || adoptedBird?.area ? `${adoptedBird?.area} #${adoptedBird?.number}` : '未绑定' }}</div>
              <div class="status-label">鸟笼</div>
            </div>
          </div>
        </div>
        
        <!-- 喂食操作区域 -->
        <div class="feeds-section" :class="{ 'fade-in': isLoaded }">
          <div class="feed-card">
            <div class="feed-card-visual">
              <div class="feed-bird-icon">🐣</div>
              <div class="feed-divider"></div>
              <div class="feed-icon">🍞</div>
            </div>
            <h3 class="feed-title">每日喂食</h3>
            <p class="feed-desc">每天可以喂食一次，帮助雏鸟健康成长</p>
            <div class="feed-status-row">
              <span v-if="feedingStatus?.completed" class="feed-status done">
                <span class="status-dot done"></span>今日已完成
              </span>
              <span v-else class="feed-status pending">
                <span class="status-dot pending"></span>剩余 {{ feedingStatus?.remaining ?? 1 }} / {{ feedingStatus?.daily_limit ?? 1 }} 次
              </span>
            </div>
            <div v-if="feedingStatus?.last_action_time" class="feed-last-time">
              上次喂食：{{ formatFeedTime(feedingStatus.last_action_time) }}
            </div>
            <button
              class="feed-action-btn"
              :class="{ 'disabled': feedingStatus?.completed || feedingLoading }"
              :disabled="feedingStatus?.completed || feedingLoading"
              @click="triggerFeeding"
            >
              <span class="btn-content">
                <span class="btn-icon-inner">{{ feedingLoading ? '⏳' : '' }}</span>
                <span class="btn-label">{{ feedingLoading ? '执行中...' : '立即喂食' }}</span>
              </span>
            </button>
          </div>
        </div>

        <!-- 环境数据环形进度条 -->
          <div class="env-gauges-section">
            <h3 class="gauges-title">环境数据</h3>
            <!-- 鸟笼未绑定提示 -->
            <div v-if="!adoptedBird?.birdcage" class="c3-missing-alert">
              <span class="c3-missing-icon">📦</span>
              <span>雏鸟尚未绑定鸟笼，无法获取环境数据</span>
            </div>
            <!-- C3 传感器缺失提示 -->
            <div v-else-if="adoptedBird?.birdcage && !c3DeviceId" class="c3-missing-alert">
              <span class="c3-missing-icon">⚠️</span>
              <span>鸟笼中未检测到 ESP32-C3 传感器设备，环境数据暂不可用</span>
            </div>
            <template v-if="c3DeviceId">
              <div class="gauges-row">
                <div class="gauge-item">
                  <div ref="aqiGaugeRef" class="gauge-chart"></div>
                  <div class="gauge-label">空气质量</div>
                </div>
                <div class="gauge-item">
                  <div ref="dbGaugeRef" class="gauge-chart"></div>
                  <div class="gauge-label">声音分贝</div>
                </div>
              </div>
              <div class="gauges-row gauges-row-second">
                <div class="gauge-item">
                  <div ref="luxGaugeRef" class="gauge-chart"></div>
                  <div class="gauge-label">光照强度</div>
                </div>
                <div class="gauge-item">
                  <div ref="uvGaugeRef" class="gauge-chart"></div>
                  <div class="gauge-label">紫外线指数</div>
                </div>
              </div>
            </template>
            <div v-if="camDeviceId" class="gauges-row gauges-row-third">
              <div class="gauge-item">
                <div ref="tempGaugeRef" class="gauge-chart"></div>
                <div class="gauge-label">温度</div>
              </div>
              <div class="gauge-item">
                <div ref="humidityGaugeRef" class="gauge-chart"></div>
                <div class="gauge-label">湿度</div>
              </div>
            </div>
        </div>
        
        <!-- 详细信息区域 -->
        <div class="bird-details-section">
          <h3 class="section-title">基本信息</h3>
          
          <div class="detail-item">
            <span class="detail-label">出生日期：</span>
            <span class="detail-value">{{ adoptedBird?.birth_date || '未知' }}</span>
          </div>
          
          <div class="detail-item description">
            <span class="detail-label">描述：</span>
            <p class="detail-text">{{ adoptedBird?.description || '暂无描述' }}</p>
          </div>
        </div>
        
        <!-- 操作区域 -->
        <div class="action-section" :class="{ 'fade-in': isLoaded }">
          <button class="release-btn" @click="releaseBird">
            <span class="btn-icon">🔓</span>
            <span class="btn-text">释放雏鸟</span>
          </button>
        </div>
      </div>
      
      <!-- 加载状态 -->
      <div v-else-if="loading" class="loading-section">
        <div class="loading-spinner"></div>
        <p class="loading-text">检查中...</p>
      </div>
      
      <!-- 未认领且无雏鸟状态 -->
      <div v-else class="empty-section" :class="{ 'fade-in': isLoaded }">
        <div class="empty-card">
          <div class="empty-icon">🐦</div>
          <p class="empty-text">暂无雏鸟信息</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, computed, onUnmounted, watch, nextTick } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import { api } from '@/utils/api';
import * as echarts from 'echarts';

const router = useRouter();

// 状态管理
const adoptedBird = ref<any>(null);
const loading = ref(true);
const isLoaded = ref(false);
const camDeviceId = computed(() => adoptedBird.value?.birdcage?.cam_device_id ?? null);
const c3DeviceId = computed(() => adoptedBird.value?.birdcage?.c3_device_id ?? null);

// WebSocket 相关状态
const currentFrameImage = ref<string>('');
const lastFrameTime = ref<string>('');
const connectionError = ref<string>('');
const isStreamDisconnected = ref<boolean>(false);
const currentFps = ref<number>(0);
let lastFrameTimestamp = 0;
let previousFrameTimestamp = 0;
let streamCheckInterval: number | null = null;
let ws: WebSocket | null = null;
let currentImageUrl: string = '';

// 帧率计算相关
const fpsHistory: number[] = [];
const MAX_FPS = 60; // 最大合理帧率
const MIN_FPS = 1; // 最小合理帧率
const SMOOTHING_FACTOR = 0.7; // 平滑因子
let smoothedFps = 0;

// 获取登录token
const getToken = () => {
  return localStorage.getItem('accessToken');
};

// 视频翻转控制
const flipHorizontal = ref(false);
const flipVertical = ref(false);
const imageQuality = ref(32); // 默认画质为32
const frameSize = ref('FRAMESIZE_VGA'); // 默认视频尺寸

// 喂食状态
const feedingStatus = ref<{ completed: boolean; today_count: number; daily_limit: number; remaining: number; last_action_time: string | null } | null>(null);
const feedingLoading = ref(false);
let feedStatusTimer: number | null = null;

// 计算属性：是否已认领雏鸟
const hasAdoptedBird = computed(() => {
  return adoptedBird.value !== null;
});

// 计算WebSocket是否已连接
const isWebSocketConnected = computed(() => {
  return ws !== null && ws.readyState === WebSocket.OPEN;
});

// 发送画质设置到WebSocket连接
const sendStreamQuality = async (quality: number) => {
  // 使用当前已建立的WebSocket连接发送画质设置
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    console.warn('WebSocket连接未建立，无法发送画质设置');
    return;
  }
  
  const message = JSON.stringify({
    code: 1,
    item: 'camera',
    key: 'jpeg_quality',
    values: quality.toString()
  });
  
  try {
    ws.send(message);
  } catch (error) {
  }
};

// 发送视频尺寸设置到WebSocket连接
const sendFrameSize = async (size: string) => {
  // 使用当前已建立的WebSocket连接发送视频尺寸设置
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    console.warn('WebSocket连接未建立，无法发送视频尺寸设置');
    return;
  }
  
  // 重置帧率计算
  fpsHistory.length = 0;
  currentFps.value = 0;
  smoothedFps = 0;
  previousFrameTimestamp = 0;
  
  const message = JSON.stringify({
    code: 1,
    item: 'camera',
    key: 'frame_size',
    values: size
  });
  
  try {
    ws.send(message);
  } catch (error) {
  }
};

// 监听滑块变化
watch(imageQuality, (newQuality) => {
  // 双端统一：1-63 -> 63-1
  const invertedQuality = 63 - newQuality;
  sendStreamQuality(invertedQuality);
});

// 监听视频尺寸变化
watch(frameSize, (newSize) => {
  sendFrameSize(newSize);
});

// 切换左右翻转
const toggleHorizontalFlip = () => {
  flipHorizontal.value = !flipHorizontal.value;
};

// 切换上下翻转
const toggleVerticalFlip = () => {
  flipVertical.value = !flipVertical.value;
};

// 计算日龄
const calculateAge = (birthDate: string) => {
  if (!birthDate) return 0;
  
  const birth = new Date(birthDate);
  const today = new Date();
  const diffTime = Math.abs(today.getTime() - birth.getTime());
  const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24));
  
  return diffDays;
};

// 释放雏鸟
const releaseBird = async () => {
  try {
    const response = await api.delete('/api/birds/adopted/me');
    
    if (response.code === 200) {
      ElMessage.success('释放成功！');
      adoptedBird.value = null;
      stopRealtimeMonitoring();
      stopEnvDataTimer();
      stopFeedStatusTimer();
      feedingStatus.value = null;
      aqiGaugeChart?.dispose();
      dbGaugeChart?.dispose();
      luxGaugeChart?.dispose();
      uvGaugeChart?.dispose();
      tempGaugeChart?.dispose();
      humidityGaugeChart?.dispose();
    } else {
      ElMessage.error(response.msg || '释放失败，请重试');
    }
  } catch (error: any) {
    if (error.data && error.data.msg) {
      ElMessage.error(error.data.msg);
    } else if (error.message) {
      ElMessage.error(error.message);
    } else {
      ElMessage.error('网络错误，请检查网络连接');
    }
  }
};

// 跳转到认领页面
const goToAdoptBirds = () => {
  router.push('/cloud/adopt-birds');
};

// 处理接收到的图片帧数据
const handleFrameData = (data: any) => {
  try {
    let blob: Blob;
    if (typeof data === 'string') {
      // 假设是base64编码的图片数据
      blob = base64ToBlob(data);
    } else if (data instanceof Blob) {
      blob = data;
    } else if (data instanceof ArrayBuffer) {
      blob = new Blob([data], { type: 'image/jpeg' });
    } else if (data instanceof Uint8Array) {
      // 使用类型断言告诉TypeScript这是一个有效的BlobPart
      blob = new Blob([data as any], { type: 'image/jpeg' });
    } else {
      throw new Error('不支持的数据类型');
    }
    
    // 释放旧的URL对象
    if (currentImageUrl) {
      URL.revokeObjectURL(currentImageUrl);
    }
    
    // 创建新的URL
    currentImageUrl = URL.createObjectURL(blob);
    currentFrameImage.value = currentImageUrl;
    lastFrameTime.value = new Date().toLocaleTimeString('zh-CN');
    
    // 计算帧率
    const now = Date.now();
    if (previousFrameTimestamp > 0) {
      const frameInterval = now - previousFrameTimestamp;
      if (frameInterval > 0) {
        let frameFps = 1000 / frameInterval;
        
        // 过滤异常值
        if (frameFps >= MIN_FPS && frameFps <= MAX_FPS) {
          fpsHistory.push(frameFps);
          // 只保留最近30帧的数据
          if (fpsHistory.length > 30) {
            fpsHistory.shift();
          }
        }
      }
    }
    previousFrameTimestamp = now;
    
    lastFrameTimestamp = now;
    isStreamDisconnected.value = false;
  } catch (error) {
    console.error('处理帧数据失败:', error);
  }
};

// 将base64字符串转换为Blob
const base64ToBlob = (base64: string) => {
  const parts = base64.split(';base64,');
  if (parts.length < 2 || !parts[1] || !parts[0]) {
    throw new Error('Invalid base64 string');
  }
  const contentParts = parts[0].split(':');
  const contentType = contentParts.length > 1 ? contentParts[1] : 'image/jpeg';
  const raw = window.atob(parts[1]);
  const rawLength = raw.length;
  const uInt8Array = new Uint8Array(rawLength);
  
  for (let i = 0; i < rawLength; ++i) {
    uInt8Array[i] = raw.charCodeAt(i);
  }
  
  return new Blob([uInt8Array], { type: contentType });
};

// 开始实时监控
const startRealtimeMonitoring = async () => {
  if (!camDeviceId.value) {
    ElMessage.warning('没有关联的鸟笼设备');
    return;
  }
  
  stopRealtimeMonitoring();
  
  try {
    let token = await refreshToken();
    if (!token) {
      connectionError.value = '未登录，请先登录';
      return;
    }
    
    const wsUrl = `/api/stream/viewer/ws/${camDeviceId.value}?token=${token}`;
    connectionError.value = '正在开启ws连接';
    ws = new WebSocket(wsUrl);
    
    ws.onopen = () => {
      connectionError.value = '';
      lastFrameTimestamp = Date.now();
      isStreamDisconnected.value = false;
      
      if (streamCheckInterval) {
        clearInterval(streamCheckInterval);
      }
      streamCheckInterval = window.setInterval(() => {
        const now = Date.now();
        if (now - lastFrameTimestamp > 2000 && currentFrameImage.value) {
          isStreamDisconnected.value = true;
        }
      }, 2500);
    };
    
    ws.onmessage = (event) => {
      try {
        if (typeof event.data === 'string' && event.data.includes('设备已断开')) {
          connectionError.value = '设备已断开';
          return;
        }
        
        try {
          const data = JSON.parse(event.data);
          if (data.code === 400) {
            connectionError.value = '订阅失败';
          }
        } catch (jsonError) {
          handleFrameData(event.data);
        }
      } catch (error) {
        console.error('处理WebSocket消息失败:', error);
      }
    };
    
    ws.onerror = () => {
      connectionError.value = 'WSS连接错误';
    };
    
    ws.onclose = (event) => {
      if (!event.wasClean) {
        connectionError.value = 'WSS连接已关闭';
      }
    };
    
  } catch (error) {
    connectionError.value = '建立WSS连接失败';
  }
};

// 手动重连WebSocket
const reconnectWebSocket = () => {
  if (ws && ws.readyState === WebSocket.OPEN) {
    // 简单版：直接重连，不显示确认对话框
    startRealtimeMonitoring();
  } else {
    startRealtimeMonitoring();
  }
};

// 刷新token
const refreshToken = async () => {
  try {
    const data = await api.post('/api/refresh');
    if (data.access_token) {
      localStorage.setItem('accessToken', data.access_token);
      return data.access_token;
    }
    return getToken();
  } catch (error) {
    return getToken();
  }
};

// 停止实时监控
const stopRealtimeMonitoring = async () => {
  // 清除图片流检查定时器
  if (streamCheckInterval) {
    clearInterval(streamCheckInterval);
    streamCheckInterval = null;
  }
  
  if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) {
    // 新的连接模式：WebSocket断开时会自动清理订阅关系
    ws.close();
    ws = null;
  }
  
  // 清空帧率历史数据
  fpsHistory.length = 0;
  currentFps.value = 0;
  smoothedFps = 0;
  previousFrameTimestamp = 0;
  
  if (currentImageUrl) {
    URL.revokeObjectURL(currentImageUrl);
    currentImageUrl = '';
  }
  
  currentFrameImage.value = '';
  connectionError.value = '';
  isStreamDisconnected.value = false;
};

// 计算平均帧率
const calculateAverageFps = () => {
  if (fpsHistory.length === 0) {
    currentFps.value = 0;
    smoothedFps = 0;
    return;
  }
  
  // 计算当前平均值
  const sum = fpsHistory.reduce((acc, fps) => acc + fps, 0);
  const currentAverage = sum / fpsHistory.length;
  
  // 使用指数移动平均（EMA）进行平滑
  if (smoothedFps === 0) {
    // 首次计算
    smoothedFps = currentAverage;
  } else {
    // EMA: 新值 = 旧值 * (1 - 平滑因子) + 当前值 * 平滑因子
    smoothedFps = smoothedFps * (1 - SMOOTHING_FACTOR) + currentAverage * SMOOTHING_FACTOR;
  }
  
  currentFps.value = Math.round(smoothedFps);
};

// 每0.5秒计算一次帧率
let fpsUpdateInterval: number | null = null;

// 启动帧率计算
const startFpsUpdate = () => {
  if (fpsUpdateInterval) {
    clearInterval(fpsUpdateInterval);
  }
  fpsUpdateInterval = window.setInterval(calculateAverageFps, 500);
};

// 停止帧率计算
const stopFpsUpdate = () => {
  if (fpsUpdateInterval) {
    clearInterval(fpsUpdateInterval);
    fpsUpdateInterval = null;
  }
};

// ─── 环境数据环形进度条相关 ───
// AQI 计算：根据 PM2.5、PM10、CO2、TVOC 综合计算空气质量指数
const calculateAQI = (pm25: number, pm10: number, co2: number, tvoc: number): number => {
  const pm25Breakpoints: [number, number, number, number][] = [
    [0, 35, 0, 50],
    [36, 75, 51, 100],
    [76, 115, 101, 150],
    [116, 150, 151, 200],
    [151, 250, 201, 300],
    [251, 500, 301, 500],
  ];
  const pm10Breakpoints: [number, number, number, number][] = [
    [0, 50, 0, 50],
    [51, 150, 51, 100],
    [151, 250, 101, 150],
    [251, 350, 151, 200],
    [351, 420, 201, 300],
    [421, 600, 301, 500],
  ];
  const co2Breakpoints: [number, number, number, number][] = [
    [0, 400, 0, 50],
    [401, 600, 51, 100],
    [601, 1000, 101, 150],
    [1001, 1400, 151, 200],
    [1401, 2000, 201, 300],
    [2001, 5000, 301, 500],
  ];
  const tvocBreakpoints: [number, number, number, number][] = [
    [0, 250, 0, 50],
    [251, 500, 51, 100],
    [501, 1000, 101, 150],
    [1001, 1600, 151, 200],
    [1601, 3000, 201, 300],
    [3001, 10000, 301, 500],
  ];

  const calc = (c: number, bp: [number, number, number, number][]): number => {
    if (c <= 0) return 0;
    for (const [cLo, cHi, iLo, iHi] of bp) {
      if (c >= cLo && c <= cHi) {
        return ((iHi - iLo) / (cHi - cLo)) * (c - cLo) + iLo;
      }
    }
    return 500;
  };

  const pm25Aqi = pm25 > 0 ? calc(pm25, pm25Breakpoints) : -1;
  const pm10Aqi = pm10 > 0 ? calc(pm10, pm10Breakpoints) : -1;
  const co2Aqi = co2 > 0 ? calc(co2, co2Breakpoints) : -1;
  const tvocAqi = tvoc > 0 ? calc(tvoc, tvocBreakpoints) : -1;

  const validAqi = [pm25Aqi, pm10Aqi, co2Aqi, tvocAqi].filter(v => v >= 0);
  return validAqi.length > 0 ? Math.round(Math.max(...validAqi)) : 0;
};

// ─── 5分钟滑动平均 ───
const MAX_HISTORY = 60;
const pm25History: number[] = [];
const pm10History: number[] = [];
const co2History: number[] = [];
const tvocHistory: number[] = [];
const luxHistory: number[] = [];
const uvHistory: number[] = [];

const pushHistory = (arr: number[], value: number) => {
  arr.push(value);
  if (arr.length > MAX_HISTORY) arr.shift();
};

const slidingAvg = (arr: number[]): number => {
  if (arr.length === 0) return 0;
  const sum = arr.reduce((a, b) => a + b, 0);
  return sum / arr.length;
};

// 环境数据值
const aqiValue = ref(0);
const dbValue = ref(0);
const tempValue = ref(0);
const humidityValue = ref(0);
const aqiGaugeRef = ref<HTMLElement | null>(null);
const dbGaugeRef = ref<HTMLElement | null>(null);
const luxGaugeRef = ref<HTMLElement | null>(null);
const uvGaugeRef = ref<HTMLElement | null>(null);
const tempGaugeRef = ref<HTMLElement | null>(null);
const humidityGaugeRef = ref<HTMLElement | null>(null);
let aqiGaugeChart: echarts.ECharts | null = null;
let dbGaugeChart: echarts.ECharts | null = null;
let luxGaugeChart: echarts.ECharts | null = null;
let uvGaugeChart: echarts.ECharts | null = null;
let tempGaugeChart: echarts.ECharts | null = null;
let humidityGaugeChart: echarts.ECharts | null = null;

// 环境数据采集定时器
let envDataTimer: number | null = null;

const updateAQIFromHistory = () => {
  aqiValue.value = calculateAQI(
    slidingAvg(pm25History),
    slidingAvg(pm10History),
    slidingAvg(co2History),
    slidingAvg(tvocHistory),
  );
  updateGaugeCharts();
};

const fetchAirQualityData = async () => {
  if (!c3DeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: c3DeviceId.value,
      sensor_type: 'pms9103m',
      limit: 1,
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const record = response.data[0];
      const parsed = JSON.parse(record.data);
      if (parsed.pm2_5_cf1 !== undefined || parsed.pm10_cf1 !== undefined) {
        pushHistory(pm25History, Number(parsed.pm2_5_cf1) || 0);
        pushHistory(pm10History, Number(parsed.pm10_cf1) || 0);
        updateAQIFromHistory();
      }
    }
  } catch (error) {
    console.error('获取颗粒物数据失败:', error);
  }
};

const fetchSGP30Data = async () => {
  if (!c3DeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: c3DeviceId.value,
      sensor_type: 'sgp30',
      limit: 1,
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const record = response.data[0];
      const parsed = JSON.parse(record.data);
      if (parsed.co2_ppm !== undefined || parsed.tvoc_ppb !== undefined) {
        pushHistory(co2History, Number(parsed.co2_ppm) || 0);
        pushHistory(tvocHistory, Number(parsed.tvoc_ppb) || 0);
        updateAQIFromHistory();
      }
    }
  } catch (error) {
    console.error('获取SGP30数据失败:', error);
  }
};

const fetchSoundData = async () => {
  if (!c3DeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: c3DeviceId.value,
      sensor_type: 'sound_meter',
      limit: 1,
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const record = response.data[0];
      const parsed = JSON.parse(record.data);
      if (parsed.db !== undefined) {
        dbValue.value = Math.round(parsed.db);
        updateGaugeCharts();
      }
    }
  } catch (error) {
    console.error('获取声音分贝数据失败:', error);
  }
};

const fetchLuxData = async () => {
  if (!c3DeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: c3DeviceId.value,
      sensor_type: 'veml7700',
      limit: 1,
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const record = response.data[0];
      const parsed = JSON.parse(record.data);
      if (parsed.lux !== undefined) {
        pushHistory(luxHistory, Number(parsed.lux) || 0);
        updateLuxGauge();
      }
    }
  } catch (error) {
    console.error('获取光照数据失败:', error);
  }
};

const fetchUVData = async () => {
  if (!c3DeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: c3DeviceId.value,
      sensor_type: 'uv_meter',
      limit: 1,
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const record = response.data[0];
      const parsed = JSON.parse(record.data);
      if (parsed.uv_index !== undefined) {
        pushHistory(uvHistory, Number(parsed.uv_index) || 0);
        updateUVGauge();
      }
    }
  } catch (error) {
    console.error('获取紫外线数据失败:', error);
  }
};

const fetchTempHumidityData = async () => {
  if (!camDeviceId.value) return;
  try {
    const response = await api.get('/api/sensors/grouped', {
      period: '300',
      group: '1',
      device_id: camDeviceId.value.toString(),
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const latest = response.data[0];
      const temp = latest.avg_temperature;
      const hum = latest.avg_humidity;
      if (temp !== undefined && temp !== null) {
        tempValue.value = Math.round(Number(temp) * 10) / 10;
      }
      if (hum !== undefined && hum !== null) {
        humidityValue.value = Math.round(Number(hum) * 10) / 10;
      }
      updateGaugeCharts();
    }
  } catch (error) {
    console.error('获取温湿度数据失败:', error);
  }
};

const fetchAllEnvData = () => {
  fetchAirQualityData();
  fetchSGP30Data();
  fetchSoundData();
  fetchLuxData();
  fetchUVData();
  fetchTempHumidityData();
};

const startEnvDataTimer = () => {
  stopEnvDataTimer();
  fetchAllEnvData();
  envDataTimer = window.setInterval(fetchAllEnvData, 5000);
};

const stopEnvDataTimer = () => {
  if (envDataTimer) {
    clearInterval(envDataTimer);
    envDataTimer = null;
  }
};

// 获取光照强度颜色
const getLuxColor = (lux: number) => {
  if (lux > 50000) return '#f56c6c';
  if (lux > 25000) return '#e6a23c';
  return '#67c23a';
};
const getLuxBg = (lux: number) => {
  if (lux > 50000) return '#f56c6c1a';
  if (lux > 25000) return '#e6a23c1a';
  return '#67c23a1a';
};

// 获取紫外线颜色
const getUVColor = (uv: number) => {
  if (uv > 11) return '#f56c6c';
  if (uv > 8) return '#f56c6c';
  if (uv > 5) return '#e6a23c';
  if (uv > 2) return '#e6a23c';
  return '#67c23a';
};
const getUVBg = (uv: number) => {
  if (uv > 11) return '#f56c6c1a';
  if (uv > 8) return '#f56c6c1a';
  if (uv > 5) return '#e6a23c1a';
  if (uv > 2) return '#e6a23c1a';
  return '#67c23a1a';
};

// Lux 值格式化
const formatLux = (v: number) => {
  if (v >= 100000) return (v / 100000).toFixed(1) + 'M';
  if (v >= 1000) return (v / 1000).toFixed(1) + 'K';
  return String(Math.round(v));
};

const getGaugeOption = (max: number, color: string, bgColor: string, value: number, name: string, formatter?: (v: number) => string) => ({
  series: [{
    type: 'gauge',
    radius: '95%',
    center: ['50%', '60%'],
    startAngle: 180,
    endAngle: 0,
    min: 0,
    max,
    splitNumber: 1,
    pointer: {
      show: false
    },
    progress: {
      show: true,
      overlap: false,
      roundCap: true,
      clip: false,
      itemStyle: {
        color: {
          type: 'linear',
          x: 0, y: 0, x2: 1, y2: 0,
          colorStops: [
            { offset: 0, color: color },
            { offset: 1, color: color + 'aa' }
          ]
        },
        shadowColor: color + '40',
        shadowBlur: 10,
        shadowOffsetY: 2
      }
    },
    axisLine: {
      lineStyle: {
        width: 14,
        color: [[1, bgColor]]
      }
    },
    splitLine: { show: false },
    axisTick: { show: false },
    axisLabel: { show: false },
    title: { show: false },
    detail: {
      show: true,
      valueAnimation: true,
      fontSize: 20,
      fontWeight: 'bold',
      color: color,
      formatter: formatter ? formatter : '{value}',
      offsetCenter: [0, '0%']
    },
    data: [{ value: Math.min(value, max), name }]
  }]
});

const initGaugeCharts = () => {
  if (aqiGaugeRef.value) {
    if (aqiGaugeChart) aqiGaugeChart.dispose();
    aqiGaugeChart = echarts.init(aqiGaugeRef.value);
  }
  if (dbGaugeRef.value) {
    if (dbGaugeChart) dbGaugeChart.dispose();
    dbGaugeChart = echarts.init(dbGaugeRef.value);
  }
  if (luxGaugeRef.value) {
    if (luxGaugeChart) luxGaugeChart.dispose();
    luxGaugeChart = echarts.init(luxGaugeRef.value);
  }
  if (uvGaugeRef.value) {
    if (uvGaugeChart) uvGaugeChart.dispose();
    uvGaugeChart = echarts.init(uvGaugeRef.value);
  }
  if (tempGaugeRef.value) {
    if (tempGaugeChart) tempGaugeChart.dispose();
    tempGaugeChart = echarts.init(tempGaugeRef.value);
  }
  if (humidityGaugeRef.value) {
    if (humidityGaugeChart) humidityGaugeChart.dispose();
    humidityGaugeChart = echarts.init(humidityGaugeRef.value);
  }
  initGaugeChartOption();
  updateGaugeCharts();
};

const initGaugeChartOption = () => {
  const aqiColor = aqiValue.value > 150 ? '#f56c6c' : aqiValue.value > 100 ? '#e6a23c' : '#67c23a';
  const dbColor = dbValue.value > 85 ? '#f56c6c' : dbValue.value > 65 ? '#e6a23c' : '#67c23a';
  const aqiBg = aqiValue.value > 150 ? '#f56c6c1a' : aqiValue.value > 100 ? '#e6a23c1a' : '#67c23a1a';
  const dbBg = dbValue.value > 85 ? '#f56c6c1a' : dbValue.value > 65 ? '#e6a23c1a' : '#67c23a1a';
  const luxVal = slidingAvg(luxHistory);
  const luxColorVal = getLuxColor(luxVal);
  const luxBgVal = getLuxBg(luxVal);
  const uvVal = slidingAvg(uvHistory);
  const uvColorVal = getUVColor(uvVal);
  const uvBgVal = getUVBg(uvVal);

  if (aqiGaugeChart) {
    aqiGaugeChart.setOption(getGaugeOption(300, aqiColor, aqiBg, aqiValue.value, 'AQI'));
  }
  if (dbGaugeChart) {
    dbGaugeChart.setOption(getGaugeOption(120, dbColor, dbBg, dbValue.value, 'dB'));
  }
  if (luxGaugeChart) {
    luxGaugeChart.setOption(getGaugeOption(100000, luxColorVal, luxBgVal, luxVal, 'Lux', formatLux));
  }
  if (uvGaugeChart) {
    uvGaugeChart.setOption(getGaugeOption(12, uvColorVal, uvBgVal, uvVal, 'UV'));
  }
  if (tempGaugeChart) {
    tempGaugeChart.setOption(getGaugeOption(50, '#ff7875', '#ff78751a', tempValue.value, '℃'));
  }
  if (humidityGaugeChart) {
    humidityGaugeChart.setOption(getGaugeOption(100, '#69c0ff', '#69c0ff1a', humidityValue.value, '%'));
  }
};

const updateGaugeCharts = () => {
  const aqiColor = aqiValue.value > 150 ? '#f56c6c' : aqiValue.value > 100 ? '#e6a23c' : '#67c23a';
  const dbColor = dbValue.value > 85 ? '#f56c6c' : dbValue.value > 65 ? '#e6a23c' : '#67c23a';
  const aqiBg = aqiValue.value > 150 ? '#f56c6c1a' : aqiValue.value > 100 ? '#e6a23c1a' : '#67c23a1a';
  const dbBg = dbValue.value > 85 ? '#f56c6c1a' : dbValue.value > 65 ? '#e6a23c1a' : '#67c23a1a';
  const luxVal = slidingAvg(luxHistory);
  const luxColorVal = getLuxColor(luxVal);
  const luxBgVal = getLuxBg(luxVal);
  const uvVal = slidingAvg(uvHistory);
  const uvColorVal = getUVColor(uvVal);
  const uvBgVal = getUVBg(uvVal);

  if (aqiGaugeChart) {
    aqiGaugeChart.setOption(getGaugeOption(300, aqiColor, aqiBg, aqiValue.value, 'AQI'));
  }
  if (dbGaugeChart) {
    dbGaugeChart.setOption(getGaugeOption(120, dbColor, dbBg, dbValue.value, 'dB'));
  }
  if (luxGaugeChart) {
    luxGaugeChart.setOption(getGaugeOption(100000, luxColorVal, luxBgVal, luxVal, 'Lux', formatLux));
  }
  if (uvGaugeChart) {
    uvGaugeChart.setOption(getGaugeOption(12, uvColorVal, uvBgVal, uvVal, 'UV'));
  }
  if (tempGaugeChart) {
    tempGaugeChart.setOption(getGaugeOption(50, '#ff7875', '#ff78751a', tempValue.value, '℃'));
  }
  if (humidityGaugeChart) {
    humidityGaugeChart.setOption(getGaugeOption(100, '#69c0ff', '#69c0ff1a', humidityValue.value, '%'));
  }
};

const updateLuxGauge = () => {
  const luxVal = slidingAvg(luxHistory);
  const luxColorVal = getLuxColor(luxVal);
  const luxBgVal = getLuxBg(luxVal);
  if (luxGaugeChart) {
    luxGaugeChart.setOption(getGaugeOption(100000, luxColorVal, luxBgVal, luxVal, 'Lux', formatLux));
  }
};

const updateUVGauge = () => {
  const uvVal = slidingAvg(uvHistory);
  const uvColorVal = getUVColor(uvVal);
  const uvBgVal = getUVBg(uvVal);
  if (uvGaugeChart) {
    uvGaugeChart.setOption(getGaugeOption(12, uvColorVal, uvBgVal, uvVal, 'UV'));
  }
};

// ─── 喂食功能 ───
const formatFeedTime = (isoString: string) => {
  const d = new Date(isoString);
  return d.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' });
};

const fetchFeedStatus = async () => {
  if (!camDeviceId.value) return;
  const birdId = adoptedBird.value?.bird_id || adoptedBird.value?.id;
  if (!birdId) return;
  try {
    const response = await api.get('/api/feeding/status', {
      bird_id: birdId,
    });
    if (response.code === 200 && response.data) {
      feedingStatus.value = response.data.feeding;
    }
  } catch (error) {
    console.error('获取喂食状态失败:', error);
  }
};

const startFeedStatusTimer = () => {
  stopFeedStatusTimer();
  fetchFeedStatus();
  feedStatusTimer = window.setInterval(fetchFeedStatus, 30000);
};

const stopFeedStatusTimer = () => {
  if (feedStatusTimer) {
    clearInterval(feedStatusTimer);
    feedStatusTimer = null;
  }
};

const triggerFeeding = async () => {
  if (feedingLoading.value || feedingStatus.value?.completed) return;
  const birdId = adoptedBird.value?.bird_id || adoptedBird.value?.id;
  if (!birdId) {
    ElMessage.warning('未找到雏鸟信息，无法执行喂食');
    return;
  }
  feedingLoading.value = true;
  try {
    const response = await api.post(`/api/feeding/trigger?bird_id=${birdId}`);
    ElMessage.closeAll();
    if (response.code === 200) {
      ElMessage.success('喂食指令已发送！');
      await fetchFeedStatus();
    } else {
      ElMessage.error(response.msg || '喂食失败');
    }
  } catch (error: any) {
    ElMessage.closeAll();
    if (error.data && error.data.msg) {
      ElMessage.error(error.data.msg);
    } else if (error.message) {
      ElMessage.error(error.message);
    } else {
      ElMessage.error('网络错误，请检查网络连接');
    }
  } finally {
    feedingLoading.value = false;
  }
};

// 检查用户是否已认领雏鸟
const checkAdoptedBird = async () => {
  try {
    const response = await api.get('/api/birds/adopted/me');
    console.log('Adopted bird response:', response);

    if (response.code === 200 && response.data?.adopted_bird && Object.keys(response.data.adopted_bird).length > 0) {
      adoptedBird.value = response.data.adopted_bird;
      console.log('Adopted bird data:', response.data.adopted_bird);
      console.log('CAM device ID:', camDeviceId.value, 'C3 device ID:', c3DeviceId.value);
    } else {
      adoptedBird.value = null;
    }
  } catch (error) {
    console.error('Error fetching adopted bird:', error);
    adoptedBird.value = null;
  } finally {
    loading.value = false;
    // 延迟显示动画
    setTimeout(() => {
      isLoaded.value = true;
    }, 100);
  }
};

// 页面加载时检查
onMounted(() => {
  checkAdoptedBird();
  startFpsUpdate();
});

// 监听 adoptedBird 变化，当有设备时启动环境数据采集
watch(adoptedBird, (newBird) => {
  if (newBird && camDeviceId.value) {
    nextTick(() => {
      initGaugeCharts();
      startEnvDataTimer();
      startFeedStatusTimer();
    });
  } else {
    stopEnvDataTimer();
    stopFeedStatusTimer();
  }
});

// 监听 camDeviceId 变化
watch(camDeviceId, (newId) => {
  if (newId) {
    nextTick(() => {
      initGaugeCharts();
      startEnvDataTimer();
      startFeedStatusTimer();
    });
  } else {
    stopEnvDataTimer();
    stopFeedStatusTimer();
  }
});

// 页面卸载时清理
onUnmounted(() => {
  stopRealtimeMonitoring();
  stopFpsUpdate();
  stopEnvDataTimer();
  stopFeedStatusTimer();
  aqiGaugeChart?.dispose();
  dbGaugeChart?.dispose();
  luxGaugeChart?.dispose();
  uvGaugeChart?.dispose();
  tempGaugeChart?.dispose();
  humidityGaugeChart?.dispose();
});
</script>

<style scoped>
.birds-page {
  min-height: 100vh;
  background: linear-gradient(135deg, #E6F4EA 0%, #D4EBD0 50%, #B8DFC7 100%);
  padding-bottom: calc(80px + env(safe-area-inset-bottom, 0px));
  position: relative;
  overflow: hidden;
}

/* 背景装饰 */
.background-decorations {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 1;
}

.decoration {
  position: absolute;
  border-radius: 50%;
  background: rgba(139, 173, 66, 0.1);
  filter: blur(60px);
  animation: float 8s ease-in-out infinite;
}

.decoration-1 {
  top: 10%;
  left: 10%;
  width: 200px;
  height: 200px;
  animation-delay: 0s;
}

.decoration-2 {
  top: 60%;
  right: 10%;
  width: 150px;
  height: 150px;
  animation-delay: 2s;
}

.decoration-3 {
  bottom: 20%;
  left: 20%;
  width: 180px;
  height: 180px;
  animation-delay: 4s;
}

/* 浮动动画 */
@keyframes float {
  0%, 100% {
    transform: translateY(0px) translateX(0px);
  }
  25% {
    transform: translateY(-20px) translateX(10px);
  }
  50% {
    transform: translateY(10px) translateX(-10px);
  }
  75% {
    transform: translateY(-15px) translateX(5px);
  }
}

.page-content {
  padding: 20px 16px;
  position: relative;
  z-index: 2;
}

.page-title {
  font-size: 24px;
  font-weight: 700;
  color: #166534;
  margin-bottom: 20px;
  text-align: center;
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out;
}

/* 渐入动画 */
.fade-in {
  opacity: 1 !important;
  transform: translateY(0) !important;
}

/* 操作区域 */
.action-section {
  margin-bottom: 20px;
  text-align: center;
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.2s;
  display: flex;
  justify-content: center;
  padding: 0 16px 20px;
}

.release-btn {
  background: linear-gradient(135deg, #f87171 0%, #ef4444 100%);
  color: white;
  border: none;
  border-radius: 16px;
  padding: 16px 36px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  gap: 8px;
  transition: all 0.3s ease;
  box-shadow: 0 4px 16px rgba(239, 68, 68, 0.3);
  min-width: 220px;
  justify-content: center;
  position: relative;
  overflow: hidden;
}

.release-btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.release-btn:hover::before {
  left: 100%;
}

.release-btn:hover {
  transform: translateY(-3px);
  box-shadow: 0 6px 20px rgba(239, 68, 68, 0.4);
  background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
}

.release-btn:active {
  transform: translateY(0);
  box-shadow: 0 2px 8px rgba(239, 68, 68, 0.3);
}

.adopt-btn {
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
  color: white;
  border: none;
  border-radius: 16px;
  padding: 14px 28px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  gap: 8px;
  box-shadow: 0 4px 16px rgba(139, 173, 66, 0.3);
  transition: all 0.3s ease;
  position: relative;
  overflow: hidden;
}

.adopt-btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.adopt-btn:hover::before {
  left: 100%;
}

.adopt-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(139, 173, 66, 0.4);
}

.adopt-btn:active {
  transform: translateY(0);
  box-shadow: 0 2px 8px rgba(139, 173, 66, 0.3);
}

.btn-icon {
  font-size: 18px;
}

.btn-text {
  font-size: 16px;
}

/* 已认领状态 - 全屏沉浸式设计 */
.adopted-bird-section {
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.2s;
  display: flex;
  flex-direction: column;
  gap: 24px;
}

/* 雏鸟头部信息 */
.bird-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 16px;
}

.bird-name {
  font-size: 28px;
  font-weight: 700;
  color: #166534;
  margin: 0;
  text-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.status-badge {
  background: rgba(16, 185, 129, 0.1);
  color: #10b981;
  padding: 8px 16px;
  border-radius: 20px;
  font-size: 14px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  box-shadow: 0 2px 8px rgba(16, 185, 129, 0.2);
}

/* 雏鸟形象展示区 */
.bird-hero-section {
  display: flex;
  flex-direction: column;
  gap: 24px;
  padding: 0 16px;
}

.bird-image-container {
  border-radius: 20px;
  overflow: hidden;
  background: linear-gradient(135deg, #f0fdf4 0%, #dcfce7 100%);
  border: 2px dashed rgba(139, 173, 66, 0.3);
  min-height: 500px;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 8px 32px rgba(139, 173, 66, 0.1);
  transition: all 0.3s ease;
}

.bird-image-container:hover {
  box-shadow: 0 12px 40px rgba(139, 173, 66, 0.15);
  transform: translateY(-2px);
}

.image-placeholder {
  padding: 60px 40px;
  text-align: center;
}

.image-placeholder .placeholder-icon {
  font-size: 80px;
  margin-bottom: 16px;
  animation: pulse 2s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% {
    transform: scale(1);
  }
  50% {
    transform: scale(1.1);
  }
}

.image-placeholder .placeholder-text {
  font-size: 16px;
  color: #6b7280;
  font-weight: 500;
  opacity: 0.8;
  margin-bottom: 16px;
}

.start-stream-btn {
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
  color: white;
  border: none;
  border-radius: 12px;
  padding: 10px 20px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  box-shadow: 0 4px 12px rgba(139, 173, 66, 0.3);
  margin-top: 8px;
}

.start-stream-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(139, 173, 66, 0.4);
}

/* 视频流容器 */
.video-container {
  position: relative;
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 16px;
  overflow: hidden;
  background: #f3f4f6;
}

.video-stream {
  max-width: 100%;
  max-height: 400px;
  width: 100%;
  height: auto;
  object-fit: contain;
  border-radius: 16px;
  transition: all 0.3s ease;
}

.video-stream.disconnected {
  opacity: 0.7;
  filter: grayscale(30%);
}

/* 视频翻转效果 */
.video-stream.flip-horizontal {
  transform: scaleX(-1);
}

.video-stream.flip-vertical {
  transform: scaleY(-1);
}

.video-stream.flip-horizontal.flip-vertical {
  transform: scaleX(-1) scaleY(-1);
}

/* 帧信息 */
.frame-info {
  position: absolute;
  bottom: 8px;
  left: 8px;
  right: 8px;
  background: rgba(0, 0, 0, 0.6);
  color: white;
  padding: 6px 12px;
  border-radius: 8px;
  font-size: 12px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  z-index: 10;
}

.fps-info {
  font-weight: 600;
  color: #10b981;
}

/* 断联覆盖层 */
.stream-disconnected-overlay {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.7);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 20;
  border-radius: 16px;
}

.disconnected-content {
  text-align: center;
  color: white;
}

.disconnected-icon {
  font-size: 48px;
  margin-bottom: 12px;
}

.disconnected-message {
  font-size: 16px;
  font-weight: 600;
}

/* 错误状态 */
.video-error {
  display: flex;
  align-items: center;
  justify-content: center;
  height: 100%;
  padding: 40px 20px;
  text-align: center;
  background: #fef2f2;
  border-radius: 16px;
  border: 1px solid #fecaca;
}

.error-content {
  max-width: 280px;
}

.error-icon {
  font-size: 48px;
  margin-bottom: 16px;
  color: #ef4444;
}

.error-message {
  font-size: 14px;
  color: #b91c1c;
  margin-bottom: 20px;
  line-height: 1.4;
}

.retry-btn {
  background: #ef4444;
  color: white;
  border: none;
  border-radius: 12px;
  padding: 10px 20px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s ease;
  box-shadow: 0 4px 12px rgba(239, 68, 68, 0.3);
}

.retry-btn:hover {
  background: #dc2626;
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(239, 68, 68, 0.4);
}

/* 关键状态信息 */
.bird-status-info {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 32px;
  background: rgba(255, 255, 255, 0.7);
  backdrop-filter: blur(16px);
  border-radius: 20px;
  padding: 20px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.5);
}

.status-item {
  text-align: center;
  flex: 1;
}

.status-value {
  font-size: 32px;
  font-weight: 700;
  color: #166534;
  line-height: 1;
  margin-bottom: 4px;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
}

.status-label {
  font-size: 14px;
  color: #6b7280;
  font-weight: 500;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.status-divider {
  width: 1px;
  height: 40px;
  background: rgba(139, 173, 66, 0.2);
}

/* 详细信息区域 */
.bird-details-section {
  background: rgba(255, 255, 255, 0.7);
  backdrop-filter: blur(16px);
  border-radius: 20px;
  padding: 24px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.5);
  margin: 0 16px;
}

.section-title {
  font-size: 18px;
  font-weight: 600;
  color: #166534;
  margin: 0 0 16px 0;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
}

.detail-item {
  display: flex;
  align-items: flex-start;
  gap: 12px;
  margin-bottom: 16px;
}

.detail-item:last-child {
  margin-bottom: 0;
}

.detail-item.description {
  flex-direction: column;
  align-items: flex-start;
  gap: 8px;
}

.detail-label {
  font-size: 14px;
  font-weight: 600;
  color: #4b5563;
  min-width: 90px;
  flex-shrink: 0;
}

.detail-value {
  font-size: 16px;
  color: #6b7280;
  flex: 1;
  line-height: 1.4;
}

.detail-text {
  font-size: 16px;
  color: #6b7280;
  line-height: 1.5;
  margin: 0;
  flex: 1;
}

/* 喂食操作区域 */
.feeds-section {
  background: rgba(255, 255, 255, 0.7);
  backdrop-filter: blur(16px);
  border-radius: 20px;
  padding: 28px 24px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.5);
  margin: 0 16px;
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.3s;
}

.feeds-section.fade-in {
  opacity: 1;
  transform: translateY(0);
}

.feed-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 14px;
}

.feed-card-visual {
  display: flex;
  align-items: center;
  gap: 16px;
  background: linear-gradient(135deg, #f0fdf4 0%, #dcfce7 100%);
  border: 2px solid rgba(139, 173, 66, 0.15);
  border-radius: 50px;
  padding: 12px 28px;
}

.feed-bird-icon {
  font-size: 36px;
  animation: bird-bounce 2s ease-in-out infinite;
}

@keyframes bird-bounce {
  0%, 100% { transform: translateY(0); }
  50% { transform: translateY(-4px); }
}

.feed-divider {
  width: 2px;
  height: 32px;
  background: linear-gradient(to bottom, transparent, rgba(139, 173, 66, 0.3), transparent);
  border-radius: 1px;
}

.feed-icon {
  font-size: 32px;
}

.feed-title {
  font-size: 20px;
  font-weight: 700;
  color: #166534;
  margin: 0;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.08);
}

.feed-desc {
  font-size: 14px;
  color: #6b7280;
  text-align: center;
  margin: 0;
  line-height: 1.5;
}

.feed-status-row {
  text-align: center;
}

.feed-status {
  font-size: 14px;
  font-weight: 500;
  display: flex;
  align-items: center;
  gap: 6px;
  justify-content: center;
}

.feed-status.done {
  color: #10b981;
}

.feed-status.pending {
  color: #e6a23c;
}

.status-dot {
  display: inline-block;
  width: 8px;
  height: 8px;
  border-radius: 50%;
}

.status-dot.done {
  background: #10b981;
  box-shadow: 0 0 6px rgba(16, 185, 129, 0.4);
}

.status-dot.pending {
  background: #e6a23c;
  box-shadow: 0 0 6px rgba(230, 162, 60, 0.4);
}

.feed-last-time {
  font-size: 12px;
  color: #9ca3af;
}

.feed-action-btn {
  width: 100%;
  max-width: 320px;
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
  color: white;
  border: none;
  border-radius: 14px;
  padding: 14px 28px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 4px 16px rgba(139, 173, 66, 0.3);
  position: relative;
  overflow: hidden;
}

.feed-action-btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.feed-action-btn:hover::before {
  left: 100%;
}

.feed-action-btn:hover {
  transform: translateY(-3px);
  box-shadow: 0 8px 24px rgba(139, 173, 66, 0.45);
}

.feed-action-btn:active {
  transform: translateY(-1px);
  box-shadow: 0 2px 8px rgba(139, 173, 66, 0.2);
  transition: all 0.1s ease;
}

.btn-content {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
}

.btn-icon-inner {
  font-size: 20px;
}

.btn-label {
  font-size: 16px;
}

.feed-action-btn.disabled,
.feed-action-btn:disabled {
  background: linear-gradient(135deg, #d1d5db 0%, #9ca3af 100%);
  cursor: not-allowed;
  box-shadow: none;
  transform: none;
}

.feed-action-btn.disabled::before,
.feed-action-btn:disabled::before {
  display: none;
}

/* 加载状态 */
.loading-section {
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  min-height: 60vh;
  gap: 16px;
}

.loading-spinner {
  width: 40px;
  height: 40px;
  border: 3px solid rgba(139, 173, 66, 0.2);
  border-top: 3px solid #8BAD42;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

.loading-text {
  font-size: 16px;
  color: #6b7280;
  font-weight: 500;
}

/* 空状态 */
.empty-section {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 60vh;
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.2s;
}

.empty-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(20px);
  border-radius: 20px;
  padding: 40px;
  text-align: center;
  box-shadow: 
    0 12px 40px rgba(139, 173, 66, 0.15),
    0 4px 16px rgba(139, 173, 66, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
  border: 1px solid rgba(139, 173, 66, 0.1);
}

.empty-icon {
  font-size: 64px;
  margin-bottom: 16px;
}

.empty-title {
  font-size: 20px;
  font-weight: 700;
  color: #166534;
  margin: 0 0 8px 0;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
}

.empty-text {
  font-size: 16px;
  color: #6b7280;
  font-weight: 500;
  margin: 0;
}

/* 响应式设计 */
@media (max-width: 768px) {
  .page-content {
    padding: 16px 12px;
  }
  
  .page-title {
    font-size: 20px;
    margin-bottom: 16px;
  }
  
  /* 已认领状态响应式 */
  .bird-header {
    padding: 0 12px;
  }
  
  .bird-name {
    font-size: 24px;
  }
  
  .status-badge {
    padding: 6px 12px;
    font-size: 12px;
  }
  
  .bird-hero-section {
    padding: 0 12px;
    gap: 20px;
  }
  
  .bird-image-container {
    min-height: 180px;
    border-radius: 16px;
  }
  
  .image-placeholder {
    padding: 40px 30px;
  }
  
  .image-placeholder .placeholder-icon {
    font-size: 64px;
  }
  
  .bird-status-info {
    gap: 24px;
    padding: 16px;
    border-radius: 16px;
  }
  
  .status-value {
    font-size: 28px;
  }
  
  .gauge-chart {
    width: 110px;
    height: 110px;
  }
  
  .gauge-label {
    font-size: 12px;
  }
  
  .bird-details-section {
    margin: 0 12px;
    padding: 20px;
    border-radius: 16px;
  }
  
  .section-title {
    font-size: 16px;
  }
  
  .action-section {
    padding: 0 12px 20px;
  }
  
  .release-btn {
    padding: 12px 28px;
    font-size: 15px;
    min-width: 180px;
  }
  
  .empty-card {
    padding: 32px 24px;
    margin: 0 16px;
  }
  
  /* 喂食平板响应式 */
  .feeds-section {
    margin: 0 12px;
    padding: 24px 18px;
  }

  .feed-card-visual {
    padding: 10px 22px;
    gap: 12px;
  }

  .feed-bird-icon {
    font-size: 30px;
  }

  .feed-icon {
    font-size: 28px;
  }

  .feed-title {
    font-size: 18px;
  }

  .feed-desc {
    font-size: 13px;
  }

  .feed-status {
    font-size: 13px;
  }

  .feed-action-btn {
    padding: 12px 22px;
    font-size: 15px;
    max-width: 280px;
  }
}

@media (max-width: 480px) {
  .adopt-btn {
    padding: 12px 24px;
    font-size: 15px;
  }
  
  /* 已认领状态响应式 */
  .bird-name {
    font-size: 20px;
  }
  
  .bird-image-container {
    min-height: 150px;
  }
  
  .image-placeholder {
    padding: 30px 20px;
  }
  
  .image-placeholder .placeholder-icon {
    font-size: 48px;
  }
  
  .bird-status-info {
    gap: 16px;
    padding: 12px;
  }
  
  .status-value {
    font-size: 24px;
  }
  
  .status-label {
    font-size: 12px;
  }
  
  .gauge-chart {
    width: 100px;
    height: 100px;
  }
  
  .gauge-label {
    font-size: 11px;
  }
  
  .bird-details-section {
    padding: 16px;
  }
  
  .detail-label {
    min-width: 80px;
    font-size: 13px;
  }
  
  .detail-value, .detail-text {
    font-size: 14px;
  }
  
  .release-btn {
    padding: 12px 24px;
    font-size: 14px;
    min-width: 160px;
  }
}

/* 环境数据环形进度条 */
.env-gauges-section {
  margin-top: 20px;
}

.c3-missing-alert {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 16px;
  background: linear-gradient(135deg, #fffbeb 0%, #fef3c7 100%);
  border: 1px solid rgba(245, 158, 11, 0.3);
  border-radius: 12px;
  margin-bottom: 16px;
  font-size: 13px;
  color: #92400e;
  line-height: 1.5;
}

.c3-missing-icon {
  font-size: 18px;
  flex-shrink: 0;
}

.gauges-title {
  font-size: 16px;
  font-weight: 600;
  color: #166534;
  margin: 0 0 14px 0;
  text-align: center;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
}

.gauges-row {
  display: flex;
  gap: 16px;
  justify-content: center;
}

.gauges-row-second,
.gauges-row-third {
  margin-top: 16px;
}

.gauge-item {
  flex: 1;
  max-width: 180px;
  display: flex;
  flex-direction: column;
  align-items: center;
  background: rgba(255, 255, 255, 0.45);
  backdrop-filter: blur(20px);
  -webkit-backdrop-filter: blur(20px);
  border: 1px solid rgba(139, 173, 66, 0.2);
  border-radius: 20px;
  padding: 20px 16px 16px;
  box-shadow: 0 8px 32px rgba(139, 173, 66, 0.08), inset 0 1px 0 rgba(255, 255, 255, 0.6);
  transition: all 0.3s ease;
}

.gauge-item:hover {
  transform: translateY(-2px);
  box-shadow: 0 12px 40px rgba(139, 173, 66, 0.12), inset 0 1px 0 rgba(255, 255, 255, 0.8);
}

.gauge-chart {
  width: 130px;
  height: 130px;
}

.gauge-label {
  font-size: 13px;
  font-weight: 500;
  color: #4a6741;
  margin-top: 2px;
  letter-spacing: 0.5px;
}

/* 监控控制栏 */
.monitoring-controls {
  background: rgba(255, 255, 255, 0.8);
  backdrop-filter: blur(12px);
  border-radius: 16px;
  padding: 16px;
  margin-bottom: 20px;
  box-shadow: 0 4px 16px rgba(139, 173, 66, 0.1);
  border: 1px solid rgba(139, 173, 66, 0.2);
}

.control-group {
  display: flex;
  gap: 12px;
  margin-bottom: 16px;
  justify-content: center;
}

.control-btn {
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
  color: white;
  border: none;
  border-radius: 12px;
  padding: 10px 16px;
  font-size: 14px;
  font-weight: 600;
  cursor: pointer;
  display: flex;
  align-items: center;
  gap: 6px;
  transition: all 0.3s ease;
  box-shadow: 0 4px 12px rgba(139, 173, 66, 0.3);
  flex: 1;
  max-width: 120px;
  justify-content: center;
}

.control-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(139, 173, 66, 0.4);
}

.control-btn.active {
  background: linear-gradient(135deg, #6A9A35 0%, #5a8a25 100%);
  box-shadow: 0 4px 12px rgba(139, 173, 66, 0.4);
}

.btn-icon {
  font-size: 16px;
}

.btn-text {
  font-size: 14px;
}

/* 视频设置区域 */
.video-settings {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.setting-item {
  display: flex;
  align-items: center;
  gap: 12px;
}

.setting-label {
  font-size: 14px;
  font-weight: 600;
  color: #166534;
  min-width: 60px;
  flex-shrink: 0;
}

.quality-slider {
  flex: 1;
  height: 6px;
  border-radius: 3px;
  background: #e5e7eb;
  outline: none;
  -webkit-appearance: none;
  appearance: none;
}

.quality-slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 16px;
  height: 16px;
  border-radius: 50%;
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
  cursor: pointer;
  box-shadow: 0 2px 6px rgba(139, 173, 66, 0.3);
}

.quality-slider::-moz-range-thumb {
  width: 16px;
  height: 16px;
  border-radius: 50%;
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
  cursor: pointer;
  border: none;
  box-shadow: 0 2px 6px rgba(139, 173, 66, 0.3);
}

.setting-value {
  font-size: 14px;
  font-weight: 600;
  color: #6A9A35;
  min-width: 30px;
  text-align: right;
}

.resolution-select {
  flex: 1;
  padding: 8px 12px;
  border: 1px solid rgba(139, 173, 66, 0.3);
  border-radius: 8px;
  background: white;
  font-size: 14px;
  color: #166534;
  cursor: pointer;
  transition: all 0.3s ease;
}

.resolution-select:hover {
  border-color: #8BAD42;
  box-shadow: 0 2px 8px rgba(139, 173, 66, 0.2);
}

.resolution-select:focus {
  outline: none;
  border-color: #6A9A35;
  box-shadow: 0 0 0 3px rgba(139, 173, 66, 0.1);
}
/* 移动端响应式 */
@media (max-width: 480px) {
  .bird-hero-section {
    padding: 0 16px;
  }
  
  .bird-image-container {
    min-height: 300px;
  }
  
  .video-stream {
    max-height: 300px;
  }
  
  .frame-info {
    font-size: 10px;
    padding: 4px 8px;
  }
  
  .disconnected-icon {
    font-size: 32px;
  }
  
  .disconnected-message {
    font-size: 14px;
  }
  
  .error-icon {
    font-size: 32px;
  }
  
  .error-message {
    font-size: 12px;
  }
  
  .retry-btn,
  .start-stream-btn {
    padding: 8px 16px;
    font-size: 12px;
  }
  
  /* 监控控制栏响应式 */
  .monitoring-controls {
    padding: 12px;
    margin-bottom: 16px;
  }
  
  .control-group {
    gap: 8px;
    margin-bottom: 12px;
  }
  
  .control-btn {
    padding: 8px 12px;
    font-size: 12px;
    max-width: 100px;
  }
  
  .btn-icon {
    font-size: 14px;
  }
  
  .btn-text {
    font-size: 12px;
  }
  
  .setting-item {
    gap: 8px;
  }
  
  .setting-label {
    font-size: 12px;
    min-width: 50px;
  }
  
  .setting-value {
    font-size: 12px;
  }
  
  .resolution-select {
    padding: 6px 10px;
    font-size: 12px;
  }
  
  /* 喂食移动端响应式 */
  .feeds-section {
    margin: 0 12px;
    padding: 20px 16px;
  }

  .feed-card-visual {
    padding: 8px 18px;
    gap: 10px;
  }

  .feed-bird-icon {
    font-size: 26px;
  }

  .feed-divider {
    height: 24px;
  }

  .feed-icon {
    font-size: 24px;
  }

  .feed-title {
    font-size: 17px;
  }

  .feed-desc {
    font-size: 12px;
  }

  .feed-status {
    font-size: 12px;
  }

  .feed-last-time {
    font-size: 10px;
  }

  .feed-action-btn {
    padding: 10px 20px;
    font-size: 14px;
    max-width: 260px;
    border-radius: 12px;
  }

  .btn-icon-inner {
    font-size: 17px;
  }

  .btn-label {
    font-size: 14px;
  }
}
</style>
