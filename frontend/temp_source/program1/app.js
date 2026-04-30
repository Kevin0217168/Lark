// app.js
App({
  globalData: {
    // 用户信息
    userInfo: {
      avatar: '',
      nickname: '',
      id: ''
    },
    // 设备连接状态
    deviceStatus: {
      isConnected: false,
      deviceName: '',
      deviceType: '',
      lastHeartbeat: null
    },
    // 当前设备数据
    deviceData: {
      temperature: 0,
      humidity: 0,
      foodRemaining: 0,
      feedCountToday: 0,
      lastFeedTime: null
    },
    // 预警信息
    alertList: [],
    // 云开发环境ID（需替换为实际环境ID）
    cloudEnv: ''
  },

  onLaunch() {
    console.log('伴鸟小程序启动');
    this.initCloud();
    this.checkDeviceStatus();
    this.getUserInfo();
  },

  // 初始化云开发
  initCloud() {
    try {
      if (!wx.cloud) {
        console.error('请使用 2.2.3 或以上的基础库以使用云能力');
        return;
      }
      // 云开发初始化（实际使用时需要替换 envId）
      // wx.cloud.init({
      //   env: this.globalData.cloudEnv,
      //   traceUser: true
      // });
    } catch (error) {
      console.error('云开发初始化失败:', error);
    }
  },

  // 检查设备连接状态
  checkDeviceStatus() {
    // 模拟设备连接检查
    const mockCheck = setInterval(() => {
      this.updateDeviceData();
    }, 30000); // 30秒检查一次
  },

  // 更新设备数据
// 更新设备数据（连接真实 Node.js 后端）
updateDeviceData() {
  const that = this; // 存一下 this，防止作用域丢失

  // 发起网络请求，呼叫你的本地服务器
  wx.request({
    url: 'http://localhost:3000/api/parrot/status', // 确保你后端的这段代码还在运行
    method: 'GET',
    success: (res) => {
      // 如果成功拿到后端的数据
      if (res.data && res.data.data) {
        const backendData = res.data.data;
        
        console.log("成功获取后端真数据：", backendData);

        // 1. 用后端的真数据，替换掉原来的 Math.random()
        const newData = {
          temperature: backendData.temperature,
          humidity: backendData.humidity,
          foodRemaining: backendData.foodRemaining,
          feedCountToday: 3, // 如果后端还没写这个接口，可以先写死
          lastFeedTime: new Date().getTime()
        };

        // 2. 更新到全局变量
        that.globalData.deviceData = newData;
        
        // 3. 用真实的温度去触发你的预警检查逻辑！
        that.checkAlerts(newData);
      }
    },
    fail: (error) => {
      console.error("连接 Node.js 后端失败，请检查服务器是否开启", error);
    }
  });
},

  // 检查预警
  checkAlerts(data) {
    const alerts = [];
    
    if (data.temperature > 28 || data.temperature < 22) {
      alerts.push({
        type: 'danger',
        message: `温度异常：${data.temperature.toFixed(1)}℃`,
        time: new Date().getTime()
      });
    } else if (data.temperature > 27 || data.temperature < 23) {
      alerts.push({
        type: 'warning',
        message: `温度波动：${data.temperature.toFixed(1)}℃`,
        time: new Date().getTime()
      });
    }

    if (data.humidity > 75 || data.humidity < 55) {
      alerts.push({
        type: 'warning',
        message: `湿度异常：${data.humidity.toFixed(0)}%RH`,
        time: new Date().getTime()
      });
    }

    if (data.foodRemaining < 20) {
      alerts.push({
        type: 'danger',
        message: '余粮不足，请及时添加',
        time: new Date().getTime()
      });
    } else if (data.foodRemaining < 40) {
      alerts.push({
        type: 'info',
        message: `余粮剩余：${data.foodRemaining}%`,
        time: new Date().getTime()
      });
    }

    this.globalData.alertList = alerts;
  },

  // 获取用户信息
  getUserInfo() {
    wx.getSetting({
      success: (res) => {
        if (res.authSetting['scope.userInfo']) {
          wx.getUserInfo({
            success: (userRes) => {
              this.globalData.userInfo = userRes.userInfo;
            }
          });
        }
      }
    });
  }
})
