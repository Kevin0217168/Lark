<template>
  <div v-if="!isMobile" class="aq-detail-card">
    <!-- 头部 -->
    <div class="aq-header">
      <div class="aq-header-left">
        <el-button type="primary" text @click="goBack">
          <el-icon><ArrowLeft /></el-icon>
          返回数据分析
        </el-button>
        <h2>空气质量详情</h2>
      </div>
      <div class="aq-header-right">
        <el-select v-model="selectedBirdcageKey" @change="handleBirdcageChange" class="device-select" placeholder="选择鸟笼">
          <el-option
            v-for="group in birdcageGroups"
            :key="`${group.area}-${group.number}`"
            :label="group.label"
            :value="`${group.area}|${group.number}`"
          />
        </el-select>
        <div class="time-range-selector">
          <button
            v-for="label in timeRangeLabels"
            :key="label.value"
            class="time-button"
            :class="{ active: timeRange === label.value }"
            @click="setTimeRange(label.value)"
          >
            {{ label.text }}
          </button>
        </div>
      </div>
    </div>

    <!-- 图表区域 -->
    <div class="aq-charts-content">
      <!-- PM 质量浓度 (cf1) -->
      <div class="chart-container">
        <h3>PM 质量浓度 (cf1) <span class="time-range">({{ timeRangeText }})</span></h3>
        <div ref="pmCf1ChartRef" class="chart"></div>
      </div>
      <!-- PM 质量浓度 (atm) -->
      <div class="chart-container">
        <h3>PM 质量浓度 (atm) <span class="time-range">({{ timeRangeText }})</span></h3>
        <div ref="pmStdChartRef" class="chart"></div>
      </div>
      <!-- 小粒径颗粒数 -->
      <div class="chart-container">
        <h3>小粒径颗粒数 <span class="time-range">({{ timeRangeText }})</span></h3>
        <div ref="smallParticlesChartRef" class="chart"></div>
      </div>
      <!-- 大粒径颗粒数 -->
      <div class="chart-container">
        <h3>大粒径颗粒数 <span class="time-range">({{ timeRangeText }})</span></h3>
        <div ref="largeParticlesChartRef" class="chart"></div>
      </div>
      <!-- CO2 浓度 -->
      <div class="chart-container">
        <h3>CO₂ 浓度 <span class="time-range">({{ timeRangeText }})</span></h3>
        <div ref="co2ChartRef" class="chart"></div>
      </div>
      <!-- TVOC 浓度 -->
      <div class="chart-container">
        <h3>TVOC 浓度 <span class="time-range">({{ timeRangeText }})</span></h3>
        <div ref="tvocChartRef" class="chart"></div>
      </div>
    </div>
  </div>

  <!-- 移动端 -->
  <div v-else class="mobile-aq-detail">
    <!-- 顶部导航栏 -->
    <div class="mobile-top-nav">
      <div class="nav-back" @click="goBack">
        <el-icon><ArrowLeft /></el-icon>
      </div>
      <h1 class="nav-title">空气质量详情</h1>
      <div class="nav-placeholder"></div>
    </div>

    <!-- 设备选择器 -->
    <div class="mobile-selector-bar">
      <el-select v-model="selectedBirdcageKey" @change="handleBirdcageChange" class="mobile-select" popper-class="mobile-select-dropdown" placeholder="选择鸟笼">
        <el-option
          v-for="group in birdcageGroups"
          :key="`${group.area}-${group.number}`"
          :label="group.label"
          :value="`${group.area}|${group.number}`"
        />
      </el-select>
    </div>

    <!-- 时间范围切换 -->
    <div class="mobile-time-tabs">
      <div
        v-for="label in timeRangeLabels"
        :key="label.value"
        class="time-tab"
        :class="{ active: timeRange === label.value }"
        @click="setTimeRange(label.value)"
      >
        {{ label.text }}
      </div>
    </div>

    <!-- 数据概览卡片 -->
    <div class="mobile-overview">
      <div class="overview-card aqi-card" :class="aqiLevelClass">
        <div class="overview-info">
          <div class="overview-value">{{ latestAqi ?? '--' }}</div>
          <div class="overview-label">AQI</div>
          <div class="overview-level" v-if="aqiLevelText">{{ aqiLevelText }}</div>
        </div>
      </div>
      <div class="overview-grid">
        <div class="overview-item">
          <div class="item-dot" style="background: #69c0ff;"></div>
          <div class="item-info">
            <div class="item-value">{{ latestPm25 ?? '--' }}</div>
            <div class="item-label">PM2.5 μg/m³</div>
          </div>
        </div>
        <div class="overview-item">
          <div class="item-dot" style="background: #f56c6c;"></div>
          <div class="item-info">
            <div class="item-value">{{ latestPm10 ?? '--' }}</div>
            <div class="item-label">PM10 μg/m³</div>
          </div>
        </div>
        <div class="overview-item">
          <div class="item-dot" style="background: #409eff;"></div>
          <div class="item-info">
            <div class="item-value">{{ latestCo2 ?? '--' }}</div>
            <div class="item-label">CO₂ ppm</div>
          </div>
        </div>
        <div class="overview-item">
          <div class="item-dot" style="background: #b37feb;"></div>
          <div class="item-info">
            <div class="item-value">{{ latestTvoc ?? '--' }}</div>
            <div class="item-label">TVOC ppb</div>
          </div>
        </div>
      </div>
    </div>

    <!-- 图表区域 -->
    <div class="mobile-chart-list">
      <!-- PM 浓度组合图表 -->
      <div class="mobile-chart-card">
        <div class="card-header">
          <div class="card-title">
            <span class="title-icon">💨</span>
            <span>PM 质量浓度</span>
          </div>
          <div class="card-unit">μg/m³</div>
        </div>
        <div class="card-tabs">
          <div
            class="card-tab"
            :class="{ active: pmTab === 'cf1' }"
            @click="pmTab = 'cf1'"
          >标准(cf1)</div>
          <div
            class="card-tab"
            :class="{ active: pmTab === 'atm' }"
            @click="pmTab = 'atm'"
          >大气(atm)</div>
        </div>
        <div v-show="pmTab === 'cf1'" ref="pmCf1ChartRef" class="chart"></div>
        <div v-show="pmTab === 'atm'" ref="pmStdChartRef" class="chart"></div>
      </div>

      <!-- 颗粒数组合图表 -->
      <div class="mobile-chart-card">
        <div class="card-header">
          <div class="card-title">
            <span class="title-icon">🔬</span>
            <span>颗粒数浓度</span>
          </div>
          <div class="card-unit">个/0.1L</div>
        </div>
        <div class="card-tabs">
          <div
            class="card-tab"
            :class="{ active: particleTab === 'small' }"
            @click="particleTab = 'small'"
          >小粒径</div>
          <div
            class="card-tab"
            :class="{ active: particleTab === 'large' }"
            @click="particleTab = 'large'"
          >大粒径</div>
        </div>
        <div v-show="particleTab === 'small'" ref="smallParticlesChartRef" class="chart"></div>
        <div v-show="particleTab === 'large'" ref="largeParticlesChartRef" class="chart"></div>
      </div>

      <!-- CO2 图表 -->
      <div class="mobile-chart-card">
        <div class="card-header">
          <div class="card-title">
            <span class="title-icon">🏭</span>
            <span>CO₂ 浓度</span>
          </div>
          <div class="card-unit">ppm</div>
        </div>
        <div ref="co2ChartRef" class="chart"></div>
      </div>

      <!-- TVOC 图表 -->
      <div class="mobile-chart-card">
        <div class="card-header">
          <div class="card-title">
            <span class="title-icon">🧪</span>
            <span>TVOC 浓度</span>
          </div>
          <div class="card-unit">ppb</div>
        </div>
        <div ref="tvocChartRef" class="chart"></div>
      </div>
    </div>

    <!-- 底部留白 -->
    <div class="mobile-bottom-space"></div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted, nextTick } from 'vue';
