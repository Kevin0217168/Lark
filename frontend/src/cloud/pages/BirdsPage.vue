<template>
  <div class="birds-page">
    <!-- 背景装饰 -->
    <div class="background-decorations">
      <div class="decoration decoration-1"></div>
      <div class="decoration decoration-2"></div>
      <div class="decoration decoration-3"></div>
    </div>
    
    <div class="page-content">
      <h1 class="page-title" :class="{ 'fade-in': isLoaded }">我的雏鸟</h1>
      
      <!-- 未认领状态 -->
      <div v-if="!hasAdoptedBird && !loading">
        <!-- 提示信息 -->
        <div class="empty-section" :class="{ 'fade-in': isLoaded }">
          <div class="empty-card">
            <div class="empty-icon">🐦</div>
            <h3 class="empty-title">还没有认领雏鸟</h3>
            <p class="empty-text">点击下方按钮去认领一只可爱的雏鸟吧！</p>
          </div>
        </div>
        
        <!-- 操作按钮 -->
        <div class="action-section" :class="{ 'fade-in': isLoaded }">
          <button class="adopt-btn" @click="goToAdoptBirds">
            <span class="btn-icon">🏷️</span>
            <span class="btn-text">去认领雏鸟</span>
          </button>
        </div>
      </div>
      
      <!-- 已认领状态 - 全屏沉浸式设计 -->
      <div v-else-if="hasAdoptedBird && !loading" class="adopted-bird-section" :class="{ 'fade-in': isLoaded }">
        <!-- 雏鸟头部信息 -->
        <div class="bird-header">
          <h2 class="bird-name">{{ adoptedBird?.bird_name }}</h2>
          <div class="status-badge">已认领</div>
        </div>
        
        <!-- 雏鸟形象展示区 -->
        <div class="bird-hero-section">
          <div class="bird-image-container">
            <div class="image-placeholder">
              <div class="placeholder-icon">🐣</div>
              <p class="placeholder-text">雏鸟画面显示区域</p>
            </div>
          </div>
          
          <!-- 关键状态信息 -->
          <div class="bird-status-info">
            <div class="status-item">
              <div class="status-value">{{ calculateAge(adoptedBird?.birth_date) }}</div>
              <div class="status-label">日龄</div>
            </div>
            <div class="status-divider"></div>
            <div class="status-item">
              <div class="status-value">{{ adoptedBird?.bird_species }}</div>
              <div class="status-label">品种</div>
            </div>
          </div>
        </div>
        
        <!-- 详细信息区域 -->
        <div class="bird-details-section">
          <h3 class="section-title">基本信息</h3>
          
          <div class="detail-item">
            <span class="detail-label">出生日期：</span>
            <span class="detail-value">{{ adoptedBird?.birth_date || '未知' }}</span>
          </div>
          
          <div class="detail-item description">
            <span class="detail-label">描述：</span>
            <p class="detail-text">{{ adoptedBird?.description || '暂无描述' }}</p>
          </div>
        </div>
        
        <!-- 操作区域 -->
        <div class="action-section" :class="{ 'fade-in': isLoaded }">
          <button class="release-btn" @click="releaseBird">
            <span class="btn-icon">🔓</span>
            <span class="btn-text">释放雏鸟</span>
          </button>
        </div>
      </div>
      
      <!-- 加载状态 -->
      <div v-else-if="loading" class="loading-section">
        <div class="loading-spinner"></div>
        <p class="loading-text">检查中...</p>
      </div>
      
      <!-- 未认领且无雏鸟状态 -->
      <div v-else class="empty-section" :class="{ 'fade-in': isLoaded }">
        <div class="empty-card">
          <div class="empty-icon">🐦</div>
          <p class="empty-text">暂无雏鸟信息</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, computed } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import { api } from '@/utils/api';

const router = useRouter();

// 状态管理
const adoptedBird = ref<any>(null);
const loading = ref(true);
const isLoaded = ref(false);

// 计算属性：是否已认领雏鸟
const hasAdoptedBird = computed(() => {
  return adoptedBird.value !== null;
});

