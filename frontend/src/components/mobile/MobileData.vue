<template>
  <div class="mobile-data">
    <!-- 实时数据 -->
    <div v-if="activeTab === 'realtime'" class="realtime-container">
      <!-- 设备选择 -->
      <div class="device-selector">
        <el-select v-model="selectedDeviceId" placeholder="选择设备" @change="handleDeviceChange" style="width: 100%;">
          <el-option 
            v-for="device in devices" 
            :key="device.id" 
            :label="device.name" 
            :value="device.id" 
          />
        </el-select>
      </div>
      
      <!-- 设备数据卡片 -->
      <div v-if="selectedDevice" class="device-data-card">
        <div class="data-header">
          <h3>{{ selectedDevice.name }}</h3>
          <el-tag :type="selectedDevice.isOnline ? 'success' : 'danger'" size="small">
            {{ selectedDevice.isOnline ? '在线' : '离线' }}
          </el-tag>
        </div>
        <div class="data-grid">
          <div class="data-item">
            <div class="data-label">温度</div>
            <div class="data-value">{{ selectedDevice.temperature }}°C</div>
          </div>
          <div class="data-item">
            <div class="data-label">湿度</div>
            <div class="data-value">{{ selectedDevice.humidity }}%</div>
          </div>
          <div class="data-item full">
            <div class="data-label">创建时间</div>
            <div class="data-value small">{{ selectedDevice.createTime }}</div>
          </div>
        </div>
      </div>
      
      <!-- 实时监控 -->
      <div v-if="selectedDevice" class="video-section">
        <div class="video-header">
          <div class="video-header-top">
            <h3>实时监控</h3>
            <div class="video-controls">
              <el-button 
                type="warning"
                size="small"
                @click="reconnectWebSocket"
                :disabled="!selectedDeviceId || isWebSocketConnected"
              >
                <el-icon><Refresh /></el-icon>
                重连
              </el-button>
              <el-button 
                :type="flipHorizontal ? 'primary' : 'default'"
                size="small"
                @click="toggleHorizontalFlip"
              >
                <el-icon><Switch /></el-icon>
                左右
              </el-button>
              <el-button 
                :type="flipVertical ? 'primary' : 'default'"
                size="small"
                @click="toggleVerticalFlip"
              >
                <el-icon><Sort /></el-icon>
                上下
              </el-button>
            </div>
          </div>
          
          <!-- 视频质量和尺寸控制 -->
          <div class="video-settings">
            <div class="setting-item">
              <span class="setting-label">画质</span>
              <el-slider 
                v-model="imageQuality" 
                :min="0" 
                :max="63" 
                :step="1"
                :show-tooltip="false"
                size="small"
                style="flex: 1; margin-left: 8px;"
              />
            </div>
            <div class="setting-item">
              <span class="setting-label">视频尺寸</span>
              <el-select v-model="frameSize" style="flex: 1; margin-left: 8px;" size="small">
                <el-option label="128x128" value="FRAMESIZE_128X128" />
                <el-option label="240x240" value="FRAMESIZE_240X240" />
                <el-option label="320x320" value="FRAMESIZE_320X320" />
                <el-option label="VGA" value="FRAMESIZE_VGA" />
                <el-option label="SVGA" value="FRAMESIZE_SVGA" />
                <el-option label="HD" value="FRAMESIZE_HD" />
                <el-option label="FHD" value="FRAMESIZE_FHD" />
              </el-select>
            </div>
          </div>
        </div>
        <div class="video-card">
          <div class="video-container" v-if="currentFrameImage">
            <img 
              :src="currentFrameImage" 
              class="video-stream"
              :class="{ 'flip-horizontal': flipHorizontal, 'flip-vertical': flipVertical, 'disconnected': isStreamDisconnected }"
              alt="实时监控画面"
            />
            <div class="frame-info">
                <span>更新时间: {{ lastFrameTime }}</span>
                <span class="fps-info">帧率: {{ currentFps }} FPS</span>
              </div>
            <div class="stream-disconnected-overlay" v-if="isStreamDisconnected">
              <div class="disconnected-content">
                <el-icon class="disconnected-icon" :size="48"><Warning /></el-icon>
                <div class="disconnected-message">视频流已断联</div>
              </div>
            </div>
          </div>
          <div class="video-error" v-else-if="connectionError">
            <div class="error-content">
              <el-icon class="error-icon" :size="48"><Warning /></el-icon>
              <div class="error-message">{{ connectionError }}</div>
            </div>
          </div>
          <div class="video-placeholder" v-else>
            <el-empty description="未开始实时监控" />
          </div>
        </div>
      </div>
    </div>
    
    <!-- 分析 -->
    <div v-else-if="activeTab === 'analysis'" class="analysis-container">
      <div class="device-selector">
        <el-select v-model="selectedDeviceId" placeholder="选择设备" @change="handleDeviceChange" style="width: 100%;">
          <el-option 
            v-for="device in devices" 
            :key="device.id" 
            :label="device.name" 
            :value="device.id" 
          />
        </el-select>
      </div>
      
      <!-- 图表 -->
      <div class="chart-section">
        <div class="chart-card">
          <h3>所有设备平均值 <span class="time-range">(最近6小时)</span></h3>
          <div ref="averageChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>温度数据 <span class="time-range">(最近6小时)</span></h3>
          <div ref="temperatureChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>湿度数据 <span class="time-range">(最近6小时)</span></h3>
          <div ref="humidityChartRef" class="chart"></div>
        </div>
       
      </div>
    </div>
    
    <!-- 历史数据 -->
    <div v-else-if="activeTab === 'history'" class="history-container">
      <div class="device-selector">
        <el-select v-model="historyDeviceId" placeholder="选择设备" @change="handleHistoryDeviceChange" style="width: 100%;">
          <el-option 
            v-for="device in devices" 
            :key="device.id" 
            :label="device.name" 
            :value="device.id" 
          />
        </el-select>
      </div>
      
      <div v-if="selectedHistoryDevice" class="history-content">
        <!-- 设备信息卡片 -->
        <div class="device-info-card">
          <div class="info-item">
            <span class="info-label">设备名称</span>
            <span class="info-value">{{ selectedHistoryDevice.name }}</span>
          </div>
          <div class="info-item">
            <span class="info-label">设备状态</span>
            <el-tag :type="selectedHistoryDevice.isOnline ? 'success' : 'danger'" size="small">
              {{ selectedHistoryDevice.isOnline ? '在线' : '离线' }}
            </el-tag>
          </div>
        </div>
        
        <!-- 历史数据列表 -->
        <div class="history-list">
          <div v-for="(item, index) in paginatedHistoryData" :key="index" class="history-item">
            <div class="history-row">
              <span class="device-name">{{ item.deviceName }}</span>
              <span class="device-id">ID: {{ item.deviceId }}</span>
              <span class="history-time">{{ item.timestamp }}</span>
            </div>
            <div class="history-row data-row">
              <span class="data-item">温度 {{ item.temperature }}°C</span>
              <span class="data-item">湿度 {{ item.humidity }}%</span>
            </div>
          </div>
        </div>
        
        <!-- 分页组件 -->
        <div class="pagination-container">
          <el-pagination
            v-model:current-page="currentPage"
            v-model:page-size="pageSize"
            :page-sizes="[5, 10, 20]"
            :total="historyDataList.length"
            layout="prev, pager, next"
            background
            :pager-count="3"
            @size-change="handleSizeChange"
            @current-change="handleCurrentChange"
          />
        </div>
      </div>
      
      <div v-else class="history-placeholder">
        <el-empty description="请选择设备查看历史数据" />
      </div>
    </div>

    <!-- 备案信息 -->
    <MobileBeian />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted, nextTick } from 'vue';
