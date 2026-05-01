<template>
  <div class="home-container" :class="{ 'mobile': isMobile }">
    <div v-if="loading" class="loading-overlay">
      <el-icon class="loading-icon"><Loading /></el-icon>
      <p>加载鸟场概览...</p>
    </div>

    <template v-else>
      <div class="home-card">
      <!-- KPI 指标行 -->
      <div class="kpi-row">
        <div class="kpi-card purple" @click="goToDeviceManagement">
          <div class="kpi-icon"><el-icon><Monitor /></el-icon></div>
          <div class="kpi-body">
            <p class="kpi-value">{{ deviceStats.online }}/{{ deviceStats.total }}</p>
            <p class="kpi-label">设备在线</p>
          </div>
        </div>
        <div class="kpi-card" :class="alertTotal === 0 ? 'green' : 'red'" @click="goToDeviceLogs">
          <div class="kpi-icon"><el-icon><Warning /></el-icon></div>
          <div class="kpi-body">
            <p class="kpi-value">{{ alertTotal }}</p>
            <p class="kpi-label">24h 告警</p>
          </div>
        </div>
        <div class="kpi-card orange" @click="goToBirdsPage">
          <div class="kpi-icon"><span class="bird-emoji">🐤</span></div>
          <div class="kpi-body">
            <p class="kpi-value">{{ birdsTotal }}</p>
            <p class="kpi-label">在养雏鸟</p>
          </div>
        </div>
        <div class="kpi-card aqi" :class="aqiLevelClass">
          <div class="kpi-icon"><span class="aqi-icon-text">AQI</span></div>
          <div class="kpi-body">
            <p class="kpi-value">{{ todayAQI }}</p>
            <p class="kpi-label">今日 AQI</p>
          </div>
        </div>
        <div class="kpi-card temp-card">
          <div class="kpi-icon"><span class="env-emoji">🌡</span></div>
          <div class="kpi-body">
            <p class="kpi-value">{{ avgTemp }}°C</p>
            <p class="kpi-label">平均温度</p>
          </div>
        </div>
        <div class="kpi-card humid-card">
          <div class="kpi-icon"><span class="env-emoji">💧</span></div>
          <div class="kpi-body">
            <p class="kpi-value">{{ avgHumidity }}%</p>
            <p class="kpi-label">平均湿度</p>
          </div>
        </div>
      </div>

      <!-- 鸟笼快照网格 -->
      <div class="section-header">
        <h3 class="section-title">鸟笼实时快照</h3>
        <span class="section-subtitle">点击卡片查看详细趋势</span>
      </div>
      <div class="cage-grid">
        <div
          v-for="cage in cageSnapshots"
          :key="cage.label"
          class="cage-card"
          :class="{ selected: selectedCage?.label === cage.label, warning: cage.hasWarning }"
          @click="selectCage(cage)"
        >
          <div class="cage-card-header">
            <span class="cage-label">{{ cage.label }}</span>
            <span class="cage-birds">{{ cage.birdCount }} 只</span>
          </div>
          <div class="cage-sensors">
            <div class="cage-sensor-item temp">
              <span class="sensor-icon">🌡</span>
              <span class="sensor-value">{{ cage.temperature !== null ? cage.temperature + '°C' : '--' }}</span>
            </div>
            <div class="cage-sensor-item humid">
              <span class="sensor-icon">💧</span>
              <span class="sensor-value">{{ cage.humidity !== null ? cage.humidity + '%' : '--' }}</span>
            </div>
            <div class="cage-sensor-item pm25">
              <span class="sensor-label">PM2.5</span>
              <span class="sensor-value">{{ cage.pm25 !== null ? cage.pm25 : '--' }}</span>
            </div>
            <div class="cage-sensor-item db">
              <span class="sensor-label">dB</span>
              <span class="sensor-value">{{ cage.db !== null ? cage.db : '--' }}</span>
            </div>
            <div class="cage-sensor-item lux">
              <span class="sensor-label">Lux</span>
              <span class="sensor-value">{{ cage.lux !== null ? cage.lux : '--' }}</span>
            </div>
            <div class="cage-sensor-item uv">
              <span class="sensor-label">UV</span>
              <span class="sensor-value">{{ cage.uv !== null ? cage.uv : '--' }}</span>
            </div>
          </div>
        </div>
        <div v-if="cageSnapshots.length === 0" class="no-cages">
          <p>暂无鸟笼数据，请先配置设备和鸟笼</p>
        </div>
      </div>

      <!-- 趋势图表 -->
      <div v-if="selectedCage" class="section-header chart-section-header">
        <h3 class="section-title">{{ selectedCage.label }} 趋势</h3>
      </div>
      <div v-if="selectedCage" class="chart-section">
        <div class="chart-card">
          <div class="chart-header">
            <h4 class="chart-title">温度 / 湿度 <span class="time-range">(最近24小时)</span></h4>
          </div>
          <div ref="tempHumidityChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <div class="chart-header">
            <h4 class="chart-title">空气质量 / 声音 / 光照 / 紫外线 <span class="time-range">(最近24小时)</span></h4>
          </div>
          <div ref="envChartRef" class="chart"></div>
        </div>
      </div>
      </div>
    </template>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue';
