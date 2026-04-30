<template>
  <div class="home-wrapper">
    <div class="home-container">
      <!-- 顶部预警通知栏：使用 Element Plus 的 el-alert -->
      <div v-if="showAlert" style="margin-bottom: 12px;">
        <el-alert
          :title="alertTitle"
          :description="alertMessage"
          :type="alertLevel === 'danger' ? 'error' : alertLevel === 'info' ? 'info' : 'warning'"
          show-icon
          @close="closeAlert"
          style="border-radius: 8px;"
        />
      </div>

      <!-- 核心数据卡片区域：使用 Element Plus 的 el-card -->
      <div class="data-cards">
        
        <!-- 温湿度监测卡片 -->
        <el-card class="custom-card incubator-card" shadow="hover" :body-style="{ padding: '16px' }">
          <div class="card-header">
            <div class="card-title-wrap">
              <span class="card-icon">🌡️</span>
              <span class="card-title">温湿度检测仪</span>
            </div>
            <div class="card-status online">
              <span class="status-dot"></span>
              <span class="status-text">在线</span>
            </div>
          </div>
          
          <div class="card-body">
            <div class="data-row">
              <div class="data-item temp">
                <div class="data-value temp">
                  <span class="number">{{temp}}</span>
                  <span class="unit">℃</span>
                </div>
                <div class="data-label">实时温度</div>
                <div class="data-precision">精度 ±0.5℃</div>
              </div>
              
              <div class="data-divider"></div>
              
              <div class="data-item humidity">
                <div class="data-value humidity">
                  <span class="number">{{humid}}</span>
                  <span class="unit">%RH</span>
                </div>
                <div class="data-label">实时湿度</div>
                <div class="data-precision">精度 ±3%RH</div>
              </div>
            </div>
            
            <div class="update-time">
              <span>更新时间：{{updateTime}}</span>
            </div>
          </div>
          
          <div class="card-footer">
            <el-button round style="width: 100%; border-color: #8BAD42; color: #8BAD42;" @click="refreshData">
              <span class="btn-icon">🔄</span>
              <span>刷新数据</span>
            </el-button>
          </div>
        </el-card>

        <!-- 设备控制卡片 -->
        <el-card class="custom-card feeder-card" shadow="hover" :body-style="{ padding: '16px' }">
          <div class="card-header">
            <div class="card-title-wrap">
              <span class="card-icon">🥣</span>
              <span class="card-title">智能喂鸟器</span>
            </div>
            <div class="card-status online">
              <span class="status-dot"></span>
              <span class="status-text">在线</span>
            </div>
          </div>
          
          <div class="card-body">
            <div class="food-status">
              <div class="food-info">
                <div style="display: flex; justify-content: space-between; align-items: baseline;">
                  <span class="food-label">余粮状态</span>
                  <span class="food-level" style="font-size: 24px; font-weight: bold; color: #8BAD42;">{{foodLevel}}%</span>
                </div>
                <!-- 替换为 Element Plus 进度条 -->
                <el-progress 
                  :percentage="foodLevel" 
                  :color="foodLevel < 30 ? '#f56c6c' : '#8BAD42'" 
                  :stroke-width="10" 
                  :show-text="false"
                  style="margin-top: 4px; margin-bottom: 8px;"
                />
                <div style="font-size: 12px; color: #888;">
                  雷达测距：{{foodDistance}} cm
                </div>
              </div>
            </div>
            
            <div class="feed-stats">
              <div class="stat-item">
                <span class="stat-value">{{todayFeedCount}}</span>
                <span class="stat-label">今日进食</span>
              </div>
              <div class="stat-divider"></div>
              <div class="stat-item">
                <span class="stat-value">{{lastFeedTime}}</span>
                <span class="stat-label">上次投喂</span>
              </div>
            </div>
            
            <div class="feed-control">
              <div class="control-title">定量投喂</div>
              <div class="control-row">
                <button class="feed-btn" @click="decreaseFood">
                  <span>-</span>
                </button>
                <div class="feed-amount">
                  <span class="amount-value">{{feedAmount}}</span>
                  <span class="amount-unit">克</span>
                </div>
                <button class="feed-btn" @click="increaseFood">
                  <span>+</span>
                </button>
              </div>
              <el-button type="primary" round style="width: 100%; background-color: #8BAD42; border-color: #8BAD42;" @click="feedNow">
                <span class="btn-icon">🎯</span>
                <span>立即投喂</span>
              </el-button>
            </div>
          </div>
        </el-card>

        <!-- 设备总览 -->
        <el-card class="custom-card devices-card" shadow="hover" :body-style="{ padding: '16px' }">
          <div class="card-header">
            <div class="card-title-wrap">
              <span class="card-icon">📱</span>
              <span class="card-title">设备总览</span>
            </div>
          </div>
          
          <div class="device-list">
            <div class="device-item" v-for="item in devices" :key="item.id">
              <div class="device-icon">
                <span>{{item.icon}}</span>
              </div>
              <div class="device-info">
                <span class="device-name">{{item.name}}</span>
                <span :class="['device-status', item.online ? 'online' : 'offline']">
                  {{item.online ? '在线' : '离线'}}
                </span>
              </div>
              <div class="device-arrow">
                <span class="arrow">›</span>
              </div>
            </div>
          </div>
        </el-card>
      </div>

      <!-- 快捷功能入口 -->
      <div class="quick-actions">
        <div class="quick-item" @click="goToAI">
          <span class="quick-icon">🤖</span>
          <span class="quick-label">AI自检</span>
        </div>
        <div class="quick-item" @click="goToClinic">
          <span class="quick-icon">👨‍⚕️</span>
          <span class="quick-label">在线问诊</span>
        </div>
        <div class="quick-item" @click="goToReport">
          <span class="quick-icon">📊</span>
          <span class="quick-label">健康报告</span>
        </div>
        <div class="quick-item" @click="goToSettings">
          <span class="quick-icon">⚙️</span>
          <span class="quick-label">设备设置</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue'
