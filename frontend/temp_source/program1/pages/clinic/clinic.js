// pages/clinic/clinic.js
Page({
  data: {
    // 搜索筛选
    searchKeyword: '',
    selectedFilter: 'all',
    sortType: 'rating',
    
    // 电子报告
    unreadReports: 2,
    latestReport: '2026/2/20 健康体检报告',
    hasAdvice: true,
    
    // 医生列表
    doctors: [
      {
        id: 1,
        name: '张伟华',
        title: '首席兽医 / 异宠专科',
        specialty: '鸟类',
        avatar: 'https://via.placeholder.com/120?text=Dr.%E5%BC%A0',
        rating: 4.9,
        consultCount: 12580,
        successRate: 98,
        price: 68,
        isOnline: true,
        intro: '从事异宠临床诊疗15年，擅长鸟类疾病诊断与治疗，发表论文30余篇，曾获全国优秀兽医称号。'
      },
      {
        id: 2,
        name: '李晓梅',
        title: '资深兽医 / 爬行类专家',
        specialty: '爬行',
        avatar: 'https://via.placeholder.com/120?text=Dr.%E6%9D%8E',
        rating: 4.8,
        consultCount: 8920,
        successRate: 96,
        price: 58,
        isOnline: true,
        intro: '专注爬行动物健康管理12年，擅长蜥蜴、龟类疾病防治，拥有丰富的异宠手术经验。'
      },
      {
        id: 3,
        name: '王建国',
        title: '高级兽医 / 两栖类专家',
        specialty: '两栖',
        avatar: 'https://via.placeholder.com/120?text=Dr.%E7%8E%8B',
        rating: 4.7,
        consultCount: 6540,
        successRate: 95,
        price: 48,
        isOnline: false,
        intro: '两栖动物疾病专家，擅长蛙类、蝾螈的健康管理与疾病诊断，多次参与国际学术交流。'
      }
    ],
    filteredDoctors: [],
    hasMore: false,
    loadingMore: false
  },

  onLoad() {
    this.filterDoctors()
  },

  // 搜索输入
  onSearchInput(e) {
    this.setData({ searchKeyword: e.detail.value })
    this.filterDoctors()
  },

  // 选择筛选
  selectFilter(e) {
    const filter = e.currentTarget.dataset.filter
    this.setData({ selectedFilter: filter })
    this.filterDoctors()
  },

  // 切换排序
  toggleSort() {
    const newSortType = this.data.sortType === 'rating' ? 'consultCount' : 'rating'
    this.setData({ sortType: newSortType })
    this.filterDoctors()
  },

  // 筛选医生
  filterDoctors() {
    let filtered = [...this.data.doctors]
    
    // 关键词搜索
    if (this.data.searchKeyword) {
      const keyword = this.data.searchKeyword.toLowerCase()
      filtered = filtered.filter(doctor => 
        doctor.name.toLowerCase().includes(keyword) ||
        doctor.specialty.toLowerCase().includes(keyword) ||
        doctor.title.toLowerCase().includes(keyword)
      )
    }
    
    // 分类筛选
    if (this.data.selectedFilter !== 'all') {
      filtered = filtered.filter(doctor => 
        doctor.specialty === this.data.selectedFilter
      )
    }
    
    // 排序
    if (this.data.sortType === 'rating') {
      filtered.sort((a, b) => b.rating - a.rating)
    } else {
      filtered.sort((a, b) => b.consultCount - a.consultCount)
    }
    
    this.setData({ filteredDoctors: filtered })
  },

  // 急诊求助
  handleEmergency() {
    wx.showModal({
      title: '在线急诊',
      content: '是否立即连接在线急诊通道？紧急情况将优先响应。',
      confirmText: '立即连接',
      success: (res) => {
        if (res.confirm) {
          wx.showToast({
            title: '正在连接急诊...',
            icon: 'loading'
          })
          
          setTimeout(() => {
            wx.showToast({
              title: '急诊通道已连接',
              icon: 'success'
            })
          }, 2000)
        }
      }
    })
  },

  // 查看电子报告
  gotoReports() {
    wx.showToast({
      title: '打开健康报告',
      icon: 'none'
    })
  },

  // 查看医生详情
  viewDoctorDetail(e) {
    const id = e.currentTarget.dataset.id
    wx.showToast({
      title: '医生详情',
      icon: 'none'
    })
  },

  // 预约医生
  bookDoctor(e) {
    const id = e.currentTarget.dataset.id
    const doctor = this.data.doctors.find(d => d.id === id)
    
    if (!doctor.isOnline) {
      wx.showToast({
        title: '医生当前离线',
        icon: 'none'
      })
      return
    }
    
    wx.showModal({
      title: `预约${doctor.name}医生`,
      content: `咨询费用: ¥${doctor.price}\n\n是否确认预约？`,
      confirmText: '确认预约',
      success: (res) => {
        if (res.confirm) {
          wx.showToast({
            title: '预约成功',
            icon: 'success'
          })
        }
      }
    })
  }
})
