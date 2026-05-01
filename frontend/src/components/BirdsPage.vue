<template>
  <div class="birds-page">
    <div class="birds-header">
      <h1>雏鸟管理</h1>
      <el-button type="primary" @click="handleAddBird">
        <el-icon><Plus /></el-icon>
        新增雏鸟
      </el-button>
    </div>
    
    <!-- 筛选条件 -->
    <div class="birds-filter">
      <el-form :inline="true" :model="filterForm" class="filter-form">
        <el-form-item label="名称">
          <el-input v-model="filterForm.name" placeholder="请输入雏鸟名称" />
        </el-form-item>
        <el-form-item label="品种">
          <el-input v-model="filterForm.species" placeholder="请输入品种" />
        </el-form-item>
        <el-form-item label="状态">
          <el-select v-model="filterForm.status" placeholder="请选择状态">
            <el-option label="可认领" value="available" />
            <el-option label="已认领" value="adopted" />
            <el-option label="已成年" value="grown" />
          </el-select>
        </el-form-item>
        <el-form-item>
          <el-button type="primary" @click="handleSearch">查询</el-button>
          <el-button @click="resetFilter">重置</el-button>
        </el-form-item>
      </el-form>
    </div>
    
    <!-- 雏鸟列表 - 桌面端 -->
    <div class="birds-content desktop-content">
      <el-table :data="birdsList" style="width: 100%">
        <el-table-column prop="id" label="ID" width="80" />
        <el-table-column prop="name" label="名称" />
        <el-table-column prop="species" label="品种" />
        <el-table-column prop="birth_date" label="出生日期" />
        <el-table-column prop="status" label="状态">
          <template #default="scope">
            <el-tag :type="getStatusType(scope.row.status)">
              {{ getStatusText(scope.row.status) }}
            </el-tag>
          </template>
        </el-table-column>
        <el-table-column label="鸟笼" width="140">
          <template #default="scope">
            <span v-if="scope.row.area">{{ scope.row.area }} #{{ scope.row.number }}</span>
            <span v-else class="no-cage">未绑定</span>
          </template>
        </el-table-column>
        <el-table-column label="操作" width="180" fixed="right">
          <template #default="scope">
            <el-button size="small" @click="handleEditBird(scope.row)">编辑</el-button>
            <el-button size="small" type="danger" @click="handleDeleteBird(scope.row.id)">删除</el-button>
          </template>
        </el-table-column>
      </el-table>
      
      <!-- 分页 -->
      <div class="pagination" v-if="total > 0">
        <el-pagination
          v-model:current-page="currentPage"
          v-model:page-size="pageSize"
          :page-sizes="[10, 20, 50, 100]"
          layout="total, sizes, prev, pager, next, jumper"
          :total="total"
          @size-change="handleSizeChange"
          @current-change="handleCurrentChange"
        />
      </div>
      
      <!-- 空状态 -->
      <div class="empty-content" v-if="birdsList.length === 0 && !loading">
        <el-empty description="暂无雏鸟数据" />
      </div>
    </div>
    
    <!-- 雏鸟卡片 - 移动端 -->
    <div class="mobile-content" v-if="birdsList.length > 0">
      <div class="birds-grid">
        <div 
          v-for="(bird, index) in birdsList" 
          :key="bird.id"
          class="bird-card"
        >
          <div class="card-header">
            <div class="bird-name">{{ bird.name }}</div>
            <el-tag :type="getStatusType(bird.status)">
              {{ getStatusText(bird.status) }}
            </el-tag>
          </div>
          <div class="card-body">
            <div class="bird-info">
              <span class="info-label">品种:</span>
              <span class="info-value">{{ bird.species }}</span>
            </div>
            <div class="bird-info">
              <span class="info-label">出生日期:</span>
              <span class="info-value">{{ bird.birth_date }}</span>
            </div>
            <div class="bird-info" v-if="bird.area">
              <span class="info-label">鸟笼:</span>
              <span class="info-value">{{ bird.area }} #{{ bird.number }}</span>
            </div>
          </div>
          <div class="card-footer">
            <el-button size="small" @click="handleEditBird(bird)">编辑</el-button>
            <el-button size="small" type="danger" @click="handleDeleteBird(bird.id)">删除</el-button>
          </div>
        </div>
      </div>
      
      <!-- 移动端分页 -->
      <div class="mobile-pagination" v-if="total > 0">
        <el-pagination
          v-model:current-page="currentPage"
          v-model:page-size="pageSize"
          :page-sizes="[10, 20, 50, 100]"
          layout="total, prev, pager, next"
          :total="total"
          @size-change="handleSizeChange"
          @current-change="handleCurrentChange"
        />
      </div>
    </div>
    
    <!-- 移动端空状态 -->
    <div class="mobile-empty" v-if="birdsList.length === 0 && !loading">
      <div class="empty-icon">
        <el-icon :size="64"><Avatar /></el-icon>
      </div>
      <div class="empty-text">暂无雏鸟数据</div>
      <div class="empty-desc">点击上方按钮添加新雏鸟</div>
    </div>
    
    <!-- 新增/编辑雏鸟对话框 -->
    <el-dialog v-model="dialogVisible" :title="dialogTitle" :width="isMobile ? '90%' : '500px'">
      <el-form :model="birdForm" :rules="rules" ref="birdFormRef" :label-width="isMobile ? '80px' : '100px'">
        <el-form-item label="名称" prop="name">
          <el-input v-model="birdForm.name" placeholder="请输入雏鸟名称" />
        </el-form-item>
        <el-form-item label="品种" prop="species">
          <el-input v-model="birdForm.species" placeholder="请输入品种" />
        </el-form-item>
        <el-form-item label="出生日期" prop="birth_date">
          <el-date-picker
            v-model="birdForm.birth_date"
            type="date"
            placeholder="选择日期"
            format="YYYY-MM-DD"
            value-format="YYYY-MM-DD"
            style="width: 100%"
          />
        </el-form-item>
        <el-form-item label="鸟笼">
          <el-select v-model="birdCageKey" placeholder="选择鸟笼（可选）" clearable style="width: 100%">
            <el-option
              v-for="group in birdcageGroups"
              :key="`${group.area}|${group.number}`"
              :label="group.label"
              :value="`${group.area}|${group.number}`"
            />
          </el-select>
        </el-form-item>
        <el-form-item label="状态" prop="status">
          <el-select v-model="birdForm.status" placeholder="请选择状态">
            <el-option label="可认领" value="available" />
            <el-option label="已认领" value="adopted" />
            <el-option label="已成年" value="grown" />
          </el-select>
        </el-form-item>
        <el-form-item label="描述">
          <el-input v-model="birdForm.description" type="textarea" placeholder="请输入描述" :rows="isMobile ? 3 : 4" />
        </el-form-item>
        <el-form-item label="头像URL">
          <el-input v-model="birdForm.avatar_url" placeholder="请输入头像URL" />
        </el-form-item>
      </el-form>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="dialogVisible = false">取消</el-button>
          <el-button type="primary" @click="handleSaveBird">保存</el-button>
        </span>
      </template>
    </el-dialog>
    
    <!-- 删除确认对话框 -->
    <el-dialog v-model="deleteDialogVisible" title="确认删除" :width="isMobile ? '80%' : '300px'">
      <span>确定要删除该雏鸟吗？此操作不可恢复。</span>
      <template #footer>
        <span class="dialog-footer">
          <el-button @click="deleteDialogVisible = false">取消</el-button>
          <el-button type="danger" @click="confirmDelete">确定删除</el-button>
        </span>
      </template>
    </el-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, onMounted, computed } from 'vue';
