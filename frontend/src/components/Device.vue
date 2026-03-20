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
            <p>设备ID: {{ device.id }}</p>
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
      
      <el-table :data="devices" style="width: 100%" border>
        <el-table-column prop="name" label="设备名称" min-width="150" />
        <el-table-column prop="id" label="设备ID" min-width="100" />
        <el-table-column label="在线状态" min-width="120">
          <template #default="scope">
            <el-tag :type="scope.row.isOnline ? 'success' : 'danger'">
              {{ scope.row.isOnline ? '在线' : '离线' }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column prop="createTime" label="添加时间" min-width="180" />
        <el-table-column label="操作" min-width="300" fixed="right">
          <template #default="scope">
            <el-button size="small" @click="handleEdit(scope.row)">编辑</el-button>
            <el-button size="small" type="success" @click="handleUpdateFirmware(scope.row)">更新固件</el-button>
            <el-button size="small" type="danger" @click="handleDelete(scope.row)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
      
      <div v-if="devices.length === 0" class="no-devices">
        <p>暂无设备数据</p>
      </div>
    </div>
    
    <!-- 日志视图 -->
    <div v-else-if="activeTab === 'logs'">
      <div class="logs-header">
        <h4>设备日志</h4>
        <el-select v-model="selectedDeviceId" placeholder="选择设备" style="width: 200px;">
          <el-option label="全部设备" :value="null" />
          <el-option 
            v-for="device in devices" 
            :key="device.id" 
            :label="device.name" 
            :value="device.id" 
          />
        </el-select>
      </div>
      
      <el-table :data="filteredLogs" style="width: 100%" border>
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
      
      <div v-if="filteredLogs.length === 0" class="no-logs">
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
      <el-form-item label="设备ID">
        <el-input v-model="deviceForm.id" placeholder="请输入设备ID" :disabled="isEdit" />
      </el-form-item>
    </el-form>
    <template #footer>
      <el-button @click="dialogVisible = false">取消</el-button>
      <el-button type="primary" @click="handleSave">确定</el-button>
    </template>
  </el-dialog>
</template>

<script lang="ts" setup>
import { ref, computed } from 'vue';
import { ElMessage, ElMessageBox } from 'element-plus';
import { useDeviceStore, type Device, type DeviceLog } from '../stores/deviceStore';

// 接收从父组件传递的 activeTab 属性
defineProps<{
  activeTab: string;
}>();

const { devices, getDeviceLogs, addDevice, updateDevice, deleteDevice } = useDeviceStore();

const selectedDeviceId = ref<number | null>(null);

const filteredLogs = computed<DeviceLog[]>(() => {
  const logs = getDeviceLogs(selectedDeviceId.value || undefined);
  return logs.sort((a, b) => {
    return new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime();
  });
});

const dialogVisible = ref(false);
const isEdit = ref(false);
const deviceForm = ref({
  id: '',
  name: '',
  status: 'offline'
});

// 显示添加对话框
const showAddDialog = () => {
  isEdit.value = false;
  deviceForm.value = {
    id: '',
    name: '',
    status: 'offline'
  };
  dialogVisible.value = true;
};

// 编辑设备
const handleEdit = (device: Device) => {
  isEdit.value = true;
  deviceForm.value = {
    id: device.id.toString(),
    name: device.name,
    status: device.status
  };
  dialogVisible.value = true;
};

// 保存设备
const handleSave = () => {
  if (!deviceForm.value.name || !deviceForm.value.id) {
    ElMessage.warning('请填写完整信息');
    return;
  }

  if (isEdit.value) {
    // 编辑模式
    updateDevice(parseInt(deviceForm.value.id), {
      name: deviceForm.value.name
    });
    ElMessage.success('设备更新成功');
  } else {
    // 添加模式，状态默认为 offline，等待后端更新
    const newDevice: Device = {
      id: parseInt(deviceForm.value.id),
      name: deviceForm.value.name,
      status: 'standby', // 默认为待机状态
      isOnline: false, // 默认为离线
      createTime: new Date().toLocaleString('zh-CN')
    };
    addDevice(newDevice);
    ElMessage.success('设备添加成功');
  }
  dialogVisible.value = false;
};

// 删除设备
const handleDelete = (device: Device) => {
  ElMessageBox.confirm(
    `确定要删除设备 "${device.name}" 吗？`,
    '警告',
    {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'warning',
    }
  ).then(() => {
    deleteDevice(device.id);
    ElMessage.success('设备删除成功');
  }).catch(() => {
    // 取消删除
  });
};

// 更新固件
const handleUpdateFirmware = (device: Device) => {
  ElMessageBox.confirm(
    `确定要为设备 "${device.name}" 更新固件吗？`,
    '更新固件',
    {
      confirmButtonText: '确定',
      cancelButtonText: '取消',
      type: 'info',
    }
  ).then(() => {
    // 模拟固件更新
      ElMessage.success(`设备 "${device.name}" 固件更新成功`);

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
</style>