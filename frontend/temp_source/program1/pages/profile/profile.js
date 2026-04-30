// pages/profile/profile.js
const app = getApp()

Page({
  data: {
    // 用户信息
    userInfo: {
      avatar: '',
      nickname: '',
      id: 'BN88888',
      isVip: false,
      deviceCount: 3,
      consultCount: 12,
      days: 88
    },
    
    // 订阅相关
    selectedPlan: 'basic',
    
    // 设备状态
    deviceStatus: {
      total: 3,
      online: 3,
      unconfigured: 0
    },
    
    // 订单相关
    orderCount: 2,
    
    // 康复打卡
    recoveryDays: 15
  },

  onLoad() {
    this.loadUserInfo()
  },

  onShow() {
    this.checkDeviceStatus()
  },

  // 加载用户信息
  loadUserInfo() {
    // 从全局获取或从存储获取用户信息
    const appData = app.globalData
    if (appData.userInfo && appData.userInfo.avatar) {
      this.setData({
        userInfo: {
          ...this.data.userInfo,
          ...appData.userInfo
        }
      })
    }
    
    // 检查订阅状态（模拟）
    const isVip = wx.getStorageSync('isVip') || false
    const plan = wx.getStorageSync('selectedPlan') || 'basic'
    
    this.setData({
      userInfo: {
        ...this.data.userInfo,
        isVip
      },
      selectedPlan: plan
    })
  },

  // 检查设备状态
  checkDeviceStatus() {
    // 模拟检查设备状态
    const unconfigured = Math.random() > 0.8 ? 1 : 0
    this.setData({
      deviceStatus: {
        ...this.data.deviceStatus,
        unconfigured
      }
    })
  },

  // 选择订阅方案
  selectPlan(e) {
    const plan = e.currentTarget.dataset.plan
    this.setData({ selectedPlan: plan })
  },

  // 处理订阅
  handleSubscribe() {
    const plan = this.data.selectedPlan
    
    if (plan === 'basic') {
      wx.showModal({
        title: '切换方案',
        content: '确定要切换到基础监测版吗？将失去深度版所有权益。',
        confirmText: '确定切换',
        success: (res) => {
          if (res.confirm) {
            wx.setStorageSync('isVip', false)
            wx.setStorageSync('selectedPlan', 'basic')
            this.setData({
              'userInfo.isVip': false
            })
            wx.showToast({
              title: '已切换至基础版',
              icon: 'success'
            })
          }
        }
      })
    } else {
      // 深度版开通
      wx.showModal({
        title: '开通深度健康管理版',
        content: '费用: ¥99/年\n\n立即开通享受所有深度版权益',
        confirmText: '立即开通',
        success: (res) => {
          if (res.confirm) {
            wx.showLoading({
              title: '开通中...',
              mask: true
            })
            
            // 模拟支付过程
            setTimeout(() => {
              wx.setStorageSync('isVip', true)
              wx.setStorageSync('selectedPlan', 'premium')
              this.setData({
                'userInfo.isVip': true
              })
              
              wx.hideLoading()
              wx.showToast({
                title: '开通成功',
                icon: 'success'
              })
            }, 2000)
          }
        }
      })
    }
  },

  // 跳转设置
  gotoSettings() {
    wx.showToast({
      title: '设置',
      icon: 'none'
    })
  },

  // 管理设备
  manageDevices() {
    wx.showToast({
      title: '设备管理',
      icon: 'none'
    })
  },

  // 设备配网
  gotoDevice() {
    wx.showToast({
      title: '设备配网',
      icon: 'none'
    })
  },

  // 我的订单
  gotoOrders() {
    wx.showToast({
      title: '我的订单',
      icon: 'none'
    })
  },

  // 康复追踪
  gotoRecovery() {
    wx.showToast({
      title: '康复追踪打卡',
      icon: 'none'
    })
  },

  // 健康报告
  gotoReports() {
    wx.switchTab({
      url: '/pages/clinic/clinic'
    })
  },

  // 联系客服
  gotoSupport() {
    wx.showModal({
      title: '联系客服',
      content: '客服电话: 400-888-8888\n在线时间: 7×24小时',
      confirmText: '拨打',
      cancelText: '关闭',
      success: (res) => {
        if (res.confirm) {
          wx.makePhoneCall({
            phoneNumber: '400-888-8888'
          })
        }
      }
    })
  },

  // 关于伴鸟
  gotoAbout() {
    wx.showModal({
      title: '关于伴鸟',
      content: '伴鸟是一款专注于异宠智能物联网管理的小程序，通过智能硬件与AI技术，为您的宠物提供全方位的健康监测与管理服务。\n\n版本: v1.0.0',
      showCancel: false
    })
  },

  // 退出登录
  handleLogout() {
    wx.showModal({
      title: '退出登录',
      content: '确定要退出登录吗？',
      confirmText: '确定',
      confirmColor: '#FF7675',
      success: (res) => {
        if (res.confirm) {
          // 清除用户信息
          wx.clearStorageSync()
          
          // 重置全局数据
          app.globalData.userInfo = {
            avatar: '',
            nickname: '',
            id: ''
          }
          
          wx.showToast({
            title: '已退出登录',
            icon: 'success'
          })
        }
      }
    })
  }
})