import { ElMessage, ElMessageBox } from 'element-plus';
import { Plus, Avatar } from '@element-plus/icons-vue';
import { api } from '@/utils/api';

// 检测是否为移动设备
const isMobile = ref(window.innerWidth < 768);

// 监听窗口大小变化
const handleWindowResize = () => {
  isMobile.value = window.innerWidth < 768;
};

onMounted(() => {
  window.addEventListener('resize', handleWindowResize);
  fetchBirds();
  fetchBirdcageGroups();
});

// 类型定义
interface Bird {
  id: number;
  name: string;
  species: string;
  birth_date: string;
  status: string;
  area?: string;
  number?: number;
  description?: string;
  avatar_url?: string;
  created_at?: string;
  updated_at?: string;
}

// 响应式数据
const birdsList = ref<Bird[]>([]);
const loading = ref(false);
const total = ref(0);
const currentPage = ref(1);
const pageSize = ref(10);

// 筛选表单
const filterForm = reactive({
  name: '',
  species: '',
  status: ''
});

// 对话框
const dialogVisible = ref(false);
const deleteDialogVisible = ref(false);
const dialogTitle = ref('新增雏鸟');
const birdForm = reactive({
  id: 0,
  name: '',
  species: '',
  birth_date: '',
  status: 'available',
  area: '',
  number: undefined as number | undefined,
  description: '',
  avatar_url: ''
});
const birdFormRef = ref();
const birdIdToDelete = ref(0);
const birdCageKey = ref('');
const birdcageGroups = ref<{ area: string; number: number; label: string; devices: any[] }[]>([]);