import { useRouter, useRoute } from 'vue-router';
import { useDeviceStore } from '../stores/deviceStore';
import { ElMessage } from 'element-plus';
import { ArrowLeft } from '@element-plus/icons-vue';
import { api } from '../utils/api';
import * as echarts from 'echarts';
import { shouldUseMobilePage } from '../utils/mobileAdapter';

const router = useRouter();
const route = useRoute();
const { getDevices, fetchDevices } = useDeviceStore();

const isMobile = ref(false);
const checkMobileStatus = () => {
  isMobile.value = shouldUseMobilePage();
};

const goBack = () => {
  router.push({
    path: '/Data',
    query: { activeTab: 'analysis', birdcage: selectedBirdcageKey.value }
  });
};

const birdcageGroups = computed(() => {
  const allDevices = getDevices() as any[];
  const groupMap = new Map<string, { area: string; number: number; label: string; devices: any[] }>();
  for (const d of allDevices) {
    const key = `${d.area || '未知'}-${d.number ?? '?'}`;
    if (!groupMap.has(key)) {
      groupMap.set(key, { area: d.area || '未知', number: d.number ?? 0, label: `${d.area || '未知'} #${d.number ?? '?'}`, devices: [] });
    }
    groupMap.get(key)!.devices.push(d);
  }
  return Array.from(groupMap.values()).sort((a, b) => a.label.localeCompare(b.label));
});

const selectedBirdcageKey = ref<string>('');

const c3DeviceId = computed(() => {
  if (!selectedBirdcageKey.value) return null;
  const allDevices = getDevices() as any[];
  const [area, numStr] = selectedBirdcageKey.value.split('|');
  const c3 = allDevices.find((d: any) =>
    d.area === area && d.number === Number(numStr) && d.device_type === 'ESP32-C3'
  );
  return c3?.id ?? null;
});

const handleBirdcageChange = async () => {
  if (!selectedBirdcageKey.value) return;
  const [area, numStr] = selectedBirdcageKey.value.split('|');
  ElMessage.success(`已选择鸟笼: ${area} #${numStr}`);
  await initCharts();
};

// 时间范围
const timeRange = ref<'today' | 'two_days'>('today');
const timeRangeLabels = [
  { text: '今日', value: 'today' as const },
  { text: '两天', value: 'two_days' as const }
];
const timeRangeText = computed(() => {
  return timeRange.value === 'today' ? '今日' : '两天';
});
const setTimeRange = (range: 'today' | 'two_days') => {
  timeRange.value = range;
  initCharts();
};

// 移动端标签页状态
const pmTab = ref<'cf1' | 'atm'>('cf1');
const particleTab = ref<'small' | 'large'>('small');

// 最新数据
const latestAqi = ref<number | null>(null);
const latestPm25 = ref<number | null>(null);
const latestPm10 = ref<number | null>(null);
const latestCo2 = ref<number | null>(null);
const latestTvoc = ref<number | null>(null);

const aqiLevelClass = computed(() => {
  const val = latestAqi.value;
  if (val === null) return '';
  if (val > 150) return 'aqi-bad';
  if (val > 100) return 'aqi-moderate';
  return 'aqi-good';
});

