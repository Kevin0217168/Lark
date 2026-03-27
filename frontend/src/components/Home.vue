<template>
  <div class="home-container">
    
    <!-- 统计卡片 -->
    <div class="stats-cards">
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
          <h3 class="chart-title">所有设备平均值 <span class="time-range">(最近24小时)</span></h3>
        </div>
        <div ref="averageChartRef" class="chart"></div>
      </div>
    </div>
    

  </div>
</template>

<script lang="ts" setup>
import { ref, computed, onMounted, onUnmounted, watch } from 'vue';
import { useRouter } from 'vue-router';
import { useDeviceStore } from '../stores/deviceStore';
import { ElMessage, ElIcon } from 'element-plus';
import { Monitor, DataAnalysis, Warning } from '@element-plus/icons-vue';
import * as echarts from 'echarts';

const router = useRouter();
const { 
  devices, 
  deviceHistoryData,
  getDeviceLogs, 
  getDeviceAverageData,
  getDeviceExtremaData,
  fetchDevices,
  getOrUpdateDevices,
  fetchDeviceHistoryData
} = useDeviceStore();

// 计算设备统计信息
const deviceStats = computed(() => {
  const total = devices.value.length;
  const online = devices.value.filter(d => d.isOnline).length;
  const offline = devices.value.filter(d => !d.isOnline).length;
  const normal = online;
  const abnormal = 0;
  
  return {
    total,
    online,
    offline,
    normal,
    abnormal
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

// 计算错误率（基于日志总数）
const errorPercentage = computed(() => {
  const allLogs = getDeviceLogs();
  if (allLogs.length === 0) return 0;
  const errorLogs = allLogs.filter(log => log.level === 'warning' || log.level === 'error');
  return Math.round((errorLogs.length / allLogs.length) * 100);
});

// 计算环境数据平均值
const avgEnvironmentData = computed(() => {
  // 从设备数据中计算平均值
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
  
  return {
    temperature: Number((totalTemperature / onlineDevices.length).toFixed(1)),
    humidity: Number((totalHumidity / onlineDevices.length).toFixed(1))
  };
});

// 计算24小时内所有设备的温度湿度的最高最低值
const extremaEnvironmentData = computed(() => {
  return getDeviceExtremaData();
});



// 图表容器引用
const averageChartRef = ref<HTMLElement | null>(null);

// 图表实例
let averageChart: echarts.ECharts | null = null;

// 初始化图表
const initChart = () => {
  if (averageChartRef.value) {
    if (averageChart) {
      averageChart.dispose();
    }
    averageChart = echarts.init(averageChartRef.value);
    
    // 获取平均值数据
    const avgData = getDeviceAverageData();
    
    averageChart.setOption({
      tooltip: {
        trigger: 'axis'
      },
      legend: {
        data: ['平均温度', '平均湿度'],
        top: 30
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

// 组件挂载时初始化
onMounted(async () => {
  window.addEventListener('resize', handleResize);
  // 从后端获取设备数据
  await getOrUpdateDevices();
  // 获取所有设备的历史数据用于平均值图表
  await fetchDeviceHistoryData();
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

// 组件卸载时清理
onUnmounted(() => {
  window.removeEventListener('resize', handleResize);
  averageChart?.dispose();
});


</script>

<style lang="scss" scoped>
.home-container {
  width: 90%;
  max-width: 1200px;
  margin: 2% auto;
  padding: 20px;
  position: relative;
  z-index: 1;
}



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
  font-size: 32px;
  font-weight: 600;
  margin: 0 0 15px 0;
}

.stats-progress {
  width: 100%;
  height: 4px;
  background: rgba(255, 255, 255, 0.2);
  border-radius: 2px;
  margin-bottom: 8px;
  overflow: hidden;
}

.progress-bar {
  height: 100%;
  background: rgba(255, 255, 255, 0.8);
  border-radius: 2px;
  transition: width 0.3s ease;
}

.stats-percentage {
  font-size: 12px;
  opacity: 0.8;
  margin: 8px 0 0 0;
  text-align: right;
}

.device-progress-wrapper {
  flex: 1;
  display: flex;
  flex-direction: column;
  justify-content: center;
  margin: 10px 0;
}

.dual-progress-bar {
  display: flex;
  width: 100%;
  height: 32px;
  border-radius: 16px;
  overflow: hidden;
  background: rgba(255, 255, 255, 0.1);
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
  font-size: 14px;
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
  font-size: 14px;
  font-weight: 600;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  padding: 0 8px;
}

.env-data {
  flex: 1;
  display: flex;
  flex-direction: column;
  justify-content: space-evenly;
  margin: 10px 0;
}

.env-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.env-label {
  font-size: 14px;
  opacity: 0.9;
}

.env-values {
  display: flex;
  gap: 10px;
  width: 230px;
  justify-content: space-between;
}

.env-max {
  font-size: 16px;
  font-weight: 600;
  color: #ffffff;
  text-shadow: 0 1px 3px rgba(0, 0, 0, 0.5);
}

.env-min {
  font-size: 16px;
  font-weight: 600;
  color: #ffffff;
  text-shadow: 0 1px 3px rgba(0, 0, 0, 0.5);
}

.card-action {
  margin-top: auto;
  text-align: center;
  padding-top: 15px;
  border-top: 1px solid rgba(255, 255, 255, 0.2);
}

.card-action .el-button {
  width: 100%;
  justify-content: center;
  border: none;
  font-weight: 500;
}

.device-btn {
  background: rgba(255, 255, 255, 0.2);
  color: white;
}

.device-btn:hover {
  background: rgba(255, 255, 255, 0.3);
  color: white;
}

.env-btn {
  background: rgba(255, 255, 255, 0.2);
  color: white;
}

.env-btn:hover {
  background: rgba(255, 255, 255, 0.3);
  color: white;
}

.error-btn {
  background: rgba(255, 255, 255, 0.2);
  color: white;
}

.error-btn:hover {
  background: rgba(255, 255, 255, 0.3);
  color: white;
}

.placeholder {
  height: 20px;
  margin-bottom: 22px;
}

.error-logs-preview {
  min-height: 60px;
  margin-bottom: 10px;
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
}

.log-message {
  color: rgba(255, 255, 255, 0.9);
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  flex: 1;
}

.no-logs {
  text-align: center;
  color: rgba(255, 255, 255, 0.6);
  font-size: 13px;
  padding: 20px 0;
}

.chart-section {
  margin-bottom: 30px;
}

.chart-card {
  background: #fff;
  border-radius: 15px;
  padding: 20px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.1);
}

.chart-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.chart-title {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
  margin: 0;
}

.time-range {
  font-size: 12px;
  color: #909399;
  font-weight: normal;
  margin-left: 8px;
}

.time-select {
  width: 120px;
}

.chart {
  width: 100%;
  height: 400px;
  border-radius: 10px;
  background-color: #f9f9f9;
}



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
    height: 300px;
  }
  
  .action-buttons {
    flex-direction: column;
  }
}
</style>