<template>
  <div class="background-decorations" aria-hidden="true">
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
</template>

<script setup lang="ts">
</script>

<style scoped>
.background-decorations {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  z-index: 1;
  pointer-events: none;
  overflow: hidden;
}

.decoration {
  position: absolute;
  border-radius: 50%;
  background: rgba(167, 243, 208, 0.4);
  filter: blur(60px);
  animation: float 8s ease-in-out infinite;
  will-change: transform;
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
  will-change: transform, opacity;
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

@media (prefers-reduced-motion: reduce) {
  .decoration,
  .floating-element {
    animation: none;
  }
}
</style>
