<template>
  <div class="ai-page">
    <!-- 顶部导航 -->
    <div class="tab-header">
      <div 
        class="tab-item" 
        :class="{ active: activeTab === 'image' }" 
        @click="activeTab = 'image'"
      >
        <el-icon><Picture /></el-icon>
        <span>图像识别</span>
      </div>
      <div 
        class="tab-item" 
        :class="{ active: activeTab === 'chat' }" 
        @click="activeTab = 'chat'"
      >
        <el-icon><ChatDotRound /></el-icon>
        <span>智能问诊</span>
      </div>
    </div>

    <!-- 图像识别内容 -->
    <div v-if="activeTab === 'image'" class="image-section">
      <div class="upload-card">
        <div class="preview-area" v-if="uploadedImage">
          <img :src="uploadedImage" alt="预览" class="preview-img" />
          <div class="re-upload" @click="handleUpload">重新拍摄</div>
        </div>
        <div class="upload-placeholder" v-else @click="handleUpload">
          <el-icon class="upload-icon"><Plus /></el-icon>
          <p>上传粪便照片或拍摄</p>
          <span class="tip">AI将分析鸟类健康状况</span>
        </div>
      </div>

      <div class="hardware-trigger">
        <el-button type="success" plain round @click="triggerCageCapture" :loading="isCapturing">
          <el-icon><Camera /></el-icon> 触发智能鸟笼抓拍
        </el-button>
        <p class="hardware-tip">自动调用鸟笼底盘摄像头进行抓拍</p>
      </div>

      <div class="action-btn-wrap">
        <el-button 
          type="primary" 
          class="analyze-btn" 
          :disabled="!uploadedImage" 
          :loading="isAnalyzing"
          @click="analyzeImage"
        >
          {{ isAnalyzing ? 'AI深度分析中...' : '开始AI分析' }}
        </el-button>
      </div>

      <!-- 分析结果 -->
      <transition name="fade">
        <div class="result-card" v-if="analysisResult">
          <div class="result-header">
            <span class="result-title">分析报告</span>
            <el-tag :type="getTagType(analysisResult.status)" effect="dark">
              {{ analysisResult.diagnosis }}
            </el-tag>
          </div>
          <div class="result-body">
            <div class="result-item">
              <span class="label">检测形态：</span>
              <span class="value">{{ analysisResult.shape }}</span>
            </div>
            <div class="result-item">
              <span class="label">颜色分析：</span>
              <span class="value">{{ analysisResult.color }}</span>
            </div>
            <div class="result-item">
              <span class="label">异常检测：</span>
              <span class="value">{{ analysisResult.abnormal }}</span>
            </div>
            <div class="result-item">
              <span class="label">AI可信度：</span>
              <span class="value">{{ analysisResult.confidence }}%</span>
            </div>
            <el-divider />
            <div class="suggestion-box">
              <div class="suggestion-title">💡 专家建议</div>
              <p class="suggestion-text">{{ analysisResult.suggestion }}</p>
            </div>
          </div>
        </div>
      </transition>

      <!-- 历史记录 -->
      <div class="history-section">
        <div class="section-header">
          <h3>历史分析记录</h3>
          <el-button link type="primary">查看全部</el-button>
        </div>
        <div class="history-list">
          <div class="history-item" v-for="item in analysisHistory" :key="item.id">
            <div class="history-info">
              <div class="history-diag">{{ item.diagnosis }}</div>
              <div class="history-time">{{ item.time }}</div>
            </div>
            <el-tag :type="getTagType(item.status)" size="small">{{ item.status === 'normal' ? '健康' : '异常' }}</el-tag>
          </div>
        </div>
      </div>
    </div>

    <!-- 智能问诊聊天内容 -->
    <div v-else class="chat-section">
      <div class="chat-container" ref="chatScroll">
        <div class="welcome-msg">
          <div class="ai-avatar">🤖</div>
          <div class="bubble">
            您好！我是您的智能育鸟助手。您可以描述宠物鸟的症状，或选择下方的快捷问题进行咨询。
          </div>
        </div>

        <div v-for="msg in messages" :key="msg.id" :class="['message-item', msg.role]">
          <div class="avatar">{{ msg.role === 'ai' ? '🤖' : '👤' }}</div>
          <div class="bubble">{{ msg.content }}</div>
        </div>

        <div v-if="isTyping" class="message-item ai">
          <div class="avatar">🤖</div>
          <div class="bubble typing">正在思考...</div>
        </div>
      </div>

      <div class="chat-footer">
        <div class="quick-questions">
          <div 
            v-for="q in quickQuestions" 
            :key="q" 
            class="q-tag"
            @click="sendQuickQuestion(q)"
          >
            {{ q }}
          </div>
        </div>
        <div class="input-wrap">
          <el-input
            v-model="inputMessage"
            placeholder="描述您的问题..."
            @keyup.enter="sendMessage"
          >
            <template #append>
              <el-button @click="sendMessage">发送</el-button>
            </template>
          </el-input>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, nextTick, onMounted } from 'vue';
