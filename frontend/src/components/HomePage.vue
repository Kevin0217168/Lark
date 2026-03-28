<template>
  <div class="home-container" :class="{ 'mobile': isMobile }">
    <!-- 统计卡片 -->
    <div v-if="isMobile" class="mobile-stats-cards">
      <!-- 设备状态 -->
      <div class="mobile-stats-card purple" @click="goToDeviceManagement">
        <div class="stats-header">
          <h3 class="stats-title">设备状态</h3>
          <el-icon><Monitor /></el-icon>
        </div>
        <p class="stats-value">{{ deviceStats.online }}/{{ deviceStats.total }}</p>
        <div class="device-progress-wrapper">
          <div class="dual-progress-bar">
            <div class="online-section" :style="{ width: onlinePercentage + '%' }">
              <span class="section-label">在线</span>
            </div>
            <div class="offline-section" :style="{ width: (100 - onlinePercentage) + '%' }">
              <span class="section-label">离线</span>
            </div>
          </div>
          <p class="stats-percentage">{{ onlinePercentage }}% 在线</p>
        </div>
        <div class="card-action">
          <span class="action-text">设备管理</span>
          <el-icon><ArrowRight /></el-icon>
        </div>
      </div>

      <!-- 环境数据 -->
      <div class="mobile-stats-card blue" @click="goToDataAnalysis">
        <div class="stats-header">
          <h3 class="stats-title">环境数据</h3>
          <el-icon><DataAnalysis /></el-icon>
        </div>
        <div class="env-data">
          <div class="env-item">
            <span class="env-label">温度</span>
            <div class="env-values">
              <span class="env-max">最高 {{ extremaEnvironmentData.temperature.max }}°C</span>
              <span class="env-min">最低 {{ extremaEnvironmentData.temperature.min }}°C</span>
            </div>
          </div>
          <div class="env-item">
            <span class="env-label">湿度</span>
            <div class="env-values">
              <span class="env-max">最高 {{ extremaEnvironmentData.humidity.max }}%</span>
              <span class="env-min">最低 {{ extremaEnvironmentData.humidity.min }}%</span>
            </div>
          </div>
        </div>
        <div class="card-action">
          <span class="action-text">数据分析</span>
          <el-icon><ArrowRight /></el-icon>
        </div>
      </div>

      <!-- 错误数量 -->
      <div class="mobile-stats-card" :class="errorCount === 0 ? 'green-purple' : 'red'" @click="goToDeviceLogs">
        <div class="stats-header">
          <h3 class="stats-title">错误数量</h3>
          <el-icon><Warning /></el-icon>
        </div>
        <p class="stats-value">{{ errorCount }}</p>
        <div class="error-logs-preview">
          <div v-for="(log, index) in recentErrorLogs" :key="index" class="error-log-item">
            <span class="log-device">{{ log.deviceName }}</span>
            <span class="log-message" :title="log.message">{{ log.message }}</span>
          </div>
          <div v-if="recentErrorLogs.length === 0" class="no-logs">暂无错误日志</div>
        </div>
        <div class="card-action">
          <span class="action-text">查看日志</span>
          <el-icon><ArrowRight /></el-icon>
        </div>
      </div>
    </div>

    <div v-else class="stats-cards">
      <div class="stats-card purple">
        <div class="stats-content">
          <h3 class="stats-title">设备状态</h3>
          <p class="stats-value">{{ deviceStats.online }}/{{ deviceStats.total }}</p>
          <div class="device-progress-wrapper">
            <div class="dual-progress-bar">
              <div class="online-section" :style="{ width: onlinePercentage + '%' }">
                <span class="section-label">在线</span>
              </div>
              <div class="offline-section" :style="{ width: (100 - onlinePercentage) + '%' }">
                <span class="section-label">离线</span>
              </div>
            </div>
            <p class="stats-percentage">{{ onlinePercentage }}% 在线</p>
          </div>
          <div class="card-action">
            <el-button class="device-btn" size="small" @click="goToDeviceManagement">
              <el-icon><Monitor /></el-icon>
              设备管理
            </el-button>
          </div>
        </div>
      </div>
      
      <div class="stats-card blue">
        <div class="stats-content">
          <h3 class="stats-title">环境数据</h3>
          <div class="env-data">
            <div class="env-item">
              <span class="env-label">温度</span>
              <div class="env-values">
                <span class="env-max">最高 {{ extremaEnvironmentData.temperature.max }}°C</span>
                <span class="env-min">最低 {{ extremaEnvironmentData.temperature.min }}°C</span>
              </div>
            </div>
            <div class="env-item">
              <span class="env-label">湿度</span>
              <div class="env-values">
                <span class="env-max">最高 {{ extremaEnvironmentData.humidity.max }}%</span>
                <span class="env-min">最低 {{ extremaEnvironmentData.humidity.min }}%</span>
              </div>
            </div>
          </div>
          <div class="card-action">
            <el-button class="env-btn" size="small" @click="goToDataAnalysis">
              <el-icon><DataAnalysis /></el-icon>
              数据分析
            </el-button>
          </div>
        </div>
      </div>
      
      <div class="stats-card" :class="errorCount === 0 ? 'green-purple' : 'red'">
        <div class="stats-content">
          <h3 class="stats-title">错误数量</h3>
          <p class="stats-value">{{ errorCount }}</p>
          <div class="error-logs-preview">
            <div v-for="(log, index) in recentErrorLogs" :key="index" class="error-log-item">
              <span class="log-device">{{ log.deviceName }}</span>
              <span class="log-message" :title="log.message">{{ log.message }}</span>
            </div>
            <div v-if="recentErrorLogs.length === 0" class="no-logs">暂无错误日志</div>
          </div>
          <div class="card-action">
            <el-button class="error-btn" size="small" @click="goToDeviceLogs">
              <el-icon><Warning /></el-icon>
              查看日志
            </el-button>
          </div>
        </div>
      </div>
    </div>
    
    <!-- 图表区域 -->
    <div class="chart-section">
      <div class="chart-card">
        <div class="chart-header">
          <h3 class="chart-title">所有设备平均值 <span class="time-range">(最近{{ isMobile ? 6 : 24 }}小时)</span></h3>
        </div>
        <div ref="averageChartRef" class="chart"></div>
      </div>
    </div>
    
    <!-- 备案信息 (仅移动端显示) -->
    <MobileBeian v-if="isMobile" />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue';
