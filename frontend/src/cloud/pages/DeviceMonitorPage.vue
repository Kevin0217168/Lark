<template>
  <div class="cuser-container" :class="{ 'mobile': isMobile }">
    <!-- 顶部预警通知栏 -->
    <el-alert
      v-if="showAlert"
      :title="alertTitle"
      :type="alertType"
      :description="alertMessage"
      show-icon
      @close="closeAlert"
      class="mb-4"
    />

    <div class="data-cards">
      <el-row :gutter="20">
        <!-- 温湿度监测卡片 -->
        <el-col :xs="24" :sm="12" :md="12" :lg="12" :xl="12" class="mb-4">
          <el-card class="monitor-card" shadow="hover">
            <template #header>
              <div class="card-header">
                <div class="title-wrap">
                  <span class="icon">🌡️</span>
                  <span class="title">智能孵化监测仪</span>
                </div>
                <el-tag size="small" type="success" effect="dark" round>
                  <i class="dot"></i> 在线
                </el-tag>
              </div>
            </template>
            
            <div class="card-body">
              <div class="data-display">
                <div class="data-item">
                  <div class="value">
                    <span class="number">{{ temperature }}</span>
                    <span class="unit">℃</span>
                  </div>
                  <div class="label">实时温度</div>
                  <div class="precision">精度 ±0.5℃</div>
                </div>
                <el-divider direction="vertical" border-style="dashed" />
                <div class="data-item">
                  <div class="value">
                    <span class="number">{{ humidity }}</span>
                    <span class="unit">%RH</span>
                  </div>
                  <div class="label">实时湿度</div>
                  <div class="precision">精度 ±3%RH</div>
                </div>
              </div>
              
              <div class="update-info">
                <span>更新时间：{{ updateTime }}</span>
              </div>
            </div>
            
            <div class="card-footer">
              <el-button type="primary" plain size="small" @click="refreshData" :loading="refreshing">
                <el-icon><Refresh /></el-icon> 刷新数据
              </el-button>
            </div>
          </el-card>
        </el-col>

        <!-- 智能喂鸟器卡片 -->
        <el-col :xs="24" :sm="12" :md="12" :lg="12" :xl="12" class="mb-4">
          <el-card class="feeder-card" shadow="hover">
            <template #header>
              <div class="card-header">
                <div class="title-wrap">
                  <span class="icon">🥣</span>
                  <span class="title">智能喂鸟器</span>
                </div>
                <el-tag size="small" type="success" effect="dark" round>
                  <i class="dot"></i> 在线
                </el-tag>
              </div>
            </template>
            
            <div class="card-body">
              <div class="food-status">
                <div class="food-label-wrap">
                  <span class="food-label">余粮状态</span>
                  <el-tag :type="foodLevel < 30 ? 'danger' : 'success'" size="small">
                    {{ foodLevel < 30 ? '⚠️ 粮食不足' : '✅ 粮食充足' }}
                  </el-tag>
                </div>
                
                <div class="progress-wrap">
                  <div class="percentage">{{ foodLevel }}%</div>
                  <el-progress 
                    :percentage="foodLevel" 
                    :status="foodLevel < 30 ? 'exception' : ''"
                    :stroke-width="12"
                    :show-text="false"
                    color="#8BAD42"
                  />
                </div>
                
                <div class="distance-info">
                  雷达测距：{{ foodDistance }} cm
                </div>
              </div>
              
              <div class="feed-stats">
                <div class="stat-item">
                  <div class="stat-value">{{ todayFeedCount }}</div>
                  <div class="stat-label">今日进食</div>
                </div>
                <el-divider direction="vertical" />
                <div class="stat-item">
                  <div class="stat-value">{{ lastFeedTime }}</div>
                  <div class="stat-label">上次投喂</div>
                </div>
              </div>
              
              <div class="feed-control">
                <div class="control-label">定量投喂 (克)</div>
                <div class="control-row">
                  <el-input-number v-model="feedAmount" :min="5" :max="50" :step="5" size="default" />
                  <el-button type="primary" @click="confirmFeed" class="feed-btn">
                    <el-icon><Pointer /></el-icon> 立即投喂
                  </el-button>
                </div>
              </div>
            </div>
          </el-card>
        </el-col>
      </el-row>
    </div>

    <!-- 快捷功能 -->
    <div class="quick-actions mt-4">
      <el-row :gutter="10">
        <el-col :span="6" v-for="action in quickActions" :key="action.label">
          <div class="action-item" @click="handleAction(action)">
            <div class="action-icon">{{ action.icon }}</div>
            <div class="action-label">{{ action.label }}</div>
          </div>
        </el-col>
      </el-row>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, computed } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessageBox, ElMessage } from 'element-plus';
import { Refresh, Pointer } from '@element-plus/icons-vue';

