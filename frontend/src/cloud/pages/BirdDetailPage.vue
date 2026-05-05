<template>
  <div class="bird-detail-page">
    <BackgroundDecoration />

    <div class="page-content">
      <div v-if="loading" class="loading-section">
        <div class="loading-spinner"></div>
        <p class="loading-text">加载中...</p>
      </div>

      <div v-else-if="error" class="error-section">
        <p class="error-message">{{ error }}</p>
        <button class="back-btn" @click="$router.back()">返回</button>
      </div>

      <template v-else-if="bird">
        <div class="page-header">
          <button class="back-link" @click="$router.back()">
            <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
              <polyline points="15 18 9 12 15 6"></polyline>
            </svg>
            <span>返回</span>
          </button>
        </div>

        <BirdInfoCard :bird="bird" />

        <div class="bird-description" v-if="bird.description">
          <h3 class="section-title">简介</h3>
          <p class="description-text">{{ bird.description }}</p>
        </div>

        <div class="bird-cage-info" v-if="bird.cageId">
          <h3 class="section-title">鸟笼信息</h3>
          <div class="cage-details">
            <div class="cage-item">
              <span class="cage-label">鸟笼编号</span>
              <span class="cage-value">{{ bird.cageId }}</span>
            </div>
            <div class="cage-item" v-if="bird.cageLocation">
              <span class="cage-label">位置</span>
              <span class="cage-value">{{ bird.cageLocation }}</span>
            </div>
          </div>
        </div>

        <div class="action-section" v-if="!bird.isAdopted">
          <button class="adopt-btn" :disabled="adopting" @click="handleAdopt">
            <span v-if="adopting" class="btn-spinner"></span>
            {{ adopting ? '认领中...' : '认领这只雏鸟' }}
          </button>
        </div>
      </template>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import { useRoute } from 'vue-router';
import { api } from '@/utils/api';
import BirdInfoCard from '../components/BirdInfoCard.vue';
import BackgroundDecoration from '../components/BackgroundDecoration.vue';

const route = useRoute();
const bird = ref<any>(null);
const loading = ref(true);
const error = ref('');
const adopting = ref(false);

const fetchBird = async () => {
  try {
    loading.value = true;
    error.value = '';
    const birdId = route.params.id as string;
    const res = await api.get(`/api/birds/bird/${birdId}`, true);
    bird.value = res;
  } catch (e: any) {
    error.value = e.message || '获取雏鸟信息失败';
  } finally {
    loading.value = false;
  }
};

const handleAdopt = async () => {
  if (!bird.value) return;
  try {
    adopting.value = true;
    await api.post('/api/birds/adopt', { bird_id: bird.value.bird_id }, true);
    bird.value.isAdopted = true;
  } catch (e: any) {
    error.value = e.message || '认领失败，请重试';
  } finally {
    adopting.value = false;
  }
};

onMounted(fetchBird);
</script>

