<template>
  <div class="fullscreen-layout">
    <div class="fullscreen-header">
      <div class="header-content">
        <h3>大图模式</h3>
        <p class="hint-text">点击设备图标查看设备详细信息</p>
      </div>
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
            @mousedown="handleMouseDown($event, index)"
            @click="handleDeviceClick(placedDevice.id)"
          >
            <div class="device-marker">
              <div class="device-status" :class="placedDevice.status"></div>
              <div class="device-info">
                <div class="device-name">{{ placedDevice.name }}</div>
                <div class="device-id">ID: {{ placedDevice.id }}</div>
              </div>
              <div class="device-close" @mousedown.stop @click.stop="removePlacedDevice(index)">×</div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
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
  getDevicePositions,
  addDevicePosition,
  updateDevicePosition,
  removeDevicePosition,
  isFullscreen,
  setFullscreen,
} = useDeviceStore();

// 已放置的设备（使用相对于图片原始尺寸的百分比）
interface PlacedDevice {
  id: number;
  name: string;
  status: string;
  x: number; // 0-100 百分比
  y: number; // 0-100 百分比
}
const placedDevices = ref<PlacedDevice[]>([]);

// 从store加载设备位置
const loadDevicePositions = () => {
  const positions = getDevicePositions();
  placedDevices.value = positions.map(position => {
    const device = devices.value.find(d => d.id === position.deviceId);
    if (device) {
      return {
        id: device.id,
        name: device.name,
        status: device.status,
        x: position.x,
        y: position.y
      };
    }
    return null;
  }).filter((device): device is PlacedDevice => device !== null);
};

// 组件挂载时加载设备位置
onMounted(() => {
  loadDevicePositions();
  
  // 检查是否需要进入全屏状态
  if (route.query.fullscreen === 'true') {
    setFullscreen(true);
  }
});

// 监听路由参数变化
watch(() => route.query.fullscreen, (newFullscreen) => {
  if (newFullscreen === 'true') {
    setFullscreen(true);
  } else if (newFullscreen === 'false' || newFullscreen === undefined || newFullscreen === null) {
    setFullscreen(false);
  }
}, { immediate: true });

const draggedDevice = ref<{ id: number; name: string; status: string } | null>(null);

// 正在拖动的设备索引
const draggingIndex = ref<number | null>(null);
// 拖动起始位置
const dragStart = ref({ x: 0, y: 0 });
// 是否正在拖动
const isDragging = ref(false);
// 是否刚刚完成了拖动
const justDragged = ref(false);

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

// 退出全屏
const exitFullscreen = () => {
  setFullscreen(false);
  // 清除URL中的fullscreen参数
  router.replace({ query: {} });
};

// 拖拽开始
const handleDragStart = (event: DragEvent, device: { id: number; name: string; status: string }) => {
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
    status: draggedDevice.value.status,
    x: x,
    y: y
  });

  // 保存设备位置到store
  addDevicePosition({
    deviceId: draggedDevice.value.id,
    x: x,
    y: y
  });

  draggedDevice.value = null;
  ElMessage.success('设备放置成功');
};

// 移除放置的设备
const removePlacedDevice = (index: number) => {
  const device = placedDevices.value[index];
  if (device) {
    // 从store中删除设备位置
    removeDevicePosition(device.id);
  }
  placedDevices.value.splice(index, 1);
  ElMessage.success('设备已移除');
};

// 点击设备图标跳转到数据界面
const handleDeviceClick = (deviceId: number) => {
  // 如果正在拖动，则不执行点击操作
  if (isDragging.value) {
    return;
  }
  
  // 如果刚刚完成了拖动，则不执行点击操作
  if (justDragged.value) {
    return;
  }
  
  // 只在全屏状态下才允许跳转
  if (!isFullscreen.value) {
    return;
  }
  
  // 跳转到数据界面
  router.push({
    path: '/Data',
    query: { deviceId: deviceId.toString() }
  });
};

// 鼠标按下开始拖动
const handleMouseDown = (event: MouseEvent, index: number) => {
  // 防止事件冒泡，避免触发其他元素的事件
  event.stopPropagation();
  
  // 开始拖动
  draggingIndex.value = index;
  dragStart.value = {
    x: event.clientX,
    y: event.clientY
  };
  isDragging.value = false;
  
  // 添加鼠标移动和释放事件监听器
  document.addEventListener('mousemove', handleMouseMove);
  document.addEventListener('mouseup', handleMouseUp);
};

