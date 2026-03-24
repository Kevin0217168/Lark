<template>
  <div class="data-card">
    <div class="title">
      <h2>数据管理</h2>
    </div>
    
    <div class="data-content">
      <!-- 实时数据 -->
      <div v-if="props.activeTab === 'realtime'" class="realtime-container">
        <!-- 左侧数据区域 -->
        <div class="data-left">
          <!-- 设备选择 -->
          <div class="device-selector">
            <h3>设备选择</h3>
            <el-select v-model="selectedDeviceId" placeholder="选择设备" @change="handleDeviceChange" style="width: 100%;">
              <el-option 
                v-for="device in devices" 
                :key="device.id" 
                :label="device.name" 
                :value="device.id" 
              />
            </el-select>
          </div>
          
          <!-- 设备数据 -->
          <div class="device-data" v-if="selectedDevice">
            <h3>设备数据</h3>
            <div class="data-card-content compact">
              <div class="data-row">
                <div class="data-cell">
                  <el-icon><Monitor /></el-icon>
                  <span class="cell-label">名称</span>
                  <span class="cell-value">{{ selectedDevice.name }}</span>
                </div>
                <div class="data-cell">
                  <el-icon><CircleCheck v-if="selectedDevice.isOnline" /><CircleClose v-else /></el-icon>
                  <span class="cell-label">状态</span>
                  <el-tag :type="selectedDevice.isOnline ? 'success' : 'danger'" size="small">
                    {{ selectedDevice.isOnline ? '在线' : '离线' }}
                  </el-tag>
                </div>
              </div>
              <div class="data-row">
                <div class="data-cell">
                  <el-icon><Odometer /></el-icon>
                  <span class="cell-label">温度</span>
                  <span class="cell-value highlight">{{ selectedDevice.temperature }}°C</span>
                </div>
                <div class="data-cell">
                  <el-icon><Sort /></el-icon>
                  <span class="cell-label">湿度</span>
                  <span class="cell-value highlight">{{ selectedDevice.humidity }}%</span>
                </div>
              </div>
              <div class="data-row full">
                <div class="data-cell">
                  <el-icon><Clock /></el-icon>
                  <span class="cell-label">创建时间</span>
                  <span class="cell-value time">{{ selectedDevice.createTime }}</span>
                </div>
              </div>
            </div>
          </div>
        </div>
        
        <!-- 右侧视频流区域 -->
        <div class="data-right">
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
                手动重连
              </el-button>
              <el-button 
                :type="flipHorizontal ? 'primary' : 'default'"
                size="small"
                @click="toggleHorizontalFlip"
              >
                <el-icon><Switch /></el-icon>
                左右翻转
              </el-button>
              <el-button 
                :type="flipVertical ? 'primary' : 'default'"
                size="small"
                @click="toggleVerticalFlip"
              >
                <el-icon><Sort /></el-icon>
                上下翻转
              </el-button>
              <div class="quality-control">
                <span class="quality-label">视频质量</span>
                <el-slider v-model="imageQuality" :min="1" :max="100" :disabled="true" style="width: 120px" />
              </div>
            </div>
          </div>
          <div class="video-card">
            <div class="video-container" v-if="selectedDevice && currentFrameImage">
              <img 
                :src="currentFrameImage" 
                class="video-stream"
                :class="{ 'flip-horizontal': flipHorizontal, 'flip-vertical': flipVertical, 'disconnected': isStreamDisconnected }"
                alt="实时监控画面"
              />
              <div class="frame-info">
                <span>更新时间: {{ lastFrameTime }}</span>
              </div>
              <div class="stream-disconnected-overlay" v-if="isStreamDisconnected">
                <div class="disconnected-content">
                  <el-icon class="disconnected-icon" :size="48"><Warning /></el-icon>
                  <div class="disconnected-message">视频流已断联，请重连</div>
                </div>
              </div>
            </div>
            <div class="video-error" v-else-if="selectedDevice && connectionError">
              <div class="error-content">
                <el-icon class="error-icon" :size="48"><Warning /></el-icon>
                <div class="error-message">{{ connectionError }}</div>
              </div>
            </div>
            <div class="video-placeholder" v-else-if="selectedDevice">
              <el-empty description="未开始实时监控" />
            </div>
            <div class="video-placeholder" v-else>
              <el-empty description="请选择设备" />
            </div>
          </div>
        </div>
      </div>
      
      <!-- 分析 -->
      <div v-else-if="props.activeTab === 'analysis'" class="tab-content">
        <div class="card-header">
          <div class="card-title">
            <span>数据分析</span>
          </div>
          <div class="card-actions">
            <el-select v-model="selectedDeviceId" @change="handleDeviceChange" class="device-select">
                <el-option
                v-for="device in devices"
                :key="device.id"
                :label="device.name"
                :value="device.id"
              />
            </el-select>
          </div>
        </div>
        <div class="analysis-content">
          <!-- 所有设备平均值图表 -->
          <div class="chart-container average-chart">
            <h3>所有设备平均值</h3>
            <div ref="averageChartRef" class="chart"></div>
          </div>
          <div class="chart-container">
            <h3>温度数据</h3>
            <div ref="temperatureChartRef" class="chart"></div>
          </div>
          <div class="chart-container">
            <h3>湿度数据</h3>
            <div ref="humidityChartRef" class="chart"></div>
          </div>
          <div class="chart-container">
            <h3>环境质量</h3>
            <div ref="qualityChartRef" class="chart"></div>
          </div>
        </div>
      </div>
      
      <!-- 历史数据 -->
      <div v-else-if="props.activeTab === 'history'" class="tab-content">
        <div class="card-header">
          <div class="card-title">
            <span>历史数据</span>
          </div>
        </div>
        <div class="history-content" v-if="historyDeviceId">
          <div class="top-section">
            <div class="device-selector">
              <h3>设备选择</h3>
              <el-select v-model="historyDeviceId" @change="handleHistoryDeviceChange" class="device-select">
                <el-option
                  v-for="device in devices"
                  :key="device.id"
                  :label="device.name"
                  :value="device.id"
                />
              </el-select>
            </div>
            <div class="device-info">
              <h3>设备信息</h3>
              <div class="info-card">
                <div class="info-item">
                  <span class="info-label">设备名称:</span>
                  <span class="info-value">{{ selectedHistoryDevice?.name }}</span>
                </div>
                <div class="info-item">
                  <span class="info-label">设备ID:</span>
                  <span class="info-value">{{ selectedHistoryDevice?.id }}</span>
                </div>
                <div class="info-item">
                  <span class="info-label">设备状态:</span>
                  <span class="info-value" :class="selectedHistoryDevice?.status">
                    {{ selectedHistoryDevice?.status === 'online' ? '在线' : '离线' }}
                  </span>
                </div>
              </div>
            </div>
          </div>
          
          <div class="history-table">
            <h3>历史数据记录</h3>
            <el-table :data="historyDataList" stripe style="width: 100%;" max-height="400" border>
              <el-table-column prop="timestamp" label="时间" min-width="180" align="left" />
              <el-table-column prop="temperature" label="温度 (℃)" min-width="120" align="center" />
              <el-table-column prop="humidity" label="湿度 (%)" min-width="120" align="center" />
              <el-table-column prop="quality" label="环境质量指数" min-width="150" align="center" />
              <el-table-column label="状态" min-width="100" align="center">
                <template #default="{ row }">
                  <el-tag :type="getQualityType(row.quality)">
                    {{ getQualityText(row.quality) }}
                  </el-tag>
                </template>
              </el-table-column>
            </el-table>
          </div>
        </div>
        <div class="history-placeholder" v-else>
          <el-empty description="请选择设备查看历史数据" />
        </div>
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed, watch, onMounted, onUnmounted, nextTick } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { useDeviceStore } from '../stores/deviceStore';
import { ElMessage, ElMessageBox, ElIcon } from 'element-plus';
import { Warning, Switch, Sort, Monitor, CircleCheck, CircleClose, Odometer, Clock, FullScreen } from '@element-plus/icons-vue';
import * as echarts from 'echarts';

