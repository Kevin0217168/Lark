<template>
  <div class="page-container">
    <!-- 自定义导航栏保留 -->
    <div class="custom-navbar">
      <div class="navbar-content">
        <span class="navbar-title">伴鸟 · 线上问诊</span>
        <div class="clinic-badge">
          <span class="badge-icon">🏥</span>
          <span class="badge-text">专业兽医</span>
        </div>
      </div>
    </div>

    <!-- 在线急诊入口 -->
    <div class="emergency-section">
      <div class="emergency-card" @click="handleEmergency">
        <div class="emergency-content">
          <div class="emergency-icon">⚡</div>
          <div class="emergency-info">
            <span class="emergency-title">在线急诊</span>
            <span class="emergency-desc">紧急情况优先响应，24小时在线</span>
          </div>
        </div>
        <div class="emergency-btn">
          <span class="btn-text">立即求助</span>
          <span class="btn-arrow">></span>
        </div>
      </div>
    </div>

    <!-- 搜索筛选区 -->
    <div class="search-section">
      <div class="search-box">
        <!-- 升级为 Element Plus 输入框 -->
        <el-input 
          v-model="searchKeyword" 
          placeholder="搜索医生姓名或专科" 
          clearable 
          :prefix-icon="Search"
          class="custom-search-input"
        />
      </div>
      <div class="filter-tags">
        <div 
          :class="['filter-tag', selectedFilter === 'all' ? 'active' : '']" 
          @click="selectedFilter = 'all'"
        >
          <span>全部</span>
        </div>
        <div 
          :class="['filter-tag', selectedFilter === '鸟类' ? 'active' : '']" 
          @click="selectedFilter = '鸟类'"
        >
          <span>鸟类专科</span>
        </div>
        <div 
          :class="['filter-tag', selectedFilter === '爬行' ? 'active' : '']" 
          @click="selectedFilter = '爬行'"
        >
          <span>爬行类</span>
        </div>
        <div 
          :class="['filter-tag', selectedFilter === '两栖' ? 'active' : '']" 
          @click="selectedFilter = '两栖'"
        >
          <span>两栖类</span>
        </div>
      </div>
    </div>

    <!-- 电子健康报告入口 -->
    <div class="report-section">
      <div class="report-card" @click="gotoReports">
        <div class="report-header">
          <div class="report-title">
            <span class="title-icon">📋</span>
            <span>我的电子健康报告与医嘱</span>
          </div>
          <div class="report-badge" v-if="unreadReports > 0">
            <span>{{unreadReports}}</span>
          </div>
        </div>
        <div class="report-preview">
          <div class="preview-item">
            <span class="preview-label">最新报告</span>
            <span class="preview-value">{{latestReport}}</span>
          </div>
          <div class="preview-divider"></div>
          <div class="preview-item">
            <span class="preview-label">医嘱状态</span>
            <span :class="['preview-value', hasAdvice ? 'has-advice' : '']">
              {{hasAdvice ? '待执行' : '暂无医嘱'}}
            </span>
          </div>
        </div>
        <div class="report-footer">
          <span class="report-tips">包含健康评估、诊断报告、用药指导等完整记录</span>
        </div>
      </div>
    </div>

    <!-- 医生列表区 -->
    <div class="doctors-section">
      <div class="section-header">
        <span class="section-title">认证兽医团队</span>
        <div class="section-sort" @click="toggleSort">
          <span class="sort-text">{{sortType === 'rating' ? '评分优先' : '接诊量优先'}}</span>
          <span class="sort-icon">🔄</span>
        </div>
      </div>

      <div class="doctors-list">
        <!-- 升级为 Element Plus 卡片 -->
        <el-card 
          class="doctor-card" 
          v-for="item in filteredDoctors" 
          :key="item.id"
          @click="viewDoctorDetail(item.id)"
          shadow="hover"
          :body-style="{ padding: '16px', display: 'flex', flexDirection: 'column' }"
        >
          <div class="doctor-main">
            <div class="doctor-avatar">
              <img :src="item.avatar" alt="avatar" />
              <div :class="['online-status', item.isOnline ? 'online' : 'offline']">
                <span class="status-dot"></span>
              </div>
            </div>
            <div class="doctor-info">
              <div class="doctor-header">
                <span class="doctor-name">{{item.name}}</span>
                <div class="doctor-tags">
                  <span class="tag tag-certified">认证</span>
                  <span class="tag tag-specialty">{{item.specialty}}</span>
                </div>
              </div>
              <span class="doctor-title">{{item.title}}</span>
              <div class="doctor-stats">
                <div class="stat-item">
                  <span class="stat-icon">⭐</span>
                  <span class="stat-value">{{item.rating}}</span>
                </div>
                <div class="stat-divider">|</div>
                <div class="stat-item">
                  <span class="stat-icon">👥</span>
                  <span class="stat-value">{{item.consultCount}}次</span>
                </div>
                <div class="stat-divider">|</div>
                <div class="stat-item">
                  <span class="stat-icon">🎯</span>
                  <span class="stat-value">{{item.successRate}}%</span>
                </div>
              </div>
              <span class="doctor-intro">{{item.intro}}</span>
            </div>
          </div>
          
          <div class="doctor-footer">
            <div class="consult-price">
              <span class="price-label">咨询费</span>
              <span class="price-value">¥{{item.price}}</span>
            </div>
            <button 
              :class="['consult-btn', item.isOnline ? 'active' : '']" 
              @click.stop="bookDoctor(item.id)"
            >
              <span v-if="item.isOnline">立即预约</span>
              <span v-else>离线中</span>
            </button>
          </div>
        </el-card>
      </div>
    </div>

    <!-- 加载更多 -->
    <div class="load-more" v-if="hasMore">
      <span v-if="!loadingMore">加载更多医生</span>
      <span v-else>加载中...</span>
    </div>

    <!-- 空状态 -->
    <div class="empty-state" v-if="filteredDoctors.length === 0">
      <div class="empty-icon">🔍</div>
      <span class="empty-text">未找到相关医生</span>
      <span class="empty-tip">试试其他关键词或筛选条件</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { ElMessage, ElMessageBox } from 'element-plus'
