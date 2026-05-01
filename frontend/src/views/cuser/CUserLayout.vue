<template>
  <div class="cuser-layout">
    <!-- 二级导航栏 -->
    <div class="sub-nav">
      <div class="nav-container">
        <div class="nav-links">
          <router-link to="/CUser/monitor" class="nav-link" active-class="active">
            <el-icon><Monitor /></el-icon> 首页监测
          </router-link>
          <router-link to="/CUser/clinic" class="nav-link" active-class="active">
            <el-icon><FirstAidKit /></el-icon> 线上问诊
          </router-link>
          <router-link to="/CUser/ai" class="nav-link" active-class="active">
            <el-icon><Cpu /></el-icon> AI 自检
          </router-link>
          <router-link to="/CUser/profile" class="nav-link" active-class="active">
            <el-icon><User /></el-icon> 个人中心
          </router-link>
        </div>
      </div>
    </div>

    <!-- 内容渲染区 -->
    <div class="content-view">
      <router-view v-slot="{ Component }">
        <transition name="fade-transform" mode="out-in">
          <component :is="Component" />
        </transition>
      </router-view>
    </div>
  </div>
</template>

<script setup lang="ts">
import { Monitor, Cpu, User, FirstAidKit } from '@element-plus/icons-vue';
</script>

<style scoped lang="scss">
.cuser-layout {
  min-height: calc(100vh - 70px);
  background-color: #f4f7f5;
}

.sub-nav {
  background-color: #fff;
  border-bottom: 1px solid #e2e8f0;
  position: sticky;
  top: 0;
  z-index: 100;
  box-shadow: 0 2px 4px rgba(0,0,0,0.02);
}

.nav-container {
  max-width: 1400px;
  margin: 0 auto;
  padding: 0 40px;
}

.nav-links {
  display: flex;
  gap: 40px;
  height: 50px;
  align-items: center;
}

.nav-link {
  text-decoration: none;
  color: #64748b;
  font-size: 15px;
  font-weight: 600;
  display: flex;
  align-items: center;
  gap: 8px;
  height: 100%;
  border-bottom: 2px solid transparent;
  transition: all 0.3s ease;

  &:hover {
    color: #8BAD42;
  }

  &.active {
    color: #8BAD42;
    border-bottom-color: #8BAD42;
  }
}

.content-view {
  padding: 30px 40px;
  max-width: 1400px;
  margin: 0 auto;
}

/* 页面切换动画 */
.fade-transform-enter-active,
.fade-transform-leave-active {
  transition: all 0.3s;
}

.fade-transform-enter-from {
  opacity: 0;
  transform: translateY(10px);
}

.fade-transform-leave-to {
  opacity: 0;
  transform: translateY(-10px);
}
</style>