import { ElMessage, ElMessageBox, ElLoading } from 'element-plus'
import { useRouter } from 'vue-router'

const router = useRouter()

// 预警信息
const showAlert = ref(true)
const alertLevel = ref('warning')
const alertTitle = ref('活动量提醒')
const alertMessage = ref('检测到活动量骤减，请注意观察')

// 温湿度数据
const temp = ref<number | string>(37.5)
const humid = ref<number | string>(65)
const updateTime = ref('10:30:25')

// 喂鸟器数据
const foodLevel = ref(75)
const foodDistance = ref<number | string>('--')
const todayFeedCount = ref(3)
const lastFeedTime = ref('09:15')
const feedAmount = ref(20)

// 设备列表
const devices = ref([
  { id: 1, name: '温湿度检测仪', icon: '🌡️', online: true },
  { id: 2, name: '智能喂鸟器', icon: '🥣', online: true },
  { id: 3, name: '环境传感器', icon: '📊', online: true }
])

let radarTimer: ReturnType<typeof setInterval> | null = null

onMounted(() => {
  getBirdFoodData()
  getBirdEnvData()

  // 开启轮询，每隔 5 秒向巴法云请求最新数据
  radarTimer = setInterval(() => {
    getBirdFoodData()
    getBirdEnvData()
  }, 5000)
})

onUnmounted(() => {
  if (radarTimer) {
    clearInterval(radarTimer)
  }
})

// ==================== 巴法云取件函数 ====================
const getBirdFoodData = async () => {
  try {
    const res = await fetch('https://api.bemfa.com/api/device/v1/data/1/get/?uid=8a512c3a9261c60beea0bce9e3ee38a7&topic=birdFood&num=1', {
      method: 'GET'
    })
    const data = await res.json()
    if (data && data.msg) {
      const distance = parseFloat(data.msg)
      foodDistance.value = distance
      
      // 换算成余粮百分比 (假设粮仓总深度是 20 厘米)
      let calcFoodLevel = 100 - (distance / 20 * 100)
      if (calcFoodLevel < 0) calcFoodLevel = 0
      if (calcFoodLevel > 100) calcFoodLevel = 100
      
      foodLevel.value = parseInt(calcFoodLevel.toString())
      checkAlerts()
    }
  } catch (error) {
    console.error("获取余粮云端数据失败", error)
  }
}

const getBirdEnvData = async () => {
  try {
    const res = await fetch('https://api.bemfa.com/api/device/v1/data/1/get/?uid=8a512c3a9261c60beea0bce9e3ee38a7&topic=birdEnv&num=1', {
      method: 'GET'
    })
    const data = await res.json()
    if (data && data.msg) {
      let rawData = data.msg
      let envData = rawData.split('#')
      if(envData.length === 2) {
        temp.value = envData[0]
        humid.value = envData[1]
      }
      
      const now = new Date()
      updateTime.value = now.toTimeString().substring(0, 8)
      checkAlerts()
    }
  } catch (error) {
    console.error("获取环境云端数据失败", error)
  }
}
// =========================================================

const refreshData = () => {
  const loading = ElLoading.service({
    text: '向服务器请求中...',
    background: 'rgba(0, 0, 0, 0.7)'
  })
  
  getBirdFoodData()
  getBirdEnvData()

  setTimeout(() => {
    loading.close()
    ElMessage({
      message: '刷新真数据成功',
      type: 'success'
    })
  }, 1000)
}

