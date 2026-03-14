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
  
  // 初始化时清理过期日志
  cleanOldLogs();
  
  return {
    devices,
    deviceLogs,
    getDevices,
    addDevice,
    updateDevice,
    deleteDevice,
    getDeviceStats,
    getDeviceLogs,
    addDeviceLog,
    cleanOldLogs
  };
};
