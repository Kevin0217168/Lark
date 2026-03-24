<template>
  <el-card class="home-card">
    <h3 class="title">系统总览</h3>
    <div class="main-layout">
      <div class="left-column">
        <el-card class="summary">
          <h4>设备统计</h4>
          <div class="stats-content">
            <p>总设备数: {{ deviceStats.total }}</p>
            <p style="color: #52C41A;">在线设备: {{ deviceStats.online }}</p>
            <p style="color: #52C41A;">正常设备: {{ deviceStats.normal }}</p>
            <p style="color: #FF4D4F;">异常设备: {{ deviceStats.abnormal }}</p>
            <hr>
            <p style="color: #8D8D8D;">离线设备: {{ deviceStats.offline }}</p>
            <div class="view-devices-section">
              <el-button type="primary" @click="goToDeviceManagement">查看设备</el-button>
            </div>
          </div>
        </el-card>
        <el-card class="data_summary">
          <h4>数据概览</h4>
          <div class="stats-content">
            <p><strong>厂区平均温度:</strong> {{ averageData.temperature }}°C</p>
            <p><strong>厂区平均湿度:</strong> {{ averageData.humidity }}%</p>

            <div class="extreme-values">
              <h5>极值数据</h5>
              <p class="extreme-hint">（最近24小时内）</p>
              <div class="extreme-item">
                <span class="extreme-label">温度极值:</span>
                <span class="extreme-high">最高 {{ temperatureExtreme.max }}°C</span>
                <span class="extreme-divider">/</span>
                <span class="extreme-low">最低 {{ temperatureExtreme.min }}°C</span>
              </div>
              <div class="extreme-item">
                <span class="extreme-label">湿度极值:</span>
                <span class="extreme-high">最高 {{ humidityExtreme.max }}%</span>
                <span class="extreme-divider">/</span>
                <span class="extreme-low">最低 {{ humidityExtreme.min }}%</span>
              </div>
              
            </div>
            <hr>
            <div class="system-status-section">
              <h5>系统状态</h5>
              <el-tag type="success" size="large">运行正常</el-tag>
              <p style="margin-top: 10px;">系统运行时间: 15天</p>
              <p style="margin-top: 10px;color: #FF4D4F;">报错数量: {{ errorCount }}</p>
            </div>
            <div class="view-logs-section">
              <el-button type="primary" @click="goToDeviceLogs">查看日志</el-button>
            </div>
          </div>
        </el-card>
      </div>
      <div class="right-column">
        <el-card class="image-preview-merged">
          <h4>厂区预览</h4>
          <el-image 
            src="/banner.jpg" 
            fit="cover"
            class="preview-image-merged"
          />
        </el-card>
      </div>
   </div>
  </el-card>
</template>

<script lang="ts" setup>
import { ref, computed, onMounted, watch } from 'vue';
import { useRouter, useRoute } from 'vue-router';
import { ElMessage } from 'element-plus';
import { useDeviceStore } from '../stores/deviceStore';

const router = useRouter();
const route = useRoute();
const { 
  devices, 
  getDeviceStats, 
  getDeviceLogs,
  deviceHistoryData,
  getDeviceAverageData
} = useDeviceStore();
const deviceStats = getDeviceStats();

const errorCount = computed(() => {
  const allLogs = getDeviceLogs();
  return allLogs.filter(log => log.level === 'warning' || log.level === 'error').length;
});

// 计算最新的平均值数据
const averageData = computed(() => {
  const data = getDeviceAverageData();
  // 获取最新的数据点（最后一个元素）
  const latestIndex = data.times.length - 1;
  return {
    temperature: data.temperatureValues[latestIndex] || 0,
    humidity: data.humidityValues[latestIndex] || 0
  };
});

// 获取24小时前的时间
const get24HoursAgo = () => {
  const date = new Date();
  date.setHours(date.getHours() - 24);
  return date;
};

