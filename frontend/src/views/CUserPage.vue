<template>
  <div class="cuser-dashboard">
    <div class="dashboard-header">
      <h1 class="page-title">智云鹰康 C 端控制台</h1>
      <p class="page-subtitle">实时监控环境数据与设备状态</p>
    </div>

    <el-row :gutter="24" class="card-grid">
      <!-- 模块一：环境舱实时监测 -->
      <el-col :xs="24" :sm="24" :md="12" :lg="12" :xl="12" class="mb-6">
        <el-card class="monitor-card" shadow="always">
          <template #header>
            <div class="card-header">
              <span class="header-title">🌡️ 环境舱实时监测</span>
              <el-tag v-if="parseFloat(temperature) > 35" type="danger" effect="dark" round>
                温度异常
              </el-tag>
              <el-tag v-else type="success" effect="dark" round>
                状态正常
              </el-tag>
            </div>
          </template>
          
          <div class="monitor-body">
            <div class="data-group">
              <div class="data-item">
                <div class="data-label">当前温度</div>
                <div class="data-value" :class="{ 'text-danger': parseFloat(temperature) > 35 }">
                  {{ temperature }}<span class="unit">℃</span>
                </div>
              </div>
              <el-divider direction="vertical" class="data-divider" />
              <div class="data-item">
                <div class="data-label">当前湿度</div>
                <div class="data-value">
                  {{ humidity }}<span class="unit">%RH</span>
                </div>
              </div>
            </div>
            
            <div class="update-time">
              <el-icon><Clock /></el-icon> 最后更新：{{ updateTime }}
            </div>
          </div>
        </el-card>
      </el-col>

      <!-- 模块二：智能喂鸟器 -->
      <el-col :xs="24" :sm="24" :md="12" :lg="12" :xl="12" class="mb-6">
        <el-card class="feeder-card" shadow="always">
          <template #header>
            <div class="card-header">
              <span class="header-title">🥣 智能喂鸟器</span>
              <el-button type="primary" size="small" color="#8BAD42" @click="handleFeed" :loading="feeding">
                远程投喂
              </el-button>
            </div>
          </template>
          
          <div class="feeder-body">
            <div class="progress-wrap">
              <el-progress 
                type="circle" 
                :percentage="foodLevel" 
                :color="foodLevelColor"
                :stroke-width="12"
                :width="150"
              >
                <template #default="{ percentage }">
                  <div class="progress-content">
                    <span class="percentage-value">{{ percentage }}%</span>
                    <span class="percentage-label">余粮状态</span>
                  </div>
                </template>
              </el-progress>
            </div>
            
            <div class="feeder-info">
              <div class="info-item">
                <span class="label">雷达测距：</span>
                <span class="value">{{ foodDistance }} cm</span>
              </div>
              <div class="info-item">
                <span class="label">状态：</span>
                <span :class="foodLevel < 20 ? 'text-danger' : 'text-success'">
                  {{ foodLevel < 20 ? '⚠️ 请及时补粮' : '✅ 粮食充足' }}
                </span>
              </div>
            </div>
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

// 配置信息
const BEMFA_UID = '8a512c3a9261c60beea0bce9e3ee38a7';
const TOPIC_FOOD = 'birdFood';
const TOPIC_ENV = 'birdEnv';

// 响应式状态
const temperature = ref('0.0');
const humidity = ref('0.0');
const foodLevel = ref(0);
const foodDistance = ref('0.0');
const updateTime = ref('--:--:--');
const feeding = ref(false);

// 余粮进度条颜色
const foodLevelColor = computed(() => {
  if (foodLevel.value < 20) return '#F56C6C';
  if (foodLevel.value < 50) return '#E6A23C';
  return '#8BAD42';
});

// 获取巴法云环境数据
const fetchEnvData = async () => {
  try {
    const response = await fetch(`https://api.bemfa.com/api/device/v1/data/1/get/?uid=${BEMFA_UID}&topic=${TOPIC_ENV}&num=1`);
    const data = await response.json();
    if (data && data.msg) {
      const parts = data.msg.split('#');
      if (parts.length >= 2) {
        temperature.value = parts[0];
        humidity.value = parts[1];
        updateTime.value = new Date().toLocaleTimeString();
      }
    }
  } catch (error) {
    console.error('Failed to fetch environment data:', error);
  }
};