import { Picture, ChatDotRound, Plus, Camera } from '@element-plus/icons-vue';
import { ElMessage } from 'element-plus';

const activeTab = ref('image');
const uploadedImage = ref('');
const isAnalyzing = ref(false);
const isCapturing = ref(false);
const analysisResult = ref<any>(null);
const analysisTime = ref('');

const analysisHistory = ref([
  { id: 1, diagnosis: '正常', status: 'normal', time: '2026/2/21 14:30' },
  { id: 2, diagnosis: '消化不良', status: 'warning', time: '2026/2/20 09:15' }
]);

const messages = ref<any[]>([]);
const inputMessage = ref('');
const isTyping = ref(false);
const chatScroll = ref<any>(null);
const messageId = ref(0);

const quickQuestions = [
  '我的鸟最近食欲不振',
  '粪便颜色异常偏白',
  '羽毛掉落严重',
  '活动量明显减少',
  '呼吸时有异常声音'
];

const handleUpload = () => {
  // 模拟上传
  const input = document.createElement('input');
  input.type = 'file';
  input.accept = 'image/*';
  input.onchange = (e: any) => {
    const file = e.target.files[0];
    if (file) {
      const reader = new FileReader();
      reader.onload = (event: any) => {
        uploadedImage.value = event.target.result;
        analysisResult.value = null;
      };
      reader.readAsDataURL(file);
    }
  };
  input.click();
};

const triggerCageCapture = () => {
  isCapturing.value = true;
  setTimeout(() => {
    isCapturing.value = false;
    ElMessage.success('抓拍成功！');
    uploadedImage.value = 'https://images.unsplash.com/photo-1552728089-571ed927bdf1?q=80&w=1000&auto=format&fit=crop';
    analysisResult.value = null;
  }, 2000);
};

const analyzeImage = () => {
  isAnalyzing.value = true;
  setTimeout(() => {
    isAnalyzing.value = false;
    const results = [
      {
        status: 'normal',
        diagnosis: '健康正常',
        shape: '形态完整',
        color: '深褐色',
        abnormal: '无异常',
        confidence: 96,
        suggestion: '您的宠物鸟粪便状态正常，继续保持良好的饲养环境和饮食习惯。'
      },
      {
        status: 'warning',
        diagnosis: '轻微消化不良',
        shape: '略微稀软',
        color: '浅褐色',
        abnormal: '水分略多',
        confidence: 88,
        suggestion: '建议减少高脂肪食物摄入，增加易消化的谷物比例，保持充足饮水。如症状持续，请咨询兽医。'
      },
      {
        status: 'danger',
        diagnosis: '疑似细菌感染',
        shape: '稀烂',
        color: '灰白色',
        abnormal: '颜色异常、形态异常',
        confidence: 92,
        suggestion: '检测到粪便异常，可能是细菌感染或消化道疾病。建议立即咨询专业兽医，暂停日常喂食，并隔离观察。'
      }
    ];
    
    const result = results[Math.floor(Math.random() * results.length)];
    analysisResult.value = result;
    
    const now = new Date();
    const timeStr = `${now.getMonth() + 1}/${now.getDate()} ${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`;
    analysisTime.value = timeStr;
    
    analysisHistory.value.unshift({
      id: Date.now(),
      diagnosis: result.diagnosis,
      status: result.status,
      time: timeStr
    });
    analysisHistory.value = analysisHistory.value.slice(0, 5);
  }, 3000);
};

const getTagType = (status: string) => {
  if (status === 'normal') return 'success';
  if (status === 'warning') return 'warning';
  return 'danger';
};

