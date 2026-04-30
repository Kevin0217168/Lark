// pages/ai/ai.js
Page({
  data: {
    // 当前Tab
    activeTab: 'image',
    
    // 图像识别相关
    uploadedImage: '',
    isAnalyzing: false,
    analysisResult: null,
    analysisTime: '',
    
    // 历史记录
    analysisHistory: [
      {
        id: 1,
        diagnosis: '正常',
        status: 'normal',
        time: '2026/2/21 14:30'
      },
      {
        id: 2,
        diagnosis: '消化不良',
        status: 'warning',
        time: '2026/2/20 09:15'
      }
    ],
    
    // 聊天相关
    messages: [],
    inputMessage: '',
    isTyping: false,
    toView: '',
    quickQuestions: [
      '我的鸟最近食欲不振',
      '粪便颜色异常偏白',
      '羽毛掉落严重',
      '活动量明显减少',
      '呼吸时有异常声音'
    ],
    messageId: 0
  },

  onLoad() {
    this.initChat()
  },

  // 初始化聊天
  initChat() {
    // 加载历史消息（可选）
  },

  // 切换Tab
  switchTab(e) {
    const tab = e.currentTarget.dataset.tab
    this.setData({ activeTab: tab })
  },

  // ===== 图像识别功能 =====
  
  // 选择图片 (原本的本地相册上传功能)
  chooseImage() {
    wx.chooseImage({
      count: 1,
      sizeType: ['compressed'],
      sourceType: ['album', 'camera'],
      success: (res) => {
        const tempFilePath = res.tempFilePaths[0]
        this.setData({ 
          uploadedImage: tempFilePath,
          analysisResult: null
        })
      }
    })
  },

  // 🦜 新增：触发单片机抓拍 (智能鸟笼硬件联动)
  triggerCageCapture() {
    wx.showLoading({
      title: '唤醒摄像头中...',
      mask: true
    });

    // 【挑战杯高分亮点】：这里预留了真实的物联网通信逻辑
    /* wx.request({
      url: 'http://localhost:3000/api/parrot/capture', // 呼叫你的 Node.js 后端
      method: 'POST',
      success: (res) => {
         // 获取到 ESP32-CAM 传回的照片地址
         this.setData({ 
           uploadedImage: res.data.imageUrl, // 自动把照片填入界面的预览框！
           analysisResult: null // 清空之前的分析结果
         });
      }
    })
    */
    
    // 现在的模拟演示效果（2秒后假装单片机抓拍成功，方便你现在看界面效果）
    setTimeout(() => {
      wx.hideLoading();
      wx.showToast({
        title: '抓拍成功！',
        icon: 'success'
      });
      
      // 模拟抓拍到的图片传回了手机
      // 这一步非常关键：将其赋值给 uploadedImage 后，你的“开始AI分析”按钮就会自动亮起解锁！
      this.setData({
        uploadedImage: 'https://images.unsplash.com/photo-1552728089-571ed927bdf1?q=80&w=1000&auto=format&fit=crop', // 这里随便放了一张鹦鹉的网图做演示，后续换成你真实接口传来的底盘照片
        analysisResult: null 
      });
    }, 2000);
  },

  // 分析图片
  analyzeImage() {
    if (!this.data.uploadedImage) {
      wx.showToast({
        title: '请先上传图片',
        icon: 'none'
      })
      return
    }
    
    this.setData({ isAnalyzing: true })
    
    // 模拟AI分析过程
    setTimeout(() => {
      this.generateMockResult()
      this.setData({ isAnalyzing: false })
    }, 3000)
  },

  // 生成模拟分析结果
  generateMockResult() {
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
    ]
    
    const randomIndex = Math.floor(Math.random() * results.length)
    const result = results[randomIndex]
    
    const now = new Date()
    const timeStr = `${now.getMonth() + 1}/${now.getDate()} ${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`
    
    // 添加到历史记录
    const newHistory = {
      id: Date.now(),
      diagnosis: result.diagnosis,
      status: result.status,
      time: timeStr
    }
    
    this.setData({
      analysisResult: result,
      analysisTime: timeStr,
      analysisHistory: [newHistory, ...this.data.analysisHistory].slice(0, 5)
    })
  },

  // 查看历史记录
  viewHistory(e) {
    const id = e.currentTarget.dataset.id
    wx.showToast({
      title: '查看历史记录',
      icon: 'none'
    })
  },

  // 显示全部历史
  showAllHistory() {
    wx.showToast({
      title: '全部历史记录',
      icon: 'none'
    })
  },

  // 跳转咨询
  gotoConsult() {
    wx.switchTab({
      url: '/pages/clinic/clinic'
    })
  },

  // ===== 聊天功能 =====
  
  // 输入框变化
  onInput(e) {
    this.setData({ inputMessage: e.detail.value })
  },

  // 发送消息
  sendMessage() {
    const message = this.data.inputMessage.trim()
    if (!message) return
    
    // 添加用户消息
    this.addMessage('user', message)
    this.setData({ inputMessage: '' })
    
    // AI回复
    this.generateAIResponse(message)
  },

  // 发送快捷问题
  sendQuickQuestion(e) {
    const question = e.currentTarget.dataset.question
    this.addMessage('user', question)
    this.generateAIResponse(question)
  },

  // 添加消息
  addMessage(role, content) {
    const now = new Date()
    const timeStr = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`
    
    const message = {
      id: ++this.data.messageId,
      role,
      content,
      time: timeStr
    }
    
    this.setData({
      messages: [...this.data.messages, message],
      toView: `msg-${message.id}`
    })
  },

  // 生成AI回复
  generateAIResponse(userMessage) {
    this.setData({ isTyping: true })
    
    // 模拟AI思考时间
    setTimeout(() => {
      const response = this.getMockAIResponse(userMessage)
      this.setData({ isTyping: false })
      this.addMessage('ai', response)
    }, 1500)
  },

  // 获取模拟AI回复
  getMockAIResponse(message) {
    const responses = {
      '食欲': '食欲不振可能是多种原因导致的，如环境压力、疾病、食物问题等。建议您：1）检查环境温度是否适宜（22-28℃）；2）观察是否有其他症状如精神萎靡、羽毛蓬松；3）提供易消化的食物；4）如症状持续超过24小时，建议咨询专业兽医。',
      '粪便颜色异常偏白': '粪便颜色偏白可能提示肝脏或肾脏问题，或者是尿酸沉积。建议您：1）持续观察几天，记录其他症状；2）检查饮食是否过于单一；3）确保充足饮水；4）如果伴随精神状态不佳，请立即联系兽医进行血液检查。',
      '羽毛掉落': '正常换毛期会有羽毛脱落，但如果掉落严重可能是以下原因：1）营养缺乏，特别是蛋白质和维生素；2）寄生虫感染；3）压力或环境变化；4）激素失调。建议检查饲养环境，提供营养均衡的食物，如情况严重请咨询兽医。',
      '活动量减少': '活动量减少可能预示健康问题，可能原因包括：1）疾病或感染；2）温度不适；3）环境压力；4）营养不良。建议：观察鸟儿是否有其他症状如呼吸急促、眼部异常，监测环境温度，如持续超过24小时不进食或不动，请立即就医。',
      '呼吸异常': '呼吸异常是严重症状，可能原因包括：1）呼吸道感染；2）肺炎或气胸；3）异物堵塞；4）过敏反应。建议立即隔离观察，保持环境温暖通风，并尽快联系专业兽医进行急诊处理。'
    }
    
    // 简单匹配关键词
    for (const [key, value] of Object.entries(responses)) {
      if (message.includes(key)) {
        return value
      }
    }
    
    // 默认回复
    return `根据您的描述"${message.substring(0, 20)}..."，这可能是多种因素导致的。建议您：\n\n1）观察宠物鸟的整体状态（精神、食欲、活动量）\n2）检查饲养环境（温度、湿度、清洁度）\n3）查看是否有其他异常症状\n4）如症状持续或加重，建议咨询专业兽医进行详细检查\n\n您能提供更多详细信息吗？比如症状持续的时间、是否有其他异常表现等。`
  }
})