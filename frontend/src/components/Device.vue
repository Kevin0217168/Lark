<template>
  <el-card class="device-card">
    <h3 class="title">设备管理</h3>
    
    <!-- 总览视图 -->
    <div v-if="activeTab === 'overview'">
      <h4>设备总览</h4>
      <el-row :gutter="20">
        <el-col :span="8" v-for="device in devices" :key="device.id">
          <el-card :class="device.isOnline ? 'device-online' : 'device-offline'">
            <h5>{{ device.name }}</h5>
            <p>区域: {{ device.area || '未设置' }}</p>
            <p>编号: {{ device.number || '-' }}</p>
            <p>类型: {{ device.device_type || '未设置' }}</p>
            <p>固件版本: {{ deviceVersions[device.id] || '加载中...' }}</p>
            <p>
              状态:
              <el-tag :type="device.isOnline ? 'success' : 'danger'">
                {{ device.isOnline ? '在线' : '离线' }}
              </el-tag>
            </p>
          </el-card>
        </el-col>
      </el-row>
      <div v-if="devices.length === 0" class="no-devices">
        <p>暂无设备数据</p>
      </div>
    </div>
    
    <!-- 管理视图 -->
    <div v-else-if="activeTab === 'management'">
      <div class="management-header">
        <h4>设备管理</h4>
        <el-button type="primary" @click="showAddDialog">添加设备</el-button>
      </div>
      
      <!-- 筛选查询 -->
      <div class="filter-section">
        <el-input 
          v-model="filterForm.name" 
          placeholder="设备名称" 
          style="width: 200px; margin-right: 10px;"
          clearable
        />
        <el-select 
          v-model="filterForm.area" 
          placeholder="所属区域" 
          style="width: 200px; margin-right: 10px;"
          clearable
          multiple
          collapse-tags
          collapse-tags-tooltip
        >
          <el-option 
            v-for="area in uniqueAreas" 
            :key="area" 
            :label="area" 
            :value="area" 
          />
        </el-select>
        <el-select 
          v-model="filterForm.isOnline" 
          placeholder="在线状态" 
          style="width: 150px; margin-right: 10px;"
          clearable
        >
          <el-option label="在线" :value="true" />
          <el-option label="离线" :value="false" />
        </el-select>
        <el-button @click="resetFilter">重置</el-button>
      </div>
      
      <el-table :data="filteredDevices" style="width: 100%" border>
        <el-table-column prop="id" label="设备ID" min-width="80" />
        <el-table-column prop="name" label="设备名称" min-width="150" />
        <el-table-column prop="area" label="所属区域" min-width="120" />
        <el-table-column prop="number" label="设备编号" min-width="100" />
        <el-table-column prop="device_type" label="设备类型" min-width="120" />
        <el-table-column label="固件版本" min-width="120">
          <template #default="scope">
            <span v-if="deviceVersions[scope.row.id]">{{ deviceVersions[scope.row.id] }}</span>
            <span v-else class="loading">加载中...</span>
          </template>
        </el-table-column>
        <el-table-column label="在线状态" min-width="120">
          <template #default="scope">
            <el-tag :type="scope.row.isOnline ? 'success' : 'danger'">
              {{ scope.row.isOnline ? '在线' : '离线' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="推流状态" min-width="120">
          <template #default="scope">
            <el-tag :type="scope.row.status === 'stream' ? 'success' : 'info'">
              {{ scope.row.status === 'stream' ? '推流中' : '待机' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="操作" min-width="300" fixed="right">
          <template #default="scope">
            <el-button size="small" @click="handleEdit(scope.row)">编辑</el-button>
            <el-button size="small" type="success" @click="handleUpdateFirmware(scope.row)">更新固件</el-button>
            <el-button size="small" type="warning" @click="handleRestart(scope.row)">重启</el-button>
            <el-button size="small" type="danger" @click="handleDelete(scope.row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
      
      <div v-if="filteredDevices.length === 0" class="no-devices">
        <p>暂无设备数据</p>
      </div>
    </div>
    
    <!-- 日志视图 -->
    <div v-else-if="activeTab === 'logs'">
      <div class="logs-header">
        <h4>设备日志</h4>
      </div>
      
      <!-- 筛选查询 -->
      <div class="filter-section">
        <el-input 
          v-model="logFilterForm.deviceName" 
          placeholder="设备名称" 
          style="width: 200px; margin-right: 10px;"
          clearable
        />
        <el-select 
          v-model="logFilterForm.area" 
          placeholder="所属区域" 
          style="width: 200px; margin-right: 10px;"
          clearable
          multiple
          collapse-tags
          collapse-tags-tooltip
        >
          <el-option 
            v-for="area in uniqueAreas" 
            :key="area" 
            :label="area" 
            :value="area" 
          />
        </el-select>
        <el-select 
          v-model="logFilterForm.level" 
          placeholder="日志级别" 
          style="width: 150px; margin-right: 10px;"
          clearable
          multiple
          collapse-tags
          collapse-tags-tooltip
        >
          <el-option label="信息" value="info" />
          <el-option label="警告" value="warning" />
          <el-option label="错误" value="error" />
        </el-select>
        <el-button @click="resetLogFilter">重置</el-button>
      </div>
      
      <el-table :data="filteredLogsByFilter" style="width: 100%" border>
        <el-table-column prop="id" label="日志ID" min-width="80" />
        <el-table-column prop="deviceName" label="设备名称" min-width="120" />
        <el-table-column prop="level" label="级别" min-width="100">
          <template #default="scope">
            <el-tag 
              :type="{
                'info': 'info',
                'warning': 'warning',
                'error': 'danger'
              }[scope.row.level as 'info' | 'warning' | 'error']"
            >
              {{ {
                'info': '信息',
                'warning': '警告',
                'error': '错误'
              }[scope.row.level as 'info' | 'warning' | 'error'] }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="message" label="日志内容" min-width="300" />
        <el-table-column prop="timestamp" label="时间" min-width="180" />
      </el-table>
      
      <div v-if="filteredLogsByFilter.length === 0" class="no-logs">
        <p>暂无日志数据</p>
      </div>
    </div>
  </el-card>
  
  <!-- 添加/编辑设备对话框 -->
  <el-dialog v-model="dialogVisible" :title="isEdit ? '编辑设备' : '添加设备'" width="500px">
    <el-form :model="deviceForm" label-width="100px">
      <el-form-item label="设备名称">
        <el-input v-model="deviceForm.name" placeholder="请输入设备名称" />
      </el-form-item>
      <el-form-item label="设备密钥" v-if="!isEdit">
        <el-input v-model="deviceForm.secret" placeholder="请输入设备密钥" />
      </el-form-item>
      <el-form-item label="所属区域">
        <el-input v-model="deviceForm.area" placeholder="请输入所属区域" />
      </el-form-item>
      <el-form-item label="设备编号">
        <el-input-number v-model="deviceForm.number" :min="1" placeholder="请输入设备编号" />
      </el-form-item>
      <el-form-item label="设备类型">
        <el-input v-model="deviceForm.device_type" placeholder="请输入设备类型" />
      </el-form-item>
    </el-form>
    <template #footer>
      <el-button @click="dialogVisible = false">取消</el-button>
      <el-button type="primary" @click="handleSave">确定</el-button>
    </template>
  </el-dialog>
</template>

<script lang="ts" setup>
import { ref, computed, onMounted } from 'vue';
import { ElMessage, ElMessageBox } from 'element-plus';
import { useDeviceStore, type Device, type DeviceLog } from '../stores/deviceStore';

// 接收从父组件传递的 activeTab 属性
defineProps<{
  activeTab: string;
}>();

const store = useDeviceStore();
const { devices, getDeviceLogs, addDevice, updateDevice, deleteDevice } = store;

// 获取设备列表（带固件版本查询）
const fetchDevices = async () => {
  try {
    await store.fetchDevices();
    // 获取每个设备的固件版本
    devices.value.forEach((device: Device) => {
      fetchDeviceVersion(device.id);
    });
  } catch (error) {
    console.error('获取设备列表出错:', error);
    ElMessage.error('获取设备列表失败，请检查网络连接');
  }
};

// 获取设备固件版本
const fetchDeviceVersion = async (deviceId: number) => {
  try {
    // 先尝试刷新token
    const refreshToken = localStorage.getItem('refreshToken');
    if (refreshToken) {
      const refreshResponse = await fetch('/api/auth/refresh', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'application/json'
        },
        body: JSON.stringify({ refresh_token: refreshToken })
      });

      if (refreshResponse.ok) {
        const refreshData = await refreshResponse.json();
        if (refreshData.access_token) {
          localStorage.setItem('accessToken', refreshData.access_token);
        }
      }
    }

    // 获取最新的token
    const token = localStorage.getItem('accessToken');

    // 调用查询固件版本API
    const response = await fetch(`/api/devices/${deviceId}/version`, {
      method: 'GET',
      headers: {
        'Accept': 'application/json',
        'Authorization': token ? `Bearer ${token}` : ''
      }
    });

    if (response.ok) {
        const data = await response.json();
        console.log('查询固件版本成功响应:', data);
        deviceVersions.value[deviceId] = data.data.values;
      } else if (response.status === 403) {
        const errorData = await response.json();
        console.log('查询固件版本权限不足:', errorData);
        deviceVersions.value[deviceId] = '权限不足';
      } else if (response.status === 404) {
        const errorData = await response.json();
        console.log('查询固件版本设备不存在:', errorData);
        deviceVersions.value[deviceId] = '设备不存在';
      } else if (response.status === 422) {
        const errorData = await response.json();
        console.log('查询固件版本参数错误:', errorData);
        deviceVersions.value[deviceId] = '参数错误';
      } else {
        console.log('查询固件版本失败响应:', response);
        deviceVersions.value[deviceId] = '查询失败';
      }
  } catch (error) {
    console.error('查询固件版本出错:', error);
    deviceVersions.value[deviceId] = '查询失败';
  }
};

const selectedDeviceId = ref<number | null>(null);

// 日志筛选表单
const logFilterForm = ref({
  deviceName: '',
  area: [] as string[],
  level: [] as string[]
});

// 筛选表单
const filterForm = ref({
  name: '',
  area: [] as string[],
  isOnline: null as boolean | null
});

// 获取所有不重复的区域
const uniqueAreas = computed(() => {
  const areas = devices.value
    .map(device => device.area)
    .filter((area): area is string => area !== undefined && area !== null && area !== '');
  return Array.from(new Set(areas)).sort();
});

// 筛选后的设备列表
const filteredDevices = computed(() => {
  return devices.value.filter(device => {
    const nameMatch = !filterForm.value.name || device.name.toLowerCase().includes(filterForm.value.name.toLowerCase());
    const areaMatch = filterForm.value.area.length === 0 || (device.area && filterForm.value.area.includes(device.area));
    const onlineMatch = filterForm.value.isOnline === null || device.isOnline === filterForm.value.isOnline;
    return nameMatch && areaMatch && onlineMatch;
  });
});

// 重置筛选
const resetFilter = () => {
  filterForm.value = {
    name: '',
    area: [],
    isOnline: null
  };
};

const filteredLogs = computed<DeviceLog[]>(() => {
  const logs = getDeviceLogs(selectedDeviceId.value || undefined);
  return logs.sort((a, b) => {
    return new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime();
  });
});

// 筛选后的日志列表
const filteredLogsByFilter = computed(() => {
  return filteredLogs.value.filter(log => {
    const deviceNameMatch = !logFilterForm.value.deviceName || 
      log.deviceName.toLowerCase().includes(logFilterForm.value.deviceName.toLowerCase());
    
    // 区域筛选：通过设备名称找到对应设备的区域
    const areaMatch = logFilterForm.value.area.length === 0 || (() => {
      const device = devices.value.find(d => d.name === log.deviceName);
      return device && device.area && logFilterForm.value.area.includes(device.area);
    })();
    
    const levelMatch = logFilterForm.value.level.length === 0 || logFilterForm.value.level.includes(log.level);
    return deviceNameMatch && areaMatch && levelMatch;
  });
});

// 重置日志筛选
const resetLogFilter = () => {
  logFilterForm.value = {
    deviceName: '',
    area: [],
    level: []
  };
};

const dialogVisible = ref(false);
const isEdit = ref(false);
const deviceForm = ref({
  id: '',
  name: '',
  secret: '',
  area: '',
  number: 1,
  status: 'none',
  isOnline: false,
  device_type: ''
});

// 存储设备固件版本
const deviceVersions = ref<{ [key: number]: string }>({});

// 记录接口调用时间，防止重复调用
const lastCallTime = ref<{ [key: string]: number }>({});
const CALL_INTERVAL = 10000; // 10秒间隔

// 检查是否可以调用接口
const canCall = (key: string): boolean => {
  const now = Date.now();
  const lastTime = lastCallTime.value[key] || 0;
  if (now - lastTime > CALL_INTERVAL) {
    lastCallTime.value[key] = now;
    return true;
  }
  return false;
};

// 组件挂载时获取设备列表和固件版本
onMounted(() => {
  fetchDevices();
});

// 显示添加对话框
const showAddDialog = () => {
  isEdit.value = false;
  deviceForm.value = {
    id: '',
    name: '',
    secret: '',
    area: '',
    number: 1,
    status: 'none',
    isOnline: false,
    device_type: ''
  };
  dialogVisible.value = true;
};

// 编辑设备
const handleEdit = (device: Device) => {
  isEdit.value = true;
  deviceForm.value = {
    id: device.id.toString(),
    name: device.name,
    secret: '',
    area: device.area || '',
    number: device.number || 1,
    status: device.status,
    isOnline: device.isOnline,
    device_type: device.device_type || ''
  };
  dialogVisible.value = true;
};

// 保存设备
const handleSave = async () => {
  if (!deviceForm.value.name) {
    ElMessage.warning('请填写设备名称');
    return;
  }

  if (isEdit.value) {
    // 编辑模式 - 调用后端API
    if (!deviceForm.value.area || !deviceForm.value.number) {
      ElMessage.warning('请填写完整的设备信息');
      return;
    }

    // 检查编号是否与同区域的设备编号重复
    const deviceId = parseInt(deviceForm.value.id);
    const duplicateDevice = devices.value.find(d => 
      d.id !== deviceId && 
      d.area === deviceForm.value.area && 
      d.number === deviceForm.value.number
    );

    if (duplicateDevice) {
      ElMessage.error(`编号 ${deviceForm.value.number} 在区域 "${deviceForm.value.area}" 中已被设备 "${duplicateDevice.name}" 使用`);
      return;
    }

    try {
      const token = localStorage.getItem('accessToken');
      const deviceId = parseInt(deviceForm.value.id);
      const response = await fetch(`/api/devices/${deviceId}`, {
        method: 'PUT',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'application/json',
          'Authorization': token ? `Bearer ${token}` : ''
        },
        body: JSON.stringify({
          name: deviceForm.value.name,
          area: deviceForm.value.area,
          number: deviceForm.value.number,
          device_type: deviceForm.value.device_type,
          isOnline: deviceForm.value.isOnline,
          status: deviceForm.value.status
        })
      });

      if (response.ok) {
        const data = await response.json();
        ElMessage.success('设备更新成功');
        // 刷新设备列表
        await fetchDevices();
        dialogVisible.value = false;
      } else if (response.status === 404) {
        const errorData = await response.json();
        ElMessage.error(errorData.detail || '设备不存在');
      } else if (response.status === 422) {
        const errorData = await response.json();
        ElMessage.error('请求参数错误：' + JSON.stringify(errorData.detail));
      } else {
        ElMessage.error('设备更新失败：' + response.statusText);
      }
    } catch (error) {
      console.error('更新设备出错:', error);
      ElMessage.error('更新设备失败，请检查网络连接');
    }
  } else {
    // 添加模式 - 调用后端API
    if (!deviceForm.value.secret || !deviceForm.value.area || !deviceForm.value.number) {
      ElMessage.warning('请填写完整的设备信息（密钥、区域、编号）');
      return;
    }

    // 检查编号是否与同区域的设备编号重复
    const duplicateDevice = devices.value.find(d => 
      d.area === deviceForm.value.area && 
      d.number === deviceForm.value.number
    );

    if (duplicateDevice) {
      ElMessage.error(`编号 ${deviceForm.value.number} 在区域 "${deviceForm.value.area}" 中已被设备 "${duplicateDevice.name}" 使用`);
      return;
    }

    try {
      const token = localStorage.getItem('accessToken');
      const response = await fetch('/api/devices', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'application/json',
          'Authorization': token ? `Bearer ${token}` : ''
        },
        body: JSON.stringify({
          secret: deviceForm.value.secret,
          name: deviceForm.value.name,
          area: deviceForm.value.area,
          number: deviceForm.value.number,
          isOnline: false,
          status: 'none'
        })
      });

      if (response.ok) {
        const data = await response.json();
        ElMessage.success('设备添加成功');
        // 刷新设备列表
        await fetchDevices();
        dialogVisible.value = false;
      } else if (response.status === 400) {
        const errorData = await response.json();
        ElMessage.error(errorData.detail || '设备已存在');
      } else if (response.status === 422) {
        const errorData = await response.json();
        ElMessage.error('请求参数错误：' + JSON.stringify(errorData.detail));
      } else {
        ElMessage.error('设备添加失败：' + response.statusText);
      }
    } catch (error) {
      console.error('添加设备出错:', error);
      ElMessage.error('添加设备失败，请检查网络连接');
    }
  }
};

