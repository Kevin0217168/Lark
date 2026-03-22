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
        <div class="video-card">
          <div class="video-container" v-if="currentFrameImage">
            <img 
              :src="currentFrameImage" 
              class="video-stream"
              :class="{ 'flip-horizontal': flipHorizontal, 'flip-vertical': flipVertical, 'disconnected': isStreamDisconnected }"
              alt="实时监控画面"
            />
            <div class="frame-info">
              <span>{{ lastFrameTime }}</span>
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
          <h3>所有设备平均值</h3>
          <div ref="averageChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>温度数据</h3>
          <div ref="temperatureChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>湿度数据</h3>
          <div ref="humidityChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>环境质量</h3>
          <div ref="qualityChartRef" class="chart"></div>
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
          <div v-for="(item, index) in historyDataList" :key="index" class="history-item">
            <div class="history-time">{{ item.timestamp }}</div>
            <div class="history-data">
              <div class="data-point">
                <span class="label">温度</span>
                <span class="value">{{ item.temperature }}°C</span>
              </div>
              <div class="data-point">
                <span class="label">湿度</span>
                <span class="value">{{ item.humidity }}%</span>
              </div>
              <div class="data-point">
                <span class="label">质量</span>
                <el-tag :type="getQualityType(item.quality)" size="small">
                  {{ getQualityText(item.quality) }}
                </el-tag>
              </div>
            </div>
          </div>
        </div>
      </div>
      
      <div v-else class="history-placeholder">
        <el-empty description="请选择设备查看历史数据" />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted, nextTick } from 'vue';
import { useDeviceStore } from '../../stores/deviceStore';
import { ElMessage, ElMessageBox } from 'element-plus';
import { Warning, Switch, Sort, Refresh } from '@element-plus/icons-vue';
import * as echarts from 'echarts';

const getToken = () => {
  return localStorage.getItem('accessToken');
};

const { getDevices, getDeviceHistoryData, getDeviceAverageData, fetchDevices } = useDeviceStore();
const devices = getDevices();

const props = defineProps<{
  activeTab?: string;
}>();

const activeTab = computed(() => props.activeTab || 'realtime');

const selectedDeviceId = ref<number | null>(null);

const selectedDevice = computed(() => {
  if (!selectedDeviceId.value) return null;
  return devices.find(device => device.id === selectedDeviceId.value) || null;
});

const isWebSocketConnected = computed(() => {
  return ws !== null && ws.readyState === WebSocket.OPEN;
});

const handleDeviceChange = () => {
  if (selectedDevice.value) {
    ElMessage.success(`已切换到设备: ${selectedDevice.value.name}`);
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
  return historyData
    .map(data => ({
      ...data,
      timestamp: data.timestamp
    }))
    .sort((a, b) => new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime());
});

const handleHistoryDeviceChange = () => {
  if (selectedHistoryDevice.value) {
    ElMessage.success(`已切换到设备: ${selectedHistoryDevice.value.name}`);
  }
};

const getQualityType = (quality: number) => {
  if (quality >= 80) return 'success';
  if (quality >= 60) return 'warning';
  return 'danger';
};

const getQualityText = (quality: number) => {
  if (quality >= 80) return '优秀';
  if (quality >= 60) return '良好';
  return '较差';
};

// 图表容器引用
const temperatureChartRef = ref<HTMLElement | null>(null);
const humidityChartRef = ref<HTMLElement | null>(null);
const qualityChartRef = ref<HTMLElement | null>(null);
const averageChartRef = ref<HTMLElement | null>(null);

let temperatureChart: echarts.ECharts | null = null;
let humidityChart: echarts.ECharts | null = null;
let qualityChart: echarts.ECharts | null = null;
let averageChart: echarts.ECharts | null = null;