<style scoped>
.bird-detail-page {
  position: relative;
  min-height: calc(100vh - 80px);
  overflow: hidden;
  background: linear-gradient(135deg, #f0fdf4 0%, #dcfce7 50%, #f0fdf4 100%);
}

.page-content {
  position: relative;
  z-index: 2;
  padding: 24px 20px;
}

.page-header {
  margin-bottom: 20px;
}

.back-link {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  background: none;
  border: none;
  color: #059669;
  font-size: 15px;
  font-weight: 600;
  cursor: pointer;
  padding: 8px 4px;
  border-radius: 8px;
  transition: all 0.2s ease;
}

.back-link:hover {
  background: rgba(5, 150, 105, 0.06);
  transform: translateX(-2px);
}

.loading-section {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 80px 0;
}

.loading-spinner {
  width: 40px;
  height: 40px;
  border: 3px solid rgba(5, 150, 105, 0.15);
  border-top-color: #059669;
  border-radius: 50%;
  animation: spin 0.8s linear infinite;
}

@keyframes spin { to { transform: rotate(360deg); } }

.loading-text {
  margin-top: 16px;
  color: #6b7280;
  font-size: 14px;
}

.error-section {
  text-align: center;
  padding: 80px 0;
}

.error-message {
  color: #ef4444;
  font-size: 15px;
  margin-bottom: 24px;
}

.back-btn {
  padding: 10px 28px;
  background: linear-gradient(135deg, #059669, #047857);
  color: #fff;
  border: none;
  border-radius: 12px;
  font-size: 15px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.2s ease;
}

.back-btn:hover {
  box-shadow: 0 4px 12px rgba(5, 150, 105, 0.3);
  transform: translateY(-1px);
}

.bird-description {
  margin-top: 24px;
  padding: 24px;
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(12px);
  border-radius: 20px;
  border: 1px solid rgba(74, 222, 128, 0.15);
}

.section-title {
  font-size: 17px;
  font-weight: 700;
  color: #065f46;
  margin: 0 0 12px;
}

.description-text {
  color: #4b5563;
  font-size: 14px;
  line-height: 1.7;
  margin: 0;
}

.bird-cage-info {
  margin-top: 24px;
  padding: 24px;
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(12px);
  border-radius: 20px;
  border: 1px solid rgba(74, 222, 128, 0.15);
}

.cage-details {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.cage-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: rgba(240, 253, 244, 0.8);
  border-radius: 12px;
}

.cage-label {
  color: #6b7280;
  font-size: 13px;
  font-weight: 500;
}

.cage-value {
  color: #111827;
  font-size: 14px;
  font-weight: 600;
}

.action-section {
  margin-top: 32px;
  text-align: center;
}

.adopt-btn {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 14px 48px;
  background: linear-gradient(135deg, #10b981, #059669);
  color: #fff;
  border: none;
  border-radius: 16px;
  font-size: 17px;
  font-weight: 700;
  cursor: pointer;
  transition: all 0.3s ease;
  box-shadow: 0 4px 16px rgba(16, 185, 129, 0.3);
}

.adopt-btn:hover:not(:disabled) {
  transform: translateY(-2px);
  box-shadow: 0 8px 24px rgba(16, 185, 129, 0.4);
}

.adopt-btn:disabled {
  opacity: 0.6;
  cursor: not-allowed;
}

.btn-spinner {
  width: 18px;
  height: 18px;
  border: 2px solid rgba(255, 255, 255, 0.3);
  border-top-color: #fff;
  border-radius: 50%;
  animation: spin 0.6s linear infinite;
}

/* ==================== 桌面端完全重写 ==================== */
@media (min-width: 769px) {
  .bird-detail-page {
    min-height: 100vh;
    background: linear-gradient(135deg, #f8fafc 0%, #f0fdf4 50%, #ecfdf5 100%);
  }

  .page-content {
    max-width: 720px;
    margin: 0 auto;
    padding: 40px 48px;
  }

  .page-header {
    margin-bottom: 28px;
  }

  .back-link {
    font-size: 15px;
    padding: 10px 8px;
  }

  .bird-description,
  .bird-cage-info {
    border-radius: 16px;
    padding: 28px;
    background: rgba(255, 255, 255, 0.85);
    backdrop-filter: blur(12px);
    border: 1px solid rgba(209, 213, 219, 0.2);
  }

  .section-title {
    font-size: 18px;
    margin-bottom: 16px;
  }

  .description-text {
    font-size: 15px;
    line-height: 1.8;
  }

  .cage-item {
    padding: 14px 18px;
    border-radius: 10px;
  }

  .cage-label {
    font-size: 13px;
  }

  .cage-value {
    font-size: 15px;
  }

  .action-section {
    margin-top: 40px;
  }

  .adopt-btn {
    padding: 16px 56px;
    font-size: 17px;
    border-radius: 16px;
    min-width: 280px;
  }
}
</style>