// 表单验证规则
const rules = reactive({
  name: [{ required: true, message: '请输入雏鸟名称', trigger: 'blur' }],
  species: [{ required: true, message: '请输入品种', trigger: 'blur' }],
  birth_date: [{ required: true, message: '请选择出生日期', trigger: 'blur' }],
  status: [{ required: true, message: '请选择状态', trigger: 'blur' }]
});

// 状态文本映射
const getStatusText = (status: string): string => {
  const statusMap: Record<string, string> = {
    'available': '可认领',
    'adopted': '已认领',
    'grown': '已成年'
  };
  return statusMap[status] || status;
};

// 状态标签类型映射
const getStatusType = (status: string): string => {
  const typeMap: Record<string, string> = {
    'available': 'success',
    'adopted': 'warning',
    'grown': 'info'
  };
  return typeMap[status] || '';
};

// 获取雏鸟列表
const fetchBirds = async () => {
  loading.value = true;
  try {
    // 构建查询参数，只包含非空值
    const params: any = {};
    
    if (filterForm.name.trim()) {
      params.name = filterForm.name.trim();
    }
    
    if (filterForm.species) {
      params.species = filterForm.species;
    }
    
    if (filterForm.status) {
      params.status = filterForm.status;
    }

    // 调用后端 API
    const response = await api.get('/api/birds', params);

    if (response.code === 200) {
      birdsList.value = response.data || [];
      total.value = birdsList.value.length;
    } else {
      ElMessage.error('获取雏鸟列表失败');
    }
  } catch (error) {
    ElMessage.error('获取雏鸟列表失败');
    console.error('获取雏鸟列表失败:', error);
  } finally {
    loading.value = false;
  }
};

// 获取可用鸟笼列表
const fetchBirdcageGroups = async () => {
  try {
    const response = await api.get('/api/devices/birdcage-groups');
    if (response.code === 200) {
      birdcageGroups.value = response.data || [];
    }
  } catch (error) {
    console.error('获取鸟笼列表失败:', error);
  }
};

// 新增雏鸟
const handleAddBird = () => {
  dialogTitle.value = '新增雏鸟';
  birdCageKey.value = '';
  Object.assign(birdForm, {
    id: 0,
    name: '',
    species: '',
    birth_date: '',
    status: 'available',
    area: '',
    number: undefined,
    description: '',
    avatar_url: ''
  });
  fetchBirdcageGroups();
  dialogVisible.value = true;
};

// 编辑雏鸟
const handleEditBird = (bird: Bird) => {
  dialogTitle.value = '编辑雏鸟';
  Object.assign(birdForm, {
    ...bird,
    area: bird.area || '',
    number: bird.number ?? undefined,
  });
  // 只有 area 和 number 同时有效时才设置 birdCageKey
  birdCageKey.value = bird.area && bird.number != null ? `${bird.area}|${bird.number}` : '';
  fetchBirdcageGroups();
  dialogVisible.value = true;
};

// 删除雏鸟
const handleDeleteBird = (id: number) => {
  birdIdToDelete.value = id;
  deleteDialogVisible.value = true;
};

