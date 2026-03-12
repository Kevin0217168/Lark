<template>
  <el-image
    class="banner"
    src="/public/d139694517c41efc8d4c6e2db4288fd.jpg"
    fit="cover"
  />
  <el-container class="app-container">
    <el-header>
      <Header />
    </el-header>
    <Sider />
    <el-main>
      <RouterView />
    </el-main>
    <el-footer> © 2026 </el-footer>
  </el-container>
</template>

<script setup lang="ts">
import { el } from "element-plus/es/locale/index.mjs";
import Header from "./components/Header.vue";
import Login from "./components/Login.vue";
import Sider from "./components/Sider.vue";

import { RouterView, RouterLink } from "vue-router";
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