// 删除设备
const handleDelete = (device: Device) => {
  ElMessageBox.confirm(
    '确定要删除该设备吗？删除后将无法恢复！',
    '确认删除',
    {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    }
  ).then(async () => {
    const key = `device_${device.id}`;
    if (!canCall(key)) {
      ElMessage.warning('操作过于频繁，请10秒后再试');
      return;
    }
    
    try {
      // 先尝试刷新token
      const refreshToken = localStorage.getItem('refreshToken');
      if (refreshToken) {
        const refreshResponse = await fetch('/api/auth/refresh', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Accept': 'application/json'
          },
          body: JSON.stringify({ refresh_token: refreshToken })
        });

        if (refreshResponse.ok) {
          const refreshData = await refreshResponse.json();
          if (refreshData.access_token) {
            localStorage.setItem('accessToken', refreshData.access_token);
          }
        }
      }

      // 获取最新的token
      const token = localStorage.getItem('accessToken');

      // 调用删除设备API
      const response = await fetch(`/api/devices/${device.id}`, {
        method: 'DELETE',
        headers: {
          'Accept': 'application/json',
          'Authorization': token ? `Bearer ${token}` : ''
        }
      });

      if (response.ok) {
        const data = await response.json();
        ElMessage.success('设备删除成功');
        // 刷新设备列表
        await fetchDevices();
      } else if (response.status === 403) {
        const errorData = await response.json();
        ElMessage.error(errorData.msg || '权限不足，仅root用户可删除设备');
      } else if (response.status === 404) {
        const errorData = await response.json();
        ElMessage.error(errorData.msg || '设备不存在');
      } else if (response.status === 422) {
        const errorData = await response.json();
        ElMessage.error('请求参数错误：' + JSON.stringify(errorData.detail));
      } else {
        ElMessage.error('设备删除失败：' + response.statusText);
      }
    } catch (error) {
      console.error('删除设备出错:', error);
      ElMessage.error('删除设备失败，请检查网络连接');
    }
  }).catch(() => {
    // 取消确认
  });
};

