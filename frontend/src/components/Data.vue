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
          
          <!-- 环境数据环形进度条 -->
          <div class="env-gauges" v-if="selectedDevice">
            <h3>环境数据</h3>
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
            <div class="gauges-row">
              <div class="gauge-item">
                <div ref="luxGaugeRef" class="gauge-chart"></div>
                <div class="gauge-label">光照强度</div>
              </div>
              <div class="gauge-item">
                <div ref="uvGaugeRef" class="gauge-chart"></div>
                <div class="gauge-label">紫外线指数</div>
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

        <!-- 时间范围选择器 -->
        <div class="time-range-selector">
          <div class="time-buttons-container">
            <button
              v-for="label in timeRangeLabels"
              :key="label.value"
              class="time-button"
              :class="{ active: timeRange === label.value }"
              @click="setTimeRange(label.value)"
            >
              {{ label.text }}
            </button>
          </div>
        </div>

        <div class="analysis-content">
          <!-- 所有设备平均值图表 -->
          <div class="chart-container average-chart">
            <h3>所有设备平均值 <span class="time-range">({{ timeRangeText }})</span></h3>
            <div ref="averageChartRef" class="chart"></div>
          </div>
          <div class="chart-container">
            <h3>温度数据 <span class="time-range">({{ timeRangeText }})</span></h3>
            <div ref="temperatureChartRef" class="chart"></div>
          </div>
          <div class="chart-container">
            <h3>湿度数据 <span class="time-range">({{ timeRangeText }})</span></h3>
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
      
      <!-- 环境数据环形进度条（移动端） -->
      <div v-if="selectedDevice" class="mobile-gauges">
        <div class="mobile-gauges-row">
          <div class="mobile-gauge-item">
             <div ref="aqiGaugeRefMobile" class="mobile-gauge-chart"></div>
             <div class="mobile-gauge-label">空气质量</div>
           </div>
           <div class="mobile-gauge-item">
             <div ref="dbGaugeRefMobile" class="mobile-gauge-chart"></div>
             <div class="mobile-gauge-label">声音分贝</div>
           </div>
        </div>
        <div class="mobile-gauges-row mobile-gauges-row-second">
          <div class="mobile-gauge-item">
             <div ref="luxGaugeRefMobile" class="mobile-gauge-chart"></div>
             <div class="mobile-gauge-label">光照强度</div>
           </div>
           <div class="mobile-gauge-item">
             <div ref="uvGaugeRefMobile" class="mobile-gauge-chart"></div>
             <div class="mobile-gauge-label">紫外线指数</div>
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

      <!-- 时间轴筛选器 -->
      <div class="time-range-selector mobile">
        <div class="time-slider-container">
          <div class="time-labels">
            <span
              v-for="label in timeRangeLabels"
              :key="label.value"
              class="time-label"
              :class="{ active: timeRange === label.value }"
              @click="setTimeRange(label.value)"
            >
              {{ label.text }}
            </span>
          </div>
          <div class="slider-track">
            <div
              class="slider-progress"
              :style="{ width: getTimeRangeProgress() + '%' }"
            ></div>
            <div
              class="slider-thumb"
              :style="{ left: getTimeRangeProgress() + '%' }"
            ></div>
          </div>
        </div>
      </div>

      <!-- 图表 -->
      <div class="chart-section">
        <div class="chart-card">
          <h3>所有设备平均值 <span class="time-range">({{ timeRangeText }})</span></h3>
          <div ref="averageChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>温度数据 <span class="time-range">({{ timeRangeText }})</span></h3>
          <div ref="temperatureChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>湿度数据 <span class="time-range">({{ timeRangeText }})</span></h3>
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
  }
};