import { useDeviceStore } from '../../stores/deviceStore';
import { ElMessage, ElMessageBox } from 'element-plus';
import { Warning, Switch, Sort, Refresh } from '@element-plus/icons-vue';
import * as echarts from 'echarts';
import MobileBeian from './MobileBeian.vue';

const getToken = () => {
  return localStorage.getItem('accessToken');
};

const { getDevices, getDeviceHistoryData, getDeviceAverageData, fetchDevices, getOrUpdateDeviceHistoryData, updateDevice, fetchRealtimeSensorData } = useDeviceStore();
const devices = getDevices();

const props = defineProps<{
  activeTab?: string;
}>();

const activeTab = computed(() => props.activeTab || 'realtime');

const selectedDeviceId = ref<number | null>(null);

const selectedDevice = computed(() => {
  if (!selectedDeviceId.value) return null;
  // 每次计算时重新获取最新的设备列表
  const latestDevices = getDevices();
  return latestDevices.find(device => device.id === selectedDeviceId.value) || null;
});

const isWebSocketConnected = computed(() => {
  return ws !== null && ws.readyState === WebSocket.OPEN;
});

// 实时温湿度数据定时器
let realtimeDataTimer: number | null = null;

// 获取实时温湿度数据
const fetchRealtimeData = async () => {
  if (!selectedDeviceId.value) return;
  
  const sensorData = await fetchRealtimeSensorData(selectedDeviceId.value);
  if (sensorData) {
    updateDevice(selectedDeviceId.value, { 
      temperature: sensorData.temperature, 
      humidity: sensorData.humidity 
    });
    console.log(`[实时数据] 设备 ${selectedDeviceId.value} 温度: ${sensorData.temperature}°C, 湿度: ${sensorData.humidity}%`);
  }
};

