// 创建一个路由器

import { createRouter, createWebHistory } from "vue-router";
import LoginView from "@/views/LoginView.vue";
import RegisterView from "@/views/RegisterView.vue";
import Home from "@/components/HomePage.vue";
import Stream from "@/components/Stream.vue";
import Device from "@/components/DevicePage.vue";
import Data from "@/components/Data.vue";
import Profile from "@/components/Profile.vue";
import UserManage from "@/components/UserManage.vue";

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: "/Login",
      component: LoginView,
    },
    {
      path: "/Register",
      component: RegisterView,
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
      props: (route) => ({ activeTab: route.query.activeTab || 'realtime' })
    },
    {
      path: "/Profile",
      component: Profile,
    },
    {
      path: "/UserManage",
      component: UserManage,
    },
    {
      path: "/",
      redirect: "/Login"
    }
  ],
});

// 路由守卫：检查登录状态
router.beforeEach((to, from, next) => {
  const isAuthenticated = localStorage.getItem('isAuthenticated') === 'true';
  
  // 如果访问的不是登录页面和注册页面，且未登录，重定向到登录页面
  if (to.path !== '/Login' && to.path !== '/Register' && !isAuthenticated) {
    next('/Login');
  } else {
    next();
  }
});

export default router;
