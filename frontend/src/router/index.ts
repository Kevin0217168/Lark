// 创建一个路由器

import { createRouter, createWebHistory } from "vue-router";
import Login from "@/components/Login.vue";
import Home from "@/components/Home.vue";
import Stream from "@/components/Stream.vue";
import Device from "@/components/Device.vue";
import Data from "@/components/Data.vue";

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: "/Login",
      component: Login,
    },
    {
      path: "/Home",
      component: Home,
    },
    {
      path: "/Stream",
      component: Stream,
    },
    {
      path: "/Device",
      component: Device,
    },
    {
      path: "/Data",
      component: Data,
    }
  ],
});

export default router;
