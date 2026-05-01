<template>
  <div class="page-container">
    <!-- 自定义导航栏 -->
    <div class="custom-navbar">
      <div class="navbar-content">
        <span class="navbar-title">伴鸟 · 个人中心</span>
        <div class="settings-icon" @click="gotoSettings">
          <el-icon size="20" color="#fff"><Setting /></el-icon>
        </div>
      </div>
    </div>

    <!-- 用户信息卡片 -->
    <div class="user-section">
      <div class="user-card">
        <div class="user-main">
          <div class="user-avatar">
            <img :src="userInfo.avatar || 'https://api.dicebear.com/7.x/avataaars/svg?seed=Lucky'" mode="aspectFill" />
            <div class="vip-badge" v-if="userInfo.isVip">
              <span class="vip-text">VIP</span>
            </div>
          </div>
          <div class="user-info">
            <div class="user-name-row">
              <span class="user-name">{{userInfo.nickname || '伴鸟用户'}}</span>
              <div class="user-id">
                <span>ID: {{userInfo.id || 'BN88888'}}</span>
              </div>
            </div>
            <div class="user-stats">
              <div class="stat-item">
                <span class="stat-value">{{userInfo.deviceCount || 3}}</span>
                <span class="stat-label">设备</span>
              </div>
              <div class="stat-divider"></div>
              <div class="stat-item">
                <span class="stat-value">{{userInfo.consultCount || 12}}</span>
                <span class="stat-label">问诊</span>
              </div>
              <div class="stat-divider"></div>
              <div class="stat-item">
                <span class="stat-value">{{userInfo.days || 88}}</span>
                <span class="stat-label">守护天数</span>
              </div>
            </div>
          </div>
        </div>
        <div class="user-footer">
          <div class="device-list">
            <div class="device-item">
              <span class="device-icon">🌡️</span>
              <span class="device-name">智能孵化仪</span>
            </div>
            <div class="device-item">
              <span class="device-icon">🍽️</span>
              <span class="device-name">智能喂鸟器</span>
            </div>
            <div class="device-item">
              <span class="device-icon">📷</span>
              <span class="device-name">智能摄像头</span>
            </div>
          </div>
          <div class="device-more" @click="manageDevices">
            <span>管理设备</span>
            <span class="arrow">></span>
          </div>
        </div>
      </div>
    </div>

    <!-- 服务订阅卡片 -->
    <div class="subscription-section">
      <div class="section-header">
        <div class="section-title">
          <span class="title-icon">💎</span>
          <span>服务订阅</span>
        </div>
        <span class="section-desc">解锁更多专业服务</span>
      </div>

      <div class="subscription-card">
        <!-- 订阅对比 -->
        <div class="plan-compare">
          <div 
            :class="['plan-item', selectedPlan === 'basic' ? 'active' : '']" 
            @click="selectPlan('basic')"
          >
            <div class="plan-header">
              <span class="plan-name">基础监测版</span>
              <div class="plan-badge basic">
                <span>免费</span>
              </div>
            </div>
            <div class="plan-price">
              <span class="price-value">¥0</span>
              <span class="price-period">/永久</span>
            </div>
            <div class="plan-features">
              <div class="feature-item">
                <span class="feature-icon">✓</span>
                <span class="feature-text">实时温湿度监测</span>
              </div>
              <div class="feature-item">
                <span class="feature-icon">✓</span>
                <span class="feature-text">基础数据记录</span>
              </div>
              <div class="feature-item">
                <span class="feature-icon">✓</span>
                <span class="feature-text">设备远程控制</span>
              </div>
            </div>
          </div>

          <div 
            :class="['plan-item', selectedPlan === 'premium' ? 'active' : '']" 
            @click="selectPlan('premium')"
          >
            <div class="plan-header">
              <span class="plan-name">深度健康管理版</span>
              <div class="plan-badge premium">
                <span>推荐</span>
              </div>
            </div>
            <div class="plan-price">
              <span class="price-value">¥99</span>
              <span class="price-period">/年</span>
            </div>
            <div class="plan-features">
              <div class="feature-item">
                <span class="feature-icon">✓</span>
                <span class="feature-text">基础版所有功能</span>
              </div>
              <div class="feature-item">
                <span class="feature-icon">✓</span>
                <span class="feature-text">AI健康自检</span>
              </div>
              <div class="feature-item">
                <span class="feature-icon">✓</span>
                <span class="feature-text">定期健康检测包</span>
              </div>
              <div class="feature-item">
                <span class="feature-icon">✓</span>
                <span class="feature-text">专属健康顾问</span>
              </div>
              <div class="feature-item">
                <span class="feature-icon">✓</span>
                <span class="feature-text">问诊折扣优惠</span>
              </div>
              <div class="feature-item">
                <span class="feature-icon">✓</span>
                <span class="feature-text">7×24小时客服</span>
              </div>
            </div>
          </div>
        </div>

        <!-- 深度版权益 -->
        <div class="premium-benefits" v-if="selectedPlan === 'premium'">
          <div class="benefits-title">
            <span class="title-icon">🎁</span>
            <span>深度版专属权益</span>
          </div>
          <div class="benefits-list">
            <div class="benefit-item">
              <div class="benefit-icon">📦</div>
              <div class="benefit-content">
                <span class="benefit-name">定期健康检测包邮寄</span>
                <span class="benefit-desc">每季度免费寄送专业健康检测包，包含便检卡、消毒用品等</span>
              </div>
            </div>
            <div class="benefit-item">
              <div class="benefit-icon">👨‍⚕️</div>
              <div class="benefit-content">
                <span class="benefit-name">专属健康顾问</span>
                <span class="benefit-desc">一对一健康顾问，提供饲养指导、疾病预防等专属服务</span>
              </div>
            </div>
            <div class="benefit-item">
              <div class="benefit-icon">💰</div>
              <div class="benefit-content">
                <span class="benefit-name">全年多次问诊折扣</span>
                <span class="benefit-desc">全年问诊8折优惠，紧急情况优先响应</span>
              </div>
            </div>
            <div class="benefit-item">
              <div class="benefit-icon">📊</div>
              <div class="benefit-content">
                <span class="benefit-name">高级数据分析报告</span>
                <span class="benefit-desc">AI生成深度健康分析报告，提供个性化养护建议</span>
              </div>
            </div>
            <div class="benefit-item">
              <div class="benefit-icon">🎓</div>
              <div class="benefit-content">
                <span class="benefit-name">专家在线课程</span>
                <span class="benefit-desc">免费参加专家讲座和在线课程，学习专业饲养知识</span>
              </div>
            </div>
          </div>
        </div>

        <!-- 订阅按钮 -->
        <button 
          :class="['subscribe-btn', selectedPlan === 'premium' ? 'premium' : '']" 
          @click="handleSubscribe"
        >
          <span v-if="selectedPlan === 'basic' && !userInfo.isVip">切换到基础版</span>
          <span v-else-if="selectedPlan === 'basic' && userInfo.isVip">已订阅基础版</span>
          <span v-else>立即开通深度版</span>
        </button>
      </div>
    </div>

    <!-- 功能菜单列表 -->
    <div class="menu-section">
      <div class="menu-card">
        <div class="menu-item" @click="gotoDevice">
          <div class="menu-left">
            <div class="menu-icon device-icon">
              <span>🔧</span>
            </div>
            <span class="menu-text">设备配网</span>
          </div>
          <div class="menu-right">
            <span class="menu-tips" v-if="deviceStatus.unconfigured > 0">{{deviceStatus.unconfigured}}台待配置</span>
            <span class="menu-arrow">></span>
          </div>
        </div>

        <div class="menu-item" @click="gotoOrders">
          <div class="menu-left">
            <div class="menu-icon order-icon">
              <span>📋</span>
            </div>
            <span class="menu-text">我的订单</span>
          </div>
          <div class="menu-right">
            <div class="menu-badge" v-if="orderCount > 0">
              <span>{{orderCount}}</span>
            </div>
            <span class="menu-arrow">></span>
          </div>
        </div>

        <div class="menu-item" @click="gotoRecovery">
          <div class="menu-left">
            <div class="menu-icon recovery-icon">
              <span>📈</span>
            </div>
            <span class="menu-text">康复追踪打卡</span>
          </div>
          <div class="menu-right">
            <span class="menu-tips">连续打卡{{recoveryDays}}天</span>
            <span class="menu-arrow">></span>
          </div>
        </div>

        <div class="menu-item" @click="gotoReports">
          <div class="menu-left">
            <div class="menu-icon report-icon">
              <span>📊</span>
            </div>
            <span class="menu-text">健康报告</span>
          </div>
          <div class="menu-right">
            <span class="menu-arrow">></span>
          </div>
        </div>

        <div class="menu-item" @click="gotoSupport">
          <div class="menu-left">
            <div class="menu-icon support-icon">
              <span>💬</span>
            </div>
            <span class="menu-text">联系客服</span>
          </div>
          <div class="menu-right">
            <span class="menu-tips">7×24小时</span>
            <span class="menu-arrow">></span>
          </div>
        </div>

        <div class="menu-item" @click="gotoAbout">
          <div class="menu-left">
            <div class="menu-icon about-icon">
              <span>ℹ️</span>
            </div>
            <span class="menu-text">关于伴鸟</span>
          </div>
          <div class="menu-right">
            <span class="menu-arrow">></span>
          </div>
        </div>
      </div>
    </div>

    <!-- 退出登录 -->
    <div class="logout-section">
      <button class="logout-btn" @click="handleLogout">
        <span>退出登录</span>
      </button>
    </div>

    <!-- 版本信息 -->
    <div class="version-info">
      <span>伴鸟 v1.0.0</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import { ElMessage, ElMessageBox, ElLoading } from 'element-plus'
