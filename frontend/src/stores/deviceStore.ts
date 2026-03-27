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

// 定义数据更新周期（例如5分钟）
const DATA_UPDATE_INTERVAL = 5 * 60 * 1000;

// 数据最后更新时间
const lastUpdateTime = ref<Date | null>(null);

// 历史数据最后更新时间（按设备ID记录）
const lastHistoryUpdateTime = ref<Map<number, Date>>(new Map());

// 将本地时间格式化为ISO8601格式（保持本地时区，后端使用UTC+8）
const formatLocalISO = (date: Date): string => {
  const year = date.getFullYear();
  const month = String(date.getMonth() + 1).padStart(2, '0');
  const day = String(date.getDate()).padStart(2, '0');
  const hours = String(date.getHours()).padStart(2, '0');
  const minutes = String(date.getMinutes()).padStart(2, '0');
  const seconds = String(date.getSeconds()).padStart(2, '0');
  const ms = String(date.getMilliseconds()).padStart(3, '0');
  return `${year}-${month}-${day}T${hours}:${minutes}:${seconds}.${ms}`;
};

// 从后端获取设备数据
const fetchDevices = async () => {
  try {
    const token = localStorage.getItem('accessToken');
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
        lastUpdateTime.value = new Date();
      }
    }
  } catch (error) {
    console.error('获取设备数据失败:', error);
  }
};

// 从后端获取所有设备的传感器数据
const fetchAllSensorsData = async () => {
  try {
    const token = localStorage.getItem('accessToken');
    if (!token) {
      console.error('未登录，无法获取传感器数据');
      return;
    }
    
    // 获取所有设备
    const allDevices = devices.value;
    if (allDevices.length === 0) {
      console.error('没有设备，无法获取传感器数据');
      return;
    }
    
    // 并行获取所有设备的传感器数据
    const promises = allDevices.map(async (device) => {
      if (device.id) {
        // 计算时间范围：最近24小时
        const endTime = new Date();
        const startTime = new Date();
        startTime.setHours(startTime.getHours() - 24);
        
        // 格式化为ISO8601格式（本地时区）
        const startISO = formatLocalISO(startTime);
        const endISO = formatLocalISO(endTime);
        
        // 构建查询参数
        const params = new URLSearchParams({
          start_time: startISO,
          end_time: endISO,
          skip: '0',
          limit: '1'
        });
        
        // 发送请求到后端接口
        const response = await fetch(`/api/sensors/${device.id}?${params.toString()}`, {
          method: 'GET',
          headers: {
            'Accept': 'application/json',
            'Authorization': `Bearer ${token}`
          }
        });
        
        if (response.ok) {
          const data = await response.json();
          if (data.code === 200 && data.data && Array.isArray(data.data) && data.data.length > 0) {
            // 更新设备的温度和湿度
            const latestData = data.data[data.data.length - 1];
            if (latestData) {
              const index = devices.value.findIndex(d => d.id === device.id);
              if (index !== -1) {
                const device = devices.value[index];
                if (device) {
                  device.temperature = latestData.temperature;
                  device.humidity = latestData.humidity;
                }
              }
            }
          }
        }
      }
    });
    
    // 等待所有请求完成
    await Promise.all(promises);
  } catch (error) {
    console.error('获取传感器数据失败:', error);
  }
};

// 检查数据是否需要更新
const shouldUpdateData = () => {
  if (!lastUpdateTime.value) return true;
  const now = new Date();
  return now.getTime() - lastUpdateTime.value.getTime() > DATA_UPDATE_INTERVAL;
};

