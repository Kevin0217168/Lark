<template>
  <div class="p-4 space-y-4 pb-20">
    <!-- 顶部动态预警提醒 (接管原小程序的 alert-bar) -->
    <el-alert
      v-if="showAlert"
      :title="alertTitle"
      :type="alertType"
      :description="alertMessage"
      show-icon
      @close="showAlert = false"
      class="rounded-xl shadow-sm"
    />

    <!-- 卡片1：智能孵化监测仪 -->
    <el-card shadow="never" class="rounded-2xl border-none shadow-sm">
      <template #header>
        <div class="flex justify-between items-center">
          <div class="flex items-center gap-2 font-bold text-gray-800">
            <span class="text-xl">🌡️</span> 智能孵化监测仪
          </div>
          <el-tag type="success" size="small" effect="light" round>
            <span class="w-1.5 h-1.5 inline-block bg-green-500 rounded-full mr-1 animate-pulse"></span> 在线
          </el-tag>
        </div>
      </template>

      <div class="grid grid-cols-2 gap-4 text-center mb-4">
        <div class="bg-gray-50 rounded-xl p-4 border border-green-100">
          <div class="text-3xl font-bold text-[#8BAD42] flex items-baseline justify-center">
            {{ temp }} <span class="text-sm text-gray-500 ml-1">°C</span>
          </div>
          <div class="text-sm text-gray-600 mt-1">实时温度</div>
          <div class="text-xs text-gray-400 bg-gray-100 rounded-full px-2 py-0.5 inline-block mt-2">精度 ±0.5°C</div>
        </div>
        
        <div class="bg-gray-50 rounded-xl p-4 border border-blue-100">
          <div class="text-3xl font-bold text-blue-500 flex items-baseline justify-center">
            {{ humid }} <span class="text-sm text-gray-500 ml-1">%</span>
          </div>
          <div class="text-sm text-gray-600 mt-1">实时湿度</div>
          <div class="text-xs text-gray-400 bg-gray-100 rounded-full px-2 py-0.5 inline-block mt-2">精度 ±3%RH</div>
        </div>
      </div>

      <div class="text-center text-xs text-gray-400 mb-4">
        最后更新时间: {{ updateTime }}
      </div>

      <el-button type="primary" color="#8BAD42" plain class="w-full rounded-xl" @click="refreshData" :loading="isRefreshing">
        <el-icon class="mr-1"><Refresh /></el-icon> 刷新设备数据
      </el-button>
    </el-card>

    <!-- 卡片2：智能喂鸟器 -->
    <el-card shadow="never" class="rounded-2xl border-none shadow-sm">
      <template #header>
        <div class="flex justify-between items-center">
          <div class="flex items-center gap-2 font-bold text-gray-800">
            <span class="text-xl">🥣</span> 智能喂鸟器
          </div>
          <el-tag type="success" size="small" effect="light" round>
            <span class="w-1.5 h-1.5 inline-block bg-green-500 rounded-full mr-1 animate-pulse"></span> 在线
          </el-tag>
        </div>
      </template>

      <!-- 余粮状态条 -->
      <div class="bg-gray-50 p-4 rounded-xl mb-4">
        <div class="flex justify-between text-sm text-gray-600 mb-2">
          <span>当前余粮</span>
          <span class="text-xs text-gray-400">雷达测距: {{ foodDistance }} cm</span>
        </div>
        <div class="flex items-center gap-4">
          <span class="text-3xl font-bold text-[#8BAD42] w-16">{{ foodLevel }}%</span>
          <el-progress :percentage="foodLevel" :color="foodLevel < 30 ? '#F56C6C' : '#8BAD42'" :show-text="false" class="flex-1" :stroke-width="12" />
        </div>
      </div>

      <!-- 喂食统计 -->
      <div class="flex justify-around bg-gray-50 rounded-xl p-3 mb-4">
        <div class="text-center">
          <div class="text-xl font-bold text-gray-800">{{ todayFeedCount }}</div>
          <div class="text-xs text-gray-500">今日进食(次)</div>
        </div>
        <div class="w-px bg-gray-200"></div>
        <div class="text-center">
          <div class="text-xl font-bold text-gray-800">{{ lastFeedTime }}</div>
          <div class="text-xs text-gray-500">上次投喂</div>
        </div>
      </div>

      <!-- 定量投喂控制区 -->
      <div class="bg-gray-50 rounded-xl p-4">
        <div class="text-center text-sm font-bold text-gray-700 mb-3">定量投喂控制</div>
        <div class="flex justify-center items-center gap-6 mb-4">
          <el-button circle @click="decreaseFood" :disabled="feedAmount <= 5"><el-icon><Minus /></el-icon></el-button>
          <div class="text-3xl font-bold text-[#8BAD42] w-20 text-center">{{ feedAmount }} <span class="text-sm text-gray-500">克</span></div>
          <el-button circle @click="increaseFood" :disabled="feedAmount >= 50"><el-icon><Plus /></el-icon></el-button>
        </div>
        <el-button type="primary" color="#8BAD42" class="w-full rounded-xl shadow-md" @click="feedNow" :loading="isFeeding">
          <el-icon class="mr-1"><Chicken /></el-icon> 立即远程投喂
        </el-button>
      </div>
    </el-card>

    <!-- 底部快捷导航 (原 Quick Actions) -->
    <div class="grid grid-cols-4 gap-2 bg-white p-4 rounded-2xl shadow-sm">
      <div class="flex flex-col items-center gap-1 active:scale-95 transition-transform cursor-pointer" @click="router.push('/cloud/checkup')">
        <div class="w-12 h-12 rounded-full bg-green-50 flex items-center justify-center text-2xl">🤖</div>
        <span class="text-xs text-gray-600">AI自检</span>
      </div>
      <div class="flex flex-col items-center gap-1 active:scale-95 transition-transform cursor-pointer" @click="router.push('/cloud/consultation')">
        <div class="w-12 h-12 rounded-full bg-blue-50 flex items-center justify-center text-2xl">👨‍⚕️</div>
        <span class="text-xs text-gray-600">在线问诊</span>
      </div>
      <div class="flex flex-col items-center gap-1 active:scale-95 transition-transform cursor-pointer" @click="ElMessage.success('报告生成中...')">
        <div class="w-12 h-12 rounded-full bg-yellow-50 flex items-center justify-center text-2xl">📊</div>
        <span class="text-xs text-gray-600">健康报告</span>
      </div>
      <div class="flex flex-col items-center gap-1 active:scale-95 transition-transform cursor-pointer" @click="router.push('/cloud/profile')">
        <div class="w-12 h-12 rounded-full bg-purple-50 flex items-center justify-center text-2xl">⚙️</div>
        <span class="text-xs text-gray-600">设备设置</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage, ElMessageBox } from 'element-plus';
