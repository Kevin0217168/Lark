<template>
  <div class="device-container" :class="{ 'mobile': isMobile }">
    <!-- 桌面端：使用 el-card 包装 -->
    <el-card v-if="!isMobile" class="device-card">
      <h3 class="title">设备管理</h3>
      
      <!-- 总览视图 -->
      <div v-if="activeTab === 'overview'" class="overview-desktop">
        <h4 class="overview-title">鸟笼总览</h4>
        <el-row :gutter="20">
          <el-col :span="8" v-for="(group, gi) in birdcageGroups" :key="gi">
            <div class="birdcage-card">
              <div class="bc-card-header">
                <span class="bc-icon">🏠</span>
                <span class="bc-label">{{ group.label }}</span>
              </div>
              <div class="bc-devices">
                <!-- CAM 设备行 -->
                <div class="bc-device-row cam-row" v-if="group.cam">
                  <span class="bc-device-tag cam-tag">CAM</span>
                  <div class="bc-device-info">
                    <span class="bc-device-name">{{ group.cam.name }}</span>
                    <span class="bc-device-meta">ID: {{ group.cam.id }} | {{ group.cam.device_type }}</span>
                  </div>
                  <div class="bc-device-status">
                    <el-tag :type="group.cam.isOnline ? 'success' : 'danger'" size="small">
                      {{ group.cam.isOnline ? '在线' : '离线' }}
                    </el-tag>
                  </div>
                </div>
                <div class="bc-device-row cam-row" v-else>
                  <span class="bc-device-tag cam-tag dimmed">CAM</span>
                  <span class="bc-no-device">未配置</span>
                </div>
                <!-- C3 设备行 -->
                <div class="bc-device-row c3-row" v-if="group.c3">
                  <span class="bc-device-tag c3-tag">C3</span>
                  <div class="bc-device-info">
                    <span class="bc-device-name">{{ group.c3.name }}</span>
                    <span class="bc-device-meta">ID: {{ group.c3.id }} | {{ group.c3.device_type }}</span>
                  </div>
                  <div class="bc-device-status">
                    <el-tag :type="group.c3.isOnline ? 'success' : 'danger'" size="small">
                      {{ group.c3.isOnline ? '在线' : '离线' }}
                    </el-tag>
                  </div>
                </div>
                <div class="bc-device-row c3-row" v-else>
                  <span class="bc-device-tag c3-tag dimmed">C3</span>
                  <span class="bc-no-device">未配置</span>
                </div>
              </div>
              <!-- 固件版本 -->
              <div class="bc-versions" v-if="group.cam || group.c3">
                <div class="bc-ver-item" v-if="group.cam">
                  固件: {{ deviceVersions[group.cam.id] || '加载中...' }}
                </div>
                <div class="bc-ver-item" v-if="group.c3">
                  固件: {{ deviceVersions[group.c3.id] || '加载中...' }}
                </div>
              </div>
            </div>
          </el-col>
        </el-row>
        <div v-if="birdcageGroups.length === 0" class="no-devices">
          <el-empty description="暂无鸟笼数据" />
        </div>
      </div>
      
      <!-- 管理视图 -->
      <div v-else-if="activeTab === 'management'">
        <div class="management-header">
          <h4>设备管理</h4>
          <div class="add-device-section">
            <el-button type="primary" @click="showAddDialog" :disabled="!isRootUser">添加设备</el-button>
            <div v-if="!isRootUser" class="permission-hint-desktop">
              仅管理员用户可操作
            </div>
          </div>
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
            v-model="filterForm.birdcage" 
            placeholder="选择鸟笼" 
            style="width: 200px; margin-right: 10px;"
            clearable
          >
            <el-option 
              v-for="group in birdcageGroups" 
              :key="`${group.area}|${group.number}`"
              :label="group.label" 
              :value="`${group.area}|${group.number}`"
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
              <el-button size="small" @click="handleEdit(scope.row)" :disabled="!isRootUser">编辑</el-button>
              <el-button size="small" type="success" @click="handleUpdateFirmware(scope.row)" :disabled="!isRootUser">更新固件</el-button>
              <el-button size="small" type="warning" @click="handleRestart(scope.row)" :disabled="!isRootUser">重启</el-button>
              <el-button size="small" type="danger" @click="handleDelete(scope.row)" :disabled="!isRootUser">删除</el-button>
              <div v-if="!isRootUser" style="margin-top: 5px; font-size: 12px; color: #909399;">
                仅管理员用户可操作
              </div>
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
            v-model="logsBirdcageKey" 
            placeholder="选择鸟笼" 
            style="width: 200px; margin-right: 10px;"
            clearable
            @change="handleLogsBirdcageChange"
          >
            <el-option 
              v-for="group in birdcageGroups" 
              :key="`${group.area}|${group.number}`"
              :label="group.label" 
              :value="`${group.area}|${group.number}`"
            />
          </el-select>
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
            v-model="selectedEndTime"
            type="datetime"
            placeholder="选择结束时间"
            style="width: 220px; margin-right: 10px;"
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
        
        <!-- 终端样式日志 -->
        <div v-else class="terminal-logs">
          <div class="terminal-header">
            <span class="terminal-title">📟 日志终端</span>
            <span class="terminal-count">共 {{ logTotal }} 条</span>
            <el-button 
              :type="wrapEnabled ? 'primary' : 'default'" 
              size="small" 
              @click="wrapEnabled = !wrapEnabled"
              class="wrap-toggle"
            >
              {{ wrapEnabled ? '自动换行' : '不换行' }}
            </el-button>
          </div>
          <div class="terminal-content" :class="{ 'no-wrap': !wrapEnabled }" ref="terminalContentRef">
            <div 
              v-for="log in currentLogs" 
              :key="log.id" 
              class="terminal-line"
              :class="log.level.toLowerCase()"
            >
              <span class="terminal-timestamp">[{{ formatTimestamp(log.timestamp) }}]</span>
              <span class="terminal-device">[设备{{ log.device_id }}]</span>
              <span class="terminal-level" :class="log.level.toLowerCase()">{{ log.level.padEnd(7) }}</span>
              <span class="terminal-tag">[{{ log.tag }}]</span>
              <span class="terminal-text">{{ log.content }}</span>
            </div>
          </div>
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
          <h3>鸟笼总览</h3>
        </div>
        <div class="device-list">
          <div 
            v-for="(group, gi) in birdcageGroups" 
            :key="gi" 
            class="birdcage-card-mobile"
          >
            <div class="bcm-header">
              <span class="bcm-icon">🏠</span>
              <span class="bcm-label">{{ group.label }}</span>
            </div>
            <!-- CAM 设备行 -->
            <div class="bcm-device-row cam-row" v-if="group.cam">
              <span class="bc-device-tag cam-tag">CAM</span>
              <div class="bcm-device-info">
                <span class="bcm-device-name">{{ group.cam.name }}</span>
                <span class="bcm-device-meta">ID: {{ group.cam.id }} | {{ group.cam.device_type }}</span>
              </div>
              <el-tag :type="group.cam.isOnline ? 'success' : 'danger'" size="small">
                {{ group.cam.isOnline ? '在线' : '离线' }}
              </el-tag>
            </div>
            <div class="bcm-device-row cam-row missing" v-else>
              <span class="bc-device-tag cam-tag dimmed">CAM</span>
              <span class="bcm-missing">未配置</span>
            </div>
            <!-- C3 设备行 -->
            <div class="bcm-device-row c3-row" v-if="group.c3">
              <span class="bc-device-tag c3-tag">C3</span>
              <div class="bcm-device-info">
                <span class="bcm-device-name">{{ group.c3.name }}</span>
                <span class="bcm-device-meta">ID: {{ group.c3.id }} | {{ group.c3.device_type }}</span>
              </div>
              <el-tag :type="group.c3.isOnline ? 'success' : 'danger'" size="small">
                {{ group.c3.isOnline ? '在线' : '离线' }}
              </el-tag>
            </div>
            <div class="bcm-device-row c3-row missing" v-else>
              <span class="bc-device-tag c3-tag dimmed">C3</span>
              <span class="bcm-missing">未配置</span>
            </div>
            <!-- 固件版本 -->
            <div class="bcm-versions" v-if="group.cam || group.c3">
              <div class="bcm-ver-item" v-if="group.cam">
                固件: {{ deviceVersions[group.cam.id] || '加载中...' }}
              </div>
              <div class="bcm-ver-item" v-if="group.c3">
                固件: {{ deviceVersions[group.c3.id] || '加载中...' }}
              </div>
            </div>
          </div>
        </div>
        <div v-if="birdcageGroups.length === 0" class="empty-state">
          <el-empty description="暂无鸟笼数据" />
        </div>
      </div>
      
      <!-- 管理视图 -->
      <div v-else-if="activeTab === 'management'" class="management-container">
        <div class="management-header">
          <h3>设备管理</h3>
          <el-button type="primary" size="small" @click="showAddDialog" :disabled="!isRootUser">
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
            v-model="filterForm.birdcage" 
            placeholder="选择鸟笼" 
            clearable
            class="filter-select"
          >
            <el-option 
              v-for="group in birdcageGroups" 
              :key="`${group.area}|${group.number}`"
              :label="group.label" 
              :value="`${group.area}|${group.number}`"
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
              <el-button size="small" @click="handleEdit(device)" :disabled="!isRootUser">编辑</el-button>
              <el-button size="small" type="success" @click="handleUpdateFirmware(device)" :disabled="!isRootUser">更新固件</el-button>
              <el-button size="small" type="warning" @click="handleRestart(device)" :disabled="!isRootUser">重启</el-button>
              <el-button size="small" type="danger" @click="handleDelete(device)" :disabled="!isRootUser">删除</el-button>
            </div>
            <div v-if="!isRootUser" class="permission-hint">
              仅管理员用户可操作
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
            v-model="logsBirdcageKey" 
            placeholder="选择鸟笼" 
            clearable
            class="filter-select"
            @change="handleLogsBirdcageChange"
          >
            <el-option 
              v-for="group in birdcageGroups" 
              :key="`${group.area}|${group.number}`"
              :label="group.label" 
              :value="`${group.area}|${group.number}`"
            />
          </el-select>
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
          <el-date-picker
            v-model="selectedEndTime"
            type="datetime"
            placeholder="选择结束时间"
            class="filter-datepicker-mobile"
            value-format="YYYY-MM-DDTHH:mm:ss"
            :disabled-date="disabledDate"
            @change="handleTimeChange"
          />
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
        
        <!-- 终端样式日志 -->
        <div v-else class="terminal-logs mobile-terminal">
          <div class="terminal-header">
            <span class="terminal-title">📟 日志终端</span>
            <span class="terminal-count">共 {{ logTotal }} 条</span>
            <el-button 
              :type="wrapEnabled ? 'primary' : 'default'" 
              size="small" 
              @click="wrapEnabled = !wrapEnabled"
              class="wrap-toggle"
            >
              {{ wrapEnabled ? '换行' : '不换行' }}
            </el-button>
          </div>
          <div class="terminal-content" :class="{ 'no-wrap': !wrapEnabled }">
            <div 
              v-for="log in currentLogs" 
              :key="log.id" 
              class="terminal-line"
              :class="log.level.toLowerCase()"
            >
              <span class="terminal-timestamp">[{{ formatTimestamp(log.timestamp) }}]</span>
              <span class="terminal-device">[设备{{ log.device_id }}]</span>
              <span class="terminal-level" :class="log.level.toLowerCase()">{{ log.level.padEnd(7) }}</span>
              <span class="terminal-tag">[{{ log.tag }}]</span>
              <span class="terminal-text">{{ log.content }}</span>
            </div>
          </div>
        </div>
        

        
        <!-- 空数据提示 -->
        <el-empty 
          v-if="selectedDeviceIds.length > 0 && !logsLoading && !logsError && currentLogs.length === 0" 
          description="暂无日志数据" 
        />
      </div>
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