const checkAlerts = () => {
  const currentTemp = parseFloat(temp.value.toString())
  const currentFoodLevel = foodLevel.value
  
  if (currentFoodLevel < 30) {
    showAlert.value = true
    alertLevel.value = 'danger'
    alertTitle.value = '粮食不足'
    alertMessage.value = '余粮不足30%，请及时补充'
  } else if (currentTemp > 39 || currentTemp < 35) {
    showAlert.value = true
    alertLevel.value = 'warning'
    alertTitle.value = '温度异常'
    alertMessage.value = `当前温度${currentTemp}℃，超出正常范围`
  } else {
    showAlert.value = false
  }
}

const closeAlert = () => {
  showAlert.value = false
}

const decreaseFood = () => {
  if (feedAmount.value > 5) {
    feedAmount.value -= 5
  }
}

const increaseFood = () => {
  if (feedAmount.value < 50) {
    feedAmount.value += 5
  }
}

const feedNow = () => {
  ElMessageBox.confirm(
    `确定要投喂 ${feedAmount.value}克 粮食吗？`,
    '确认投喂',
    {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'info',
    }
  ).then(() => {
    const loading = ElLoading.service({
      text: '投喂中...',
      background: 'rgba(0, 0, 0, 0.7)'
    })

    setTimeout(() => {
      const now = new Date()
      lastFeedTime.value = now.toTimeString().substring(0, 5)
      foodLevel.value = Math.max(0, foodLevel.value - Math.floor(feedAmount.value / 5))
      todayFeedCount.value += 1

      loading.close()
      ElMessage({
        message: '投喂成功',
        type: 'success'
      })
      
      checkAlerts()
    }, 1500)
  }).catch(() => {
    // 取消投喂
  })
}

const goToAI = () => {
  router.push('/cloud/checkup')
}

const goToClinic = () => {
  router.push('/cloud/consultation')
}

const goToReport = () => {
  ElMessage('功能开发中')
}

const goToSettings = () => {
  ElMessage('功能开发中')
}
</script>

<style scoped>
.home-wrapper {
  width: 100%;
}

