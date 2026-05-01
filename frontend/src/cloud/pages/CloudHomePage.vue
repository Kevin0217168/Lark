<template>
  <div class="cloud-home-page">
    <div class="background-decorations">
      <div class="decoration decoration-1"></div>
      <div class="decoration decoration-2"></div>
      <div class="decoration decoration-3"></div>
      <div class="floating-elements">
        <div class="floating-element"></div>
        <div class="floating-element"></div>
        <div class="floating-element"></div>
        <div class="floating-element"></div>
      </div>
    </div>

    <div class="page-header">
      <div class="logo-section">
        <div class="logo-wrapper">
          <svg xmlns="http://www.w3.org/2000/svg" width="64" height="64" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="lark-logo" aria-hidden="true">
            <path d="M16 7h.01"></path>
            <path d="M3.4 18H12a8 8 0 0 0 8-8V7a4 4 0 0 0-7.28-2.3L2 20"></path>
            <path d="m20 7 2 .5-2 .5"></path>
            <path d="M10 18v3"></path>
            <path d="M14 17.75V21"></path>
            <path d="M7 18a6 6 0 0 0 3.84-10.61"></path>
          </svg>
        </div>
        <h1 class="app-name">云养鸟</h1>
        <p class="app-subtitle">与自然为伴，体验云养乐趣</p>
      </div>
    </div>

    <div v-if="loading" class="loading-section">
      <div class="loading-spinner"></div>
      <p class="loading-text">加载中...</p>
    </div>

    <template v-else>
      <!-- ==================== 已认领雏鸟：仪表盘布局 ==================== -->
      <template v-if="hasAdoptedBird && adoptedBirdData">
        <div class="dashboard-layout">
          <BirdInfoCard :bird="adoptedBirdData" />

          <FeedActionCard
            :feedingStatus="feedingStatus"
            :loading="feedingLoading"
            @feed="triggerFeeding"
          />

          <EnvQuickView :camDeviceId="camDeviceId" />

          <div class="quick-actions" :class="{ 'fade-in': isLoaded }">
            <button class="quick-btn primary" @click="goToMyBirds">
              <span class="btn-icon">🐤</span>
              <span class="btn-text">我的雏鸟</span>
            </button>
            <button class="quick-btn secondary" @click="goToAdoptBirds">
              <span class="btn-icon">🏷️</span>
              <span class="btn-text">看看其他雏鸟</span>
            </button>
          </div>

          <ActivityTimeline :birdId="adoptedBirdId" :camDeviceId="camDeviceId" />
        </div>
      </template>

      <!-- ==================== 未认领雏鸟：引导布局 ==================== -->
      <template v-else>
        <div class="user-section" :class="{ 'fade-in': isLoaded }">
          <div class="user-card">
            <div class="user-info">
              <div class="user-avatar">
                <img v-if="userInfo?.avatar" :src="userInfo.avatar" alt="头像" />
                <span v-else class="avatar-placeholder">{{ getAvatarText() }}</span>
              </div>
              <div class="user-details">
                <span class="user-name">{{ userInfo?.username || '用户' }}</span>
                <span class="user-status">❓ 未认领雏鸟</span>
              </div>
            </div>
          </div>
        </div>

        <div class="stats-section" :class="{ 'fade-in': isLoaded }">
          <div class="stats-card">
            <h3 class="stats-title">系统概览</h3>
            <div class="stats-grid">
              <div class="stat-item">
                <div class="stat-icon">🐦</div>
                <div class="stat-info">
                  <span class="stat-value">{{ stats.total }}</span>
                  <span class="stat-label">雏鸟总数</span>
                </div>
              </div>
              <div class="stat-item">
                <div class="stat-icon">🏠</div>
                <div class="stat-info">
                  <span class="stat-value">{{ stats.adopted }}</span>
                  <span class="stat-label">已认领</span>
                </div>
              </div>
              <div class="stat-item">
                <div class="stat-icon">🏷️</div>
                <div class="stat-info">
                  <span class="stat-value">{{ stats.available }}</span>
                  <span class="stat-label">可认领</span>
                </div>
              </div>
              <div class="stat-item">
                <div class="stat-icon">🎓</div>
                <div class="stat-info">
                  <span class="stat-value">{{ stats.grown }}</span>
                  <span class="stat-label">已长成</span>
                </div>
              </div>
            </div>
          </div>
        </div>

        <div class="quick-access-section" :class="{ 'fade-in': isLoaded }">
          <button class="quick-btn primary large" @click="goToAdoptBirds">
            <span class="btn-icon">🏷️</span>
            <span class="btn-text">去认领雏鸟</span>
          </button>
        </div>

        <div class="activity-section" :class="{ 'fade-in': isLoaded }">
          <div class="activity-card">
            <h3 class="activity-title">系统公告</h3>
            <div class="activity-list" v-if="announcements.length > 0">
              <div
                v-for="(item, index) in announcements"
                :key="index"
                class="activity-item"
                :style="{ animationDelay: `${index * 0.1}s` }"
              >
                <span class="activity-icon">{{ item.icon }}</span>
                <span class="activity-text">{{ item.text }}</span>
              </div>
            </div>
            <div class="activity-list" v-else>
              <div class="activity-item">
                <span class="activity-icon">📢</span>
                <span class="activity-text">暂无系统公告</span>
              </div>
            </div>
          </div>
        </div>
      </template>
    </template>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, computed } from 'vue';
