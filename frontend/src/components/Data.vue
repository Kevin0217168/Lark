<template>
  <!-- 桌面端界面 -->
  <div v-if="!isMobile" class="data-card">
    <div class="title">
      <h2>数据管理</h2>
    </div>
    
    <div class="data-content">
      <!-- 实时数据 -->
      <div v-if="activeTab === 'realtime'" class="realtime-container">
        <!-- 所有设备离线提示 -->
        <el-alert
          v-if="showAllOfflineAlert"
          title="所有设备均处于离线状态"
          type="warning"
          :closable="false"
          show-icon
          style="margin-bottom: 16px;"
        />
        
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
                <div class="data-cell">
                  <el-icon><Monitor /></el-icon>
                  <span class="cell-label">类型</span>
                  <span class="cell-value">{{ selectedDevice.device_type || '未设置' }}</span>
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
                <el-slider v-model="imageQuality" :min="1" :max="63" :step="1" :show-tooltip="false" style="width: 120px" />
              </div>
              <div class="quality-control">
                <span class="quality-label">视频尺寸</span>
                <el-select v-model="frameSize" style="width: 120px" size="small">
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
            <div class="video-container" v-if="selectedDevice && currentFrameImage">
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
      <div v-else-if="activeTab === 'analysis'" class="tab-content">
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
            <h3>所有设备平均值 <span class="time-range">(最近24小时)</span></h3>
            <div ref="averageChartRef" class="chart"></div>
          </div>
          <div class="chart-container">
            <h3>温度数据 <span class="time-range">(最近24小时)</span></h3>
            <div ref="temperatureChartRef" class="chart"></div>
          </div>
          <div class="chart-container">
            <h3>湿度数据 <span class="time-range">(最近24小时)</span></h3>
            <div ref="humidityChartRef" class="chart"></div>
          </div>

        </div>
      </div>
      
      <!-- 历史数据 -->
      <div v-else-if="activeTab === 'history'" class="tab-content">
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
                <span class="info-label">设备类型:</span>
                <span class="info-value">{{ selectedHistoryDevice?.device_type || '未设置' }}</span>
              </div>
              </div>
            </div>
          </div>
          
          <div class="history-table">
            <h3>历史数据记录</h3>
            <el-table :data="paginatedHistoryData" stripe style="width: 100%;" max-height="400" border>
              <el-table-column prop="deviceName" label="设备名称" min-width="150" align="left" />
              <el-table-column prop="deviceId" label="设备ID" min-width="100" align="center" />
              <el-table-column prop="device_type" label="设备类型" min-width="120" align="center" />
              <el-table-column prop="timestamp" label="时间" min-width="180" align="left" />
              <el-table-column prop="temperature" label="温度 (℃)" min-width="120" align="center" />
              <el-table-column prop="humidity" label="湿度 (%)" min-width="120" align="center" />
            </el-table>
            <div class="pagination-container">
              <el-pagination
                v-model:current-page="currentPage"
                v-model:page-size="pageSize"
                :page-sizes="[10, 20, 50, 100]"
                :total="historyDataList.length"
                layout="total, sizes, prev, pager, next, jumper"
                background
                :prev-text="'上一页'"
                :next-text="'下一页'"
                :total-text="'共 {total} 条'"
                :page-size-text="'条/页'"
                :jumper-text="'前往'"
                :jumper-prepend-text="''"
                :jumper-append-text="'页'"
                @size-change="handleSizeChange"
                @current-change="handleCurrentChange"
              />
            </div>
          </div>
        </div>
        <div class="history-placeholder" v-else>
          <el-empty description="请选择设备查看历史数据" />
        </div>
      </div>
    </div>
  </div>

  <!-- 移动端界面 -->
  <div v-else class="mobile-data">
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
      
      <!-- 所有设备离线提示 -->
      <el-alert
        v-if="showAllOfflineAlert"
        title="所有设备均处于离线状态"
        type="warning"
        :closable="false"
        show-icon
        style="margin-bottom: 16px;"
      />
      
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
            <div class="data-label">类型</div>
            <div class="data-value">{{ selectedDevice.device_type || '未设置' }}</div>
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
                :min="1" 
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
            <span class="info-label">设备类型</span>
            <span class="info-value">{{ selectedHistoryDevice.device_type || '未设置' }}</span>
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
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted, nextTick } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { useDeviceStore } from '../stores/deviceStore';
import { ElMessage, ElMessageBox } from 'element-plus';
import { Warning, Switch, Sort, Monitor, CircleCheck, CircleClose, Odometer, Clock, Refresh } from '@element-plus/icons-vue';
import { api } from '../utils/api';
import * as echarts from 'echarts';
import { shouldUseMobilePage } from '../utils/mobileAdapter';