// 重启设备
const handleRestart = (device: Device) => {
  ElMessageBox.confirm(
    '确定要重启该设备吗？重启后设备将离线约500ms并重新连接。',
    '确认重启',
    {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning'
    }
  ).then(async () => {
    const key = `device_${device.id}`;
    if (!canCall(key)) {
      ElMessage.warning('操作过于频繁，请10秒后再试');
      return;
    }
    
    try {
      // 先尝试刷新token
      const refreshToken = localStorage.getItem('refreshToken');
      if (refreshToken) {
        const refreshResponse = await fetch('/api/auth/refresh', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Accept': 'application/json'
          },
          body: JSON.stringify({ refresh_token: refreshToken })
        });

        if (refreshResponse.ok) {
          const refreshData = await refreshResponse.json();
          if (refreshData.access_token) {
            localStorage.setItem('accessToken', refreshData.access_token);
          }
        }
      }

      // 获取最新的token
      const token = localStorage.getItem('accessToken');

      // 调用重启设备API
      const response = await fetch(`/api/devices/${device.id}/restart`, {
        method: 'POST',
        headers: {
          'Accept': 'application/json',
          'Authorization': token ? `Bearer ${token}` : ''
        }
      });

      if (response.ok) {
        const data = await response.json();
        ElMessage.success('重启命令已发送，设备将在约500ms后重启');
        // 刷新设备列表
        await fetchDevices();
      } else if (response.status === 403) {
        const errorData = await response.json();
        ElMessage.error(errorData.msg || '权限不足，仅root用户可重启设备');
      } else if (response.status === 404) {
        const errorData = await response.json();
        ElMessage.error(errorData.msg || '设备不存在');
      } else if (response.status === 422) {
        const errorData = await response.json();
        ElMessage.error('请求参数错误：' + JSON.stringify(errorData.detail));
      } else {
        ElMessage.error('重启设备失败：' + response.statusText);
      }
    } catch (error) {
      console.error('重启设备出错:', error);
      ElMessage.error('重启设备失败，请检查网络连接');
    }
  }).catch(() => {
    // 取消确认
  });
};

