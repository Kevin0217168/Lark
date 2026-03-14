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
            <el-card class="data-card-content">
              <div class="data-item">
                <span class="data-label">设备名称:</span>
                <span class="data-value">{{ selectedDevice.name }}</span>
              </div>
              <div class="data-item">
                <span class="data-label">设备状态:</span>
                <span class="data-value" :class="selectedDevice.status">{{ selectedDevice.status === 'online' ? '在线' : '离线' }}</span>
              </div>
              <div class="data-item">
                <span class="data-label">温度:</span>
                <span class="data-value">{{ selectedDevice.temperature }} °C</span>
              </div>
              <div class="data-item">
                <span class="data-label">湿度:</span>
                <span class="data-value">{{ selectedDevice.humidity }} %</span>
              </div>
              <div class="data-item">
                <span class="data-label">创建时间:</span>
                <span class="data-value">{{ selectedDevice.createTime }}</span>
              </div>
              <div class="data-actions">
                <el-button type="primary" @click="goToFullscreen">查看大图模式</el-button>
              </div>
            </el-card>
          </div>
        </div>
        
        <!-- 右侧视频流区域 -->
        <div class="data-right">
          <h3>视频流</h3>
          <el-card class="video-card">
            <div class="video-container" v-if="selectedDevice && selectedDevice.videoStreamUrl">
              <video 
                :src="selectedDevice.videoStreamUrl" 
                class="video-stream"
                controls
                autoplay
                muted
              ></video>
            </div>
            <div class="video-placeholder" v-else>
              <el-empty description="请选择设备查看视频流" />
            </div>
          </el-card>
        </div>
      </div>
      
      <!-- 分析 -->
      <div v-else-if="props.activeTab === 'analysis'" class="tab-content">
        <el-card class="data-card">
          <template #header>
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
          </template>
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
        </el-card>
      </div>
      
      <!-- 历史数据 -->
      <div v-else-if="props.activeTab === 'history'" class="tab-content">
        <el-card class="history-card">
          <template #header>
            <div class="card-header">
              <div class="card-title">
                <span>历史数据</span>
              </div>
              <div class="card-actions">
                <el-select v-model="historyDeviceId" @change="handleHistoryDeviceChange" class="device-select">
                  <el-option
                    v-for="device in devices"
                    :key="device.id"
                    :label="device.name"
                    :value="device.id"
                  />
                </el-select>
              </div>
            </div>
          </template>
          <div class="history-content" v-if="historyDeviceId">
            <div class="device-info">
              <h3>设备信息</h3>
              <el-card class="info-card">
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
              </el-card>
            </div>
            
            <div class="history-table">
              <h3>历史数据记录</h3>
              <el-table :data="historyDataList" stripe style="width: 100%" max-height="400">
                <el-table-column prop="timestamp" label="时间" width="180" />
                <el-table-column prop="temperature" label="温度 (℃)" width="120" />
                <el-table-column prop="humidity" label="湿度 (%)" width="120" />
                <el-table-column prop="quality" label="环境质量指数" width="150" />
                <el-table-column label="状态" width="100">
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
        </el-card>
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed, watch, onMounted, onUnmounted, nextTick } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { useDeviceStore } from '../stores/deviceStore';
import { ElMessage } from 'element-plus';
import * as echarts from 'echarts';

const route = useRoute();
const router = useRouter();
const { getDevices, getDeviceHistoryData } = useDeviceStore();
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

