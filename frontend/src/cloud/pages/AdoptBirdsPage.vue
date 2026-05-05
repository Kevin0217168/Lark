<template>
  <div class="adopt-birds-page">
    <BackgroundDecoration />

    <div class="page-header">
      <h1 class="page-title">认领雏鸟</h1>
      <p class="page-subtitle">找到属于你的专属雏鸟</p>
    </div>
    
    <!-- 搜索筛选区域 -->
    <div class="search-container">
      <div class="search-card">
        <div class="search-row">
          <el-input 
            v-model="searchParams.name" 
            placeholder="输入雏鸟名称"
            class="modern-input"
            size="large"
            prefix-icon="Search"
          />
        </div>
        <div class="search-row">
          <el-select 
            v-model="searchParams.species" 
            placeholder="选择品种"
            class="modern-select"
            size="large"
            popper-class="modern-select-dropdown"
          >
            <el-option label="所有品种" value="" />
            <el-option label="鹦鹉" value="鹦鹉" />
            <el-option label="鸽子" value="鸽子" />
            <el-option label="文鸟" value="文鸟" />
            <el-option label="白脸角鸮" value="白脸角鸮" />
          </el-select>
          <el-select 
            v-model="searchParams.status" 
            placeholder="选择状态"
            class="modern-select"
            size="large"
            popper-class="modern-select-dropdown"
          >
            <el-option label="所有状态" value="" />
            <el-option label="可认领" value="available" />
            <el-option label="已认领" value="adopted" />
            <el-option label="已长成" value="grown" />
          </el-select>
        </div>
        <div class="search-row">
          <el-button 
            type="primary" 
            @click="searchBirds" 
            class="modern-button"
            size="large"
            :loading="loading"
          >
            <el-icon><search /></el-icon>
            开始搜索
          </el-button>
        </div>
      </div>
    </div>
    
    <!-- 雏鸟列表 -->
    <div class="birds-grid" v-if="birds.length > 0">
      <div 
        v-for="(bird, index) in birds" 
        :key="bird.id" 
        class="bird-card"
        :style="{ animationDelay: `${index * 0.1}s` }"
      >
        <div class="card-header">
          <div class="status-indicator" :class="`status-${bird.status}`">
            {{ getStatusLabel(bird.status) }}
          </div>
        </div>
        <div class="card-image">
          <img 
            :src="bird.avatar_url || 'https://trae-api-cn.mchost.guru/api/ide/v1/text_to_image?prompt=cute%20baby%20bird%20avatar%20cartoon%20style&image_size=square'" 
            :alt="bird.name"
            class="bird-avatar"
          />
        </div>
        <div class="card-body">
          <h3 class="bird-name">{{ bird.name }}</h3>
          <div class="bird-meta">
            <span class="meta-item">
              <el-icon class="meta-icon"><InfoFilled /></el-icon>
              {{ bird.species }}
            </span>
            <span class="meta-item">
              <el-icon class="meta-icon"><Calendar /></el-icon>
              {{ bird.birth_date }}
            </span>
            <span class="meta-item" v-if="bird.area">
              <el-icon class="meta-icon"><Monitor /></el-icon>
              {{ bird.area }} #{{ bird.number }}
            </span>
          </div>
          <p class="bird-description" v-if="bird.description">
            {{ bird.description }}
          </p>
        </div>
        <div class="card-footer">
          <el-button 
            type="primary" 
            @click="adoptBird(bird)"
            :disabled="bird.status !== 'available'"
            class="adopt-button"
          >
            {{ bird.status === 'available' ? '立即认领' : '已认领' }}
          </el-button>
        </div>
      </div>
    </div>
    
    <!-- 空状态 -->
    <div class="empty-container" v-else-if="!loading">
      <div class="empty-card">
        <div class="empty-icon">🐣</div>
        <h3 class="empty-title">暂无雏鸟</h3>
        <p class="empty-text">没有找到符合条件的雏鸟，请尝试调整搜索条件</p>
        <el-button @click="resetSearch" class="reset-button">
          重置搜索
        </el-button>
      </div>
    </div>
    
    <!-- 加载状态 -->
    <div class="loading-container" v-else>
      <div class="loading-grid">
        <div v-for="i in 6" :key="i" class="loading-card">
          <el-skeleton :rows="6" animated />
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import { useRouter } from 'vue-router';
import { api } from '@/utils/api';
import { ElMessage, ElSkeleton, ElButton, ElInput, ElSelect, ElOption } from 'element-plus';
import { Search, InfoFilled, Calendar, Monitor } from '@element-plus/icons-vue';
import BackgroundDecoration from '../components/BackgroundDecoration.vue';