import { useRouter } from 'vue-router';
import { ElMessage } from 'element-plus';
import { api } from '@/utils/api';
import BirdInfoCard from '@/cloud/components/BirdInfoCard.vue';
import FeedActionCard from '@/cloud/components/FeedActionCard.vue';
import EnvQuickView from '@/cloud/components/EnvQuickView.vue';
import ActivityTimeline from '@/cloud/components/ActivityTimeline.vue';

const router = useRouter();

const userInfo = ref<any>(null);
const hasAdoptedBird = ref(false);
const adoptedBirdData = ref<any>(null);
const loading = ref(true);
const isLoaded = ref(false);

const stats = ref({
  total: 0,
  adopted: 0,
  available: 0,
  grown: 0
});

const announcements = ref<Array<{ icon: string; text: string }>>([]);

const adoptedBirdId = computed(() => {
  return adoptedBirdData.value?.bird_id || adoptedBirdData.value?.id || null;
});

const camDeviceId = computed(() => {
  return adoptedBirdData.value?.birdcage?.cam_device_id ?? null;
});

const feedingStatus = ref<{
  completed: boolean;
  today_count: number;
  daily_limit: number;
  remaining: number;
  last_action_time: string | null;
} | null>(null);

const feedingLoading = ref(false);
let feedStatusTimer: number | null = null;

const getAvatarText = () => {
  if (userInfo.value?.username) {
    return userInfo.value.username.charAt(0).toUpperCase();
  }
  return 'U';
};

const getUserInfo = async () => {
  try {
    const response = await api.get('/api/users/me');
    if (response.code === 200 && response.data) {
      userInfo.value = response.data;
      localStorage.setItem('username', userInfo.value.username || '');
      localStorage.setItem('avatar', userInfo.value.avatar || '');
    }
  } catch (error) {
    console.error('获取用户信息失败:', error);
    const savedUsername = localStorage.getItem('username');
    const savedAvatar = localStorage.getItem('avatar');
    if (savedUsername) {
      userInfo.value = {
        username: savedUsername,
        avatar: savedAvatar,
        name: savedUsername
      };
    }
  }
};

const checkAdoptedBird = async () => {
  try {
    const response = await api.get('/api/birds/adopted/me');
    if (response.code === 200 && response.data?.adopted_bird && Object.keys(response.data.adopted_bird).length > 0) {
      hasAdoptedBird.value = true;
      adoptedBirdData.value = response.data.adopted_bird;
    } else {
      hasAdoptedBird.value = false;
      adoptedBirdData.value = null;
    }
  } catch (error) {
    console.error('检查认领状态失败:', error);
    hasAdoptedBird.value = false;
    adoptedBirdData.value = null;
  }
};

const getStats = async () => {
  try {
    const response = await api.get('/api/birds');
    if (response.code === 200) {
      const birds = response.data || [];
      stats.value = {
        total: birds.length,
        adopted: birds.filter((b: any) => b.status === 'adopted').length,
        available: birds.filter((b: any) => b.status === 'available').length,
        grown: birds.filter((b: any) => b.status === 'grown').length
      };
    }
  } catch (error) {
    console.error('获取统计数据失败:', error);
  }
};

const getAnnouncements = () => {
  announcements.value = [
    { icon: '🔥', text: '今日新到3只雏鸟，快来认领吧！' },
    { icon: '🎉', text: `系统已有${stats.value.adopted}只雏鸟被认领` },
    { icon: '💡', text: '提示：每用户最多认领1只雏鸟' }
  ];
};

