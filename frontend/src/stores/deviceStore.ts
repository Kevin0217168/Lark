import { ref, computed } from 'vue';

export interface Device {
  id: number;
  name: string;
  status: string; // standby待机，没有任何观看者；stream推流
  isOnline: boolean; // 是否在线
  createTime: string;
  area?: string; // 所属区域
  number?: number; // 设备编号
  temperature?: number; // 温度数据
  humidity?: number; // 湿度数据
  videoStreamUrl?: string; // 视频流数据
}

export interface DeviceHistoryData {
  deviceId: number;
  timestamp: string;
  temperature: number;
  humidity: number;
  quality: number; // 环境质量指数
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

const devices = ref<Device[]>([]);

// 从后端获取设备数据
const fetchDevices = async () => {
  try {
    const token = localStorage.getItem('token');
    const response = await fetch('/api/devices', {
      method: 'GET',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${token}`
      }
    });
    
    if (response.ok) {
      const data = await response.json();
      if (data.code === 200 && Array.isArray(data.data)) {
        devices.value = data.data;
      }
    }
  } catch (error) {
    console.error('获取设备数据失败:', error);
  }
};

// 初始化时获取设备数据
fetchDevices();

const deviceLogs = ref<DeviceLog[]>([
  { id: 1, deviceId: 1, deviceName: '设备1', level: 'info', message: '设备上线', timestamp: '2026/03/14 08:00:00' },
  { id: 2, deviceId: 1, deviceName: '设备1', level: 'info', message: '开始采集数据', timestamp: '2026/03/14 08:01:00' },
  { id: 3, deviceId: 1, deviceName: '设备1', level: 'warning', message: '温度偏高', timestamp: '2026/03/14 08:30:00' },
  { id: 4, deviceId: 2, deviceName: '设备2', level: 'error', message: '设备离线', timestamp: '2026/03/14 09:00:00' },
]);

// 设备位置存储
const devicePositions = ref<DevicePosition[]>([]);

// 设备历史数据存储
const deviceHistoryData = ref<DeviceHistoryData[]>([]);

// 全屏状态
const isFullscreen = ref(false);

// 生成设备历史数据，后期改成从后端获取历史数据
const generateDeviceHistoryData = () => {
  const history: DeviceHistoryData[] = [];
  
  // 为每个设备生成最近72小时（3天）的历史数据
  devices.value.forEach(device => {
    for (let i = 71; i >= 0; i--) {
      const date = new Date();
      date.setHours(date.getHours() - i);
      
      // 根据设备ID生成不同的基础数据
      const baseTemp = 25 + (device.id * 2);
      const baseHumidity = 60 + (device.id * 5);
      const baseQuality = 80 + (device.id * 3);
      
      // 添加随机波动
      const temperature = Number((baseTemp + (Math.random() * 10 - 5)).toFixed(1));
      const humidity = Number((baseHumidity + (Math.random() * 20 - 10)).toFixed(1));
      const quality = Number((baseQuality + (Math.random() * 30 - 15)).toFixed(1));
      
      history.push({
        deviceId: device.id,
        timestamp: date.toLocaleString('zh-CN'),
        temperature,
        humidity,
        quality
      });
    }
  });
  
  deviceHistoryData.value = history;
};

// 初始化时生成历史数据
generateDeviceHistoryData();

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
      const online = devices.value.filter(d => d.isOnline).length;
      const offline = devices.value.filter(d => !d.isOnline).length;
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
  
  // 获取设备历史数据
  const getDeviceHistoryData = (deviceId?: number) => {
    if (deviceId) {
      return deviceHistoryData.value.filter(data => data.deviceId === deviceId);
    }
    return deviceHistoryData.value;
  };
  
  // 计算所有设备的平均值数据（最近24小时）
  const getDeviceAverageData = () => {
    const times: string[] = [];
    const temperatureValues: number[] = [];
    const humidityValues: number[] = [];
    const qualityValues: number[] = [];
    
    // 获取所有设备的历史数据
    const allHistoryData = deviceHistoryData.value;
    
    // 只取最近24小时的数据
    const filteredData = allHistoryData.slice(-24 * devices.value.length);
    
    // 按时间分组
    const timeGroups: Record<string, { temp: number[], humidity: number[], quality: number[] }> = {};
    
    filteredData.forEach(data => {
      const date = new Date(data.timestamp);
      const hourKey = `${date.getHours().toString().padStart(2, '0')}:00`;
      
      if (!timeGroups[hourKey]) {
        timeGroups[hourKey] = { temp: [], humidity: [], quality: [] };
      }
      
      timeGroups[hourKey].temp.push(data.temperature);
      timeGroups[hourKey].humidity.push(data.humidity);
      timeGroups[hourKey].quality.push(data.quality);
    });
    
    // 生成最近24小时的时间
    for (let i = 23; i >= 0; i--) {
      const date = new Date();
      date.setHours(date.getHours() - i);
      const hourKey = `${date.getHours().toString().padStart(2, '0')}:00`;
      times.push(hourKey);
      
      // 计算每个时间点的平均值
      const group = timeGroups[hourKey];
      if (group) {
        const tempAvg = group.temp.length > 0 ? group.temp.reduce((sum, val) => sum + val, 0) / group.temp.length : 0;
        const humidityAvg = group.humidity.length > 0 ? group.humidity.reduce((sum, val) => sum + val, 0) / group.humidity.length : 0;
        const qualityAvg = group.quality.length > 0 ? group.quality.reduce((sum, val) => sum + val, 0) / group.quality.length : 0;
        
        temperatureValues.push(Number(tempAvg.toFixed(1)));
        humidityValues.push(Number(humidityAvg.toFixed(1)));
        qualityValues.push(Number(qualityAvg.toFixed(1)));
      } else {
        temperatureValues.push(0);
        humidityValues.push(0);
        qualityValues.push(0);
      }
    }
    
    return { times, temperatureValues, humidityValues, qualityValues };
  };
  
  return {
    devices,
    deviceLogs,
    deviceHistoryData,
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
    getDeviceHistoryData,
    getDeviceAverageData,
    fetchDevices,
    setFullscreen: (value: boolean) => {
      isFullscreen.value = value;
    },
    getFullscreen: () => isFullscreen.value
  };
};