import { useRouter } from 'vue-router';
import { useDeviceStore } from '../stores/deviceStore';
import { ElIcon } from 'element-plus';
import { Monitor, Warning, Loading } from '@element-plus/icons-vue';
import * as echarts from 'echarts';
import { api } from '../utils/api';

const router = useRouter();
const {
  devices,
  deviceHistoryData,
  getDeviceLogs,
  getOrUpdateDevices,
  fetchDeviceHistoryData,
  fetchDeviceLogs,
  fetchSensorData,
} = useDeviceStore();

const isMobile = ref(window.innerWidth < 768);
const loading = ref(true);

interface Bird {
  id: number;
  name: string;
  species: string;
  birth_date: string;
  status: string;
  area?: string;
  number?: number;
}

interface BirdcageGroup {
  area: string;
  number: number;
  label: string;
  devices: DeviceItem[];
  cam_device: DeviceItem | null;
  c3_device: DeviceItem | null;
}

interface DeviceItem {
  id: number;
  name: string;
  device_type: string;
  area: string;
  number: number;
  isOnline: boolean;
  status: string;
}

interface CageSnapshot {
  area: string;
  number: number;
  label: string;
  birdCount: number;
  camDeviceId: number | null;
  c3DeviceId: number | null;
  temperature: number | null;
  humidity: number | null;
  pm25: number | null;
  db: number | null;
  lux: number | null;
  uv: number | null;
  hasWarning: boolean;
}

const birdcageGroups = ref<BirdcageGroup[]>([]);
const birds = ref<Bird[]>([]);
const selectedCage = ref<CageSnapshot | null>(null);

const tempHumidityChartRef = ref<HTMLElement | null>(null);
const envChartRef = ref<HTMLElement | null>(null);
let tempHumidityChart: echarts.ECharts | null = null;
let envChart: echarts.ECharts | null = null;

const deviceStats = computed(() => {
  return {
    total: devices.value.length,
    online: devices.value.filter(d => d.isOnline).length,
  };
});

const alertTotal = computed(() => {
  const allLogs = getDeviceLogs();
  return allLogs.filter(log => log.level === 'WARNING' || log.level === 'ERROR').length;
});

const birdsTotal = computed(() => birds.value.length);

const avgTemp = computed(() => {
  const cages = cageSnapshots.value.filter(c => c.temperature !== null);
  if (cages.length === 0) return '--';
  const avg = cages.reduce((s, c) => s + c.temperature!, 0) / cages.length;
  return avg.toFixed(1);
});

const avgHumidity = computed(() => {
  const cages = cageSnapshots.value.filter(c => c.humidity !== null);
  if (cages.length === 0) return '--';
  const avg = cages.reduce((s, c) => s + c.humidity!, 0) / cages.length;
  return avg.toFixed(1);
});