.home-container {
  min-height: 100vh;
  padding: 12px;
  background: linear-gradient(180deg, #f5f7fa 0%, #ffffff 100%);
  --radius-sm: 4px;
  --radius-md: 8px;
  --radius-lg: 16px;
  --shadow-light: 0 2px 12px 0 rgba(0, 0, 0, 0.05);
  --info-color: #909399;
  --warning-color: #e6a23c;
  --danger-color: #f56c6c;
  --success-color: #8BAD42;
  --primary-color: #8BAD42;
  --primary-light: #a6c965;
  --secondary-color: #409EFF;
  --text-primary: #303133;
  --text-secondary: #606266;
  --text-light: #909399;
  --bg-card: #ffffff;
  --bg-secondary: #f5f7fa;
  --border-color: #ebeef5;
}

/* 覆盖 el-card 的圆角，去除边框 */
.custom-card {
  border-radius: var(--radius-lg);
  border: none;
  margin-bottom: 12px;
  box-shadow: var(--shadow-light) !important;
}

/* 数据卡片 */
.data-cards { 
  display: flex; 
  flex-direction: column; 
  gap: 12px; 
}
.card-header { 
  display: flex; 
  justify-content: space-between; 
  align-items: center; 
  margin-bottom: 12px; 
}
.card-title-wrap { 
  display: flex; 
  align-items: center; 
  gap: 6px; 
}
.card-icon { 
  font-size: 18px; 
}
.card-title { 
  font-size: 16px; 
  font-weight: 600; 
  color: var(--text-primary); 
}
.card-status { 
  display: flex; 
  align-items: center; 
  gap: 4px; 
  padding: 4px 8px; 
  background: rgba(139, 173, 66, 0.1); 
  border-radius: 10px; 
}
.card-status.online .status-dot { 
  width: 6px; 
  height: 6px; 
  background: var(--success-color); 
  border-radius: 50%; 
}
.card-status.offline .status-dot { 
  width: 6px; 
  height: 6px; 
  background: var(--text-light); 
  border-radius: 50%; 
}
.status-text { 
  font-size: 12px; 
  color: var(--success-color); 
}
.card-status.offline .status-text { 
  color: var(--text-light); 
}

/* 孵化监测仪卡片 */
.incubator-card .card-body { margin-bottom: 12px; }
.data-row { display: flex; align-items: center; justify-content: space-between; gap: 12px; }
.data-item { flex: 1; text-align: center; padding: 12px; background: linear-gradient(135deg, #f8f9fa 0%, #ffffff 100%); border-radius: var(--radius-md); }
.data-item.temp { border: 1px solid rgba(139, 173, 66, 0.2); }
.data-item.humidity { border: 1px solid rgba(64, 158, 255, 0.2); }
.data-value { font-size: 28px; font-weight: 600; margin-bottom: 6px; display: flex; align-items: baseline; justify-content: center; gap: 2px; }
.data-value .number { font-size: 36px; }
.data-value.temp .number { color: var(--primary-color); }
.data-value.humidity .number { color: var(--secondary-color); }
.unit { font-size: 16px; color: var(--text-secondary); font-weight: 500; }
.data-label { font-size: 14px; color: var(--text-primary); margin-bottom: 4px; }
.data-precision { font-size: 10px; color: var(--text-light); background: var(--bg-secondary); padding: 2px 6px; border-radius: 6px; display: inline-block; }
.data-divider { width: 1px; height: 40px; background: var(--border-color); }
.update-time { text-align: center; font-size: 12px; color: var(--text-light); margin-top: 12px; }
.card-footer { text-align: center; margin-top: 12px; }

/* 喂鸟器卡片 */
.food-status { display: flex; justify-content: space-between; align-items: center; padding: 10px; background: var(--bg-secondary); border-radius: var(--radius-md); margin-bottom: 12px; }
.food-info { flex: 1; }
.food-label { display: block; font-size: 12px; color: var(--text-secondary); margin-bottom: 6px; }

.feed-stats { display: flex; align-items: center; justify-content: space-around; padding: 12px; background: linear-gradient(135deg, #f8f9fa 0%, #ffffff 100%); border-radius: var(--radius-md); margin-bottom: 12px; }
.stat-item { text-align: center; flex: 1; }
.stat-value { display: block; font-size: 18px; font-weight: 600; color: var(--text-primary); margin-bottom: 4px; }
.stat-label { font-size: 12px; color: var(--text-secondary); }
.stat-divider { width: 1px; height: 30px; background: var(--border-color); }

/* 投喂控制 */
.feed-control { padding: 12px; background: var(--bg-secondary); border-radius: var(--radius-md); }
.control-title { font-size: 14px; font-weight: 600; color: var(--text-primary); margin-bottom: 10px; text-align: center; }
.control-row { display: flex; align-items: center; justify-content: center; gap: 12px; margin-bottom: 12px; }
.feed-btn { width: 36px; height: 36px; border-radius: 50%; background: var(--bg-card); border: 1px solid var(--border-color); font-size: 24px; color: var(--text-primary); display: flex; align-items: center; justify-content: center; padding: 0; cursor: pointer; outline: none; transition: background-color 0.2s; }
.feed-btn:active { background: #f0f2f5; }
.feed-amount { min-width: 100px; text-align: center; }
.amount-value { font-size: 36px; font-weight: 600; color: var(--primary-color); }
.amount-unit { font-size: 16px; color: var(--text-secondary); margin-left: 4px; }

/* 设备列表 */
.device-list { display: flex; flex-direction: column; }
.device-item { display: flex; align-items: center; padding: 12px; background: var(--bg-secondary); border-radius: var(--radius-md); margin-bottom: 8px; }
.device-item:last-child { margin-bottom: 0; }
.device-icon { width: 40px; height: 40px; background: linear-gradient(135deg, var(--primary-color), var(--primary-light)); border-radius: var(--radius-md); display: flex; align-items: center; justify-content: center; font-size: 20px; margin-right: 10px; }
.device-info { flex: 1; }
.device-name { display: block; font-size: 14px; font-weight: 500; color: var(--text-primary); margin-bottom: 4px; }
.device-status { display: inline-block; padding: 2px 6px; border-radius: 6px; font-size: 10px; }
.device-status.online { background: rgba(139, 173, 66, 0.1); color: var(--success-color); }
.device-status.offline { background: rgba(144, 147, 153, 0.1); color: var(--text-light); }
.device-arrow { font-size: 24px; color: var(--text-light); }

/* 快捷功能入口 */
.quick-actions { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; margin-top: 16px; padding: 16px; background: var(--bg-card); border-radius: var(--radius-lg); box-shadow: var(--shadow-light); }
.quick-item { display: flex; flex-direction: column; align-items: center; padding: 10px 6px; background: var(--bg-secondary); border-radius: var(--radius-md); transition: all 0.3s; cursor: pointer; }
.quick-item:active { transform: scale(0.95); }
.quick-icon { font-size: 24px; margin-bottom: 6px; }
.quick-label { font-size: 12px; color: var(--text-secondary); text-align: center; }

/* General utility overrides */
.btn-icon { margin-right: 4px; font-size: 14px; }
</style>
