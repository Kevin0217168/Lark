<template>
  <div class="ai-check-module">
    <el-tabs v-model="activeTab" class="ai-tabs">
      <!-- 图像识别视图 -->
      <el-tab-pane label="图像识别" name="image">
        <div class="max-w-4xl mx-auto py-10">
          <div 
            class="upload-area border-2 border-dashed border-slate-200 rounded-3xl p-16 flex flex-col items-center justify-center bg-white hover:border-[#8BAD42] transition-colors cursor-pointer group shadow-sm"
          >
            <div class="bg-[#8BAD42]/10 p-6 rounded-full group-hover:bg-[#8BAD42]/20 transition-colors mb-4">
              <el-icon :size="48" color="#8BAD42"><PictureFilled /></el-icon>
            </div>
            <h3 class="text-xl font-bold text-slate-700">点击上传或拍摄</h3>
            <p class="text-slate-400 mt-2">支持 JPG, PNG 格式，AI 将自动分析鸟粪便健康状态</p>
          </div>

          <div class="mt-12">
            <h4 class="text-lg font-bold text-slate-700 mb-6 flex items-center gap-2">
              <el-icon><List /></el-icon> 历史识别记录
            </h4>
            <div class="grid grid-cols-2 gap-4">
              <el-card v-for="item in history" :key="item.date" shadow="hover" class="!rounded-2xl">
                <div class="flex justify-between items-center">
                  <div class="flex items-center gap-4">
                    <div class="w-12 h-12 bg-slate-100 rounded-lg overflow-hidden">
                      <img :src="item.img" class="w-full h-full object-cover" />
                    </div>
                    <div>
                      <p class="text-sm font-bold text-slate-700">{{ item.status }}</p>
                      <p class="text-xs text-slate-400">{{ item.date }}</p>
                    </div>
                  </div>
                  <el-tag :type="item.tagType" size="small">{{ item.tag }}</el-tag>
                </div>
              </el-card>
            </div>
          </div>
        </div>

        <!-- 悬浮抓拍按钮 -->
        <div class="fixed bottom-10 right-10 z-50">
          <el-button 
            type="primary" 
            color="#8BAD42" 
            circle 
            class="!w-16 !h-16 shadow-xl shadow-[#8BAD42]/30 animate-bounce"
            @click="triggerCapture"
          >
            <el-icon :size="28"><Camera /></el-icon>
          </el-button>
          <div class="absolute -top-10 left-1/2 -translate-x-1/2 bg-slate-800 text-white text-[10px] px-2 py-1 rounded whitespace-nowrap">
            鸟笼实时抓拍
          </div>
        </div>
      </el-tab-pane>

      <!-- AI 预诊视图 -->
      <el-tab-pane label="AI 预诊" name="chat">
        <div class="chat-container bg-white rounded-3xl shadow-sm border border-slate-100 overflow-hidden flex flex-col h-[600px] max-w-4xl mx-auto">
          <!-- 聊天内容区 -->
          <div class="flex-1 p-6 overflow-y-auto space-y-6">
            <div class="flex gap-4">
              <div class="w-10 h-10 bg-[#8BAD42] rounded-full flex items-center justify-center text-white">
                <el-icon :size="24"><ChatDotRound /></el-icon>
              </div>
              <div class="bg-slate-100 p-4 rounded-2xl rounded-tl-none max-w-[80%]">
                <p class="text-sm text-slate-700 leading-relaxed">
                  您好！我是您的智能育鸟助手。我已经学习了超过 50,000 例异宠病例。您可以直接描述宠物鸟的异常表现，或点击下方的快捷问题。
                </p>
              </div>
            </div>
          </div>

          <!-- 快捷提问区 -->
          <div class="p-6 border-t border-slate-50">
            <p class="text-xs text-slate-400 mb-3">猜您想问：</p>
            <div class="flex flex-wrap gap-2 mb-4">
              <el-button 
                v-for="q in quickQuestions" 
                :key="q" 
                size="small" 
                round 
                class="!bg-slate-50 !border-slate-200 hover:!text-[#8BAD42] hover:!border-[#8BAD42]"
                @click="sendQuestion(q)"
              >
                {{ q }}
              </el-button>
            </div>
            <el-input
              v-model="input"
              placeholder="请输入您观察到的症状..."
              class="custom-input"
            >
              <template #append>
                <el-button type="primary" color="#8BAD42">发送</el-button>
              </template>
            </el-input>
          </div>
        </div>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import { PictureFilled, Camera, List, ChatDotRound } from '@element-plus/icons-vue';
import { ElMessage } from 'element-plus';

const activeTab = ref('image');
const input = ref('');

const history = [
  { status: '健康正常', date: '2026-04-29 14:20', tag: '正常', tagType: 'success', img: 'https://via.placeholder.com/100' },
  { status: '疑似消化不良', date: '2026-04-25 09:10', tag: '异常', tagType: 'warning', img: 'https://via.placeholder.com/100' },
];

const quickQuestions = [
  '我的鸟最近食欲不振',
  '粪便颜色异常偏白',
  '羽毛掉落严重',
  '总是缩在角落不动'
];

const triggerCapture = () => {
  ElMessage.success('正在调用智能鸟笼摄像头...');
};

const sendQuestion = (q: string) => {
  input.value = q;
};
</script>

<style scoped lang="scss">
.ai-tabs {
  :deep(.el-tabs__nav-wrap::after) {
    display: none;
  }
  :deep(.el-tabs__item.is-active) {
    color: #8BAD42;
    font-weight: 800;
    font-size: 1.1rem;
  }
  :deep(.el-tabs__active-bar) {
    background-color: #8BAD42;
    height: 3px;
    border-radius: 3px;
  }
}

.custom-input {
  :deep(.el-input-group__append) {
    background-color: #8BAD42;
    color: white;
    border: none;
    padding: 0 20px;
  }
}
</style>