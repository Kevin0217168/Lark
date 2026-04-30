# Lark 前端开发文档

## 项目简介

Lark 是一个智能鸟厂监控系统前端项目，提供设备管理、实时数据监控、数据分析、雏鸟管理等功能。用户可以通过 Web 界面实时监控鸟厂环境数据、管理设备、查看历史记录、领养雏鸟等。项目支持桌面端和移动端双端适配，提供一致且现代化的用户体验。

## 技术栈

### 核心框架
- **Vue 3.5** - 渐进式 JavaScript 框架，使用 Composition API
- **TypeScript 5.9** - JavaScript 的超集，提供静态类型检查
- **Vite 7.3** - 下一代前端构建工具，提供快速的开发体验

### UI 组件库
- **Element Plus 2.13** - 基于 Vue 3 的企业级组件库
- **@element-plus/icons-vue** - Element Plus 图标库

### 数据可视化
- **ECharts 6** - 强大的图表库，用于数据可视化展示

### HTTP 请求
- **原生 Fetch API** - 封装了请求/响应拦截器、Token 自动刷新、错误处理

### 状态管理
- **Pinia（通过组合式函数）** - `deviceStore.ts` 使用组合式 API 管理设备状态

## 文件树结构

```
├── README.md                  本文档
├── index.html                 HTML 入口文件
├── package.json               项目配置文件
├── vite.config.ts             Vite 配置文件
├── tsconfig.json              TypeScript 配置
├── tsconfig.app.json          TypeScript 应用配置
├── tsconfig.node.json         TypeScript Node 配置
├── public                     公共资源目录
│   ├── banner.jpg             横幅图片
│   └── favicon.ico            网站图标
├── src                        源代码目录
│   ├── App.vue                根组件（管理后台布局）
│   ├── main.ts                Vue 入口文件
│   ├── assets                 静态资源目录
│   │   └── style.css          全局样式文件
│   ├── components             管理后台组件目录
│   │   ├── AuthPage.vue       身份验证页面
│   │   ├── BirdsPage.vue      雏鸟管理页面（管理后台）
│   │   ├── Data.vue           数据管理组件（实时/分析/历史）
│   │   ├── DevicePage.vue     设备管理组件（总览/管理/日志）
│   │   ├── Header.vue         顶部导航栏
│   │   ├── HomePage.vue       总览页面
│   │   ├── ProfilePage.vue    用户信息页面
│   │   ├── Sider.vue          侧边导航栏
│   │   ├── Stream.vue         实时视频流组件
│   │   └── UserManage.vue     用户管理组件
│   ├── cloud                  云养鸟移动端系统
│   │   ├── cloud_App.vue      云养系统根布局
│   │   ├── components
│   │   │   └── BottomNav.vue  底部导航栏
│   │   └── pages
│   │       ├── LoginPage.vue      云养登录
│   │       ├── RegisterPage.vue   云养注册
│   │       ├── CloudHomePage.vue  云养首页
│   │       ├── BirdsPage.vue      雏鸟列表/详情
│   │       ├── BirdDetailPage.vue 雏鸟详情
│   │       ├── AdoptBirdsPage.vue 领养雏鸟
│   │       └── MyPage.vue         个人中心
│   ├── pages
│   │   └── MobilePage.vue     移动端页面容器
│   ├── router
│   │   └── index.ts           路由配置
│   ├── stores
│   │   └── deviceStore.ts     设备状态管理
│   ├── utils
│   │   ├── api.ts             API 请求封装
│   │   └── mobileAdapter.ts   移动端适配工具
│   └── views
│       ├── LoginView.vue      登录视图包装器
│       └── RegisterView.vue   注册视图包装器
```

## 双系统架构

项目包含两个独立的用户系统：

### 1. 管理后台系统（桌面端为主）
- **入口**：`/` → `/Login`
- **用户角色**：root（管理员）、user（普通用户）
- **布局**：Header 顶部导航 + Sider 侧边栏
- **功能模块**：
  - 总览（设备状态、环境数据、错误日志）
  - 数据（实时监控、数据分析、历史数据）
  - 设备（总览、管理、日志）
  - 雏鸟管理
  - 用户管理

### 2. 云养鸟系统（移动端专属）
- **入口**：`/cloud`
- **用户角色**：clouduser（云养用户）
- **布局**：全屏页面 + BottomNav 底部导航
- **功能模块**：
  - 首页（鸟场介绍、可领养雏鸟）
  - 雏鸟（我的雏鸟列表、详情、喂食、环境数据）
  - 我的（个人中心、设置）
- **限制**：桌面端访问会提示"请使用移动端访问"

## 路由结构

```
/                            → 重定向到 /Login
/Login                       → 管理后台登录
/Register                    → 管理后台注册
/Home                        → 总览页面
/Stream                      → 实时视频流
/Device                      → 设备管理
/Data?activeTab=realtime     → 数据管理（实时/分析/历史）
/Profile                     → 个人信息
/UserManage                  → 用户管理（仅管理员）
/birds                       → 雏鸟管理（管理后台）
/cloud                       → 云养系统入口
/cloud/login                 → 云养登录
/cloud/register              → 云养注册
/cloud/home                  → 云养首页
/cloud/birds                 → 我的雏鸟
/cloud/adopt-birds           → 领养雏鸟
/cloud/my                    → 个人中心
/cloud/bird/:id              → 雏鸟详情
```

