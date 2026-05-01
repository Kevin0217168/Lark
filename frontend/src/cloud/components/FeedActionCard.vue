<template>
  <div class="feed-action-card" :class="{ 'fade-in': isLoaded }">
    <div class="card-inner">
      <div class="feed-visual">
        <div class="feed-bird-icon">🐣</div>
        <div class="feed-divider"></div>
        <div class="feed-icon">🍞</div>
      </div>
      <h3 class="feed-title">每日喂食</h3>
      <p class="feed-desc">每天可以喂食一次，帮助雏鸟健康成长</p>
      <div class="feed-status-row">
        <span v-if="feedingStatus?.completed" class="feed-status done">
          <span class="status-dot done"></span>今日已完成
        </span>
        <span v-else class="feed-status pending">
          <span class="status-dot pending"></span>剩余 {{ feedingStatus?.remaining ?? 1 }} / {{ feedingStatus?.daily_limit ?? 1 }} 次
        </span>
      </div>
      <div v-if="feedingStatus?.last_action_time" class="feed-last-time">
        上次喂食：{{ formatFeedTime(feedingStatus.last_action_time) }}
      </div>
      <button
        class="feed-btn"
        :class="{ 'disabled': feedingStatus?.completed || loading }"
        :disabled="feedingStatus?.completed || loading"
        @click="$emit('feed')"
      >
        <span class="btn-content">
          <span class="btn-icon">{{ loading ? '⏳' : '🍞' }}</span>
          <span class="btn-label">{{ loading ? '执行中...' : '立即喂食' }}</span>
        </span>
      </button>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';

defineProps<{
  feedingStatus: {
    completed: boolean;
    today_count: number;
    daily_limit: number;
    remaining: number;
    last_action_time: string | null;
  } | null;
  loading: boolean;
}>();

defineEmits<{
  feed: [];
}>();

const isLoaded = ref(false);

const formatFeedTime = (isoString: string) => {
  const d = new Date(isoString);
  return d.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' });
};

onMounted(() => {
  setTimeout(() => {
    isLoaded.value = true;
  }, 200);
});
</script>

<style scoped>
.feed-action-card {
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.1s;
}

.feed-action-card.fade-in {
  opacity: 1;
  transform: translateY(0);
}

.card-inner {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(16px);
  border-radius: 20px;
  padding: 24px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(209, 213, 219, 0.3);
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12px;
}

.feed-visual {
  display: flex;
  align-items: center;
  gap: 14px;
  background: linear-gradient(135deg, #f0fdf4 0%, #dcfce7 100%);
  border: 2px solid rgba(22, 163, 74, 0.15);
  border-radius: 50px;
  padding: 10px 24px;
}

.feed-bird-icon {
  font-size: 32px;
  animation: bird-bounce 2s ease-in-out infinite;
}

@keyframes bird-bounce {
  0%, 100% { transform: translateY(0); }
  50% { transform: translateY(-4px); }
}

.feed-divider {
  width: 2px;
  height: 28px;
  background: linear-gradient(to bottom, transparent, rgba(22, 163, 74, 0.3), transparent);
  border-radius: 1px;
}

.feed-icon {
  font-size: 28px;
}

.feed-title {
  font-size: 18px;
  font-weight: 700;
  color: #166534;
  margin: 0;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.08);
}

.feed-desc {
  font-size: 13px;
  color: #6b7280;
  text-align: center;
  margin: 0;
  line-height: 1.5;
}

.feed-status-row {
  text-align: center;
}

.feed-status {
  font-size: 14px;
  font-weight: 500;
  display: flex;
  align-items: center;
  gap: 6px;
  justify-content: center;
}

.feed-status.done {
  color: #10b981;
}

.feed-status.pending {
  color: #e6a23c;
}

.status-dot {
  display: inline-block;
  width: 8px;
  height: 8px;
  border-radius: 50%;
}

.status-dot.done {
  background: #10b981;
  box-shadow: 0 0 6px rgba(16, 185, 129, 0.4);
}

.status-dot.pending {
  background: #e6a23c;
  box-shadow: 0 0 6px rgba(230, 162, 60, 0.4);
}

.feed-last-time {
  font-size: 12px;
  color: #9ca3af;
}

.feed-btn {
  width: 100%;
  max-width: 320px;
  background: linear-gradient(135deg, #16a34a 0%, #15803d 100%);
  color: white;
  border: none;
  border-radius: 14px;
  padding: 14px 28px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 4px 16px rgba(22, 163, 74, 0.3);
  position: relative;
  overflow: hidden;
}

.feed-btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.feed-btn:hover:not(:disabled)::before {
  left: 100%;
}

.feed-btn:hover:not(:disabled) {
  transform: translateY(-3px);
  box-shadow: 0 8px 24px rgba(22, 163, 74, 0.45);
}

.feed-btn:active:not(:disabled) {
  transform: translateY(-1px);
  box-shadow: 0 2px 8px rgba(22, 163, 74, 0.2);
  transition: all 0.1s ease;
}

.feed-btn.disabled,
.feed-btn:disabled {
  background: linear-gradient(135deg, #d1d5db 0%, #9ca3af 100%);
  cursor: not-allowed;
  box-shadow: none;
  transform: none;
}

.feed-btn.disabled::before,
.feed-btn:disabled::before {
  display: none;
}

.btn-content {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
}

.btn-icon {
  font-size: 20px;
}

.btn-label {
  font-size: 16px;
}

@media (max-width: 480px) {
  .feed-btn {
    padding: 12px 20px;
    font-size: 15px;
    max-width: 280px;
  }

  .feed-visual {
    padding: 8px 20px;
    gap: 10px;
  }

  .feed-bird-icon {
    font-size: 26px;
  }

  .feed-icon {
    font-size: 24px;
  }
}
</style>
