<template>
  <div class="device-container" :class="{ 'mobile': isMobile }">
    <!-- 桌面端：使用 el-card 包装 -->
    <el-card v-if="!isMobile" class="device-card">
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

    <!-- 移动端：不使用 el-card 包装 -->
    <div v-else class="mobile-device">
      <!-- 总览视图 -->
      <div v-if="activeTab === 'overview'" class="overview-container">
        <div class="overview-header">
          <h3>设备总览</h3>
        </div>
        <div class="device-list">
          <div 
            v-for="device in devices" 
            :key="device.id" 
            class="device-card"
            :class="device.isOnline ? 'online' : 'offline'"
          >
            <div class="device-header">
              <h4>{{ device.name }}</h4>
              <el-tag :type="device.isOnline ? 'success' : 'danger'" size="small">
                {{ device.isOnline ? '在线' : '离线' }}
              </el-tag>
            </div>
            <div class="device-info">
              <div class="info-row">
                <span class="label">区域</span>
                <span class="value">{{ device.area || '未设置' }}</span>
              </div>
              <div class="info-row">
                <span class="label">编号</span>
                <span class="value">{{ device.number || '-' }}</span>
              </div>
              <div class="info-row">
                <span class="label">类型</span>
                <span class="value">{{ device.device_type || '未设置' }}</span>
              </div>
              <div class="info-row">
                <span class="label">固件版本</span>
                <span class="value">{{ deviceVersions[device.id] || '加载中...' }}</span>
              </div>
            </div>
          </div>
        </div>
        <div v-if="devices.length === 0" class="empty-state">
          <el-empty description="暂无设备数据" />
        </div>
      </div>
      
      <!-- 管理视图 -->
      <div v-else-if="activeTab === 'management'" class="management-container">
        <div class="management-header">
          <h3>设备管理</h3>
          <el-button type="primary" size="small" @click="showAddDialog">
            <el-icon><Plus /></el-icon>
            添加
          </el-button>
        </div>
        
        <!-- 筛选 -->
        <div class="filter-section">
          <el-input 
            v-model="filterForm.name" 
            placeholder="设备名称" 
            clearable
            class="filter-input"
          />
          <el-select 
            v-model="filterForm.area" 
            placeholder="所属区域" 
            clearable
            multiple
            collapse-tags
            collapse-tags-tooltip
            class="filter-select"
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
            clearable
            class="filter-select"
          >
            <el-option label="在线" :value="true" />
            <el-option label="离线" :value="false" />
          </el-select>
          <el-button size="small" @click="resetFilter">重置</el-button>
        </div>
        
        <!-- 设备列表 -->
        <div class="device-list">
          <div 
            v-for="device in filteredDevices" 
            :key="device.id" 
            class="device-card"
            :class="device.isOnline ? 'online' : 'offline'"
          >
            <div class="device-header">
              <h4>{{ device.name }}</h4>
              <div class="device-status">
                <el-tag :type="device.isOnline ? 'success' : 'danger'" size="small">
                  {{ device.isOnline ? '在线' : '离线' }}
                </el-tag>
                <el-tag :type="device.status === 'stream' ? 'success' : 'info'" size="small">
                  {{ device.status === 'stream' ? '推流中' : '待机' }}
                </el-tag>
              </div>
            </div>
            <div class="device-info">
              <div class="info-row">
                <span class="label">设备ID</span>
                <span class="value">{{ device.id }}</span>
              </div>
              <div class="info-row">
                <span class="label">区域</span>
                <span class="value">{{ device.area || '未设置' }}</span>
              </div>
              <div class="info-row">
                <span class="label">编号</span>
                <span class="value">{{ device.number || '-' }}</span>
              </div>
              <div class="info-row">
                <span class="label">类型</span>
                <span class="value">{{ device.device_type || '未设置' }}</span>
              </div>
              <div class="info-row">
                <span class="label">固件版本</span>
                <span class="value">{{ deviceVersions[device.id] || '加载中...' }}</span>
              </div>
            </div>
            <div class="device-actions">
              <el-button size="small" @click="handleEdit(device)">编辑</el-button>
              <el-button size="small" type="success" @click="handleUpdateFirmware(device)">更新固件</el-button>
              <el-button size="small" type="warning" @click="handleRestart(device)">重启</el-button>
              <el-button size="small" type="danger" @click="handleDelete(device)">删除</el-button>
            </div>
          </div>
        </div>
        <div v-if="filteredDevices.length === 0" class="empty-state">
          <el-empty description="暂无设备数据" />
        </div>
      </div>
      
      <!-- 日志视图 -->
      <div v-else-if="activeTab === 'logs'" class="logs-container">
        <div class="logs-header">
          <h3>设备日志</h3>
        </div>
        
        <!-- 筛选 -->
        <div class="filter-section">
          <el-input 
            v-model="logFilterForm.deviceName" 
            placeholder="设备名称" 
            clearable
            class="filter-input"
          />
          <el-select 
            v-model="logFilterForm.area" 
            placeholder="所属区域" 
            clearable
            multiple
            collapse-tags
            collapse-tags-tooltip
            class="filter-select"
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
            clearable
            multiple
            collapse-tags
            collapse-tags-tooltip
            class="filter-select"
          >
            <el-option label="信息" value="info" />
            <el-option label="警告" value="warning" />
            <el-option label="错误" value="error" />
          </el-select>
          <el-button size="small" @click="resetLogFilter">重置</el-button>
        </div>
        
        <!-- 日志列表 -->
        <div class="log-list">
          <div 
            v-for="log in filteredLogsByFilter" 
            :key="log.id" 
            class="log-card"
            :class="log.level"
          >
            <div class="log-header">
              <span class="log-device">{{ log.deviceName }}</span>
              <el-tag 
                :type="{
                  'info': 'info',
                  'warning': 'warning',
                  'error': 'danger'
                }[log.level]"
                size="small"
              >
                {{ {
                  'info': '信息',
                  'warning': '警告',
                  'error': '错误'
                }[log.level] }}
              </el-tag>
            </div>
            <div class="log-content">{{ log.message }}</div>
            <div class="log-time">{{ log.timestamp }}</div>
          </div>
        </div>
        <div v-if="filteredLogsByFilter.length === 0" class="empty-state">
          <el-empty description="暂无日志数据" />
        </div>
      </div>

      <!-- 备案信息 -->
      <MobileBeian />
    </div>
  </div>

  <!-- 添加/编辑设备对话框 -->
  <el-dialog 
    v-model="dialogVisible" 
    :title="isEdit ? '编辑设备' : '添加设备'" 
    :width="isMobile ? '90%' : '500px'"
  >
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
        <el-input-number v-model="deviceForm.number" :min="1" placeholder="请输入设备编号" style="width: 100%;" />
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

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue';
import { ElMessage, ElMessageBox } from 'element-plus';
import { Plus } from '@element-plus/icons-vue';
import { useDeviceStore, type Device, type DeviceLog } from '../stores/deviceStore';
import MobileBeian from './mobile/MobileBeian.vue';
import { api } from '../utils/api';

