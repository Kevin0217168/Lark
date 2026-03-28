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
        
        <!-- 设备选择和分页控制 -->
        <div class="filter-section">
          <el-select 
            v-model="selectedDeviceIds" 
            placeholder="选择设备" 
            style="width: 250px; margin-right: 10px;"
            clearable
            multiple
            collapse-tags
            collapse-tags-tooltip
            @change="handleDeviceChange"
          >
            <el-option 
              v-for="device in devices" 
              :key="device.id" 
              :label="device.name" 
              :value="device.id" 
            />
          </el-select>
          <el-select 
            v-model="selectedLogLevels" 
            placeholder="日志等级" 
            style="width: 200px; margin-right: 10px;"
            clearable
            multiple
            collapse-tags
            collapse-tags-tooltip
            @change="handleLevelChange"
          >
            <el-option label="INFO" value="INFO" />
            <el-option label="WARNING" value="WARNING" />
            <el-option label="ERROR" value="ERROR" />
            <el-option label="DEBUG" value="DEBUG" />
          </el-select>
          <el-date-picker
            v-model="selectedTimeRange"
            type="datetimerange"
            range-separator="至"
            start-placeholder="开始时间"
            end-placeholder="结束时间"
            style="width: 360px; margin-right: 10px;"
            value-format="YYYY-MM-DDTHH:mm:ss"
            :disabled-date="disabledDate"
            @change="handleTimeChange"
          />
          <el-button type="primary" @click="fetchLogs" :loading="logsLoading" :disabled="selectedDeviceIds.length === 0">
            查询
          </el-button>
          <el-button @click="resetLogFilter">重置</el-button>
        </div>
        
        <!-- 加载状态 -->
        <div v-if="logsLoading" class="loading-container">
          <el-icon class="is-loading" :size="32"><loading /></el-icon>
          <span>加载中...</span>
        </div>
        
        <!-- 错误提示 -->
        <el-alert 
          v-else-if="logsError" 
          :title="logsError" 
          type="error" 
          show-icon 
          closable
          @close="logsError = ''"
          style="margin-bottom: 20px;"
        />
        
        <!-- 未选择设备提示 -->
        <el-empty 
          v-else-if="selectedDeviceIds.length === 0" 
          description="请选择设备查看日志" 
        />
        
        <!-- 日志表格 -->
        <el-table v-else :data="currentLogs" style="width: 100%" border>
          <el-table-column prop="id" label="日志ID" width="100" />
          <el-table-column prop="device_id" label="设备ID" width="100" />
          <el-table-column prop="level" label="级别" width="100">
            <template #default="scope">
              <el-tag 
                :type="{
                  'INFO': 'info',
                  'WARNING': 'warning',
                  'ERROR': 'danger',
                  'DEBUG': ''
                }[scope.row.level as 'INFO' | 'WARNING' | 'ERROR' | 'DEBUG']"
              >
                {{ scope.row.level }}
              </el-tag>
            </template>
          </el-table-column>
          <el-table-column prop="tag" label="标签" width="120" />
          <el-table-column prop="content" label="日志内容" min-width="300" show-overflow-tooltip />
          <el-table-column prop="tick" label="Tick" width="100" />
          <el-table-column prop="timestamp" label="时间" width="180">
            <template #default="scope">
              {{ formatTimestamp(scope.row.timestamp) }}
            </template>
          </el-table-column>
        </el-table>
        
        <!-- 分页控制 -->
        <div v-if="selectedDeviceIds.length > 0 && !logsLoading && !logsError && currentLogs.length > 0" class="pagination-container">
          <el-pagination
            v-model:current-page="logCurrentPage"
            :page-size="logPagination.limit"
            :total="logTotal"
            layout="total, prev, pager, next, jumper"
            @current-change="handleCurrentChange"
          />
        </div>
        
        <!-- 空数据提示 -->
        <el-empty 
          v-if="selectedDeviceIds.length > 0 && !logsLoading && !logsError && currentLogs.length === 0" 
          description="暂无日志数据" 
        />
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
        
        <!-- 设备选择和分页控制 -->
        <div class="filter-section">
          <el-select 
            v-model="selectedDeviceIds" 
            placeholder="选择设备" 
            clearable
            multiple
            collapse-tags
            collapse-tags-tooltip
            class="filter-select"
            @change="handleDeviceChange"
          >
            <el-option 
              v-for="device in devices" 
              :key="device.id" 
              :label="device.name" 
              :value="device.id" 
            />
          </el-select>
          <el-select 
            v-model="selectedLogLevels" 
            placeholder="日志等级" 
            clearable
            multiple
            collapse-tags
            collapse-tags-tooltip
            class="filter-select"
            @change="handleLevelChange"
          >
            <el-option label="INFO" value="INFO" />
            <el-option label="WARNING" value="WARNING" />
            <el-option label="ERROR" value="ERROR" />
            <el-option label="DEBUG" value="DEBUG" />
          </el-select>
          <div class="mobile-time-range">
            <el-date-picker
              v-model="startTime"
              type="datetime"
              placeholder="开始时间"
              class="filter-datepicker-mobile"
              value-format="YYYY-MM-DDTHH:mm:ss"
              :disabled-date="disabledDate"
              @change="handleMobileTimeChange"
            />
            <span class="time-separator">至</span>
            <el-date-picker
              v-model="endTime"
              type="datetime"
              placeholder="结束时间"
              class="filter-datepicker-mobile"
              value-format="YYYY-MM-DDTHH:mm:ss"
              :disabled-date="disabledDate"
              @change="handleMobileTimeChange"
            />
          </div>
          <div class="mobile-button-group">
            <el-button type="primary" size="small" @click="fetchLogs" :loading="logsLoading" :disabled="selectedDeviceIds.length === 0" class="mobile-action-button">
              查询
            </el-button>
            <el-button size="small" @click="resetLogFilter" class="mobile-action-button">
              重置
            </el-button>
          </div>
        </div>
        
        <!-- 加载状态 -->
        <div v-if="logsLoading" class="loading-container">
          <el-icon class="is-loading" :size="32"><loading /></el-icon>
          <span>加载中...</span>
        </div>
        
        <!-- 错误提示 -->
        <el-alert 
          v-else-if="logsError" 
          :title="logsError" 
          type="error" 
          show-icon 
          closable
          @close="logsError = ''"
          style="margin-bottom: 20px;"
        />
        
        <!-- 未选择设备提示 -->
        <el-empty 
          v-else-if="selectedDeviceIds.length === 0" 
          description="请选择设备查看日志" 
        />
        
        <!-- 日志列表 -->
        <div v-else class="log-list">
          <div 
            v-for="log in currentLogs" 
            :key="log.id" 
            class="log-card"
            :class="log.level.toLowerCase()"
          >
            <div class="log-header">
              <span class="log-device">设备ID: {{ log.device_id }}</span>
              <el-tag 
                :type="{
                  'INFO': 'info',
                  'WARNING': 'warning',
                  'ERROR': 'danger',
                  'DEBUG': ''
                }[log.level]"
                size="small"
              >
                {{ log.level }}
              </el-tag>
            </div>
            <div class="log-tag">标签: {{ log.tag }}</div>
            <div class="log-content">{{ log.content }}</div>
            <div class="log-footer">
              <span class="log-tick">Tick: {{ log.tick }}</span>
              <span class="log-time">{{ formatTimestamp(log.timestamp) }}</span>
            </div>
          </div>
        </div>
        
        <!-- 分页控制 -->
        <div v-if="selectedDeviceIds.length > 0 && !logsLoading && !logsError && currentLogs.length > 0" class="pagination-container">
          <el-pagination
            v-model:current-page="logCurrentPage"
            :page-size="logPagination.limit"
            :total="logTotal"
            layout="total, prev, pager, next"
            small
            @current-change="handleCurrentChange"
          />
        </div>
        
        <!-- 空数据提示 -->
        <el-empty 
          v-if="selectedDeviceIds.length > 0 && !logsLoading && !logsError && currentLogs.length === 0" 
          description="暂无日志数据" 
        />
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
import { Plus, Loading } from '@element-plus/icons-vue';
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
  setDefaultTimeRange();
});