// 跳转到认领页面
const goToAdoptBirds = () => {
  router.push('/cloud/adopt-birds');
};

// 计算日龄
const calculateAge = (birthDate: string) => {
  if (!birthDate) return 0;
  
  const birth = new Date(birthDate);
  const today = new Date();
  const diffTime = Math.abs(today.getTime() - birth.getTime());
  const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24));
  
  return diffDays;
};

// 释放雏鸟
const releaseBird = async () => {
  try {
    const response = await api.delete('/api/birds/adopted/me');
    
    if (response.code === 200) {
      ElMessage.success('释放成功！');
      adoptedBird.value = null;
    } else {
      ElMessage.error(response.msg || '释放失败，请重试');
    }
  } catch (error: any) {
    // 处理后端返回的错误信息
    if (error.data && error.data.msg) {
      ElMessage.error(error.data.msg);
    } else if (error.message) {
      ElMessage.error(error.message);
    } else {
      ElMessage.error('网络错误，请检查网络连接');
    }
  }
};

// 检查用户是否已认领雏鸟
const checkAdoptedBird = async () => {
  try {
    const response = await api.get('/api/birds/adopted/me');

    if (response.code === 200 && response.data?.adopted_bird && Object.keys(response.data.adopted_bird).length > 0) {
      adoptedBird.value = response.data.adopted_bird;
    } else {
      adoptedBird.value = null;
    }
  } catch (error) {
    adoptedBird.value = null;
  } finally {
    loading.value = false;
    // 延迟显示动画
    setTimeout(() => {
      isLoaded.value = true;
    }, 100);
  }
};

// 页面加载时检查
onMounted(() => {
  checkAdoptedBird();
});
</script>

