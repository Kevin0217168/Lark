// pages/home/home.js
const app = getApp();

Page({
  data: {
    // 预警信息
    showAlert: true,
    alertLevel: 'warning', // 'info', 'warning', 'danger'
    alertTitle: '活动量提醒',
    alertMessage: '检测到活动量骤减，请注意观察',
    alertIcon: '⚠️',
    
    // 温湿度数据
    temperature: 37.5,
    humidity: 65,
    updateTime: '10:30:25',
    
    // 喂鸟器数据
    foodLevel: 75,
    todayFeedCount: 3,
    lastFeedTime: '09:15',
    feedAmount: 20,
    
    // ☁️ 新增：存放巴法云传来的真实距离数据
    foodDistance: '--',
    
    // 设备列表
    devices: [
      {
        id: 1,
        name: '智能孵化监测仪',
        icon: '🌡️',
        online: true
      },
      {
        id: 2,
        name: '智能喂鸟器',
        icon: '🥣',
        online: true
      },
      {
        id: 3,
        name: '环境传感器',
        icon: '📊',
        online: true
      }
    ]
  },

  onLoad: function (options) {
    // 页面刚加载时，调用原有的刷新逻辑
    //this.refreshData();
    // ☁️ 新增：页面加载时，立即去巴法云拿一次超声波数据
    this.getBirdFoodData();
  },

  // ==================== ☁️ 新增：巴法云取件函数 ====================
  getBirdFoodData() {
    wx.request({
      // 巴法云的官方提货接口
      url: 'https://api.bemfa.com/api/device/v1/data/1/get/', 
      data: {
        uid: '8a512c3a9261c60beea0bce9e3ee38a7',  // ⚠️ 别忘了换成你长长的那串字母数字 UID
        topic: 'birdFood',                 // 你的主题名字
        num: 1                             // 只要最新鲜的 1 条数据
      },
      method: 'GET',
      success: (res) => {
        console.log("巴法云原始回复内容：", res.data); 

        // 🚨 修改了这里的判断条件：只要里面有 msg 字段，就说明拿到了！
        if (res.data && res.data.msg) {
          // 直接拿 msg 字段里的距离数值
          const distance = res.data.msg; 
          
          console.log("☁️ 云端快递已送达！当前余粮距离：", distance, "厘米");
          
          // 弹窗提示
        

          // 换算成余粮百分比 (假设粮仓总深度是 20 厘米)
          let calcFoodLevel = 100 - (distance / 20 * 100);
          if(calcFoodLevel < 0) calcFoodLevel = 0;
          if(calcFoodLevel > 100) calcFoodLevel = 100;

          // 把真实数据塞进页面的 data 里
          this.setData({ 
            foodDistance: distance,
            foodLevel: parseInt(calcFoodLevel) 
          });
        }
      },
      fail: (err) => {
        console.error("获取云端数据失败", err);
      }
    })
  },
  // =================================================================
// 🌟 新增：获取温湿度的函数
getBirdEnvData() {
  wx.request({
    url: 'https://api.bemfa.com/api/device/v1/data/1/get/',
    data: {
      uid: '8a512c3a9261c60beea0bce9e3ee38a7', // 你的私钥
      topic: 'birdEnv', // 去刚才建的新仓库拿
      num: 1
    },
    method: 'GET',
    success: (res) => {
      if (res.data && res.data.msg) {
        // 拿到的原始数据类似于 "26.50#55.00"
        let rawData = res.data.msg;
        // 用 split 方法按井号 '#' 一刀切开，变成数组 [温度, 湿度]
        let envData = rawData.split('#'); 
        
        if(envData.length === 2) {
          // 把切好的数据塞给页面的 temp 和 humid 变量
          this.setData({
            temp: envData[0],
            humid: envData[1]
          });
        }
      }
    }
  });
},
  // 把全局仓库的数据，搬到当前页面的 data 里展示
  syncDataToPage: function() {
    const deviceData = app.globalData.deviceData;
    if (deviceData && deviceData.temperature) {
      this.setData({
        temp: deviceData.temperature,
        humid: deviceData.humidity,
        foodLevel: deviceData.foodRemaining 
      });
    }
  },

// 页面每次显示在屏幕上时执行
onShow: function() {
  // 🚨 开启“自动巡逻”：每隔 5 秒（5000毫秒）去巴法云拿一次最新距离！
  this.radarTimer = setInterval(() => {
    this.getBirdFoodData();
    this.getBirdEnvData();
  }, 5000);

  // 原本的本地数据同步（保留你的老代码逻辑）
  this.localTimer = setInterval(() => {
    this.syncDataToPage();
  }, 2000);
},

// 🚨 新增：当用户切换到其他页面时，把巡逻关掉，省电省流量
onHide: function() {
  clearInterval(this.radarTimer);
  clearInterval(this.localTimer);
},

  // 刷新数据
  refreshData() {
    wx.showLoading({
      title: '向服务器请求中...'
    });
    app.updateDeviceData();
    setTimeout(() => {
      const deviceData = app.globalData.deviceData;
      const now = new Date();
      const updateTime = now.toTimeString().substring(0, 8);
      
      if (deviceData && deviceData.temperature) {
        this.setData({
          temp: deviceData.temperature,
          humid: deviceData.humidity,
          foodLevel: deviceData.foodRemaining,
          updateTime: updateTime
        });

        if(typeof this.checkAlerts === 'function') {
          this.checkAlerts();
        }

        wx.hideLoading();
        wx.showToast({
          title: '刷新真数据成功',
          icon: 'success'
        });
      } else {
        wx.hideLoading();
        wx.showToast({
          title: '服务器未响应',
          icon: 'error'
        });
      }
    }, 1000); 
  },

  // 检查预警
  checkAlerts() {
    const temp = parseFloat(this.data.temperature);
    const foodLevel = this.data.foodLevel;
    
    if (foodLevel < 30) {
      this.setData({
        showAlert: true,
        alertLevel: 'danger',
        alertTitle: '粮食不足',
        alertMessage: '余粮不足30%，请及时补充',
        alertIcon: '🚨'
      });
    } else if (temp > 39 || temp < 35) {
      this.setData({
        showAlert: true,
        alertLevel: 'warning',
        alertTitle: '温度异常',
        alertMessage: `当前温度${temp}℃，超出正常范围`,
        alertIcon: '⚠️'
      });
    } else {
      this.setData({
        showAlert: false
      });
    }
  },

  // 关闭预警
  closeAlert() {
    this.setData({
      showAlert: false
    });
  },

  // 减少投喂量
  decreaseFood() {
    let amount = this.data.feedAmount;
    if (amount > 5) {
      amount -= 5;
      this.setData({
        feedAmount: amount
      });
    }
  },

  // 增加投喂量
  increaseFood() {
    let amount = this.data.feedAmount;
    if (amount < 50) {
      amount += 5;
      this.setData({
        feedAmount: amount
      });
    }
  },

  // 立即投喂
  feedNow() {
    const amount = this.data.feedAmount;
    wx.showModal({
      title: '确认投喂',
      content: `确定要投喂 ${amount}克 粮食吗？`,
      success: (res) => {
        if (res.confirm) {
          wx.showLoading({
            title: '投喂中...'
          });

          setTimeout(() => {
            const now = new Date();
            const lastFeedTime = now.toTimeString().substring(0, 5);
            const newFoodLevel = Math.max(0, this.data.foodLevel - Math.floor(amount / 5));
            
            this.setData({
              foodLevel: newFoodLevel,
              todayFeedCount: this.data.todayFeedCount + 1,
              lastFeedTime: lastFeedTime
            });

            app.updateDeviceData({
              feeder: {
                foodLevel: newFoodLevel,
                todayFeedCount: this.data.todayFeedCount + 1
              }
            });

            wx.hideLoading();
            wx.showToast({
              title: '投喂成功',
              icon: 'success'
            });
            if (newFoodLevel < 30) {
              setTimeout(() => {
                this.setData({
                  showAlert: true,
                  alertLevel: 'danger',
                  alertTitle: '粮食不足',
                  alertMessage: '余粮不足30%，请及时补充',
                  alertIcon: '🚨'
                });
              }, 1000);
            }
          }, 1500);
        }
      }
    });
  },

  // 定时刷新数据
  startDataRefresh() {
    setInterval(() => {
      if (!this.data.showAlert) {
        const now = new Date();
        this.setData({
          updateTime: now.toTimeString().substring(0, 8)
        });
      }
    }, 30000);
  },

  // 跳转到AI自检
  goToAI() {
    wx.switchTab({
      url: '/pages/ai/ai'
    });
  },

  // 跳转到问诊
  goToClinic() {
    wx.switchTab({
      url: '/pages/clinic/clinic'
    });
  },

  // 跳转到健康报告
  goToReport() {
    wx.showToast({
      title: '功能开发中',
      icon: 'none'
    });
  },

  // 跳转到设备设置
  goToSettings() {
    wx.showToast({
      title: '功能开发中',
      icon: 'none'
    });
  }
});