const todayAQI = computed(() => {
  const pm25Values = cageSnapshots.value
    .filter(c => c.pm25 !== null)
    .map(c => c.pm25!);
  if (pm25Values.length === 0) return '--';
  const avg = pm25Values.reduce((s, v) => s + v, 0) / pm25Values.length;
  return calculateAQI(avg, 0, 0, 0);
});

const aqiLevelClass = computed(() => {
  const val = todayAQI.value;
  if (val === '--') return '';
  const n = Number(val);
  if (n > 150) return 'aqi-bad';
  if (n > 100) return 'aqi-moderate';
  return 'aqi-good';
});

const calculateAQI = (pm25: number, pm10: number, _co2: number, _tvoc: number): number => {
  if (pm25 <= 0 && pm10 <= 0) return 0;
  const calc = (c: number, bp: number[], ip: number[]) => {
    if (c < 0) return -1;
    const idx = bp.findIndex(b => c <= b);
    if (idx < 0) return ip[ip.length - 1]!;
    if (idx === 0) return Math.round((ip[0]! / bp[0]!) * c);
    return Math.round(((ip[idx]! - ip[idx - 1]!) / (bp[idx]! - bp[idx - 1]!)) * (c - bp[idx - 1]!) + ip[idx - 1]!);
  };
  const pm25AQI = calc(pm25, [12, 35.4, 55.4, 150.4, 250.4, 350.4, 500.4], [50, 100, 150, 200, 300, 400, 500]);
  const pm10AQI = calc(pm10, [54, 154, 254, 354, 424, 504, 604], [50, 100, 150, 200, 300, 400, 500]);
  const values = [pm25AQI, pm10AQI].filter(v => v >= 0);
  if (values.length === 0) return 0;
  return Math.max(...values);
};

const cageSnapshots = ref<CageSnapshot[]>([]);

const buildCageSnapshots = () => {
  const prev = new Map<string, CageSnapshot>();
  for (const s of cageSnapshots.value) {
    prev.set(`${s.area}-${s.number}`, s);
  }

  const result: CageSnapshot[] = birdcageGroups.value.map(group => {
    const cageBirds = birds.value.filter(b => b.area === group.area && b.number === group.number);
    const camId = group.cam_device?.id ?? null;
    const c3Id = group.c3_device?.id ?? null;

    let temperature: number | null = null;
    let humidity: number | null = null;
    if (camId !== null) {
      const camData = deviceHistoryData.value
        .filter(d => d.deviceId === camId)
        .sort((a, b) => new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime());
      if (camData.length > 0) {
        temperature = camData[0]!.temperature;
        humidity = camData[0]!.humidity;
      }
    }

    const hasWarning = camId !== null
      ? devices.value.find(d => d.id === camId)?.status === 'warning' || devices.value.find(d => d.id === camId)?.status === 'error'
      : false;

    const key = `${group.area}-${group.number}`;
    const existing = prev.get(key);

    return {
      area: group.area,
      number: group.number,
      label: group.label,
      birdCount: cageBirds.length,
      camDeviceId: camId,
      c3DeviceId: c3Id,
      temperature,
      humidity,
      pm25: existing?.pm25 ?? null,
      db: existing?.db ?? null,
      lux: existing?.lux ?? null,
      uv: existing?.uv ?? null,
      hasWarning: !!hasWarning,
    };
  });

  cageSnapshots.value = result;
};

watch([devices, deviceHistoryData], () => buildCageSnapshots());

const selectCage = async (cage: CageSnapshot) => {
  selectedCage.value = cage;
  await initChartsForCage(cage);
};

const fetchBirdcageGroups = async () => {
  try {
    const response = await api.get('/api/devices/birdcage-groups');
    if (response.code === 200) {
      birdcageGroups.value = response.data || [];
    }
  } catch (error) {
    console.error('获取鸟笼分组失败:', error);
  }
};