const router = useRouter();

// 配置信息
const BEMFA_UID = '8a512c3a9261c60beea0bce9e3ee38a7';
const TOPIC_FOOD = 'birdFood';
const TOPIC_ENV = 'birdEnv';

// 响应式数据
const isMobile = ref(window.innerWidth < 768);
const refreshing = ref(false);

// 环境数据
const temperature = ref('37.5');
const humidity = ref('65');
const updateTime = ref('--:--:--');

// 喂鸟器数据
const foodLevel = ref(75);
const foodDistance = ref('--');
const todayFeedCount = ref(3);
const lastFeedTime = ref('09:15');
const feedAmount = ref(20);

// 预警状态
const showAlert = ref(false);
const alertTitle = ref('');
const alertMessage = ref('');
const alertType = ref<'warning' | 'error' | 'success' | 'info'>('warning');

// 快捷功能
const quickActions = [
  { label: 'AI自检', icon: '🤖', path: '/cloud/ai' },
  { label: '在线问诊', icon: '👨‍⚕️', path: '/cloud/clinic' },
  { label: '健康报告', icon: '📊', path: '/cloud/report' },
  { label: '设备设置', icon: '⚙️', path: '/cloud/settings' }
];

// 获取巴法云环境数据 (温湿度)
const getBirdEnvData = async () => {
  try {
    const response = await fetch(`https://api.bemfa.com/api/device/v1/data/1/get/?uid=${BEMFA_UID}&topic=${TOPIC_ENV}&num=1`);
    const data = await response.json();
    
    if (data && data.msg) {
      const rawData = data.msg; // "26.50#55.00"
      const envData = rawData.split('#');
      if (envData.length === 2) {
        temperature.value = envData[0];
        humidity.value = envData[1];
        updateTime.value = new Date().toLocaleTimeString();
        checkAlerts();
      }
    }
  } catch (error) {
    console.error('获取环境数据失败:', error);
  }
};

// 获取巴法云余粮数据
const getBirdFoodData = async () => {
  try {
    const response = await fetch(`https://api.bemfa.com/api/device/v1/data/1/get/?uid=${BEMFA_UID}&topic=${TOPIC_FOOD}&num=1`);
    const data = await response.json();
    
    if (data && data.msg) {
      const distance = parseFloat(data.msg);
      foodDistance.value = distance.toString();
      
      // 换算成余粮百分比 (假设粮仓总深度是 20 厘米)
      let calcLevel = 100 - (distance / 20 * 100);
      calcLevel = Math.max(0, Math.min(100, Math.round(calcLevel)));
      foodLevel.value = calcLevel;
      
      checkAlerts();
    }
  } catch (error) {
    console.error('获取余粮数据失败:', error);
  }
};

// 检查预警
const checkAlerts = () => {
  const temp = parseFloat(temperature.value);
  if (foodLevel.value < 30) {
    showAlert.value = true;
    alertType.value = 'error';
    alertTitle.value = '粮食不足';
    alertMessage.value = `余粮不足30% (当前 ${foodLevel.value}%)，请及时补充`;
  } else if (temp > 39 || temp < 35) {
    showAlert.value = true;
    alertType.value = 'warning';
    alertTitle.value = '温度异常';
    alertMessage.value = `当前温度 ${temp}℃，超出正常范围 (35-39℃)`;
  } else {
    showAlert.value = false;
  }
};

const closeAlert = () => {
  showAlert.value = false;
};

// 刷新数据
const refreshData = async () => {
  refreshing.value = true;
  await Promise.all([getBirdEnvData(), getBirdFoodData()]);
  setTimeout(() => {
    refreshing.value = false;
    ElMessage.success('数据已更新');
  }, 500);
};

// 投喂逻辑
const confirmFeed = () => {
  ElMessageBox.confirm(
    `确定要投喂 ${feedAmount.value}克 粮食吗？`,
    '确认投喂',
    {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'info',
    }
  ).then(() => {
    // 模拟投喂请求
    ElMessage({
      type: 'success',
      message: `已发送投喂指令: ${feedAmount.value}克`,
    });
    
    setTimeout(() => {
      const now = new Date();
      lastFeedTime.value = `${now.getHours().toString().padStart(2, '0')}:${now.getMinutes().toString().padStart(2, '0')}`;
      todayFeedCount.value++;
      
      // 假设投喂后余粮减少 (仅前端模拟，实际应由硬件回传)
      const mockConsumption = Math.floor(feedAmount.value / 5);
      foodLevel.value = Math.max(0, foodLevel.value - mockConsumption);
    }, 1000);
  });
};

const handleAction = (action: any) => {
  if (action.path.startsWith('/cloud')) {
    router.push(action.path);
  } else {
    ElMessage.info(`跳转到: ${action.label} (功能开发中)`);
  }
};