import { Refresh, Plus, Minus, Chicken } from '@element-plus/icons-vue';

const router = useRouter();

// === 状态定义 ===
const showAlert = ref(false);
const alertType = ref<'success' | 'warning' | 'info' | 'error'>('warning');
const alertTitle = ref('');
const alertMessage = ref('');

const temp = ref('26.50');
const humid = ref('55.00');
const updateTime = ref('--:--:--');

const foodLevel = ref(75);
const foodDistance = ref('12.04');
const todayFeedCount = ref(3);
const lastFeedTime = ref('09:15');
const feedAmount = ref(20);

const isRefreshing = ref(false);
const isFeeding = ref(false);

let pollingTimer: number | null = null;

// === 巴法云 API 逻辑 ===
const BEMFA_UID = '8a512c3a9261c60beea0bce9e3ee38a7';

// 获取环境温湿度
const getBirdEnvData = async () => {
  try {
    const res = await fetch(`https://api.bemfa.com/api/device/v1/data/1/get/?uid=${BEMFA_UID}&topic=birdEnv&num=1`);
    const data = await res.json();
    if (data && data.msg) {
      const envData = data.msg.split('#');
      if (envData.length === 2) {
        temp.value = envData[0];
        humid.value = envData[1];
      }
    }
  } catch (error) {
    console.error("环境数据获取失败", error);
  }
};

