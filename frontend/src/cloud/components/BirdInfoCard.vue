<template>
  <div class="bird-info-card" :class="{ 'fade-in': isLoaded }">
    <div class="card-inner">
      <div class="bird-header">
        <div class="bird-avatar-section">
          <div class="bird-avatar">
            <img v-if="bird.avatar_url" :src="bird.avatar_url" alt="雏鸟头像" />
            <span v-else class="avatar-emoji">🐤</span>
          </div>
        </div>
        <div class="bird-name-section">
          <h2 class="bird-name">{{ bird.bird_name || bird.name || '我的雏鸟' }}</h2>
          <span class="status-badge">已认领</span>
        </div>
      </div>
      <div class="bird-meta">
        <div class="meta-item">
          <span class="meta-icon">📅</span>
          <span class="meta-label">品种</span>
          <span class="meta-value">{{ bird.bird_species || bird.species || '未知' }}</span>
        </div>
        <div class="meta-item">
          <span class="meta-icon">🎂</span>
          <span class="meta-label">日龄</span>
          <span class="meta-value accent">{{ calculateAge(bird.birth_date) }} 天</span>
        </div>
        <div class="meta-item">
          <span class="meta-icon">🏠</span>
          <span class="meta-label">鸟笼</span>
          <span class="meta-value">{{ cageLabel }}</span>
        </div>
        <div class="meta-item">
          <span class="meta-icon">🗓️</span>
          <span class="meta-label">出生</span>
          <span class="meta-value">{{ bird.birth_date || '未知' }}</span>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed, ref, onMounted } from 'vue';

const props = defineProps<{
  bird: Record<string, any>;
}>();

const isLoaded = ref(false);

const calculateAge = (birthDate: string) => {
  if (!birthDate) return 0;
  const birth = new Date(birthDate);
  const today = new Date();
  const diffTime = Math.abs(today.getTime() - birth.getTime());
  return Math.ceil(diffTime / (1000 * 60 * 60 * 24));
};

const cageLabel = computed(() => {
  const birdcage = props.bird?.birdcage;
  if (birdcage?.label) return birdcage.label;
  if (props.bird?.area) return `${props.bird.area} #${props.bird.number || '?'}`;
  return '未绑定';
});

onMounted(() => {
  setTimeout(() => {
    isLoaded.value = true;
  }, 100);
});
</script>

<style scoped>
.bird-info-card {
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out;
}

.bird-info-card.fade-in {
  opacity: 1;
  transform: translateY(0);
}

.card-inner {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(16px);
  border-radius: 20px;
  padding: 20px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(209, 213, 219, 0.3);
}

.bird-header {
  display: flex;
  align-items: center;
  gap: 16px;
  margin-bottom: 20px;
}

.bird-avatar-section {
  flex-shrink: 0;
}

.bird-avatar {
  width: 64px;
  height: 64px;
  border-radius: 50%;
  overflow: hidden;
  background: linear-gradient(135deg, #f0fdf4 0%, #dcfce7 100%);
  border: 3px solid rgba(22, 163, 74, 0.2);
  display: flex;
  align-items: center;
  justify-content: center;
}

.bird-avatar img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.avatar-emoji {
  font-size: 36px;
}

.bird-name-section {
  flex: 1;
  display: flex;
  align-items: center;
  justify-content: space-between;
}

.bird-name {
  font-size: 24px;
  font-weight: 700;
  color: #166534;
  margin: 0;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
}

.status-badge {
  background: rgba(16, 185, 129, 0.1);
  color: #10b981;
  padding: 6px 14px;
  border-radius: 20px;
  font-size: 12px;
  font-weight: 600;
  text-transform: uppercase;
  letter-spacing: 0.5px;
  box-shadow: 0 2px 8px rgba(16, 185, 129, 0.2);
  flex-shrink: 0;
}

.bird-meta {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
}

.meta-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 12px;
  background: linear-gradient(135deg, rgba(22, 163, 74, 0.05), rgba(21, 128, 61, 0.05));
  border-radius: 12px;
}

.meta-icon {
  font-size: 18px;
  flex-shrink: 0;
}

.meta-label {
  font-size: 12px;
  color: #6b7280;
  font-weight: 500;
  flex-shrink: 0;
}

.meta-value {
  font-size: 14px;
  color: #374151;
  font-weight: 600;
  margin-left: auto;
  text-align: right;
}

.meta-value.accent {
  color: #16a34a;
}

@media (max-width: 480px) {
  .bird-name {
    font-size: 20px;
  }

  .bird-avatar {
    width: 52px;
    height: 52px;
  }

  .avatar-emoji {
    font-size: 30px;
  }

  .meta-item {
    padding: 10px;
  }
}
</style>