// 检测是否为移动设备
const isMobile = ref(window.innerWidth < 768);

// 监听窗口大小变化
const handleWindowResize = () => {
  isMobile.value = window.innerWidth < 768;
};

onMounted(() => {
  window.addEventListener('resize', handleWindowResize);
  fetchDevices();
});

const props = defineProps<{
  activeTab?: string;
}>();

const activeTab = computed(() => props.activeTab || 'overview');

const { devices, getDeviceLogs, fetchDevices: storeFetchDevices } = useDeviceStore();

// 获取设备列表
const fetchDevices = async () => {
  try {
    await storeFetchDevices();
    // 获取每个设备的固件版本（顺序拉取，避免并发刷新 token / 请求风暴）
      for (const device of devices.value as Device[]) {
        await fetchDeviceVersion(device.id);
      }
  } catch (error) {
    console.error('获取设备列表出错:', error);
    ElMessage.error('获取设备列表失败，请检查网络连接');
  }
};

// 获取设备固件版本
const fetchDeviceVersion = async (deviceId: number) => {
  try {
    // 调用查询固件版本API
    const data = await api.get(`/api/devices/${deviceId}/version`);
    console.log('查询固件版本成功:', data);
    deviceVersions.value[deviceId] = data.data.values;
  } catch (error: any) {
    console.error('查询固件版本出错:', error);
    if (error.response) {
      const status = error.response.status;
      if (status === 403) {
        deviceVersions.value[deviceId] = '权限不足';
      } else if (status === 404) {
        deviceVersions.value[deviceId] = '设备不存在';
      } else if (status === 422) {
        deviceVersions.value[deviceId] = '参数错误';
      } else {
        deviceVersions.value[deviceId] = '查询失败';
      }
    } else {
      deviceVersions.value[deviceId] = '查询失败';
    }
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
      const deviceId = parseInt(deviceForm.value.id);
      const data = await api.put(`/api/devices/${deviceId}`, {
        name: deviceForm.value.name,
        area: deviceForm.value.area,
        number: deviceForm.value.number,
        device_type: deviceForm.value.device_type,
        isOnline: deviceForm.value.isOnline,
        status: deviceForm.value.status
      });

      ElMessage.success('设备更新成功');
      await fetchDevices();
      dialogVisible.value = false;
    } catch (error: any) {
      console.error('更新设备出错:', error);
      if (error.response) {
        const status = error.response.status;
        if (status === 404) {
          ElMessage.error(error.response.data?.detail || '设备不存在');
        } else if (status === 422) {
          ElMessage.error('请求参数错误：' + JSON.stringify(error.response.data?.detail));
        } else {
          ElMessage.error('设备更新失败：' + error.response.statusText);
        }
      } else {
        ElMessage.error('更新设备失败，请检查网络连接');
      }
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
      const data = await api.post('/api/devices', {
        secret: deviceForm.value.secret,
        name: deviceForm.value.name,
        area: deviceForm.value.area,
        number: deviceForm.value.number,
        device_type: deviceForm.value.device_type,
        isOnline: false,
        status: 'none'
      });

      ElMessage.success('设备添加成功');
      await fetchDevices();
      dialogVisible.value = false;
    } catch (error: any) {
      console.error('添加设备出错:', error);
      if (error.response) {
        const status = error.response.status;
        if (status === 400) {
          ElMessage.error(error.response.data?.detail || '设备已存在');
        } else if (status === 422) {
          ElMessage.error('请求参数错误：' + JSON.stringify(error.response.data?.detail));
        } else {
          ElMessage.error('设备添加失败：' + error.response.statusText);
        }
      } else {
        ElMessage.error('添加设备失败，请检查网络连接');
      }
    }
  }
};