// 获取余粮雷达数据
const getBirdFoodData = async () => {
  try {
    const res = await fetch(`https://api.bemfa.com/api/device/v1/data/1/get/?uid=${BEMFA_UID}&topic=birdFood&num=1`);
    const data = await res.json();
    if (data && data.msg) {
      const distance = parseFloat(data.msg);
      foodDistance.value = distance.toFixed(2);
      // 算法还原：假设20cm为空，计算百分比
      let calcFoodLevel = 100 - (distance / 20 * 100);
      calcFoodLevel = Math.max(0, Math.min(100, calcFoodLevel));
      foodLevel.value = parseInt(calcFoodLevel.toString());
      checkAlerts();
    }
  } catch (error) {
    console.error("粮食数据获取失败", error);
  }
};

// 预警检查逻辑
const checkAlerts = () => {
  const currentTemp = parseFloat(temp.value);
  if (foodLevel.value < 30) {
    showAlert.value = true;
    alertType.value = 'error';
    alertTitle.value = '余粮告警';
    alertMessage.value = '余粮不足30%，请及时补充粮食以防宠物鸟挨饿！';
  } else if (currentTemp > 35 || currentTemp < 20) {
    showAlert.value = true;
    alertType.value = 'warning';
    alertTitle.value = '温度异常';
    alertMessage.value = `当前温度 ${currentTemp}°C 偏离舒适区间，请检查设备！`;
  } else {
    showAlert.value = false;
  }
};

// === 交互逻辑 ===
const refreshData = async () => {
  isRefreshing.value = true;
  await Promise.all([getBirdEnvData(), getBirdFoodData()]);
  const now = new Date();
  updateTime.value = now.toTimeString().substring(0, 8);
  ElMessage.success('设备数据已同步');
  isRefreshing.value = false;
};

const decreaseFood = () => { if (feedAmount.value > 5) feedAmount.value -= 5; };
const increaseFood = () => { if (feedAmount.value < 50) feedAmount.value += 5; };

const feedNow = () => {
  ElMessageBox.confirm(
    `即将远程投放 ${feedAmount.value}克 鸟粮，确定执行吗？`,
    '投喂确认',
    { confirmButtonText: '立即执行', cancelButtonText: '取消', type: 'warning' }
  ).then(() => {
    isFeeding.value = true;
    // 模拟投喂指令下发
    setTimeout(() => {
      const now = new Date();
      lastFeedTime.value = now.toTimeString().substring(0, 5);
      foodLevel.value = Math.max(0, foodLevel.value - Math.floor(feedAmount.value / 5));
      todayFeedCount.value++;
      
      isFeeding.value = false;
      ElMessage.success('指令已下发，投喂成功！');
      checkAlerts();
    }, 1500);
  }).catch(() => {});
};

// === 生命周期 ===
onMounted(() => {
  refreshData();
  // 完美复刻小程序的定时轮询机制 (5秒一次)
  pollingTimer = window.setInterval(() => {
    getBirdEnvData();
    getBirdFoodData();
  }, 5000);
});

onUnmounted(() => {
  if (pollingTimer) clearInterval(pollingTimer);
});
</script>

<style scoped>
/* 使用 Tailwind 后，CSS 大量减少，仅保留部分动画增强 */
.animate-pulse {
  animation: pulse 2s cubic-bezier(0.4, 0, 0.6, 1) infinite;
}
@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: .5; }
}
</style>