const sendMessage = () => {
  const content = inputMessage.value.trim();
  if (!content) return;
  
  addMessage('user', content);
  inputMessage.value = '';
  
  isTyping.value = true;
  setTimeout(() => {
    isTyping.value = false;
    const response = getAIResponse(content);
    addMessage('ai', response);
  }, 1500);
};

const sendQuickQuestion = (q: string) => {
  addMessage('user', q);
  isTyping.value = true;
  setTimeout(() => {
    isTyping.value = false;
    const response = getAIResponse(q);
    addMessage('ai', response);
  }, 1200);
};

const addMessage = (role: string, content: string) => {
  messages.value.push({
    id: ++messageId.value,
    role,
    content,
    time: new Date().toLocaleTimeString()
  });
  nextTick(() => {
    if (chatScroll.value) {
      chatScroll.value.scrollTop = chatScroll.value.scrollHeight;
    }
  });
};

const getAIResponse = (content: string) => {
  const responses: Record<string, string> = {
    '食欲': '食欲不振可能是多种原因导致的，如环境压力、疾病、食物问题等。建议您：1）检查环境温度是否适宜（22-28℃）；2）观察是否有其他症状如精神萎靡、羽毛蓬松；3）提供易消化的食物；4）如症状持续超过24小时，建议咨询专业兽医。',
    '粪便颜色异常偏白': '粪便颜色偏白可能提示肝脏或肾脏问题，或者是尿酸沉积。建议您：1）持续观察几天，记录其他症状；2）检查饮食是否过于单一；3）确保充足饮水；4）如果伴随精神状态不佳，请立即联系兽医进行血液检查。',
    '羽毛掉落': '正常换毛期会有羽毛脱落，但如果掉落严重可能是以下原因：1）营养缺乏，特别是蛋白质和维生素；2）寄生虫感染；3）压力或环境变化；4）激素失调。建议检查饲养环境，提供营养均衡的食物，如情况严重请咨询兽医。',
    '活动量减少': '活动量减少可能预示健康问题，可能原因包括：1）疾病或感染；2）温度不适；3）环境压力；4）营养不良。建议：观察鸟儿是否有其他症状如呼吸急促、眼部异常，监测环境温度，如持续超过24小时不进食或不动，请立即就医。',
    '呼吸异常': '呼吸异常是严重症状，可能原因包括：1）呼吸道感染；2）肺炎或气胸；3）异物堵塞；4）过敏反应。建议立即隔离观察，保持环境温暖通风，并尽快联系专业兽医进行急诊处理。'
  };
  
  for (const key in responses) {
    if (content.includes(key)) return responses[key];
  }
  
  return `根据您的描述，这可能是多种因素导致的。建议您：\n\n1）观察宠物鸟的整体状态\n2）检查饲养环境\n3）查看是否有其他异常症状\n4）如症状持续或加重，建议咨询专业兽医进行详细检查。`;
};
</script>

<style scoped lang="scss">
.ai-page {
  display: flex;
  flex-direction: column;
  height: 100vh;
  background-color: #f5f7fa;
}

.tab-header {
  display: flex;
  background-color: #fff;
  padding: 10px 0;
  box-shadow: 0 2px 8px rgba(0,0,0,0.05);
  
  .tab-item {
    flex: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 4px;
    font-size: 14px;
    color: #909399;
    cursor: pointer;
    transition: all 0.3s;
    
    .el-icon {
      font-size: 20px;
    }
    
    &.active {
      color: #8BAD42;
      font-weight: 600;
    }
  }
}

.image-section {
  padding: 20px;
  overflow-y: auto;
  flex: 1;
}

.upload-card {
  background-color: #fff;
  border-radius: 12px;
  height: 200px;
  display: flex;
  align-items: center;
  justify-content: center;
  border: 2px dashed #dcdfe6;
  margin-bottom: 20px;
  overflow: hidden;
  
  .upload-placeholder {
    text-align: center;
    color: #909399;
    
    .upload-icon {
      font-size: 40px;
      margin-bottom: 10px;
    }
    
    p { margin: 0; font-size: 16px; font-weight: 500; }
    .tip { font-size: 12px; opacity: 0.7; }
  }
  
  .preview-area {
    width: 100%;
    height: 100%;
    position: relative;
    
    .preview-img {
      width: 100%;
      height: 100%;
      object-fit: contain;
    }
    
    .re-upload {
      position: absolute;
      bottom: 10px;
      right: 10px;
      background: rgba(0,0,0,0.5);
      color: #fff;
      padding: 4px 12px;
      border-radius: 20px;
      font-size: 12px;
    }
  }
}

