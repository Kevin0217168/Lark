// 创建一个路由器

import { createRouter, createWebHistory } from "vue-router";
import LoginView from "@/views/LoginView.vue";
import RegisterView from "@/views/RegisterView.vue";
import Home from "@/components/HomePage.vue";
import Stream from "@/components/Stream.vue";
import Device from "@/components/DevicePage.vue";
import Data from "@/components/Data.vue";
import Profile from "@/components/ProfilePage.vue";
import UserManage from "@/components/UserManage.vue";
import BirdsPage from "@/components/BirdsPage.vue";
import CloudApp from "@/cloud/cloud_App.vue";

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
      path: "/air-quality",
      component: () => import('@/components/AirQualityDetail.vue'),
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
      path: "/birds",
      component: BirdsPage,
    },
    {
      path: "/cloud",
      component: CloudApp,
      children: [
        {
          path: '',
          redirect: '/cloud/login'
        },
        {
          path: 'login',
          component: () => import('@/cloud/pages/LoginPage.vue')
        },
        {
          path: 'register',
          component: () => import('@/cloud/pages/RegisterPage.vue')
        },
        {
          path: 'home',
          component: () => import('@/cloud/pages/CloudHomePage.vue')
        },
        {
          path: 'birds',
          component: () => import('@/cloud/pages/BirdsPage.vue')
        },
        {
          path: 'adopt-birds',
          component: () => import('@/cloud/pages/AdoptBirdsPage.vue')
        },
        {
          path: 'my',
          component: () => import('@/cloud/pages/MyPage.vue')
        },
        {
          path: 'bird/:id',
          component: () => import('@/cloud/pages/BirdDetailPage.vue')
        }
      ]
    },
    {
      path: "/",
      redirect: "/Login"
    }
  ],
});

// 路由守卫：检查登录状态和权限
router.beforeEach((to, from) => {
  const isAuthenticated = localStorage.getItem('isAuthenticated') === 'true';
  const userRole = localStorage.getItem('role') || '';

  // 检查是否是 cloud 路由
  const isCloudRoute = to.path.startsWith('/cloud');

  // 检查是否是 cloud 登录/注册页面
  const isCloudAuthPage = to.path === '/cloud/login' || to.path === '/cloud/register';

  // 检查是否是 clouduser 角色
  const isclouduser = userRole === 'clouduser';

  // clouduser 角色权限控制
  if (isclouduser) {
    if (isCloudRoute) {
      // clouduser 可以访问 cloud 路由
      return true;
    } else {
      // clouduser 尝试访问非 cloud 路由，阻止并显示提示
      alert('权限不足，您只能访问云养系统页面');
      // 重定向到 cloud 系统默认页面
      return '/cloud/my';
    }
  }
  // 管理后台路由的登录检查（排除 cloud 路由）
  else if (!isCloudRoute && to.path !== '/Login' && to.path !== '/Register' && !isAuthenticated) {
    return '/Login';
  }
  // 其他情况允许访问（包括未登录用户访问 /cloud/login 和 /cloud/register）
  return true;
});

export default router;
