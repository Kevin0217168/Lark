import { createRouter, createWebHistory } from 'vue-router';

const router = createRouter({
  history: createWebHistory(),
  routes: [
    {
      path: '/cloud/login',
      name: 'CloudLogin',
      component: () => import('./pages/LoginPage.vue'),
      meta: {
        title: '云养登录'
      }
    },
    {
      path: '/cloud/register',
      name: 'CloudRegister',
      component: () => import('./pages/RegisterPage.vue'),
      meta: {
        title: '云养注册'
      }
    },
    {
      path: '/cloud/home',
      name: 'CloudHome',
      component: () => import('./pages/CloudHomePage.vue'),
      meta: {
        title: '云养首页'
      }
    },
    {
      path: '/cloud/birds',
      name: 'CloudBirds',
      component: () => import('./pages/BirdsPage.vue'),
      meta: {
        title: '幼鸟列表'
      }
    },
    {
      path: '/cloud/my',
      name: 'CloudMy',
      component: () => import('./pages/MyPage.vue'),
      meta: {
        title: '我的云养'
      }
    },
    {
      path: '/cloud/bird/:id',
      name: 'CloudBirdDetail',
      component: () => import('./pages/BirdDetailPage.vue'),
      meta: {
        title: '幼鸟详情'
      }
    }
  ]
});

export default router;