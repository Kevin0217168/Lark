<template>
  <div class="clinic-page">
    <!-- 顶部状态卡片 -->
    <div class="status-cards">
      <div class="status-card report-card" @click="gotoReports">
        <div class="card-info">
          <div class="card-title">电子报告 <el-badge :value="2" class="report-badge" /></div>
          <div class="card-desc">最新：{{ latestReport }}</div>
          <div class="advice-tag" v-if="hasAdvice">✨ 有专家新建议</div>
        </div>
        <el-icon class="card-icon"><Document /></el-icon>
      </div>

      <div class="status-card emergency-card" @click="handleEmergency">
        <div class="card-info">
          <div class="card-title">在线急诊</div>
          <div class="card-desc">7*24小时优先响应</div>
        </div>
        <el-icon class="card-icon"><Phone /></el-icon>
      </div>
    </div>

    <!-- 搜索与筛选 -->
    <div class="filter-section">
      <div class="search-bar">
        <el-input
          v-model="searchKeyword"
          placeholder="搜索医生、专长、疾病..."
          clearable
          @input="filterDoctors"
        >
          <template #prefix>
            <el-icon><Search /></el-icon>
          </template>
        </el-input>
      </div>

      <div class="filter-tabs">
        <div 
          class="filter-tab" 
          :class="{ active: selectedFilter === 'all' }"
          @click="selectFilter('all')"
        >全部</div>
        <div 
          class="filter-tab" 
          :class="{ active: selectedFilter === '鸟类' }"
          @click="selectFilter('鸟类')"
        >鸟类专场</div>
        <div 
          class="filter-tab" 
          :class="{ active: selectedFilter === '爬行' }"
          @click="selectFilter('爬行')"
        >爬行类</div>
        <div 
          class="filter-tab" 
          :class="{ active: selectedFilter === '两栖' }"
          @click="selectFilter('两栖')"
        >两栖类</div>
      </div>

      <div class="sort-bar">
        <span>当前共找到 {{ filteredDoctors.length }} 位医生</span>
        <div class="sort-action" @click="toggleSort">
          {{ sortType === 'rating' ? '评分最高' : '接诊最多' }}
          <el-icon><Sort /></el-icon>
        </div>
      </div>
    </div>

    <!-- 医生列表 -->
    <div class="doctor-list">
      <div v-for="doctor in filteredDoctors" :key="doctor.id" class="doctor-card" @click="viewDoctorDetail(doctor)">
        <div class="doctor-main">
          <div class="doctor-avatar">
            <img :src="doctor.avatar" alt="医生" />
            <div :class="['online-status', { online: doctor.isOnline }]"></div>
          </div>
          <div class="doctor-info">
            <div class="doctor-name-wrap">
              <span class="doctor-name">{{ doctor.name }}</span>
              <el-tag size="small" type="success" effect="plain">{{ doctor.specialty }}</el-tag>
            </div>
            <div class="doctor-title">{{ doctor.title }}</div>
            <div class="doctor-stats">
              <span class="stat rating">⭐ {{ doctor.rating }}</span>
              <span class="stat consult">接诊 {{ doctor.consultCount }}</span>
              <span class="stat rate">成功率 {{ doctor.successRate }}%</span>
            </div>
          </div>
          <div class="price-wrap">
            <span class="price">¥{{ doctor.price }}</span>
            <span class="price-unit">/次</span>
          </div>
        </div>
        <div class="doctor-intro">{{ doctor.intro }}</div>
        <div class="doctor-actions">
          <el-button 
            type="primary" 
            round 
            @click.stop="bookDoctor(doctor)" 
            :disabled="!doctor.isOnline"
            class="book-btn"
          >
            立即咨询
          </el-button>
        </div>
      </div>
      
      <div v-if="filteredDoctors.length === 0" class="empty-state">
        <el-empty description="未找到匹配的医生" />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import { Search, Sort, Document, Phone } from '@element-plus/icons-vue';