const router = useRouter();

// 搜索参数
const searchParams = ref({
  name: '',
  species: '',
  status: ''
});

// 雏鸟列表
const birds = ref<any[]>([]);
const loading = ref(false);

// 状态标签
const getStatusLabel = (status: string) => {
  switch (status) {
    case 'available': return '可认领';
    case 'adopted': return '已认领';
    case 'grown': return '已长成';
    default: return status;
  }
};

// 搜索雏鸟
const searchBirds = async () => {
  loading.value = true;
  try {
    // 构建查询参数，只包含非空值
    const params: any = {};
    
    if (searchParams.value.name.trim()) {
      params.name = searchParams.value.name.trim();
    }
    
    if (searchParams.value.species) {
      params.species = searchParams.value.species;
    }
    
    if (searchParams.value.status) {
      params.status = searchParams.value.status;
    }

    // 调用后端 API
    const response = await api.get('/api/birds', params);

    if (response.code === 200) {
      birds.value = response.data || [];
    } else {
      ElMessage.error('获取雏鸟列表失败');
    }
  } catch (error) {
    ElMessage.error('网络请求失败');
  } finally {
    loading.value = false;
  }
};

// 重置搜索
const resetSearch = () => {
  searchParams.value = {
    name: '',
    species: '',
    status: ''
  };
  searchBirds();
};

// 认领雏鸟
const adoptBird = async (bird: any) => {
  try {
    // 调用后端 API 进行认领操作
    const data = await api.post(`/api/birds/${bird.id}/adopt`);

    if (data.code === 200 && data.data) {
      ElMessage.success(data.data.message || `已成功认领雏鸟 ${bird.name}`);
      // 跳转到我的雏鸟界面
      setTimeout(() => {
        router.push('/cloud/birds');
      }, 1000);
    } else {
      ElMessage.error(data.msg || '认领失败，请重试');
    }
  } catch (error) {
    ElMessage.error('网络错误，请检查网络连接');
  }
};

// 初始加载
onMounted(() => {
  searchBirds();
});
</script>

