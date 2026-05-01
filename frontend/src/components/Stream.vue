<template>
  <el-card class="card">
    <h3 class="title">数据管理</h3>
    
    <!-- 实时数据 -->
    <div v-if="activeTab === 'realtime'">
      <h4>实时数据</h4>
      <el-row>
        <el-col :span="8">
          <el-text>{{ text }}</el-text>
          <el-select v-model="value" placeholder="Select" style="width: 240px">
            <el-option
              v-for="item in device_options"
              :key="item.id"
              :label="item.name"
              :value="item.id"
              :disabled="item.status==`offline`"
            />
          </el-select>
        </el-col>
        <el-col :offset="2" :span="14">
          <el-card> </el-card>
        </el-col>
      </el-row>
    </div>
    
    <!-- 分析 -->
    <div v-else-if="activeTab === 'analysis'">
      <h4>数据分析</h4>
      <p>数据分析功能待实现</p>
    </div>
    
    <!-- 历史数据 -->
    <div v-else-if="activeTab === 'history'">
      <h4>历史数据</h4>
      <p>历史数据功能待实现</p>
    </div>
  </el-card>
</template>

<script lang="ts" setup>
import { ref, onMounted } from "vue";
import axios from "axios";

// 接收从父组件传递的 activeTab 属性
defineProps<{
  activeTab: string;
}>();

const text = ref("");
const device_options = ref();
const value = ref();

// 获取欢迎信息
axios
  .get("/fastapi/?name=Vue.js")
  .then(function (response) {
    text.value = response.data;
  })
  .catch(function (err) {
    console.error(err);
  });

// 获取设备列表
axios.get("/fastapi/device/list")
  .then(function (response) {
    device_options.value = response.data.devices
    console.log(device_options)
  })
  .catch(function (err) {
    console.error(err);
  });
</script>

<style lang="scss" scoped>
.card {
  z-index: 1;
  width: 75%;
  min-height: 600px;
  border-radius: 16px;
  padding: 24px;
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(12px);
  -webkit-backdrop-filter: blur(12px);
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(255, 255, 255, 0.6);
}

.title {
  text-align: center;
  margin-bottom: 20px;
}
</style>