.hardware-trigger {
  text-align: center;
  margin-bottom: 20px;
  
  .hardware-tip {
    font-size: 12px;
    color: #909399;
    margin-top: 8px;
  }
}

.action-btn-wrap {
  margin-bottom: 30px;
  .analyze-btn {
    width: 100%;
    height: 48px;
    font-size: 16px;
    background-color: #8BAD42;
    border-color: #8BAD42;
    
    &:disabled {
      background-color: #c2d69b;
      border-color: #c2d69b;
    }
  }
}

.result-card {
  background-color: #fff;
  border-radius: 12px;
  padding: 20px;
  margin-bottom: 30px;
  box-shadow: 0 4px 12px rgba(0,0,0,0.05);
  
  .result-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
    
    .result-title {
      font-size: 18px;
      font-weight: 600;
      color: #303133;
    }
  }
  
  .result-item {
    margin-bottom: 12px;
    font-size: 14px;
    
    .label { color: #909399; }
    .value { color: #303133; font-weight: 500; }
  }
  
  .suggestion-box {
    background-color: #f0f9eb;
    padding: 15px;
    border-radius: 8px;
    
    .suggestion-title {
      color: #67c23a;
      font-weight: 600;
      margin-bottom: 8px;
    }
    
    .suggestion-text {
      font-size: 13px;
      color: #606266;
      line-height: 1.6;
      margin: 0;
    }
  }
}

.history-section {
  .section-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 15px;
    
    h3 { margin: 0; font-size: 16px; color: #303133; }
  }
  
  .history-list {
    display: flex;
    flex-direction: column;
    gap: 12px;
    
    .history-item {
      background-color: #fff;
      padding: 12px 15px;
      border-radius: 8px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      
      .history-diag { font-size: 14px; font-weight: 500; color: #303133; }
      .history-time { font-size: 12px; color: #909399; margin-top: 4px; }
    }
  }
}

.chat-section {
  display: flex;
  flex-direction: column;
  height: calc(100vh - 60px);
}

.chat-container {
  flex: 1;
  padding: 20px;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.message-item {
  display: flex;
  gap: 12px;
  max-width: 85%;
  
  .avatar {
    width: 36px;
    height: 36px;
    background-color: #e4e7ed;
    border-radius: 50%;
    display: flex;
    align-items: center;
    justify-content: center;
    font-size: 20px;
    flex-shrink: 0;
  }
  
  .bubble {
    padding: 12px 16px;
    border-radius: 12px;
    font-size: 14px;
    line-height: 1.5;
    position: relative;
    white-space: pre-wrap;
  }
  
  &.ai {
    align-self: flex-start;
    .bubble {
      background-color: #fff;
      color: #303133;
      border-top-left-radius: 0;
      box-shadow: 0 2px 8px rgba(0,0,0,0.05);
    }
    .typing { color: #909399; font-style: italic; }
  }
  
  &.user {
    align-self: flex-end;
    flex-direction: row-reverse;
    .bubble {
      background-color: #8BAD42;
      color: #fff;
      border-top-right-radius: 0;
    }
  }
}

.welcome-msg {
  display: flex;
  gap: 12px;
  margin-bottom: 10px;
  .ai-avatar { font-size: 24px; }
  .bubble {
    background-color: #e1f3d8;
    color: #67c23a;
    padding: 12px;
    border-radius: 8px;
    font-size: 13px;
  }
}

.chat-footer {
  background-color: #fff;
  padding: 15px;
  border-top: 1px solid #ebeef5;
  
  .quick-questions {
    display: flex;
    overflow-x: auto;
    gap: 8px;
    margin-bottom: 12px;
    padding-bottom: 5px;
    
    &::-webkit-scrollbar { display: none; }
    
    .q-tag {
      background-color: #f0f2f5;
      padding: 6px 12px;
      border-radius: 15px;
      font-size: 12px;
      color: #606266;
      white-space: nowrap;
      cursor: pointer;
      
      &:hover { background-color: #e4e7ed; }
    }
  }
}

.fade-enter-active, .fade-leave-active {
  transition: opacity 0.5s;
}
.fade-enter-from, .fade-leave-to {
  opacity: 0;
}
</style>