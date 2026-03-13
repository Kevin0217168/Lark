<template>
  <el-card class="device-card">
    <h3 class="title">设备管理</h3>
    
    <!-- 总览视图 -->
    <div v-if="activeTab === 'overview'">
      <h4>设备总览</h4>
      <p>设备总览功能待实现</p>
    </div>
    
    <!-- 管理视图 -->
    <div v-else-if="activeTab === 'management'">
      <h4>设备管理</h4>
      <p>设备管理功能待实现</p>
    </div>
    
    <!-- 日志视图 -->
    <div v-else-if="activeTab === 'logs'">
      <h4>设备日志</h4>
      <p>设备日志功能待实现</p>
    </div>
  </el-card>
</template>

<script lang="ts" setup>
// 接收从父组件传递的 activeTab 属性
defineProps<{
  activeTab: string;
}>();
</script>

<style lang="scss" scoped>
.device-card {
  z-index: 1;
  width: 75%;
  margin-top: 1%;
  border-radius: 15px;
  padding: 20px;
}

.title {
  text-align: center;
  margin-bottom: 20px;
}
</style>