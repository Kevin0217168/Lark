<template>
  <div class="cloud-home-page">
    <BackgroundDecoration />

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

          <!-- 食量统计表格 -->
          <div class="feed-table-section" :class="{ 'fade-in': isLoaded }">
            <div class="feed-table-card">
              <h3 class="feed-table-title">每日食量</h3>
              <p class="feed-table-desc">近5天食量记录</p>
              <div class="feed-table">
                <div class="feed-table-header">
                  <span class="th-date">日期</span>
                  <span class="th-grams">食量 (克)</span>
                  <span class="th-bar">占比</span>
                </div>
                <div
                  v-for="(row, idx) in feedTableData"
                  :key="idx"
                  class="feed-table-row"
                  :style="{ animationDelay: `${idx * 0.1}s` }"
                >
                  <span class="td-date">{{ row.date }}</span>
                  <span class="td-grams">{{ row.grams }} g</span>
                  <div class="td-bar-wrapper">
                    <div
                      class="td-bar-fill"
                      :style="{ width: (row.grams / maxFeedGrams * 100) + '%' }"
                    ></div>
                  </div>
                </div>
              </div>
            </div>
          </div>

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
import { api } from '@/utils/api';
import BirdInfoCard from '@/cloud/components/BirdInfoCard.vue';
import EnvQuickView from '@/cloud/components/EnvQuickView.vue';
import ActivityTimeline from '@/cloud/components/ActivityTimeline.vue';
import BackgroundDecoration from '@/cloud/components/BackgroundDecoration.vue';

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

// 模拟食量表格数据（近5天，每天15-30克）
const seededRandom = (seed: number) => {
  let s = seed;
  return () => {
    s = (s * 1664525 + 1013904223) & 0xffffffff;
    return (s >>> 0) / 0xffffffff;
  };
};

const feedTableData = computed(() => {
  const today = new Date();
  const rng = seededRandom(today.getDate() + today.getMonth() * 31);
  const rows: Array<{ date: string; grams: number }> = [];
  for (let i = 0; i < 5; i++) {
    const d = new Date(today);
    d.setDate(d.getDate() - (4 - i));
    const month = d.getMonth() + 1;
    const day = d.getDate();
    rows.push({
      date: `${month}月${day}日`,
      grams: Math.round((rng() * 16 + 15) * 10) / 10,
    });
  }
  return rows;
});

const maxFeedGrams = computed(() => {
  const max = Math.max(...feedTableData.value.map(r => r.grams), 15);
  return Math.ceil(max / 5) * 5;
});

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
    // 已认领雏鸟，食量表格数据会自动展示
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

/* 食量统计表格 */
.feed-table-section {
  opacity: 0;
  transform: translateY(20px);
  transition: all 0.6s ease-out 0.3s;
}

.feed-table-section.fade-in {
  opacity: 1;
  transform: translateY(0);
}

.feed-table-card {
  background: rgba(255, 255, 255, 0.75);
  backdrop-filter: blur(16px);
  border-radius: 20px;
  padding: 20px;
  box-shadow: 0 4px 20px rgba(0, 0, 0, 0.06);
  border: 1px solid rgba(209, 213, 219, 0.3);
}

.feed-table-title {
  font-size: 16px;
  font-weight: 700;
  color: #166534;
  margin: 0 0 4px 0;
}

.feed-table-desc {
  font-size: 12px;
  color: #9ca3af;
  margin: 0 0 16px 0;
}

.feed-table {
  width: 100%;
}

.feed-table-header {
  display: flex;
  align-items: center;
  padding: 8px 12px;
  background: rgba(22, 163, 74, 0.06);
  border-radius: 10px;
  margin-bottom: 6px;
}

.th-date {
  width: 72px;
  font-size: 12px;
  font-weight: 600;
  color: #6b7280;
}

.th-grams {
  width: 72px;
  font-size: 12px;
  font-weight: 600;
  color: #6b7280;
  text-align: center;
}

.th-bar {
  flex: 1;
  font-size: 12px;
  font-weight: 600;
  color: #6b7280;
  padding-left: 12px;
}