import { Search } from '@element-plus/icons-vue'

// 状态管理转换为 ref
const searchKeyword = ref('')
const selectedFilter = ref('all')
const sortType = ref('rating')

// 电子报告数据
const unreadReports = ref(2)
const latestReport = ref('2026/2/20 健康体检报告')
const hasAdvice = ref(true)

// 医生列表数据
const doctors = ref([
  {
    id: 1,
    name: '张伟华',
    title: '首席兽医 / 异宠专科',
    specialty: '鸟类',
    avatar: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Felix',
    rating: 4.9,
    consultCount: 12580,
    successRate: 98,
    price: 68,
    isOnline: true,
    intro: '从事异宠临床诊疗15年，擅长鸟类疾病诊断与治疗，发表论文30余篇，曾获全国优秀兽医称号。'
  },
  {
    id: 2,
    name: '李晓梅',
    title: '资深兽医 / 爬行类专家',
    specialty: '爬行',
    avatar: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Aneka',
    rating: 4.8,
    consultCount: 8920,
    successRate: 96,
    price: 58,
    isOnline: true,
    intro: '专注爬行动物健康管理12年，擅长蜥蜴、龟类疾病防治，拥有丰富的异宠手术经验。'
  },
  {
    id: 3,
    name: '王建国',
    title: '高级兽医 / 两栖类专家',
    specialty: '两栖',
    avatar: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Jack',
    rating: 4.7,
    consultCount: 6540,
    successRate: 95,
    price: 48,
    isOnline: false,
    intro: '两栖动物疾病专家，擅长蛙类、蝾螈的健康管理与疾病诊断，多次参与国际学术交流。'
  }
])

const hasMore = ref(false)
const loadingMore = ref(false)

// 核心逻辑重构：彻底删除手动 filter 方法，使用 computed 实现自动计算属性
const filteredDoctors = computed(() => {
  let filtered = [...doctors.value]
  
  // 关键词搜索
  if (searchKeyword.value) {
    const keyword = searchKeyword.value.toLowerCase()
    filtered = filtered.filter(doctor => 
      doctor.name.toLowerCase().includes(keyword) ||
      doctor.specialty.toLowerCase().includes(keyword) ||
      doctor.title.toLowerCase().includes(keyword)
    )
  }
  
  // 分类筛选
  if (selectedFilter.value !== 'all') {
    filtered = filtered.filter(doctor => 
      doctor.specialty === selectedFilter.value
    )
  }
  
  // 排序逻辑
  if (sortType.value === 'rating') {
    filtered.sort((a, b) => b.rating - a.rating)
  } else {
    filtered.sort((a, b) => b.consultCount - a.consultCount)
  }
  
  return filtered
})

const toggleSort = () => {
  sortType.value = sortType.value === 'rating' ? 'consultCount' : 'rating'
}

