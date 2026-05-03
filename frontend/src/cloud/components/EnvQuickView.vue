<template>
  <div class="env-quick-view" :class="{ 'fade-in': isLoaded }">
    <div class="card-inner">
      <h3 class="env-title">环境数据</h3>
      <div v-if="!hasDevice" class="no-device-alert">
        <span class="alert-icon">📦</span>
        <span>雏鸟尚未绑定鸟笼设备，无法获取环境数据</span>
      </div>
      <div v-else class="env-grid">
        <div class="env-item" v-for="item in envItems" :key="item.label">
          <div class="env-value-wrapper">
            <span class="env-value" :style="{ color: item.color }">{{ item.value }}</span>
            <span class="env-unit">{{ item.unit }}</span>
          </div>
          <span class="env-label">
            <span class="env-label-icon">{{ item.icon }}</span>
            {{ item.label }}
          </span>
          <div class="env-status-bar">
            <div
              class="env-status-fill"
              :style="{
                width: item.percent + '%',
                background: item.color
              }"
            ></div>
          </div>
        </div>
      </div>
      <div v-if="hasDevice" class="env-update-time">
        上次更新：{{ lastUpdateTime }}
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted } from 'vue';
import { api } from '@/utils/api';

const props = defineProps<{
  camDeviceId: string | null;
}>();

const isLoaded = ref(false);
const tempValue = ref(0);
const humidityValue = ref(0);
const lastUpdateTime = ref('--');
let fetchTimer: number | null = null;

const hasDevice = computed(() => !!props.camDeviceId);

const getTempColor = (temp: number) => {
  if (temp > 35) return '#f56c6c';
  if (temp > 28) return '#e6a23c';
  if (temp > 18) return '#67c23a';
  if (temp > 10) return '#e6a23c';
  return '#409eff';
};

const getHumidityColor = (hum: number) => {
  if (hum >= 40 && hum <= 70) return '#67c23a';
  if (hum >= 30 && hum <= 80) return '#e6a23c';
  return '#f56c6c';
};

const envItems = computed(() => [
  {
    label: '温度',
    icon: '🌡️',
    value: tempValue.value.toFixed(1),
    unit: '°C',
    color: getTempColor(tempValue.value),
    percent: Math.min((tempValue.value / 50) * 100, 100)
  },
  {
    label: '湿度',
    icon: '💧',
    value: humidityValue.value.toFixed(1),
    unit: '%',
    color: getHumidityColor(humidityValue.value),
    percent: Math.min(humidityValue.value, 100)
  }
]);

const fetchEnvData = async () => {
  if (!props.camDeviceId) return;
  try {
    const response = await api.get('/api/sensors/grouped', {
      period: '300',
      group: '1',
      device_id: props.camDeviceId.toString(),
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const latest = response.data[0];
      if (latest.avg_temperature !== undefined && latest.avg_temperature !== null) {
        tempValue.value = Math.round(Number(latest.avg_temperature) * 10) / 10;
      }
      if (latest.avg_humidity !== undefined && latest.avg_humidity !== null) {
        humidityValue.value = Math.round(Number(latest.avg_humidity) * 10) / 10;
      }
      lastUpdateTime.value = new Date().toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit', second: '2-digit' });
    }
  } catch (error) {
    console.error('获取环境数据失败:', error);
  }
};

onMounted(() => {
  setTimeout(() => {
    isLoaded.value = true;
  }, 300);

  if (props.camDeviceId) {
    fetchEnvData();
    fetchTimer = window.setInterval(fetchEnvData, 10000);
  }
});

onUnmounted(() => {
  if (fetchTimer) {
    clearInterval(fetchTimer);
    fetchTimer = null;
  }
});
</script>

<style scoped>
.env-quick-view {
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.2s;
}

.env-quick-view.fade-in {
  opacity: 1;
  transform: translateY(0);
}

.card-inner {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(16px);
  border-radius: 20px;
  padding: 20px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(209, 213, 219, 0.3);
}

.env-title {
  font-size: 16px;
  font-weight: 700;
  color: #166534;
  margin: 0 0 16px 0;
}

.no-device-alert {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px 16px;
  background: linear-gradient(135deg, #fffbeb 0%, #fef3c7 100%);
  border: 1px solid rgba(245, 158, 11, 0.3);
  border-radius: 12px;
  font-size: 13px;
  color: #92400e;
  line-height: 1.5;
}

.alert-icon {
  font-size: 18px;
  flex-shrink: 0;
}

.env-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
}

.env-item {
  padding: 16px;
  background: linear-gradient(135deg, rgba(22, 163, 74, 0.05), rgba(21, 128, 61, 0.05));
  border-radius: 14px;
  text-align: center;
  transition: all 0.3s ease;
}

.env-item:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(22, 163, 74, 0.1);
}

.env-value-wrapper {
  display: flex;
  align-items: baseline;
  justify-content: center;
  gap: 4px;
  margin-bottom: 6px;
}

.env-value {
  font-size: 32px;
  font-weight: 800;
  line-height: 1;
  transition: color 0.3s ease;
}

.env-unit {
  font-size: 14px;
  font-weight: 600;
  color: #6b7280;
}

.env-label {
  font-size: 13px;
  color: #6b7280;
  font-weight: 500;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
  margin-bottom: 8px;
}

.env-label-icon {
  font-size: 14px;
}

.env-status-bar {
  height: 4px;
  background: #e5e7eb;
  border-radius: 2px;
  overflow: hidden;
}

.env-status-fill {
  height: 100%;
  border-radius: 2px;
  transition: width 0.6s ease, background 0.3s ease;
}

.env-update-time {
  text-align: center;
  font-size: 12px;
  color: #9ca3af;
  margin-top: 12px;
}

@media (max-width: 480px) {
  .env-value {
    font-size: 28px;
  }

  .env-item {
    padding: 14px 12px;
  }
}
</style>