const fetchBirds = async () => {
  try {
    const response = await api.get('/api/birds');
    if (response.code === 200) {
      birds.value = response.data || [];
    }
  } catch (error) {
    console.error('获取雏鸟列表失败:', error);
  }
};

const fetchC3SensorData = async () => {
  const snapshots = [...cageSnapshots.value];
  const cagePromises = snapshots.map(async (cage, i) => {
    if (!cage.c3DeviceId) return { index: i, cage };
    try {
      const results = await Promise.allSettled([
        api.get('/api/sensor-upload', { device_id: cage.c3DeviceId, sensor_type: 'pms9103m', limit: 1 }),
        api.get('/api/sensor-upload', { device_id: cage.c3DeviceId, sensor_type: 'sound_meter', limit: 1 }),
        api.get('/api/sensor-upload', { device_id: cage.c3DeviceId, sensor_type: 'veml7700', limit: 1 }),
        api.get('/api/sensor-upload', { device_id: cage.c3DeviceId, sensor_type: 'uv_meter', limit: 1 }),
      ]);
      const extractValue = (result: PromiseSettledResult<any>, key: string): number | null => {
        if (result.status !== 'fulfilled') return null;
        const r = result.value;
        if (r?.code === 200 && r.data && r.data.length > 0) {
          const parsed = typeof r.data[0].data === 'string' ? JSON.parse(r.data[0].data) : r.data[0].data;
          const val = Number(parsed[key]);
          return isNaN(val) ? null : val;
        }
        return null;
      };
      return {
        index: i,
        cage: {
          ...cage,
          pm25: extractValue(results[0]!, 'pm2_5_cf1'),
          db: extractValue(results[1]!, 'db'),
          lux: extractValue(results[2]!, 'lux'),
          uv: extractValue(results[3]!, 'uv_index'),
        },
      };
    } catch (error) {
      console.error(`获取 ${cage.label} C3 传感器数据失败:`, error);
      return { index: i, cage };
    }
  });

  const results = await Promise.allSettled(cagePromises);
  for (const result of results) {
    if (result.status === 'fulfilled') {
      const { index, cage: updatedCage } = result.value;
      snapshots[index] = updatedCage;
    }
  }

  const refArray = cageSnapshots.value;
  for (let i = 0; i < snapshots.length; i++) {
    refArray[i] = snapshots[i]!;
  }
};

const formatLocalISO = (date: Date): string => {
  const parts = [
    date.getFullYear(),
    String(date.getMonth() + 1).padStart(2, '0'),
    String(date.getDate()).padStart(2, '0'),
    String(date.getHours()).padStart(2, '0'),
    String(date.getMinutes()).padStart(2, '0'),
    String(date.getSeconds()).padStart(2, '0'),
    String(date.getMilliseconds()).padStart(3, '0'),
  ];
  return `${parts[0]}-${parts[1]}-${parts[2]}T${parts[3]}:${parts[4]}:${parts[5]}.${parts[6]}`;
};

const initChartsForCage = async (cage: CageSnapshot) => {
  if (!cage.camDeviceId && !cage.c3DeviceId) return;

  if (cage.camDeviceId) {
    const data = await fetchSensorData(cage.camDeviceId, 'today');
    initTempHumidityChart(data.times, data.temperatureValues, data.humidityValues);
  }

  if (cage.c3DeviceId) {
    const results = await Promise.allSettled([
      fetchSensorTimeSeries(cage.c3DeviceId, 'pms9103m', 'pm2_5_cf1'),
      fetchSensorTimeSeries(cage.c3DeviceId, 'sound_meter', 'db'),
      fetchSensorTimeSeries(cage.c3DeviceId, 'veml7700', 'lux'),
      fetchSensorTimeSeries(cage.c3DeviceId, 'uv_meter', 'uv_index'),
    ]);
    const extract = (r: PromiseSettledResult<{ times: string[]; values: number[] }>) =>
      r.status === 'fulfilled' ? r.value : { times: [] as string[], values: [] as number[] };
    const aqi = extract(results[0]!);
    const sound = extract(results[1]!);
    const lux = extract(results[2]!);
    const uv = extract(results[3]!);
    initEnvChart(aqi.times, aqi.values, sound.values, lux.values, uv.values);
  }
};