// 启动实时数据定时器
const startRealtimeDataTimer = () => {
  // 先清除旧的定时器
  stopRealtimeDataTimer();
  
  // 立即获取一次数据
  fetchRealtimeData();
  
  // 每分钟获取一次数据
  realtimeDataTimer = window.setInterval(() => {
    fetchRealtimeData();
  }, 60000); // 60秒
};

// 停止实时数据定时器
const stopRealtimeDataTimer = () => {
  if (realtimeDataTimer) {
    clearInterval(realtimeDataTimer);
    realtimeDataTimer = null;
  }
};

const handleDeviceChange = async () => {
  if (selectedDevice.value) {
    ElMessage.success(`已切换到设备: ${selectedDevice.value.name}`);
    
    // 如果在实时界面，立即获取实时温湿度数据并启动定时器
    if (activeTab.value === 'realtime') {
      await fetchRealtimeData();
      startRealtimeDataTimer();
    }
    
    // 如果在分析界面，需要先获取历史数据再初始化图表
    if (activeTab.value === 'analysis' && selectedDeviceId.value) {
      await getOrUpdateDeviceHistoryData(selectedDeviceId.value);
    }
    nextTick(() => {
      initCharts();
    });
  }
};

// 历史数据相关
const historyDeviceId = ref<number | null>(null);

const selectedHistoryDevice = computed(() => {
  if (!historyDeviceId.value) return null;
  return devices.find(device => device.id === historyDeviceId.value) || null;
});

const historyDataList = computed(() => {
  if (!historyDeviceId.value) return [];
  const historyData = getDeviceHistoryData(historyDeviceId.value);
  const device = selectedHistoryDevice.value;
  const deviceName = device?.name || '';
  return historyData
    .map(data => ({
      ...data,
      deviceName: deviceName,
      deviceId: historyDeviceId.value,
      timestamp: data.timestamp
    }))
    .sort((a, b) => new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime());
});

// 分页相关
const currentPage = ref(1);
const pageSize = ref(10);

// 分页后的历史数据
const paginatedHistoryData = computed(() => {
  const start = (currentPage.value - 1) * pageSize.value;
  const end = start + pageSize.value;
  return historyDataList.value.slice(start, end);
});

// 处理每页条数变化
const handleSizeChange = (val: number) => {
  pageSize.value = val;
  currentPage.value = 1;
};

// 处理页码变化
const handleCurrentChange = (val: number) => {
  currentPage.value = val;
};

const handleHistoryDeviceChange = async () => {
  if (selectedHistoryDevice.value && historyDeviceId.value) {
    ElMessage.success(`已切换到设备: ${selectedHistoryDevice.value.name}`);
    // 获取该设备的历史数据
    console.log('Fetching history data for device:', historyDeviceId.value);
    await getOrUpdateDeviceHistoryData(historyDeviceId.value);
    console.log('History data fetched');
    // 重置页码
    currentPage.value = 1;
  }
};



// 图表容器引用
const temperatureChartRef = ref<HTMLElement | null>(null);
const humidityChartRef = ref<HTMLElement | null>(null);
const averageChartRef = ref<HTMLElement | null>(null);

// 图表实例
let temperatureChart: echarts.ECharts | null = null;
let humidityChart: echarts.ECharts | null = null;
let averageChart: echarts.ECharts | null = null;

// 实时监控图片帧
const currentFrameImage = ref<string>('');
const lastFrameTime = ref<string>('');
const connectionError = ref<string>('');
const isStreamDisconnected = ref<boolean>(false);
const currentFps = ref<number>(0);
const imageQuality = ref(32); // 默认画质为32
const frameSize = ref('FRAMESIZE_VGA'); // 默认视频尺寸
let lastFrameTimestamp = 0;
let previousFrameTimestamp = 0;
let streamCheckInterval: number | null = null;
let ws: WebSocket | null = null;
let currentImageUrl: string = '';

// 视频翻转控制
const flipHorizontal = ref(false);
const flipVertical = ref(false);

const toggleHorizontalFlip = () => {
  flipHorizontal.value = !flipHorizontal.value;
};

const toggleVerticalFlip = () => {
  flipVertical.value = !flipVertical.value;
};

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
  
  console.log('向ws连接发送画质设置:', message);
  
  try {
    ws.send(message);
  } catch (error) {
    console.error('发送画质设置失败:', error);
  }
};