const fetchLuxData = async () => {
  if (!selectedDeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: selectedDeviceId.value,
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
  if (!selectedDeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: selectedDeviceId.value,
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
  const currentDevices = getDevices();

  const onlineDevices = currentDevices
    .filter(device => device?.isOnline)
    .sort((a, b) => (a?.id || 0) - (b?.id || 0));

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
    startEnvDataTimer();
    nextTick(() => {
      initGaugeCharts();
    });
  } else {
    selectedDeviceId.value = null;
    showAllOfflineAlert.value = true;
  }
};

const handleDeviceChange = async () => {
  if (selectedDevice.value) {
    ElMessage.success(`已切换到设备: ${selectedDevice.value.name}`);
    
    // 如果在实时界面，立即获取实时温湿度数据并启动定时器
    if (activeTab.value === 'realtime') {
      await fetchRealtimeData();
      startRealtimeDataTimer();
      startEnvDataTimer();
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
      initGaugeCharts();
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
    if (isMobile.value) {
      await getOrUpdateDeviceHistoryData(historyDeviceId.value);
    } else {
      await fetchDeviceHistoryData(historyDeviceId.value);
    }
    currentPage.value = 1;
  }
};

// 图表容器引用
const temperatureChartRef = ref<HTMLElement | null>(null);
const humidityChartRef = ref<HTMLElement | null>(null);
const averageChartRef = ref<HTMLElement | null>(null);

// 时间范围状态
const timeRange = ref<'today' | 'two_days'>('today');
const timeRangeLabels = [
  { text: '今日', value: 'today' as const },
  { text: '两天', value: 'two_days' as const }
];

// 获取时间范围显示文本
const timeRangeText = computed(() => {
  return timeRange.value === 'today' ? '今日' : '两天';
});

// 获取时间范围进度百分比
const getTimeRangeProgress = () => {
  const index = timeRangeLabels.findIndex(l => l.value === timeRange.value);
  return (index / (timeRangeLabels.length - 1)) * 100;
};

// 设置时间范围
const setTimeRange = (range: 'today' | 'two_days') => {
  timeRange.value = range;
  refreshChartsForTimeRange();
};

// 将 HH:MM 格式的时间字符串转换为从当天0点开始的分钟数
const timeToMinutes = (timeStr: string): number => {
  const parts = timeStr.split(':');
  if (parts.length !== 2) return 0;
  const hoursStr = parts[0];
  const minutesStr = parts[1];
  if (!hoursStr || !minutesStr) return 0;
  const hours = parseInt(hoursStr, 10);
  const minutes = parseInt(minutesStr, 10);
  if (isNaN(hours) || isNaN(minutes)) return 0;
  return hours * 60 + minutes;
};

// 根据时间范围刷新图表
const refreshChartsForTimeRange = async () => {
  if (!selectedDeviceId.value) return;

  if (isMobile.value) {
    initMobileCharts();
  } else {
    initDesktopCharts();
  }
};

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

// 帧率计算相关
const fpsHistory: number[] = [];
let fpsUpdateInterval: number | null = null;
const MAX_FPS = 60; // 最大合理帧率
const MIN_FPS = 1; // 最小合理帧率
const SMOOTHING_FACTOR = 0.7; // 平滑因子，值越大平滑效果越好，但响应越慢
let smoothedFps = 0;

// 计算0.5秒内的平均帧率（使用平滑算法）
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

// 视频翻转控制
const flipHorizontal = ref(false);
const flipVertical = ref(false);
const imageQuality = ref(32); // 默认画质为32
const frameSize = ref('FRAMESIZE_VGA'); // 默认视频尺寸

// 环境数据环形进度条
const aqiValue = ref(75);
const dbValue = ref(45);
const aqiGaugeRef = ref<HTMLElement | null>(null);
const dbGaugeRef = ref<HTMLElement | null>(null);
const aqiGaugeRefMobile = ref<HTMLElement | null>(null);
const dbGaugeRefMobile = ref<HTMLElement | null>(null);
const luxGaugeRef = ref<HTMLElement | null>(null);
const uvGaugeRef = ref<HTMLElement | null>(null);
const luxGaugeRefMobile = ref<HTMLElement | null>(null);
const uvGaugeRefMobile = ref<HTMLElement | null>(null);
let aqiGaugeChart: echarts.ECharts | null = null;
let dbGaugeChart: echarts.ECharts | null = null;
let aqiGaugeChartMobile: echarts.ECharts | null = null;
let dbGaugeChartMobile: echarts.ECharts | null = null;
let luxGaugeChart: echarts.ECharts | null = null;
let uvGaugeChart: echarts.ECharts | null = null;
let luxGaugeChartMobile: echarts.ECharts | null = null;
let uvGaugeChartMobile: echarts.ECharts | null = null;

// 环境数据采集定时器
let envDataTimer: number | null = null;

// AQI 计算：根据 PM2.5、PM10、CO2、TVOC 综合计算空气质量指数
const calculateAQI = (pm25: number, pm10: number, co2: number, tvoc: number): number => {
  // PM2.5 断点 (浓度下限, 浓度上限, AQI下限, AQI上限)
  const pm25Breakpoints: [number, number, number, number][] = [
    [0, 35, 0, 50],
    [36, 75, 51, 100],
    [76, 115, 101, 150],
    [116, 150, 151, 200],
    [151, 250, 201, 300],
    [251, 500, 301, 500],
  ];
  // PM10 断点
  const pm10Breakpoints: [number, number, number, number][] = [
    [0, 50, 0, 50],
    [51, 150, 51, 100],
    [151, 250, 101, 150],
    [251, 350, 151, 200],
    [351, 420, 201, 300],
    [421, 600, 301, 500],
  ];
  // CO2 断点 (ppm) — 室内空气质量参考
  const co2Breakpoints: [number, number, number, number][] = [
    [0, 400, 0, 50],
    [401, 600, 51, 100],
    [601, 1000, 101, 150],
    [1001, 1400, 151, 200],
    [1401, 2000, 201, 300],
    [2001, 5000, 301, 500],
  ];
  // TVOC 断点 (ppb)
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

  // 计算各分指数（缺失传感器时返回 -1 表示忽略）
  const pm25Aqi = pm25 > 0 ? calc(pm25, pm25Breakpoints) : -1;
  const pm10Aqi = pm10 > 0 ? calc(pm10, pm10Breakpoints) : -1;
  const co2Aqi = co2 > 0 ? calc(co2, co2Breakpoints) : -1;
  const tvocAqi = tvoc > 0 ? calc(tvoc, tvocBreakpoints) : -1;

  // 取有效分指数中的最大值（容错：至少需要1个传感器有效）
  const validAqi = [pm25Aqi, pm10Aqi, co2Aqi, tvocAqi].filter(v => v >= 0);
  return validAqi.length > 0 ? Math.round(Math.max(...validAqi)) : 0;
};

// ─── 5分钟滑动平均 ───
// 5分钟 = 300秒，每5秒采样一次 → 需要60个数据点
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
  if (!selectedDeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: selectedDeviceId.value,
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
  if (!selectedDeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: selectedDeviceId.value,
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
  if (!selectedDeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: selectedDeviceId.value,
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

const fetchAllEnvData = () => {
  fetchAirQualityData();
  fetchSGP30Data();
  fetchSoundData();
  fetchLuxData();
  fetchUVData();
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

// 初始化环形进度条
const initGaugeCharts = () => {
  if (isMobile.value) {
    if (aqiGaugeRefMobile.value) {
      if (aqiGaugeChartMobile) aqiGaugeChartMobile.dispose();
      aqiGaugeChartMobile = echarts.init(aqiGaugeRefMobile.value);
    }
    if (dbGaugeRefMobile.value) {
      if (dbGaugeChartMobile) dbGaugeChartMobile.dispose();
      dbGaugeChartMobile = echarts.init(dbGaugeRefMobile.value);
    }
    if (luxGaugeRefMobile.value) {
      if (luxGaugeChartMobile) luxGaugeChartMobile.dispose();
      luxGaugeChartMobile = echarts.init(luxGaugeRefMobile.value);
    }
    if (uvGaugeRefMobile.value) {
      if (uvGaugeChartMobile) uvGaugeChartMobile.dispose();
      uvGaugeChartMobile = echarts.init(uvGaugeRefMobile.value);
    }
  } else {
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
  }
  initGaugeChartOption();
  updateGaugeCharts();
};

const getActiveGaugeCharts = () => {
  if (isMobile.value) {
    return {
      aqi: aqiGaugeChartMobile, db: dbGaugeChartMobile,
      lux: luxGaugeChartMobile, uv: uvGaugeChartMobile
    };
  }
  return {
    aqi: aqiGaugeChart, db: dbGaugeChart,
    lux: luxGaugeChart, uv: uvGaugeChart
  };
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

const initGaugeChartOption = () => {
  const aqiColor = aqiValue.value > 150 ? '#f56c6c' : aqiValue.value > 100 ? '#e6a23c' : '#67c23a';
  const dbColor = dbValue.value > 85 ? '#f56c6c' : dbValue.value > 65 ? '#e6a23c' : '#67c23a';
  const aqiBg = aqiValue.value > 150 ? '#f56c6c1a' : aqiValue.value > 100 ? '#e6a23c1a' : '#67c23a1a';
  const dbBg = dbValue.value > 85 ? '#f56c6c1a' : dbValue.value > 65 ? '#e6a23c1a' : '#67c23a1a';
  const luxVal = slidingAvg(luxHistory);
  const luxColor = getLuxColor(luxVal);
  const luxBg = getLuxBg(luxVal);
  const uvVal = slidingAvg(uvHistory);
  const uvColor = getUVColor(uvVal);
  const uvBg = getUVBg(uvVal);
  const { aqi, db, lux, uv } = getActiveGaugeCharts();

  if (aqi) {
    aqi.setOption(getGaugeOption(300, aqiColor, aqiBg, aqiValue.value, 'AQI'));
  }
  if (db) {
    db.setOption(getGaugeOption(120, dbColor, dbBg, dbValue.value, 'dB'));
  }
  if (lux) {
    lux.setOption(getGaugeOption(100000, luxColor, luxBg, luxVal, 'Lux', formatLux));
  }
  if (uv) {
    uv.setOption(getGaugeOption(12, uvColor, uvBg, uvVal, 'UV'));
  }
};

const updateGaugeCharts = () => {
  const aqiColor = aqiValue.value > 150 ? '#f56c6c' : aqiValue.value > 100 ? '#e6a23c' : '#67c23a';
  const dbColor = dbValue.value > 85 ? '#f56c6c' : dbValue.value > 65 ? '#e6a23c' : '#67c23a';
  const aqiBg = aqiValue.value > 150 ? '#f56c6c1a' : aqiValue.value > 100 ? '#e6a23c1a' : '#67c23a1a';
  const dbBg = dbValue.value > 85 ? '#f56c6c1a' : dbValue.value > 65 ? '#e6a23c1a' : '#67c23a1a';
  const luxVal = slidingAvg(luxHistory);
  const luxColor = getLuxColor(luxVal);
  const luxBg = getLuxBg(luxVal);
  const uvVal = slidingAvg(uvHistory);
  const uvColor = getUVColor(uvVal);
  const uvBg = getUVBg(uvVal);
  const { aqi, db, lux, uv } = getActiveGaugeCharts();

  if (aqi) {
    aqi.setOption(getGaugeOption(300, aqiColor, aqiBg, aqiValue.value, 'AQI'));
  }
  if (db) {
    db.setOption(getGaugeOption(120, dbColor, dbBg, dbValue.value, 'dB'));
  }
  if (lux) {
    lux.setOption(getGaugeOption(100000, luxColor, luxBg, luxVal, 'Lux', formatLux));
  }
  if (uv) {
    uv.setOption(getGaugeOption(12, uvColor, uvBg, uvVal, 'UV'));
  }
};

const updateLuxGauge = () => {
  const luxVal = slidingAvg(luxHistory);
  const luxColor = getLuxColor(luxVal);
  const luxBg = getLuxBg(luxVal);
  const { lux } = getActiveGaugeCharts();
  if (lux) {
    lux.setOption(getGaugeOption(100000, luxColor, luxBg, luxVal, 'Lux', formatLux));
  }
};

const updateUVGauge = () => {
  const uvVal = slidingAvg(uvHistory);
  const uvColor = getUVColor(uvVal);
  const uvBg = getUVBg(uvVal);
  const { uv } = getActiveGaugeCharts();
  if (uv) {
    uv.setOption(getGaugeOption(12, uvColor, uvBg, uvVal, 'UV'));
  }
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
          // 只保留最近30帧的数据，避免数组过大且能快速响应变化
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
  stopRealtimeMonitoring();
  
  if (!selectedDeviceId.value) {
    return;
  }
  
  try {
    let token = await refreshToken();
    if (!token) {
      connectionError.value = '未登录，请先登录';
      return;
    }
    
    const wsUrl = `/api/stream/viewer/ws/${selectedDeviceId.value}?token=${token}`;
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
        startRealtimeMonitoring();
      })
      .catch(() => {
      });
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

// 开启设备推流
const startDeviceStreaming = async () => {
  if (!selectedDeviceId.value) return;
  
  try {
    const data = await api.post(`/api/stream/viewer/following/${selectedDeviceId.value}`);
    
    await new Promise(resolve => setTimeout(resolve, 4000));
    
    const isSuccess = (data.code === 1 || 
      (data.msg && (data.msg.includes('OK') || data.msg.includes('success') || data.msg.includes('成功'))));
    
    if (isSuccess) {
      connectionError.value = '';
    } else if (data.msg && data.msg.toLowerCase().includes('device has subscribed')) {
      connectionError.value = '';
    } else if (data.msg && data.msg.toLowerCase().includes('viewer has not registerd')) {
      connectionError.value = '';
    } else {
      connectionError.value = '设备推流开启失败';
    }
  } catch (error) {
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
  
  // 清空帧率历史数据
  fpsHistory.length = 0;
  currentFps.value = 0;
  smoothedFps = 0;
  previousFrameTimestamp = 0;
  
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
  const sensorData = await fetchSensorData(selectedDeviceId.value!, timeRange.value);
  
  // 获取时间范围
  const now = new Date();
  const nowMinutes = now.getHours() * 60 + now.getMinutes();

  // 过滤数据 - fetchSensorData返回HH:MM格式的时间
  // 今日：显示从00:00到当前时刻的数据
  // 两天：显示所有48小时的数据
  const filteredTimes = [];
  const filteredTemperatureValues = [];
  const filteredHumidityValues = [];

  for (let i = 0; i < sensorData.times.length; i++) {
    const timeStr = sensorData.times[i];
    if (timeStr) {
      const timeMinutes = timeToMinutes(timeStr);

      if (timeRange.value === 'today') {
        // 今日：只显示00:00到当前时刻的数据
        if (timeMinutes <= nowMinutes) {
          filteredTimes.push(timeStr);
          filteredTemperatureValues.push(sensorData.temperatureValues[i]);
          filteredHumidityValues.push(sensorData.humidityValues[i]);
        }
      } else {
        // 两天：显示所有数据
        filteredTimes.push(timeStr);
        filteredTemperatureValues.push(sensorData.temperatureValues[i]);
        filteredHumidityValues.push(sensorData.humidityValues[i]);
      }
    }
  }
  
  // 温度图表
  if (temperatureChartRef.value) {
    if (temperatureChart) {
      temperatureChart.dispose();
    }
    temperatureChart = echarts.init(temperatureChartRef.value);
    // 计算温度数据的最小值和最大值
    const validTempValues = filteredTemperatureValues.filter(val => val !== undefined && val !== null) as number[];
    const tempMin = validTempValues.length > 0 ? Math.min(...validTempValues) : 0;
    const tempMax = validTempValues.length > 0 ? Math.max(...validTempValues) : 100;
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
        data: filteredTimes
      },
      yAxis: {
        type: 'value',
        name: '温度 (℃)',
        min: tempYMin,
        max: tempYMax
      },
      series: [{
        data: filteredTemperatureValues,
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
    const validHumidityValues = filteredHumidityValues.filter(val => val !== undefined && val !== null) as number[];
    const humidityMin = validHumidityValues.length > 0 ? Math.min(...validHumidityValues) : 0;
    const humidityMax = validHumidityValues.length > 0 ? Math.max(...validHumidityValues) : 100;
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
        data: filteredTimes
      },
      yAxis: {
        type: 'value',
        name: '湿度 (%)',
        min: humidityYMin,
        max: humidityYMax
      },
      series: [{
        data: filteredHumidityValues,
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
    
    // 过滤平均数据
    const filteredAvgTimes = [];
    const filteredAvgTempValues = [];
    const filteredAvgHumidityValues = [];
    
    for (let i = 0; i < avgData.times.length; i++) {
      const timeStr = avgData.times[i];
      if (timeStr) {
        const timeMinutes = timeToMinutes(timeStr);
        if (timeRange.value === 'today') {
          if (timeMinutes <= nowMinutes) {
            filteredAvgTimes.push(timeStr);
            filteredAvgTempValues.push(avgData.temperatureValues[i]);
            filteredAvgHumidityValues.push(avgData.humidityValues[i]);
          }
        } else {
          filteredAvgTimes.push(timeStr);
          filteredAvgTempValues.push(avgData.temperatureValues[i]);
          filteredAvgHumidityValues.push(avgData.humidityValues[i]);
        }
      }
    }

    averageChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      legend: {
        data: ['平均温度', '平均湿度']
      },
      xAxis: {
        type: 'category',
        data: filteredAvgTimes
      },
      yAxis: {
        type: 'value',
        name: '数值'
      },
      series: [
        {
          name: '平均温度',
          data: filteredAvgTempValues,
          type: 'line',
          smooth: true,
          itemStyle: {
            color: '#ff7875'
          }
        },
        {
          name: '平均湿度',
          data: filteredAvgHumidityValues,
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

  // 获取当前时间分钟数
  const now = new Date();
  const nowMinutes = now.getHours() * 60 + now.getMinutes();

  // 初始化平均图表
  if (averageChartRef.value) {
    averageChart = echarts.init(averageChartRef.value);
    const avgData = getDeviceAverageData();

    // 过滤平均数据
    const filteredAvgTimes = [];
    const filteredAvgTempValues = [];
    const filteredAvgHumidityValues = [];

    for (let i = 0; i < avgData.times.length; i++) {
      const timeStr = avgData.times[i];
      if (timeStr) {
        const timeMinutes = timeToMinutes(timeStr);
        if (timeRange.value === 'today') {
          if (timeMinutes <= nowMinutes) {
            filteredAvgTimes.push(timeStr);
            filteredAvgTempValues.push(avgData.temperatureValues[i]);
            filteredAvgHumidityValues.push(avgData.humidityValues[i]);
          }
        } else {
          filteredAvgTimes.push(timeStr);
          filteredAvgTempValues.push(avgData.temperatureValues[i]);
          filteredAvgHumidityValues.push(avgData.humidityValues[i]);
        }
      }
    }
    
    // 采样12个点显示
    const sampledTimes = sampleData(filteredAvgTimes, 12);
    const sampledTempValues = sampleData(filteredAvgTempValues, 12);
    const sampledHumidityValues = sampleData(filteredAvgHumidityValues, 12);
    
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

    // 过滤数据 - historyData的timestamp是完整日期字符串，可以使用Date比较
    const timeRangeStartDate = new Date();
    timeRangeStartDate.setDate(timeRangeStartDate.getDate() - (timeRange.value === 'two_days' ? 1 : 0));
    timeRangeStartDate.setHours(0, 0, 0, 0);
    const historyData = allHistoryData.filter(d => new Date(d.timestamp) >= timeRangeStartDate);
    
    // 采样12个点显示
    const sampledHistoryData = sampleData(historyData, 12);
    const tempValues = sampledHistoryData.map(d => d.temperature);
    const tempMin = tempValues.length > 0 ? Math.min(...tempValues) : 0;
    const tempMax = tempValues.length > 0 ? Math.max(...tempValues) : 100;
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

    // 过滤数据 - historyData的timestamp是完整日期字符串，可以使用Date比较
    const timeRangeStartDate = new Date();
    timeRangeStartDate.setDate(timeRangeStartDate.getDate() - (timeRange.value === 'two_days' ? 1 : 0));
    timeRangeStartDate.setHours(0, 0, 0, 0);
    const historyData = allHistoryData.filter(d => new Date(d.timestamp) >= timeRangeStartDate);
    
    // 采样12个点显示
    const sampledHistoryData = sampleData(historyData, 12);
    const humidityValues = sampledHistoryData.map(d => d.humidity);
    const humidityMin = humidityValues.length > 0 ? Math.min(...humidityValues) : 0;
    const humidityMax = humidityValues.length > 0 ? Math.max(...humidityValues) : 100;
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

  await fetchDevices();

  const latestDevices = getDevices();

  // 启动帧率计算定时器（每0.5秒计算一次）
  fpsUpdateInterval = window.setInterval(calculateAverageFps, 500);

  setTimeout(async () => {
    const currentDevices = getDevices();

    if (currentDevices.length > 0) {
      if (activeTab.value === 'realtime') {
        const currentDevice = currentDevices.find((d: any) => d.id === selectedDeviceId.value);
        const isCurrentDeviceOnline = currentDevice?.isOnline ?? false;
        
        if (!selectedDeviceId.value || !isCurrentDeviceOnline) {
          await autoSelectFirstOnlineDevice();
        } else {
          await fetchRealtimeData();
          startRealtimeDataTimer();
          startEnvDataTimer();
          nextTick(() => {
            initGaugeCharts();
          });
        }
      } else if (activeTab.value === 'history' && !historyDeviceId.value) {
        historyDeviceId.value = currentDevices[0]?.id || null;
        if (historyDeviceId.value) {
          if (isMobile.value) {
            await getOrUpdateDeviceHistoryData(historyDeviceId.value);
          } else {
            await fetchDeviceHistoryData(historyDeviceId.value);
          }
        }
      } else if (activeTab.value === 'analysis') {
        if (!selectedDeviceId.value) {
          selectedDeviceId.value = currentDevices[0]?.id || null;
        }
        if (isMobile.value) {
          await getOrUpdateDeviceHistoryData();
        } else {
          await fetchDeviceHistoryData();
        }
      }
    }
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
  stopEnvDataTimer();
  // 清理帧率计算定时器
  if (fpsUpdateInterval) {
    clearInterval(fpsUpdateInterval);
    fpsUpdateInterval = null;
  }
  temperatureChart?.dispose();
  humidityChart?.dispose();
  averageChart?.dispose();
  aqiGaugeChart?.dispose();
  dbGaugeChart?.dispose();
  aqiGaugeChartMobile?.dispose();
  dbGaugeChartMobile?.dispose();
  luxGaugeChart?.dispose();
  uvGaugeChart?.dispose();
  luxGaugeChartMobile?.dispose();
  uvGaugeChartMobile?.dispose();
});

// 监听activeTab变化，重置设备选择
watch(() => props.activeTab, async (newTab, oldTab) => {
  if (newTab === 'analysis') {
    if (isMobile.value) {
      await getOrUpdateDeviceHistoryData();
    } else {
      await fetchDeviceHistoryData();
    }
    nextTick(async () => {
      await initCharts();
    });
    stopRealtimeMonitoring();
    stopRealtimeDataTimer();
  } else if (newTab === 'history') {
    stopRealtimeDataTimer();
    fetchDevices().then(() => {
      const latestDevices = getDevices();
      if (latestDevices.length > 0) {
        const deviceId = latestDevices[0]?.id || null;
        historyDeviceId.value = deviceId;
        
        if (deviceId) {
          nextTick(async () => {
            if (isMobile.value) {
              await getOrUpdateDeviceHistoryData(deviceId);
            } else {
              await fetchDeviceHistoryData(deviceId);
            }
          });
        }
      }
    });
    stopRealtimeMonitoring();
  } else if (newTab === 'realtime') {
    fetchDevices().then(async () => {
      const latestDevices = getDevices();
      const currentDevice = latestDevices.find((d: any) => d.id === selectedDeviceId.value);
      const isCurrentDeviceOnline = currentDevice?.isOnline ?? false;
      
      if (!selectedDeviceId.value || !isCurrentDeviceOnline) {
          await autoSelectFirstOnlineDevice();
        } else {
          await fetchRealtimeData();
          startRealtimeDataTimer();
          startEnvDataTimer();
          nextTick(() => {
            initGaugeCharts();
          });
        }
      
      if (selectedDeviceId.value) {
        nextTick(() => {
          startRealtimeMonitoring();
        });
      }
    });
  } else {
    selectedDeviceId.value = null;
    historyDeviceId.value = null;
    stopRealtimeMonitoring();
  }
}, { immediate: true });

// 监听selectedDeviceId变化，更新设备数据
watch(() => selectedDeviceId.value, async (newDeviceId, oldDeviceId) => {
  if (newDeviceId && activeTab.value === 'realtime') {
    await fetchDevices();
    if (!isMobile.value) {
      const sensorData = await fetchSensorData(newDeviceId);
      if (sensorData.temperatureValues.length > 0 && sensorData.humidityValues.length > 0) {
        const latestTemp = sensorData.temperatureValues[sensorData.temperatureValues.length - 1];
        const latestHumidity = sensorData.humidityValues[sensorData.humidityValues.length - 1];
        updateDevice(newDeviceId, { temperature: latestTemp, humidity: latestHumidity });
      }
    }
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
  if (newPath === '/Data' || newPath === '/Stream') {
    setTimeout(async () => {
      const latestDevices = getDevices();

      if (latestDevices.length > 0) {
        if (activeTab.value === 'realtime') {
          const currentDevice = latestDevices.find((d: any) => d.id === selectedDeviceId.value);
          const isCurrentDeviceOnline = currentDevice?.isOnline ?? false;
          
          if (!selectedDeviceId.value || !isCurrentDeviceOnline) {
            await autoSelectFirstOnlineDevice();
          }
        } else if (!selectedDeviceId.value) {
          selectedDeviceId.value = latestDevices[0]?.id || null;
        }
      }
    }, 500);
  }
});

// 监听selectedDeviceId变化，自动开启实时监控
watch(() => selectedDeviceId.value, (newDeviceId, oldDeviceId) => {
  nextTick(() => {
    if (newDeviceId && activeTab.value === 'realtime') {
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

/* 环境数据环形进度条 */
.env-gauges {
  margin-top: 20px;
}

.env-gauges h3 {
  margin: 0 0 14px 0;
  font-size: 16px;
  font-weight: 600;
  color: #2d5016;
}

.gauges-row {
  display: flex;
  gap: 16px;
  justify-content: center;
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

.gauge-value {
  font-size: 16px;
  font-weight: 700;
  color: #2d5016;
  margin-top: 4px;
  transition: color 0.3s ease;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.05);
}

.gauge-value.alert {
  color: #dc2626;
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

/* 时间轴筛选器 */
.time-range-selector {
  margin-bottom: 16px;
  margin-top: 12px;
  padding: 0;
  background: transparent;
  border-radius: 0;
  box-shadow: none;
}

.time-buttons-container {
  display: flex;
  gap: 12px;
  align-items: center;
}

.time-button {
  font-size: 14px;
  font-weight: 500;
  color: #606266;
  background: #f5f7fa;
  border: 1px solid #e4e7ed;
  border-radius: 8px;
  padding: 8px 16px;
  cursor: pointer;
  transition: all 0.2s ease;
  white-space: nowrap;
  min-width: 60px;
  text-align: center;
}

.time-button:hover {
  color: #409eff;
  border-color: #c6e2ff;
  background: #ecf5ff;
  transform: translateY(-1px);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.08);
}

.time-button.active {
  color: #fff;
  background: linear-gradient(135deg, #409eff 0%, #66b1ff 100%);
  border-color: #409eff;
  box-shadow: 0 3px 6px rgba(64, 158, 255, 0.3);
  transform: translateY(-1px);
}

.time-button.active:hover {
  background: linear-gradient(135deg, #66b1ff 0%, #409eff 100%);
  box-shadow: 0 4px 8px rgba(64, 158, 255, 0.4);
}

/* 移动端保持原有滑块样式 */
.time-range-selector.mobile {
  margin-bottom: 20px;
  padding: 16px 20px;
  background: #fff;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.time-range-selector.mobile .time-slider-container {
  position: relative;
}

.time-range-selector.mobile .time-labels {
  display: flex;
  justify-content: space-between;
  margin-bottom: 12px;
}

.time-range-selector.mobile .time-label {
  font-size: 14px;
  font-weight: 500;
  color: #909399;
  cursor: pointer;
  padding: 6px 16px;
  border-radius: 20px;
  transition: all 0.3s ease;
}

.time-range-selector.mobile .time-label:hover {
  color: #409eff;
  background: rgba(64, 158, 255, 0.1);
}

.time-range-selector.mobile .time-label.active {
  color: #fff;
  background: linear-gradient(135deg, #409eff 0%, #66b1ff 100%);
  box-shadow: 0 2px 8px rgba(64, 158, 255, 0.3);
}

.time-range-selector.mobile .slider-track {
  position: relative;
  height: 6px;
  background: #e4e7ed;
  border-radius: 3px;
  margin: 0 20px;
}

.time-range-selector.mobile .slider-progress {
  position: absolute;
  left: 0;
  top: 0;
  height: 100%;
  background: linear-gradient(90deg, #409eff 0%, #66b1ff 100%);
  border-radius: 3px;
  transition: width 0.3s ease;
}

.time-range-selector.mobile .slider-thumb {
  position: absolute;
  top: 50%;
  width: 16px;
  height: 16px;
  background: #fff;
  border: 3px solid #409eff;
  border-radius: 50%;
  transform: translate(-50%, -50%);
  box-shadow: 0 2px 6px rgba(64, 158, 255, 0.4);
  transition: left 0.3s ease;
  cursor: pointer;
}

.time-range-selector.mobile .slider-thumb:hover {
  transform: translate(-50%, -50%) scale(1.2);
  box-shadow: 0 4px 10px rgba(64, 158, 255, 0.5);
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

/* 移动端环境数据环形进度条 */
.mobile-data .mobile-gauges {
  margin-bottom: 16px;
}

.mobile-data .mobile-gauges-row {
  display: flex;
  gap: 12px;
}

.mobile-data .mobile-gauge-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  background: rgba(255, 255, 255, 0.5);
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
  border: 1px solid rgba(139, 173, 66, 0.18);
  border-radius: 16px;
  padding: 14px 8px 10px;
  box-shadow: 0 4px 20px rgba(139, 173, 66, 0.06), inset 0 1px 0 rgba(255, 255, 255, 0.5);
}

.mobile-data .mobile-gauge-chart {
  width: 100px;
  height: 100px;
}

.mobile-data .mobile-gauge-label {
  font-size: 12px;
  font-weight: 500;
  color: #4a6741;
  margin-top: 2px;
  letter-spacing: 0.3px;
}

.mobile-data .mobile-gauge-value {
  font-size: 14px;
  font-weight: 700;
  color: #2d5016;
  margin-top: 2px;
  transition: color 0.3s ease;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.05);
}

.mobile-data .mobile-gauge-value.alert {
  color: #dc2626;
}

.mobile-data .mobile-gauges-row-second {
  margin-top: 12px;
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

.mobile-data .time-range-selector {
  margin: 16px 0;
  padding: 12px 16px;
}

.mobile-data .time-range-selector .time-label {
  font-size: 13px;
  padding: 5px 12px;
}

.mobile-data .time-range-selector .slider-track {
  margin: 0 12px;
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