// 获取登录token
const getToken = () => {
  return localStorage.getItem('accessToken');
};

const route = useRoute();
const router = useRouter();
const { getDevices, getDeviceHistoryData, getDeviceAverageData, fetchDevices, updateDevice, fetchDeviceHistoryData, getOrUpdateDeviceHistoryData, fetchSensorData, fetchRealtimeSensorData } = useDeviceStore();

// 直接获取设备数据
const devices = getDevices();

// 接收父组件传递的activeTab
const props = withDefaults(defineProps<{
  activeTab?: string
}>(), {
  activeTab: 'realtime'
});

// 计算当前激活的标签页
const activeTab = computed(() => props.activeTab || 'realtime');

// 移动端检测
const isMobile = ref(false);
const checkMobileStatus = () => {
  isMobile.value = shouldUseMobilePage();
};

const selectedDeviceId = ref<number | null>(null);
const showAllOfflineAlert = ref(false);

const selectedDevice = computed(() => {
  if (!selectedDeviceId.value) return null;
  // 每次计算时重新获取最新的设备列表
  const latestDevices = getDevices();
  const device = latestDevices.find((device: any) => device.id === selectedDeviceId.value) || null;
  return device;
});

// 计算WebSocket是否已连接
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

// 获取在线设备列表（按ID升序排序）
const getOnlineDevices = () => {
  // 重新获取最新的设备数据
  const currentDevices = getDevices();
  console.log('[getOnlineDevices] 总设备数:', currentDevices.length);
  console.log('[getOnlineDevices] 设备在线状态:', currentDevices.map(d => ({ id: d.id, name: d.name, isOnline: d.isOnline })));

  const onlineDevices = currentDevices
    .filter(device => device?.isOnline)
    .sort((a, b) => (a?.id || 0) - (b?.id || 0));

  console.log('[getOnlineDevices] 在线设备数:', onlineDevices.length);
  return onlineDevices;
};

// 自动选择第一个在线设备
const autoSelectFirstOnlineDevice = async () => {
  const onlineDevices = getOnlineDevices();

  if (onlineDevices.length > 0 && onlineDevices[0]) {
    // 有在线设备，选择第一个
    const firstDevice = onlineDevices[0];
    selectedDeviceId.value = firstDevice.id;
    showAllOfflineAlert.value = false;

    // 立即获取实时温湿度数据并启动定时器
    await fetchRealtimeData();
    startRealtimeDataTimer();

    console.log(`[自动选择] 已选择第一个在线设备: ${firstDevice.name} (ID: ${firstDevice.id})`);
  } else {
    // 没有在线设备
    selectedDeviceId.value = null;
    showAllOfflineAlert.value = true;
    console.log('[自动选择] 所有设备均处于离线状态');
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
      if (isMobile.value) {
        await getOrUpdateDeviceHistoryData(selectedDeviceId.value);
      } else {
        await fetchDeviceHistoryData(); // 不传参数，获取所有设备数据
      }
    }
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
  // 每次计算时重新获取最新的设备列表
  const latestDevices = getDevices();
  return latestDevices.find((device: any) => device.id === historyDeviceId.value) || null;
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
    if (isMobile.value) {
      await getOrUpdateDeviceHistoryData(historyDeviceId.value);
    } else {
      await fetchDeviceHistoryData(historyDeviceId.value);
    }
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
let lastFrameTimestamp = 0;
let previousFrameTimestamp = 0;
let streamCheckInterval: number | null = null;
let ws: WebSocket | null = null;
let currentImageUrl: string = '';

// 视频翻转控制
const flipHorizontal = ref(false);
const flipVertical = ref(false);
const imageQuality = ref(32); // 默认画质为32
const frameSize = ref('FRAMESIZE_VGA'); // 默认视频尺寸

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
      currentFps.value = Math.round(1000 / frameInterval);
    }
    previousFrameTimestamp = now;
    
    lastFrameTimestamp = now;
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
    
    // 建立WebSocket连接（新的连接模式：一个WebSocket对应一个设备）
    console.log('开始建立WebSocket连接');
    const wsUrl = `/api/stream/viewer/ws/${selectedDeviceId.value}?token=${token}`;
    console.log('WebSocket URL:', wsUrl.substring(0, 70) + '...');
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
      
      // 新的连接模式不需要单独发送请求开启设备推流
    };
    
    // 监听消息事件
    ws.onmessage = (event) => {
      try {
        // 检查是否是设备断开的文本消息
        if (typeof event.data === 'string' && event.data.includes('设备已断开')) {
          console.log('收到设备断开消息:', event.data);
          connectionError.value = '设备已断开';
          return;
        }
        
        // 尝试解析JSON数据
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
    const data = await api.post('/api/refresh');
    if (data.access_token) {
      // 保存新token
      localStorage.setItem('accessToken', data.access_token);
      console.log('Token刷新成功');
      return data.access_token;
    }
    console.log('Token刷新失败，响应数据格式不正确');
    return getToken();
  } catch (error) {
    console.error('刷新token失败:', error);
    return getToken();
  }
};

