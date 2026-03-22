<template>
  <div class="mobile-home">
    <!-- 统计卡片 -->
    <div class="mobile-stats-cards">
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
            <span class="env-value">{{ avgEnvironmentData.temperature }}°C</span>
          </div>
          <div class="env-item">
            <span class="env-label">湿度</span>
            <span class="env-value">{{ avgEnvironmentData.humidity }}%</span>
          </div>
          <div class="env-item">
            <span class="env-label">空气质量</span>
            <span class="env-value">{{ avgEnvironmentData.quality }}</span>
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

    <!-- 图表区域 -->
    <div class="mobile-chart-section">
      <div class="chart-card">
        <div class="chart-header">
          <h3 class="chart-title">所有设备平均值</h3>
        </div>
        <div ref="averageChartRef" class="chart"></div>
      </div>
    </div>

    <!-- 备案信息 -->
    <MobileBeian />
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue';
import { useRouter } from 'vue-router';
import { Monitor, DataAnalysis, Warning, ArrowRight } from '@element-plus/icons-vue';
import { useDeviceStore } from '../../stores/deviceStore';
import * as echarts from 'echarts';
import MobileBeian from './MobileBeian.vue';

const router = useRouter();
const { 
  devices, 
  getDeviceLogs, 
  getDeviceAverageData,
  fetchDevices
} = useDeviceStore();

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

// 计算环境数据平均值
const avgEnvironmentData = computed(() => {
  const onlineDevices = devices.value.filter(d => d.isOnline);
  
  if (onlineDevices.length === 0) {
    return {
      temperature: 0,
      humidity: 0,
      quality: 0
    };
  }
  
  const totalTemperature = onlineDevices.reduce((sum, device) => sum + (device.temperature || 0), 0);
  const totalHumidity = onlineDevices.reduce((sum, device) => sum + (device.humidity || 0), 0);
  
  const historyData = getDeviceAverageData();
  const latestQuality = historyData.qualityValues.length > 0 ? historyData.qualityValues[historyData.qualityValues.length - 1] : 0;
  
  return {
    temperature: Number((totalTemperature / onlineDevices.length).toFixed(1)),
    humidity: Number((totalHumidity / onlineDevices.length).toFixed(1)),
    quality: latestQuality
  };
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
    
    const avgData = getDeviceAverageData();
    
    averageChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      legend: {
        data: ['平均温度', '平均湿度', '平均质量指数'],
        top: 10,
        textStyle: {
          fontSize: 12
        },
        itemGap: 15,
        backgroundColor: 'transparent'
      },
      xAxis: {
        type: 'category',
        data: avgData.times.slice(-12),
        axisLabel: {
          fontSize: 10,
          rotate: 45
        }
      },
      yAxis: {
        type: 'value',
        name: '数值',
        axisLabel: {
          fontSize: 10
        }
      },
      series: [
        {
          name: '平均温度',
          data: avgData.temperatureValues.slice(-12),
          type: 'line',
          smooth: true,
          itemStyle: {
            color: '#ff7875'
          }
        },
        {
          name: '平均湿度',
          data: avgData.humidityValues.slice(-12),
          type: 'line',
          smooth: true,
          itemStyle: {
            color: '#69c0ff'
          }
        },
        {
          name: '平均质量指数',
          data: avgData.qualityValues.slice(-12),
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
  await fetchDevices();
  initChart();
  window.addEventListener('resize', handleResize);
});

onUnmounted(() => {
  window.removeEventListener('resize', handleResize);
  averageChart?.dispose();
});
</script>

<style scoped>
.mobile-home {
  padding: 16px;
  display: flex;
  flex-direction: column;
  min-height: calc(100vh - 180px);
}

/* 统计卡片 */
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
}

.mobile-stats-card:active {
  transform: scale(0.98);
}

.mobile-stats-card.purple {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}

.mobile-stats-card.blue {
  background: linear-gradient(135deg, #4facfe 0%, #00f2fe 100%);
}

.mobile-stats-card.red {
  background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
}

.mobile-stats-card.green-purple {
  background: linear-gradient(135deg, #5a9a9a 0%, #7fb3b3 100%);
}

.stats-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.stats-title {
  font-size: 14px;
  font-weight: 500;
  margin: 0;
  opacity: 0.9;
}

.stats-header .el-icon {
  font-size: 20px;
  opacity: 0.8;
}

.stats-value {
  font-size: 28px;
  font-weight: 600;
  margin: 0 0 12px 0;
}

/* 设备状态进度条 */
.device-progress-wrapper {
  margin: 12px 0;
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
.env-data {
  margin: 12px 0;
}

.env-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
}

.env-item:last-child {
  margin-bottom: 0;
}

.env-label {
  font-size: 14px;
  opacity: 0.9;
}

.env-value {
  font-size: 16px;
  font-weight: 600;
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
}

.error-log-item:last-child {
  margin-bottom: 0;
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

/* 图表区域 */
.mobile-chart-section {
  margin-bottom: 24px;
}

.chart-card {
  background: white;
  border-radius: 16px;
  padding: 16px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
}

.chart-header {
  margin-bottom: 16px;
}

.chart-title {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
  margin: 0;
}

.chart {
  width: 100%;
  height: 250px;
  border-radius: 10px;
  background-color: #f9f9f9;
}
</style>