const fetchSensorTimeSeries = async (deviceId: number, sensorType: string, valueKey: string): Promise<{ times: string[]; values: number[] }> => {
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: deviceId,
      sensor_type: sensorType,
      limit: 200,
    });
    if (response.code === 200 && response.data && Array.isArray(response.data)) {
      const pairs: { time: string; value: number }[] = [];
      const cutoff = new Date();
      cutoff.setSeconds(cutoff.getSeconds() - 86400);
      for (const item of response.data) {
        const ts = new Date(item.timestamp + '+00:00');
        if (ts < cutoff || isNaN(ts.getTime())) continue;
        const msUTC8 = ts.getTime() + 8 * 3600 * 1000;
        const ts8 = new Date(msUTC8);
        let parsed: any;
        try {
          parsed = typeof item.data === 'string' ? JSON.parse(item.data) : item.data;
        } catch { continue; }
        if (parsed[valueKey] !== undefined && parsed[valueKey] !== null) {
          const hh = ts8.getUTCHours().toString().padStart(2, '0');
          const mm = ts8.getUTCMinutes().toString().padStart(2, '0');
          pairs.push({ time: `${hh}:${mm}`, value: Number(parsed[valueKey]) });
        }
      }
      pairs.reverse();
      return { times: pairs.map(p => p.time), values: pairs.map(p => p.value) };
    }
  } catch (error) {
    console.error(`获取${sensorType}时间序列失败:`, error);
  }
  return { times: [], values: [] };
};

const initTempHumidityChart = (times: string[], tempValues: number[], humidValues: number[]) => {
  if (tempHumidityChartRef.value) {
    if (tempHumidityChart) tempHumidityChart.dispose();
    tempHumidityChart = echarts.init(tempHumidityChartRef.value);
    tempHumidityChart.setOption({
      tooltip: { trigger: 'axis' },
      legend: { data: ['温度', '湿度'], top: 10 },
      xAxis: { type: 'category', data: times },
      yAxis: { type: 'value', name: '数值' },
      series: [
        { name: '温度', data: tempValues, type: 'line', smooth: true, itemStyle: { color: '#ff7875' } },
        { name: '湿度', data: humidValues, type: 'line', smooth: true, itemStyle: { color: '#69c0ff' } },
      ],
    });
  }
};

const initEnvChart = (times: string[], pm25Values: number[], dbValues: number[], luxValues: number[], uvValues: number[]) => {
  if (envChartRef.value) {
    if (envChart) envChart.dispose();
    envChart = echarts.init(envChartRef.value);
    envChart.setOption({
      tooltip: { trigger: 'axis' },
      legend: { data: ['PM2.5', 'dB', 'Lux', 'UV'], top: 10, type: 'scroll' },
      xAxis: { type: 'category', data: times },
      yAxis: [
        { type: 'value', name: 'PM2.5 / dB' },
        { type: 'value', name: 'Lux / UV' },
      ],
      series: [
        { name: 'PM2.5', data: pm25Values, type: 'line', smooth: true, itemStyle: { color: '#ff7875' } },
        { name: 'dB', data: dbValues, type: 'line', smooth: true, itemStyle: { color: '#69c0ff' } },
        { name: 'Lux', data: luxValues, type: 'line', smooth: true, yAxisIndex: 1, itemStyle: { color: '#ffc53d' } },
        { name: 'UV', data: uvValues, type: 'line', smooth: true, yAxisIndex: 1, itemStyle: { color: '#b37feb' } },
      ],
    });
  }
};

const goToDeviceManagement = () => router.push({ path: '/Device', query: { tab: 'management' } });
const goToDeviceLogs = () => router.push({ path: '/Device', query: { tab: 'logs' } });
const goToBirdsPage = () => router.push({ path: '/birds' });
const goToDataAnalysis = (cage?: CageSnapshot) => {
  router.push({ path: '/Data', query: { activeTab: 'analysis' } });
};