const aqiLevelText = computed(() => {
  const val = latestAqi.value;
  if (val === null) return '';
  if (val <= 50) return '优';
  if (val <= 100) return '良';
  if (val <= 150) return '轻度污染';
  if (val <= 200) return '中度污染';
  if (val <= 300) return '重度污染';
  return '严重污染';
});

// 图表引用
const pmCf1ChartRef = ref<HTMLElement | null>(null);
const pmStdChartRef = ref<HTMLElement | null>(null);
const smallParticlesChartRef = ref<HTMLElement | null>(null);
const largeParticlesChartRef = ref<HTMLElement | null>(null);
const co2ChartRef = ref<HTMLElement | null>(null);
const tvocChartRef = ref<HTMLElement | null>(null);

let pmCf1Chart: echarts.ECharts | null = null;
let pmStdChart: echarts.ECharts | null = null;
let smallParticlesChart: echarts.ECharts | null = null;
let largeParticlesChart: echarts.ECharts | null = null;
let co2Chart: echarts.ECharts | null = null;
let tvocChart: echarts.ECharts | null = null;

// 颜色配置
const areaColorMap: Record<string, string[]> = {
  '#ff7875': ['rgba(255, 120, 117, 0.3)', 'rgba(255, 120, 117, 0.05)'],
  '#69c0ff': ['rgba(105, 192, 255, 0.3)', 'rgba(105, 192, 255, 0.05)'],
  '#e6a23c': ['rgba(230, 162, 60, 0.3)', 'rgba(230, 162, 60, 0.05)'],
  '#909399': ['rgba(144, 147, 153, 0.3)', 'rgba(144, 147, 153, 0.05)'],
  '#67c23a': ['rgba(103, 194, 58, 0.3)', 'rgba(103, 194, 58, 0.05)'],
  '#f56c6c': ['rgba(245, 108, 108, 0.3)', 'rgba(245, 108, 108, 0.05)'],
  '#409eff': ['rgba(64, 158, 255, 0.3)', 'rgba(64, 158, 255, 0.05)'],
  '#b37feb': ['rgba(179, 127, 235, 0.3)', 'rgba(179, 127, 235, 0.05)'],
};

// 动态Y轴计算
const calcYAxisRange = (values: number[]) => {
  const validValues = values.filter(v => v !== undefined && v !== null);
  const vMin = validValues.length > 0 ? Math.min(...validValues) : 0;
  const vMax = validValues.length > 0 ? Math.max(...validValues) : 100;
  const range = vMax - vMin;
  let yMin = vMin;
  let yMax = vMax;
  if (range === 0) {
    yMin = Math.max(0, vMin - 1);
    yMax = vMax + 1;
  } else if (range < 5) {
    const padding = Math.max(range * 0.3, 0.2);
    yMin = Math.max(0, vMin - padding);
    yMax = vMax + padding;
  }
  return { min: yMin, max: yMax };
};

// AQI 计算
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

// 单线图表配置
const makeLineChartOption = (times: string[], values: number[], name: string, color: string, unit: string) => {
  const yRange = calcYAxisRange(values);
  return {
    tooltip: { trigger: 'axis' as const },
    xAxis: { type: 'category' as const, data: times },
    yAxis: { type: 'value' as const, name: unit, min: yRange.min, max: yRange.max },
    series: [{
      name,
      data: values,
      type: 'line' as const,
      smooth: true,
      itemStyle: { color },
      areaStyle: {
        color: {
          type: 'linear' as const,
          x: 0, y: 0, x2: 0, y2: 1,
          colorStops: [
            { offset: 0, color: (areaColorMap[color] || ['rgba(0,0,0,0.1)'])[0] },
            { offset: 1, color: (areaColorMap[color] || ['rgba(0,0,0,0.05)', 'rgba(0,0,0,0.01)'])[1] }
          ]
        }
      }
    }]
  };
};

// 多线图表配置
const makeMultiLineChartOption = (
  times: string[],
  seriesData: { name: string; values: number[]; color: string }[],
  unit: string
) => {
  const allValues = seriesData.flatMap(s => s.values);
  const yRange = calcYAxisRange(allValues);
  return {
    tooltip: { trigger: 'axis' as const },
    legend: { data: seriesData.map(s => s.name), top: 0 },
    xAxis: { type: 'category' as const, data: times },
    yAxis: { type: 'value' as const, name: unit, min: yRange.min, max: yRange.max },
    series: seriesData.map(s => ({
      name: s.name,
      data: s.values,
      type: 'line' as const,
      smooth: true,
      itemStyle: { color: s.color },
      areaStyle: {
        color: {
          type: 'linear' as const,
          x: 0, y: 0, x2: 0, y2: 1,
          colorStops: [
            { offset: 0, color: (areaColorMap[s.color] || ['rgba(0,0,0,0.1)'])[0] },
            { offset: 1, color: (areaColorMap[s.color] || ['rgba(0,0,0,0.05)', 'rgba(0,0,0,0.01)'])[1] }
          ]
        }
      }
    }))
  };
};

// 数据采样：均匀抽取数据点，减少移动端图标密度
const sampleData = (times: string[], values: number[], targetCount: number) => {
  if (times.length <= targetCount) return { times, values };
  const result: { times: string[]; values: number[] } = { times: [], values: [] };
  if (times.length === 0) return result;
  const step = (times.length - 1) / (targetCount - 1);
  for (let i = 0; i < targetCount; i++) {
    const idx = Math.min(Math.round(i * step), times.length - 1);
    result.times.push(times[idx]!);
    result.values.push(values[idx]!);
  }
  return result;
};