// 更新固件
const handleUpdateFirmware = (device: Device) => {
  ElMessageBox.confirm(
    '确定要为设备更新固件吗？更新过程中摄像头会被临时关闭，设备可能会自动重启。',
    '更新固件',
    {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'info',
    }
  ).then(async () => {
    try {
      // 先尝试刷新token
      const refreshToken = localStorage.getItem('refreshToken');
      if (refreshToken) {
        const refreshResponse = await fetch('/api/auth/refresh', {
          method: 'POST',
          headers: {
            'Content-Type': 'application/json',
            'Accept': 'application/json'
          },
          body: JSON.stringify({ refresh_token: refreshToken })
        });

        if (refreshResponse.ok) {
          const refreshData = await refreshResponse.json();
          if (refreshData.access_token) {
            localStorage.setItem('accessToken', refreshData.access_token);
          }
        }
      }

      // 获取最新的token
      const token = localStorage.getItem('accessToken');

      // 调用OTA更新API
      const response = await fetch(`/api/devices/${device.id}/ota`, {
        method: 'POST',
        headers: {
          'Accept': 'application/json',
          'Authorization': token ? `Bearer ${token}` : ''
        }
      });

      if (response.ok) {
        const data = await response.json();
        console.log('OTA更新成功响应:', data);
        ElMessage.success('OTA更新命令已发送，设备将自动检查并下载新版本');
        // 刷新设备列表
        await fetchDevices();
      } else if (response.status === 403) {
        const errorData = await response.json();
        console.log('OTA更新权限不足:', errorData);
        ElMessage.error(errorData.msg || '权限不足，仅root用户可更新固件');
      } else if (response.status === 404) {
        const errorData = await response.json();
        console.log('OTA更新设备不存在:', errorData);
        ElMessage.error(errorData.msg || '设备不存在');
      } else if (response.status === 422) {
        const errorData = await response.json();
        console.log('OTA更新参数错误:', errorData);
        ElMessage.error('请求参数错误：' + JSON.stringify(errorData.detail));
      } else {
        console.log('OTA更新失败响应:', response);
        ElMessage.error('固件更新失败：' + response.statusText);
      }
    } catch (error) {
      console.error('更新固件出错:', error);
      ElMessage.error('更新固件失败，请检查网络连接');
    }
  }).catch(() => {
    // 取消更新
  });
};
</script>

<style lang="scss" scoped>
.device-card {
  z-index: 1;
  width: 75%;
  margin-top: 1%;
  border-radius: 15px;
  padding: 20px;
}

.device-online {
  border-left: 4px solid #67c23a;
}

.device-offline {
  border-left: 4px solid #f56c6c;
  opacity: 0.8;
}

.title {
  text-align: center;
  margin-bottom: 20px;
}

.no-devices {
  text-align: center;
  padding: 40px 0;
  color: #999;
}

.management-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.management-header h4 {
  margin: 0;
}

.filter-section {
  display: flex;
  align-items: center;
  margin-bottom: 20px;
  padding: 15px;
  background: #f5f7fa;
  border-radius: 4px;
}

.logs-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 20px;
}

.logs-header h4 {
  margin: 0;
}

.no-logs {
  text-align: center;
  padding: 40px 0;
  color: #999;
}

.loading {
  color: #909399;
  font-style: italic;
}
</style>