// 开启设备推流
const startDeviceStreaming = async () => {
  if (!selectedDeviceId.value) return;
  
  try {
    // 发送POST请求开启设备推流
    const data = await api.post(`/api/stream/viewer/following/${selectedDeviceId.value}`);
    
    // 延迟3秒再判断返回情况
    await new Promise(resolve => setTimeout(resolve, 4000));
    
    // 判断成功的条件：code为1 或 消息包含成功关键字
    const isSuccess = (data.code === 1 || 
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
    // 新的连接模式：WebSocket断开时会自动清理订阅关系
    ws.close();
    ws = null;
  }
  
  if (isMobile.value && currentImageUrl) {
    URL.revokeObjectURL(currentImageUrl);
    currentImageUrl = '';
  }
  
  if (isMobile.value) {
    currentFrameImage.value = '';
  }
  connectionError.value = '';
  isStreamDisconnected.value = false;
};

// 格式化时间标签，只显示小时和分钟（移动端专用）
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

// 采样函数：从数组中均匀采样指定数量的点（移动端专用）
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

// 窗口大小变化时调整图表
const handleResize = () => {
  temperatureChart?.resize();
  humidityChart?.resize();
  averageChart?.resize();
};

// 初始化图表
const initCharts = async () => {
  if (!selectedDeviceId.value) return;
  
  if (isMobile.value) {
    // 移动端图表初始化
    initMobileCharts();
  } else {
    // 桌面端图表初始化
    initDesktopCharts();
  }
};

// 桌面端图表初始化
const initDesktopCharts = async () => {
  // 获取传感器数据
  const sensorData = await fetchSensorData(selectedDeviceId.value!);
  
  // 温度图表
  if (temperatureChartRef.value) {
    if (temperatureChart) {
      temperatureChart.dispose();
    }
    temperatureChart = echarts.init(temperatureChartRef.value);
    // 计算温度数据的最小值和最大值
    const tempMin = Math.min(...sensorData.temperatureValues);
    const tempMax = Math.max(...sensorData.temperatureValues);
    // 调整纵轴范围，使数据更直观
    const tempRange = tempMax - tempMin;
    let tempYMin = tempMin;
    let tempYMax = tempMax;
    if (tempRange < 5) {
      tempYMin = Math.floor(tempMin - 1);
      tempYMax = Math.ceil(tempMax + 1);
    }
    temperatureChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      xAxis: {
        type: 'category',
        data: sensorData.times
      },
      yAxis: {
        type: 'value',
        name: '温度 (℃)',
        min: tempYMin,
        max: tempYMax
      },
      series: [{
        data: sensorData.temperatureValues,
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
    // 计算湿度数据的最小值和最大值
    const humidityMin = Math.min(...sensorData.humidityValues);
    const humidityMax = Math.max(...sensorData.humidityValues);
    // 调整纵轴范围，使数据更直观
    const humidityRange = humidityMax - humidityMin;
    let humidityYMin = humidityMin;
    let humidityYMax = humidityMax;
    if (humidityRange < 5) {
      humidityYMin = Math.floor(humidityMin - 1);
      humidityYMax = Math.ceil(humidityMax + 1);
    }
    humidityChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      xAxis: {
        type: 'category',
        data: sensorData.times
      },
      yAxis: {
        type: 'value',
        name: '湿度 (%)',
        min: humidityYMin,
        max: humidityYMax
      },
      series: [{
        data: sensorData.humidityValues,
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
        data: ['平均温度', '平均湿度']
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
        }
      ]
    });
  }
};

// 移动端图表初始化
const initMobileCharts = () => {
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

// 组件挂载时初始化
onMounted(async () => {
  // 初始化移动端检测
  checkMobileStatus();
  window.addEventListener('resize', handleResize);

  console.log('[Data.vue] Component mounted');
  console.log('[Data.vue] Initial active tab:', activeTab.value);
  console.log('[Data.vue] Is mobile:', isMobile.value);
  console.log('[Data.vue] Route query:', route.query);

  // 从后端获取设备数据
  console.log('[Data.vue] 开始获取设备数据...');
  await fetchDevices();

  const latestDevices = getDevices();
  console.log('[Data.vue] 设备数据获取完成，设备数量:', latestDevices.length);
  console.log('[Data.vue] 设备列表:', latestDevices.map(d => ({ id: d.id, name: d.name, isOnline: d.isOnline })));

  // 延迟1秒后执行自动选择，确保界面渲染完成
  setTimeout(async () => {
    // 重新获取最新的设备数据
    const currentDevices = getDevices();
    console.log('[自动选择] ========== 开始自动选择 ==========');
    console.log('[自动选择] 当前设备数量:', currentDevices.length);
    console.log('[自动选择] 当前标签页:', activeTab.value);
    console.log('[自动选择] 当前选中设备ID:', selectedDeviceId.value);
    console.log('[自动选择] 设备在线状态:', currentDevices.map(d => ({ id: d.id, name: d.name, isOnline: d.isOnline })));

    if (currentDevices.length > 0) {
      if (activeTab.value === 'realtime') {
        // 检查当前选择的设备是否在线
        const currentDevice = currentDevices.find((d: any) => d.id === selectedDeviceId.value);
        const isCurrentDeviceOnline = currentDevice?.isOnline ?? false;
        
        if (!selectedDeviceId.value || !isCurrentDeviceOnline) {
          console.log('[自动选择] 条件满足：realtime标签页且未选择设备或当前设备离线');
          console.log('[自动选择] - 是否已选择设备:', !!selectedDeviceId.value);
          console.log('[自动选择] - 当前设备是否在线:', isCurrentDeviceOnline);
          console.log('[自动选择] 执行自动选择第一个在线设备...');
          // 自动选择第一个在线设备
          await autoSelectFirstOnlineDevice();
          console.log('[自动选择] 自动选择完成，选中设备ID:', selectedDeviceId.value);
        } else {
          console.log('[自动选择] 当前设备已在线，无需自动选择');
          // 启动实时数据定时器
          await fetchRealtimeData();
          startRealtimeDataTimer();
        }
      } else if (activeTab.value === 'history' && !historyDeviceId.value) {
        console.log('[自动选择] 历史数据标签页，选择第一个设备');
        // 历史数据标签页，选择第一个设备
        historyDeviceId.value = currentDevices[0]?.id || null;
        // 获取历史数据
        if (historyDeviceId.value) {
          if (isMobile.value) {
            await getOrUpdateDeviceHistoryData(historyDeviceId.value);
          } else {
            await fetchDeviceHistoryData(historyDeviceId.value);
          }
        }
      } else if (activeTab.value === 'analysis') {
        console.log('[自动选择] 分析标签页，选择第一个设备');
        // 分析界面需要选择第一个设备并获取所有设备的历史数据
        if (!selectedDeviceId.value) {
          selectedDeviceId.value = currentDevices[0]?.id || null;
        }
        if (isMobile.value) {
          await getOrUpdateDeviceHistoryData(); // 不传参数，获取所有设备数据
        } else {
          await fetchDeviceHistoryData(); // 不传参数，获取所有设备数据
        }
      } else {
        console.log('[自动选择] 条件不满足，跳过自动选择');
        console.log('[自动选择] - 标签页是否为realtime:', activeTab.value === 'realtime');
        console.log('[自动选择] - 是否已选择设备:', !!selectedDeviceId.value);
      }
    } else {
      console.log('[自动选择] 没有可用设备，跳过自动选择');
    }
    console.log('[自动选择] ========== 自动选择结束 ==========');
  }, 1000);

  // 如果在分析标签页，初始化图表
  if (activeTab.value === 'analysis') {
    nextTick(async () => {
      await initCharts();
    });
  }
});

// 组件卸载时清理
onUnmounted(() => {
  window.removeEventListener('resize', handleResize);
  stopRealtimeMonitoring();
  stopRealtimeDataTimer();
  temperatureChart?.dispose();
  humidityChart?.dispose();
  averageChart?.dispose();
});

// 监听activeTab变化，重置设备选择
watch(() => props.activeTab, async (newTab, oldTab) => {
  console.log('Active tab changed:', newTab, 'Old tab:', oldTab);
  console.log('Devices available:', devices.length);
  console.log('Selected device ID:', selectedDeviceId.value);
  
  if (newTab === 'analysis') {
    // 切换到分析标签时，获取所有设备的历史数据
    if (isMobile.value) {
      await getOrUpdateDeviceHistoryData(); // 不传参数，获取所有设备数据
    } else {
      await fetchDeviceHistoryData(); // 不传参数，获取所有设备数据
    }
    nextTick(async () => {
      await initCharts();
    });
    // 停止实时监控和实时数据定时器
    stopRealtimeMonitoring();
    stopRealtimeDataTimer();
  } else if (newTab === 'history') {
    // 切换到历史数据标签时，停止实时数据定时器
    stopRealtimeDataTimer();
    // 重新获取设备数据
    fetchDevices().then(() => {
      // 确保有设备被选择
      const latestDevices = getDevices();
      if (latestDevices.length > 0) {
        // 不管之前是否有选择，都强制选择第一个设备
        const deviceId = latestDevices[0]?.id || null;
        historyDeviceId.value = deviceId;
        
        // 获取该设备的历史数据
        if (deviceId) {
          console.log('Auto-selecting device for history:', deviceId);
          // 使用nextTick确保historyDeviceId已更新
          nextTick(async () => {
            if (isMobile.value) {
              await getOrUpdateDeviceHistoryData(deviceId);
            } else {
              await fetchDeviceHistoryData(deviceId);
            }
            console.log('History data fetched for device:', deviceId);
          });
        }
      } else {
        console.log('No devices available, cannot load history data');
      }
    });
    // 停止实时监控
    stopRealtimeMonitoring();
  } else if (newTab === 'realtime') {
    // 切换到实时数据标签时，重新获取设备数据
    fetchDevices().then(async () => {
      const latestDevices = getDevices();
      const currentDevice = latestDevices.find((d: any) => d.id === selectedDeviceId.value);
      const isCurrentDeviceOnline = currentDevice?.isOnline ?? false;
      
      // 只有在没有选中设备，或当前设备不在线时，才执行自动选择
      if (!selectedDeviceId.value || !isCurrentDeviceOnline) {
        console.log('[watch activeTab] 切换到realtime标签，当前设备离线或未选择，执行自动选择');
        console.log('[watch activeTab] - 是否已选择设备:', !!selectedDeviceId.value);
        console.log('[watch activeTab] - 当前设备是否在线:', isCurrentDeviceOnline);
        await autoSelectFirstOnlineDevice();
      } else {
        console.log('[watch activeTab] 切换到realtime标签，当前设备已在线，保持当前选择');
        // 启动实时数据定时器
        await fetchRealtimeData();
        startRealtimeDataTimer();
      }
      
      // 如果成功选择了设备，启动实时监控
      if (selectedDeviceId.value) {
        console.log('Auto-starting realtime monitoring for device:', selectedDeviceId.value);
        // 使用nextTick确保selectedDeviceId已更新
        nextTick(() => {
          startRealtimeMonitoring();
        });
      } else {
        console.log('No online devices available, cannot start realtime monitoring');
      }
    });
  } else {
    selectedDeviceId.value = null;
    historyDeviceId.value = null;
    // 停止实时监控
    stopRealtimeMonitoring();
  }
}, { immediate: true });

// 监听selectedDeviceId变化，更新设备数据
watch(() => selectedDeviceId.value, async (newDeviceId, oldDeviceId) => {
  console.log('Selected device ID changed:', newDeviceId, 'Old device ID:', oldDeviceId);
  if (newDeviceId && activeTab.value === 'realtime') {
    // 切换设备时，更新设备数据
    console.log('Updating device data for new device:', newDeviceId);
    // 从后端获取最新的设备数据
    await fetchDevices();
    if (!isMobile.value) {
      // 获取最新的温度湿度数据（仅桌面端）
      const sensorData = await fetchSensorData(newDeviceId);
      if (sensorData.temperatureValues.length > 0 && sensorData.humidityValues.length > 0) {
        const latestTemp = sensorData.temperatureValues[sensorData.temperatureValues.length - 1];
        const latestHumidity = sensorData.humidityValues[sensorData.humidityValues.length - 1];
        updateDevice(newDeviceId, { temperature: latestTemp, humidity: latestHumidity });
        console.log('Latest sensor data stored to deviceStore:', { temperature: latestTemp, humidity: latestHumidity });
      }
    }
    console.log('Device data updated for new device');
  }
});

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
watch(() => route.path, async (newPath) => {
  console.log('[路由监听] Route path changed:', newPath);
  if (newPath === '/Data' || newPath === '/Stream') {
    console.log('[路由监听] Navigated to data page, checking device selection');
    // 延迟执行，确保设备数据已加载
    setTimeout(async () => {
      const latestDevices = getDevices();
      console.log('[路由监听] 设备数量:', latestDevices.length);
      console.log('[路由监听] 当前选中设备:', selectedDeviceId.value);
      console.log('[路由监听] 当前标签页:', activeTab.value);

      if (latestDevices.length > 0) {
        if (activeTab.value === 'realtime') {
          // 检查当前选择的设备是否在线
          const currentDevice = latestDevices.find((d: any) => d.id === selectedDeviceId.value);
          const isCurrentDeviceOnline = currentDevice?.isOnline ?? false;
          
          // 如果没有选中设备，或当前设备不在线，执行自动选择
          if (!selectedDeviceId.value || !isCurrentDeviceOnline) {
            console.log('[路由监听] 执行自动选择第一个在线设备');
            console.log('[路由监听] - 是否已选择设备:', !!selectedDeviceId.value);
            console.log('[路由监听] - 当前设备是否在线:', isCurrentDeviceOnline);
            await autoSelectFirstOnlineDevice();
          } else {
            console.log('[路由监听] 当前设备已在线，无需自动选择');
          }
        } else if (!selectedDeviceId.value) {
          console.log('[路由监听] 非实时监控标签页，选择第一个设备:', latestDevices[0]?.id);
          selectedDeviceId.value = latestDevices[0]?.id || null;
        }
      }
    }, 500);
  }
});

// 监听selectedDeviceId变化，自动开启实时监控
watch(() => selectedDeviceId.value, (newDeviceId, oldDeviceId) => {
  console.log('Selected device ID changed:', newDeviceId, 'Old device ID:', oldDeviceId);
  console.log('Current active tab:', activeTab.value);
  
  // 使用nextTick确保activeTab已经更新
  nextTick(() => {
    console.log('Active tab after nextTick:', activeTab.value);
    // 只有当有设备ID且当前是实时标签时才启动监控
    if (newDeviceId && activeTab.value === 'realtime') {
      console.log('Auto-starting realtime monitoring for device:', newDeviceId);
      startRealtimeMonitoring();
    }
  });
});

// 跳转到总览界面的大图模式全屏状态
const goToFullscreen = () => {
  router.push({
    path: '/Home',
    query: { fullscreen: 'true' }
  });
};
</script>

<style lang="scss" scoped>
/* ==================== 桌面端样式 ==================== */
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

.chart-container .time-range {
  font-size: 12px;
  color: #909399;
  font-weight: normal;
  margin-left: 8px;
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
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.fps-info {
  font-weight: bold;
  color: #409EFF;
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

.pagination-container {
  margin-top: 20px;
  display: flex;
  justify-content: center;
}

.history-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 300px;
}

/* ==================== 移动端样式 ==================== */
.mobile-data {
  min-height: calc(100vh - 120px);
  background: #f5f7fa;
  display: flex;
  flex-direction: column;
}

/* 设备选择 */
.mobile-data .device-selector {
  margin-bottom: 16px;
}

/* 实时数据 */
.mobile-data .realtime-container {
  padding: 0;
}

/* 设备数据卡片 */
.mobile-data .device-data-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  margin-bottom: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.mobile-data .data-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.mobile-data .data-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.mobile-data .data-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.mobile-data .data-item {
  padding: 12px;
  background: #f5f7fa;
  border-radius: 8px;
}

.mobile-data .data-item.full {
  grid-column: 1 / -1;
}

.mobile-data .data-label {
  font-size: 12px;
  color: #909399;
  margin-bottom: 4px;
}

.mobile-data .data-value {
  font-size: 18px;
  font-weight: 600;
  color: #303133;
}

.mobile-data .data-value.small {
  font-size: 14px;
}

/* 视频监控 */
.mobile-data .video-section {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.mobile-data .video-header {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 12px;
  margin-bottom: 12px;
  width: 100%;
}

.mobile-data .video-header-top {
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
}

.mobile-data .video-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.mobile-data .video-controls {
  display: flex;
  gap: 8px;
}

.mobile-data .video-settings {
  width: 100%;
  display: flex;
  flex-direction: column;
  gap: 12px;
  padding: 12px;
  background: #f5f7fa;
  border-radius: 8px;
}

.mobile-data .setting-item {
  display: flex;
  align-items: center;
  gap: 8px;
}

.mobile-data .setting-label {
  font-size: 14px;
  color: #606266;
  min-width: 60px;
}

.mobile-data .setting-value {
  font-size: 14px;
  color: #409EFF;
  font-weight: 600;
  min-width: 30px;
  text-align: right;
}

.mobile-data .video-card {
  position: relative;
  width: 100%;
  background: #000;
  border-radius: 8px;
  overflow: hidden;
}

.mobile-data .video-container {
  position: relative;
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #000;
}

.mobile-data .video-stream {
  max-width: 100%;
  max-height: 70vh;
  width: auto;
  height: auto;
  object-fit: contain;
  display: block;
}

.mobile-data .video-stream.flip-horizontal {
  transform: scaleX(-1);
}

.mobile-data .video-stream.flip-vertical {
  transform: scaleY(-1);
}

.mobile-data .video-stream.disconnected {
  opacity: 0.5;
}

.mobile-data .frame-info {
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

.mobile-data .fps-info {
  font-weight: bold;
  color: #409EFF;
}

.mobile-data .stream-disconnected-overlay {
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

.mobile-data .disconnected-content {
  text-align: center;
  color: white;
}

.mobile-data .disconnected-icon {
  color: #f56c6c;
}

.mobile-data .disconnected-message {
  margin-top: 8px;
  font-size: 14px;
}

.mobile-data .video-error {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
  background: #f5f7fa;
}

.mobile-data .error-content {
  text-align: center;
}

.mobile-data .error-icon {
  color: #f56c6c;
}

.mobile-data .error-message {
  margin-top: 8px;
  color: #606266;
}

.mobile-data .video-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
}

/* 分析 */
.mobile-data .analysis-container {
  padding: 0;
}

.mobile-data .chart-section {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.mobile-data .chart-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.mobile-data .chart-card h3 {
  margin: 0 0 12px 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.mobile-data .chart-card h3 .time-range {
  font-size: 12px;
  font-weight: 400;
  color: #909399;
  margin-left: 8px;
}

.mobile-data .chart {
  width: 100%;
  height: 200px;
}

/* 历史数据 */
.mobile-data .history-container {
  padding: 0;
}

.mobile-data .history-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.mobile-data .device-info-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.mobile-data .device-info-card .info-item {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.mobile-data .device-info-card .info-label {
  font-size: 12px;
  color: #909399;
}

.mobile-data .device-info-card .info-value {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
}

.mobile-data .history-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.mobile-data .history-item {
  background: white;
  border-radius: 8px;
  padding: 10px 12px;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.06);
}

.mobile-data .history-row {
  display: flex;
  justify-content: space-between;
  align-items: center;
  gap: 6px;
}

.mobile-data .history-row .device-name {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
  flex-shrink: 0;
}

.mobile-data .history-row .device-id {
  font-size: 12px;
  color: #909399;
  flex-shrink: 0;
}

.mobile-data .history-row .history-time {
  font-size: 12px;
  color: #909399;
  margin-left: auto;
}

.mobile-data .history-row.data-row {
  margin-top: 6px;
  padding-top: 6px;
  border-top: 1px solid #f0f0f0;
}

.mobile-data .history-row .data-item {
  font-size: 14px;
  color: #303133;
}

.mobile-data .history-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.mobile-data .pagination-container {
  margin-top: 16px;
  display: flex;
  justify-content: center;
  padding: 0 4px;
  overflow-x: auto;
}

.mobile-data .pagination-container :deep(.el-pagination) {
  --el-pagination-button-width: 28px;
  --el-pagination-button-height: 28px;
  font-size: 12px;
}

.mobile-data .pagination-container :deep(.el-pagination .el-select .el-input) {
  width: 80px;
}

.mobile-data .history-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
}

/* ==================== 响应式适配 ==================== */
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