import { useRouter } from 'vue-router';
import { useDeviceStore } from '../stores/deviceStore';
import { ElMessage, ElIcon } from 'element-plus';
import { Monitor, DataAnalysis, Warning, ArrowRight } from '@element-plus/icons-vue';
import * as echarts from 'echarts';
import MobileBeian from './mobile/MobileBeian.vue';

const router = useRouter();
const { 
  devices, 
  deviceHistoryData,
  getDeviceLogs, 
  getDeviceAverageData,
  getDeviceExtremaData,
  fetchDevices,
  getOrUpdateDevices,
  fetchDeviceHistoryData,
  getOrUpdateDeviceHistoryData
} = useDeviceStore();

// 检测是否为移动设备
const isMobile = ref(window.innerWidth < 768);

// 监听窗口大小变化
const handleWindowResize = () => {
  isMobile.value = window.innerWidth < 768;
  averageChart?.resize();
};

// 计算设备统计信息
const deviceStats = computed(() => {
  const total = devices.value.length;
  const online = devices.value.filter(d => d.isOnline).length;
  return {
    total,
    online
  };
});

// 计算错误数量
const errorCount = computed(() => {
  const allLogs = getDeviceLogs();
  return allLogs.filter(log => log.level === 'warning' || log.level === 'error').length;
});

// 获取最近两条错误日志
const recentErrorLogs = computed(() => {
  const allLogs = getDeviceLogs();
  const errorLogs = allLogs.filter(log => log.level === 'warning' || log.level === 'error');
  return errorLogs.slice(0, 2);
});