// 删除设备
const handleDelete = async (device: Device) => {
  if (isMobile.value) {
    // 移动端：二次确认
    ElMessageBox.confirm(
      `确定要删除设备 "${device.name}" 吗？`,
      '警告',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    ).then(() => {
      // 第二次确认 - 二次提醒防止误删除
      ElMessageBox.confirm(
        `此操作不可恢复，请再次确认是否删除设备 "${device.name}"（ID: ${device.id}）？`,
        '二次确认',
        {
          confirmButtonText: '确认删除',
          cancelButtonText: '取消',
          type: 'error',
          confirmButtonClass: 'el-button--danger',
        }
      ).then(async () => {
        try {
          // 调用删除设备API
          const data = await api.delete(`/api/devices/${device.id}`);
          ElMessage.success('设备删除成功');
          await fetchDevices();
        } catch (error: any) {
          console.error('删除设备出错:', error);
          if (error.response) {
            const status = error.response.status;
            if (status === 403) {
              ElMessage.error(error.response.data?.msg || '权限不足，仅root用户可删除设备');
            } else if (status === 404) {
              ElMessage.error(error.response.data?.msg || '设备不存在');
            } else if (status === 422) {
              ElMessage.error('请求参数错误：' + JSON.stringify(error.response.data?.detail));
            } else {
              ElMessage.error('设备删除失败：' + error.response.statusText);
            }
          } else {
            ElMessage.error('删除设备失败，请检查网络连接');
          }
        }
      }).catch(() => {
        // 取消第二次确认
      });
    }).catch(() => {
      // 取消第一次确认
    });
  } else {
    // 桌面端：单次确认
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
        // 调用删除设备API
        const data = await api.delete(`/api/devices/${device.id}`);
        ElMessage.success('设备删除成功');
        // 刷新设备列表
        await fetchDevices();
      } catch (error: any) {
        console.error('删除设备出错:', error);
        if (error.response) {
          const status = error.response.status;
          if (status === 403) {
            ElMessage.error(error.response.data?.msg || '权限不足，仅root用户可删除设备');
          } else if (status === 404) {
            ElMessage.error(error.response.data?.msg || '设备不存在');
          } else if (status === 422) {
            ElMessage.error('请求参数错误：' + JSON.stringify(error.response.data?.detail));
          } else {
            ElMessage.error('设备删除失败：' + error.response.statusText);
          }
        } else {
          ElMessage.error('删除设备失败，请检查网络连接');
        }
      }
    }).catch(() => {
      // 取消确认
    });
  }
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
    const key = `device_${device.id}`;
    if (!canCall(key)) {
      ElMessage.warning('操作过于频繁，请10秒后再试');
      return;
    }
    
    try {
      // 调用OTA更新API
      const data = await api.post(`/api/devices/${device.id}/ota`);
      console.log('OTA更新成功响应:', data);
      ElMessage.success('OTA更新命令已发送，设备将自动检查并下载新版本');
      // 刷新设备列表
      await fetchDevices();
    } catch (error: any) {
      console.error('更新固件出错:', error);
      if (error.response) {
        const status = error.response.status;
        if (status === 403) {
          ElMessage.error(error.response.data?.msg || '权限不足，仅root用户可更新固件');
        } else if (status === 404) {
          ElMessage.error(error.response.data?.msg || '设备不存在');
        } else if (status === 422) {
          ElMessage.error('请求参数错误：' + JSON.stringify(error.response.data?.detail));
        } else {
          ElMessage.error('固件更新失败：' + error.response.statusText);
        }
      } else {
        ElMessage.error('更新固件失败，请检查网络连接');
      }
    }
  }).catch(() => {
    // 取消更新
  });
};

