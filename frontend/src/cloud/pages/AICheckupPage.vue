<template>
  <div class="ai-checkup-container">
    <div class="dashboard-header">
      <h1 class="page-title">AI 自检</h1>
      <p class="page-subtitle">智能图像分析与健康咨询服务</p>
    </div>

    <el-tabs v-model="activeTab" class="custom-tabs">
      <!-- 图像识别面板 -->
      <el-tab-pane label="图像识别" name="image">
        <el-row :gutter="24">
          <el-col :xs="24" :sm="24" :md="16" class="mb-6">
            <el-card class="styled-card upload-card" shadow="hover">
              <template #header>
                <div class="card-header">
                  <span class="header-title"><el-icon><Picture /></el-icon> 粪便/状态智能识别</span>
                </div>
              </template>
              
              <el-upload
                class="image-uploader"
                drag
                action="#"
                :auto-upload="false"
                :show-file-list="false"
                @change="handleImageChange"
              >
                <div v-if="imageUrl" class="preview-container">
                  <img :src="imageUrl" class="uploaded-image" />
                  <div class="upload-hover-mask">
                    <el-icon><ZoomIn /></el-icon>
                    <span>点击重新上传</span>
                  </div>
                </div>
                <div v-else class="upload-placeholder">
                  <el-icon class="el-icon--upload"><Camera /></el-icon>
                  <div class="el-upload__text">
                    将照片拖到此处，或 <em>点击上传</em>
                  </div>
                  <div class="el-upload__tip">
                    支持 jpg/png 格式，建议上传清晰的粪便或鸟类特写照片
                  </div>
                </div>
              </el-upload>

              <div class="action-bar" v-if="imageUrl">
                <el-button type="primary" class="analyze-btn" :loading="analyzing" @click="startAnalysis">
                  开始 AI 分析
                </el-button>
              </div>
            </el-card>
          </el-col>

          <el-col :xs="24" :sm="24" :md="8" class="mb-6">
            <el-card class="styled-card history-card" shadow="hover">
              <template #header>
                <div class="card-header">
                  <span class="header-title"><el-icon><Clock /></el-icon> 历史记录</span>
                </div>
              </template>
              
              <div class="history-list">
                <div v-for="(item, index) in historyRecords" :key="index" class="history-item">
                  <div class="history-date">{{ item.date }}</div>
                  <div class="history-content">
                    <div class="history-result" :class="item.status === 'normal' ? 'text-success' : 'text-warning'">
                      {{ item.result }}
                    </div>
                    <div class="history-desc">{{ item.desc }}</div>
                  </div>
                </div>
              </div>
            </el-card>
          </el-col>
        </el-row>

        <!-- 悬浮抓拍按钮 -->
        <div class="floating-btn" @click="handleSnapshot">
          <el-icon><CameraFilled /></el-icon>
          <span>鸟笼抓拍</span>
        </div>
      </el-tab-pane>

      <!-- AI 问诊面板 -->
      <el-tab-pane label="AI 问诊" name="chat">
        <el-card class="styled-card chat-card" shadow="hover">
          <div class="chat-header">
            <el-avatar :size="64" src="https://cube.elemecdn.com/0/88/03b0d39583f48206768a7534e55bcpng.png" class="robot-avatar" />
            <div class="welcome-text">
              <h3>你好，我是智云鹰康 AI 助手</h3>
              <p>请描述鸟儿的症状，或者选择下方的常见问题，我将为您提供专业的护理建议。</p>
            </div>
          </div>

          <div class="quick-questions">
            <h4 class="section-title">常见问题快捷提问</h4>
            <el-row :gutter="16">
              <el-col :xs="12" :sm="8" :md="6" v-for="(q, index) in quickQuestions" :key="index" class="mb-4">
                <div class="question-card" @click="askQuestion(q.text)">
                  <el-icon class="q-icon"><component :is="q.icon" /></el-icon>
                  <span class="q-text">{{ q.label }}</span>
                </div>
              </el-col>
            </el-row>
          </div>

          <el-divider border-style="dashed" />

          <div class="chat-input-area">
             <el-input
                v-model="chatInput"
                type="textarea"
                :rows="3"
                placeholder="例如：我家鹦鹉最近一直炸毛，而且食欲不好，是怎么回事？"
                resize="none"
              />
              <div class="input-actions">
                <el-button type="primary" class="send-btn" @click="askQuestion(chatInput)" :disabled="!chatInput.trim()">
                  发送咨询
                </el-button>
              </div>
          </div>
        </el-card>

        <!-- AI 回复弹窗模拟 -->
        <el-dialog
          v-model="dialogVisible"
          title="AI 诊断建议"
          width="500px"
          class="styled-dialog"
          :show-close="false"
        >
          <div class="ai-reply-content">
            <div class="user-query"><strong>您问：</strong>{{ currentQuery }}</div>
            <el-divider />
            <div class="ai-response" v-loading="replying" element-loading-text="AI 正在思考中...">
               <template v-if="!replying">
                 <div class="reply-text" v-html="currentReply"></div>
                 <div class="disclaimer">
                   <el-icon><Warning /></el-icon>
                   以上建议仅供参考，不能替代专业兽医的诊断。如果症状严重，请及时就医。
                 </div>
               </template>
            </div>
          </div>
          <template #footer>
            <span class="dialog-footer">
              <el-button type="primary" @click="dialogVisible = false" class="confirm-btn">我知道了</el-button>
            </span>
          </template>
        </el-dialog>
      </el-tab-pane>
    </el-tabs>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue';
