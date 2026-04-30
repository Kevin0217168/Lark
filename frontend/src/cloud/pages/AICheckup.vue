<template>
  <div class="page-container">
    <!-- 自定义导航栏 -->
    <div class="custom-navbar">
      <div class="navbar-content">
        <span class="navbar-title">伴鸟 · AI自检</span>
        <div class="ai-badge">
          <span class="badge-icon">🤖</span>
          <span class="badge-text">AI智能</span>
        </div>
      </div>
    </div>

    <!-- 功能切换Tab -->
    <div class="function-tabs">
      <div 
        :class="['tab-item', activeTab === 'image' ? 'active' : '']" 
        @click="activeTab = 'image'"
      >
        <span class="tab-icon">📸</span>
        <span class="tab-text">图像识别</span>
      </div>
      <div 
        :class="['tab-item', activeTab === 'chat' ? 'active' : '']" 
        @click="activeTab = 'chat'"
      >
        <span class="tab-icon">💬</span>
        <span class="tab-text">AI预诊</span>
      </div>
    </div>

    <!-- 图像识别模块 -->
    <div class="image-section" v-if="activeTab === 'image'">
      <div class="image-card">
        <!-- 原生文件上传隐藏输入框 -->
        <input type="file" accept="image/*" ref="fileInput" style="display: none" @change="onFileChange" />

        <!-- 上传区域 -->
        <div class="upload-area" @click="triggerUpload" v-if="!uploadedImage">
          <div class="upload-placeholder">
            <div class="upload-icon">📷</div>
            <span class="upload-text">点击拍摄或上传粪便图像</span>
            <span class="upload-tip">支持jpg/png格式,建议清晰拍摄</span>
          </div>
        </div>

        <!-- 预览区域 -->
        <div class="preview-area" v-else>
          <img class="preview-image" :src="uploadedImage" style="object-fit: cover;" />
          <div class="reupload-btn" @click="triggerUpload">
            <span>🔄 重新选择</span>
          </div>
        </div>

        <!-- 分析按钮 -->
        <button 
          :class="['analyze-btn', isAnalyzing ? 'analyzing' : '']" 
          @click="analyzeImage"
          :disabled="isAnalyzing || !uploadedImage"
        >
          <span v-if="!isAnalyzing">🔍 开始AI分析</span>
          <span v-else>🔄 分析中...</span>
        </button>

        <!-- 分析结果 -->
        <div class="result-section" v-if="analysisResult">
          <div class="result-header">
            <span class="result-title">AI分析报告</span>
            <div class="result-time">{{analysisTime}}</div>
          </div>

          <div :class="['result-card', 'result-' + analysisResult.status]">
            <div class="result-icon">
              <span v-if="analysisResult.status === 'normal'">✓</span>
              <span v-else-if="analysisResult.status === 'warning'">⚠️</span>
              <span v-else>⛔</span>
            </div>
            <div class="result-content">
              <span class="result-label">诊断结果</span>
              <span class="result-value">{{analysisResult.diagnosis}}</span>
            </div>
            <div class="result-confidence">
              <span class="confidence-label">置信度</span>
              <span class="confidence-value">{{analysisResult.confidence}}%</span>
            </div>
          </div>

          <div class="detail-section">
            <div class="detail-title">详细分析</div>
            <div class="detail-content">
              <div class="detail-item">
                <span class="detail-label">粪便形态</span>
                <span class="detail-value">{{analysisResult.shape}}</span>
              </div>
              <div class="detail-item">
                <span class="detail-label">颜色特征</span>
                <span class="detail-value">{{analysisResult.color}}</span>
              </div>
              <div class="detail-item">
                <span class="detail-label">异常指标</span>
                <span class="detail-value">{{analysisResult.abnormal}}</span>
              </div>
            </div>
          </div>

          <div class="suggestion-section" v-if="analysisResult.suggestion">
            <div class="suggestion-title">💡 AI建议</div>
            <span class="suggestion-text">{{analysisResult.suggestion}}</span>
          </div>

          <button class="consult-btn" @click="gotoConsult" v-if="analysisResult.status !== 'normal'">
            <span>👨‍⚕️ 立即咨询专业兽医</span>
          </button>
        </div>

        <!-- 历史记录 -->
        <div class="history-section" v-if="analysisHistory.length > 0">
          <div class="section-header">
            <span class="section-title">历史记录</span>
            <span class="section-more" @click="showAllHistory">查看全部 ></span>
          </div>
          <div class="history-list">
            <div 
              class="history-item" 
              v-for="item in analysisHistory" 
              :key="item.id"
              @click="viewHistory(item.id)"
            >
              <div :class="['history-icon', 'history-' + item.status]">
                <span v-if="item.status === 'normal'">✓</span>
                <span v-else-if="item.status === 'warning'">⚠️</span>
                <span v-else>⛔</span>
              </div>
              <div class="history-content">
                <span class="history-diagnosis">{{item.diagnosis}}</span>
                <span class="history-time">{{item.time}}</span>
              </div>
              <div class="history-arrow">></div>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- AI预诊模块 -->
    <div class="chat-section" v-if="activeTab === 'chat'">
      <div class="chat-container">
        <!-- 欢迎消息 -->
        <div class="chat-welcome" v-if="messages.length === 0">
          <div class="welcome-avatar">🤖</div>
          <div class="welcome-title">伴鸟AI预诊助手</div>
          <span class="welcome-desc">我是您的智能预诊助手,请描述您宠物鸟的症状,我会为您提供初步诊断建议</span>
          
          <div class="quick-questions">
            <div class="question-title">常见问题</div>
            <div class="question-list">
              <div 
                class="question-item" 
                v-for="(item, index) in quickQuestions" 
                :key="index"
                @click="sendQuickQuestion(item)"
              >
                <span>{{item}}</span>
              </div>
            </div>
          </div>
        </div>

        <!-- 对话消息列表 (带自动滚动) -->
        <div 
          class="chat-messages" 
          ref="chatContainer"
          v-show="messages.length > 0 || isTyping"
        >
          <div 
            :class="['message-wrapper', 'message-' + item.role]" 
            v-for="item in messages" 
            :key="item.id"
          >
            <div class="message-avatar" v-if="item.role === 'ai'">
              <span>🤖</span>
            </div>
            <div class="message-content-wrapper">
              <div :class="['message-bubble', 'message-' + item.role + '-bubble']">
                <span class="message-text" style="white-space: pre-wrap;">{{item.content}}</span>
              </div>
              <span class="message-time">{{item.time}}</span>
            </div>
            <div class="message-avatar" v-if="item.role === 'user'">
              <span>🐦</span>
            </div>
          </div>
          
          <!-- AI正在输入 -->
          <div class="message-wrapper message-ai" v-if="isTyping">
            <div class="message-avatar">
              <span>🤖</span>
            </div>
            <div class="message-content-wrapper">
              <div class="message-bubble message-ai-bubble typing">
                <div class="typing-dots">
                  <div class="dot"></div>
                  <div class="dot"></div>
                  <div class="dot"></div>
                </div>
              </div>
            </div>
          </div>
        </div>

        <!-- 输入区域 -->
        <div class="chat-input-area">
          <input 
            class="chat-input" 
            placeholder="描述宠物鸟的症状..."
            v-model="inputMessage"
            @keyup.enter="sendMessage"
          />
          <div 
            :class="['send-btn', inputMessage.trim() ? 'active' : '']" 
            @click="sendMessage"
          >
            <span>发送</span>
          </div>
        </div>
      </div>
    </div>
    
    <!-- 智能鸟笼悬浮抓拍按钮 -->
    <div class="capture-fab" @click="triggerCageCapture" v-if="activeTab === 'image'">
      <div class="capture-icon">📸</div>
      <span class="capture-text">鸟笼抓拍</span>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, nextTick } from 'vue'