import { Setting } from '@element-plus/icons-vue'

const router = useRouter()

// 状态管理使用 ref
const userInfo = ref({
  avatar: '',
  nickname: '养鸟达人',
  id: 'BN10086',
  isVip: false,
  deviceCount: 3,
  consultCount: 12,
  days: 88
})

const selectedPlan = ref('basic')

const deviceStatus = ref({
  total: 3,
  online: 3,
  unconfigured: 0
})

const orderCount = ref(2)
const recoveryDays = ref(15)

onMounted(() => {
  loadUserInfo()
  checkDeviceStatus()
})

const loadUserInfo = () => {
  // 从标准 localStorage 获取订阅状态
  const isVip = localStorage.getItem('isVip') === 'true'
  const plan = localStorage.getItem('selectedPlan') || 'basic'
  
  userInfo.value.isVip = isVip
  selectedPlan.value = plan
}

const checkDeviceStatus = () => {
  // 模拟检查设备状态
  const unconfigured = Math.random() > 0.8 ? 1 : 0
  deviceStatus.value.unconfigured = unconfigured
}

const selectPlan = (plan: string) => {
  selectedPlan.value = plan
}

const handleSubscribe = () => {
  const plan = selectedPlan.value
  
  if (plan === 'basic') {
    ElMessageBox.confirm(
      '确定要切换到基础监测版吗？将失去深度版所有权益。',
      '切换方案',
      {
        confirmButtonText: '确定切换',
        cancelButtonText: '取消',
        type: 'warning'
      }
    ).then(() => {
      localStorage.setItem('isVip', 'false')
      localStorage.setItem('selectedPlan', 'basic')
      userInfo.value.isVip = false
      ElMessage.success('已切换至基础版')
    }).catch(() => {})
  } else {
    // 深度版开通
    ElMessageBox.confirm(
      '费用: ¥99/年\n\n立即开通享受所有深度版权益',
      '开通深度健康管理版',
      {
        confirmButtonText: '立即开通',
        cancelButtonText: '取消',
        type: 'info'
      }
    ).then(() => {
      const loading = ElLoading.service({
        text: '开通中...',
        background: 'rgba(0, 0, 0, 0.7)'
      })
      
      // 模拟支付过程
      setTimeout(() => {
        localStorage.setItem('isVip', 'true')
        localStorage.setItem('selectedPlan', 'premium')
        userInfo.value.isVip = true
        
        loading.close()
        ElMessage.success('开通成功')
      }, 2000)
    }).catch(() => {})
  }
}