import { ElMessageBox, ElMessage } from 'element-plus';

const searchKeyword = ref('');
const selectedFilter = ref('all');
const sortType = ref('rating');
const latestReport = ref('2026/2/20 健康体检报告');
const hasAdvice = ref(true);

const doctors = [
  {
    id: 1,
    name: '张伟华',
    title: '首席兽医 / 异宠专科',
    specialty: '鸟类',
    avatar: 'https://img.js.design/assets/img/65a39626b8969f69430c51d2.jpg',
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
    avatar: 'https://img.js.design/assets/img/65a396248386401614777d0a.jpg',
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
    avatar: 'https://img.js.design/assets/img/65a3961f71587d4617e94917.jpg',
    rating: 4.7,
    consultCount: 6540,
    successRate: 95,
    price: 48,
    isOnline: false,
    intro: '两栖动物疾病专家，擅长蛙类、蝾螈的健康管理与疾病诊断，多次参与国际学术交流。'
  }
];

const filteredDoctors = ref<any[]>([]);

const filterDoctors = () => {
  let filtered = [...doctors];
  
  if (searchKeyword.value) {
    const keyword = searchKeyword.value.toLowerCase();
    filtered = filtered.filter(doctor => 
      doctor.name.toLowerCase().includes(keyword) ||
      doctor.specialty.toLowerCase().includes(keyword) ||
      doctor.title.toLowerCase().includes(keyword)
    );
  }
  
  if (selectedFilter.value !== 'all') {
    filtered = filtered.filter(doctor => 
      doctor.specialty === selectedFilter.value
    );
  }
  
  if (sortType.value === 'rating') {
    filtered.sort((a, b) => b.rating - a.rating);
  } else {
    filtered.sort((a, b) => b.consultCount - a.consultCount);
  }
  
  filteredDoctors.value = filtered;
};

const selectFilter = (filter: string) => {
  selectedFilter.value = filter;
  filterDoctors();
};

const toggleSort = () => {
  sortType.value = sortType.value === 'rating' ? 'consultCount' : 'rating';
  filterDoctors();
};

const handleEmergency = () => {
  ElMessageBox.confirm(
    '是否立即连接在线急诊通道？紧急情况将优先响应。',
    '在线急诊',
    {
      confirmButtonText: '立即连接',
      cancelButtonText: '取消',
      type: 'warning',
    }
  ).then(() => {
    ElMessage.success('正在为您连接急诊医生...');
  });
};

const gotoReports = () => {
  ElMessage.info('查看健康报告 (功能开发中)');
};

const viewDoctorDetail = (doctor: any) => {
  ElMessage.info(`查看 ${doctor.name} 的详情 (功能开发中)`);
};

const bookDoctor = (doctor: any) => {
  if (!doctor.isOnline) {
    ElMessage.warning('该医生当前离线');
    return;
  }
  
  ElMessageBox.confirm(
    `咨询费用: ¥${doctor.price}\n\n是否确认预约？`,
    `预约${doctor.name}医生`,
    {
      confirmButtonText: '确认支付',
      cancelButtonText: '取消',
      type: 'info',
    }
  ).then(() => {
    ElMessage.success('预约成功，正在进入对话...');
  });
};

onMounted(() => {
  filterDoctors();
});
</script>

<style scoped lang="scss">
.clinic-page {
  padding: 16px;
  background-color: #f8fafc;
  min-height: 100vh;
  padding-bottom: 100px;
}

