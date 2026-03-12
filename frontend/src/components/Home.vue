<template>
  <el-card class="home-card">
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