import { ElMessage } from 'element-plus';
import { 
  Picture, Camera, ZoomIn, Clock, CameraFilled, 
  Warning, Chicken, FirstAidKit, Sunny, WindPower
} from '@element-plus/icons-vue';
import type { UploadFile } from 'element-plus';

const activeTab = ref('image');

// --- 图像识别逻辑 ---
const imageUrl = ref('');
const analyzing = ref(false);

const handleImageChange = (file: UploadFile) => {
  if (file.raw) {
    imageUrl.value = URL.createObjectURL(file.raw);
  }
};

const startAnalysis = () => {
  analyzing.value = true;
  setTimeout(() => {
    analyzing.value = false;
    ElMessage.success('分析完成，未发现明显异常。');
    // 添加到历史记录
    historyRecords.value.unshift({
      date: new Date().toLocaleString(),
      result: '状态正常',
      status: 'normal',
      desc: '排泄物形态和颜色均在正常范围内。'
    });
  }, 2500);
};

const handleSnapshot = () => {
  ElMessage({
    message: '正在调用摄像头进行抓拍...',
    type: 'success',
    icon: CameraFilled
  });
  // 模拟抓拍成功后上传图片
  setTimeout(() => {
    imageUrl.value = 'https://images.unsplash.com/photo-1552728089-571ed927bdf1?ixlib=rb-4.0.3&auto=format&fit=crop&w=800&q=80';
    ElMessage.success('抓拍成功，请点击分析。');
  }, 1500);
};

const historyRecords = ref([
  { date: '今天 10:23', result: '状态正常', status: 'normal', desc: '排泄物形态健康。' },
  { date: '昨天 14:05', result: '消化不良', status: 'warning', desc: '水分偏多，建议控制水分摄入，观察精神状态。' },
  { date: '前天 09:12', result: '状态正常', status: 'normal', desc: '无异常。' },
]);

// --- AI 问诊逻辑 ---
const chatInput = ref('');
const dialogVisible = ref(false);
const replying = ref(false);
const currentQuery = ref('');
const currentReply = ref('');

const quickQuestions = [
  { label: '食欲不振', text: '鸟儿最近食欲不振，不怎么吃东西怎么办？', icon: Chicken },
  { label: '异常掉毛', text: '鸟儿非换毛季大量掉毛，是怎么回事？', icon: Sunny },
  { label: '呼吸急促', text: '鸟儿张嘴呼吸、呼吸急促并伴有杂音，怎么办？', icon: WindPower },
  { label: '精神萎靡', text: '鸟儿整天闭着眼睛，炸毛，喜欢缩在角落，什么原因？', icon: FirstAidKit },
];

const askQuestion = (query: string) => {
  currentQuery.value = query;
  dialogVisible.value = true;
  replying.value = true;
  chatInput.value = '';
  
  // 模拟 AI 回复
  setTimeout(() => {
    replying.value = false;
    currentReply.value = `
      <p>针对您描述的情况（<strong>${query.substring(0, 10)}...</strong>），可能的原因如下：</p>
      <ul>
        <li>环境温度突变或受凉感冒。</li>
        <li>肠胃道感染或寄生虫问题。</li>
        <li>营养不均衡或缺乏特定维生素。</li>
      </ul>
      <p><strong>建议措施：</strong></p>
      <ol>
        <li>立即提供保暖，将环境温度提升至 28-30℃。</li>
        <li>观察排泄物是否异常（水便、绿便等），如有异常可使用本平台的【图像识别】功能进行辅助判断。</li>
        <li>喂食易消化的食物，并在饮水中添加适量电解质或维生素。</li>
      </ol>
    `;
  }, 1500);
};
</script>

