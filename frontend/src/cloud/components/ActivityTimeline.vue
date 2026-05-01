<template>
  <div class="activity-timeline" :class="{ 'fade-in': isLoaded }">
    <div class="card-inner">
      <h3 class="timeline-title">最近动态</h3>
      <div class="timeline-list" v-if="activities.length > 0">
        <div
          v-for="(item, index) in activities"
          :key="index"
          class="timeline-item"
          :style="{ animationDelay: `${index * 0.1}s` }"
        >
          <div class="timeline-dot" :style="{ background: item.color }"></div>
          <div class="timeline-content">
            <span class="timeline-text">{{ item.text }}</span>
            <span class="timeline-time">{{ item.time }}</span>
          </div>
        </div>
      </div>
      <div class="timeline-empty" v-else>
        <span class="empty-icon">📋</span>
        <span class="empty-text">暂无动态记录</span>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue';
import { api } from '@/utils/api';

const props = defineProps<{
  birdId: number | null;
  camDeviceId: string | null;
}>();

const isLoaded = ref(false);
const activities = ref<Array<{ text: string; time: string; color: string }>>([]);

const fetchFeedingHistory = async () => {
  if (!props.birdId) return;

  const items: Array<{ text: string; time: string; color: string }> = [];

  try {
    const response = await api.get('/api/feeding/status', {
      bird_id: props.birdId,
    });

    if (response.code === 200 && response.data?.feeding?.last_action_time) {
      const feedTime = new Date(response.data.feeding.last_action_time);
      const now = new Date();
      const diffDays = Math.floor((now.getTime() - feedTime.getTime()) / (1000 * 60 * 60 * 24));
      const timeStr = feedTime.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' });
      const dateStr = feedTime.toLocaleDateString('zh-CN', { month: 'short', day: 'numeric' });

      let timeLabel: string;
      if (diffDays === 0) timeLabel = `今天 ${timeStr}`;
      else if (diffDays === 1) timeLabel = `昨天 ${timeStr}`;
      else timeLabel = `${dateStr} ${timeStr}`;

      items.push({
        text: '完成今日喂食',
        time: timeLabel,
        color: '#67c23a'
      });
    }
  } catch (error) {
    console.error('获取喂食历史失败:', error);
  }

  activities.value = items;
};

onMounted(async () => {
  await fetchFeedingHistory();

  setTimeout(() => {
    isLoaded.value = true;
  }, 400);
});
</script>

<style scoped>
.activity-timeline {
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.3s;
}

.activity-timeline.fade-in {
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

.timeline-title {
  font-size: 16px;
  font-weight: 700;
  color: #166534;
  margin: 0 0 16px 0;
}

.timeline-list {
  display: flex;
  flex-direction: column;
  gap: 0;
}

.timeline-item {
  display: flex;
  gap: 14px;
  padding: 12px 0;
  position: relative;
  animation: slideInRight 0.5s ease-out both;
}

.timeline-item:not(:last-child)::after {
  content: '';
  position: absolute;
  left: 5px;
  top: 32px;
  bottom: 0;
  width: 2px;
  background: rgba(22, 163, 74, 0.12);
}

@keyframes slideInRight {
  from {
    opacity: 0;
    transform: translateX(20px);
  }
  to {
    opacity: 1;
    transform: translateX(0);
  }
}

.timeline-dot {
  width: 12px;
  height: 12px;
  border-radius: 50%;
  flex-shrink: 0;
  margin-top: 4px;
  box-shadow: 0 0 8px rgba(103, 194, 58, 0.3);
}

.timeline-content {
  flex: 1;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.timeline-text {
  font-size: 14px;
  color: #374151;
  font-weight: 500;
}

.timeline-time {
  font-size: 12px;
  color: #9ca3af;
  font-weight: 400;
  flex-shrink: 0;
  margin-left: 12px;
}

.timeline-empty {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 24px 0;
}

.empty-icon {
  font-size: 20px;
}

.empty-text {
  font-size: 14px;
  color: #9ca3af;
}

@media (max-width: 480px) {
  .timeline-content {
    flex-direction: column;
    align-items: flex-start;
    gap: 4px;
  }

  .timeline-time {
    margin-left: 0;
  }
}
</style>
