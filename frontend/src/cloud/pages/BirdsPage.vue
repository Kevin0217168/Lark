<template>
  <div class="birds-page">
    <div class="page-content">
      <h1 class="page-title">雏鸟列表</h1>
      <div class="placeholder-content">
        <div class="placeholder-card">
          <div class="placeholder-icon">🐦</div>
          <p class="placeholder-text">雏鸟列表内容区域</p>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
</script>

<style scoped>
.birds-page {
  min-height: 100vh;
  background: linear-gradient(135deg, #f5f7fa 0%, #e8f0e8 100%);
  padding-bottom: calc(80px + env(safe-area-inset-bottom, 0px));
}

.page-content {
  padding: 20px 16px;
}

.page-title {
  font-size: 24px;
  font-weight: 700;
  color: #1f2937;
  margin-bottom: 20px;
  text-align: center;
}

.placeholder-content {
  display: flex;
  justify-content: center;
  align-items: center;
  min-height: 60vh;
}

.placeholder-card {
  background: rgba(255, 255, 255, 0.8);
  backdrop-filter: blur(10px);
  border-radius: 20px;
  padding: 40px;
  text-align: center;
  box-shadow: 0 4px 20px rgba(139, 173, 66, 0.1);
  border: 1px solid rgba(139, 173, 66, 0.1);
}

.placeholder-icon {
  font-size: 64px;
  margin-bottom: 16px;
}

.placeholder-text {
  font-size: 16px;
  color: #6b7280;
  font-weight: 500;
}
</style>
