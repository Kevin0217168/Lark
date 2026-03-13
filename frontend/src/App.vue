<template>
  <el-image
    class="banner"
    src="/public/banner.jpg"
    fit="cover"
  />
  <el-container class="app-container">
    <el-header>
      <Header />
    </el-header>
    <Sider @tabChange="handleTabChange" />
    <el-main>
      <RouterView v-slot="{ Component }">
        <component :is="Component" :activeTab="activeTab" />
      </RouterView>
    </el-main>
    <el-footer> © 2026 </el-footer>
  </el-container>
</template>

<script setup lang="ts">
import { ref } from "vue";
import { el } from "element-plus/es/locale/index.mjs";
import Header from "./components/Header.vue";
import Login from "./components/Login.vue";
import Sider from "./components/Sider.vue";

import { RouterView, RouterLink } from "vue-router";

const activeTab = ref<string>('realtime');

const handleTabChange = (tab: string) => {
  activeTab.value = tab;
};
</script>

<style scoped>
/* Main */
main,
footer {
  display: flex;
  justify-content: center;
  align-items: center;
  text-align: center;
}

header {
  position: sticky;
  top: 0;
  overflow: visible;
  z-index: 1000;
}

.banner {
  position: absolute;
  z-index: 0;
  top: 0;
  left: 0;
  width: 100%;
  margin: 0;
  --el-header-padding: 0;
  height: 400px;
}
</style>