// 计算在线率
const onlinePercentage = computed(() => {
  if (deviceStats.value.total === 0) return 0;
  return Math.round((deviceStats.value.online / deviceStats.value.total) * 100);
});

// 计算24小时内所有设备的温度湿度的最高最低值
const extremaEnvironmentData = computed(() => {
  return getDeviceExtremaData();
});

// 图表容器引用
const averageChartRef = ref<HTMLElement | null>(null);
let averageChart: echarts.ECharts | null = null;

// 初始化图表
const initChart = () => {
  if (averageChartRef.value) {
    if (averageChart) {
      averageChart.dispose();
    }
    averageChart = echarts.init(averageChartRef.value);
    
    const avgData = getDeviceAverageData(isMobile.value ? 6 : 24); // 移动端6小时，桌面端24小时
    
    averageChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      legend: {
        data: ['平均温度', '平均湿度'],
        top: isMobile.value ? 10 : 30,
        textStyle: {
          fontSize: isMobile.value ? 12 : 14
        },
        itemGap: isMobile.value ? 15 : 20,
        backgroundColor: 'transparent'
      },
      xAxis: {
        type: 'category',
        data: isMobile.value ? avgData.times.slice(-12) : avgData.times,
        axisLabel: {
          fontSize: isMobile.value ? 10 : 12,
          rotate: isMobile.value ? 45 : 0
        }
      },
      yAxis: {
        type: 'value',
        name: '数值', 
        axisLabel: {
          fontSize: isMobile.value ? 10 : 12
        }
      },
      series: [
        {
          name: '平均温度',
          data: isMobile.value ? avgData.temperatureValues.slice(-12) : avgData.temperatureValues,
          type: 'line',
          smooth: true,
          itemStyle: {
            color: '#ff7875'
          }
        },
        {
          name: '平均湿度',
          data: isMobile.value ? avgData.humidityValues.slice(-12) : avgData.humidityValues,
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

// 窗口大小变化时调整图表
const handleResize = () => {
  averageChart?.resize();
};

// 跳转到设备管理
const goToDeviceManagement = () => {
  router.push({
    path: '/Device',
    query: { tab: 'management' }
  });
};

// 跳转到数据分析
const goToDataAnalysis = () => {
  router.push({
    path: '/Data',
    query: { activeTab: 'analysis' }
  });
};

// 跳转到设备日志
const goToDeviceLogs = () => {
  router.push({
    path: '/Device',
    query: { tab: 'logs' }
  });
};

onMounted(async () => {
  window.addEventListener('resize', handleWindowResize);
  window.addEventListener('resize', handleResize);
  
  // 获取设备数据
  await getOrUpdateDevices();
  
  // 获取所有设备的历史数据
  if (devices.value.length > 0) {
    if (isMobile.value) {
      // 移动端：逐个获取设备历史数据
      for (const device of devices.value) {
        await getOrUpdateDeviceHistoryData(device.id);
      }
    } else {
      // 桌面端：一次性获取所有设备历史数据
      await fetchDeviceHistoryData();
    }
  }
  
  // 初始化图表
  initChart();
});

// 监听设备数据变化
watch(devices, () => {
  // 设备数据变化时重新初始化图表
  initChart();
}, { deep: true });

// 监听历史数据变化
watch(deviceHistoryData, () => {
  // 历史数据变化时重新初始化图表
  initChart();
}, { deep: true });

onUnmounted(() => {
  window.removeEventListener('resize', handleWindowResize);
  window.removeEventListener('resize', handleResize);
  averageChart?.dispose();
});
</script>

<style scss scoped>
.home-container {
  width: 90%;
  max-width: 1200px;
  margin: 2% auto;
  padding: 20px;
  position: relative;
  z-index: 1;

  &.mobile {
    padding: 16px;
    display: flex;
    flex-direction: column;
    min-height: calc(100vh - 180px);
    width: 100%;
    max-width: 100%;
    margin: 0;
    overflow-x: hidden;
    box-sizing: border-box;
  }
}

/* 移动端统计卡片 */
.mobile-stats-cards {
  display: flex;
  flex-direction: column;
  gap: 16px;
  margin-bottom: 24px;
}

.mobile-stats-card {
  border-radius: 16px;
  padding: 16px;
  color: white;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  transition: transform 0.3s ease, box-shadow 0.3s ease;
  cursor: pointer;
  position: relative;
  overflow: hidden;

  &:active {
    transform: scale(0.98);
  }

  &.purple {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  }

  &.blue {
    background: linear-gradient(135deg, #4a8fe7 0%, #0099ff 100%);
  }

  &.red {
    background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
  }

  &.green-purple {
    background: linear-gradient(135deg, #5a9a9a 0%, #7fb3b3 100%);
  }
}

/* 桌面端统计卡片 */
.stats-cards {
  display: flex;
  gap: 15px;
  margin-bottom: 30px;
  flex-wrap: nowrap;
  justify-content: center;
}

.stats-card {
  flex: 1;
  max-width: 300px;
  min-width: 280px;
  border-radius: 12px;
  padding: 18px;
  position: relative;
  overflow: hidden;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  transition: transform 0.3s ease, box-shadow 0.3s ease;
  color: white;

  &:hover {
    transform: translateY(-5px);
    box-shadow: 0 8px 24px rgba(0, 0, 0, 0.15);
  }

  &.purple {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  }

  &.blue {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  }

  &.red {
    background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
  }

  &.green-purple {
    background: linear-gradient(135deg, #5a9a9a 0%, #7fb3b3 100%);
  }
}

/* 公共样式 */
.stats-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.stats-content {
  position: relative;
  z-index: 2;
  color: white;
  display: flex;
  flex-direction: column;
  height: 100%;
}

.stats-title {
  font-size: 14px;
  font-weight: 500;
  margin: 0 0 10px 0;
  opacity: 0.9;
}

.stats-value {
  font-size: 28px;
  font-weight: 600;
  margin: 0 0 12px 0;
}

/* 设备状态进度条 */
.device-progress-wrapper {
  margin: 12px 0;
  flex: 1;
  display: flex;
  flex-direction: column;
  justify-content: center;
}

.dual-progress-bar {
  display: flex;
  width: 100%;
  height: 28px;
  border-radius: 14px;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.1);
  margin-bottom: 8px;
}

.online-section {
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #7fcdbb 0%, #41b6c4 100%);
  transition: width 0.3s ease;
  min-width: 0;
}

.online-section .section-label {
  color: white;
  font-size: 12px;
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  padding: 0 8px;
}

.offline-section {
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
  transition: width 0.3s ease;
  min-width: 0;
}

.offline-section .section-label {
  color: white;
  font-size: 12px;
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  padding: 0 8px;
}

.stats-percentage {
  font-size: 12px;
  opacity: 0.8;
  margin: 0;
  text-align: right;
}

/* 环境数据 */
.mobile-stats-card .env-data {
  margin: 12px 0;
}

.mobile-stats-card .env-item {
  margin-bottom: 10px;

  &:last-child {
    margin-bottom: 0;
  }
}

.mobile-stats-card .env-label {
  font-size: 14px;
  opacity: 0.9;
  display: block;
  margin-bottom: 4px;
}

.mobile-stats-card .env-values {
  display: flex;
  gap: 12px;
}

.mobile-stats-card .env-max, .mobile-stats-card .env-min {
  font-size: 13px;
  font-weight: 500;
  padding: 2px 8px;
  border-radius: 4px;
  background: rgba(255, 255, 255, 0.15);
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.7);
  border: 1px solid rgba(255, 255, 255, 0.3);
}

.mobile-stats-card .env-max {
  color: #ffffff;
  background: rgba(255, 107, 107, 0.15);
}

.mobile-stats-card .env-min {
  color: #ffffff;
  background: rgba(77, 171, 247, 0.15);
}

.stats-card .env-data {
  flex: 1;
  display: flex;
  flex-direction: column;
  justify-content: space-evenly;
  margin: 10px 0;
}

.stats-card .env-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.stats-card .env-label {
  font-size: 14px;
  opacity: 0.9;
}

.stats-card .env-values {
  display: flex;
  gap: 10px;
  width: 230px;
  justify-content: space-between;
}

.stats-card .env-max {
  font-size: 16px;
  font-weight: 600;
  color: #ffffff;
  text-shadow: 0 1px 3px rgba(0, 0, 0, 0.5);
}

.stats-card .env-min {
  font-size: 16px;
  font-weight: 600;
  color: #ffffff;
  text-shadow: 0 1px 3px rgba(0, 0, 0, 0.5);
}

/* 错误日志 */
.error-logs-preview {
  margin: 12px 0;
  min-height: 50px;
}

.error-log-item {
  display: flex;
  align-items: center;
  padding: 6px 10px;
  margin-bottom: 6px;
  background: rgba(255, 255, 255, 0.15);
  border-radius: 6px;
  font-size: 12px;
  line-height: 1.4;

  &:last-child {
    margin-bottom: 0;
  }
}

.log-device {
  flex-shrink: 0;
  font-weight: 600;
  color: #fff;
  margin-right: 8px;
  padding: 2px 6px;
  background: rgba(255, 255, 255, 0.2);
  border-radius: 4px;
  white-space: nowrap;
  font-size: 10px;
}

.log-message {
  color: rgba(255, 255, 255, 0.9);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  flex: 1;
  font-size: 11px;
}

.no-logs {
  text-align: center;
  color: rgba(255, 255, 255, 0.6);
  font-size: 12px;
  padding: 12px 0;
}

/* 卡片底部操作 */
.card-action {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-top: 12px;
  padding-top: 12px;
  border-top: 1px solid rgba(255, 255, 255, 0.2);
}

.action-text {
  font-size: 14px;
  font-weight: 500;
  opacity: 0.9;
}

.card-action .el-icon {
  font-size: 16px;
  opacity: 0.8;
}

/* 桌面端按钮样式 */
.card-action .el-button {
  width: 100%;
  justify-content: center;
  border: none;
  font-weight: 500;
}

.device-btn {
  background: rgba(255, 255, 255, 0.2);
  color: white;

  &:hover {
    background: rgba(255, 255, 255, 0.3);
    color: white;
  }
}

.env-btn {
  background: rgba(255, 255, 255, 0.2);
  color: white;

  &:hover {
    background: rgba(255, 255, 255, 0.3);
    color: white;
  }
}

.error-btn {
  background: rgba(255, 255, 255, 0.2);
  color: white;

  &:hover {
    background: rgba(255, 255, 255, 0.3);
    color: white;
  }
}

/* 图表区域 */
.mobile-chart-section {
  margin-bottom: 24px;
}

.chart-section {
  margin-bottom: 30px;
}

.chart-card {
  background: #fff;
  border-radius: 15px;
  padding: 20px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
  box-sizing: border-box;
  width: 100%;
}

.chart-header {
  margin-bottom: 20px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.chart-title {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
  margin: 0;
}

.chart-title .time-range {
  font-size: 12px;
  font-weight: normal;
  color: #909399;
  margin-left: 8px;
}

.chart {
  width: 100%;
  height: 400px;
  border-radius: 10px;
  background-color: transparent;
}

/* 响应式设计 */
@media (max-width: 1024px) {
  .stats-cards {
    flex-wrap: wrap;
  }
  
  .stats-card {
    max-width: calc(50% - 10px);
  }
}

@media (max-width: 768px) {
  .home-container {
    width: 95%;
    padding: 10px;
  }
  
  .stats-cards {
    flex-direction: column;
  }
  
  .stats-card {
    max-width: 100%;
    min-width: auto;
  }
  
  .chart {
    height: 250px;
    background-color: #f9f9f9;
  }
  
  .action-buttons {
    flex-direction: column;
  }
}
</style>