import { ElMessage, ElLoading } from 'element-plus'
import { useRouter } from 'vue-router'

const router = useRouter()

// ===== 状态响应式全面升级 =====
const activeTab = ref('image')

// 图像识别相关
const fileInput = ref<HTMLInputElement | null>(null)
const uploadedImage = ref('')
const isAnalyzing = ref(false)
const analysisResult = ref<any>(null)
const analysisTime = ref('')

const analysisHistory = ref([
  { id: 1, diagnosis: '正常', status: 'normal', time: '2026/2/21 14:30' },
  { id: 2, diagnosis: '消化不良', status: 'warning', time: '2026/2/20 09:15' }
])

// 聊天相关
const messages = ref<any[]>([])
const inputMessage = ref('')
const isTyping = ref(false)
const chatContainer = ref<HTMLElement | null>(null)
const quickQuestions = ref([
  '我的鸟最近食欲不振',
  '粪便颜色异常偏白',
  '羽毛掉落严重',
  '活动量明显减少',
  '呼吸时有异常声音'
])
let messageId = 0

// ===== 核心 Web API 重构 =====

// 触发隐藏的 input file
const triggerUpload = () => {
  fileInput.value?.click()
}

// 处理本地图片选择并生成 URL
const onFileChange = (e: Event) => {
  const target = e.target as HTMLInputElement
  const file = target.files?.[0]
  if (file) {
    uploadedImage.value = URL.createObjectURL(file)
    analysisResult.value = null
  }
  if (target) target.value = '' // 清空允许重选同文件
}