const gotoSettings = () => {
  ElMessage('设置功能开发中')
}

const manageDevices = () => {
  ElMessage('设备管理开发中')
}

const gotoDevice = () => {
  ElMessage('设备配网开发中')
}

const gotoOrders = () => {
  ElMessage('我的订单开发中')
}

const gotoRecovery = () => {
  ElMessage('康复追踪打卡开发中')
}

const gotoReports = () => {
  router.push('/cloud/consultation')
}

const gotoSupport = () => {
  ElMessageBox.confirm(
    '客服电话: 400-888-8888\n在线时间: 7×24小时',
    '联系客服',
    {
      confirmButtonText: '拨打',
      cancelButtonText: '关闭',
      type: 'info'
    }
  ).then(() => {
    // 原生 web 下可以通过 a 标签 tel: 唤起拨号，此处作简单演示
    window.location.href = 'tel:400-888-8888'
  }).catch(() => {})
}

const gotoAbout = () => {
  ElMessageBox.alert(
    '伴鸟是一款专注于异宠智能物联网管理的应用，通过智能硬件与AI技术，为您的宠物提供全方位的健康监测与管理服务。\n\n版本: v1.0.0',
    '关于伴鸟',
    {
      confirmButtonText: '确定'
    }
  )
}

const handleLogout = () => {
  ElMessageBox.confirm(
    '确定要退出登录吗？',
    '退出登录',
    {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning',
      confirmButtonClass: 'el-button--danger'
    }
  ).then(() => {
    // 清除用户信息
    localStorage.removeItem('isVip')
    localStorage.removeItem('selectedPlan')
    
    // 重置数据
    userInfo.value.isVip = false
    selectedPlan.value = 'basic'
    
    ElMessage.success('已退出登录')
  }).catch(() => {})
}
</script>