const props = defineProps<{
  activeTab?: string;
}>();

const activeTab = computed(() => props.activeTab || 'overview');

const { devices, fetchDevices: storeFetchDevices } = useDeviceStore();

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

const selectedDeviceIds = ref<number[]>([]);
const selectedLogLevels = ref<('INFO' | 'WARNING' | 'ERROR' | 'DEBUG')[]>([]);
const selectedTimeRange = ref<[string, string] | null>(null);

// 移动端时间选择（分开的开始时间和结束时间）
const startTime = ref<string>('');
const endTime = ref<string>('');

// 日志相关状态
const logsLoading = ref(false);
const logsError = ref('');
const currentLogs = ref<DeviceLog[]>([]);
const logTotal = ref(0);
const logCurrentPage = ref(1);
const logPagination = ref({
  skip: 0,
  limit: 50
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

// 获取设备日志
const fetchLogs = async () => {
  if (selectedDeviceIds.value.length === 0) {
    ElMessage.warning('请选择至少一个设备');
    return;
  }

  logsLoading.value = true;
  logsError.value = '';

  // 打印查询参数
  console.log('========== 查询日志参数 ==========');
  console.log('设备ID:', selectedDeviceIds.value);
  console.log('日志等级:', selectedLogLevels.value.length > 0 ? selectedLogLevels.value : '全部');
  console.log('时间范围:', selectedTimeRange.value ? `${selectedTimeRange.value[0]} 至 ${selectedTimeRange.value[1]}` : '不限');
  console.log('分页参数:', { skip: logPagination.value.skip, limit: logPagination.value.limit });
  console.log('==================================');

  try {
    const { fetchDeviceLogs } = useDeviceStore();
    const levels = selectedLogLevels.value.length > 0 ? selectedLogLevels.value : undefined;
    const startTime = selectedTimeRange.value ? selectedTimeRange.value[0] : undefined;
    const endTime = selectedTimeRange.value ? selectedTimeRange.value[1] : undefined;

    const result = await fetchDeviceLogs(
      selectedDeviceIds.value,
      logPagination.value.skip,
      logPagination.value.limit,
      levels,
      startTime,
      endTime
    );

    // 打印查询结果
    console.log('========== 查询日志结果 ==========');
    console.log('查询成功:', result.success);
    console.log('返回日志数量:', result.data.length);
    console.log('总日志数:', result.total);
    if (result.data.length > 0) {
      console.log('日志样本:', result.data.slice(0, 3));
    }
    console.log('==================================');

    if (result.success) {
      currentLogs.value = result.data.sort((a: DeviceLog, b: DeviceLog) => {
        return new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime();
      });
      logTotal.value = result.total;
    } else {
      logsError.value = '获取日志失败，请稍后重试';
    }
  } catch (error: any) {
    console.error('获取日志出错:', error);
    logsError.value = error.response?.data?.detail || '获取日志失败，请检查网络连接';
  } finally {
    logsLoading.value = false;
  }
};

// 设备选择变化
const handleDeviceChange = () => {
  logCurrentPage.value = 1;
  logPagination.value.skip = 0;
  currentLogs.value = [];
  logTotal.value = 0;
  logsError.value = '';
};

// 日志等级变化
const handleLevelChange = () => {
  logCurrentPage.value = 1;
  logPagination.value.skip = 0;
  if (selectedDeviceIds.value.length > 0) {
    fetchLogs();
  }
};

// 时间范围变化
const handleTimeChange = () => {
  logCurrentPage.value = 1;
  logPagination.value.skip = 0;
  if (selectedDeviceIds.value.length > 0) {
    fetchLogs();
  }
};

// 移动端时间变化
const handleMobileTimeChange = () => {
  // 更新selectedTimeRange以兼容现有的fetchLogs逻辑
  if (startTime.value && endTime.value) {
    selectedTimeRange.value = [startTime.value, endTime.value];
  } else {
    selectedTimeRange.value = null;
  }
  logCurrentPage.value = 1;
  logPagination.value.skip = 0;
  if (selectedDeviceIds.value.length > 0) {
    fetchLogs();
  }
};



// 当前页变化
const handleCurrentChange = (page: number) => {
  logCurrentPage.value = page;
  logPagination.value.skip = (page - 1) * logPagination.value.limit;
  fetchLogs();
};

// 格式化时间戳
const formatTimestamp = (timestamp: string) => {
  try {
    const date = new Date(timestamp);
    return date.toLocaleString('zh-CN', {
      year: 'numeric',
      month: '2-digit',
      day: '2-digit',
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit'
    });
  } catch {
    return timestamp;
  }
};

// 格式化日期为 YYYY-MM-DDTHH:mm:ss 格式
const formatDateForPicker = (date: Date) => {
  const year = date.getFullYear();
  const month = String(date.getMonth() + 1).padStart(2, '0');
  const day = String(date.getDate()).padStart(2, '0');
  const hours = String(date.getHours()).padStart(2, '0');
  const minutes = String(date.getMinutes()).padStart(2, '0');
  const seconds = String(date.getSeconds()).padStart(2, '0');
  return `${year}-${month}-${day}T${hours}:${minutes}:${seconds}`;
};

// 设置默认时间范围为最近三天
const setDefaultTimeRange = () => {
  const now = new Date();
  const threeDaysAgo = new Date();
  threeDaysAgo.setTime(threeDaysAgo.getTime() - 3 * 24 * 60 * 60 * 1000);
  
  const startTimeStr = formatDateForPicker(threeDaysAgo);
  const endTimeStr = formatDateForPicker(now);
  
  selectedTimeRange.value = [startTimeStr, endTimeStr];
  startTime.value = startTimeStr;
  endTime.value = endTimeStr;
};

// 限制时间范围为最近三天
const disabledDate = (time: Date) => {
  const threeDaysAgo = new Date();
  threeDaysAgo.setTime(threeDaysAgo.getTime() - 3 * 24 * 60 * 60 * 1000);
  threeDaysAgo.setHours(0, 0, 0, 0);
  const now = new Date();
  now.setHours(23, 59, 59, 999);
  return time < threeDaysAgo || time > now;
};

// 重置日志筛选
const resetLogFilter = () => {
  selectedDeviceIds.value = [];
  selectedLogLevels.value = [];
  setDefaultTimeRange();
  logCurrentPage.value = 1;
  logPagination.value = {
    skip: 0,
    limit: 50
  };
  currentLogs.value = [];
  logTotal.value = 0;
  logsError.value = '';
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

.loading-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 60px 0;
  color: #909399;
  
  .el-icon {
    margin-bottom: 12px;
  }
}

.pagination-container {
  display: flex;
  justify-content: center;
  margin-top: 20px;
  padding: 20px 0;
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
.filter-select,
.filter-datepicker {
  width: 100% !important;
}

/* 移动端时间范围选择 - 经典设计 */
.mobile-time-range {
  display: flex;
  align-items: center;
  gap: 8px;
  width: 100%;
}

.filter-datepicker-mobile {
  flex: 1;
}

.filter-datepicker-mobile :deep(.el-input__wrapper) {
  border-radius: 8px;
  padding: 8px 12px;
}

.filter-datepicker-mobile :deep(.el-input__inner) {
  font-size: 14px;
}

.time-separator {
  font-size: 14px;
  color: #606266;
  white-space: nowrap;
  padding: 0 4px;
}

/* 移动端按钮组 - 确保查询和重置按钮对齐 */
.mobile-button-group {
  display: flex;
  gap: 12px;
  width: 100%;
}

.mobile-action-button {
  flex: 1;
  height: 40px;
  font-size: 14px;
  font-weight: 500;
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

.mobile-device .log-card.info {
  border-left-color: #909399;
}

.mobile-device .log-card.warning {
  border-left-color: #e6a23c;
}

.mobile-device .log-card.error {
  border-left-color: #f56c6c;
}

.mobile-device .log-card.debug {
  border-left-color: #67c23a;
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

.mobile-device .log-tag {
  font-size: 12px;
  color: #909399;
  margin-bottom: 8px;
}

.mobile-device .log-content {
  font-size: 14px;
  color: #606266;
  margin-bottom: 8px;
  line-height: 1.5;
  word-break: break-all;
}

.mobile-device .log-footer {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding-top: 8px;
  border-top: 1px solid #f0f0f0;
}

.mobile-device .log-tick {
  font-size: 12px;
  color: #909399;
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