const birdcageGroups = computed(() => {
  const groups: { area: string; number: number; label: string; cam: Device | null; c3: Device | null }[] = [];
  const seen = new Map<string, number>();
  for (const rd of devices.value as Device[]) {
    const d: any = rd;
    const key = `${d.area || ''}-${d.number ?? 0}`;
    if (!seen.has(key)) {
      seen.set(key, groups.length);
      groups.push({
        area: d.area || '',
        number: d.number ?? 0,
        label: d.area ? `${d.area} #${d.number}` : '未绑定',
        cam: null,
        c3: null,
      });
    }
    const idx = seen.get(key) as number;
    if (d.device_type === 'ESP32-CAM') groups[idx]!.cam = d;
    else if (d.device_type === 'ESP32-C3') groups[idx]!.c3 = d;
  }
  groups.sort((a, b) => a.label.localeCompare(b.label));
  return groups;
});

// 检查是否为 root 用户
const isRootUser = computed(() => {
  const role = localStorage.getItem('role');
  return role === 'root';
});

// 获取设备列表
const fetchDevices = async () => {
  try {
    await storeFetchDevices();
    // 获取每个设备的固件版本（顺序拉取，避免并发刷新 token / 请求风暴）
      for (const device of devices.value as Device[]) {
        await fetchDeviceVersion(device.id);
      }
  } catch (error) {
    ElMessage.error('获取设备列表失败，请检查网络连接');
  }
};

