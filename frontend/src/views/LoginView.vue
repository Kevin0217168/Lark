<template>
  <component :is="currentComponent" />
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import Login from '@/components/Login.vue';
import MobileLogin from '@/components/mobile/MobileLogin.vue';

const currentComponent = ref(Login);

const checkDevice = () => {
  const isMobile = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent) || window.innerWidth < 768;
  currentComponent.value = isMobile ? MobileLogin : Login;
};

onMounted(() => {
  checkDevice();
  window.addEventListener('resize', checkDevice);
});

onUnmounted(() => {
  window.removeEventListener('resize', checkDevice);
});
</script>