const handleDeviceChange = () => {
  if (selectedDevice.value) {
    ElMessage.success(`已切换到设备: ${selectedDevice.value.name}`);
    // 切换设备后更新图表
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

// 生成所有设备的平均值数据
const generateAverageData = () => {
  const times: string[] = [];
  const temperatureValues: number[] = [];
  const humidityValues: number[] = [];
  const qualityValues: number[] = [];
  
  // 从store获取所有设备的历史数据
  const allHistoryData = getDeviceHistoryData();
  
  // 只取最近24小时的数据
  const filteredData = allHistoryData.slice(-24 * devices.length);
  
  // 按时间分组
  const timeGroups: Record<string, { temp: number[], humidity: number[], quality: number[] }> = {};
  
  filteredData.forEach(data => {
    const date = new Date(data.timestamp);
    const hourKey = `${date.getHours().toString().padStart(2, '0')}:00`;
    
    if (!timeGroups[hourKey]) {
      timeGroups[hourKey] = { temp: [], humidity: [], quality: [] };
    }
    
    timeGroups[hourKey].temp.push(data.temperature);
    timeGroups[hourKey].humidity.push(data.humidity);
    timeGroups[hourKey].quality.push(data.quality);
  });
  
  // 生成最近24小时的时间
  for (let i = 23; i >= 0; i--) {
    const date = new Date();
    date.setHours(date.getHours() - i);
    const hourKey = `${date.getHours().toString().padStart(2, '0')}:00`;
    times.push(hourKey);
    
    // 计算每个时间点的平均值
    const group = timeGroups[hourKey];
    if (group) {
      const tempAvg = group.temp.length > 0 ? group.temp.reduce((sum, val) => sum + val, 0) / group.temp.length : 0;
      const humidityAvg = group.humidity.length > 0 ? group.humidity.reduce((sum, val) => sum + val, 0) / group.humidity.length : 0;
      const qualityAvg = group.quality.length > 0 ? group.quality.reduce((sum, val) => sum + val, 0) / group.quality.length : 0;
      
      temperatureValues.push(Number(tempAvg.toFixed(1)));
      humidityValues.push(Number(humidityAvg.toFixed(1)));
      qualityValues.push(Number(qualityAvg.toFixed(1)));
    } else {
      temperatureValues.push(0);
      humidityValues.push(0);
      qualityValues.push(0);
    }
  }
  
  return { times, temperatureValues, humidityValues, qualityValues };
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
    const avgData = generateAverageData();
    
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
onMounted(() => {
  window.addEventListener('resize', handleResize);
  
  // 如果默认有设备，初始化图表
  if (devices.length > 0 && !selectedDeviceId.value) {
    selectedDeviceId.value = devices[0]?.id || null;
  }
});

// 组件卸载时清理
onUnmounted(() => {
  window.removeEventListener('resize', handleResize);
  temperatureChart?.dispose();
  humidityChart?.dispose();
  qualityChart?.dispose();
  averageChart?.dispose();
});

// 监听activeTab变化，重置设备选择
watch(() => props.activeTab, (newTab) => {
  if (newTab === 'analysis') {
    // 切换到分析标签时，如果有设备，初始化图表
    if (devices.length > 0 && !selectedDeviceId.value) {
      selectedDeviceId.value = devices[0]?.id || null;
    }
    nextTick(() => {
      initCharts();
    });
  } else if (newTab === 'history') {
    // 切换到历史数据标签时，如果有设备，自动选择第一个设备
    if (devices.length > 0 && !historyDeviceId.value) {
      historyDeviceId.value = devices[0]?.id || null;
    }
  } else {
    selectedDeviceId.value = null;
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

// 组件挂载时检查URL参数
onMounted(() => {
  const deviceId = route.query.deviceId;
  if (deviceId) {
    const id = parseInt(deviceId as string);
    if (!isNaN(id)) {
      selectedDeviceId.value = id;
    }
  }
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
.data-card {
  z-index: 1;
  width: 75%;
  margin: 1% auto;
  border-radius: 15px;
  background-color: #fff;
  padding: 30px;
  min-height: 80vh;
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
  min-height: 0;
  height: 100%;
  display: flex;
  flex-direction: column;
}

.video-container {
  flex: 1;
  background: #f5f7fa;
  border-radius: 10px;
  overflow: hidden;
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 0;
}

.video-stream {
  width: 100%;
  height: 100%;
  object-fit: cover;
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

.device-info {
  margin-bottom: 20px;
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