// 获取设备固件版本
const fetchDeviceVersion = async (deviceId: number) => {
  try {
    const data = await api.get(`/api/devices/${deviceId}/version`);
    deviceVersions.value[deviceId] = data.data.values;
  } catch (error: any) {
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
const selectedEndTime = ref<string>('');
const logsBirdcageKey = ref('');

const handleLogsBirdcageChange = () => {
  if (!logsBirdcageKey.value) return;
  const [area, numStr] = logsBirdcageKey.value.split('|');
  const number = parseInt(numStr || '0', 10);
  const cageDevices = (devices.value as Device[]).filter((d: any) => d.area === area && d.number === number);
  const currentIds = new Set(selectedDeviceIds.value);
  for (const d of cageDevices) {
    currentIds.add(d.id);
  }
  selectedDeviceIds.value = Array.from(currentIds);
};

// 日志相关状态
const logsLoading = ref(false);
const logsError = ref('');
const currentLogs = ref<DeviceLog[]>([]);
const logTotal = ref(0);
const logPagination = ref({
  skip: 0,
  limit: 500
});
const wrapEnabled = ref(true);

// 筛选表单
const filterForm = ref({
  name: '',
  area: [] as string[],
  isOnline: null as boolean | null,
  birdcage: '' as string,
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
    let birdcageMatch = true;
    if (filterForm.value.birdcage) {
      const [bcArea, bcNumStr] = filterForm.value.birdcage.split('|');
      birdcageMatch = device.area === bcArea && device.number === parseInt(bcNumStr || '0', 10);
    }
    return nameMatch && areaMatch && onlineMatch && birdcageMatch;
  });
});

