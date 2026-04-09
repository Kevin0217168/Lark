<template>
  <div class="birds-page">
    <div class="page-content">
      <h1 class="page-title">雏鸟列表</h1>
      <div class="action-section">
        <button class="adopt-btn" @click="goToAdoptBirds">
          <span class="btn-icon">🏷️</span>
          <span class="btn-text">去认领雏鸟</span>
        </button>
      </div>
      <div class="placeholder-content">
        <div class="placeholder-card">
          <div class="placeholder-icon">🐦</div>
          <p class="placeholder-text">雏鸟列表内容区域</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { useRouter } from 'vue-router';

const router = useRouter();

const goToAdoptBirds = () => {
  router.push('/cloud/adopt-birds');
};
</script>

<style scoped>
.birds-page {
  min-height: 100vh;
  background: linear-gradient(135deg, #f5f7fa 0%, #e8f0e8 100%);
  padding-bottom: calc(80px + env(safe-area-inset-bottom, 0px));
}

.page-content {
  padding: 20px 16px;
}

.page-title {
  font-size: 24px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 20px;
  text-align: center;
}

.placeholder-content {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 60vh;
}

.placeholder-card {
  background: rgba(255, 255, 255, 0.8);
  backdrop-filter: blur(10px);
  border-radius: 20px;
  padding: 40px;
  text-align: center;
  box-shadow: 0 4px 20px rgba(139, 173, 66, 0.1);
  border: 1px solid rgba(139, 173, 66, 0.1);
}

.placeholder-icon {
  font-size: 64px;
  margin-bottom: 16px;
}

.placeholder-text {
  font-size: 16px;
  color: #6b7280;
  font-weight: 500;
}

.action-section {
  margin-bottom: 20px;
  text-align: center;
}

.adopt-btn {
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
  color: white;
  border: none;
  border-radius: 12px;
  padding: 12px 24px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  display: inline-flex;
  align-items: center;
  gap: 8px;
  box-shadow: 0 4px 12px rgba(139, 173, 66, 0.3);
  transition: all 0.3s ease;
}

.adopt-btn:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 16px rgba(139, 173, 66, 0.4);
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
</style>
