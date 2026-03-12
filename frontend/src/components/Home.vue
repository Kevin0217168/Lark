<template>
  <el-card class="home-card">
    <!-- <h2>欢迎来到Lark设备管理系统</h2>
        <p>这是一个基于Vue3和Element Plus构建的设备管理系统，提供了设备的总览、数据分析和设备管理功能。</p>
        <p>请使用上方的导航栏进行操作，或者点击下面的按钮进入登录页面。</p>
        <router-link to="/Login" custom v-slot="{ navigate }">
            <el-button @click="navigate" color="#8bad42" plain>登录</el-button>
        </router-link> -->
    <el-row>
      <el-col :span="9">
        <el-card class="summary">设备总览</el-card>
      </el-col>
      <el-col :span="14" :offset="1">
        <el-card class="grid-preview">设备总览</el-card>
      </el-col>
    </el-row>
    <el-row>
      <el-col :span="9">
        <el-card class="summary">设备总览</el-card>
      </el-col>
      <el-col :span="14" :offset="1">
        <el-card class="grid-preview">设备总览</el-card>
      </el-col>
    </el-row>
  </el-card>
</template>

<script lang="ts" setup></script>

<style lang="scss" scoped>
.home-card {
  z-index: 1;
  width: 75%;
  margin-top: 1%;
  border-radius: 15px;
  text-align: center;
  background-color: #fff;
}

.summary {
  height: 300px;
  border-radius: 15px;
  // background-color: ;
}
.grid-preview {
  height: 300px;
  border-radius: 15px;
}
</style>