.status-cards {
  display: flex;
  gap: 12px;
  margin-bottom: 24px;
  
  .status-card {
    flex: 1;
    background: #fff;
    border-radius: 12px;
    padding: 16px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    box-shadow: 0 4px 12px rgba(0,0,0,0.05);
    cursor: pointer;
    transition: transform 0.2s;
    
    &:active { transform: scale(0.98); }
    
    .card-title {
      font-size: 16px;
      font-weight: 600;
      color: #334155;
      display: flex;
      align-items: center;
      gap: 6px;
    }
    
    .card-desc {
      font-size: 12px;
      color: #94a3b8;
      margin-top: 4px;
    }
    
    .card-icon {
      font-size: 32px;
      opacity: 0.2;
    }
    
    &.report-card {
      border-left: 4px solid #8BAD42;
      .advice-tag {
        font-size: 10px;
        color: #8BAD42;
        background-color: #f0fdf4;
        padding: 2px 6px;
        border-radius: 10px;
        margin-top: 8px;
        display: inline-block;
      }
    }
    
    &.emergency-card {
      border-left: 4px solid #ef4444;
      .card-title { color: #ef4444; }
    }
  }
}

.filter-section {
  background: #fff;
  border-radius: 16px;
  padding: 16px;
  margin-bottom: 20px;
  box-shadow: 0 2px 8px rgba(0,0,0,0.03);
  
  .search-bar { margin-bottom: 16px; }
  
  .filter-tabs {
    display: flex;
    gap: 10px;
    overflow-x: auto;
    padding-bottom: 10px;
    margin-bottom: 12px;
    
    &::-webkit-scrollbar { display: none; }
    
    .filter-tab {
      padding: 6px 16px;
      border-radius: 20px;
      font-size: 13px;
      background-color: #f1f5f9;
      color: #64748b;
      white-space: nowrap;
      cursor: pointer;
      
      &.active {
        background-color: #8BAD42;
        color: #fff;
      }
    }
  }
  
  .sort-bar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 12px;
    color: #94a3b8;
    
    .sort-action {
      display: flex;
      align-items: center;
      gap: 4px;
      color: #334155;
      font-weight: 500;
      cursor: pointer;
    }
  }
}

.doctor-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.doctor-card {
  background: #fff;
  border-radius: 16px;
  padding: 16px;
  box-shadow: 0 4px 15px rgba(0,0,0,0.04);
  
  .doctor-main {
    display: flex;
    gap: 16px;
    align-items: flex-start;
  }
  
  .doctor-avatar {
    position: relative;
    width: 60px;
    height: 60px;
    flex-shrink: 0;
    
    img {
      width: 100%;
      height: 100%;
      border-radius: 12px;
      object-fit: cover;
    }
    
    .online-status {
      position: absolute;
      bottom: -2px;
      right: -2px;
      width: 12px;
      height: 12px;
      border-radius: 50%;
      background-color: #94a3b8;
      border: 2px solid #fff;
      
      &.online { background-color: #22c55e; }
    }
  }
  
  .doctor-info {
    flex: 1;
    
    .doctor-name-wrap {
      display: flex;
      align-items: center;
      gap: 8px;
      
      .doctor-name {
        font-size: 16px;
        font-weight: 700;
        color: #1e293b;
      }
    }
    
    .doctor-title {
      font-size: 12px;
      color: #64748b;
      margin-top: 4px;
    }
    
    .doctor-stats {
      margin-top: 8px;
      display: flex;
      gap: 12px;
      font-size: 11px;
      color: #94a3b8;
      
      .rating { color: #f59e0b; font-weight: 600; }
    }
  }
  
  .price-wrap {
    text-align: right;
    .price { font-size: 18px; font-weight: 700; color: #ef4444; }
    .price-unit { font-size: 11px; color: #94a3b8; }
  }
  
  .doctor-intro {
    margin-top: 12px;
    padding: 10px;
    background-color: #f8fafc;
    border-radius: 8px;
    font-size: 12px;
    color: #64748b;
    line-height: 1.5;
    display: -webkit-box;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
    overflow: hidden;
  }
  
  .doctor-actions {
    margin-top: 16px;
    display: flex;
    justify-content: flex-end;
    
    .book-btn {
      background-color: #8BAD42;
      border-color: #8BAD42;
      padding: 0 24px;
      
      &:hover { background-color: #9dc24d; }
    }
  }
}

.report-badge :deep(.el-badge__content) {
  background-color: #ef4444;
}
</style>