<style scoped>
/* 全局样式 */
.adopt-birds-page {
  min-height: 100vh;
  background: linear-gradient(135deg, #f0fdf4 0%, #dcfce7 50%, #bbf7d0 100%);
  padding: 20px 16px 100px;
  position: relative;
  overflow: hidden;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
}

/* 背景装饰 */
/* 页面头部 */
.page-header {
  text-align: center;
  margin-bottom: 32px;
  position: relative;
  z-index: 2;
  animation: fadeInUp 0.8s ease-out forwards;
}

@keyframes fadeInUp {
  to {
    opacity: 1;
    transform: translateY(0);
  }
  from {
    opacity: 0;
    transform: translateY(30px);
  }
}

.page-title {
  font-size: 32px;
  font-weight: 800;
  color: #166534;
  margin: 0 0 8px 0;
  background: linear-gradient(135deg, #16a34a, #15803d);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  letter-spacing: 1px;
}

.page-subtitle {
  font-size: 16px;
  color: #16a34a;
  margin: 0;
  font-weight: 500;
  opacity: 0.8;
}

/* 搜索容器 */
.search-container {
  margin-bottom: 32px;
  position: relative;
  z-index: 2;
  animation: slideIn 0.6s ease-out 0.2s both;
}

@keyframes slideIn {
  from {
    opacity: 0;
    transform: translateY(20px) scale(0.95);
  }
  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}

.search-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(20px);
  border-radius: 20px;
  padding: 24px;
  box-shadow: 
    0 12px 40px rgba(0, 0, 0, 0.08),
    0 4px 16px rgba(0, 0, 0, 0.05),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
  border: 1px solid rgba(209, 213, 219, 0.3);
  transition: all 0.3s ease;
}

.search-card:hover {
  box-shadow: 
    0 16px 48px rgba(0, 0, 0, 0.12),
    0 6px 24px rgba(0, 0, 0, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
  transform: translateY(-2px);
}

.search-row {
  display: flex;
  gap: 12px;
  margin-bottom: 16px;
  align-items: center;
}

.search-row:last-child {
  margin-bottom: 0;
}

.modern-input {
  flex: 1;
  border-radius: 16px !important;
  border: 2px solid rgba(74, 222, 128, 0.2) !important;
  transition: all 0.3s ease !important;
  background: rgba(255, 255, 255, 0.9) !important;
  height: 56px !important;
}

.modern-input:focus {
  border-color: #16a34a !important;
  box-shadow: 0 0 0 4px rgba(22, 163, 74, 0.1) !important;
  background: rgba(255, 255, 255, 0.95) !important;
}

.modern-input .el-input__wrapper {
  border-radius: 16px !important;
  box-shadow: none !important;
  overflow: visible;
}

.modern-input .el-input__inner {
  white-space: nowrap;
  overflow: visible;
  text-overflow: clip;
  color: #166534 !important;
}

.modern-input .el-input__placeholder {
  white-space: nowrap;
  overflow: visible;
  text-overflow: clip;
  color: rgba(22, 163, 74, 0.5) !important;
}

.modern-select {
  flex: 1;
  border-radius: 16px !important;
  border: 2px solid rgba(74, 222, 128, 0.2) !important;
  transition: all 0.3s ease !important;
  background: rgba(255, 255, 255, 0.9) !important;
  height: 56px !important;
}

.modern-select:focus {
  border-color: #16a34a !important;
  box-shadow: 0 0 0 4px rgba(22, 163, 74, 0.1) !important;
  background: rgba(255, 255, 255, 0.95) !important;
}

.modern-select .el-select__wrapper {
  border-radius: 16px !important;
  box-shadow: none !important;
}

.modern-select .el-select__wrapper .el-input__wrapper {
  border-radius: 16px !important;
  box-shadow: none !important;
  overflow: visible;
}

.modern-select .el-select__placeholder {
  white-space: nowrap;
  overflow: visible;
  text-overflow: clip;
  color: rgba(22, 163, 74, 0.5) !important;
}

.modern-select .el-select__selected-item {
  white-space: nowrap;
  overflow: visible;
  text-overflow: clip;
  color: #166534 !important;
}

.modern-button {
  border-radius: 16px !important;
  padding: 16px 32px !important;
  font-weight: 700 !important;
  font-size: 16px !important;
  background: linear-gradient(135deg, #16a34a 0%, #15803d 100%) !important;
  border: none !important;
  transition: all 0.3s ease !important;
  box-shadow: 0 4px 16px rgba(22, 163, 74, 0.3) !important;
  min-width: 120px;
  position: relative;
  overflow: hidden;
}

.modern-button::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.modern-button:hover::before {
  left: 100%;
}

.modern-button:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(22, 163, 74, 0.4) !important;
}

.modern-button:active:not(:disabled) {
  transform: translateY(0);
  box-shadow: 0 2px 8px rgba(22, 163, 74, 0.3) !important;
}

/* 雏鸟网格 */
.birds-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 20px;
  position: relative;
  z-index: 2;
}

/* 雏鸟卡片 */
.bird-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(16px);
  border-radius: 24px;
  overflow: hidden;
  box-shadow: 
    0 12px 40px rgba(0, 0, 0, 0.08),
    0 4px 16px rgba(0, 0, 0, 0.05),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
  border: 1px solid rgba(209, 213, 219, 0.3);
  transition: all 0.3s ease;
  display: flex;
  flex-direction: column;
  animation: slideInUp 0.6s ease-out both;
}

@keyframes slideInUp {
  from {
    opacity: 0;
    transform: translateY(30px) scale(0.95);
  }
  to {
    opacity: 1;
    transform: translateY(0) scale(1);
  }
}

.bird-card:hover {
  transform: translateY(-4px);
  box-shadow: 
    0 16px 48px rgba(0, 0, 0, 0.12),
    0 6px 24px rgba(0, 0, 0, 0.08),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
}

.card-header {
  padding: 16px 20px 0;
  display: flex;
  justify-content: flex-end;
}

