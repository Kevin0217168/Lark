<template>
  <div class="clinic-module">
    <!-- 顶部红色急诊 Banner -->
    <div class="emergency-banner mb-6">
      <div class="flex justify-between items-center bg-red-500 text-white p-6 rounded-2xl shadow-lg shadow-red-200">
        <div class="flex items-center gap-4">
          <div class="bg-white/20 p-3 rounded-full animate-pulse">
            <el-icon size="24"><PhoneFilled /></el-icon>
          </div>
          <div>
            <h2 class="text-xl font-bold">在线急诊通道</h2>
            <p class="text-sm opacity-90">响应时间 < 5 分钟，全国异宠专家实时待命</p>
          </div>
        </div>
        <el-button color="#ffffff" plain round class="!text-red-500 font-bold">立即连线</el-button>
      </div>
    </div>

    <el-row :gutter="24">
      <el-col :span="16">
        <!-- 搜索与筛选 -->
        <div class="flex gap-4 mb-6">
          <el-input
            v-model="search"
            placeholder="搜索医生姓名、专科或病症..."
            prefix-icon="Search"
            class="flex-1 shadow-sm"
          />
          <el-select v-model="filter" placeholder="全部分类" class="w-40">
            <el-option label="全部" value="all" />
            <el-option label="鸟类专科" value="bird" />
            <el-option label="爬行类" value="reptile" />
            <el-option label="两栖类" value="amphibian" />
          </el-select>
        </div>

        <!-- 医生列表 -->
        <div class="space-y-4">
          <el-card v-for="doc in doctors" :key="doc.name" class="doctor-card" shadow="hover">
            <div class="flex gap-6">
              <el-avatar :size="80" :src="doc.avatar" class="border-2 border-[#8BAD42]/20" />
              <div class="flex-1">
                <div class="flex justify-between items-start">
                  <div>
                    <h3 class="text-lg font-bold text-slate-700">{{ doc.name }}</h3>
                    <p class="text-sm text-[#8BAD42] font-semibold">{{ doc.title }}</p>
                  </div>
                  <div class="text-right">
                    <p class="text-amber-500 font-bold">⭐ {{ doc.rating }}</p>
                    <p class="text-xs text-slate-400">已问诊 {{ doc.cases }} 次</p>
                  </div>
                </div>
                <p class="text-sm text-slate-500 mt-2 line-clamp-1">{{ doc.desc }}</p>
                <div class="mt-4 flex justify-end gap-3">
                  <el-button size="small">医生主页</el-button>
                  <el-button size="small" type="primary" color="#8BAD42">立即预约</el-button>
                </div>
              </div>
            </div>
          </el-card>
        </div>
      </el-col>

      <el-col :span="8">
        <!-- 电子报告卡片 -->
        <el-card class="report-card mb-6" shadow="hover">
          <template #header>
            <div class="flex items-center gap-2 font-bold">
              <el-icon color="#8BAD42"><Document /></el-icon>
              电子健康报告与医嘱
            </div>
          </template>
          <div class="text-center py-4">
            <div class="text-xs text-slate-400 mb-2">最新报告日期：2026-04-28</div>
            <el-tag type="warning" class="mb-4">待执行医嘱：2 项</el-tag>
            <el-divider />
            <el-button link type="primary" color="#8BAD42">查看历史报告</el-button>
          </div>
        </el-card>

        <!-- 常见问题提示 -->
        <div class="bg-[#8BAD42]/10 p-6 rounded-2xl border border-[#8BAD42]/20">
          <h4 class="font-bold text-[#8BAD42] mb-3">问诊贴士</h4>
          <ul class="text-sm text-slate-600 space-y-2">
            <li>• 准备好宠物鸟的清晰照片</li>
            <li>• 记录下异常行为的具体时长</li>
            <li>• 如有排泄物异常，请勿清理</li>
          </ul>
        </div>
      </el-col>
    </el-row>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import { PhoneFilled, Document } from '@element-plus/icons-vue';

const search = ref('');
const filter = ref('all');

const doctors = [
  {
    name: '张伟华',
    title: '首席兽医 / 异宠专科',
    rating: '4.9',
    cases: '1258',
    avatar: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Felix',
    desc: '擅长鹦鹉类呼吸道感染及消化系统疾病诊治，拥有 15 年临床经验。',
  },
  {
    name: '李丽',
    title: '资深兽医 / 禽类病理学专家',
    rating: '4.8',
    cases: '856',
    avatar: 'https://api.dicebear.com/7.x/avataaars/svg?seed=Aneka',
    desc: '专注雏鸟发育期营养管理及常见寄生虫防治。',
  }
];
</script>

<style scoped>
.doctor-card {
  border-radius: 1rem;
  transition: transform 0.2s;
}
.doctor-card:hover {
  transform: scale(1.01);
}
</style>