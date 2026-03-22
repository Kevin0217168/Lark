<template>
  <component :is="currentComponent" />
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted } from 'vue';
import Register from '@/components/Register.vue';
import MobileRegister from '@/components/mobile/MobileRegister.vue';

const currentComponent = ref(Register);

const checkDevice = () => {
  const isMobile = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent) || window.innerWidth < 768;
  currentComponent.value = isMobile ? MobileRegister : Register;
};

onMounted(() => {
  checkDevice();
  window.addEventListener('resize', checkDevice);
});

onUnmounted(() => {
  window.removeEventListener('resize', checkDevice);
});
</script>