.status-indicator {
  padding: 6px 12px;
  border-radius: 16px;
  font-size: 12px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.status-available {
  background: rgba(16, 185, 129, 0.1);
  color: #10b981;
}

.status-adopted {
  background: rgba(22, 163, 74, 0.1);
  color: #16a34a;
}

.status-grown {
  background: rgba(245, 158, 11, 0.1);
  color: #f59e0b;
}

.card-image {
  padding: 20px;
  display: flex;
  justify-content: center;
  align-items: center;
  background: linear-gradient(135deg, #f0fdf4 0%, #dcfce7 100%);
}

.bird-avatar {
  width: 120px;
  height: 120px;
  border-radius: 50%;
  object-fit: cover;
  box-shadow: 0 4px 16px rgba(74, 222, 128, 0.2);
  transition: transform 0.3s ease;
  animation: pulse 2s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% {
    box-shadow: 0 4px 16px rgba(74, 222, 128, 0.2);
  }
  50% {
    box-shadow: 0 6px 20px rgba(74, 222, 128, 0.3);
  }
}

.bird-card:hover .bird-avatar {
  transform: scale(1.05);
}

.card-body {
  padding: 20px;
  flex: 1;
}

.bird-name {
  font-size: 20px;
  font-weight: 700;
  color: #166534;
  margin: 0 0 12px 0;
}

.bird-meta {
  display: flex;
  flex-direction: column;
  gap: 8px;
  margin-bottom: 16px;
}

.meta-item {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 14px;
  color: #6b7280;
  opacity: 0.9;
}

.meta-icon {
  font-size: 16px;
  color: #9ca3af;
  opacity: 0.8;
}

.bird-description {
  font-size: 14px;
  color: #6b7280;
  opacity: 0.8;
  line-height: 1.5;
  margin: 0 0 20px 0;
  display: -webkit-box;
  -webkit-line-clamp: 3;
  -webkit-box-orient: vertical;
  overflow: hidden;
}

.card-footer {
  padding: 0 20px 20px;
  display: flex;
  gap: 12px;
}

.adopt-button {
  flex: 2;
  border-radius: 12px !important;
  padding: 12px !important;
  font-weight: 600 !important;
  background: linear-gradient(135deg, #16a34a 0%, #15803d 100%) !important;
  border: none !important;
  transition: all 0.3s ease !important;
  box-shadow: 0 4px 16px rgba(22, 163, 74, 0.3) !important;
  position: relative;
  overflow: hidden;
}

.adopt-button::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.adopt-button:hover::before {
  left: 100%;
}

.adopt-button:hover:not(:disabled) {
  transform: translateY(-1px);
  box-shadow: 0 6px 20px rgba(22, 163, 74, 0.4) !important;
}

.detail-button {
  flex: 1;
  border-radius: 12px !important;
  padding: 12px !important;
  font-weight: 600 !important;
  background: rgba(243, 244, 246, 0.8) !important;
  color: #6b7280 !important;
  border: 1px solid rgba(209, 213, 219, 0.5) !important;
  transition: all 0.3s ease !important;
}

.detail-button:hover {
  background: rgba(229, 231, 235, 0.9) !important;
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.08) !important;
  color: #4b5563 !important;
}

/* 空状态 */
.empty-container {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 400px;
  position: relative;
  z-index: 2;
  animation: fadeIn 1s ease-out 0.4s both;
}

@keyframes fadeIn {
  from {
    opacity: 0;
  }
  to {
    opacity: 1;
  }
}

.empty-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(20px);
  border-radius: 24px;
  padding: 48px 32px;
  text-align: center;
  box-shadow: 
    0 12px 40px rgba(0, 0, 0, 0.08),
    0 4px 16px rgba(0, 0, 0, 0.05),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
  border: 1px solid rgba(209, 213, 219, 0.3);
  max-width: 400px;
  width: 100%;
}

.empty-icon {
  font-size: 80px;
  margin-bottom: 24px;
  animation: bounce 2s infinite;
}

@keyframes bounce {
  0%, 20%, 50%, 80%, 100% {
    transform: translateY(0);
  }
  40% {
    transform: translateY(-20px);
  }
  60% {
    transform: translateY(-10px);
  }
}

.empty-title {
  font-size: 24px;
  font-weight: 700;
  color: #166534;
  margin: 0 0 12px 0;
}

.empty-text {
  font-size: 16px;
  color: #6b7280;
  opacity: 0.9;
  margin: 0 0 24px 0;
  line-height: 1.5;
}

.reset-button {
  border-radius: 12px !important;
  padding: 12px 24px !important;
  font-weight: 600 !important;
  background: linear-gradient(135deg, #16a34a 0%, #15803d 100%) !important;
  border: none !important;
  transition: all 0.3s ease !important;
  box-shadow: 0 4px 16px rgba(22, 163, 74, 0.3) !important;
  position: relative;
  overflow: hidden;
}

.reset-button::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.reset-button:hover::before {
  left: 100%;
}

.reset-button:hover {
  transform: translateY(-1px);
  box-shadow: 0 6px 20px rgba(22, 163, 74, 0.4) !important;
}

/* 加载状态 */
.loading-container {
  position: relative;
  z-index: 2;
}

.loading-grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(300px, 1fr));
  gap: 20px;
}