// 发送视频尺寸设置到WebSocket连接
const sendFrameSize = async (size: string) => {
  // 使用当前已建立的WebSocket连接发送视频尺寸设置
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    console.warn('WebSocket连接未建立，无法发送视频尺寸设置');
    return;
  }
  
  const message = JSON.stringify({
    code: 1,
    item: 'camera',
    key: 'frame_size',
    values: size
  });
  
  console.log('向ws连接发送视频尺寸设置:', message);
  
  try {
    ws.send(message);
  } catch (error) {
    console.error('发送视频尺寸设置失败:', error);
  }
};

const handleFrameData = (data: any) => {
  try {
    let blob: Blob;
    if (typeof data === 'string') {
      blob = base64ToBlob(data);
    } else if (data instanceof Blob) {
      blob = data;
    } else if (data instanceof ArrayBuffer) {
      blob = new Blob([data], { type: 'image/jpeg' });
    } else if (data instanceof Uint8Array) {
      blob = new Blob([data as any], { type: 'image/jpeg' });
    } else {
      throw new Error('不支持的数据类型');
    }
    
    if (currentImageUrl) {
      URL.revokeObjectURL(currentImageUrl);
    }
    
    currentImageUrl = URL.createObjectURL(blob);
    currentFrameImage.value = currentImageUrl;
    lastFrameTime.value = new Date().toLocaleTimeString('zh-CN');
    
    // 计算帧率
    const now = Date.now();
    if (previousFrameTimestamp > 0) {
      const frameInterval = now - previousFrameTimestamp;
      currentFps.value = Math.round(1000 / frameInterval);
    }
    previousFrameTimestamp = now;
    
    lastFrameTimestamp = now;
    isStreamDisconnected.value = false;
  } catch (error) {
    console.error('处理图片数据失败:', error);
  }
};

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

const startRealtimeMonitoring = async () => {
  stopRealtimeMonitoring();
  
  if (!selectedDeviceId.value) {
    console.log('没有选择设备，无法启动实时监控');
    return;
  }
  
  try {
    let token = await refreshToken();
    if (!token) {
      connectionError.value = '未登录，请先登录';
      console.log('没有token，无法建立WebSocket连接');
      return;
    }
    
    // 建立WebSocket连接（新的连接模式：一个WebSocket对应一个设备）
    const wsUrl = `/api/stream/viewer/ws/${selectedDeviceId.value}?token=${token}`;
    connectionError.value = '正在开启ws连接';
    ws = new WebSocket(wsUrl);
    
    ws.onopen = () => {
      console.log('WebSocket连接已建立');
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
      
      // 新的连接模式不需要单独发送请求开启设备推流
    };
    
    ws.onmessage = (event) => {
      try {
        // 检查是否是设备断开的文本消息
        if (typeof event.data === 'string' && event.data.includes('设备已断开')) {
          console.log('收到设备断开消息:', event.data);
          connectionError.value = '设备已断开';
          return;
        }
        
        try {
          const data = JSON.parse(event.data);
          // 检查是否是画质设置响应
          if (data.code === 1 && data.key === 'jpeg_quality') {
            console.log('画质设置成功:', data);
          } else if (data.code === 1 && data.key === 'frame_size') {
            console.log('视频尺寸设置成功:', data);
          } else if (data.code === 1) {
            console.log('操作成功:', data.msg);
          } else if (data.code === 400) {
            console.error('订阅失败:', data.msg);
            connectionError.value = '订阅失败';
          } else {
            console.error('操作失败:', data.msg);
          }
        } catch (jsonError) {
          handleFrameData(event.data);
          console.log('接收到图片数据');
        }
      } catch (error) {
        console.error('解析WebSocket消息失败:', error);
      }
    };
    
    ws.onerror = (error) => {
      console.error('WebSocket连接错误:', error);
      connectionError.value = 'WSS连接错误';
    };
    
    ws.onclose = (event) => {
      console.log('WebSocket连接已关闭:', event);
      if (!event.wasClean) {
        connectionError.value = 'WSS连接已关闭';
      }
    };
    
  } catch (error) {
    console.error('建立WebSocket连接失败:', error);
    connectionError.value = '建立WSS连接失败';
  }
};

const reconnectWebSocket = () => {
  console.log('手动重连WebSocket');
  if (ws && ws.readyState === WebSocket.OPEN) {
    ElMessageBox.confirm(
      '当前WebSocket连接正常，确定要重新连接吗？',
      '确认重连',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )
      .then(() => {
        console.log('用户确认重连，关闭现有连接并重新建立');
        startRealtimeMonitoring();
      })
      .catch(() => {
        console.log('用户取消重连');
      });
  } else {
    console.log('连接异常或不存在，直接重连');
    startRealtimeMonitoring();
  }
};