const handleResize = () => {
  isMobile.value = window.innerWidth < 768;
  tempHumidityChart?.resize();
  envChart?.resize();
};

onMounted(() => {
  window.addEventListener('resize', handleResize);
  loading.value = true;

  const devicePromise = getOrUpdateDevices();
  const groupsPromise = fetchBirdcageGroups();
  const birdsPromise = fetchBirds();
  const historyPromise = fetchDeviceHistoryData();

  devicePromise.then(async () => {
    const now = new Date();
    const startTime = formatLocalISO(new Date(now.getTime() - 24 * 60 * 60 * 1000));
    const endTime = formatLocalISO(now);
    const deviceIds = devices.value.map(d => d.id);
    if (deviceIds.length > 0) {
      fetchDeviceLogs(deviceIds, 0, 100, ['WARNING', 'ERROR'], startTime, endTime)
        .catch(err => console.error('获取告警日志失败:', err));
    }
  });

  Promise.all([groupsPromise, birdsPromise, historyPromise])
    .then(() => {
      buildCageSnapshots();
      return fetchC3SensorData();
    })
    .then(() => {
      loading.value = false;
      if (cageSnapshots.value.length > 0) {
        selectCage(cageSnapshots.value[0]!);
      }
    });
});

onUnmounted(() => {
  window.removeEventListener('resize', handleResize);
  tempHumidityChart?.dispose();
  envChart?.dispose();
});
</script>

<style scss scoped>
.home-container {
  position: relative;
  z-index: 1;
  width: 90%;
  max-width: 1200px;
  margin: 2% auto;
  padding: 20px;

  &.mobile {
    padding: 12px;
    width: 100%;
    max-width: 100%;
    margin: 0;
    box-sizing: border-box;
  }
}

.home-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(12px);
  -webkit-backdrop-filter: blur(12px);
  border-radius: 16px;
  padding: 24px;
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(255, 255, 255, 0.6);
}

.mobile .home-card {
  background: transparent;
  backdrop-filter: none;
  -webkit-backdrop-filter: none;
  border-radius: 0;
  padding: 0;
  box-shadow: none;
  border: none;
}

.loading-overlay {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 60vh;
  color: #909399;

  .loading-icon {
    font-size: 40px;
    animation: spin 1s linear infinite;
    margin-bottom: 16px;
  }
}

@keyframes spin {
  from { transform: rotate(0deg); }
  to { transform: rotate(360deg); }
}

/* KPI 行 */
.kpi-row {
  display: grid;
  grid-template-columns: repeat(6, 1fr);
  gap: 12px;
  margin-bottom: 24px;

  .mobile & {
    grid-template-columns: repeat(3, 1fr);
    gap: 8px;
  }
}

