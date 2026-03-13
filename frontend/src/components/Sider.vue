<template>
  <el-aside v-if="isDataPage || isDevicePage">
    <el-menu 
      class="menu" 
      :default-active="activeTab" 
      @select="handleMenuSelect"
    >
      <!-- 数据页面菜单 -->
      <template v-if="isDataPage">
        <el-menu-item index="realtime">实时</el-menu-item>
        <el-menu-item index="analysis">分析</el-menu-item>
        <el-menu-item index="history">历史数据</el-menu-item>
      </template>
      
      <!-- 设备页面菜单 -->
      <template v-else-if="isDevicePage">
        <el-menu-item index="overview">总览</el-menu-item>
        <el-menu-item index="management">管理</el-menu-item>
        <el-menu-item index="logs">日志</el-menu-item>
      </template>
    </el-menu>
  </el-aside>
</template>

<script lang="ts" setup>
import { computed, ref, watch } from 'vue';
import { useRoute } from 'vue-router';

const route = useRoute();
const activeTab = ref<string>('realtime');

// 暴露给父组件的事件
const emit = defineEmits(['tabChange']);

const isDataPage = computed(() => route.path === '/Stream');
const isDevicePage = computed(() => route.path === '/Device');

// 处理菜单选择
const handleMenuSelect = (key: string) => {
  activeTab.value = key;
  emit('tabChange', key);
};

// 监听路由变化，重置默认激活项
watch(() => route.path, (newPath) => {
  if (newPath === '/Stream') {
    activeTab.value = 'realtime';
  } else if (newPath === '/Device') {
    activeTab.value = 'overview';
  }
  emit('tabChange', activeTab.value);
}, { immediate: true });
</script>

<style lang="scss" scoped>
aside {
  position: sticky;
  left: 4%;
  top: 30%;
  // margin-left: 4%;
  // margin-top: 10%;
  width: 200px;
}
.menu {
  // display: flex;
  width: 70%;
  border-radius: 15px;
  text-align: center;
  justify-content: center;
}
</style>
