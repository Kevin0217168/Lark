<template>
  <el-card class="home-card">
    <h3 class="title" v-if="!isFullscreen">系统总览</h3>
    <div class="main-layout" v-if="!isFullscreen">
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
            <p><strong>厂区平均温度:</strong> 100°C</p>
            <p><strong>厂区平均湿度:</strong> 100%</p>
            <p><strong>厂区平均空气质量:</strong> 良好</p>
            <div class="extreme-values">
              <h5>极值数据</h5>
              <p>温度极值: </p>
              <p>湿度极值: </p>
              <p>空气质量极值: </p>
            </div>
            <hr>
            <div class="system-status-section">
              <h5>系统状态</h5>
              <el-tag type="success" size="large">运行正常</el-tag>
              <p style="margin-top: 10px;">系统运行时间: 15天</p>
              <p style="margin-top: 10px;">报错数量: </p>
            </div>
            <div class="view-logs-section">
              <el-button type="primary" @click="goToDeviceLogs">查看日志</el-button>
            </div>
          </div>
        </el-card>
      </div>
      <div class="right-column">
        <el-card class="image-preview-merged" @click="enterFullscreen">
          <h4>大图模式</h4>
          <el-image 
            src="/banner.jpg" 
            fit="cover"
            class="preview-image-merged"
          />
        </el-card>
      </div>
    </div>
    <div v-else class="fullscreen-image">
      <div class="fullscreen-header">
        <h3>大图模式</h3>
        <el-button @click="exitFullscreen" type="primary">退出</el-button>
      </div>
      <el-image 
        src="/banner.jpg" 
        fit="contain"
        class="fullscreen-img"
      />
    </div>
  </el-card>
</template>

<script lang="ts" setup>
import { ref } from 'vue';
import { useRouter } from 'vue-router';
import { useDeviceStore } from '../stores/deviceStore';

const router = useRouter();
const { getDeviceStats } = useDeviceStore();
const deviceStats = getDeviceStats();

// 全屏状态
const isFullscreen = ref(false);

// 进入全屏
const enterFullscreen = () => {
  isFullscreen.value = true;
};

// 退出全屏
const exitFullscreen = () => {
  isFullscreen.value = false;
};

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
  width: 95%;
  margin: 2% auto;
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
  height: 350px;
  border-radius: 15px;
  display: flex;
  flex-direction: column;
  justify-content: center;
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
  margin-bottom: 10px;
  color: #409eff;
  font-size: 14px;
}

.extreme-values p {
  margin: 5px 0;
  font-size: 14px;
  color: #666;
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
  cursor: pointer;
  transition: all 0.3s ease;
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

.fullscreen-image {
  width: 100%;
  height: calc(100vh - 200px);
  display: flex;
  flex-direction: column;
  padding: 20px;
}

.fullscreen-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  padding: 0 20px;
}

.fullscreen-header h3 {
  margin: 0;
}

.fullscreen-img {
  flex: 1;
  width: 100%;
  min-height: 0;
}
</style>
