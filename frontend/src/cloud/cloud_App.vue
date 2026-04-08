<template>
  <div class="cloud-app">
    <RouterView />
    <BottomNav />
  </div>
</template>

<script setup lang="ts">
import { RouterView } from 'vue-router';
import BottomNav from './components/BottomNav.vue';
</script>

<style scoped>
.cloud-app {
  min-height: 100vh;
  width: 100%;
}
</style>