const refreshToken = async () => {
  try {
    const token = getToken();
    if (!token) {
      console.log('没有token，无法刷新');
      return null;
    }
    
    console.log('开始刷新token');
    const response = await fetch('/api/refresh', {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${token}`
      }
    });
    
    console.log('刷新token响应状态:', response.status);
    if (response.ok) {
      const data = await response.json();
      console.log('刷新token响应数据:', data);
      if (data.access_token) {
        localStorage.setItem('accessToken', data.access_token);
        console.log('Token刷新成功');
        return data.access_token;
      } else {
        console.log('Token刷新失败，响应数据格式不正确');
      }
    } else {
      console.log('Token刷新失败，HTTP状态码:', response.status);
    }
    
    console.log('使用原token');
    return token;
  } catch (error) {
    console.error('刷新token失败:', error);
    return getToken();
  }
};

const startDeviceStreaming = async () => {
  if (!selectedDeviceId.value) return;
  
  try {
    const token = getToken();
    if (!token) {
      connectionError.value = '未登录，请先登录';
      return;
    }
    
    const response = await fetch(`/api/stream/viewer/following/${selectedDeviceId.value}`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${token}`
      }
    });
    
    const data = await response.json();
    
    await new Promise(resolve => setTimeout(resolve, 4000));
    
    const isSuccess = response.ok && (data.code === 1 || 
      (data.msg && (data.msg.includes('OK') || data.msg.includes('success') || data.msg.includes('成功'))));
    
    if (isSuccess) {
      console.log('设备推流开启成功:', data.msg);
      connectionError.value = '';
    } else if (data.msg && data.msg.toLowerCase().includes('device has subscribed')) {
      console.log('设备已经被订阅');
      connectionError.value = '';
    } else if (data.msg && data.msg.toLowerCase().includes('viewer has not registerd')) {
      console.log('Viewer未注册，但WebSocket可能已经成功连接');
      connectionError.value = '';
    } else if (response.status === 401) {
      console.log('认证失败，但WebSocket可能已经成功连接');
      connectionError.value = '';
    } else {
      console.error('设备推流开启失败:', data.msg || '未知错误');
      connectionError.value = '设备推流开启失败';
    }
  } catch (error) {
    console.error('开启设备推流失败:', error);
    await new Promise(resolve => setTimeout(resolve, 3000));
    connectionError.value = '开启设备推流失败';
  }
};

const stopRealtimeMonitoring = async () => {
  if (streamCheckInterval) {
    clearInterval(streamCheckInterval);
    streamCheckInterval = null;
  }
  
  if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) {
    // 新的连接模式：WebSocket断开时会自动清理订阅关系
    ws.close();
    ws = null;
  }
  
  if (currentImageUrl) {
    URL.revokeObjectURL(currentImageUrl);
    currentImageUrl = '';
  }
  
  currentFrameImage.value = '';
  connectionError.value = '';
  isStreamDisconnected.value = false;
};

// 格式化时间标签，只显示小时和分钟
const formatTimeLabel = (timestamp: string) => {
  // 如果已经是 HH:00 格式，直接返回
  if (/^\d{2}:\d{2}$/.test(timestamp)) {
    return timestamp;
  }
  const date = new Date(timestamp);
  const hours = date.getHours().toString().padStart(2, '0');
  const minutes = date.getMinutes().toString().padStart(2, '0');
  return `${hours}:${minutes}`;
};

// 采样函数：从数组中均匀采样指定数量的点
const sampleData = (data: any[], count: number) => {
  if (data.length <= count) return data;
  const result = [];
  const step = (data.length - 1) / (count - 1);
  for (let i = 0; i < count; i++) {
    const index = Math.round(i * step);
    result.push(data[index]);
  }
  return result;
};

