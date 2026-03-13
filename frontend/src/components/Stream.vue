<template>
  <el-card class="card">
    <el-row>
      <el-col :span="8">
        <el-text>
          {{ text }}
        </el-text>
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
  </el-card>
</template>

<script lang="ts" setup>
import { ref } from "vue";
import axios from "axios";

const text = ref("");
const device_options = ref();
const value = ref();

axios
  .get("/fastapi/?name=Vue.js")
  .then(function (response) {
    text.value = response.data;
  })
  .catch(function (err) {
    console.error(err);
  });

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
  height: 600px;
  border-radius: 15px;
}
</style>
