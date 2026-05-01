<template>
  <div class="cloud-app-container" style="max-width: 480px; margin: 0 auto; background-color: #f5f7fa; min-height: 100vh; position: relative; padding-bottom: 60px; box-shadow: 0 0 20px rgba(0,0,0,0.1);">
    
    <!-- 子路由内容区 -->
    <router-view />

    <!-- 底部固定导航栏 (TabBar) -->
    <div style="position: absolute; bottom: 0; width: 100%; max-width: 480px; height: 60px; background: #fff; display: flex; justify-content: space-around; align-items: center; border-top: 1px solid #ebeef5; border-bottom-left-radius: 8px; border-bottom-right-radius: 8px; z-index: 999;">
      
      <!-- 监测 -->
      <router-link to="/cloud/home" class="tab-item" active-class="tab-active">
        <svg class="tab-icon" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 19v-6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2a2 2 0 002-2zm0 0V9a2 2 0 012-2h2a2 2 0 012 2v10m-6 0a2 2 0 002 2h2a2 2 0 002-2m0 0V5a2 2 0 012-2h2a2 2 0 012 2v14a2 2 0 01-2 2h-2a2 2 0 01-2-2z" />
        </svg>
        <span class="tab-text">监测</span>
      </router-link>

      <!-- 问诊 -->
      <router-link to="/cloud/consultation" class="tab-item" active-class="tab-active">
        <svg class="tab-icon" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M17 8h2a2 2 0 012 2v6a2 2 0 01-2 2h-2v4l-4-4H9a1.994 1.994 0 01-1.414-.586m0 0L11 14h4a2 2 0 002-2V6a2 2 0 00-2-2H5a2 2 0 00-2 2v6a2 2 0 002 2h2v4l.586-.586z" />
        </svg>
        <span class="tab-text">问诊</span>
      </router-link>

      <!-- 自检 -->
      <router-link to="/cloud/checkup" class="tab-item" active-class="tab-active">
        <svg class="tab-icon" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z" />
        </svg>
        <span class="tab-text">自检</span>
      </router-link>

      <!-- 我的 -->
      <router-link to="/cloud/profile" class="tab-item" active-class="tab-active">
        <svg class="tab-icon" fill="none" viewBox="0 0 24 24" stroke="currentColor">
          <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M16 7a4 4 0 11-8 0 4 4 0 018 0zM12 14a7 7 0 00-7 7h14a7 7 0 00-7-7z" />
        </svg>
        <span class="tab-text">我的</span>
      </router-link>

    </div>
  </div>
</template>

<script setup lang="ts">
// C 端全局布局组件
</script>

<style scoped>
/* 导航项默认状态（未激活） */
.tab-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  text-decoration: none;
  color: #909399; /* 灰色 */
  flex: 1;
  height: 100%;
  transition: color 0.2s ease-in-out;
}

.tab-icon {
  width: 24px;
  height: 24px;
  margin-bottom: 4px;
}

.tab-text {
  font-size: 12px;
  font-weight: 500;
}

/* 导航项激活状态 */
.tab-active {
  color: #8BAD42; /* 主题色 */
}
</style>