// 确认删除
const confirmDelete = async () => {
  try {
    const response = await api.delete(`/api/birds/${birdIdToDelete.value}`);
    if (response.code === 200) {
      ElMessage.success('删除成功');
      deleteDialogVisible.value = false;
      fetchBirds();
    } else {
      // 显示后端返回的错误信息
      ElMessage.error(response.msg || '删除失败');
    }
  } catch (error: any) {
    // 处理网络错误或其他异常
    if (error.response) {
      // 服务器返回了错误状态码
      ElMessage.error(error.response.data?.msg || '删除失败');
    } else if (error.message) {
      // 网络错误等
      ElMessage.error(error.message);
    } else {
      ElMessage.error('删除失败');
    }
    console.error('删除雏鸟失败:', error);
  }
};

// 保存雏鸟
const handleSaveBird = async () => {
  if (!birdFormRef.value) return;
  
  await birdFormRef.value.validate(async (valid: boolean) => {
    if (valid) {
      try {
        // 从鸟笼选择器中解析 area/number
        const payload: any = { ...birdForm };
        delete (payload as any).id;
        
        if (birdCageKey.value) {
          const [area, numStr] = birdCageKey.value.split('|');
          payload.area = area;
          payload.number = parseInt(numStr || '0', 10);
        } else {
          payload.area = '';
          payload.number = null;
        }

        let response;
        if (birdForm.id) {
          response = await api.put(`/api/birds/${birdForm.id}`, payload);
        } else {
          response = await api.post('/api/birds', payload);
        }
        
        if (response.code === 200) {
          ElMessage.success(birdForm.id ? '更新成功' : '创建成功');
          dialogVisible.value = false;
          fetchBirds();
        } else {
          ElMessage.error(response.msg || '操作失败');
        }
      } catch (error: any) {
        if (error.response) {
          ElMessage.error(error.response.data?.msg || '操作失败');
        } else if (error.message) {
          ElMessage.error(error.message);
        } else {
          ElMessage.error('操作失败');
        }
        console.error('保存雏鸟失败:', error);
      }
    }
  });
};

// 查询
const handleSearch = () => {
  currentPage.value = 1;
  fetchBirds();
};

// 重置筛选
const resetFilter = () => {
  Object.assign(filterForm, {
    name: '',
    species: '',
    status: ''
  });
  currentPage.value = 1;
  fetchBirds();
};

// 分页
const handleSizeChange = (size: number) => {
  pageSize.value = size;
  fetchBirds();
};

const handleCurrentChange = (current: number) => {
  currentPage.value = current;
  fetchBirds();
};

// 组件挂载时获取数据
onMounted(() => {
  fetchBirds();
});
</script>

<style scoped>
/* 全局样式重置 */
* {
  box-sizing: border-box;
  margin: 0;
  padding: 0;
}

/* 桌面端样式 */
.birds-page {
  width: 90%;
  max-width: 1200px;
  margin: 0 auto;
  padding: 24px;
  position: relative;
  z-index: 1;
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(12px);
  -webkit-backdrop-filter: blur(12px);
  border-radius: 16px;
  box-shadow: 0 4px 24px rgba(0, 0, 0, 0.08);
  border: 1px solid rgba(255, 255, 255, 0.6);
}

/* 页面头部 */
.birds-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
}

.birds-header h1 {
  font-size: 28px;
  font-weight: bold;
  color: #303133;
  margin: 0;
  display: flex;
  align-items: center;
  gap: 12px;
}

