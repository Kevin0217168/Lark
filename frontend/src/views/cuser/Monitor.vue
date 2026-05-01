<template>
  <div class="monitor-module">
    <el-row :gutter="24">
      <!-- 左侧：环境舱实时监测 -->
      <el-col :span="12" :xs="24">
        <el-card class="monitor-card h-full" shadow="hover">
          <template #header>
            <div class="flex justify-between items-center">
              <span class="text-lg font-bold text-slate-700 flex items-center gap-2">
                🌡️ 环境舱实时监测
              </span>
              <el-tag v-if="parseFloat(temperature) > 35" type="danger" effect="dark">温度异常</el-tag>
              <el-tag v-else type="success" effect="dark">状态正常</el-tag>
            </div>
          </template>
          
          <div class="flex flex-col items-center py-10">
            <div class="flex justify-around w-full mb-10">
              <div class="text-center">
                <p class="text-sm text-slate-400 mb-2">当前温度</p>
                <p class="text-6xl font-black" :class="parseFloat(temperature) > 35 ? 'text-red-500' : 'text-[#8BAD42]'">
                  {{ temperature }}<span class="text-xl ml-1 text-slate-400 font-normal">℃</span>
                </p>
              </div>
              <el-divider direction="vertical" class="h-24" />
              <div class="text-center">
                <p class="text-sm text-slate-400 mb-2">当前湿度</p>
                <p class="text-6xl font-black text-[#8BAD42]">
                  {{ humidity }}<span class="text-xl ml-1 text-slate-400 font-normal">%RH</span>
                </p>
              </div>
            </div>
            <div class="text-slate-400 text-sm flex items-center gap-2">
              <el-icon><Clock /></el-icon> 最后更新：{{ updateTime }}
            </div>
          </div>
        </el-card>
      </el-col>

      <!-- 右侧：智能喂鸟器 -->
      <el-col :span="12" :xs="24">
        <el-card class="feeder-card h-full" shadow="hover">
          <template #header>
            <div class="flex justify-between items-center">
              <span class="text-lg font-bold text-slate-700 flex items-center gap-2">
                🥣 智能喂鸟器
              </span>
              <el-button type="primary" color="#8BAD42" size="small" @click="handleFeed" :loading="feeding">
                立即投喂
              </el-button>
            </div>
          </template>

          <div class="flex items-center justify-around py-6">
            <el-progress 
              type="circle" 
              :percentage="foodLevel" 
              :width="160"
              :stroke-width="12"
              :color="foodLevelColor"
            >
              <template #default="{ percentage }">
                <div class="flex flex-col items-center">
                  <span class="text-3xl font-bold text-slate-700">{{ percentage }}%</span>
                  <span class="text-xs text-slate-400 mt-1">余粮状态</span>
                </div>
              </template>
            </el-progress>

            <div class="space-y-4">
              <div class="bg-slate-50 p-4 rounded-xl border border-slate-100 min-w-[180px]">
                <p class="text-xs text-slate-400 mb-1">雷达测距</p>
                <p class="text-xl font-bold text-slate-700">{{ foodDistance }} cm</p>
              </div>
              <div class="bg-slate-50 p-4 rounded-xl border border-slate-100">
                <p class="text-xs text-slate-400 mb-1">今日进食</p>
                <p class="text-xl font-bold text-slate-700">125 <span class="text-xs text-slate-400 font-normal">克</span></p>
              </div>
            </div>
          </div>

          <div class="mt-6 flex gap-4">
            <el-input-number v-model="feedAmount" :min="5" :max="100" :step="5" class="flex-1" />
            <el-button class="w-24">定量设置</el-button>
          </div>
        </el-card>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue';
import { ElMessage, ElMessageBox } from 'element-plus';
import { Clock } from '@element-plus/icons-vue';

const BEMFA_UID = '8a512c3a9261c60beea0bce9e3ee38a7';
const TOPIC_FOOD = 'birdFood';
const TOPIC_ENV = 'birdEnv';

const temperature = ref('0.0');
const humidity = ref('0.0');
const foodLevel = ref(0);
const foodDistance = ref('0.0');
const updateTime = ref('--:--:--');
const feeding = ref(false);
const feedAmount = ref(20);

const foodLevelColor = computed(() => {
  if (foodLevel.value < 20) return '#F56C6C';
  if (foodLevel.value < 50) return '#E6A23C';
  return '#8BAD42';
});

const fetchData = async () => {
  try {
    // 环境数据
    const envRes = await fetch(`https://api.bemfa.com/api/device/v1/data/1/get/?uid=${BEMFA_UID}&topic=${TOPIC_ENV}&num=1`);
    const envData = await envRes.json();
    if (envData?.msg) {
      const parts = envData.msg.split('#');
      if (parts.length >= 2) {
        temperature.value = parts[0];
        humidity.value = parts[1];
        updateTime.value = new Date().toLocaleTimeString();
      }
    }

    // 食物数据
    const foodRes = await fetch(`https://api.bemfa.com/api/device/v1/data/1/get/?uid=${BEMFA_UID}&topic=${TOPIC_FOOD}&num=1`);
    const foodData = await foodRes.json();
    if (foodData?.msg) {
      const dist = parseFloat(foodData.msg);
      foodDistance.value = dist.toFixed(1);
      let level = 100 - ((dist - 2) / (20 - 2) * 100);
      foodLevel.value = Math.max(0, Math.min(100, Math.round(level)));
    }
  } catch (e) { console.error(e); }
};

const handleFeed = () => {
  ElMessageBox.confirm(`确认远程投喂 ${feedAmount.value}g 饲料吗？`, '投喂确认').then(() => {
    feeding.value = true;
    setTimeout(() => {
      feeding.value = false;
      ElMessage.success('指令已送达');
    }, 1500);
  });
};

let timer: any = null;
onMounted(() => {
  fetchData();
  timer = setInterval(fetchData, 5000);
});
onUnmounted(() => clearInterval(timer));
</script>

<style scoped>
:deep(.el-card) {
  border-radius: 1rem;
}
</style>