<style scoped lang="scss">
.ai-checkup-container {
  padding: 30px;
  max-width: 1280px;
  margin: 0 auto;
  min-height: 100vh;
  background-color: #f9fafb;
  position: relative;
}

.dashboard-header {
  margin-bottom: 32px;
  border-left: 5px solid #8BAD42;
  padding-left: 20px;

  .page-title {
    font-size: 28px;
    font-weight: 700;
    color: #111827;
    margin: 0;
  }

  .page-subtitle {
    font-size: 15px;
    color: #6b7280;
    margin-top: 6px;
  }
}

.mb-6 { margin-bottom: 24px; }
.mb-4 { margin-bottom: 16px; }

/* 卡片基础样式 */
.styled-card {
  border: none;
  border-radius: 16px;
  transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
  box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -1px rgba(0, 0, 0, 0.06);

  &:hover {
    transform: translateY(-4px);
    box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05);
  }

  :deep(.el-card__header) {
    border-bottom: 1px solid #f3f4f6;
    padding: 18px 24px;
  }
}

.card-header {
  .header-title {
    font-size: 18px;
    font-weight: 600;
    color: #374151;
    display: flex;
    align-items: center;
    gap: 8px;
    .el-icon { color: #8BAD42; font-size: 20px; }
  }
}

/* Tabs 样式定制 */
.custom-tabs {
  :deep(.el-tabs__item) {
    font-size: 16px;
    font-weight: 500;
    &.is-active { color: #8BAD42; }
    &:hover { color: #8BAD42; }
  }
  :deep(.el-tabs__active-bar) {
    background-color: #8BAD42;
  }
  :deep(.el-tabs__nav-wrap::after) {
    height: 1px;
    background-color: #e5e7eb;
  }
}

/* 上传区域 */
.image-uploader {
  :deep(.el-upload) {
    width: 100%;
  }
  :deep(.el-upload-dragger) {
    padding: 40px;
    background-color: #fafafa;
    border: 2px dashed #d1d5db;
    border-radius: 12px;
    transition: all 0.3s;
    &:hover {
      border-color: #8BAD42;
      background-color: #f2f7e9;
    }
  }
}

.upload-placeholder {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  color: #6b7280;
  
  .el-icon--upload {
    font-size: 48px;
    color: #9ca3af;
    margin-bottom: 16px;
  }
  em {
    color: #8BAD42;
    font-style: normal;
    font-weight: 600;
  }
  .el-upload__tip {
    margin-top: 10px;
    font-size: 13px;
    color: #9ca3af;
  }
}

.preview-container {
  position: relative;
  width: 100%;
  height: 300px;
  border-radius: 12px;
  overflow: hidden;
  
  .uploaded-image {
    width: 100%;
    height: 100%;
    object-fit: cover;
  }
  
  .upload-hover-mask {
    position: absolute;
    top: 0; left: 0; right: 0; bottom: 0;
    background: rgba(0,0,0,0.5);
    color: white;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    opacity: 0;
    transition: opacity 0.3s;
    font-size: 16px;
    
    .el-icon { font-size: 32px; margin-bottom: 8px; }
  }
  
  &:hover .upload-hover-mask {
    opacity: 1;
  }
}

.action-bar {
  margin-top: 20px;
  text-align: center;
  
  .analyze-btn {
    background-color: #8BAD42;
    border-color: #8BAD42;
    padding: 12px 32px;
    font-size: 16px;
    border-radius: 8px;
    &:hover { background-color: #7a9a3a; border-color: #7a9a3a; }
  }
}

/* 历史记录 */
.history-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.history-item {
  padding: 12px;
  background-color: #f9fafb;
  border-radius: 8px;
  border-left: 4px solid #e5e7eb;
  
  &:hover { background-color: #f3f4f6; }
  
  .history-date {
    font-size: 12px;
    color: #9ca3af;
    margin-bottom: 6px;
  }
  .history-result {
    font-weight: 600;
    font-size: 15px;
    margin-bottom: 4px;
    &.text-success { color: #8BAD42; border-left-color: #8BAD42;}
    &.text-warning { color: #e6a23c; border-left-color: #e6a23c;}
  }
  .history-desc {
    font-size: 13px;
    color: #6b7280;
  }
}

.history-item:nth-child(1) { border-left-color: #8BAD42; }
.history-item:nth-child(2) { border-left-color: #e6a23c; }

/* 悬浮抓拍按钮 */
.floating-btn {
  position: fixed;
  bottom: 40px;
  right: 40px;
  background-color: #8BAD42;
  color: white;
  padding: 16px 24px;
  border-radius: 30px;
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 16px;
  font-weight: 600;
  cursor: pointer;
  box-shadow: 0 10px 15px -3px rgba(139, 173, 66, 0.4);
  transition: all 0.3s;
  z-index: 100;
  
  .el-icon { font-size: 24px; }
  
  &:hover {
    transform: translateY(-5px) scale(1.05);
    box-shadow: 0 20px 25px -5px rgba(139, 173, 66, 0.5);
  }
  &:active {
    transform: translateY(0) scale(0.95);
  }
}

/* AI 问诊 */
.chat-card {
  min-height: 500px;
  display: flex;
  flex-direction: column;
}

.chat-header {
  display: flex;
  align-items: center;
  gap: 20px;
  padding: 20px;
  background: linear-gradient(135deg, #f2f7e9 0%, #ffffff 100%);
  border-radius: 12px;
  margin-bottom: 24px;

  .robot-avatar {
    border: 3px solid white;
    box-shadow: 0 4px 6px rgba(0,0,0,0.1);
    background-color: #8BAD42;
  }
  
  .welcome-text {
    h3 { margin: 0 0 8px 0; color: #111827; font-size: 20px; }
    p { margin: 0; color: #4b5563; font-size: 14px; line-height: 1.5; }
  }
}

.quick-questions {
  margin-bottom: 24px;
  
  .section-title {
    font-size: 16px;
    color: #374151;
    margin-bottom: 16px;
    padding-left: 10px;
    border-left: 4px solid #8BAD42;
  }
}

.question-card {
  background-color: white;
  border: 1px solid #e5e7eb;
  border-radius: 12px;
  padding: 16px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 12px;
  cursor: pointer;
  transition: all 0.3s;
  height: 100%;
  
  .q-icon { font-size: 32px; color: #8BAD42; }
  .q-text { font-size: 14px; font-weight: 500; color: #4b5563; text-align: center;}
  
  &:hover {
    border-color: #8BAD42;
    background-color: #f2f7e9;
    transform: translateY(-3px);
    box-shadow: 0 4px 6px rgba(139, 173, 66, 0.1);
  }
}

.chat-input-area {
  margin-top: 20px;
  
  :deep(.el-textarea__inner) {
    border-radius: 12px;
    padding: 12px;
    font-size: 14px;
    &:focus { border-color: #8BAD42; box-shadow: 0 0 0 1px #8BAD42 inset; }
  }
  
  .input-actions {
    display: flex;
    justify-content: flex-end;
    margin-top: 12px;
    
    .send-btn {
      background-color: #8BAD42;
      border-color: #8BAD42;
      border-radius: 20px;
      padding: 8px 24px;
      &:hover { background-color: #7a9a3a; border-color: #7a9a3a; }
      &:disabled { background-color: #b0c48e; border-color: #b0c48e; }
    }
  }
}

/* 弹窗样式 */
.styled-dialog {
  border-radius: 16px !important;
  overflow: hidden;
  
  :deep(.el-dialog__header) {
    background-color: #8BAD42;
    margin-right: 0;
    padding: 16px 20px;
    .el-dialog__title { color: white; font-weight: 600; }
  }
  :deep(.el-dialog__body) {
    padding: 24px;
  }
}

.ai-reply-content {
  .user-query {
    font-size: 14px;
    color: #4b5563;
    background-color: #f3f4f6;
    padding: 12px;
    border-radius: 8px;
    margin-bottom: 16px;
    strong { color: #111827; }
  }
  
  .ai-response {
    min-height: 100px;
    font-size: 15px;
    color: #374151;
    line-height: 1.6;
    
    .reply-text {
      :deep(p) { margin-bottom: 10px; }
      :deep(ul), :deep(ol) { padding-left: 20px; margin-bottom: 10px; }
      :deep(li) { margin-bottom: 6px; }
    }
    
    .disclaimer {
      margin-top: 20px;
      font-size: 12px;
      color: #9ca3af;
      display: flex;
      align-items: flex-start;
      gap: 6px;
      background-color: #fefce8;
      color: #ca8a04;
      padding: 10px;
      border-radius: 6px;
      .el-icon { margin-top: 2px; }
    }
  }
}

.confirm-btn {
  background-color: #8BAD42;
  border-color: #8BAD42;
  &:hover { background-color: #7a9a3a; border-color: #7a9a3a; }
}

@media (max-width: 768px) {
  .floating-btn {
    bottom: 20px;
    right: 20px;
    padding: 12px 20px;
    font-size: 14px;
  }
  .chat-header {
    flex-direction: column;
    text-align: center;
  }
}
</style>