// 重启设备
const handleRestart = (device: Device) => {
  ElMessageBox.confirm(
    '确定要重启该设备吗？设备收到指令后约500ms执行重启。',
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
      // 调用重启设备API
      const data = await api.post(`/api/devices/${device.id}/restart`);
      ElMessage.success('重启命令已发送，设备将在约500ms后重启');
      // 刷新设备列表
      await fetchDevices();
    } catch (error: any) {
      console.error('重启设备出错:', error);
      if (error.response) {
        const status = error.response.status;
        if (status === 403) {
          ElMessage.error(error.response.data?.msg || '权限不足，仅root用户可重启设备');
        } else if (status === 404) {
          ElMessage.error(error.response.data?.msg || '设备不存在');
        } else if (status === 422) {
          ElMessage.error('请求参数错误：' + JSON.stringify(error.response.data?.detail));
        } else {
          ElMessage.error('重启设备失败：' + error.response.statusText);
        }
      } else {
        ElMessage.error('重启设备失败，请检查网络连接');
      }
    }
  }).catch(() => {
    // 取消确认
  });
};
</script>

<style lang="scss" scoped>
/* 桌面端样式 */
.device-container {
  width: 75%;
  margin-top: 1%;
  position: relative;
  z-index: 1;
  
  &.mobile {
    width: 100%;
    margin: 0;
  }
}

.device-card {
  z-index: 1;
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

/* 移动端样式 */
.mobile-device {
  min-height: calc(100vh - 120px);
  background: #f5f7fa;
  display: flex;
  flex-direction: column;
  overflow-x: hidden;
  box-sizing: border-box;
}

/* 总览视图 */
.overview-container,
.management-container,
.logs-container {
  padding: 0;
}

.overview-header,
.management-header,
.logs-header {
  margin-bottom: 16px;
}

.overview-header h3,
.management-header h3,
.logs-header h3 {
  margin: 0;
  font-size: 18px;
  font-weight: 600;
  color: #303133;
}

.mobile-device .management-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

/* 设备列表 */
.mobile-device .device-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.mobile-device .device-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
  border-left: 4px solid #67c23a;
}

.mobile-device .device-card.offline {
  border-left-color: #f56c6c;
  opacity: 0.8;
}

.mobile-device .device-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.mobile-device .device-header h4 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.device-status {
  display: flex;
  gap: 8px;
}

.mobile-device .device-info {
  margin-bottom: 12px;
}

.mobile-device .info-row {
  display: flex;
  justify-content: space-between;
  padding: 8px 0;
  border-bottom: 1px solid #f0f0f0;
}

.mobile-device .info-row:last-child {
  border-bottom: none;
}

.mobile-device .info-row .label {
  font-size: 14px;
  color: #909399;
}

.mobile-device .info-row .value {
  font-size: 14px;
  font-weight: 500;
  color: #303133;
}

.device-actions {
  display: flex;
  gap: 8px;
  padding-top: 12px;
  border-top: 1px solid #f0f0f0;
}

/* 移动端筛选 */
.mobile-device .filter-section {
  display: flex;
  flex-direction: column;
  gap: 12px;
  margin-bottom: 16px;
  padding: 16px;
  background: white;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.filter-input,
.filter-select {
  width: 100% !important;
}

/* 日志列表 */
.mobile-device .log-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.mobile-device .log-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
  border-left: 4px solid #909399;
}

.mobile-device .log-card.warning {
  border-left-color: #e6a23c;
}

.mobile-device .log-card.error {
  border-left-color: #f56c6c;
}

.mobile-device .log-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 8px;
}

.mobile-device .log-device {
  font-size: 14px;
  font-weight: 600;
  color: #303133;
}

.mobile-device .log-content {
  font-size: 14px;
  color: #606266;
  margin-bottom: 8px;
  line-height: 1.5;
}

.mobile-device .log-time {
  font-size: 12px;
  color: #909399;
}

/* 空状态 */
.empty-state {
  padding: 40px 0;
}
</style>