// 移动端图表配置 - 优化版
const makeMobileLineOption = (times: string[], values: number[], name: string, color: string, unit: string) => {
  const isTwoDays = timeRange.value === 'two_days';
  const targetPoints = isTwoDays ? 50 : 35;
  const sampled = sampleData(times, values, targetPoints);
  const yRange = calcYAxisRange(sampled.values);
  return {
    tooltip: {
      trigger: 'axis' as const,
      textStyle: { fontSize: 12 }
    },
    legend: {
      data: [name],
      top: 0,
      textStyle: { fontSize: 11, color: '#606266' },
      icon: 'roundRect',
      itemWidth: 14,
      itemHeight: 8
    },
    grid: {
      left: '16%',
      right: '8%',
      bottom: isTwoDays ? '22%' : '14%',
      top: '14%',
      containLabel: false
    },
    xAxis: {
      type: 'category' as const,
      data: sampled.times,
      axisLabel: {
        fontSize: 9,
        color: '#909399',
        rotate: isTwoDays ? 35 : 0,
        interval: isTwoDays ? 'auto' : 3
      },
      axisLine: { lineStyle: { color: '#e4e7ed' } }
    },
    yAxis: {
      type: 'value' as const,
      name: unit,
      nameTextStyle: {
        fontSize: 10,
        color: '#909399',
        padding: [0, 0, 4, 0]
      },
      axisLabel: {
        fontSize: 9,
        color: '#909399'
      },
      splitNumber: 4,
      splitLine: {
        lineStyle: { color: '#f0f0f0', type: 'dashed' }
      },
      min: yRange.min,
      max: yRange.max
    },
    series: [{
      name,
      data: sampled.values,
      type: 'line' as const,
      smooth: true,
      showSymbol: false,
      symbol: 'circle',
      symbolSize: 4,
      itemStyle: { color },
      lineStyle: { width: 2.5 },
      emphasis: { focus: 'series', showSymbol: true, symbolSize: 6 },
      areaStyle: {
        color: {
          type: 'linear' as const,
          x: 0, y: 0, x2: 0, y2: 1,
          colorStops: [
            { offset: 0, color: (areaColorMap[color] || ['rgba(0,0,0,0.1)'])[0] },
            { offset: 1, color: (areaColorMap[color] || ['rgba(0,0,0,0.05)', 'rgba(0,0,0,0.01)'])[1] }
          ]
        }
      }
    }]
  };
};

const makeMobileMultiLineOption = (
  times: string[],
  seriesData: { name: string; values: number[]; color: string }[],
  unit: string
) => {
  const isTwoDays = timeRange.value === 'two_days';
  const targetPoints = isTwoDays ? 50 : 35;
  const sampledTimes = sampleData(times, seriesData[0]?.values || [], targetPoints).times;
  const sampledSeries = seriesData.map(s => ({
    ...s,
    values: sampleData(times, s.values, targetPoints).values
  }));
  const allValues = sampledSeries.flatMap(s => s.values);
  const yRange = calcYAxisRange(allValues);
  return {
    tooltip: {
      trigger: 'axis' as const,
      textStyle: { fontSize: 12 }
    },
    legend: {
      data: sampledSeries.map(s => s.name),
      top: 0,
      textStyle: { fontSize: 10, color: '#606266' },
      icon: 'roundRect',
      itemWidth: 12,
      itemHeight: 6,
      itemGap: 8
    },
    grid: {
      left: '16%',
      right: '8%',
      bottom: isTwoDays ? '22%' : '14%',
      top: '16%',
      containLabel: false
    },
    xAxis: {
      type: 'category' as const,
      data: sampledTimes,
      axisLabel: {
        fontSize: 9,
        color: '#909399',
        rotate: isTwoDays ? 35 : 0,
        interval: isTwoDays ? 'auto' : 3
      },
      axisLine: { lineStyle: { color: '#e4e7ed' } }
    },
    yAxis: {
      type: 'value' as const,
      name: unit,
      nameTextStyle: {
        fontSize: 10,
        color: '#909399',
        padding: [0, 0, 4, 0]
      },
      axisLabel: {
        fontSize: 9,
        color: '#909399'
      },
      splitNumber: 4,
      splitLine: {
        lineStyle: { color: '#f0f0f0', type: 'dashed' }
      },
      min: yRange.min,
      max: yRange.max
    },
    series: sampledSeries.map(s => ({
      name: s.name,
      data: s.values,
      type: 'line' as const,
      smooth: true,
      showSymbol: false,
      symbol: 'circle',
      symbolSize: 3,
      itemStyle: { color: s.color },
      lineStyle: { width: 2 },
      emphasis: { focus: 'series', showSymbol: true, symbolSize: 5 },
      areaStyle: {
        color: {
          type: 'linear' as const, x: 0, y: 0, x2: 0, y2: 1,
          colorStops: [
            { offset: 0, color: (areaColorMap[s.color] || ['rgba(0,0,0,0.1)'])[0] },
            { offset: 1, color: (areaColorMap[s.color] || ['rgba(0,0,0,0.05)', 'rgba(0,0,0,0.01)'])[1] }
          ]
        }
      }
    }))
  };
};

const formatUTC8DateTime = (ms: number): string => {
  const d = new Date(ms);
  const pad = (n: number) => n.toString().padStart(2, '0');
  return `${d.getUTCFullYear()}-${pad(d.getUTCMonth() + 1)}-${pad(d.getUTCDate())}T${pad(d.getUTCHours())}:${pad(d.getUTCMinutes())}:${pad(d.getUTCSeconds())}`;
};