.kpi-card {
  border-radius: 12px;
  padding: 14px 16px;
  color: white;
  cursor: pointer;
  display: flex;
  align-items: center;
  gap: 12px;
  transition: transform 0.2s, box-shadow 0.2s;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);

  &:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.12);
  }

  &.purple { background: linear-gradient(135deg, #667eea, #764ba2); }
  &.orange { background: linear-gradient(135deg, #f09433, #e6683c); }
  &.red { background: linear-gradient(135deg, #f093fb, #f5576c); }
  &.green { background: linear-gradient(135deg, #43e97b, #38f9d7); color: #1a3c34; }
  &.aqi-good { background: linear-gradient(135deg, #43e97b, #38f9d7); color: #1a3c34; }
  &.aqi-moderate { background: linear-gradient(135deg, #f6d365, #fda085); }
  &.aqi-bad { background: linear-gradient(135deg, #f093fb, #f5576c); }
  &.temp-card { background: linear-gradient(135deg, #ff9a9e, #fecfef); color: #5c1a1a; }
  &.humid-card { background: linear-gradient(135deg, #a1c4fd, #c2e9fb); color: #1a3a5c; }

  .mobile & {
    padding: 10px 12px;
    gap: 8px;
  }
}

.kpi-icon {
  width: 40px;
  height: 40px;
  border-radius: 10px;
  background: rgba(255, 255, 255, 0.2);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 20px;
  flex-shrink: 0;

  .mobile & {
    width: 32px;
    height: 32px;
    font-size: 16px;
  }
}

.bird-emoji, .env-emoji {
  font-size: 20px;
  line-height: 1;

  .mobile & { font-size: 16px; }
}

.aqi-icon-text {
  font-size: 14px;
  font-weight: 700;
  letter-spacing: -0.5px;

  .mobile & { font-size: 11px; }
}

.kpi-body {
  min-width: 0;
}

.kpi-value {
  font-size: 22px;
  font-weight: 700;
  margin: 0;
  line-height: 1.2;
  white-space: nowrap;

  .mobile & { font-size: 16px; }
}

.kpi-label {
  font-size: 12px;
  opacity: 0.85;
  margin: 2px 0 0;
  white-space: nowrap;

  .mobile & { font-size: 10px; }
}

/* 区域标题 */
.section-header {
  display: flex;
  align-items: baseline;
  gap: 12px;
  margin-bottom: 12px;
}

.section-title {
  font-size: 16px;
  font-weight: 600;
  color: #303133;
  margin: 0;
}

.section-subtitle {
  font-size: 12px;
  color: #909399;
}

.chart-section-header {
  margin-top: 24px;
}

/* 鸟笼快照网格 */
.cage-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
  gap: 12px;
  margin-bottom: 24px;

  .mobile & {
    grid-template-columns: repeat(2, 1fr);
    gap: 8px;
  }
}

.cage-card {
  border: 2px solid #ebeef5;
  border-radius: 12px;
  padding: 14px;
  cursor: pointer;
  transition: all 0.2s;
  background: white;

  &:hover {
    border-color: #667eea;
    box-shadow: 0 2px 12px rgba(102, 126, 234, 0.15);
  }

  &.selected {
    border-color: #667eea;
    box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.15);
  }

  &.warning {
    border-color: #f56c6c;
    background: #fef0f0;
  }

  .mobile & {
    padding: 10px;
  }
}

.cage-card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 10px;
  padding-bottom: 8px;
  border-bottom: 1px solid #f2f3f5;
}

.cage-label {
  font-size: 15px;
  font-weight: 600;
  color: #303133;
}

.cage-birds {
  font-size: 13px;
  color: #606266;
  background: #f5f7fa;
  padding: 2px 8px;
  border-radius: 10px;
}

.cage-sensors {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 6px;
}

.cage-sensor-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 7px 4px;
  border-radius: 8px;
  background: #f5f7fa;

  &.temp { background: #fff1f0; }
  &.humid { background: #e6f7ff; }
  &.pm25 { background: #fff7e6; }
  &.db { background: #f6ffed; }
  &.lux { background: #f9f0ff; }
  &.uv { background: #fff0f6; }

  .mobile & { padding: 5px 2px; }
}

.sensor-icon {
  font-size: 14px;
  line-height: 1;

  .mobile & { font-size: 12px; }
}

.sensor-label {
  font-size: 10px;
  color: #909399;
  margin-bottom: 2px;
}

.sensor-value {
  font-size: 13px;
  font-weight: 600;
  color: #303133;

  .mobile & { font-size: 11px; }
}

.no-cages {
  grid-column: 1 / -1;
  text-align: center;
  padding: 40px;
  color: #909399;
}

/* 图表区域 */
.chart-section {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 16px;

  .mobile & {
    grid-template-columns: 1fr;
  }
}

.chart-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.chart-header {
  margin-bottom: 12px;
}

.chart-title {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
  margin: 0;

  .time-range {
    font-size: 11px;
    font-weight: 400;
    color: #909399;
    margin-left: 6px;
  }
}

.chart {
  width: 100%;
  height: 340px;

  .mobile & {
    height: 240px;
  }
}
</style>