// 鼠标移动处理
const handleMouseMove = (event: MouseEvent) => {
  if (draggingIndex.value === null) return;
  
  // 检测是否发生了移动
  const moveDistance = Math.sqrt(
    Math.pow(event.clientX - dragStart.value.x, 2) + 
    Math.pow(event.clientY - dragStart.value.y, 2)
  );
  
  // 如果移动距离超过5像素，则认为是拖动操作
  if (moveDistance > 5) {
    isDragging.value = true;
  }
  
  const imgElement = imageRef.value;
  if (!imgElement) return;
  
  const imgRect = imgElement.getBoundingClientRect();
  
  // 计算新位置
  const newX = ((event.clientX - imgRect.left) / imgRect.width) * 100;
  const newY = ((event.clientY - imgRect.top) / imgRect.height) * 100;
  
  // 边界检查
  const clampedX = Math.max(0, Math.min(100, newX));
  const clampedY = Math.max(0, Math.min(100, newY));
  
  // 更新设备位置
  const device = placedDevices.value[draggingIndex.value];
  if (device) {
    device.x = clampedX;
    device.y = clampedY;
  }
};

// 鼠标释放结束拖动
const handleMouseUp = () => {
  // 移除事件监听器
  document.removeEventListener('mousemove', handleMouseMove);
  document.removeEventListener('mouseup', handleMouseUp);
  
  // 如果正在拖动，更新设备位置到store
  if (draggingIndex.value !== null && isDragging.value) {
    const device = placedDevices.value[draggingIndex.value];
    if (device) {
      updateDevicePosition(device.id, device.x, device.y);
    }
    // 标记刚刚完成了拖动
    justDragged.value = true;
    // 100ms后清除标记，允许点击事件
    setTimeout(() => {
      justDragged.value = false;
    }, 100);
  }
  
  // 结束拖动
  draggingIndex.value = null;
  isDragging.value = false;
};
</script>

<style lang="scss" scoped>
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

.header-content {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
}

.hint-text {
  margin: 5px 0 0 0;
  font-size: 12px;
  color: #909399;
  font-weight: normal;
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

.device-marker {
  background: rgba(255, 255, 255, 0.7);
  border: 1px solid #409eff;
  border-radius: 6px;
  padding: 4px 8px;
  box-shadow: 0 1px 6px 0 rgba(0, 0, 0, 0.1);
  min-width: 80px;
  max-width: 120px;
  display: flex;
  align-items: center;
  gap: 6px;
  z-index: 100;
  font-size: 10px;
  line-height: 1.2;
}

.device-status {
  width: 6px;
  height: 6px;
  border-radius: 50%;
  flex-shrink: 0;
  position: relative;
}

.device-status::after {
  content: '';
  position: absolute;
  top: 50%;
  left: 50%;
  transform: translate(-50%, -50%);
  width: 10px;
  height: 10px;
  border-radius: 50%;
  opacity: 0.5;
}

.device-status.online {
  background-color: #67c23a;
}

.device-status.online::after {
  background-color: #67c23a;
  box-shadow: 0 0 6px #67c23a;
}

.device-status.offline {
  background-color: #f56c6c;
}

.device-status.offline::after {
  background-color: #f56c6c;
  box-shadow: 0 0 6px #f56c6c;
}

.device-info {
  flex: 1;
  min-width: 0;
}

.device-name {
  font-weight: 600;
  font-size: 10px;
  color: #303133;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
}

.device-id {
  font-size: 9px;
  color: #909399;
  white-space: nowrap;
  overflow: hidden;
  text-overflow: ellipsis;
  margin-top: 1px;
}

.device-close {
  font-size: 12px;
  color: #909399;
  cursor: pointer;
  flex-shrink: 0;
  padding: 0 3px;
  border-radius: 3px;
  transition: all 0.3s;
  line-height: 1;
}

.device-close:hover {
  background-color: #f5f7fa;
  color: #606266;
}

/* 拖动时的样式 */
.placed-device:active {
  cursor: grabbing;
}

.placed-device {
  cursor: grab;
  user-select: none;
}

.placed-device:active .device-marker {
  box-shadow: 0 2px 12px 0 rgba(64, 158, 255, 0.3);
  border-color: #66b1ff;
  background: rgba(255, 255, 255, 0.8);
}
</style>