// 获取或更新设备数据
const getOrUpdateDevices = async () => {
  if (shouldUpdateData()) {
    await fetchDevices();
    // 获取所有设备的传感器数据
    await fetchAllSensorsData();
    // 获取所有设备的历史数据
    const latestDevices = devices.value;
    if (latestDevices.length > 0) {
      for (const device of latestDevices) {
        if (device.id) {
          // 计算时间范围：最近24小时
          const endTime = new Date();
          const startTime = new Date();
          startTime.setHours(startTime.getHours() - 24);
          
          // 格式化为ISO8601格式（本地时区）
          const startISO = formatLocalISO(startTime);
          const endISO = formatLocalISO(endTime);
          
          // 构建查询参数
          const params = new URLSearchParams({
            start_time: startISO,
            end_time: endISO,
            skip: '0',
            limit: '1000'
          });
          
          // 发送请求到后端接口
          const token = localStorage.getItem('accessToken');
          if (true) {
            console.log('向后端发送的请求信息:', {
              url: `/api/sensors/${device.id}?${params.toString()}`,
              method: 'GET',
              headers: {
                'Accept': 'application/json',
                'Authorization': `Bearer ${token}`
              }
            });
            const response = await fetch(`/api/sensors/${device.id}?${params.toString()}`, {
              method: 'GET',
              headers: {
                'Accept': 'application/json',
                'Authorization': `Bearer ${token}`
              }
            });
            
            if (response.ok) {
              const data = await response.json();
              if (data.code === 200 && data.data && Array.isArray(data.data)) {
                // 清空旧数据
                deviceHistoryData.value = deviceHistoryData.value.filter(d => d.deviceId !== device.id);
                // 添加新数据
                data.data.forEach((item: any) => {
                  deviceHistoryData.value.push({
                    deviceId: device.id,
                    timestamp: item.timestamp,
                    temperature: item.temperature,
                    humidity: item.humidity
                  });
                });
              }
            }
          }
        }
      }
    }
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
      
      // 添加随机波动
      const temperature = Number((baseTemp + (Math.random() * 10 - 5)).toFixed(1));
      const humidity = Number((baseHumidity + (Math.random() * 20 - 10)).toFixed(1));
      
      history.push({
        deviceId: device.id,
        timestamp: date.toLocaleString('zh-CN'),
        temperature,
        humidity
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
  
  // 从后端获取设备历史数据（支持获取单个设备或所有设备）
const fetchDeviceHistoryData = async (deviceId?: number) => {
  try {
    const token = localStorage.getItem('accessToken');
    // 使用新的分组统计接口，24小时内分成48组
    const params = new URLSearchParams({
      period: '86400', // 24小时（秒）
      group: '48'      // 分成48组
    });
    
    // 如果指定了设备ID，添加到参数中
    if (deviceId !== undefined) {
      params.append('device_id', deviceId.toString());
      console.log(`[fetchDeviceHistoryData] 设备 ${deviceId} 请求分组统计数据`);
    } else {
      console.log(`[fetchDeviceHistoryData] 请求所有设备的分组统计数据`);
    }
    
    const response = await fetch(`/api/sensors/grouped?${params.toString()}`, {
      method: 'GET',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${token}`
      }
    });
    
    if (response.ok) {
      const data = await response.json();
      if (data.code === 200 && Array.isArray(data.data)) {
        if (deviceId !== undefined) {
          // 清空该设备的旧数据
          deviceHistoryData.value = deviceHistoryData.value.filter(d => d.deviceId !== deviceId);
          // 添加新数据
          data.data.forEach((item: any) => {
            deviceHistoryData.value.push({
              deviceId: deviceId,
              timestamp: item.end_time,
              temperature: Number(item.avg_temperature.toFixed(2)),
              humidity: Number(item.avg_humidity.toFixed(2))
            });
          });
          // 更新该设备的历史数据更新时间
          lastHistoryUpdateTime.value.set(deviceId, new Date());
          console.log(`[fetchDeviceHistoryData] 设备 ${deviceId} 历史数据已更新，共 ${data.data.length} 条记录`);
        } else {
          // 获取所有设备数据时，清空所有旧数据
          deviceHistoryData.value = [];
          // 添加新数据（需要从响应中获取设备ID）
          data.data.forEach((item: any) => {
            // 如果返回的数据包含设备ID，使用它；否则使用0作为默认值
            const itemDeviceId = item.device_id || 0;
            deviceHistoryData.value.push({
              deviceId: itemDeviceId,
              timestamp: item.end_time,
              temperature: Number(item.avg_temperature.toFixed(2)),
              humidity: Number(item.avg_humidity.toFixed(2))
            });
          });
          // 更新所有设备的历史数据更新时间
          const now = new Date();
          devices.value.forEach(device => {
            lastHistoryUpdateTime.value.set(device.id, now);
          });
          console.log(`[fetchDeviceHistoryData] 所有设备历史数据已更新，共 ${data.data.length} 条记录`);
        }
      }
    }
  } catch (error) {
    console.error('获取设备历史数据失败:', error);
  }
};

// 获取或更新设备历史数据（支持获取单个设备或所有设备）
const getOrUpdateDeviceHistoryData = async (deviceId?: number) => {
  if (deviceId !== undefined) {
    // 获取单个设备的历史数据
    const existingData = deviceHistoryData.value.filter(d => d.deviceId === deviceId);
    
    // 检查是否需要更新（使用独立的历史数据更新时间）
    const lastUpdate = lastHistoryUpdateTime.value.get(deviceId);
    const needsUpdate = existingData.length === 0 || !lastUpdate || 
      (new Date().getTime() - lastUpdate.getTime() > DATA_UPDATE_INTERVAL);
    
    if (needsUpdate) {
      console.log(`[getOrUpdateDeviceHistoryData] 设备 ${deviceId} 需要更新历史数据`);
      await fetchDeviceHistoryData(deviceId);
    } else {
      console.log(`[getOrUpdateDeviceHistoryData] 设备 ${deviceId} 使用缓存的历史数据`);
    }
  } else {
    // 获取所有设备的历史数据
    const now = new Date();
    const needsUpdate = deviceHistoryData.value.length === 0 || 
      devices.value.some(device => {
        const lastUpdate = lastHistoryUpdateTime.value.get(device.id);
        return !lastUpdate || (now.getTime() - lastUpdate.getTime() > DATA_UPDATE_INTERVAL);
      });
    
    if (needsUpdate) {
      console.log(`[getOrUpdateDeviceHistoryData] 需要更新所有设备的历史数据`);
      await fetchDeviceHistoryData(); // 不传参数，获取所有设备数据
    } else {
      console.log(`[getOrUpdateDeviceHistoryData] 使用缓存的所有设备历史数据`);
    }
  }
};

// 获取设备历史数据
const getDeviceHistoryData = (deviceId?: number) => {
  if (deviceId) {
    return deviceHistoryData.value.filter(data => data.deviceId === deviceId);
  }
  return deviceHistoryData.value;
};

// 从后端接口获取传感器数据
const fetchSensorData = async (deviceId: number) => {
  const times: string[] = [];
  const temperatureValues: number[] = [];
  const humidityValues: number[] = [];
  
  try {
    // 获取登录token
    const token = localStorage.getItem('accessToken');
    if (!token) {
      console.error('未登录，无法获取传感器数据');
      return { times, temperatureValues, humidityValues };
    }
    
    // 使用新的分组统计接口，24小时内分成48组
    const params = new URLSearchParams({
      period: '86400', // 24小时（秒）
      group: '48',     // 分成48组
      device_id: deviceId.toString()
    });
    
    // 发送请求到后端接口
    const response = await fetch(`/api/sensors/grouped?${params.toString()}`, {
      method: 'GET',
      headers: {
        'Accept': 'application/json',
        'Authorization': `Bearer ${token}`
      }
    });
    
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }
    
    const data = await response.json();
    
    // 处理响应数据
    if (data.code === 200 && data.data && Array.isArray(data.data)) {
      data.data.forEach((item: any) => {
        const date = new Date(item.end_time);
        times.push(`${date.getHours().toString().padStart(2, '0')}:${date.getMinutes().toString().padStart(2, '0')}`);
        temperatureValues.push(Number(item.avg_temperature.toFixed(2)));
        humidityValues.push(Number(item.avg_humidity.toFixed(2)));
      });
    }
  } catch (error) {
    console.error('获取传感器数据失败:', error);
  }
  
  return { times, temperatureValues, humidityValues };
};
  
  // 计算所有设备的平均值数据（支持自定义时间范围，默认24小时，按30分钟间隔采样，使用所有设备数据做平均）
  const getDeviceAverageData = (hours: number = 24) => {
    const times: string[] = [];
    const temperatureValues: number[] = [];
    const humidityValues: number[] = [];
    
    // 获取所有设备的历史数据
    const allHistoryData = deviceHistoryData.value;
    
    if (allHistoryData.length === 0) {
      return { times, temperatureValues, humidityValues };
    }
    
    // 首先，找到数据的时间范围，然后生成30分钟间隔的时间点
    const now = new Date();
    
    // 生成指定小时内的30分钟间隔时间点
    const timePoints: Date[] = [];
    const pointCount = hours * 2; // 每小时2个点（30分钟一个点）
    for (let i = pointCount - 1; i >= 0; i--) {
      const pointTime = new Date(now);
      pointTime.setMinutes(Math.floor(pointTime.getMinutes() / 30) * 30 - i * 30);
      pointTime.setSeconds(0);
      pointTime.setMilliseconds(0);
      timePoints.push(pointTime);
    }
    
    // 对每个时间点，取前后15分钟范围内的数据计算平均值
    timePoints.forEach(pointTime => {
      const timeKey = `${pointTime.getHours().toString().padStart(2, '0')}:${pointTime.getMinutes().toString().padStart(2, '0')}`;
      
      // 定义时间范围：前后15分钟
      const startTimeRange = new Date(pointTime);
      startTimeRange.setMinutes(startTimeRange.getMinutes() - 15);
      
      const endTimeRange = new Date(pointTime);
      endTimeRange.setMinutes(endTimeRange.getMinutes() + 15);
      
      // 收集这个时间范围内的所有设备数据
      const temps: number[] = [];
      const humids: number[] = [];
      const deviceSet = new Set<number>();
      
      allHistoryData.forEach(data => {
        const dataTime = new Date(data.timestamp);
        
        // 检查数据是否在时间范围内
        if (dataTime >= startTimeRange && dataTime <= endTimeRange) {
          // 确保每个设备只计算一次（取该设备在时间范围内的最后一条数据）
          if (!deviceSet.has(data.deviceId)) {
            deviceSet.add(data.deviceId);
            temps.push(data.temperature);
            humids.push(data.humidity);
          }
        }
      });
      
      // 如果有数据，计算平均值
      if (temps.length > 0) {
        times.push(timeKey);
        
        const tempAvg = temps.reduce((sum, val) => sum + val, 0) / temps.length;
        const humidityAvg = humids.reduce((sum, val) => sum + val, 0) / humids.length;
        
        temperatureValues.push(Number(tempAvg.toFixed(2)));
        humidityValues.push(Number(humidityAvg.toFixed(2)));
      }
    });
    
    return {
      times,
      temperatureValues,
      humidityValues
    };
  };

  // 计算24小时内所有设备的温度湿度的最高最低值
    const getDeviceExtremaData = () => {
      // 获取所有设备的历史数据
      const allHistoryData = deviceHistoryData.value;
      
      if (allHistoryData.length === 0) {
        return {
          temperature: { max: 0, min: 0 },
          humidity: { max: 0, min: 0 }
        };
      }
      
      // 过滤掉无效数据（温度为0或湿度为0的数据）
      const validData = allHistoryData.filter(data => data.temperature !== 0 && data.humidity !== 0);
      
      if (validData.length === 0) {
        return {
          temperature: { max: 0, min: 0 },
          humidity: { max: 0, min: 0 }
        };
      }
      
      // 计算温度最高最低
      const temperatures = validData.map(data => data.temperature);
      const tempMax = Math.max(...temperatures);
      const tempMin = Math.min(...temperatures);
      
      // 计算湿度最高最低
      const humidities = validData.map(data => data.humidity);
      const humidityMax = Math.max(...humidities);
      const humidityMin = Math.min(...humidities);
      
      return {
        temperature: { max: tempMax, min: tempMin },
        humidity: { max: humidityMax, min: humidityMin }
      };
    };

  // 获取实时温湿度数据（2分钟内，1组）
  const fetchRealtimeSensorData = async (deviceId: number) => {
    try {
      const token = localStorage.getItem('accessToken');
      if (!token) {
        console.error('未登录，无法获取实时传感器数据');
        return null;
      }

      // 使用分组统计接口，2分钟内，1组
      const params = new URLSearchParams({
        period: '120', // 2分钟（秒）
        group: '1',    // 1组
        device_id: deviceId.toString()
      });

      const response = await fetch(`/api/sensors/grouped?${params.toString()}`, {
        method: 'GET',
        headers: {
          'Accept': 'application/json',
          'Authorization': `Bearer ${token}`
        }
      });

      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`);
      }

      const data = await response.json();

      if (data.code === 200 && data.data && Array.isArray(data.data) && data.data.length > 0) {
        const item = data.data[0];
        return {
          temperature: Number(item.avg_temperature.toFixed(2)),
          humidity: Number(item.avg_humidity.toFixed(2))
        };
      }

      return null;
    } catch (error) {
      console.error('获取实时传感器数据失败:', error);
      return null;
    }
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
    fetchDeviceHistoryData,
    getOrUpdateDeviceHistoryData,
    getDeviceAverageData,
    getDeviceExtremaData,
    fetchSensorData,
    fetchRealtimeSensorData,
    fetchAllSensorsData,
    fetchDevices,
    getOrUpdateDevices,
    setFullscreen: (value: boolean) => {
      isFullscreen.value = value;
    },
    getFullscreen: () => isFullscreen.value
  };
};