// 定时轮询
let pollTimer: any = null;

onMounted(() => {
  refreshData();
  pollTimer = setInterval(() => {
    getBirdEnvData();
    getBirdFoodData();
  }, 5000);
  
  window.addEventListener('resize', () => {
    isMobile.value = window.innerWidth < 768;
  });
});

onUnmounted(() => {
  if (pollTimer) clearInterval(pollTimer);
});
</script>

<style scoped lang="scss">
.cuser-container {
  padding: 20px;
  max-width: 1200px;
  margin: 0 auto;
  
  &.mobile {
    padding: 12px;
  }
}

.mb-4 { margin-bottom: 16px; }
.mt-4 { margin-top: 16px; }

.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  
  .title-wrap {
    display: flex;
    align-items: center;
    gap: 8px;
    font-weight: 600;
    font-size: 16px;
    color: #303133;
  }
  
  .dot {
    display: inline-block;
    width: 8px;
    height: 8px;
    background-color: #fff;
    border-radius: 50%;
    margin-right: 4px;
    animation: pulse 2s infinite;
  }
}

@keyframes pulse {
  0% { opacity: 0.4; }
  50% { opacity: 1; }
  100% { opacity: 0.4; }
}

.card-body {
  padding: 10px 0;
}

.data-display {
  display: flex;
  justify-content: space-around;
  align-items: center;
  margin: 20px 0;
  
  .data-item {
    text-align: center;
    flex: 1;
    
    .value {
      margin-bottom: 8px;
      .number {
        font-size: 32px;
        font-weight: bold;
        color: #8BAD42;
      }
      .unit {
        font-size: 14px;
        color: #909399;
        margin-left: 4px;
      }
    }
    
    .label {
      font-size: 14px;
      color: #606266;
      margin-bottom: 4px;
    }
    
    .precision {
      font-size: 12px;
      color: #C0C4CC;
    }
  }
  
  :deep(.el-divider--vertical) {
    height: 60px;
  }
}

.update-info {
  text-align: center;
  font-size: 12px;
  color: #909399;
  margin-top: 10px;
}

.card-footer {
  display: flex;
  justify-content: center;
  padding-top: 15px;
  border-top: 1px solid #EBEEF5;
}

.food-status {
  margin-bottom: 20px;
  
  .food-label-wrap {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 12px;
    
    .food-label {
      font-size: 14px;
      font-weight: 500;
      color: #606266;
    }
  }
  
  .progress-wrap {
    position: relative;
    margin-bottom: 8px;
    
    .percentage {
      position: absolute;
      right: 0;
      top: -20px;
      font-size: 14px;
      font-weight: bold;
      color: #8BAD42;
    }
  }
  
  .distance-info {
    font-size: 12px;
    color: #909399;
  }
}

.feed-stats {
  display: flex;
  background-color: #F5F7FA;
  border-radius: 8px;
  padding: 12px;
  margin-bottom: 20px;
  
  .stat-item {
    flex: 1;
    text-align: center;
    
    .stat-value {
      font-size: 18px;
      font-weight: 600;
      color: #303133;
    }
    
    .stat-label {
      font-size: 12px;
      color: #909399;
      margin-top: 4px;
    }
  }
}

.feed-control {
  .control-label {
    font-size: 14px;
    color: #606266;
    margin-bottom: 12px;
  }
  
  .control-row {
    display: flex;
    gap: 12px;
    
    :deep(.el-input-number) {
      flex: 1;
    }
    
    .feed-btn {
      flex: 1.2;
      background-color: #8BAD42;
      border-color: #8BAD42;
      
      &:hover {
        background-color: #9dc24d;
        border-color: #9dc24d;
      }
    }
  }
}

.quick-actions {
  .action-item {
    background-color: #fff;
    border-radius: 12px;
    padding: 15px 10px;
    text-align: center;
    cursor: pointer;
    transition: all 0.3s;
    border: 1px solid #EBEEF5;
    
    &:hover {
      transform: translateY(-3px);
      box-shadow: 0 4px 12px rgba(0,0,0,0.05);
      border-color: #8BAD42;
    }
    
    .action-icon {
      font-size: 24px;
      margin-bottom: 8px;
    }
    
    .action-label {
      font-size: 13px;
      color: #606266;
    }
  }
}

:deep(.el-button--primary.is-plain) {
  --el-button-text-color: #8BAD42;
  --el-button-bg-color: #f3f7ec;
  --el-button-border-color: #8BAD42;
  --el-button-hover-text-color: #fff;
  --el-button-hover-bg-color: #8BAD42;
  --el-button-hover-border-color: #8BAD42;
}

:deep(.el-tag--success) {
  --el-tag-bg-color: #8BAD42;
  --el-tag-border-color: #8BAD42;
}
</style>