// 初始化图表
const initCharts = () => {
  // 销毁旧图表实例
  temperatureChart?.dispose();
  humidityChart?.dispose();
  averageChart?.dispose();
  
  // 初始化平均图表
  if (averageChartRef.value) {
    averageChart = echarts.init(averageChartRef.value);
    const avgData = getDeviceAverageData(6); // 6小时范围
    
    // 采样12个点显示（保持6小时范围）
    const sampledTimes = sampleData(avgData.times, 12);
    const sampledTempValues = sampleData(avgData.temperatureValues, 12);
    const sampledHumidityValues = sampleData(avgData.humidityValues, 12);
    
    averageChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      legend: {
        data: ['平均温度', '平均湿度'],
        top: 5,
        textStyle: { fontSize: 11 },
        itemGap: 15
      },
      grid: {
        left: '12%',
        right: '8%',
        bottom: '15%',
        top: '18%',
        containLabel: false
      },
      xAxis: {
        type: 'category',
        data: sampledTimes,
        axisLabel: { 
          fontSize: 10, 
          interval: 1
        }
      },
      yAxis: {
        type: 'value',
        name: '数值',
        nameTextStyle: {
          fontSize: 10
        },
        axisLabel: { fontSize: 10 }
      },
      series: [
        {
          name: '平均温度',
          data: sampledTempValues,
          type: 'line',
          smooth: true,
          itemStyle: { color: '#ff7875' },
          areaStyle: {
            color: {
              type: 'linear',
              x: 0, y: 0, x2: 0, y2: 1,
              colorStops: [{
                offset: 0, color: 'rgba(255, 120, 117, 0.3)'
              }, {
                offset: 1, color: 'rgba(255, 120, 117, 0.05)'
              }]
            }
          }
        },
        {
          name: '平均湿度',
          data: sampledHumidityValues,
          type: 'line',
          smooth: true,
          itemStyle: { color: '#69c0ff' },
          areaStyle: {
            color: {
              type: 'linear',
              x: 0, y: 0, x2: 0, y2: 1,
              colorStops: [{
                offset: 0, color: 'rgba(105, 192, 255, 0.3)'
              }, {
                offset: 1, color: 'rgba(105, 192, 255, 0.05)'
              }]
            }
          }
        }
      ]
    });
  }
  
  // 初始化温度图表
  if (temperatureChartRef.value && selectedDeviceId.value) {
    temperatureChart = echarts.init(temperatureChartRef.value);
    const allHistoryData = getDeviceHistoryData(selectedDeviceId.value);
    
    // 过滤最近6小时的数据
    const sixHoursAgo = new Date();
    sixHoursAgo.setHours(sixHoursAgo.getHours() - 6);
    const historyData = allHistoryData.filter(d => new Date(d.timestamp) >= sixHoursAgo);
    
    // 采样12个点显示（保持6小时范围）
    const sampledHistoryData = sampleData(historyData, 12);
    const tempValues = sampledHistoryData.map(d => d.temperature);
    const tempMin = Math.min(...tempValues);
    const tempMax = Math.max(...tempValues);
    // 调整纵轴范围，使数据更直观
    const tempRange = tempMax - tempMin;
    let tempYMin = tempMin;
    let tempYMax = tempMax;
    if (tempRange < 5) {
      tempYMin = Math.floor(tempMin - 1);
      tempYMax = Math.ceil(tempMax + 1);
    }
    
    temperatureChart.setOption({
      tooltip: { trigger: 'axis' },
      grid: {
        left: '15%',
        right: '8%',
        bottom: '15%',
        top: '12%',
        containLabel: false
      },
      xAxis: {
        type: 'category',
        data: sampledHistoryData.map(d => d.timestamp).map(formatTimeLabel),
        axisLabel: { 
          fontSize: 10, 
          interval: 1
        }
      },
      yAxis: {
        type: 'value',
        name: '温度 (℃)',
        min: tempYMin,
        max: tempYMax,
        nameTextStyle: {
          fontSize: 10
        },
        axisLabel: { fontSize: 10 }
      },
      series: [{
        data: tempValues,
        type: 'line',
        smooth: true,
        itemStyle: { color: '#ff7875' },
        areaStyle: {
          color: {
            type: 'linear',
            x: 0, y: 0, x2: 0, y2: 1,
            colorStops: [{
              offset: 0, color: 'rgba(255, 120, 117, 0.3)'
            }, {
              offset: 1, color: 'rgba(255, 120, 117, 0.05)'
            }]
          }
        }
      }]
    });
  }
  
  // 初始化湿度图表
  if (humidityChartRef.value && selectedDeviceId.value) {
    humidityChart = echarts.init(humidityChartRef.value);
    const allHistoryData = getDeviceHistoryData(selectedDeviceId.value);
    
    // 过滤最近6小时的数据
    const sixHoursAgo = new Date();
    sixHoursAgo.setHours(sixHoursAgo.getHours() - 6);
    const historyData = allHistoryData.filter(d => new Date(d.timestamp) >= sixHoursAgo);
    
    // 采样12个点显示（保持6小时范围）
    const sampledHistoryData = sampleData(historyData, 12);
    const humidityValues = sampledHistoryData.map(d => d.humidity);
    const humidityMin = Math.min(...humidityValues);
    const humidityMax = Math.max(...humidityValues);
    // 调整纵轴范围，使数据更直观
    const humidityRange = humidityMax - humidityMin;
    let humidityYMin = humidityMin;
    let humidityYMax = humidityMax;
    if (humidityRange < 5) {
      humidityYMin = Math.floor(humidityMin - 1);
      humidityYMax = Math.ceil(humidityMax + 1);
    }
    
    humidityChart.setOption({
      tooltip: { trigger: 'axis' },
      grid: {
        left: '15%',
        right: '8%',
        bottom: '15%',
        top: '12%',
        containLabel: false
      },
      xAxis: {
        type: 'category',
        data: sampledHistoryData.map(d => d.timestamp).map(formatTimeLabel),
        axisLabel: { 
          fontSize: 10, 
          interval: 1
        }
      },
      yAxis: {
        type: 'value',
        name: '湿度 (%)',
        min: humidityYMin,
        max: humidityYMax,
        nameTextStyle: {
          fontSize: 10
        },
        axisLabel: { fontSize: 10 }
      },
      series: [{
        data: humidityValues,
        type: 'line',
        smooth: true,
        itemStyle: { color: '#69c0ff' },
        areaStyle: {
          color: {
            type: 'linear',
            x: 0, y: 0, x2: 0, y2: 1,
            colorStops: [{
              offset: 0, color: 'rgba(105, 192, 255, 0.3)'
            }, {
              offset: 1, color: 'rgba(105, 192, 255, 0.05)'
            }]
          }
        }
      }]
    });
  }
  

};