// 实时监控图片帧
const currentFrameImage = ref<string>('');
const lastFrameTime = ref<string>('');
const connectionError = ref<string>('');
const isStreamDisconnected = ref<boolean>(false);
let lastFrameTimestamp = 0;
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
    lastFrameTimestamp = Date.now();
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
    
    const wsUrl = `/api/stream/viewer/ws?token=${token}`;
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
      
      startDeviceStreaming();
    };
    
    ws.onmessage = (event) => {
      try {
        try {
          const data = JSON.parse(event.data);
          if (data.code === 1) {
            console.log('操作成功:', data.msg);
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
    if (selectedDeviceId.value) {
      try {
        const token = getToken();
        console.log('取消订阅时的token:', token);
        
        await fetch(`/api/stream/viewer/following/${selectedDeviceId.value}`, {
          method: 'DELETE',
          headers: {
            'Accept': 'application/json',
            'Authorization': `Bearer ${token}`
          }
        });
        console.log('取消设备推流成功');
      } catch (error) {
        console.error('取消设备推流失败:', error);
      }
    }
    
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

// 初始化图表
const initCharts = () => {
  // 销毁旧图表
  temperatureChart?.dispose();
  humidityChart?.dispose();
  qualityChart?.dispose();
  averageChart?.dispose();
  
  // 初始化平均图表
  if (averageChartRef.value) {
    averageChart = echarts.init(averageChartRef.value);
    const avgData = getDeviceAverageData();
    
    averageChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      legend: {
        data: ['平均温度', '平均湿度', '平均质量指数'],
        top: 10,
        textStyle: { fontSize: 12 },
        itemGap: 10
      },
      grid: {
        left: '3%',
        right: '4%',
        bottom: '3%',
        top: '60',
        containLabel: true
      },
      xAxis: {
        type: 'category',
        data: avgData.times.slice(-12),
        axisLabel: { fontSize: 10, rotate: 45 }
      },
      yAxis: {
        type: 'value',
        axisLabel: { fontSize: 10 }
      },
      series: [
        {
          name: '平均温度',
          data: avgData.temperatureValues.slice(-12),
          type: 'line',
          smooth: true,
          itemStyle: { color: '#ff7875' }
        },
        {
          name: '平均湿度',
          data: avgData.humidityValues.slice(-12),
          type: 'line',
          smooth: true,
          itemStyle: { color: '#69c0ff' }
        },
        {
          name: '平均质量指数',
          data: avgData.qualityValues.slice(-12),
          type: 'line',
          smooth: true,
          itemStyle: { color: '#73d13d' }
        }
      ]
    });
  }
  
  // 初始化温度图表
  if (temperatureChartRef.value && selectedDeviceId.value) {
    temperatureChart = echarts.init(temperatureChartRef.value);
    const historyData = getDeviceHistoryData(selectedDeviceId.value);
    
    temperatureChart.setOption({
      tooltip: { trigger: 'axis' },
      grid: {
        left: '3%',
        right: '4%',
        bottom: '3%',
        top: '30',
        containLabel: true
      },
      xAxis: {
        type: 'category',
        data: historyData.map(d => d.timestamp).slice(-12),
        axisLabel: { fontSize: 10, rotate: 45 }
      },
      yAxis: {
        type: 'value',
        name: '温度(°C)',
        axisLabel: { fontSize: 10 }
      },
      series: [{
        data: historyData.map(d => d.temperature).slice(-12),
        type: 'line',
        smooth: true,
        itemStyle: { color: '#ff7875' }
      }]
    });
  }
  
  // 初始化湿度图表
  if (humidityChartRef.value && selectedDeviceId.value) {
    humidityChart = echarts.init(humidityChartRef.value);
    const historyData = getDeviceHistoryData(selectedDeviceId.value);
    
    humidityChart.setOption({
      tooltip: { trigger: 'axis' },
      grid: {
        left: '3%',
        right: '4%',
        bottom: '3%',
        top: '30',
        containLabel: true
      },
      xAxis: {
        type: 'category',
        data: historyData.map(d => d.timestamp).slice(-12),
        axisLabel: { fontSize: 10, rotate: 45 }
      },
      yAxis: {
        type: 'value',
        name: '湿度(%)',
        axisLabel: { fontSize: 10 }
      },
      series: [{
        data: historyData.map(d => d.humidity).slice(-12),
        type: 'line',
        smooth: true,
        itemStyle: { color: '#69c0ff' }
      }]
    });
  }
  
  // 初始化质量图表
  if (qualityChartRef.value && selectedDeviceId.value) {
    qualityChart = echarts.init(qualityChartRef.value);
    const historyData = getDeviceHistoryData(selectedDeviceId.value);
    
    qualityChart.setOption({
      tooltip: { trigger: 'axis' },
      grid: {
        left: '3%',
        right: '4%',
        bottom: '3%',
        top: '30',
        containLabel: true
      },
      xAxis: {
        type: 'category',
        data: historyData.map(d => d.timestamp).slice(-12),
        axisLabel: { fontSize: 10, rotate: 45 }
      },
      yAxis: {
        type: 'value',
        name: '质量指数',
        axisLabel: { fontSize: 10 }
      },
      series: [{
        data: historyData.map(d => d.quality).slice(-12),
        type: 'line',
        smooth: true,
        itemStyle: { color: '#73d13d' }
      }]
    });
  }
};

// 窗口大小变化时调整图表
const handleResize = () => {
  temperatureChart?.resize();
  humidityChart?.resize();
  qualityChart?.resize();
  averageChart?.resize();
};

// 监听activeTab变化
watch(() => props.activeTab, (newTab) => {
  if (newTab === 'analysis') {
    if (devices.length > 0 && !selectedDeviceId.value) {
      selectedDeviceId.value = devices[0]?.id || null;
    }
    nextTick(() => {
      initCharts();
    });
  } else if (newTab === 'realtime') {
    if (devices.length > 0 && !selectedDeviceId.value) {
      selectedDeviceId.value = devices[0]?.id || null;
    }
  } else if (newTab === 'history') {
    if (devices.length > 0 && !historyDeviceId.value) {
      historyDeviceId.value = devices[0]?.id || null;
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

onMounted(async () => {
  await fetchDevices();
  window.addEventListener('resize', handleResize);
  
  // 默认选择第一个设备
  if (devices.length > 0) {
    if (activeTab.value === 'realtime' && !selectedDeviceId.value) {
      selectedDeviceId.value = devices[0]?.id || null;
    } else if (activeTab.value === 'history' && !historyDeviceId.value) {
      historyDeviceId.value = devices[0]?.id || null;
    } else if (activeTab.value === 'analysis' && !selectedDeviceId.value) {
      selectedDeviceId.value = devices[0]?.id || null;
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
  window.removeEventListener('resize', handleResize);
  temperatureChart?.dispose();
  humidityChart?.dispose();
  qualityChart?.dispose();
  averageChart?.dispose();
});
</script>

<style scoped>
.mobile-data {
  min-height: 100vh;
  background: #f5f7fa;
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
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
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
  background: rgba(0, 0, 0, 0.6);
  color: white;
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 12px;
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
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.history-time {
  font-size: 12px;
  color: #909399;
  margin-bottom: 12px;
}

.history-data {
  display: flex;
  justify-content: space-between;
}

.data-point {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
}

.data-point .label {
  font-size: 12px;
  color: #909399;
}

.data-point .value {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
}

.history-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
}
</style>