<style scoped>
.page-container {
  min-height: 100vh;
  background: linear-gradient(180deg, #E8F8F5 0%, #F5F7FA 30%);
}

/* 自定义导航栏 */
.custom-navbar {
  position: sticky;
  top: 0;
  z-index: 100;
  background: linear-gradient(135deg, #00B894, #00A884);
  padding: 44px 16px 12px;
  box-shadow: 0 2px 8px rgba(0, 184, 148, 0.2);
}

.navbar-content {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.navbar-title {
  font-size: 18px;
  font-weight: 600;
  color: #FFFFFF;
}

.settings-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  padding: 4px;
  cursor: pointer;
}

/* 用户信息卡片 */
.user-section {
  padding: 12px;
}

.user-card {
  background: linear-gradient(135deg, #FFFFFF, #F8F9FA);
  border-radius: 16px;
  padding: 20px;
  box-shadow: 0 6px 16px rgba(0, 0, 0, 0.08);
}

.user-main {
  display: flex;
  gap: 16px;
  margin-bottom: 16px;
}

.user-avatar {
  position: relative;
  width: 64px;
  height: 64px;
  flex-shrink: 0;
}

.user-avatar img {
  width: 100%;
  height: 100%;
  border-radius: 50%;
  object-fit: cover;
  border: 2px solid #FFFFFF;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.1);
}

.vip-badge {
  position: absolute;
  bottom: -2px;
  right: -2px;
  background: linear-gradient(135deg, #FFD700, #FFA500);
  padding: 3px 6px;
  border-radius: 6px;
  box-shadow: 0 1px 4px rgba(255, 215, 0, 0.4);
}

.vip-text {
  font-size: 9px;
  font-weight: 700;
  color: #FFFFFF;
}

.user-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.user-name-row {
  display: flex;
  align-items: center;
  gap: 8px;
}

.user-name {
  font-size: 20px;
  font-weight: 700;
  color: #2D3436;
}

.user-id {
  padding: 3px 6px;
  background: #F5F7FA;
  border-radius: 4px;
  font-size: 10px;
  color: #B2BEC3;
}

.user-stats {
  display: flex;
  align-items: center;
  gap: 12px;
}

.stat-item {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.stat-value {
  font-size: 18px;
  font-weight: 700;
  color: #00B894;
}

.stat-label {
  font-size: 10px;
  color: #B2BEC3;
}

.stat-divider {
  width: 0.5px;
  height: 20px;
  background: #DFE6E9;
}

.user-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-top: 16px;
  border-top: 0.5px solid #DFE6E9;
}

.device-list {
  display: flex;
  gap: 8px;
}

.device-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
  padding: 8px 10px;
  background: linear-gradient(135deg, #F5F7FA, #E8F8F5);
  border-radius: 8px;
}

.device-icon {
  font-size: 18px;
}

.device-name {
  font-size: 10px;
  color: #636E72;
}

.device-more {
  display: flex;
  align-items: center;
  gap: 2px;
  font-size: 11px;
  color: #00B894;
  cursor: pointer;
}

.arrow {
  font-size: 10px;
}

/* 订阅区域 */
.subscription-section {
  padding: 0 12px 12px;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.section-title {
  display: flex;
  align-items: center;
  gap: 6px;
  font-size: 16px;
  font-weight: 600;
  color: #2D3436;
}

.title-icon {
  font-size: 20px;
}

.section-desc {
  font-size: 12px;
  color: #636E72;
}

.subscription-card {
  background: #FFFFFF;
  border-radius: 16px;
  padding: 16px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.06);
}

/* 订阅对比 */
.plan-compare {
  display: flex;
  gap: 8px;
  margin-bottom: 16px;
}

.plan-item {
  flex: 1;
  padding: 12px;
  border-radius: 12px;
  border: 1.5px solid #DFE6E9;
  transition: all 0.3s ease;
  cursor: pointer;
}

.plan-item.active {
  border-color: #00B894;
  background: linear-gradient(135deg, #E8F8F5, #FFFFFF);
  box-shadow: 0 4px 10px rgba(0, 184, 148, 0.15);
}

.plan-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
}

.plan-name {
  font-size: 14px;
  font-weight: 600;
  color: #2D3436;
}

.plan-badge {
  padding: 3px 6px;
  border-radius: 4px;
  font-size: 10px;
  font-weight: 500;
}

.plan-badge.basic {
  background: #DFE6E9;
  color: #636E72;
}

.plan-badge.premium {
  background: linear-gradient(135deg, #FF7675, #D63031);
  color: #FFFFFF;
}

.plan-price {
  display: flex;
  align-items: baseline;
  gap: 4px;
  margin-bottom: 12px;
}

.price-value {
  font-size: 24px;
  font-weight: 700;
  color: #FF7675;
}

.price-period {
  font-size: 12px;
  color: #B2BEC3;
}

.plan-features {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.feature-item {
  display: flex;
  align-items: center;
  gap: 6px;
}

.feature-icon {
  font-size: 12px;
  color: #00B894;
}

.feature-text {
  font-size: 12px;
  color: #636E72;
  line-height: 1.5;
}

/* 深度版权益 */
.premium-benefits {
  padding: 16px;
  background: linear-gradient(135deg, #FFF9E6, #FFF4D6);
  border-radius: 12px;
  margin-bottom: 16px;
  border: 1px solid #FFD700;
}

.benefits-title {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 12px;
}

.benefits-title span {
  font-size: 14px;
  font-weight: 600;
  color: #2D3436;
}

.benefits-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.benefit-item {
  display: flex;
  gap: 8px;
}

.benefit-icon {
  width: 28px;
  height: 28px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #FFFFFF;
  border-radius: 6px;
  font-size: 16px;
  flex-shrink: 0;
}

.benefit-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.benefit-name {
  font-size: 13px;
  font-weight: 600;
  color: #2D3436;
}

.benefit-desc {
  font-size: 11px;
  color: #636E72;
  line-height: 1.6;
}

/* 订阅按钮 */
.subscribe-btn {
  width: 100%;
  padding: 14px;
  background: #B2BEC3;
  color: #FFFFFF;
  border-radius: 24px;
  font-size: 16px;
  font-weight: 600;
  border: none;
  transition: all 0.3s ease;
  cursor: pointer;
  outline: none;
}

.subscribe-btn.premium {
  background: linear-gradient(135deg, #FF7675, #D63031);
  box-shadow: 0 4px 12px rgba(214, 48, 49, 0.3);
}

/* 功能菜单 */
.menu-section {
  padding: 0 12px 12px;
}

.menu-card {
  background: #FFFFFF;
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.06);
}

.menu-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  border-bottom: 0.5px solid #F5F7FA;
  transition: background 0.2s ease;
  cursor: pointer;
}

.menu-item:active {
  background: #F8F9FA;
}

.menu-item:last-child {
  border-bottom: none;
}

.menu-left {
  display: flex;
  align-items: center;
  gap: 8px;
}

.menu-icon {
  width: 28px;
  height: 28px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 6px;
  font-size: 16px;
}

.device-icon { background: linear-gradient(135deg, #74B9FF, #0984E3); }
.order-icon { background: linear-gradient(135deg, #A29BFE, #6C5CE7); }
.recovery-icon { background: linear-gradient(135deg, #00B894, #00A884); }
.report-icon { background: linear-gradient(135deg, #FDCB6E, #F39C12); }
.support-icon { background: linear-gradient(135deg, #FF7675, #D63031); }
.about-icon { background: linear-gradient(135deg, #B2BEC3, #636E72); }

.menu-text {
  font-size: 14px;
  color: #2D3436;
  font-weight: 500;
}

.menu-right {
  display: flex;
  align-items: center;
  gap: 6px;
}

.menu-tips {
  font-size: 11px;
  color: #B2BEC3;
}

.menu-badge {
  padding: 2px 5px;
  background: #FF7675;
  color: #FFFFFF;
  border-radius: 5px;
  font-size: 10px;
  font-weight: 500;
}

.menu-arrow {
  font-size: 12px;
  color: #B2BEC3;
}

/* 退出登录 */
.logout-section {
  padding: 0 12px 12px;
}

.logout-btn {
  width: 100%;
  padding: 14px;
  background: #FFFFFF;
  color: #FF7675;
  border-radius: 24px;
  font-size: 14px;
  font-weight: 600;
  border: none;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.06);
  cursor: pointer;
  outline: none;
}

/* 版本信息 */
.version-info {
  text-align: center;
  padding: 12px;
  font-size: 11px;
  color: #B2BEC3;
}
</style>