// 计算温度极值（24小时内）
const temperatureExtreme = computed(() => {
  const twentyFourHoursAgo = get24HoursAgo();
  
  const temps = deviceHistoryData.value
    .filter(d => {
      const dataTime = new Date(d.timestamp);
      return dataTime >= twentyFourHoursAgo;
    })
    .map(d => d.temperature);
  
  return {
    max: temps.length > 0 ? Math.max(...temps) : 0,
    min: temps.length > 0 ? Math.min(...temps) : 0
  };
});

// 计算湿度极值（24小时内）
const humidityExtreme = computed(() => {
  const twentyFourHoursAgo = get24HoursAgo();
  
  const humidities = deviceHistoryData.value
    .filter(d => {
      const dataTime = new Date(d.timestamp);
      return dataTime >= twentyFourHoursAgo;
    })
    .map(d => d.humidity);
  
  return {
    max: humidities.length > 0 ? Math.max(...humidities) : 0,
    min: humidities.length > 0 ? Math.min(...humidities) : 0
  };
});



const goToDeviceLogs = () => {
  router.push({
    path: '/Device',
    query: { tab: 'logs' }
  });
};

const goToDeviceManagement = () => {
  router.push({
    path: '/Device',
    query: { tab: 'overview' }
  });
};
</script>

<style lang="scss" scoped>
.home-card {
  z-index: 1;
  width: 75%;
  margin: 10% auto;
  border-radius: 15px;
  text-align: center;
  background-color: #fff;
  padding: 30px;
  min-height: 80vh;
}

.title {
  text-align: center;
  margin-bottom: 20px;
}

.main-layout {
  display: flex;
  gap: 20px;
  align-items: stretch;
}

.left-column {
  flex: 0 0 30%;
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.right-column {
  flex: 1;
}

.summary {
  border-radius: 15px;
  display: flex;
  flex-direction: column;
  justify-content: center;
  overflow: visible;
}

.data_summary {
  flex: 1;
  border-radius: 15px;
  display: flex;
  flex-direction: column;
  justify-content: center;
}

.stats-content {
  text-align: left;
  padding: 20px;
  font-size: 16px;
}

.stats-content p {
  margin: 10px 0;
}

.extreme-values {
  margin-top: 20px;
  padding-top: 15px;
  border-top: 1px solid #e4e7ed;
}

.extreme-values h5 {
  margin-bottom: 5px;
  color: #409eff;
  font-size: 14px;
}

.extreme-hint {
  margin: 0 0 10px 0;
  font-size: 12px;
  color: #909399;
  font-style: italic;
}

.extreme-item {
  margin: 8px 0;
  font-size: 14px;
  display: flex;
  align-items: center;
  flex-wrap: wrap;
  gap: 5px;
}

.extreme-label {
  color: #666;
  margin-right: 5px;
}

.extreme-high {
  color: #f56c6c;
  font-weight: 500;
}

.extreme-low {
  color: #67c23a;
  font-weight: 500;
}

.extreme-divider {
  color: #999;
  margin: 0 3px;
}

.right-column {
  display: flex;
  flex-direction: column;
  min-height: 0;
}

.image-preview-merged {
  flex: 1;
  border-radius: 15px;
  display: flex;
  flex-direction: column;
  overflow: hidden;
  min-height: 0;
}

.preview-image-merged {
  width: 100%;
  height: 90%;
  flex: 1;
  border-radius: 10px;
  margin-top: 10px;
  min-height: 0;
}

.system-status-section {
  margin-top: 15px;
  padding-top: 15px;
}

.system-status-section h5 {
  margin-bottom: 10px;
  color: #409eff;
  font-size: 14px;
}

.view-logs-section {
  margin-top: 20px;
  padding-top: 15px;
  text-align: center;
}

.view-devices-section {
  margin-top: 20px;
  padding-top: 15px;
  text-align: center;
}
</style>