// 获取登录token
const getToken = () => {
  return localStorage.getItem('accessToken');
};

const route = useRoute();
const router = useRouter();
const { getDevices, getDeviceHistoryData, getDeviceAverageData, fetchDevices } = useDeviceStore();
// 直接获取设备数据
const devices = getDevices();

// 接收父组件传递的activeTab
const props = defineProps<{
  activeTab: string
}>();

const selectedDeviceId = ref<number | null>(null);

const selectedDevice = computed(() => {
  if (!selectedDeviceId.value) return null;
  return devices.find(device => device.id === selectedDeviceId.value) || null;
});

// 计算WebSocket是否已连接
const isWebSocketConnected = computed(() => {
  return ws !== null && ws.readyState === WebSocket.OPEN;
});

const handleDeviceChange = () => {
  if (selectedDevice.value) {
    ElMessage.success(`已切换到设备: ${selectedDevice.value.name}`);
    // 切换设备后更新图表
    nextTick(() => {
      initCharts();
    });
    // 重新开始实时监控
    startRealtimeMonitoring();
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

// 获取质量状态类型
const getQualityType = (quality: number) => {
  if (quality >= 80) return 'success';
  if (quality >= 60) return 'warning';
  return 'danger';
};

// 获取质量状态文本
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

// 图表实例
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
const imageQuality = ref(80); // 图片质量滑条值（空实现）

// 切换左右翻转
const toggleHorizontalFlip = () => {
  flipHorizontal.value = !flipHorizontal.value;
};

// 切换上下翻转
const toggleVerticalFlip = () => {
  flipVertical.value = !flipVertical.value;
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
    lastFrameTimestamp = Date.now();
    isStreamDisconnected.value = false;
  } catch (error) {
    console.error('处理图片数据失败:', error);
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
  // 关闭旧的连接
  stopRealtimeMonitoring();
  
  if (!selectedDeviceId.value) {
    console.log('没有选择设备，无法启动实时监控');
    return;
  }
  
  try {
    // 先刷新token
    console.log('开始刷新token');
    let token = await refreshToken();
    if (!token) {
      connectionError.value = '未登录，请先登录';
      console.log('没有token，无法建立WebSocket连接');
      return;
    }
    
    // 建立WebSocket连接
    console.log('开始建立WebSocket连接');
    // 使用相对路径，让Vite代理转发到后端
    const wsUrl = `/api/stream/viewer/ws?token=${token}`;
    console.log('WebSocket URL:', wsUrl.substring(0, 50) + '...');
    connectionError.value = '正在开启ws连接';
    ws = new WebSocket(wsUrl);
    
    // 监听连接打开事件
    ws.onopen = () => {
      console.log('WebSocket连接已建立');
      connectionError.value = '';
      lastFrameTimestamp = Date.now();
      isStreamDisconnected.value = false;
      
      // 启动定时器检查图片流是否更新
      if (streamCheckInterval) {
        clearInterval(streamCheckInterval);
      }
      streamCheckInterval = window.setInterval(() => {
        const now = Date.now();
        if (now - lastFrameTimestamp > 2000 && currentFrameImage.value) {
          isStreamDisconnected.value = true;
        }
      }, 2500);
      
      // 连接成功后，发送请求开启设备推流
      startDeviceStreaming();
    };
    
    // 监听消息事件
    ws.onmessage = (event) => {
      try {
        // 尝试解析JSON数据
        try {
          const data = JSON.parse(event.data);
          if (data.code === 1) {
            console.log('操作成功:', data.msg);
          } else {
            console.error('操作失败:', data.msg);
          }
        } catch (jsonError) {
          // 如果不是JSON数据，可能是二进制图片数据
          handleFrameData(event.data);
          console.log('接收到图片数据');
        }
      } catch (error) {
        console.error('解析WebSocket消息失败:', error);
      }
    };
    
    // 监听错误事件
    ws.onerror = (error) => {
      console.error('WebSocket连接错误:', error);
      connectionError.value = 'WSS连接错误';
    };
    
    // 监听连接关闭事件
    ws.onclose = (event) => {
      console.log('WebSocket连接已关闭:', event);
      console.log('WebSocket关闭代码:', event.code);
      console.log('WebSocket关闭原因:', event.reason);
      if (!event.wasClean) {
        connectionError.value = 'WSS连接已关闭';
      }
    };
    
  } catch (error) {
    console.error('建立WebSocket连接失败:', error);
    connectionError.value = '建立WSS连接失败';
  }
};

// 手动重连WebSocket
const reconnectWebSocket = () => {
  console.log('手动重连WebSocket');
  // 检查当前连接状态
  if (ws && ws.readyState === WebSocket.OPEN) {
    // 连接正常，询问用户是否确定要重连
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
    // 连接异常或不存在，直接重连
    console.log('连接异常或不存在，直接重连');
    startRealtimeMonitoring();
  }
};

// 刷新token
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
        // 保存新token
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

// 开启设备推流
const startDeviceStreaming = async () => {
  if (!selectedDeviceId.value) return;
  
  try {
    // 获取登录token
    const token = getToken();
    if (!token) {
      connectionError.value = '未登录，请先登录';
      return;
    }
    
    // 发送POST请求开启设备推流，需要在请求头中包含Authorization字段
    const response = await fetch(`/api/stream/viewer/following/${selectedDeviceId.value}`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${token}`
      }
    });
    
    const data = await response.json();
    
    // 延迟3秒再判断返回情况
    await new Promise(resolve => setTimeout(resolve, 4000));
    
    // 判断成功的条件：code为1 或 消息包含成功关键字
    const isSuccess = response.ok && (data.code === 1 || 
      (data.msg && (data.msg.includes('OK') || data.msg.includes('success') || data.msg.includes('成功'))));
    
    if (isSuccess) {
      console.log('设备推流开启成功:', data.msg);
      connectionError.value = '';
    } else if (data.msg && data.msg.toLowerCase().includes('device has subscribed')) {
      // 设备已经被订阅，这是正常情况，不需要显示错误
      console.log('设备已经被订阅');
      connectionError.value = '';
    } else if (data.msg && data.msg.toLowerCase().includes('viewer has not registerd')) {
      // Viewer未注册，但WebSocket可能已经成功连接，不显示错误
      console.log('Viewer未注册，但WebSocket可能已经成功连接');
      connectionError.value = '';
    } else if (response.status === 401) {
      // 认证失败，但WebSocket可能已经成功连接，不显示错误
      console.log('认证失败，但WebSocket可能已经成功连接');
      connectionError.value = '';
    } else {
      console.error('设备推流开启失败:', data.msg || '未知错误');
      connectionError.value = '设备推流开启失败';
    }
  } catch (error) {
    console.error('开启设备推流失败:', error);
    // 延迟3秒再显示错误提示
    await new Promise(resolve => setTimeout(resolve, 3000));
    connectionError.value = '开启设备推流失败';
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
    // 如果有选中的设备，先取消订阅
    if (selectedDeviceId.value) {
      try {
        // 获取并打印当前token
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
  
  connectionError.value = '';
  isStreamDisconnected.value = false;
};



// 从deviceStore获取历史数据
const generateMockData = (deviceId: number, type: 'temperature' | 'humidity' | 'quality') => {
  const times: string[] = [];
  const values: number[] = [];
  
  // 从store获取设备历史数据
  const historyData = getDeviceHistoryData(deviceId);
  
  // 处理历史数据，只取最近24小时
  const filteredData = historyData.slice(-24);
  
  // 处理历史数据
  filteredData.forEach(data => {
    const date = new Date(data.timestamp);
    times.push(`${date.getHours().toString().padStart(2, '0')}:00`);
    
    switch (type) {
      case 'temperature':
        values.push(data.temperature);
        break;
      case 'humidity':
        values.push(data.humidity);
        break;
      case 'quality':
        values.push(data.quality);
        break;
    }
  });
  
  // 如果没有历史数据，返回空数组
  if (times.length === 0) {
    // 生成最近24小时的时间
    for (let i = 23; i >= 0; i--) {
      const date = new Date();
      date.setHours(date.getHours() - i);
      times.push(`${date.getHours().toString().padStart(2, '0')}:00`);
      values.push(0);
    }
  }
  
  return { times, values };
};

// 初始化图表
const initCharts = () => {
  if (!selectedDeviceId.value) return;
  
  // 温度图表
  if (temperatureChartRef.value) {
    if (temperatureChart) {
      temperatureChart.dispose();
    }
    temperatureChart = echarts.init(temperatureChartRef.value);
    const tempData = generateMockData(selectedDeviceId.value, 'temperature');
    temperatureChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      xAxis: {
        type: 'category',
        data: tempData.times
      },
      yAxis: {
        type: 'value',
        name: '温度 (℃)'
      },
      series: [{
        data: tempData.values,
        type: 'line',
        smooth: true,
        itemStyle: {
          color: '#ff7875'
        },
        areaStyle: {
          color: {
            type: 'linear',
            x: 0, y: 0, x2: 0, y2: 1,
            colorStops: [{
              offset: 0, color: 'rgba(255, 120, 117, 0.3)'
            }, {
              offset: 1, color: 'rgba(255, 120, 117, 0.1)'
            }]
          }
        }
      }]
    });
  }
  
  // 湿度图表
  if (humidityChartRef.value) {
    if (humidityChart) {
      humidityChart.dispose();
    }
    humidityChart = echarts.init(humidityChartRef.value);
    const humidityData = generateMockData(selectedDeviceId.value, 'humidity');
    humidityChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      xAxis: {
        type: 'category',
        data: humidityData.times
      },
      yAxis: {
        type: 'value',
        name: '湿度 (%)'
      },
      series: [{
        data: humidityData.values,
        type: 'line',
        smooth: true,
        itemStyle: {
          color: '#69c0ff'
        },
        areaStyle: {
          color: {
            type: 'linear',
            x: 0, y: 0, x2: 0, y2: 1,
            colorStops: [{
              offset: 0, color: 'rgba(105, 192, 255, 0.3)'
            }, {
              offset: 1, color: 'rgba(105, 192, 255, 0.1)'
            }]
          }
        }
      }]
    });
  }
  
  // 环境质量图表
  if (qualityChartRef.value) {
    if (qualityChart) {
      qualityChart.dispose();
    }
    qualityChart = echarts.init(qualityChartRef.value);
    const qualityData = generateMockData(selectedDeviceId.value, 'quality');
    qualityChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      xAxis: {
        type: 'category',
        data: qualityData.times
      },
      yAxis: {
        type: 'value',
        name: '质量指数'
      },
      series: [{
        data: qualityData.values,
        type: 'line',
        smooth: true,
        itemStyle: {
          color: '#73d13d'
        },
        areaStyle: {
          color: {
            type: 'linear',
            x: 0, y: 0, x2: 0, y2: 1,
            colorStops: [{
              offset: 0, color: 'rgba(115, 209, 61, 0.3)'
            }, {
              offset: 1, color: 'rgba(115, 209, 61, 0.1)'
            }]
          }
        }
      }]
    });
  }
  
  // 所有设备平均值图表
  if (averageChartRef.value) {
    if (averageChart) {
      averageChart.dispose();
    }
    averageChart = echarts.init(averageChartRef.value);
    
    // 计算所有设备的平均值
    const avgData = getDeviceAverageData();
    
    averageChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      legend: {
        data: ['平均温度', '平均湿度', '平均质量指数']
      },
      xAxis: {
        type: 'category',
        data: avgData.times
      },
      yAxis: {
        type: 'value',
        name: '数值'
      },
      series: [
        {
          name: '平均温度',
          data: avgData.temperatureValues,
          type: 'line',
          smooth: true,
          itemStyle: {
            color: '#ff7875'
          }
        },
        {
          name: '平均湿度',
          data: avgData.humidityValues,
          type: 'line',
          smooth: true,
          itemStyle: {
            color: '#69c0ff'
          }
        },
        {
          name: '平均质量指数',
          data: avgData.qualityValues,
          type: 'line',
          smooth: true,
          itemStyle: {
            color: '#73d13d'
          }
        }
      ]
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

// 组件挂载时初始化
onMounted(async () => {
  window.addEventListener('resize', handleResize);
  
  console.log('Component mounted');
  console.log('Initial active tab:', props.activeTab);
  
  // 从后端获取设备数据
  await fetchDevices();
  
  console.log('Devices available:', devices.length);
  
  // 如果默认有设备，初始化图表
  if (devices.length > 0 && !selectedDeviceId.value) {
    console.log('Setting default device:', devices[0]?.id);
    selectedDeviceId.value = devices[0]?.id || null;
  }
  
  // 如果在实时数据标签页，由watch(selectedDeviceId)处理实时监控的启动
  // 不直接调用startRealtimeMonitoring()，避免与watch重复调用
  if (props.activeTab === 'realtime' && selectedDeviceId.value) {
    console.log('Device selected, realtime monitoring will be started by watch');
  }
});

// 组件卸载时清理
onUnmounted(() => {
  window.removeEventListener('resize', handleResize);
  stopRealtimeMonitoring();
  temperatureChart?.dispose();
  humidityChart?.dispose();
  qualityChart?.dispose();
  averageChart?.dispose();
});

// 监听activeTab变化，重置设备选择
watch(() => props.activeTab, (newTab, oldTab) => {
  console.log('Active tab changed:', newTab, 'Old tab:', oldTab);
  console.log('Devices available:', devices.length);
  console.log('Selected device ID:', selectedDeviceId.value);
  
  if (newTab === 'analysis') {
    // 切换到分析标签时，如果有设备，初始化图表
    if (devices.length > 0 && !selectedDeviceId.value) {
      selectedDeviceId.value = devices[0]?.id || null;
    }
    nextTick(() => {
      initCharts();
    });
    // 停止实时监控
    stopRealtimeMonitoring();
  } else if (newTab === 'history') {
    // 切换到历史数据标签时，如果有设备，自动选择第一个设备
    if (devices.length > 0 && !historyDeviceId.value) {
      historyDeviceId.value = devices[0]?.id || null;
    }
    // 停止实时监控
    stopRealtimeMonitoring();
  } else if (newTab === 'realtime') {
    // 切换到实时数据标签时，重新获取设备数据
    fetchDevices().then(() => {
      // 确保有设备被选择
      if (devices.length > 0) {
        // 不管之前是否有选择，都强制选择第一个设备
        const deviceId = devices[0]?.id || null;
        selectedDeviceId.value = deviceId;
        
        // 直接启动实时监控，不依赖watch
        if (deviceId) {
          console.log('Auto-starting realtime monitoring for device:', deviceId);
          // 使用nextTick确保selectedDeviceId已更新
          nextTick(() => {
            startRealtimeMonitoring();
          });
        }
      } else {
        console.log('No devices available, cannot start realtime monitoring');
      }
    });
  } else {
    selectedDeviceId.value = null;
    // 停止实时监控
    stopRealtimeMonitoring();
  }
}, { immediate: true });

// 监听路由参数变化，自动选择设备
watch(() => route.query.deviceId, (newDeviceId) => {
  if (newDeviceId) {
    const deviceId = parseInt(newDeviceId as string);
    if (!isNaN(deviceId)) {
      selectedDeviceId.value = deviceId;
    }
  }
}, { immediate: true });

// 监听路由变化，确保在导航到数据页面时自动选择设备
watch(() => route.path, (newPath) => {
  console.log('Route path changed:', newPath);
  if (newPath === '/Data' || newPath === '/Stream') {
    console.log('Navigated to data page, checking device selection');
    if (devices.length > 0 && !selectedDeviceId.value) {
      console.log('Auto-selecting first device:', devices[0]?.id);
      selectedDeviceId.value = devices[0]?.id || null;
    }
  }
});

// 监听selectedDeviceId变化，自动开启实时监控
watch(() => selectedDeviceId.value, (newDeviceId, oldDeviceId) => {
  console.log('Selected device ID changed:', newDeviceId, 'Old device ID:', oldDeviceId);
  console.log('Current active tab:', props.activeTab);
  
  // 使用nextTick确保activeTab已经更新
  nextTick(() => {
    console.log('Active tab after nextTick:', props.activeTab);
    // 只有当有设备ID且当前是实时标签时才启动监控
    if (newDeviceId && props.activeTab === 'realtime') {
      console.log('Auto-starting realtime monitoring for device:', newDeviceId);
      startRealtimeMonitoring();
    }
  });
});

// 组件挂载时检查URL参数已移至上面的onMounted函数

// 跳转到总览界面的大图模式全屏状态
const goToFullscreen = () => {
  router.push({
    path: '/Home',
    query: { fullscreen: 'true' }
  });
};
</script>

<style lang="scss" scoped>
.data-card {
  z-index: 1;
  width: 75%;
  margin: 1% auto;
  border-radius: 15px;
  background-color: #fff;
  padding: 30px;
  min-height: auto;
}

.title {
  text-align: center;
  margin-bottom: 20px;
}

.title h2 {
  margin: 0;
  color: #303133;
  font-size: 20px;
}

.data-content {
  min-height: 0;
}

.realtime-container {
  display: flex;
  gap: 20px;
  min-height: 0;
}

.data-left {
  flex: 0 0 30%;
  min-height: 0;
}

.data-right {
  flex: 1;
  min-height: 0;
}

.device-selector {
  margin-bottom: 20px;
}

.device-selector h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.device-data h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.data-card-content {
  border-radius: 15px;
  min-height: 0;
}

.data-item {
  margin-bottom: 15px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 5px 0;
}

.data-label {
  color: #909399;
  font-size: 14px;
}

.data-value {
  color: #303133;
  font-size: 14px;
  font-weight: 500;
}

.data-value.online {
  color: #67c23a;
}

.data-value.offline {
  color: #f56c6c;
}

.data-actions {
  margin-top: 20px;
  text-align: center;
  padding-top: 15px;
  border-top: 1px solid #e4e7ed;
}

.data-actions .el-button {
  width: 100%;
}

/* 优化后的设备数据展示样式 */
.data-card-content.compact {
  padding: 15px;
  background: #f8f9fa;
  border-radius: 10px;
}

.data-row {
  display: flex;
  gap: 10px;
  margin-bottom: 12px;
}

.data-row:last-child {
  margin-bottom: 0;
}

.data-row.full {
  flex-direction: column;
}

.data-cell {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 10px;
  background: #fff;
  border-radius: 8px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
}

.data-cell .el-icon {
  color: #909399;
  font-size: 14px;
}

.cell-label {
  color: #909399;
  font-size: 12px;
  min-width: 36px;
}

.cell-value {
  color: #303133;
  font-size: 13px;
  font-weight: 500;
  flex: 1;
  text-align: right;
}

.cell-value.highlight {
  color: #409eff;
  font-size: 14px;
}

.cell-value.time {
  font-size: 11px;
  color: #606266;
}

/* 视频控制区域样式 */
.video-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
}

.video-header h3 {
  margin: 0;
  font-size: 16px;
  color: #303133;
}

.video-controls {
  display: flex;
  align-items: center;
  gap: 10px;
}

.quality-control {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-left: 10px;
  padding-left: 10px;
  border-left: 1px solid #e4e7ed;
}

.quality-label {
  font-size: 13px;
  color: #606266;
  white-space: nowrap;
}

/* 视频翻转样式 */
.video-stream.flip-horizontal {
  transform: scaleX(-1);
}

.video-stream.flip-vertical {
  transform: scaleY(-1);
}

.video-stream.flip-horizontal.flip-vertical {
  transform: scaleX(-1) scaleY(-1);
}

.analysis-content {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.chart-container {
  margin-bottom: 20px;
}

.chart-container h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.chart {
  width: 100%;
  height: 300px;
  border-radius: 10px;
  background-color: #f9f9f9;
}

.video-card {
  border-radius: 15px;
  min-height: 400px;
  display: flex;
  flex-direction: column;
}

.video-container {
  flex: 1;
  background: #f5f7fa;
  border-radius: 10px;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 400px;
  position: relative;
}

.video-stream {
  max-width: 100%;
  max-height: 100%;
  width: auto;
  height: auto;
  object-fit: contain;
  flex: 1;
}

.frame-info {
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
  background: rgba(0, 0, 0, 0.6);
  color: white;
  padding: 8px 12px;
  font-size: 12px;
  text-align: right;
  z-index: 10;
}

.video-stream.disconnected {
  filter: blur(5px);
}

.stream-disconnected-overlay {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  backdrop-filter: blur(5px);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 20;
}

.disconnected-content {
  text-align: center;
  color: white;
}

.disconnected-icon {
  color: #f56c6c;
  margin-bottom: 10px;
}

.disconnected-message {
  font-size: 16px;
  font-weight: 500;
}

.video-placeholder {
  flex: 1;
  background: #f5f7fa;
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 0;
}

.video-error {
  flex: 1;
  background: #f5f7fa;
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 0;
}

.error-content {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 16px;
}

.error-icon {
  color: #f56c6c;
}

.error-message {
  font-size: 16px;
  color: #606266;
  text-align: center;
}

.tab-content {
  min-height: 400px;
  background: #f5f7fa;
  border-radius: 15px;
  margin-top: 20px;
  padding: 20px;
}

.tab-content .el-card {
  background: #fff;
  border-radius: 15px;
}

.card-header {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
}

.card-title span {
  white-space: nowrap;
  font-size: 16px;
  font-weight: 500;
}

.card-actions {
  width: 100%;
}

.history-card {
  width: 100%;
}

.history-content {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.top-section {
  display: flex;
  gap: 20px;
  align-items: flex-start;
}

.device-selector {
  flex: 1;
  min-width: 200px;
}

.device-selector h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.device-select {
  width: 100%;
}

.device-info {
  flex: 1;
  min-width: 300px;
}

.device-info h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.info-card {
  border-radius: 10px;
}

.info-item {
  margin-bottom: 10px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 5px 0;
}

.info-label {
  color: #909399;
  font-size: 14px;
}

.info-value {
  color: #303133;
  font-size: 14px;
  font-weight: 500;
}

.info-value.online {
  color: #67c23a;
}

.info-value.offline {
  color: #f56c6c;
}

.history-table {
  margin-top: 20px;
}

.history-table h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.history-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 300px;
}

@media (max-width: 768px) {
  .data-card {
    width: 95%;
    padding: 20px;
  }
  
  .realtime-container {
    flex-direction: column;
  }
  
  .data-left,
  .data-right {
    flex: none;
    width: 100%;
  }
  
  .video-container,
  .video-placeholder {
    height: 300px;
  }
}
</style>