// 窗口大小变化时调整图表
const handleResize = () => {
  temperatureChart?.resize();
  humidityChart?.resize();
  averageChart?.resize();
};

// 监听activeTab变化
watch(() => props.activeTab, async (newTab) => {
  if (newTab === 'analysis') {
    // 停止实时数据定时器
    stopRealtimeDataTimer();
    
    // 分析界面需要选择第一个设备并获取所有设备的历史数据
    if (devices.length > 0 && !selectedDeviceId.value) {
      selectedDeviceId.value = devices[0]?.id || null;
    }
    // 分析界面需要获取所有设备的历史数据来显示图表
    await getOrUpdateDeviceHistoryData(); // 不传参数，获取所有设备数据
    nextTick(() => {
      initCharts();
    });
  } else if (newTab === 'realtime') {
    if (devices.length > 0 && !selectedDeviceId.value) {
      selectedDeviceId.value = devices[0]?.id || null;
    }
    // 立即获取实时温湿度数据并启动定时器
    if (selectedDeviceId.value) {
      await fetchRealtimeData();
      startRealtimeDataTimer();
    }
  } else if (newTab === 'history') {
    // 停止实时数据定时器
    stopRealtimeDataTimer();
    
    if (devices.length > 0 && !historyDeviceId.value) {
      historyDeviceId.value = devices[0]?.id || null;
    }
    // 获取历史数据
    if (historyDeviceId.value) {
      getOrUpdateDeviceHistoryData(historyDeviceId.value);
    }
  }
});

// 监听selectedDeviceId变化，自动开启实时监控
watch(() => selectedDeviceId.value, (newDeviceId, oldDeviceId) => {
  console.log('Selected device ID changed:', newDeviceId, 'Old device ID:', oldDeviceId);
  console.log('Current active tab:', activeTab.value);
  
  nextTick(() => {
    console.log('Active tab after nextTick:', activeTab.value);
    if (newDeviceId && activeTab.value === 'realtime') {
      console.log('Auto-starting realtime monitoring for device:', newDeviceId);
      startRealtimeMonitoring();
    }
  });
});

// 监听滑块变化，将值反转后发送给后端（1-63 -> 63-1）
watch(imageQuality, (newQuality) => {
  const invertedQuality = 64 - newQuality;
  sendStreamQuality(invertedQuality);
});

// 监听视频尺寸变化
watch(frameSize, (newSize) => {
  sendFrameSize(newSize);
});

onMounted(async () => {
  await fetchDevices();
  window.addEventListener('resize', handleResize);
  
  // 默认选择第一个设备
  if (devices.length > 0) {
    if (activeTab.value === 'realtime' && !selectedDeviceId.value) {
      selectedDeviceId.value = devices[0]?.id || null;
      // 立即获取实时温湿度数据并启动定时器
      if (selectedDeviceId.value) {
        await fetchRealtimeData();
        startRealtimeDataTimer();
      }
    } else if (activeTab.value === 'history' && !historyDeviceId.value) {
      historyDeviceId.value = devices[0]?.id || null;
      // 获取历史数据
      if (historyDeviceId.value) {
        await getOrUpdateDeviceHistoryData(historyDeviceId.value);
      }
    } else if (activeTab.value === 'analysis') {
      // 分析界面需要选择第一个设备并获取所有设备的历史数据
      if (!selectedDeviceId.value) {
        selectedDeviceId.value = devices[0]?.id || null;
      }
      await getOrUpdateDeviceHistoryData(); // 不传参数，获取所有设备数据
    }
  }
  
  if (activeTab.value === 'analysis') {
    nextTick(() => {
      initCharts();
    });
  }
});