// 模拟硬件抓拍 (保留原逻辑，替换为 ElLoading/ElMessage)
const triggerCageCapture = () => {
  const loading = ElLoading.service({
    text: '唤醒摄像头中...',
    background: 'rgba(0, 0, 0, 0.7)'
  })

  setTimeout(() => {
    loading.close()
    ElMessage.success('抓拍成功！')
    // 使用网图模拟抓拍结果
    uploadedImage.value = 'https://images.unsplash.com/photo-1552728089-571ed927bdf1?q=80&w=1000&auto=format&fit=crop'
    analysisResult.value = null
  }, 2000)
}

// 开始 AI 分析
const analyzeImage = () => {
  if (!uploadedImage.value) {
    ElMessage.warning('请先上传图片')
    return
  }
  
  isAnalyzing.value = true
  
  setTimeout(() => {
    generateMockResult()
    isAnalyzing.value = false
  }, 3000)
}

// 生成模拟分析结果
const generateMockResult = () => {
  const results = [
    { status: 'normal', diagnosis: '健康正常', shape: '形态完整', color: '深褐色', abnormal: '无异常', confidence: 96, suggestion: '您的宠物鸟粪便状态正常，继续保持良好的饲养环境和饮食习惯。' },
    { status: 'warning', diagnosis: '轻微消化不良', shape: '略微稀软', color: '浅褐色', abnormal: '水分略多', confidence: 88, suggestion: '建议减少高脂肪食物摄入，增加易消化的谷物比例，保持充足饮水。如症状持续，请咨询兽医。' },
    { status: 'danger', diagnosis: '疑似细菌感染', shape: '稀烂', color: '灰白色', abnormal: '颜色异常、形态异常', confidence: 92, suggestion: '检测到粪便异常，可能是细菌感染或消化道疾病。建议立即咨询专业兽医，暂停日常喂食，并隔离观察。' }
  ]
  
  const result = results[Math.floor(Math.random() * results.length)]
  const now = new Date()
  const timeStr = `${now.getMonth() + 1}/${now.getDate()} ${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`
  
  analysisResult.value = result
  analysisTime.value = timeStr
  analysisHistory.value.unshift({
    id: Date.now(),
    diagnosis: result.diagnosis,
    status: result.status,
    time: timeStr
  })
  if (analysisHistory.value.length > 5) analysisHistory.value.pop()
}

// 页面跳转 (替换 wx.switchTab)
const gotoConsult = () => {
  router.push('/cloud/consultation')
}