// 获取时间序列数据
const fetchPms9103mTimeSeries = async (deviceId: number, valueKeys: string[]): Promise<Record<string, { times: string[]; values: number[] }>> => {
  const result: Record<string, { times: string[]; values: number[] }> = {};
  for (const key of valueKeys) {
    result[key] = { times: [], values: [] };
  }
  try {
    const nowMs = Date.now();
    const secondsAgo = timeRange.value === 'today' ? 86400 : 172800;
    const startTime = formatUTC8DateTime(nowMs + 8 * 3600 * 1000 - secondsAgo * 1000);
    const response = await api.get('/api/sensor-upload', {
      device_id: deviceId,
      sensor_type: 'pms9103m',
      start_time: startTime,
      limit: 1000,
    });
    if (response.code === 200 && response.data && Array.isArray(response.data)) {
      const rawData: any[] = response.data;

      const grouped: Record<string, { time: string; value: number }[]> = {};
      for (const key of valueKeys) {
        grouped[key] = [];
      }

      for (const item of rawData) {
        const ts = new Date(item.timestamp + '+00:00');
        if (isNaN(ts.getTime())) continue;
        const msUTC8 = ts.getTime() + 8 * 3600 * 1000;
        const ts8 = new Date(msUTC8);
        let parsed: any;
        try {
          parsed = typeof item.data === 'string' ? JSON.parse(item.data) : item.data;
        } catch { continue; }

        for (const key of valueKeys) {
          if (parsed[key] !== undefined && parsed[key] !== null) {
            const timeStr = timeRange.value === 'two_days'
              ? `${(ts8.getUTCMonth() + 1).toString().padStart(2, '0')}/${ts8.getUTCDate().toString().padStart(2, '0')} ${ts8.getUTCHours().toString().padStart(2, '0')}:${ts8.getUTCMinutes().toString().padStart(2, '0')}`
              : `${ts8.getUTCHours().toString().padStart(2, '0')}:${ts8.getUTCMinutes().toString().padStart(2, '0')}`;
            grouped[key]!.push({ time: timeStr, value: Number(parsed[key]) });
          }
        }
      }

      for (const key of valueKeys) {
        const pairs = grouped[key]!.reverse();
        result[key] = { times: pairs.map(p => p.time), values: pairs.map(p => p.value) };
      }
    }
  } catch (error) {
    console.error('获取PMS9103M时间序列数据失败:', error);
  }
  return result;
};

const fetchSingleSensorTimeSeries = async (deviceId: number, sensorType: string, valueKey: string): Promise<{ times: string[]; values: number[] }> => {
  const pairs: { time: string; value: number }[] = [];
  try {
    const nowMs = Date.now();
    const secondsAgo = timeRange.value === 'today' ? 86400 : 172800;
    const startTime = formatUTC8DateTime(nowMs + 8 * 3600 * 1000 - secondsAgo * 1000);
    const response = await api.get('/api/sensor-upload', {
      device_id: deviceId,
      sensor_type: sensorType,
      start_time: startTime,
      limit: 1000,
    });
    if (response.code === 200 && response.data && Array.isArray(response.data)) {
      const rawData: any[] = response.data;

      for (const item of rawData) {
        const ts = new Date(item.timestamp + '+00:00');
        if (isNaN(ts.getTime())) continue;
        const msUTC8 = ts.getTime() + 8 * 3600 * 1000;
        const ts8 = new Date(msUTC8);
        let parsed: any;
        try {
          parsed = typeof item.data === 'string' ? JSON.parse(item.data) : item.data;
        } catch { continue; }
        if (parsed[valueKey] !== undefined && parsed[valueKey] !== null) {
          const timeStr = timeRange.value === 'two_days'
            ? `${(ts8.getUTCMonth() + 1).toString().padStart(2, '0')}/${ts8.getUTCDate().toString().padStart(2, '0')} ${ts8.getUTCHours().toString().padStart(2, '0')}:${ts8.getUTCMinutes().toString().padStart(2, '0')}`
            : `${ts8.getUTCHours().toString().padStart(2, '0')}:${ts8.getUTCMinutes().toString().padStart(2, '0')}`;
          pairs.push({ time: timeStr, value: Number(parsed[valueKey]) });
        }
      }
    }
  } catch (error) {
    console.error(`获取${sensorType}时间序列数据失败:`, error);
  }
  pairs.reverse();
  return { times: pairs.map(p => p.time), values: pairs.map(p => p.value) };
};