### 路由守卫
- `clouduser` 角色只能访问 `/cloud/*` 路由，访问其他路由会被拦截并重定向
- 管理后台路由（非 `/cloud`）需要登录状态，未登录重定向到 `/Login`
- `/cloud/login` 和 `/cloud/register` 无需登录即可访问

## API 请求封装

`src/utils/api.ts` 提供了统一的 HTTP 请求封装：

### 特性
- **请求拦截器**：自动添加 `Authorization` Header（JWT Token）
- **响应拦截器**：统一错误处理、HTTP 状态码处理
- **Token 自动刷新**：401 时自动调用 `/api/refresh` 刷新 Token
- **请求队列**：刷新 Token 期间，新请求会进入队列等待
- **登录失效处理**：刷新失败时清除登录状态并跳转登录页

### 使用方法
```typescript
import { api } from '@/utils/api';

// GET 请求
const data = await api.get('/api/devices');

// POST 请求
const result = await api.post('/api/login', { username, password });

// PUT 请求
await api.put('/api/devices/1', { name: '新名称' });

// DELETE 请求
await api.delete('/api/devices/1');
```

## 状态管理

`src/stores/deviceStore.ts` 使用 Vue 3 Composition API 实现状态管理：

### 核心数据
- `devices` - 设备列表（含在线状态、温度、湿度等）
- `deviceHistoryData` - 设备历史数据缓存
- `deviceLogs` - 设备日志
- `lastUpdateTime` / `lastHistoryUpdateTime` - 数据更新时间

### 主要方法
- `fetchDevices()` - 获取所有设备
- `fetchDeviceHistoryData(deviceId?)` - 获取设备历史数据（分组统计）
- `getOrUpdateDeviceHistoryData(deviceId?)` - 获取或更新（带缓存）
- `fetchSensorData(deviceId, timeRange)` - 获取传感器分组数据
- `updateDevice(deviceId, data)` - 更新设备信息

## 数据管理模块（Data.vue）

Data.vue 是核心数据展示组件，包含三个子界面：

### 实时数据（realtime）
- 鸟笼选择器（显示 CAM + C3 设备）
- 实时温度、湿度卡片
- 环境数据环形进度条（PM2.5、分贝、光照、UV）
- WebSocket 视频流（MJPEG）

### 数据分析（analysis）
- 鸟笼选择器
- 时间范围切换（今天 / 两天）
- 6 个传感器图表：温度、湿度、PM2.5、分贝、光照、UV
- 桌面端：并排图表布局
- 移动端：垂直滚动卡片布局

### 历史数据（history）
- 鸟笼选择器
- 合并 CAM + C3 数据的表格
- 显示：时间、温度、湿度、PM2.5、分贝、光照、UV
- 分页展示

## 设备管理模块（DevicePage.vue）

### 总览子界面
- 鸟笼卡片网格（每个卡片显示 CAM + C3 设备）
- 设备在线状态指示

### 管理子界面
- 设备表格（名称、类型、区域、编号、状态）
- 添加/编辑/删除设备
- 按鸟笼筛选

### 日志子界面
- 日志表格（ID、设备、级别、内容、时间）
- 按鸟笼/级别筛选

## 云养鸟系统（cloud/）

### 设计风格
- 自然绿色主题（#8BAD42 品牌色）
- 毛玻璃效果（backdrop-filter）
- 圆角卡片（16-20px）
- 渐变背景
- 底部安全区适配

### 核心页面

#### BirdsPage.vue（雏鸟界面）
- 顶部：日龄、品种、鸟笼信息
- 每日喂食模块（🐣 + 🍞 图标、喂食按钮）
- 环境数据模块（温度、湿度、PM2.5、分贝、光照、UV 环形进度条）
- 基本信息模块（体重、健康状态、备注）

#### 数据绑定
- 雏鸟绑定鸟笼（area + number）
- 自动获取鸟笼内的 CAM/C3 设备数据
- C3 缺失时显示 "未配置" 提示

## 移动端适配

### 检测方式
- User Agent 检测
- 屏幕宽度 < 768px
- 触摸支持检测

### 响应式设计
- Data.vue / DevicePage.vue 等组件内部通过 `isMobile` 计算属性切换模板
- 桌面端：侧边栏导航 + 表格布局
- 移动端：顶部标签切换 + 卡片布局

## 项目特点

1. **双系统架构**：管理后台 + 云养鸟系统，独立路由和权限
2. **鸟笼为中心**：设备按鸟笼（area + number）分组管理
3. **六维传感器**：温度、湿度、PM2.5、分贝、光照、紫外线
4. **实时视频流**：WebSocket MJPEG 流，支持翻转控制
5. **Token 自动刷新**：无感刷新，请求队列保障
6. **现代 UI 设计**：毛玻璃、渐变、圆角、动画过渡
7. **类型安全**：TypeScript 全项目覆盖