.loading-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(16px);
  border-radius: 24px;
  padding: 20px;
  box-shadow: 
    0 12px 40px rgba(0, 0, 0, 0.08),
    0 4px 16px rgba(0, 0, 0, 0.05),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
  border: 1px solid rgba(209, 213, 219, 0.3);
  animation: fadeIn 0.6s ease-out both;
}

/* 响应式设计 */
@media (max-width: 768px) {
  .adopt-birds-page {
    padding: 16px 12px 90px;
  }
  
  .page-title {
    font-size: 24px;
  }
  
  .search-card {
    padding: 16px;
  }
  
  .search-row {
    flex-direction: column;
    align-items: stretch;
  }
  
  .birds-grid {
    grid-template-columns: 1fr;
    gap: 16px;
  }
  
  .bird-card {
    border-radius: 20px;
  }
  
  .card-body {
    padding: 16px;
  }
  
  .card-footer {
    padding: 0 16px 16px;
  }
  
  .empty-card {
    padding: 32px 24px;
    margin: 0 16px;
  }
  
  .loading-grid {
    grid-template-columns: 1fr;
    gap: 16px;
  }
}

/* ==================== 桌面端完全重写 ==================== */
@media (min-width: 769px) {
  .adopt-birds-page {
    padding: 40px 48px;
    max-width: 1400px;
    margin: 0 auto;
    background: linear-gradient(135deg, #f8fafc 0%, #f0fdf4 50%, #ecfdf5 100%);
    min-height: 100vh;
  }

  .page-header {
    margin-bottom: 32px;
  }

  .page-title {
    font-size: 28px;
    font-weight: 800;
    color: #065f46;
  }

  .page-subtitle {
    font-size: 14px;
    color: #64748b;
  }

  .search-container {
    margin-bottom: 32px;
  }

  .search-card {
    background: rgba(255, 255, 255, 0.85);
    backdrop-filter: blur(12px);
    border-radius: 16px;
    padding: 20px 24px;
    border: 1px solid rgba(209, 213, 219, 0.2);
  }

  .search-row {
    display: flex;
    flex-direction: row;
    gap: 12px;
    align-items: center;
    margin-bottom: 0;
    flex-wrap: nowrap;
  }

  .search-row + .search-row {
    margin-top: 12px;
  }

  .search-input-wrapper {
    flex: 3;
  }

  .filter-select {
    flex: 1;
    min-width: 160px;
  }

  .modern-input :deep(.el-input__inner) {
    border-radius: 10px;
    height: 44px;
    font-size: 14px;
  }

  .modern-select :deep(.el-input__inner) {
    border-radius: 10px;
    height: 44px;
    font-size: 14px;
  }

  .search-btn {
    flex-shrink: 0;
    padding: 0 28px;
    height: 44px;
    font-size: 15px;
    border-radius: 10px;
  }

  .birds-grid {
    grid-template-columns: repeat(4, 1fr);
    gap: 20px;
  }

  .bird-card {
    background: rgba(255, 255, 255, 0.9);
    backdrop-filter: blur(12px);
    border-radius: 16px;
    padding: 24px;
    border: 1px solid rgba(209, 213, 219, 0.2);
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    cursor: pointer;
  }

  .bird-card:hover {
    transform: translateY(-4px);
    box-shadow: 0 8px 25px rgba(0, 0, 0, 0.08);
    border-color: rgba(16, 185, 129, 0.3);
  }

  .bird-avatar {
    width: 100px;
    height: 100px;
    font-size: 48px;
    margin-bottom: 16px;
  }

  .bird-name {
    font-size: 18px;
    font-weight: 700;
    color: #111827;
    margin-bottom: 4px;
  }

  .bird-species {
    font-size: 13px;
    color: #6b7280;
    margin-bottom: 8px;
  }

  .bird-cage {
    font-size: 12px;
    color: #9ca3af;
  }

  .pagination-container {
    display: flex;
    justify-content: center;
    margin-top: 32px;
  }
}

@media (min-width: 769px) and (max-width: 1199px) {
  .adopt-birds-page {
    padding: 32px 24px;
  }

  .birds-grid {
    grid-template-columns: repeat(3, 1fr);
    gap: 16px;
  }

  .search-row {
    flex-wrap: wrap;
  }
}

/* 滚动条样式 */
::-webkit-scrollbar {
  width: 8px;
}

::-webkit-scrollbar-track {
  background: rgba(255, 255, 255, 0.1);
  border-radius: 4px;
}

::-webkit-scrollbar-thumb {
  background: rgba(74, 222, 128, 0.3);
  border-radius: 4px;
}

::-webkit-scrollbar-thumb:hover {
  background: rgba(74, 222, 128, 0.5);
}
</style>

<style>
/* 现代化下拉选项样式 */
.modern-select-dropdown {
  padding: 8px;
  border-radius: 16px;
  background: rgba(255, 255, 255, 0.95) !important;
  backdrop-filter: blur(20px);
  box-shadow: 
    0 12px 40px rgba(74, 222, 128, 0.15),
    0 4px 16px rgba(74, 222, 128, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 0.9) !important;
  border: 1px solid rgba(255, 255, 255, 0.6);
  overflow: hidden;
  min-width: 200px !important;
  width: max-content !important;
  max-width: 90vw;
}

.modern-select-dropdown .el-select-dropdown__list {
  padding: 0;
}

.modern-select-dropdown .el-select-dropdown__item {
  border-radius: 12px;
  margin: 4px 8px;
  padding: 12px 16px;
  font-size: 15px;
  font-weight: 500;
  color: #166534;
  transition: all 0.2s ease;
  position: relative;
  white-space: nowrap;
  overflow: visible;
  text-overflow: clip;
  width: auto;
  max-width: 100%;
  line-height: 1.5;
}

.modern-select-dropdown .el-select-dropdown__item::before {
  content: '';
  position: absolute;
  left: 0;
  top: 0;
  bottom: 0;
  width: 3px;
  background: linear-gradient(135deg, #16a34a, #15803d);
  opacity: 0;
  transition: opacity 0.2s ease;
}

.modern-select-dropdown .el-select-dropdown__item:hover {
  background: linear-gradient(135deg, rgba(22, 163, 74, 0.08), rgba(21, 128, 61, 0.08));
  color: #16a34a;
  transform: translateX(2px);
}

.modern-select-dropdown .el-select-dropdown__item:hover::before {
  opacity: 1;
}

.modern-select-dropdown .el-select-dropdown__item.is-selected {
  background: linear-gradient(135deg, rgba(22, 163, 74, 0.12), rgba(21, 128, 61, 0.12));
  color: #16a34a;
  font-weight: 600;
}

.modern-select-dropdown .el-select-dropdown__item.is-selected::before {
  opacity: 1;
}

.modern-select-dropdown .el-select-dropdown__empty {
  padding: 24px;
  color: #16a34a;
  opacity: 0.6;
  font-size: 14px;
}

/* 下拉选项滚动条 */
.modern-select-dropdown .el-scrollbar__bar.is-vertical {
  right: 4px;
}

.modern-select-dropdown .el-scrollbar__thumb {
  background: rgba(74, 222, 128, 0.3);
  border-radius: 4px;
}

.modern-select-dropdown .el-scrollbar__thumb:hover {
  background: rgba(74, 222, 128, 0.5);
}

/* 下拉框的三角形指示器 */
.el-popper.is-modern-select-dropdown[data-popper-placement^="bottom"] .el-popper__arrow {
  display: none;
}

.el-popper.is-modern-select-dropdown[data-popper-placement^="top"] .el-popper__arrow {
  display: none;
}
</style>