<template>
  <el-aside>
    <el-menu default-active="1" class="menu">
      <el-menu-item index="1">总览</el-menu-item>
      <el-menu-item index="2">数据</el-menu-item>
      <el-menu-item index="3">设备</el-menu-item>
    </el-menu>
  </el-aside>
</template>

<script lang="ts" setup></script>

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
