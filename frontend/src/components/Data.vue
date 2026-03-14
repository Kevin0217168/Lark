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
        <el-empty description="分析功能开发中" />
      </div>
      
      <!-- 历史数据 -->
      <div v-else-if="props.activeTab === 'history'" class="tab-content">
        <el-empty description="历史数据功能开发中" />
      </div>
    </div>
  </div>
</template>

<script lang="ts" setup>
import { ref, computed, watch } from 'vue';
import { useDeviceStore } from '../stores/deviceStore';
import { ElMessage } from 'element-plus';

const { getDevices } = useDeviceStore();
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
  }
};

// 监听activeTab变化，重置设备选择
watch(() => props.activeTab, () => {
  selectedDeviceId.value = null;
});
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
  display: flex;
  align-items: center;
  justify-content: center;
  background: #f5f7fa;
  border-radius: 15px;
  margin-top: 20px;
  padding: 40px;
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