// 替换 wx API 为 Element Plus
const handleEmergency = () => {
  ElMessageBox.confirm(
    '是否立即连接在线急诊通道？紧急情况将优先响应。',
    '在线急诊',
    {
      confirmButtonText: '立即连接',
      cancelButtonText: '取消',
      type: 'warning'
    }
  ).then(() => {
    ElMessage({
      message: '正在连接急诊...',
      type: 'info'
    })
    
    setTimeout(() => {
      ElMessage({
        message: '急诊通道已连接',
        type: 'success'
      })
    }, 2000)
  }).catch(() => {
    // cancelled
  })
}

const gotoReports = () => {
  ElMessage('打开健康报告')
}

const viewDoctorDetail = (id: number) => {
  ElMessage(`查看医生详情 ID: ${id}`)
}

const bookDoctor = (id: number) => {
  const doctor = doctors.value.find(d => d.id === id)
  if (!doctor) return

  if (!doctor.isOnline) {
    ElMessage.warning('医生当前离线')
    return
  }
  
  ElMessageBox.confirm(
    `咨询费用: ¥${doctor.price}\n\n是否确认预约？`,
    `预约${doctor.name}医生`,
    {
      confirmButtonText: '确认预约',
      cancelButtonText: '取消',
      type: 'info'
    }
  ).then(() => {
    ElMessage.success('预约成功')
  }).catch(() => {
    // cancelled
  })
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

.clinic-badge {
  display: flex;
  align-items: center;
  gap: 4px;
  background: rgba(255, 255, 255, 0.2);
  padding: 4px 8px;
  border-radius: 10px;
}

.badge-icon {
  font-size: 14px;
}

.badge-text {
  font-size: 12px;
  color: #FFFFFF;
  font-weight: 500;
}

/* 急诊入口 */
.emergency-section {
  padding: 12px;
}

.emergency-card {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 16px;
  background: linear-gradient(135deg, #FF7675, #D63031);
  border-radius: 12px;
  box-shadow: 0 4px 12px rgba(214, 48, 49, 0.3);
  cursor: pointer;
}

.emergency-content {
  display: flex;
  align-items: center;
  gap: 12px;
}

.emergency-icon {
  font-size: 32px;
  animation: pulse 1.5s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% { transform: scale(1); }
  50% { transform: scale(1.1); }
}

.emergency-info {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.emergency-title {
  font-size: 18px;
  font-weight: 700;
  color: #FFFFFF;
}

.emergency-desc {
  font-size: 11px;
  color: rgba(255, 255, 255, 0.9);
}

.emergency-btn {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
  background: rgba(255, 255, 255, 0.2);
  padding: 10px 16px;
  border-radius: 10px;
  backdrop-filter: blur(5px);
}

.btn-text {
  font-size: 13px;
  color: #FFFFFF;
  font-weight: 500;
}

.btn-arrow {
  font-size: 12px;
  color: #FFFFFF;
}

/* 搜索筛选区 */
.search-section {
  padding: 0 12px 12px;
}

/* 修改以适配 Element Plus */
.search-box {
  margin-bottom: 12px;
  border-radius: 24px;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.06);
  background: #fff;
  overflow: hidden;
}

:deep(.custom-search-input .el-input__wrapper) {
  border-radius: 24px;
  box-shadow: none;
  background-color: transparent;
  padding: 2px 12px;
}

:deep(.custom-search-input .el-input__inner) {
  height: 38px;
  font-size: 14px;
}

.filter-tags {
  display: flex;
  gap: 8px;
  overflow-x: auto;
  padding-bottom: 4px;
}

.filter-tags::-webkit-scrollbar {
  display: none;
}

.filter-tag {
  padding: 6px 12px;
  background: #FFFFFF;
  border-radius: 12px;
  font-size: 12px;
  color: #636E72;
  white-space: nowrap;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.06);
  transition: all 0.3s ease;
  cursor: pointer;
}

.filter-tag.active {
  background: linear-gradient(135deg, #00B894, #55EFC4);
  color: #FFFFFF;
  font-weight: 500;
}

/* 电子健康报告入口 */
.report-section {
  padding: 0 12px 12px;
}

.report-card {
  background: linear-gradient(135deg, #FFFFFF, #F8F9FA);
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.06);
  border: 1px solid #00B894;
  cursor: pointer;
}

.report-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.report-title {
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

.report-badge {
  background: #FF7675;
  color: #FFFFFF;
  padding: 4px 8px;
  border-radius: 10px;
  font-size: 11px;
  font-weight: 600;
}

.report-preview {
  display: flex;
  gap: 12px;
  margin-bottom: 12px;
}

.preview-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.preview-label {
  font-size: 11px;
  color: #B2BEC3;
}

.preview-value {
  font-size: 13px;
  color: #2D3436;
  font-weight: 500;
}

.preview-value.has-advice {
  color: #FF7675;
  font-weight: 600;
}

.preview-divider {
  width: 0.5px;
  background: #DFE6E9;
}

.report-footer {
  padding-top: 12px;
  border-top: 0.5px solid #DFE6E9;
}

.report-tips {
  font-size: 11px;
  color: #636E72;
  line-height: 1.6;
}

/* 医生列表区 */
.doctors-section {
  padding: 0 12px 16px;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.section-title {
  font-size: 16px;
  font-weight: 600;
  color: #2D3436;
}

.section-sort {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px 10px;
  background: #FFFFFF;
  border-radius: 10px;
  box-shadow: 0 1px 4px rgba(0, 0, 0, 0.06);
  cursor: pointer;
}

.sort-text {
  font-size: 11px;
  color: #636E72;
}

.sort-icon {
  font-size: 12px;
}

/* 医生卡片 */
.doctors-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

/* 改为覆盖 Element Plus 卡片样式 */
.doctor-card {
  border-radius: 12px;
  border: none;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.06) !important;
  cursor: pointer;
  transition: all 0.3s ease;
}

.doctor-card:active {
  transform: scale(0.98);
}

.doctor-main {
  display: flex;
  gap: 12px;
  margin-bottom: 12px;
}

.doctor-avatar {
  position: relative;
  width: 60px;
  height: 60px;
  flex-shrink: 0;
}

.doctor-avatar img {
  width: 100%;
  height: 100%;
  border-radius: 50%;
  object-fit: cover;
}

.online-status {
  position: absolute;
  bottom: 2px;
  right: 2px;
  width: 14px;
  height: 14px;
  background: #FFFFFF;
  border-radius: 50%;
  padding: 2px;
}

.status-dot {
  display: block;
  width: 10px;
  height: 10px;
  border-radius: 50%;
}

.online-status.online .status-dot {
  background: #00B894;
  box-shadow: 0 0 4px rgba(0, 184, 148, 0.6);
}

.online-status.offline .status-dot {
  background: #B2BEC3;
}

.doctor-info {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.doctor-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.doctor-name {
  font-size: 16px;
  font-weight: 600;
  color: #2D3436;
}

.doctor-tags {
  display: flex;
  gap: 4px;
}

.tag {
  padding: 3px 6px;
  border-radius: 4px;
  font-size: 10px;
  font-weight: 500;
}

.tag-certified {
  background: linear-gradient(135deg, #00B894, #55EFC4);
  color: #FFFFFF;
}

.tag-specialty {
  background: #74B9FF;
  color: #FFFFFF;
}

.doctor-title {
  font-size: 12px;
  color: #636E72;
}

.doctor-stats {
  display: flex;
  align-items: center;
  gap: 8px;
}

.stat-item {
  display: flex;
  align-items: center;
  gap: 2px;
}

.stat-icon {
  font-size: 12px;
}

.stat-value {
  font-size: 12px;
  font-weight: 500;
  color: #2D3436;
}

.stat-divider {
  font-size: 10px;
  color: #B2BEC3;
}

.doctor-intro {
  font-size: 12px;
  color: #636E72;
  line-height: 1.6;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
  text-overflow: ellipsis;
}

.doctor-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-top: 12px;
  border-top: 0.5px solid #DFE6E9;
  margin-top: auto;
}

.consult-price {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.price-label {
  font-size: 11px;
  color: #B2BEC3;
}

.price-value {
  font-size: 18px;
  font-weight: 700;
  color: #FF7675;
}

.consult-btn {
  padding: 10px 20px;
  background: #B2BEC3;
  color: #FFFFFF;
  border-radius: 24px;
  font-size: 14px;
  font-weight: 500;
  border: none;
  transition: all 0.3s ease;
  cursor: pointer;
  outline: none;
}

.consult-btn.active {
  background: linear-gradient(135deg, #00B894, #55EFC4);
  box-shadow: 0 2px 6px rgba(0, 184, 148, 0.4);
}

/* 加载更多 */
.load-more {
  text-align: center;
  padding: 16px;
  font-size: 13px;
  color: #B2BEC3;
}

/* 空状态 */
.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12px;
  padding: 60px 0;
}

.empty-icon {
  font-size: 60px;
  opacity: 0.3;
}

.empty-text {
  font-size: 16px;
  color: #B2BEC3;
  font-weight: 500;
}

.empty-tip {
  font-size: 12px;
  color: #B2BEC3;
}
</style>