onUnmounted(() => {
  stopRealtimeMonitoring();
  stopRealtimeDataTimer();
  window.removeEventListener('resize', handleResize);
  temperatureChart?.dispose();
  humidityChart?.dispose();
  averageChart?.dispose();
});
</script>

<style scoped>
.mobile-data {
  min-height: calc(100vh - 120px);
  background: #f5f7fa;
  display: flex;
  flex-direction: column;
}

/* 设备选择 */
.device-selector {
  margin-bottom: 16px;
}

/* 实时数据 */
.realtime-container {
  padding: 0;
}

/* 设备数据卡片 */
.device-data-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  margin-bottom: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.data-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.data-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.data-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.data-item {
  padding: 12px;
  background: #f5f7fa;
  border-radius: 8px;
}

.data-item.full {
  grid-column: 1 / -1;
}

.data-label {
  font-size: 12px;
  color: #909399;
  margin-bottom: 4px;
}

.data-value {
  font-size: 18px;
  font-weight: 600;
  color: #303133;
}

.data-value.small {
  font-size: 14px;
}

/* 视频监控 */
.video-section {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.video-header {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 12px;
  margin-bottom: 12px;
  width: 100%;
}

.video-header-top {
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
}

.video-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.video-controls {
  display: flex;
  gap: 8px;
}

.video-settings {
  width: 100%;
  display: flex;
  flex-direction: column;
  gap: 12px;
  padding: 12px;
  background: #f5f7fa;
  border-radius: 8px;
}

.setting-item {
  display: flex;
  align-items: center;
  gap: 8px;
}

.setting-label {
  font-size: 14px;
  color: #606266;
  min-width: 60px;
}

.setting-value {
  font-size: 14px;
  color: #409EFF;
  font-weight: 600;
  min-width: 30px;
  text-align: right;
}

.video-card {
  position: relative;
  width: 100%;
  background: #000;
  border-radius: 8px;
  overflow: hidden;
}

.video-container {
  position: relative;
  width: 100%;
  padding-top: 75%; /* 4:3 比例 */
}

.video-stream {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.video-stream.flip-horizontal {
  transform: scaleX(-1);
}

.video-stream.flip-vertical {
  transform: scaleY(-1);
}

.video-stream.disconnected {
  opacity: 0.5;
}

.frame-info {
  position: absolute;
  bottom: 8px;
  left: 8px;
  right: 8px;
  background: rgba(0, 0, 0, 0.6);
  color: white;
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 12px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.fps-info {
  font-weight: bold;
  color: #409EFF;
}

.stream-disconnected-overlay {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(0, 0, 0, 0.7);
}

.disconnected-content {
  text-align: center;
  color: white;
}

.disconnected-icon {
  color: #f56c6c;
}

.disconnected-message {
  margin-top: 8px;
  font-size: 14px;
}

.video-error {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
  background: #f5f7fa;
}

.error-content {
  text-align: center;
}

.error-icon {
  color: #f56c6c;
}

.error-message {
  margin-top: 8px;
  color: #606266;
}

.video-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
}

/* 分析 */
.analysis-container {
  padding: 0;
}

.chart-section {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.chart-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.chart-card h3 {
  margin: 0 0 12px 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.chart-card h3 .time-range {
  font-size: 12px;
  font-weight: 400;
  color: #909399;
  margin-left: 8px;
}

.chart {
  width: 100%;
  height: 200px;
}

/* 历史数据 */
.history-container {
  padding: 0;
}

.history-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.device-info-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.info-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.info-label {
  font-size: 12px;
  color: #909399;
}

.info-value {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
}

.history-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.history-item {
  background: white;
  border-radius: 8px;
  padding: 10px 12px;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.06);
}

.history-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 6px;
}

.history-row .device-name {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
  flex-shrink: 0;
}

.history-row .device-id {
  font-size: 12px;
  color: #909399;
  flex-shrink: 0;
}

.history-row .history-time {
  font-size: 12px;
  color: #909399;
  margin-left: auto;
}

.history-row.data-row {
  margin-top: 6px;
  padding-top: 6px;
  border-top: 1px solid #f0f0f0;
}

.history-row .data-item {
  font-size: 14px;
  color: #303133;
}

.history-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.pagination-container {
  margin-top: 16px;
  display: flex;
  justify-content: center;
  padding: 0 4px;
  overflow-x: auto;
}

.pagination-container :deep(.el-pagination) {
  --el-pagination-button-width: 28px;
  --el-pagination-button-height: 28px;
  font-size: 12px;
}

.pagination-container :deep(.el-pagination .el-select .el-input) {
  width: 80px;
}

.history-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
}
</style>