const fetchFeedStatus = async () => {
  const birdId = adoptedBirdId.value;
  if (!birdId || !camDeviceId.value) return;
  try {
    const response = await api.get('/api/feeding/status', { bird_id: birdId });
    if (response.code === 200 && response.data) {
      feedingStatus.value = response.data.feeding;
    }
  } catch (error) {
    console.error('获取喂食状态失败:', error);
  }
};

const startFeedStatusTimer = () => {
  stopFeedStatusTimer();
  fetchFeedStatus();
  feedStatusTimer = window.setInterval(fetchFeedStatus, 30000);
};

const stopFeedStatusTimer = () => {
  if (feedStatusTimer) {
    clearInterval(feedStatusTimer);
    feedStatusTimer = null;
  }
};

const triggerFeeding = async () => {
  if (feedingLoading.value || feedingStatus.value?.completed) return;
  const birdId = adoptedBirdId.value;
  if (!birdId) {
    ElMessage.warning('未找到雏鸟信息，无法执行喂食');
    return;
  }
  feedingLoading.value = true;
  try {
    const response = await api.post(`/api/feeding/trigger?bird_id=${birdId}`);
    ElMessage.closeAll();
    if (response.code === 200) {
      ElMessage.success('喂食指令已发送！');
      await fetchFeedStatus();
    } else {
      ElMessage.error(response.msg || '喂食失败');
    }
  } catch (error: any) {
    ElMessage.closeAll();
    if (error.data && error.data.msg) {
      ElMessage.error(error.data.msg);
    } else if (error.message) {
      ElMessage.error(error.message);
    } else {
      ElMessage.error('网络错误，请检查网络连接');
    }
  } finally {
    feedingLoading.value = false;
  }
};

const goToAdoptBirds = () => {
  router.push('/cloud/adopt-birds');
};

const goToMyBirds = () => {
  router.push('/cloud/birds');
};

onMounted(async () => {
  await Promise.all([
    getUserInfo(),
    checkAdoptedBird(),
    getStats()
  ]);

  if (hasAdoptedBird.value) {
    startFeedStatusTimer();
  } else {
    getAnnouncements();
  }

  loading.value = false;
  setTimeout(() => {
    isLoaded.value = true;
  }, 100);
});
</script>