.birds-header h1::before {
  content: '';
  display: inline-block;
  width: 4px;
  height: 28px;
  background: linear-gradient(180deg, #8bad42 0%, #6b8e23 100%);
  border-radius: 2px;
}

/* 新增按钮 */
.birds-header .el-button--primary {
  background: linear-gradient(135deg, #8bad42 0%, #7da03a 100%);
  border-color: #8bad42;
  border-radius: 8px;
  padding: 10px 24px;
  font-weight: 500;
  transition: all 0.3s ease;
  box-shadow: 0 2px 8px rgba(139, 173, 66, 0.2);
}

.birds-header .el-button--primary:hover {
  background: linear-gradient(135deg, #7da03a 0%, #6b8e23 100%);
  border-color: #7da03a;
  transform: translateY(-2px);
  box-shadow: 0 4px 12px rgba(139, 173, 66, 0.3);
}

/* 筛选区域 */
.birds-filter {
  background: white;
  border-radius: 12px;
  padding: 24px;
  margin-bottom: 24px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.08);
  border: 1px solid #f0f2f5;
}

.filter-form {
  display: flex;
  flex-wrap: wrap;
  gap: 20px;
  align-items: end;
}

.filter-form .el-form-item {
  margin-bottom: 0;
}

.filter-form .el-form-item__label {
  font-weight: 500;
  color: #303133;
  font-size: 14px;
}

.filter-form .el-input,
.filter-form .el-select {
  width: 200px;
  border-radius: 8px;
  border: 1px solid #dcdfe6;
  transition: all 0.3s ease;
}

/* 状态下拉框文字居中 */
.filter-form .el-select .el-select__input {
  text-align: center;
}

.filter-form .el-select .el-select__placeholder {
  text-align: center;
}

.filter-form .el-input:hover,
.filter-form .el-select:hover {
  border-color: #c0c4cc;
}

.filter-form .el-input__wrapper.is-focus,
.filter-form .el-select .el-input.is-focus .el-input__wrapper {
  box-shadow: 0 0 0 2px rgba(139, 173, 66, 0.2);
  border-color: #8bad42;
}

.filter-form .el-button--primary {
  background: linear-gradient(135deg, #8bad42 0%, #7da03a 100%);
  border-color: #8bad42;
  border-radius: 8px;
  padding: 10px 24px;
  font-weight: 500;
  transition: all 0.3s ease;
  box-shadow: 0 2px 8px rgba(139, 173, 66, 0.2);
  color: black !important;
}

.filter-form .el-button--primary:hover {
  background: linear-gradient(135deg, #7da03a 0%, #6b8e23 100%);
  border-color: #7da03a;
  transform: translateY(-1px);
  box-shadow: 0 4px 12px rgba(139, 173, 66, 0.3);
  color: black !important;
}

.filter-form .el-button {
  border-radius: 8px;
  padding: 10px 24px;
  transition: all 0.3s ease;
  border: 1px solid #dcdfe6;
  color: #606266;
}

.filter-form .el-button:hover {
  border-color: #8bad42;
  color: #8bad42;
}

/* 雏鸟列表容器 */
.birds-content {
  background: white;
  border-radius: 12px;
  padding: 24px;
  min-height: 500px;
  box-shadow: 0 2px 12px 0 rgba(0, 0, 0, 0.08);
  border: 1px solid #f0f2f5;
}

/* 表格样式 */
.birds-content .el-table {
  border: none;
  border-radius: 8px;
  overflow: hidden;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
}

.birds-content .el-table th {
  background: #f8f9fa;
  font-weight: 600;
  color: #303133;
  padding: 16px;
  border-bottom: 2px solid #e4e7ed;
  font-size: 14px;
}

.birds-content .el-table td {
  padding: 16px;
  border-bottom: 1px solid #f0f2f5;
  font-size: 14px;
  color: #606266;
}

.birds-content .el-table tr:hover {
  background: #f5f7fa;
}

.birds-content .el-table tr.el-table__row--striped {
  background: #fafafa;
}

/* 状态标签样式 */
.birds-content .el-tag {
  padding: 4px 12px;
  border-radius: 16px;
  font-size: 12px;
  font-weight: 500;
}

/* 操作按钮 */
.birds-content .el-button {
  border-radius: 6px;
  padding: 6px 12px;
  font-size: 12px;
  transition: all 0.3s ease;
  margin-right: 8px;
}

.birds-content .el-button--small {
  padding: 5px 10px;
  font-size: 12px;
}

.birds-content .el-button:first-child {
  background: #f0f9eb;
  border-color: #e1f5c4;
  color: #8bad42;
}

.birds-content .el-button:first-child:hover {
  background: #e1f5c4;
  border-color: #c2e780;
  color: #7da03a;
}

.birds-content .el-button--danger {
  background: #fef0f0;
  border-color: #fde2e2;
  color: #f56c6c;
}

.birds-content .el-button--danger:hover {
  background: #fde2e2;
  border-color: #fbc4c4;
  color: #e64242;
}

/* 分页区域 */
.pagination {
  margin-top: 24px;
  display: flex;
  justify-content: flex-end;
  align-items: center;
  gap: 16px;
}

.pagination .el-pagination {
  font-size: 14px;
}

.pagination .el-pagination__item {
  border-radius: 6px;
  transition: all 0.3s ease;
  min-width: 32px;
  height: 32px;
  line-height: 32px;
  border: 1px solid #dcdfe6;
}

.pagination .el-pagination__item:hover {
  border-color: #8bad42;
  color: #8bad42;
}

.pagination .el-pagination__item.is-active {
  background: linear-gradient(135deg, #8bad42 0%, #7da03a 100%);
  border-color: #8bad42;
  color: white;
}

.pagination .el-pagination__size-changer {
  border-radius: 6px;
}

.pagination .el-select .el-input.is-focus .el-input__wrapper {
  box-shadow: 0 0 0 2px rgba(139, 173, 66, 0.2);
  border-color: #8bad42;
}

/* 空状态 */
.empty-content {
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-items: center;
  min-height: 400px;
  padding: 60px 40px;
  text-align: center;
  background: #fafafa;
  border-radius: 8px;
  margin: 20px 0;
}

.empty-content .el-empty {
  width: 100%;
}

.empty-content .el-empty__description {
  font-size: 16px;
  color: #909399;
  margin-top: 20px;
}

/* 对话框样式 */
.el-dialog {
  border-radius: 12px;
  overflow: hidden;
  box-shadow: 0 10px 40px rgba(0, 0, 0, 0.15);
}

.el-dialog__header {
  background: linear-gradient(135deg, #8bad42 0%, #7da03a 100%);
  color: white;
  padding: 20px 24px;
  margin: 0;
}

.el-dialog__title {
  color: white;
  font-size: 18px;
  font-weight: 600;
}

.el-dialog__body {
  padding: 24px;
  background: white;
}

.el-dialog__footer {
  padding: 16px 24px;
  background: #f5f7fa;
  border-top: 1px solid #e0e6ed;
}

.dialog-footer {
  display: flex;
  justify-content: flex-end;
  gap: 12px;
}

.dialog-footer .el-button {
  border-radius: 8px;
  padding: 8px 20px;
  font-size: 14px;
  transition: all 0.3s ease;
}

.dialog-footer .el-button--primary {
  background: linear-gradient(135deg, #8bad42 0%, #7da03a 100%);
  border-color: #8bad42;
  color: white;
}

.dialog-footer .el-button--primary:hover {
  background: linear-gradient(135deg, #7da03a 0%, #6b8e23 100%);
  border-color: #7da03a;
}

/* 表单样式 */
.el-form-item {
  margin-bottom: 20px;
}

.el-form-item__label {
  font-weight: 500;
  color: #303133;
  font-size: 14px;
}

.el-input,
.el-select,
.el-date-picker {
  border-radius: 8px;
  border: 1px solid #dcdfe6;
  transition: all 0.3s ease;
}

.el-input:hover,
.el-select:hover,
.el-date-picker:hover {
  border-color: #c0c4cc;
}

.el-input__wrapper.is-focus,
.el-select .el-input.is-focus .el-input__wrapper {
  box-shadow: 0 0 0 2px rgba(139, 173, 66, 0.2);
  border-color: #8bad42;
}

/* 响应式设计 */
@media (max-width: 1200px) {
  .birds-page {
    width: 95%;
    padding: 16px;
  }
  
  .filter-form .el-input,
  .filter-form .el-select {
    width: 180px;
  }
}

@media (max-width: 768px) {
  .birds-page {
    width: 100%;
    margin: 0;
    padding: 12px;
    box-sizing: border-box;
    background: transparent;
    backdrop-filter: none;
    -webkit-backdrop-filter: none;
    border-radius: 0;
    box-shadow: none;
    border: none;
  }

  .birds-header {
    flex-direction: column;
    align-items: flex-start;
    gap: 12px;
    padding: 0;
    margin-bottom: 16px;
  }

  .birds-header h1 {
    font-size: 20px;
    font-weight: 600;
  }

  .birds-header .el-button--primary {
    font-size: 14px;
    padding: 8px 16px;
  }

  .filter-form {
    flex-direction: column;
    align-items: flex-start;
    gap: 12px;
  }

  .filter-form .el-form-item {
    width: 100%;
    margin-bottom: 0;
  }

  .filter-form .el-form-item__label {
    font-size: 14px;
    padding-right: 8px;
  }

  .filter-form .el-input,
  .filter-form .el-select {
    width: 100% !important;
    font-size: 14px;
  }

  .filter-form .el-button {
    font-size: 14px;
    padding: 8px 16px;
  }

  /* 隐藏桌面端内容 */
  .desktop-content {
    display: none;
  }

  /* 移动端卡片布局 */
  .mobile-content {
    margin-top: 16px;
  }

  .birds-grid {
    display: grid;
    grid-template-columns: 1fr;
    gap: 16px;
  }

  .bird-card {
    background: white;
    border-radius: 12px;
    padding: 16px;
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
    transition: all 0.3s ease;
  }

  .bird-card:hover {
    transform: translateY(-2px);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.15);
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 12px;
  }

  .bird-name {
    font-size: 18px;
    font-weight: 600;
    color: #303133;
  }

  .card-body {
    margin-bottom: 16px;
  }

  .bird-info {
    display: flex;
    margin-bottom: 8px;
  }

  .info-label {
    width: 80px;
    font-size: 14px;
    color: #606266;
  }

  .info-value {
    flex: 1;
    font-size: 14px;
    color: #303133;
  }

  .card-footer {
    display: flex;
    gap: 12px;
  }

  .card-footer .el-button {
    flex: 1;
    font-size: 14px;
    padding: 8px 12px;
  }

  .mobile-pagination {
    margin-top: 24px;
    display: flex;
    justify-content: center;
  }

  .mobile-pagination .el-pagination {
    font-size: 12px;
  }

  .mobile-pagination .el-pagination__item {
    font-size: 12px;
    min-width: 28px;
    height: 28px;
    line-height: 28px;
  }

  .mobile-empty {
    display: flex;
    flex-direction: column;
    justify-content: center;
    align-items: center;
    min-height: 300px;
    text-align: center;
    padding: 40px 20px;
  }

  .empty-icon {
    margin-bottom: 16px;
    color: #c0c4cc;
  }

  .empty-text {
    font-size: 18px;
    font-weight: 600;
    color: #303133;
    margin-bottom: 8px;
  }

  .empty-desc {
    font-size: 14px;
    color: #909399;
  }

  .el-dialog {
    width: 95% !important;
    margin: 10px auto !important;
    max-height: 90vh;
    overflow-y: auto;
  }

  .el-dialog__body {
    padding: 16px;
  }

  .el-form-item {
    margin-bottom: 16px;
  }

  .el-form-item__label {
    font-size: 14px;
  }

  .el-input,
  .el-select,
  .el-date-picker {
    font-size: 14px;
  }

  .dialog-footer {
    gap: 8px;
  }

  .dialog-footer .el-button {
    font-size: 14px;
    padding: 6px 16px;
  }
}

/* 更小屏幕的适配 */
@media (max-width: 375px) {
  .birds-page {
    padding: 8px;
    background: transparent;
    backdrop-filter: none;
    -webkit-backdrop-filter: none;
    border-radius: 0;
    box-shadow: none;
    border: none;
  }

  .birds-header h1 {
    font-size: 18px;
  }

  .birds-grid {
    gap: 12px;
  }

  .bird-card {
    padding: 12px;
  }

  .bird-name {
    font-size: 16px;
  }

  .info-label {
    width: 70px;
    font-size: 13px;
  }

  .info-value {
    font-size: 13px;
  }

  .card-footer .el-button {
    font-size: 13px;
    padding: 6px 10px;
  }
}

/* 桌面端显示 */
@media (min-width: 769px) {
  .mobile-content,
  .mobile-empty {
    display: none;
  }
}
.no-cage {
  color: #c0c4cc;
  font-style: italic;
}
</style>