.feed-table-row {
  display: flex;
  align-items: center;
  padding: 10px 12px;
  border-radius: 10px;
  transition: background 0.2s ease;
  animation: slideInRight 0.5s ease-out both;
}

.feed-table-row:hover {
  background: rgba(22, 163, 74, 0.04);
}

@keyframes slideInRight {
  from { opacity: 0; transform: translateX(16px); }
  to { opacity: 1; transform: translateX(0); }
}

.td-date {
  width: 72px;
  font-size: 13px;
  font-weight: 500;
  color: #374151;
}

.td-grams {
  width: 72px;
  font-size: 14px;
  font-weight: 700;
  color: #16a34a;
  text-align: center;
}

.td-bar-wrapper {
  flex: 1;
  height: 22px;
  background: #f3f4f6;
  border-radius: 6px;
  overflow: hidden;
  margin-left: 12px;
}

.td-bar-fill {
  height: 100%;
  background: linear-gradient(90deg, #86efac, #22c55e, #16a34a);
  border-radius: 6px;
  transition: width 0.8s cubic-bezier(0.25, 0.46, 0.45, 0.94);
  min-width: 30px;
  position: relative;
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

/* ==================== 桌面端完全重写 ==================== */
@media (min-width: 769px) {
  .cloud-home-page {
    padding: 0;
    max-width: none;
    margin: 0;
    background: linear-gradient(135deg, #f8fafc 0%, #f0fdf4 50%, #ecfdf5 100%);
  }

  .page-header {
    margin-bottom: 32px;
    padding: 0 48px;
  }

  .app-name {
    font-size: 28px;
    letter-spacing: 3px;
  }

  .app-subtitle {
    font-size: 14px;
    color: #64748b;
  }

  .logo-wrapper {
    width: 80px;
    height: 80px;
    border-radius: 24px;
  }

  /* 仪表盘布局 */
  .dashboard-layout {
    display: grid;
    grid-template-columns: 1fr 380px;
    gap: 24px;
    padding: 0 48px 40px;
  }

  .dashboard-layout > :first-child {
    grid-column: 1;
    grid-row: 1;
  }

  .feed-table-section {
    grid-column: 1;
    grid-row: 2;
    margin: 0;
  }

  .feed-table-card {
    border-radius: 16px;
    padding: 28px;
    background: rgba(255, 255, 255, 0.9);
  }

  .feed-table-title {
    font-size: 18px;
  }

  .feed-table-desc {
    font-size: 13px;
    margin-bottom: 20px;
  }

  .quick-actions {
    display: flex;
    flex-direction: column;
    gap: 12px;
    grid-column: 2;
    grid-row: 2;
  }

  .quick-btn {
    height: 56px;
    font-size: 16px;
    justify-content: flex-start;
    padding: 0 24px;
  }

  /* 未认领状态 */
  .stats-grid {
    grid-template-columns: repeat(4, 1fr);
    gap: 16px;
  }

  .stat-item {
    padding: 20px;
    border-radius: 16px;
    background: linear-gradient(135deg, rgba(22, 163, 74, 0.04), rgba(21, 128, 61, 0.02));
  }

  .stat-icon {
    width: 52px;
    height: 52px;
    font-size: 28px;
  }

  .stat-value {
    font-size: 28px;
  }

  .stat-label {
    font-size: 13px;
  }

  .user-card {
    border-radius: 16px;
    padding: 24px;
  }

  .quick-access-section {
    display: flex;
    justify-content: center;
  }

  .quick-btn.primary.large {
    max-width: 400px;
  }

  .activity-card {
    border-radius: 16px;
    padding: 28px;
    max-width: 600px;
    margin: 0 auto;
  }

  .activity-title {
    font-size: 18px;
    margin-bottom: 20px;
  }
}

@media (min-width: 769px) and (max-width: 1023px) {
  .dashboard-layout {
    grid-template-columns: 1fr 320px;
    gap: 20px;
    padding: 0 24px 32px;
  }

  .page-header {
    padding: 0 24px;
  }
}

/* 移动端样式 - 原封不动 */
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