// 初始化桌面端图表
const initDesktopCharts = async () => {
  const c3Id = c3DeviceId.value;
  if (!c3Id) return;

  const pmData = await fetchPms9103mTimeSeries(c3Id, [
    'pm1_0_cf1', 'pm2_5_cf1', 'pm10_cf1',
    'pm1_0_atm', 'pm2_5_atm', 'pm10_atm'
  ]);

  if (pmCf1ChartRef.value) {
    pmCf1Chart?.dispose();
    pmCf1Chart = echarts.init(pmCf1ChartRef.value);
    pmCf1Chart.setOption(makeMultiLineChartOption(
      pmData['pm2_5_cf1']?.times || [],
      [
        { name: 'PM1.0', values: pmData['pm1_0_cf1']?.values || [], color: '#69c0ff' },
        { name: 'PM2.5', values: pmData['pm2_5_cf1']?.values || [], color: '#e6a23c' },
        { name: 'PM10', values: pmData['pm10_cf1']?.values || [], color: '#f56c6c' },
      ],
      'μg/m³'
    ));
  }

  if (pmStdChartRef.value) {
    pmStdChart?.dispose();
    pmStdChart = echarts.init(pmStdChartRef.value);
    pmStdChart.setOption(makeMultiLineChartOption(
      pmData['pm2_5_atm']?.times || [],
      [
        { name: 'PM1.0', values: pmData['pm1_0_atm']?.values || [], color: '#69c0ff' },
        { name: 'PM2.5', values: pmData['pm2_5_atm']?.values || [], color: '#e6a23c' },
        { name: 'PM10', values: pmData['pm10_atm']?.values || [], color: '#f56c6c' },
      ],
      'μg/m³'
    ));
  }

  const particleData = await fetchPms9103mTimeSeries(c3Id, [
    'cnt_0_3um', 'cnt_0_5um', 'cnt_1_0um',
    'cnt_2_5um', 'cnt_5_0um', 'cnt_10um'
  ]);

  if (smallParticlesChartRef.value) {
    smallParticlesChart?.dispose();
    smallParticlesChart = echarts.init(smallParticlesChartRef.value);
    smallParticlesChart.setOption(makeMultiLineChartOption(
      particleData['cnt_0_3um']?.times || [],
      [
        { name: '0.3μm', values: particleData['cnt_0_3um']?.values || [], color: '#409eff' },
        { name: '0.5μm', values: particleData['cnt_0_5um']?.values || [], color: '#67c23a' },
        { name: '1.0μm', values: particleData['cnt_1_0um']?.values || [], color: '#e6a23c' },
      ],
      '个/0.1L'
    ));
  }

  if (largeParticlesChartRef.value) {
    largeParticlesChart?.dispose();
    largeParticlesChart = echarts.init(largeParticlesChartRef.value);
    largeParticlesChart.setOption(makeMultiLineChartOption(
      particleData['cnt_2_5um']?.times || [],
      [
        { name: '2.5μm', values: particleData['cnt_2_5um']?.values || [], color: '#67c23a' },
        { name: '5.0μm', values: particleData['cnt_5_0um']?.values || [], color: '#e6a23c' },
        { name: '10μm', values: particleData['cnt_10um']?.values || [], color: '#f56c6c' },
      ],
      '个/0.1L'
    ));
  }

  const co2Data = await fetchSingleSensorTimeSeries(c3Id, 'sgp30', 'co2_ppm');
  if (co2ChartRef.value) {
    co2Chart?.dispose();
    co2Chart = echarts.init(co2ChartRef.value);
    co2Chart.setOption(makeLineChartOption(co2Data.times, co2Data.values, 'CO₂', '#409eff', 'ppm'));
  }

  const tvocData = await fetchSingleSensorTimeSeries(c3Id, 'sgp30', 'tvoc_ppb');
  if (tvocChartRef.value) {
    tvocChart?.dispose();
    tvocChart = echarts.init(tvocChartRef.value);
    tvocChart.setOption(makeLineChartOption(tvocData.times, tvocData.values, 'TVOC', '#b37feb', 'ppb'));
  }
};

// 初始化移动端图表
const initMobileCharts = async () => {
  const c3Id = c3DeviceId.value;
  if (!c3Id) return;

  pmCf1Chart?.dispose();
  pmStdChart?.dispose();
  smallParticlesChart?.dispose();
  largeParticlesChart?.dispose();
  co2Chart?.dispose();
  tvocChart?.dispose();

  const pmData = await fetchPms9103mTimeSeries(c3Id, [
    'pm1_0_cf1', 'pm2_5_cf1', 'pm10_cf1',
    'pm1_0_atm', 'pm2_5_atm', 'pm10_atm',
    'cnt_0_3um', 'cnt_0_5um', 'cnt_1_0um',
    'cnt_2_5um', 'cnt_5_0um', 'cnt_10um'
  ]);

  // 更新最新数据
  const pm25Values = pmData['pm2_5_cf1']?.values || [];
  const pm10Values = pmData['pm10_cf1']?.values || [];
  latestPm25.value = pm25Values.length > 0 ? Math.round(pm25Values[pm25Values.length - 1]!) : null;
  latestPm10.value = pm10Values.length > 0 ? Math.round(pm10Values[pm10Values.length - 1]!) : null;
  latestAqi.value = (latestPm25.value !== null && latestPm10.value !== null)
    ? calculateAQI(latestPm25.value, latestPm10.value, 0, 0)
    : null;

  if (pmCf1ChartRef.value) {
    pmCf1Chart = echarts.init(pmCf1ChartRef.value);
    pmCf1Chart.setOption(makeMobileMultiLineOption(
      pmData['pm2_5_cf1']?.times || [],
      [
        { name: 'PM1.0', values: pmData['pm1_0_cf1']?.values || [], color: '#69c0ff' },
        { name: 'PM2.5', values: pmData['pm2_5_cf1']?.values || [], color: '#e6a23c' },
        { name: 'PM10', values: pmData['pm10_cf1']?.values || [], color: '#f56c6c' },
      ],
      'μg/m³'
    ));
  }

  if (pmStdChartRef.value) {
    pmStdChart = echarts.init(pmStdChartRef.value);
    pmStdChart.setOption(makeMobileMultiLineOption(
      pmData['pm2_5_atm']?.times || [],
      [
        { name: 'PM1.0', values: pmData['pm1_0_atm']?.values || [], color: '#69c0ff' },
        { name: 'PM2.5', values: pmData['pm2_5_atm']?.values || [], color: '#e6a23c' },
        { name: 'PM10', values: pmData['pm10_atm']?.values || [], color: '#f56c6c' },
      ],
      'μg/m³'
    ));
  }

  if (smallParticlesChartRef.value) {
    smallParticlesChart = echarts.init(smallParticlesChartRef.value);
    smallParticlesChart.setOption(makeMobileMultiLineOption(
      pmData['cnt_0_3um']?.times || [],
      [
        { name: '0.3μm', values: pmData['cnt_0_3um']?.values || [], color: '#409eff' },
        { name: '0.5μm', values: pmData['cnt_0_5um']?.values || [], color: '#67c23a' },
        { name: '1.0μm', values: pmData['cnt_1_0um']?.values || [], color: '#e6a23c' },
      ],
      '个/0.1L'
    ));
  }

  if (largeParticlesChartRef.value) {
    largeParticlesChart = echarts.init(largeParticlesChartRef.value);
    largeParticlesChart.setOption(makeMobileMultiLineOption(
      pmData['cnt_2_5um']?.times || [],
      [
        { name: '2.5μm', values: pmData['cnt_2_5um']?.values || [], color: '#67c23a' },
        { name: '5.0μm', values: pmData['cnt_5_0um']?.values || [], color: '#e6a23c' },
        { name: '10μm', values: pmData['cnt_10um']?.values || [], color: '#f56c6c' },
      ],
      '个/0.1L'
    ));
  }

  const co2Data = await fetchSingleSensorTimeSeries(c3Id, 'sgp30', 'co2_ppm');
  latestCo2.value = co2Data.values.length > 0 ? Math.round(co2Data.values[co2Data.values.length - 1]!) : null;
  if (co2ChartRef.value) {
    co2Chart = echarts.init(co2ChartRef.value);
    co2Chart.setOption(makeMobileLineOption(co2Data.times, co2Data.values, 'CO₂', '#409eff', 'ppm'));
  }

  const tvocData = await fetchSingleSensorTimeSeries(c3Id, 'sgp30', 'tvoc_ppb');
  latestTvoc.value = tvocData.values.length > 0 ? Math.round(tvocData.values[tvocData.values.length - 1]!) : null;
  if (tvocChartRef.value) {
    tvocChart = echarts.init(tvocChartRef.value);
    tvocChart.setOption(makeMobileLineOption(tvocData.times, tvocData.values, 'TVOC', '#b37feb', 'ppb'));
  }
};