const viewHistory = (id: number) => { ElMessage.info('查看历史记录: ' + id) }
const showAllHistory = () => { ElMessage.info('全部历史记录') }

// ===== 聊天引擎与自动滚动 =====

const scrollToBottom = () => {
  nextTick(() => {
    if (chatContainer.value) {
      chatContainer.value.scrollTop = chatContainer.value.scrollHeight
    }
  })
}

// 监听消息或打字状态变化，自动触底
watch([messages, isTyping], () => {
  scrollToBottom()
}, { deep: true })

const sendMessage = () => {
  const message = inputMessage.value.trim()
  if (!message) return
  
  addMessage('user', message)
  inputMessage.value = ''
  
  generateAIResponse(message)
}

const sendQuickQuestion = (question: string) => {
  addMessage('user', question)
  generateAIResponse(question)
}

const addMessage = (role: string, content: string) => {
  const now = new Date()
  const timeStr = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`
  
  messages.value.push({
    id: ++messageId,
    role,
    content,
    time: timeStr
  })
}

const generateAIResponse = (userMessage: string) => {
  isTyping.value = true
  
  setTimeout(() => {
    const response = getMockAIResponse(userMessage)
    isTyping.value = false
    addMessage('ai', response)
  }, 1500)
}

const getMockAIResponse = (message: string) => {
  const responses: Record<string, string> = {
    '食欲': '食欲不振可能是多种原因导致的，如环境压力、疾病、食物问题等。建议您：1）检查环境温度是否适宜（22-28℃）；2）观察是否有其他症状如精神萎靡、羽毛蓬松；3）提供易消化的食物；4）如症状持续超过24小时，建议咨询专业兽医。',
    '粪便颜色异常偏白': '粪便颜色偏白可能提示肝脏或肾脏问题，或者是尿酸沉积。建议您：1）持续观察几天，记录其他症状；2）检查饮食是否过于单一；3）确保充足饮水；4）如果伴随精神状态不佳，请立即联系兽医进行血液检查。',
    '羽毛掉落': '正常换毛期会有羽毛脱落，但如果掉落严重可能是以下原因：1）营养缺乏，特别是蛋白质和维生素；2）寄生虫感染；3）压力或环境变化；4）激素失调。建议检查饲养环境，提供营养均衡的食物，如情况严重请咨询兽医。',
    '活动量减少': '活动量减少可能预示健康问题，可能原因包括：1）疾病或感染；2）温度不适；3）环境压力；4）营养不良。建议：观察鸟儿是否有其他症状如呼吸急促、眼部异常，监测环境温度，如持续超过24小时不进食或不动，请立即就医。',
    '呼吸异常': '呼吸异常是严重症状，可能原因包括：1）呼吸道感染；2）肺炎或气胸；3）异物堵塞；4）过敏反应。建议立即隔离观察，保持环境温暖通风，并尽快联系专业兽医进行急诊处理。'
  }
  
  for (const [key, value] of Object.entries(responses)) {
    if (message.includes(key)) {
      return value
    }
  }
  
  return `根据您的描述"${message.substring(0, 20)}..."，这可能是多种因素导致的。建议您：\n\n1）观察宠物鸟的整体状态（精神、食欲、活动量）\n2）检查饲养环境（温度、湿度、清洁度）\n3）查看是否有其他异常症状\n4）如症状持续或加重，建议咨询专业兽医进行详细检查\n\n您能提供更多详细信息吗？比如症状持续的时间、是否有其他异常表现等。`
}
</script>

<style scoped>
.page-container {
  min-height: 100vh;
  background: linear-gradient(180deg, #E8F8F5 0%, #F5F7FA 30%);
  position: relative;
  width: 100% !important;
  display: block !important;
  box-sizing: border-box !important;
}

/* 自定义导航栏 */
.custom-navbar {
  position: sticky;
  top: 0;
  z-index: 100;
  background: linear-gradient(135deg, #00B894, #00A884);
  padding: 44px 16px 12px;
  box-shadow: 0 2px 8px rgba(0, 184, 148, 0.2);
}

.navbar-content {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.navbar-title {
  font-size: 18px;
  font-weight: 600;
  color: #FFFFFF;
}

.ai-badge {
  display: flex;
  align-items: center;
  gap: 4px;
  background: rgba(255, 255, 255, 0.2);
  padding: 4px 8px;
  border-radius: 10px;
}

.badge-icon {
  font-size: 14px;
}

.badge-text {
  font-size: 12px;
  color: #FFFFFF;
  font-weight: 500;
}

/* 功能切换Tab */
.function-tabs {
  display: flex;
  background: #FFFFFF;
  margin: 12px;
  border-radius: 8px;
  padding: 4px;
  box-shadow: 0 2px 6px rgba(0, 0, 0, 0.06);
}

.tab-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
  padding: 12px;
  border-radius: 6px;
  transition: all 0.3s ease;
  cursor: pointer;
}

.tab-item.active {
  background: linear-gradient(135deg, #00B894, #55EFC4);
}

.tab-icon {
  font-size: 20px;
}

.tab-text {
  font-size: 12px;
  color: #636E72;
}

.tab-item.active .tab-text {
  color: #FFFFFF;
  font-weight: 500;
}

/* 图像识别模块 */
.image-section {
  padding: 0 12px;
}

.image-card {
  background: #FFFFFF;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.06);
}

/* 上传区域 */
.upload-area {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
  background: linear-gradient(135deg, #F5F7FA, #E8F8F5);
  border: 1.5px dashed #DFE6E9;
  border-radius: 12px;
  transition: all 0.3s ease;
  cursor: pointer;
}

.upload-area:active {
  border-color: #00B894;
  background: #E8F8F5;
}

.upload-placeholder {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
}

.upload-icon {
  font-size: 40px;
  opacity: 0.6;
}

.upload-text {
  font-size: 14px;
  color: #2D3436;
  font-weight: 500;
}

.upload-tip {
  font-size: 11px;
  color: #B2BEC3;
}

/* 预览区域 */
.preview-area {
  position: relative;
  border-radius: 12px;
  overflow: hidden;
}

.preview-image {
  width: 100%;
  height: 200px;
  object-fit: cover;
}

.reupload-btn {
  position: absolute;
  top: 12px;
  right: 12px;
  background: rgba(0, 0, 0, 0.7);
  color: #FFFFFF;
  padding: 6px 12px;
  border-radius: 10px;
  font-size: 12px;
  backdrop-filter: blur(5px);
  cursor: pointer;
}

/* 分析按钮 */
.analyze-btn {
  width: 100%;
  margin-top: 16px;
  padding: 14px;
  background: linear-gradient(135deg, #00B894, #00A884);
  color: #FFFFFF;
  border-radius: 24px;
  font-size: 16px;
  font-weight: 600;
  border: none;
  box-shadow: 0 4px 10px rgba(0, 184, 148, 0.4);
  transition: all 0.3s ease;
  cursor: pointer;
  display: block;
}

.analyze-btn.analyzing {
  background: linear-gradient(135deg, #B2BEC3, #636E72);
  opacity: 0.6;
  box-shadow: none;
}

.analyze-btn[disabled] {
  background: #B2BEC3;
  cursor: not-allowed;
  box-shadow: none;
}

/* 分析结果 */
.result-section {
  margin-top: 16px;
}

.result-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.result-title {
  font-size: 16px;
  font-weight: 600;
  color: #2D3436;
}

.result-time {
  font-size: 11px;
  color: #B2BEC3;
}

.result-card {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 16px;
  border-radius: 10px;
  margin-bottom: 12px;
}

.result-normal { background: linear-gradient(135deg, #D4EFDF, #A9DFBF); }
.result-warning { background: linear-gradient(135deg, #FCF3CF, #F9E79F); }
.result-danger { background: linear-gradient(135deg, #FADBD8, #F5B7B1); }

.result-icon {
  font-size: 28px;
}

.result-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 4px;
  margin: 0 12px;
}

.result-label {
  font-size: 11px;
  color: #636E72;
}

.result-value {
  font-size: 16px;
  font-weight: 600;
  color: #2D3436;
}

.result-confidence {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 2px;
}

.confidence-label {
  font-size: 10px;
  color: #636E72;
}

.confidence-value {
  font-size: 14px;
  font-weight: 600;
  color: #00B894;
}

/* 详细分析 */
.detail-section {
  background: #F8F9FA;
  padding: 12px;
  border-radius: 8px;
  margin-bottom: 12px;
}

.detail-title {
  font-size: 13px;
  font-weight: 600;
  color: #2D3436;
  margin-bottom: 8px;
}

.detail-content {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.detail-item {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.detail-label {
  font-size: 12px;
  color: #636E72;
}

.detail-value {
  font-size: 12px;
  color: #2D3436;
  font-weight: 500;
}

/* AI建议 */
.suggestion-section {
  background: linear-gradient(135deg, #E8F8F5, #D4EFDF);
  padding: 12px;
  border-radius: 8px;
  margin-bottom: 12px;
}

.suggestion-title {
  font-size: 13px;
  font-weight: 600;
  color: #2D3436;
  margin-bottom: 6px;
}

.suggestion-text {
  font-size: 12px;
  color: #636E72;
  line-height: 1.8;
}

/* 咨询按钮 */
.consult-btn {
  width: 100%;
  padding: 14px;
  background: linear-gradient(135deg, #FF7675, #D63031);
  color: #FFFFFF;
  border-radius: 24px;
  font-size: 14px;
  font-weight: 600;
  border: none;
  box-shadow: 0 4px 10px rgba(214, 48, 49, 0.3);
  cursor: pointer;
  display: block;
}

/* 历史记录 */
.history-section {
  margin-top: 16px;
  padding-top: 16px;
  border-top: 0.5px solid #DFE6E9;
}

.section-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.section-title {
  font-size: 14px;
  font-weight: 600;
  color: #2D3436;
}

.section-more {
  font-size: 11px;
  color: #00B894;
  cursor: pointer;
}

.history-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.history-item {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 10px;
  background: #F8F9FA;
  border-radius: 6px;
  cursor: pointer;
}

.history-icon {
  width: 28px;
  height: 28px;
  display: flex;
  align-items: center;
  justify-content: center;
  border-radius: 50%;
  font-size: 16px;
}

.history-normal { background: #D4EFDF; }
.history-warning { background: #FCF3CF; }
.history-danger { background: #FADBD8; }

.history-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.history-diagnosis {
  font-size: 13px;
  color: #2D3436;
  font-weight: 500;
}

.history-time {
  font-size: 11px;
  color: #B2BEC3;
}

.history-arrow {
  font-size: 14px;
  color: #B2BEC3;
}

/* 聊天模块 */
.chat-section {
  /* 固定高度，为滚动容器留出空间 */
  height: calc(100vh - 140px);
  padding: 0 12px;
  padding-bottom: 20px;
}

.chat-container {
  height: 100%;
  display: flex;
  flex-direction: column;
  gap: 12px;
}

/* 欢迎界面 */
.chat-welcome {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 12px;
  padding: 24px 12px;
}

.welcome-avatar {
  font-size: 60px;
  animation: pulse 2s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% { transform: scale(1); }
  50% { transform: scale(1.05); }
}

.welcome-title {
  font-size: 20px;
  font-weight: 700;
  color: #00B894;
}

.welcome-desc {
  font-size: 14px;
  color: #636E72;
  text-align: center;
  line-height: 1.8;
  max-width: 250px;
}

.quick-questions {
  width: 100%;
  margin-top: 16px;
}

.question-title {
  font-size: 12px;
  color: #B2BEC3;
  margin-bottom: 8px;
}

.question-list {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.question-item {
  padding: 12px;
  background: #F8F9FA;
  border-radius: 8px;
  font-size: 13px;
  color: #2D3436;
  transition: all 0.3s ease;
  cursor: pointer;
}

.question-item:active {
  background: #E8F8F5;
  color: #00B894;
}

/* 聊天消息自动滚动容器 */
.chat-messages {
  flex: 1;
  padding: 12px;
  background: #FFFFFF;
  border-radius: 12px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.06);
  overflow-y: auto;
  /* 隐藏滚动条让UI更清爽 */
  scrollbar-width: none; 
}
.chat-messages::-webkit-scrollbar {
  display: none;
}

.message-wrapper {
  display: flex;
  align-items: flex-end;
  gap: 8px;
  margin-bottom: 16px;
}

.message-ai { flex-direction: row; }
.message-user { flex-direction: row-reverse; }

.message-avatar {
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #F5F7FA, #E8F8F5);
  border-radius: 50%;
  font-size: 16px;
  flex-shrink: 0;
}

.message-content-wrapper {
  max-width: 70%;
  display: flex;
  flex-direction: column;
  gap: 4px;
}

.message-bubble {
  padding: 12px;
  border-radius: 10px;
  font-size: 14px;
  line-height: 1.6;
  word-wrap: break-word;
}

.message-ai-bubble {
  background: #F8F9FA;
  color: #2D3436;
  border-bottom-left-radius: 4px;
}

.message-user-bubble {
  background: linear-gradient(135deg, #00B894, #55EFC4);
  color: #FFFFFF;
  border-bottom-right-radius: 4px;
}

.message-time {
  font-size: 10px;
  color: #B2BEC3;
}

.message-user .message-time {
  text-align: right;
}

/* 打字动画 */
.typing { padding: 16px !important; }
.typing-dots { display: flex; gap: 4px; }
.typing-dots .dot {
  width: 6px;
  height: 6px;
  background: #B2BEC3;
  border-radius: 50%;
  animation: typing 1.4s ease-in-out infinite;
}
.typing-dots .dot:nth-child(1) { animation-delay: 0s; }
.typing-dots .dot:nth-child(2) { animation-delay: 0.2s; }
.typing-dots .dot:nth-child(3) { animation-delay: 0.4s; }

@keyframes typing {
  0%, 60%, 100% { transform: translateY(0); }
  30% { transform: translateY(-4px); }
}

/* 聊天输入区域 */
.chat-input-area {
  display: flex;
  gap: 8px;
  padding: 12px;
  background: #FFFFFF;
  border-radius: 12px;
  box-shadow: 0 4px 12px rgba(0, 0, 0, 0.06);
  flex-shrink: 0;
}

.chat-input {
  flex: 1;
  padding: 10px 12px;
  background: #F8F9FA;
  border-radius: 24px;
  font-size: 14px;
  color: #2D3436;
  border: none;
  outline: none;
}

.send-btn {
  padding: 10px 16px;
  background: #B2BEC3;
  color: #FFFFFF;
  border-radius: 24px;
  font-size: 14px;
  font-weight: 500;
  transition: all 0.3s ease;
  cursor: pointer;
  display: flex;
  align-items: center;
}

.send-btn.active {
  background: linear-gradient(135deg, #00B894, #55EFC4);
}

/* 智能鸟笼悬浮抓拍按钮 (FAB) */
.capture-fab {
  position: fixed;
  bottom: 80px;
  right: 16px;
  display: flex;
  align-items: center;
  justify-content: center;
  background: linear-gradient(135deg, #00B894, #00A884);
  padding: 12px 20px;
  border-radius: 24px;
  box-shadow: 0 4px 10px rgba(0, 184, 148, 0.4);
  transition: all 0.3s ease;
  z-index: 999;
  cursor: pointer;
}

.capture-fab:active {
  transform: scale(0.92);
  box-shadow: 0 2px 5px rgba(0, 184, 148, 0.3);
}

.capture-icon {
  font-size: 18px;
  margin-right: 6px;
}

.capture-text {
  color: #FFFFFF;
  font-size: 14px;
  font-weight: 600;
  letter-spacing: 1px;
}
</style>