<style scoped>
.cloud-home-page {
  min-height: 100vh;
  background: linear-gradient(135deg, #f0fdf4 0%, #dcfce7 50%, #bbf7d0 100%);
  padding: 20px 16px calc(80px + env(safe-area-inset-bottom, 0px));
  position: relative;
  overflow-x: hidden;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
}

/* 背景装饰 */
.background-decorations {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 1;
  pointer-events: none;
}

.decoration {
  position: absolute;
  border-radius: 50%;
  background: rgba(167, 243, 208, 0.4);
  filter: blur(60px);
  animation: float 8s ease-in-out infinite;
}

.decoration-1 {
  top: 10%;
  left: 10%;
  width: 200px;
  height: 200px;
  background: rgba(110, 231, 183, 0.3);
  animation-delay: 0s;
}

.decoration-2 {
  top: 60%;
  right: 10%;
  width: 150px;
  height: 150px;
  background: rgba(167, 243, 208, 0.2);
  animation-delay: 2s;
}

.decoration-3 {
  bottom: 20%;
  left: 20%;
  width: 180px;
  height: 180px;
  background: rgba(74, 222, 128, 0.25);
  animation-delay: 4s;
}

.floating-elements {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  overflow: hidden;
}

.floating-element {
  position: absolute;
  width: 8px;
  height: 8px;
  background: rgba(74, 222, 128, 0.6);
  border-radius: 50%;
  animation: floatUp 6s ease-in-out infinite;
}

.floating-element:nth-child(1) { top: 20%; left: 20%; }
.floating-element:nth-child(2) { top: 40%; right: 30%; animation-delay: 1s; background: rgba(110, 231, 183, 0.5); }
.floating-element:nth-child(3) { bottom: 30%; left: 40%; animation-delay: 2s; background: rgba(167, 243, 208, 0.4); }
.floating-element:nth-child(4) { top: 60%; left: 70%; animation-delay: 3s; background: rgba(74, 222, 128, 0.5); }

@keyframes float {
  0%, 100% { transform: translateY(0px) translateX(0px); }
  25% { transform: translateY(-20px) translateX(10px); }
  50% { transform: translateY(10px) translateX(-10px); }
  75% { transform: translateY(-15px) translateX(5px); }
}

@keyframes floatUp {
  0% { transform: translateY(100vh) scale(0); opacity: 0; }
  10% { opacity: 1; transform: translateY(80vh) scale(1); }
  90% { opacity: 1; transform: translateY(20vh) scale(1); }
  100% { transform: translateY(-20vh) scale(0); opacity: 0; }
}

/* 页面头部 */
.page-header {
  text-align: center;
  margin-bottom: 24px;
  position: relative;
  z-index: 2;
  animation: fadeInUp 0.8s ease-out forwards;
}

@keyframes fadeInUp {
  to { opacity: 1; transform: translateY(0); }
  from { opacity: 0; transform: translateY(30px); }
}

.logo-section {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
}

.logo-wrapper {
  width: 72px;
  height: 72px;
  background: linear-gradient(135deg, #16a34a 0%, #15803d 100%);
  border-radius: 22px;
  display: flex;
  align-items: center;
  justify-content: center;
  box-shadow: 0 8px 32px rgba(22, 163, 74, 0.3);
  animation: pulse 2s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% { box-shadow: 0 8px 32px rgba(22, 163, 74, 0.3); }
  50% { box-shadow: 0 12px 40px rgba(22, 163, 74, 0.4); }
}

.lark-logo {
  color: white;
  stroke-width: 1.5;
}

.app-name {
  font-size: 24px;
  font-weight: 800;
  color: #166534;
  margin: 0;
  background: linear-gradient(135deg, #16a34a, #15803d);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
  letter-spacing: 2px;
}

.app-subtitle {
  font-size: 13px;
  color: #6b7280;
  margin: 0;
  font-weight: 500;
  opacity: 0.9;
}

/* 加载状态 */
.loading-section {
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  min-height: 50vh;
  gap: 16px;
  position: relative;
  z-index: 2;
}

.loading-spinner {
  width: 40px;
  height: 40px;
  border: 3px solid rgba(22, 163, 74, 0.2);
  border-top: 3px solid #16a34a;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

.loading-text {
  font-size: 14px;
  color: #6b7280;
  font-weight: 500;
}

/* ==================== 仪表盘布局 ==================== */
.dashboard-layout {
  position: relative;
  z-index: 2;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

/* 快捷操作按钮组 */
.quick-actions {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.25s;
}

.quick-actions.fade-in {
  opacity: 1;
  transform: translateY(0);
}

.quick-btn {
  height: 52px;
  border-radius: 14px;
  font-size: 15px;
  font-weight: 600;
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  cursor: pointer;
  transition: all 0.3s ease;
  border: none;
  position: relative;
  overflow: hidden;
}

.quick-btn::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: left 0.6s ease;
}

.quick-btn:hover::before {
  left: 100%;
}

.quick-btn.primary {
  background: linear-gradient(135deg, #16a34a 0%, #15803d 100%);
  color: white;
  box-shadow: 0 4px 16px rgba(22, 163, 74, 0.3);
}

.quick-btn.primary:hover {
  transform: translateY(-2px);
  box-shadow: 0 6px 20px rgba(22, 163, 74, 0.4);
}

.quick-btn.secondary {
  background: rgba(255, 255, 255, 0.85);
  color: #166534;
  border: 2px solid rgba(22, 163, 74, 0.2);
}

.quick-btn.secondary:hover {
  background: rgba(255, 255, 255, 0.95);
  border-color: #16a34a;
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.08);
}

.quick-btn:active {
  transform: translateY(0);
}

.btn-icon {
  font-size: 20px;
}

.btn-text {
  font-weight: 600;
}

/* ==================== 未认领引导布局 ==================== */
.user-section {
  margin-bottom: 16px;
  position: relative;
  z-index: 2;
  opacity: 0;
  transition: opacity 0.5s ease;
}

.user-section.fade-in { opacity: 1; }

.user-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(16px);
  border-radius: 16px;
  padding: 16px 20px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(209, 213, 219, 0.3);
}

.user-info {
  display: flex;
  align-items: center;
  gap: 12px;
}

.user-avatar {
  width: 48px;
  height: 48px;
  border-radius: 50%;
  overflow: hidden;
  background: linear-gradient(135deg, #16a34a, #15803d);
  display: flex;
  align-items: center;
  justify-content: center;
  flex-shrink: 0;
}

.user-avatar img {
  width: 100%;
  height: 100%;
  object-fit: cover;
}

.avatar-placeholder {
  color: white;
  font-size: 20px;
  font-weight: 700;
}

.user-details {
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.user-name {
  font-size: 18px;
  font-weight: 700;
  color: #166534;
}

.user-status {
  font-size: 14px;
  color: #6b7280;
  font-weight: 500;
}

/* 系统概览 */
.stats-section {
  margin-bottom: 16px;
  position: relative;
  z-index: 2;
  opacity: 0;
  transition: opacity 0.5s ease 0.1s;
}

.stats-section.fade-in { opacity: 1; }

.stats-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(16px);
  border-radius: 16px;
  padding: 20px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(209, 213, 219, 0.3);
}

.stats-title {
  font-size: 16px;
  font-weight: 700;
  color: #166534;
  margin: 0 0 16px 0;
}

.stats-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
}

.stat-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px;
  background: linear-gradient(135deg, rgba(22, 163, 74, 0.05), rgba(21, 128, 61, 0.05));
  border-radius: 12px;
  transition: all 0.3s ease;
}

.stat-item:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(22, 163, 74, 0.15);
}

.stat-icon {
  font-size: 26px;
  width: 44px;
  height: 44px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(255, 255, 255, 0.9);
  border-radius: 12px;
  flex-shrink: 0;
}

.stat-info {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.stat-value {
  font-size: 22px;
  font-weight: 800;
  color: #166534;
  line-height: 1;
}

.stat-label {
  font-size: 12px;
  color: #6b7280;
  font-weight: 500;
}

/* 快捷入口 */
.quick-access-section {
  margin-bottom: 16px;
  position: relative;
  z-index: 2;
  opacity: 0;
  transition: opacity 0.5s ease 0.2s;
}

.quick-access-section.fade-in { opacity: 1; }

.quick-btn.primary.large {
  width: 100%;
  height: 60px;
  font-size: 17px;
  border-radius: 16px;
}

.quick-btn.primary.large .btn-icon {
  font-size: 24px;
}

/* 系统公告 */
.activity-section {
  position: relative;
  z-index: 2;
  opacity: 0;
  transition: opacity 0.5s ease 0.3s;
}

.activity-section.fade-in { opacity: 1; }

.activity-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(16px);
  border-radius: 16px;
  padding: 20px;
  box-shadow: 0 8px 32px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(209, 213, 219, 0.3);
}

.activity-title {
  font-size: 16px;
  font-weight: 700;
  color: #166534;
  margin: 0 0 16px 0;
}

.activity-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.activity-item {
  display: flex;
  align-items: center;
  gap: 12px;
  padding: 12px;
  background: linear-gradient(135deg, rgba(22, 163, 74, 0.05), rgba(21, 128, 61, 0.05));
  border-radius: 12px;
  animation: slideInRight 0.5s ease-out both;
  transition: all 0.3s ease;
}

@keyframes slideInRight {
  from { opacity: 0; transform: translateX(20px); }
  to { opacity: 1; transform: translateX(0); }
}

.activity-item:hover {
  background: linear-gradient(135deg, rgba(22, 163, 74, 0.1), rgba(21, 128, 61, 0.1));
  transform: translateX(4px);
}

.activity-icon {
  font-size: 24px;
  flex-shrink: 0;
}

.activity-text {
  font-size: 14px;
  color: #374151;
  font-weight: 500;
  line-height: 1.4;
}

/* 通用渐入 */
.fade-in {
  opacity: 1 !important;
}

/* 响应式 */
@media (max-width: 480px) {
  .cloud-home-page {
    padding: 16px 12px calc(80px + env(safe-area-inset-bottom, 0px));
  }

  .logo-wrapper {
    width: 60px;
    height: 60px;
    border-radius: 18px;
  }

  .lark-logo {
    width: 44px;
    height: 44px;
  }

  .app-name { font-size: 20px; }
  .app-subtitle { font-size: 12px; }

  .dashboard-layout { gap: 12px; }

  .quick-actions { gap: 10px; }
  .quick-btn { height: 48px; font-size: 14px; }

  .stats-grid { gap: 10px; }
  .stat-value { font-size: 18px; }
}
</style>
