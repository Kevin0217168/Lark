import { ref, computed } from 'vue';

export interface Device {
  id: number;
  name: string;
  status: string;
  createTime: string;
}

const devices = ref<Device[]>([
  { id: 1, name: '设备1', status: 'online', createTime: '2026/03/14 01:28:24' },
  { id: 2, name: '设备2', status: 'offline', createTime: '2026/03/14 01:28:24' },
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
  
  return {
    devices,
    getDevices,
    addDevice,
    updateDevice,
    deleteDevice,
    getDeviceStats
  };
};
