<template>
  <el-row justify="center" style="width: 100%; margin: 0; padding: 0;">
    <el-col :span="18" style="margin: 0; padding: 0;">
      <el-row class="header-card">
        <el-col :span="3">
          <router-link to="/Home" custom v-slot="{ navigate }">
            <el-button @click="navigate" style="color: #8bad42" text>
              <el-icon size="30">
                <grid />
              </el-icon>
              <p style="margin-left: 5px">Lark</p>
            </el-button>
          </router-link>
        </el-col>
        <el-col :span="2" :offset="6">
          <router-link to="/Home" custom v-slot="{ navigate }">
            <el-button 
              @click="navigate" 
              :style="{ color: isActive('/Home') ? '#8bad42' : '#000' }" 
              :type="isActive('/Home') ? 'primary' : 'default'"
              text
            > 
              总览 
            </el-button>
          </router-link>
        </el-col>
        <el-col :span="2">
          <router-link to="/Stream" custom v-slot="{ navigate }">
            <el-button 
              @click="navigate" 
              :style="{ color: isActive('/Stream') ? '#8bad42' : '#000' }" 
              :type="isActive('/Stream') ? 'primary' : 'default'"
              text
            > 
              数据 
            </el-button>
          </router-link>
        </el-col>
        <el-col :span="2">
          <router-link to="/Device" custom v-slot="{ navigate }">
            <el-button 
              @click="navigate" 
              :style="{ color: isActive('/Device') ? '#8bad42' : '#000' }" 
              :type="isActive('/Device') ? 'primary' : 'default'"
              text
            > 
              设备 
            </el-button>
          </router-link>
        </el-col>
        <el-col :span="2" :offset="5">
          <router-link to="/Login" custom v-slot="{ navigate }">
            <el-button @click="navigate" color="#8bad42" plain>登录</el-button>
          </router-link>
        </el-col>
        <el-col :span="2">
          <el-button text> 注册 </el-button>
        </el-col>
      </el-row>
    </el-col>
  </el-row>
</template>

<script lang="ts" setup>
import { useRoute } from 'vue-router';

const route = useRoute();

const isActive = (path: string) => {
  return route.path === path;
};
</script>

<style lang="scss" scoped>
.header-card {
  height: 70px;
  z-index: 1000;
  text-align: center;
  align-items: center;
  border-bottom-left-radius: 15px;
  border-bottom-right-radius: 15px;
  background-color: #fff;
  box-shadow: 5px 5px 10px rgba(0, 0, 0, 0.1);
}
.el-button {
  height: 40px;
  font-size: 16px;
  text-align: center;
  align-items: center;
}
</style>
