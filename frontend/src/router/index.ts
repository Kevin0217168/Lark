// 创建一个路由器

import { createRouter, createWebHistory } from "vue-router";
import Login from "@/components/Login.vue";
import Home from "@/components/Home.vue";
import Stream from "@/components/Stream.vue";

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
    }
  ],
});

export default router;