// 重置筛选
const resetFilter = () => {
  filterForm.value = {
    name: '',
    area: [],
    isOnline: null,
    birdcage: '',
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

  try {
    const { fetchDeviceLogs } = useDeviceStore();
    const levels = selectedLogLevels.value.length > 0 ? selectedLogLevels.value : undefined;
    const endTime = selectedEndTime.value || undefined;

    const result = await fetchDeviceLogs(
      selectedDeviceIds.value,
      0,
      logPagination.value.limit,
      levels,
      undefined,
      endTime
    );

    if (result.success) {
      currentLogs.value = result.data.sort((a: DeviceLog, b: DeviceLog) => {
        return new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime();
      });
      logTotal.value = result.total;
      ElMessage.success(`成功加载 ${result.data.length} 条日志`);
    } else {
      logsError.value = '获取日志失败，请稍后重试';
    }
  } catch (error: any) {
    logsError.value = error.response?.data?.detail || '获取日志失败，请检查网络连接';
  } finally {
    logsLoading.value = false;
  }
};

// 设备选择变化
const handleDeviceChange = () => {
  currentLogs.value = [];
  logTotal.value = 0;
  logsError.value = '';
  if (selectedDeviceIds.value.length > 0) {
    fetchLogs();
  }
};

// 日志等级变化
const handleLevelChange = () => {
  if (selectedDeviceIds.value.length > 0) {
    fetchLogs();
  }
};

// 时间范围变化
const handleTimeChange = () => {
  if (selectedDeviceIds.value.length > 0) {
    fetchLogs();
  }
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

// 设置默认结束时间为当前时间
const setDefaultTimeRange = () => {
  const now = new Date();
  selectedEndTime.value = formatDateForPicker(now);
};

// 限制时间不能晚于当前时间
const disabledDate = (time: Date) => {
  const now = new Date();
  now.setHours(23, 59, 59, 999);
  return time > now;
};

// 重置日志筛选
const resetLogFilter = () => {
  selectedDeviceIds.value = [];
  selectedLogLevels.value = [];
  setDefaultTimeRange();
  logPagination.value = {
    skip: 0,
    limit: 500
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
  if (!isRootUser.value) {
    ElMessage.warning('仅管理员用户可添加设备');
    return;
  }
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
  if (!isRootUser.value) {
    ElMessage.warning('仅管理员用户可编辑设备');
    return;
  }
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
  if (!isRootUser.value) {
    ElMessage.warning('仅管理员用户可保存设备');
    return;
  }
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

    // 检查鸟笼约束：同一区域+编号下，同类型设备只能有一台
    const deviceId = parseInt(deviceForm.value.id);
    const sameTypeInCage = devices.value.find(d => 
      d.id !== deviceId && 
      d.area === deviceForm.value.area && 
      d.number === deviceForm.value.number &&
      d.device_type === deviceForm.value.device_type
    );

    if (sameTypeInCage) {
      ElMessage.error(`${deviceForm.value.device_type || '该类型'} 设备在区域 "${deviceForm.value.area}" #${deviceForm.value.number} 中已存在`);
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

    // 检查鸟笼约束：同一区域+编号下，同类型设备只能有一台
    const sameTypeInCage = devices.value.find(d => 
      d.area === deviceForm.value.area && 
      d.number === deviceForm.value.number &&
      d.device_type === deviceForm.value.device_type
    );

    if (sameTypeInCage) {
      ElMessage.error(`${deviceForm.value.device_type || '该类型'} 设备在区域 "${deviceForm.value.area}" #${deviceForm.value.number} 中已存在`);
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
  if (!isRootUser.value) {
    ElMessage.warning('仅管理员用户可删除设备');
    return;
  }
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
  if (!isRootUser.value) {
    ElMessage.warning('仅管理员用户可更新固件');
    return;
  }
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
  if (!isRootUser.value) {
    ElMessage.warning('仅管理员用户可重启设备');
    return;
  }
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
  border-radius: 16px;
  padding: 24px;
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(12px);
  -webkit-backdrop-filter: blur(12px);
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(255, 255, 255, 0.6);
}

/* ==================== 鸟笼总览（桌面端） ==================== */
.overview-desktop {
  .overview-title {
    font-size: 18px;
    font-weight: 600;
    color: #166534;
    margin: 0 0 20px 0;
    text-align: center;
  }
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

/* 终端样式日志 */
.terminal-logs {
  background: #1e1e1e;
  border-radius: 8px;
  overflow: hidden;
  font-family: 'Consolas', 'Monaco', 'Courier New', monospace;
}

.terminal-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 12px 16px;
  background: #2d2d2d;
  border-bottom: 1px solid #3c3c3c;
}

.terminal-title {
  color: #4ec9b0;
  font-size: 14px;
  font-weight: 600;
}

.terminal-count {
  color: #858585;
  font-size: 12px;
}

.terminal-content {
  padding: 12px 16px;
  max-height: 500px;
  overflow-y: auto;
  background: #1e1e1e;
}

.terminal-content.no-wrap {
  overflow-x: auto;
  white-space: nowrap;
}

.terminal-content.no-wrap .terminal-line {
  white-space: nowrap;
}

.terminal-header .wrap-toggle {
  margin-left: auto;
}

.terminal-line {
  display: flex;
  flex-wrap: wrap;
  padding: 4px 0;
  font-size: 13px;
  line-height: 1.5;
  color: #d4d4d4;
  border-bottom: 1px solid #2a2a2a;
}

.terminal-line:last-child {
  border-bottom: none;
}

.terminal-timestamp {
  color: #858585;
  margin-right: 8px;
  white-space: nowrap;
}

.terminal-device {
  color: #9cdcfe;
  margin-right: 8px;
  white-space: nowrap;
}

.terminal-level {
  margin-right: 8px;
  white-space: nowrap;
  font-weight: 600;
}

.terminal-level.info {
  color: #909399;
}

.terminal-level.warning {
  color: #e6a23c;
}

.terminal-level.error {
  color: #f56c6c;
}

.terminal-level.debug {
  color: #67c23a;
}

.terminal-tag {
  color: #c586c0;
  margin-right: 8px;
  white-space: nowrap;
}

.terminal-text {
  color: #d4d4d4;
  word-break: break-all;
}

/* 移动端终端样式 */
.mobile-terminal .terminal-content {
  max-height: 400px;
  padding: 8px 12px;
}

.mobile-terminal .terminal-line {
  font-size: 12px;
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

.permission-hint {
  margin-top: 8px;
  font-size: 12px;
  color: #909399;
  text-align: center;
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

.filter-datepicker-mobile {
  width: 100%;
}

.filter-datepicker-mobile :deep(.el-input__wrapper) {
  border-radius: 8px;
  padding: 8px 12px;
}

.filter-datepicker-mobile :deep(.el-input__inner) {
  font-size: 14px;
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

/* ==================== 鸟笼总览卡片（桌面端） ==================== */
.birdcage-card {
  background: #fff;
  border: 1px solid rgba(139, 173, 66, 0.12);
  border-radius: 14px;
  padding: 0;
  margin-bottom: 20px;
  overflow: hidden;
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.04);
  transition: all 0.3s ease;

  &:hover {
    box-shadow: 0 6px 24px rgba(139, 173, 66, 0.1);
    transform: translateY(-2px);
  }

  .bc-card-header {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 16px 18px;
    background: linear-gradient(135deg, #f0fdf4 0%, #e8f5e9 100%);
    border-bottom: 1px solid rgba(139, 173, 66, 0.1);

    .bc-icon {
      font-size: 22px;
    }

    .bc-label {
      font-size: 16px;
      font-weight: 700;
      color: #166534;
      text-shadow: 0 1px 2px rgba(0, 0, 0, 0.06);
    }
  }

  .bc-devices {
    padding: 12px 0;
  }

  .bc-device-row {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 10px 18px;

    & + .bc-device-row {
      border-top: 1px solid #f5f5f5;
    }
  }

  .bc-device-tag {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    width: 40px;
    height: 24px;
    border-radius: 6px;
    font-size: 11px;
    font-weight: 700;
    color: #fff;
    flex-shrink: 0;

    &.cam-tag {
      background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
    }

    &.c3-tag {
      background: linear-gradient(135deg, #60a5fa 0%, #3b82f6 100%);
    }

    &.dimmed {
      opacity: 0.4;
    }
  }

  .bc-device-info {
    flex: 1;
    min-width: 0;
    display: flex;
    flex-direction: column;
    gap: 2px;

    .bc-device-name {
      font-size: 14px;
      font-weight: 600;
      color: #303133;
    }

    .bc-device-meta {
      font-size: 11px;
      color: #909399;
    }
  }

  .bc-device-status {
    flex-shrink: 0;
  }

  .bc-no-device {
    font-size: 13px;
    color: #c0c4cc;
    font-style: italic;
  }

  .bc-versions {
    padding: 10px 18px 14px 18px;
    border-top: 1px dashed #e8e8e8;
    display: flex;
    flex-direction: column;
    gap: 4px;

    .bc-ver-item {
      font-size: 12px;
      color: #909399;
    }
  }
}

/* ==================== 鸟笼总览卡片（移动端） ==================== */
.birdcage-card-mobile {
  background: #fff;
  border: 1px solid rgba(139, 173, 66, 0.1);
  border-radius: 12px;
  margin-bottom: 14px;
  overflow: hidden;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.04);

  .bcm-header {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 14px 16px;
    background: linear-gradient(135deg, #f0fdf4 0%, #e8f5e9 100%);
    border-bottom: 1px solid rgba(139, 173, 66, 0.08);

    .bcm-icon {
      font-size: 20px;
    }

    .bcm-label {
      font-size: 15px;
      font-weight: 700;
      color: #166534;
    }
  }

  .bcm-device-row {
    display: flex;
    align-items: center;
    gap: 8px;
    padding: 12px 16px;

    & + .bcm-device-row {
      border-top: 1px solid #f5f5f5;
    }

    &.missing {
      opacity: 0.5;
    }

    .bcm-device-info {
      flex: 1;
      min-width: 0;
      display: flex;
      flex-direction: column;
      gap: 2px;

      .bcm-device-name {
        font-size: 13px;
        font-weight: 600;
        color: #303133;
      }

      .bcm-device-meta {
        font-size: 11px;
        color: #909399;
      }
    }

    .bc-device-tag {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 36px;
      height: 22px;
      border-radius: 5px;
      font-size: 10px;
      font-weight: 700;
      color: #fff;
      flex-shrink: 0;

      &.cam-tag {
        background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
      }

      &.c3-tag {
        background: linear-gradient(135deg, #60a5fa 0%, #3b82f6 100%);
      }

      &.dimmed {
        opacity: 0.4;
      }
    }

    .bcm-missing {
      font-size: 12px;
      color: #c0c4cc;
      font-style: italic;
    }
  }

  .bcm-versions {
    padding: 10px 16px 12px 16px;
    border-top: 1px dashed #e8e8e8;
    display: flex;
    flex-direction: column;
    gap: 3px;

    .bcm-ver-item {
      font-size: 11px;
      color: #909399;
    }
  }
}
</style>