const initCharts = async () => {
  if (!c3DeviceId.value) return;
  if (isMobile.value) {
    await initMobileCharts();
  } else {
    await initDesktopCharts();
  }
};

// 窗口大小变化时调整图表
const handleResize = () => {
  pmCf1Chart?.resize();
  pmStdChart?.resize();
  smallParticlesChart?.resize();
  largeParticlesChart?.resize();
  co2Chart?.resize();
  tvocChart?.resize();
};

watch(pmTab, async () => {
  await nextTick();
  if (pmTab.value === 'cf1') {
    pmCf1Chart?.resize();
  } else {
    pmStdChart?.resize();
  }
});

watch(particleTab, async () => {
  await nextTick();
  if (particleTab.value === 'small') {
    smallParticlesChart?.resize();
  } else {
    largeParticlesChart?.resize();
  }
});

onMounted(async () => {
  checkMobileStatus();
  window.addEventListener('resize', handleResize);
  await fetchDevices();

  setTimeout(async () => {
    const birdcageFromQuery = route.query.birdcage as string | undefined;
    if (birdcageFromQuery && birdcageGroups.value.some(g => `${g.area}|${g.number}` === birdcageFromQuery)) {
      selectedBirdcageKey.value = birdcageFromQuery;
      await initCharts();
    } else if (birdcageGroups.value.length > 0 && !selectedBirdcageKey.value) {
      const g = birdcageGroups.value[0]!;
      selectedBirdcageKey.value = `${g.area}|${g.number}`;
      await initCharts();
    }
  }, 500);
});

onUnmounted(() => {
  window.removeEventListener('resize', handleResize);
  pmCf1Chart?.dispose();
  pmStdChart?.dispose();
  smallParticlesChart?.dispose();
  largeParticlesChart?.dispose();
  co2Chart?.dispose();
  tvocChart?.dispose();
});
</script>

<style lang="scss" scoped>
/* 桌面端 */
.aq-detail-card {
  z-index: 1;
  width: 75%;
  margin: 1% auto;
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(12px);
  -webkit-backdrop-filter: blur(12px);
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(255, 255, 255, 0.6);
  padding: 30px;
  min-height: auto;
}

.aq-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
  flex-wrap: wrap;
  gap: 16px;
}

.aq-header-left {
  display: flex;
  align-items: center;
  gap: 16px;
}

.aq-header-left h2 {
  margin: 0;
  color: #303133;
  font-size: 20px;
}

.aq-header-right {
  display: flex;
  align-items: center;
  gap: 16px;
}

.device-select {
  width: 200px;
}

.time-range-selector {
  display: flex;
  gap: 12px;
  align-items: center;
}

.time-button {
  font-size: 14px;
  font-weight: 500;
  color: #606266;
  background: #f5f7fa;
  border: 1px solid #e4e7ed;
  border-radius: 8px;
  padding: 8px 16px;
  cursor: pointer;
  transition: all 0.2s ease;
  white-space: nowrap;
  min-width: 60px;
  text-align: center;
}

.time-button:hover {
  color: #409eff;
  border-color: #c6e2ff;
  background: #ecf5ff;
  transform: translateY(-1px);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.08);
}