// 获取巴法云余粮数据
const fetchFoodData = async () => {
  try {
    const response = await fetch(`https://api.bemfa.com/api/device/v1/data/1/get/?uid=${BEMFA_UID}&topic=${TOPIC_FOOD}&num=1`);
    const data = await response.json();
    if (data && data.msg) {
      const distance = parseFloat(data.msg);
      foodDistance.value = distance.toFixed(1);
      
      // 算法：假设满载距离为2cm，空载距离为20cm
      let level = 100 - ((distance - 2) / (20 - 2) * 100);
      foodLevel.value = Math.max(0, Math.min(100, Math.round(level)));
    }
  } catch (error) {
    console.error('Failed to fetch food data:', error);
  }
};

// 远程投喂模拟
const handleFeed = () => {
  ElMessageBox.confirm('确定要进行远程投喂吗？', '操作确认', {
    confirmButtonText: '确定',
    cancelButtonText: '取消',
    type: 'info'
  }).then(() => {
    feeding.value = true;
    // 模拟物联网指令发送
    setTimeout(() => {
      feeding.value = false;
      ElMessage({
        message: '投喂指令已发送至设备',
        type: 'success'
      });
    }, 1500);
  }).catch(() => {});
};

// 定时轮询
let pollTimer: any = null;

onMounted(() => {
  fetchEnvData();
  fetchFoodData();
  pollTimer = setInterval(() => {
    fetchEnvData();
    fetchFoodData();
  }, 5000);
});

onUnmounted(() => {
  if (pollTimer) {
    clearInterval(pollTimer);
  }
});
</script>

<style scoped lang="scss">
.cuser-dashboard {
  padding: 40px;
  max-width: 1400px;
  margin: 0 auto;
  min-height: calc(100vh - 70px);
  background-color: #f8fafc;
}

.dashboard-header {
  margin-bottom: 40px;
  text-align: left;
  border-left: 6px solid #8BAD42;
  padding-left: 20px;

  .page-title {
    font-size: 32px;
    font-weight: 800;
    color: #1e293b;
    margin: 0;
  }

  .page-subtitle {
    font-size: 16px;
    color: #64748b;
    margin-top: 8px;
  }
}

.card-grid {
  display: flex;
  flex-wrap: wrap;
}

.mb-6 {
  margin-bottom: 24px;
}

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;

  .header-title {
    font-size: 18px;
    font-weight: 700;
    color: #334155;
  }
}

.monitor-card, .feeder-card {
  border-radius: 16px;
  border: none;
  transition: transform 0.3s ease, box-shadow 0.3s ease;

  &:hover {
    transform: translateY(-5px);
    box-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.1), 0 10px 10px -5px rgba(0, 0, 0, 0.04);
  }
}

.monitor-body {
  padding: 20px 0;

  .data-group {
    display: flex;
    justify-content: space-around;
    align-items: center;
    margin-bottom: 30px;

    .data-item {
      text-align: center;
      flex: 1;

      .data-label {
        font-size: 14px;
        color: #94a3b8;
        margin-bottom: 12px;
      }

      .data-value {
        font-size: 56px;
        font-weight: 800;
        color: #8BAD42;
        line-height: 1;

        &.text-danger {
          color: #f56c6c;
        }

        .unit {
          font-size: 18px;
          font-weight: 600;
          color: #94a3b8;
          margin-left: 4px;
        }
      }
    }

    .data-divider {
      height: 80px;
      border-color: #e2e8f0;
    }
  }

  .update-time {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    font-size: 14px;
    color: #94a3b8;
  }
}

.feeder-body {
  display: flex;
  align-items: center;
  justify-content: space-around;
  padding: 20px 0;

  .progress-wrap {
    flex-shrink: 0;
  }

  .progress-content {
    display: flex;
    flex-direction: column;
    align-items: center;

    .percentage-value {
      font-size: 28px;
      font-weight: 800;
      color: #334155;
    }

    .percentage-label {
      font-size: 12px;
      color: #94a3b8;
      margin-top: 4px;
    }
  }

  .feeder-info {
    display: flex;
    flex-direction: column;
    gap: 16px;

    .info-item {
      display: flex;
      align-items: center;
      font-size: 16px;

      .label {
        color: #64748b;
        font-weight: 500;
      }

      .value {
        color: #1e293b;
        font-weight: 700;
      }
    }
  }
}

.text-danger { color: #f56c6c; }
.text-success { color: #8BAD42; }

@media (max-width: 768px) {
  .cuser-dashboard {
    padding: 20px;
  }
  
  .monitor-body .data-group .data-value {
    font-size: 42px;
  }
  
  .feeder-body {
    flex-direction: column;
    gap: 30px;
  }
}
</style>