<style scoped>
.birds-page {
  min-height: 100vh;
  background: linear-gradient(135deg, #E6F4EA 0%, #D4EBD0 50%, #B8DFC7 100%);
  padding-bottom: calc(80px + env(safe-area-inset-bottom, 0px));
  position: relative;
  overflow: hidden;
}

/* 背景装饰 */
.background-decorations {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 1;
}

.decoration {
  position: absolute;
  border-radius: 50%;
  background: rgba(139, 173, 66, 0.1);
  filter: blur(60px);
  animation: float 8s ease-in-out infinite;
}

.decoration-1 {
  top: 10%;
  left: 10%;
  width: 200px;
  height: 200px;
  animation-delay: 0s;
}

.decoration-2 {
  top: 60%;
  right: 10%;
  width: 150px;
  height: 150px;
  animation-delay: 2s;
}

.decoration-3 {
  bottom: 20%;
  left: 20%;
  width: 180px;
  height: 180px;
  animation-delay: 4s;
}

/* 浮动动画 */
@keyframes float {
  0%, 100% {
    transform: translateY(0px) translateX(0px);
  }
  25% {
    transform: translateY(-20px) translateX(10px);
  }
  50% {
    transform: translateY(10px) translateX(-10px);
  }
  75% {
    transform: translateY(-15px) translateX(5px);
  }
}

.page-content {
  padding: 20px 16px;
  position: relative;
  z-index: 2;
}

.page-title {
  font-size: 24px;
  font-weight: 700;
  color: #166534;
  margin-bottom: 20px;
  text-align: center;
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out;
}

/* 渐入动画 */
.fade-in {
  opacity: 1 !important;
  transform: translateY(0) !important;
}

/* 操作区域 */
.action-section {
  margin-bottom: 20px;
  text-align: center;
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.2s;
  display: flex;
  justify-content: center;
  padding: 0 16px 20px;
}

.release-btn {
  background: linear-gradient(135deg, #f87171 0%, #ef4444 100%);
  color: white;
  border: none;
  border-radius: 16px;
  padding: 16px 36px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  gap: 8px;
  transition: all 0.3s ease;
  box-shadow: 0 4px 16px rgba(239, 68, 68, 0.3);
  min-width: 220px;
  justify-content: center;
  position: relative;
  overflow: hidden;
}

.release-btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.release-btn:hover::before {
  left: 100%;
}

.release-btn:hover {
  transform: translateY(-3px);
  box-shadow: 0 6px 20px rgba(239, 68, 68, 0.4);
  background: linear-gradient(135deg, #ef4444 0%, #dc2626 100%);
}

.release-btn:active {
  transform: translateY(0);
  box-shadow: 0 2px 8px rgba(239, 68, 68, 0.3);
}

.adopt-btn {
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
  color: white;
  border: none;
  border-radius: 16px;
  padding: 14px 28px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  gap: 8px;
  box-shadow: 0 4px 16px rgba(139, 173, 66, 0.3);
  transition: all 0.3s ease;
  position: relative;
  overflow: hidden;
}

.adopt-btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.adopt-btn:hover::before {
  left: 100%;
}

.adopt-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(139, 173, 66, 0.4);
}

.adopt-btn:active {
  transform: translateY(0);
  box-shadow: 0 2px 8px rgba(139, 173, 66, 0.3);
}

.btn-icon {
  font-size: 18px;
}

.btn-text {
  font-size: 16px;
}

/* 已认领状态 - 全屏沉浸式设计 */
.adopted-bird-section {
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.2s;
  display: flex;
  flex-direction: column;
  gap: 24px;
}

/* 雏鸟头部信息 */
.bird-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 16px;
}

.bird-name {
  font-size: 28px;
  font-weight: 700;
  color: #166534;
  margin: 0;
  text-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
}

.status-badge {
  background: rgba(16, 185, 129, 0.1);
  color: #10b981;
  padding: 8px 16px;
  border-radius: 20px;
  font-size: 14px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  box-shadow: 0 2px 8px rgba(16, 185, 129, 0.2);
}

/* 雏鸟形象展示区 */
.bird-hero-section {
  display: flex;
  flex-direction: column;
  gap: 24px;
  padding: 0 16px;
}

.bird-image-container {
  border-radius: 20px;
  overflow: hidden;
  background: linear-gradient(135deg, #f0fdf4 0%, #dcfce7 100%);
  border: 2px dashed rgba(139, 173, 66, 0.3);
  min-height: 200px;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 8px 32px rgba(139, 173, 66, 0.1);
  transition: all 0.3s ease;
}

.bird-image-container:hover {
  box-shadow: 0 12px 40px rgba(139, 173, 66, 0.15);
  transform: translateY(-2px);
}

.image-placeholder {
  padding: 60px 40px;
  text-align: center;
}

.image-placeholder .placeholder-icon {
  font-size: 80px;
  margin-bottom: 16px;
  animation: pulse 2s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% {
    transform: scale(1);
  }
  50% {
    transform: scale(1.1);
  }
}

.image-placeholder .placeholder-text {
  font-size: 16px;
  color: #6b7280;
  font-weight: 500;
  opacity: 0.8;
}

/* 关键状态信息 */
.bird-status-info {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 32px;
  background: rgba(255, 255, 255, 0.7);
  backdrop-filter: blur(16px);
  border-radius: 20px;
  padding: 20px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.5);
}

.status-item {
  text-align: center;
  flex: 1;
}

.status-value {
  font-size: 32px;
  font-weight: 700;
  color: #166534;
  line-height: 1;
  margin-bottom: 4px;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
}

.status-label {
  font-size: 14px;
  color: #6b7280;
  font-weight: 500;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.status-divider {
  width: 1px;
  height: 40px;
  background: rgba(139, 173, 66, 0.2);
}

/* 详细信息区域 */
.bird-details-section {
  background: rgba(255, 255, 255, 0.7);
  backdrop-filter: blur(16px);
  border-radius: 20px;
  padding: 24px;
  box-shadow: 0 4px 16px rgba(0, 0, 0, 0.05);
  border: 1px solid rgba(255, 255, 255, 0.5);
  margin: 0 16px;
}

.section-title {
  font-size: 18px;
  font-weight: 600;
  color: #166534;
  margin: 0 0 16px 0;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
}

.detail-item {
  display: flex;
  align-items: flex-start;
  gap: 12px;
  margin-bottom: 16px;
}

.detail-item:last-child {
  margin-bottom: 0;
}

.detail-item.description {
  flex-direction: column;
  align-items: flex-start;
  gap: 8px;
}

.detail-label {
  font-size: 14px;
  font-weight: 600;
  color: #4b5563;
  min-width: 90px;
  flex-shrink: 0;
}

.detail-value {
  font-size: 16px;
  color: #6b7280;
  flex: 1;
  line-height: 1.4;
}

.detail-text {
  font-size: 16px;
  color: #6b7280;
  line-height: 1.5;
  margin: 0;
  flex: 1;
}



/* 加载状态 */
.loading-section {
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  min-height: 60vh;
  gap: 16px;
}

.loading-spinner {
  width: 40px;
  height: 40px;
  border: 3px solid rgba(139, 173, 66, 0.2);
  border-top: 3px solid #8BAD42;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

.loading-text {
  font-size: 16px;
  color: #6b7280;
  font-weight: 500;
}

/* 空状态 */
.empty-section {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 60vh;
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.2s;
}

.empty-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(20px);
  border-radius: 20px;
  padding: 40px;
  text-align: center;
  box-shadow: 
    0 12px 40px rgba(139, 173, 66, 0.15),
    0 4px 16px rgba(139, 173, 66, 0.1),
    inset 0 1px 0 rgba(255, 255, 255, 0.9);
  border: 1px solid rgba(139, 173, 66, 0.1);
}

.empty-icon {
  font-size: 64px;
  margin-bottom: 16px;
}

.empty-title {
  font-size: 20px;
  font-weight: 700;
  color: #166534;
  margin: 0 0 8px 0;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
}

.empty-text {
  font-size: 16px;
  color: #6b7280;
  font-weight: 500;
  margin: 0;
}

/* 响应式设计 */
@media (max-width: 768px) {
  .page-content {
    padding: 16px 12px;
  }
  
  .page-title {
    font-size: 20px;
    margin-bottom: 16px;
  }
  
  /* 已认领状态响应式 */
  .bird-header {
    padding: 0 12px;
  }
  
  .bird-name {
    font-size: 24px;
  }
  
  .status-badge {
    padding: 6px 12px;
    font-size: 12px;
  }
  
  .bird-hero-section {
    padding: 0 12px;
    gap: 20px;
  }
  
  .bird-image-container {
    min-height: 180px;
    border-radius: 16px;
  }
  
  .image-placeholder {
    padding: 40px 30px;
  }
  
  .image-placeholder .placeholder-icon {
    font-size: 64px;
  }
  
  .bird-status-info {
    gap: 24px;
    padding: 16px;
    border-radius: 16px;
  }
  
  .status-value {
    font-size: 28px;
  }
  
  .bird-details-section {
    margin: 0 12px;
    padding: 20px;
    border-radius: 16px;
  }
  
  .section-title {
    font-size: 16px;
  }
  
  .action-section {
    padding: 0 12px 20px;
  }
  
  .release-btn {
    padding: 12px 28px;
    font-size: 15px;
    min-width: 180px;
  }
  
  .empty-card {
    padding: 32px 24px;
    margin: 0 16px;
  }
}

@media (max-width: 480px) {
  .adopt-btn {
    padding: 12px 24px;
    font-size: 15px;
  }
  
  /* 已认领状态响应式 */
  .bird-name {
    font-size: 20px;
  }
  
  .bird-image-container {
    min-height: 150px;
  }
  
  .image-placeholder {
    padding: 30px 20px;
  }
  
  .image-placeholder .placeholder-icon {
    font-size: 48px;
  }
  
  .bird-status-info {
    gap: 16px;
    padding: 12px;
  }
  
  .status-value {
    font-size: 24px;
  }
  
  .status-label {
    font-size: 12px;
  }
  
  .bird-details-section {
    padding: 16px;
  }
  
  .detail-label {
    min-width: 80px;
    font-size: 13px;
  }
  
  .detail-value, .detail-text {
    font-size: 14px;
  }
  
  .release-btn {
    padding: 12px 24px;
    font-size: 14px;
    min-width: 160px;
  }
}
</style>