.time-button.active {
  color: #fff;
  background: linear-gradient(135deg, #409eff 0%, #66b1ff 100%);
  border-color: #409eff;
  box-shadow: 0 3px 6px rgba(64, 158, 255, 0.3);
  transform: translateY(-1px);
}

.aq-charts-content {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
}

.chart-container {
  margin-bottom: 0;
}

.chart-container h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.chart-container .time-range {
  font-size: 12px;
  color: #909399;
  font-weight: normal;
  margin-left: 8px;
}

.chart {
  width: 100%;
  height: 300px;
  border-radius: 10px;
  background-color: #f9f9f9;
}

@media (max-width: 768px) {
  .aq-detail-card {
    width: 95%;
    padding: 20px;
  }

  .aq-charts-content {
    grid-template-columns: 1fr;
  }

  .aq-header {
    flex-direction: column;
    align-items: flex-start;
  }

  .aq-header-right {
    width: 100%;
    flex-direction: column;
    align-items: flex-start;
  }

  .device-select {
    width: 100%;
  }
}

/* ========== 移动端全新设计 ========== */
.mobile-aq-detail {
  min-height: 100vh;
  background: #f5f7fa;
  padding-bottom: 20px;
}

/* 顶部导航 */
.mobile-top-nav {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 12px 16px;
  background: #fff;
  position: sticky;
  top: 0;
  z-index: 100;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.06);
}

.nav-back {
  width: 36px;
  height: 36px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  background: #f5f7fa;
  color: #606266;
  cursor: pointer;
  transition: all 0.2s;
}

.nav-back:active {
  background: #e4e7ed;
  transform: scale(0.95);
}

.nav-title {
  margin: 0;
  font-size: 17px;
  font-weight: 600;
  color: #303133;
}

.nav-placeholder {
  width: 36px;
}

/* 设备选择器 */
.mobile-selector-bar {
  padding: 12px 16px;
  background: #fff;
  border-bottom: 1px solid #f0f0f0;
}

.mobile-select {
  width: 100%;
}

/* 时间标签 */
.mobile-time-tabs {
  display: flex;
  padding: 12px 16px;
  gap: 12px;
  background: #fff;
}

.time-tab {
  flex: 1;
  text-align: center;
  padding: 8px 0;
  border-radius: 20px;
  font-size: 13px;
  font-weight: 500;
  color: #606266;
  background: #f5f7fa;
  border: 1px solid #e4e7ed;
  cursor: pointer;
  transition: all 0.2s;
}

.time-tab.active {
  color: #fff;
  background: linear-gradient(135deg, #409eff 0%, #66b1ff 100%);
  border-color: #409eff;
  box-shadow: 0 2px 8px rgba(64, 158, 255, 0.3);
}

/* 数据概览 */
.mobile-overview {
  padding: 16px;
}

.overview-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  text-align: center;
  padding: 24px 20px;
  border-radius: 16px;
  margin-bottom: 16px;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  color: #fff;
  box-shadow: 0 4px 16px rgba(102, 126, 234, 0.3);
}

.overview-card.aqi-good {
  background: linear-gradient(135deg, #52c41a 0%, #73d13d 100%);
  box-shadow: 0 4px 16px rgba(95, 238, 24, 0.3);
}

.overview-card.aqi-moderate {
  background: linear-gradient(135deg, #f6d365 0%, #fda085 100%);
  box-shadow: 0 4px 16px rgba(246, 211, 101, 0.3);
}

.overview-card.aqi-bad {
  background: linear-gradient(135deg, #f093fb 0%, #f5576c 100%);
  box-shadow: 0 4px 16px rgba(245, 87, 108, 0.3);
}

.overview-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
}

.overview-value {
  font-size: 48px;
  font-weight: 800;
  line-height: 1.1;
  letter-spacing: -2px;
}

.overview-label {
  font-size: 13px;
  opacity: 0.75;
  letter-spacing: 2px;
}

.overview-level {
  margin-top: 6px;
  padding: 4px 14px;
  border-radius: 20px;
  background: rgba(255, 255, 255, 0.25);
  font-size: 14px;
  font-weight: 600;
  letter-spacing: 1px;
}

.overview-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.overview-item {
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 14px;
  background: #fff;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.04);
}

.item-dot {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  flex-shrink: 0;
}

.item-info {
  flex: 1;
  min-width: 0;
}

.item-value {
  font-size: 18px;
  font-weight: 700;
  color: #303133;
  line-height: 1.3;
}

.item-label {
  font-size: 11px;
  color: #909399;
  margin-top: 2px;
}

/* 图表列表 */
.mobile-chart-list {
  padding: 0 16px;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.mobile-chart-card {
  background: #fff;
  border-radius: 16px;
  padding: 16px;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.06);
}

.card-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 12px;
}

.card-title {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 15px;
  font-weight: 600;
  color: #303133;
}

.title-icon {
  font-size: 18px;
}

.card-unit {
  font-size: 11px;
  color: #909399;
  background: #f5f7fa;
  padding: 3px 8px;
  border-radius: 10px;
}

.card-tabs {
  display: flex;
  gap: 8px;
  margin-bottom: 12px;
}

.card-tab {
  flex: 1;
  text-align: center;
  padding: 6px 0;
  border-radius: 8px;
  font-size: 12px;
  font-weight: 500;
  color: #606266;
  background: #f5f7fa;
  cursor: pointer;
  transition: all 0.2s;
}

.card-tab.active {
  color: #fff;
  background: #409eff;
}

.mobile-chart-card .chart {
  width: 100%;
  height: 200px;
}

.mobile-bottom-space {
  height: 40px;
}
</style>
