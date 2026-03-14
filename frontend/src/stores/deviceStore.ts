import { ref, computed } from 'vue';

export interface Device {
  id: number;
  name: string;
  status: string;
  createTime: string;
  temperature?: number; // 温度数据
  humidity?: number; // 湿度数据
  videoStreamUrl?: string; // 视频流数据
}

export interface DeviceLog {
  id: number;
  deviceId: number;
  deviceName: string;
  level: 'info' | 'warning' | 'error';
  message: string;
  timestamp: string;
}

export interface DevicePosition {
  deviceId: number;
  x: number; // 0-100 百分比
  y: number; // 0-100 百分比
}

const devices = ref<Device[]>([
  { 
    id: 1, 
    name: '设备1', 
    status: 'online', 
    createTime: '2026/03/14 01:28:24',
    temperature: 25.5,
    humidity: 45.0,
    videoStreamUrl: 'http://example.com/stream1'
  },
  { 
    id: 2, 
    name: '设备2', 
    status: 'offline', 
    createTime: '2026/03/14 01:28:24',
    temperature: 22.0,
    humidity: 50.0,
    videoStreamUrl: 'http://example.com/stream2'
  },
]);

const deviceLogs = ref<DeviceLog[]>([
  { id: 1, deviceId: 1, deviceName: '设备1', level: 'info', message: '设备上线', timestamp: '2026/03/14 08:00:00' },
  { id: 2, deviceId: 1, deviceName: '设备1', level: 'info', message: '开始采集数据', timestamp: '2026/03/14 08:01:00' },
  { id: 3, deviceId: 1, deviceName: '设备1', level: 'warning', message: '温度偏高', timestamp: '2026/03/14 08:30:00' },
  { id: 4, deviceId: 2, deviceName: '设备2', level: 'error', message: '设备离线', timestamp: '2026/03/14 09:00:00' },
]);

// 设备位置存储
const devicePositions = ref<DevicePosition[]>([]);

// 全屏状态
const isFullscreen = ref(false);

export const useDeviceStore = () => {
  const getDevices = () => devices.value;
  
  const addDevice = (device: Device) => {
    devices.value.push(device);
  };
  
  const updateDevice = (id: number, updates: Partial<Device>) => {
    const index = devices.value.findIndex(d => d.id === id);
    if (index !== -1) {
      const device = devices.value[index];
      if (device) {
        Object.assign(device, updates);
      }
    }
  };
  
  const deleteDevice = (id: number) => {
    const index = devices.value.findIndex(d => d.id === id);
    if (index !== -1) {
      devices.value.splice(index, 1);
    }
  };
  
  const getDeviceStats = () => {
    return computed(() => {
      const total = devices.value.length;
      const online = devices.value.filter(d => d.status === 'online').length;
      const offline = devices.value.filter(d => d.status === 'offline').length;
      const normal = online;
      const abnormal = 0;
      
      return {
        total,
        online,
        offline,
        normal,
        abnormal
      };
    });
  };
  
  // 日志相关方法
  const getDeviceLogs = (deviceId?: number) => {
    if (deviceId) {
      return deviceLogs.value.filter(log => log.deviceId === deviceId);
    }
    return deviceLogs.value;
  };
  
  const cleanOldLogs = () => {
    const sevenDaysAgo = new Date();
    sevenDaysAgo.setDate(sevenDaysAgo.getDate() - 7);
    
    deviceLogs.value = deviceLogs.value.filter(log => {
      const logDate = new Date(log.timestamp);
      return logDate >= sevenDaysAgo;
    });
  };
  
  const addDeviceLog = (log: Omit<DeviceLog, 'id' | 'timestamp'>) => {
    const newLog: DeviceLog = {
      ...log,
      id: deviceLogs.value.length + 1,
      timestamp: new Date().toLocaleString('zh-CN')
    };
    deviceLogs.value.unshift(newLog); // 新日志添加到开头
    cleanOldLogs(); // 添加新日志后清理过期日志
  };
  
  // 设备位置相关方法
  const getDevicePositions = () => devicePositions.value;
  
  const addDevicePosition = (position: DevicePosition) => {
    const existingIndex = devicePositions.value.findIndex(p => p.deviceId === position.deviceId);
    if (existingIndex !== -1) {
      devicePositions.value[existingIndex] = position;
    } else {
      devicePositions.value.push(position);
    }
  };
  
  const updateDevicePosition = (deviceId: number, x: number, y: number) => {
    const existingIndex = devicePositions.value.findIndex(p => p.deviceId === deviceId);
    if (existingIndex !== -1) {
      const position = devicePositions.value[existingIndex];
      if (position) {
        position.x = x;
        position.y = y;
      }
    } else {
      devicePositions.value.push({ deviceId, x, y });
    }
  };
  
  const removeDevicePosition = (deviceId: number) => {
    const index = devicePositions.value.findIndex(p => p.deviceId === deviceId);
    if (index !== -1) {
      devicePositions.value.splice(index, 1);
    }
  };
  
  const clearDevicePositions = () => {
    devicePositions.value = [];
  };
  
  // 初始化时清理过期日志
  cleanOldLogs();
  
  return {
    devices,
    deviceLogs,
    isFullscreen,
    getDevices,
    addDevice,
    updateDevice,
    deleteDevice,
    getDeviceStats,
    getDeviceLogs,
    addDeviceLog,
    cleanOldLogs,
    getDevicePositions,
    addDevicePosition,
    updateDevicePosition,
    removeDevicePosition,
    clearDevicePositions,
    setFullscreen: (value: boolean) => {
      isFullscreen.value = value;
    },
    getFullscreen: () => isFullscreen.value
  };
};
