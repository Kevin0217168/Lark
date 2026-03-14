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
              <p style="margin-top: 10px;color: #FF4D4F;">报错数量: {{ errorCount }}</p>
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
    <div v-else class="fullscreen-layout">
      <div class="fullscreen-header">
        <h3>大图模式</h3>
        <el-button @click="exitFullscreen" type="primary">退出</el-button>
      </div>
      <div class="fullscreen-content">
        <!-- 左侧设备列表 -->
        <div class="device-list-panel">
          <h4>设备列表</h4>
          <div class="device-list">
            <div 
              v-for="device in devices" 
              :key="device.id"
              class="device-item"
              draggable="true"
              @dragstart="handleDragStart($event, device)"
            >
              {{ device.name }}
            </div>
          </div>
        </div>
        <!-- 右侧图片区域 -->
        <div 
          class="image-drop-zone"
          @dragover.prevent
          @drop="handleDrop"
        >
          <div class="image-wrapper" ref="imageWrapperRef">
            <img 
              src="/banner.jpg" 
              class="fullscreen-img"
              ref="imageRef"
              @load="onImageLoad"
            />
            <!-- 已放置的设备标记 -->
            <div
              v-for="(placedDevice, index) in placedDevices"
              :key="index"
              class="placed-device"
              :style="getPlacedDeviceStyle(placedDevice)"
            >
              <el-tag type="primary" size="small" closable @close="removePlacedDevice(index)">{{ placedDevice.name }}</el-tag>
            </div>
          </div>
        </div>
      </div>
    </div>
  </el-card>
</template>

<script lang="ts" setup>
import { ref, computed } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import { useDeviceStore } from '../stores/deviceStore';

const router = useRouter();
const { devices, getDeviceStats, getDeviceLogs } = useDeviceStore();
const deviceStats = getDeviceStats();

const errorCount = computed(() => {
  const allLogs = getDeviceLogs();
  return allLogs.filter(log => log.level === 'warning' || log.level === 'error').length;
});

// 全屏状态
const isFullscreen = ref(false);

// 已放置的设备（使用相对于图片原始尺寸的百分比）
interface PlacedDevice {
  id: number;
  name: string;
  x: number; // 0-100 百分比
  y: number; // 0-100 百分比
}
const placedDevices = ref<PlacedDevice[]>([]);
const draggedDevice = ref<{ id: number; name: string } | null>(null);

// 图片引用
const imageRef = ref<HTMLImageElement | null>(null);
const imageWrapperRef = ref<HTMLElement | null>(null);
const imageNaturalSize = ref({ width: 0, height: 0 });

// 图片加载完成后获取原始尺寸
const onImageLoad = () => {
  if (imageRef.value) {
    imageNaturalSize.value = {
      width: imageRef.value.naturalWidth,
      height: imageRef.value.naturalHeight
    };
  }
};

// 计算设备标记的样式
const getPlacedDeviceStyle = (placedDevice: PlacedDevice) => {
  return {
    left: placedDevice.x + '%',
    top: placedDevice.y + '%',
    transform: 'translate(-50%, -50%)'
  };
};

// 进入全屏
const enterFullscreen = () => {
  isFullscreen.value = true;
};

// 退出全屏
const exitFullscreen = () => {
  isFullscreen.value = false;
};

// 拖拽开始
const handleDragStart = (event: DragEvent, device: { id: number; name: string }) => {
  draggedDevice.value = device;
  if (event.dataTransfer) {
    event.dataTransfer.effectAllowed = 'copy';
  }
};

// 放置设备
const handleDrop = (event: DragEvent) => {
  event.preventDefault();
  if (!draggedDevice.value) return;

  // 检查设备是否已放置
  const existingIndex = placedDevices.value.findIndex(d => d.id === draggedDevice.value!.id);
  if (existingIndex !== -1) {
    ElMessage.warning('该设备已放置，请先移除后再放置');
    return;
  }

  const imgElement = imageRef.value;
  if (!imgElement) return;

  const imgRect = imgElement.getBoundingClientRect();

  // 检查鼠标是否在图片显示区域内
  if (
    event.clientX < imgRect.left ||
    event.clientX > imgRect.right ||
    event.clientY < imgRect.top ||
    event.clientY > imgRect.bottom
  ) {
    ElMessage.warning('请将设备放置在图片区域内');
    return;
  }

  // 计算相对于图片显示区域的位置（百分比）
  const x = ((event.clientX - imgRect.left) / imgRect.width) * 100;
  const y = ((event.clientY - imgRect.top) / imgRect.height) * 100;

  placedDevices.value.push({
    id: draggedDevice.value.id,
    name: draggedDevice.value.name,
    x: x,
    y: y
  });

  draggedDevice.value = null;
  ElMessage.success('设备放置成功');
};

// 移除已放置的设备
const removePlacedDevice = (index: number) => {
  placedDevices.value.splice(index, 1);
  ElMessage.success('设备已移除');
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

.fullscreen-layout {
  width: 100%;
  height: calc(100vh - 200px);
  display: flex;
  flex-direction: column;
  padding: 20px;
}

.fullscreen-content {
  display: flex;
  flex: 1;
  gap: 20px;
  overflow: hidden;
}

.device-list-panel {
  width: 200px;
  background: #f5f7fa;
  border-radius: 8px;
  padding: 15px;
  overflow-y: auto;
}

.device-list-panel h4 {
  margin: 0 0 15px 0;
  color: #303133;
  font-size: 16px;
}

.device-list {
  display: flex;
  flex-direction: column;
  gap: 10px;
}

.device-item {
  padding: 10px 15px;
  background: #fff;
  border-radius: 6px;
  cursor: move;
  transition: all 0.3s;
  border: 1px solid #e4e7ed;
  font-size: 14px;
}

.device-item:hover {
  background: #ecf5ff;
  border-color: #409eff;
}

.device-item:active {
  cursor: grabbing;
}

.image-drop-zone {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #f5f7fa;
  border-radius: 8px;
  overflow: hidden;
  position: relative;
}

.image-container {
  position: relative;
  width: 100%;
  height: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
}

.image-wrapper {
  position: relative;
  display: inline-block;
  max-width: 100%;
  max-height: 100%;
}

.image-wrapper img {
  max-width: 100%;
  max-height: 100%;
  object-fit: contain;
  display: block;
}

.placed-device {
  position: absolute;
  z-index: 10;
  pointer-events: auto;
}

.placed-device .el-tag {
  cursor: pointer;
}
</style>
