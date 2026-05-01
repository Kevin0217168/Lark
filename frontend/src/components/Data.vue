<template>
  <!-- 桌面端界面 -->
  <div v-if="!isMobile" class="data-card">
    <div class="title">
      <h2>数据管理</h2>
    </div>
    
    <div class="data-content">
      <!-- 实时数据 -->
      <div v-if="activeTab === 'realtime'" class="realtime-container">
        <!-- 所有设备离线提示 -->
        <el-alert
          v-if="showAllOfflineAlert"
          title="所有设备均处于离线状态"
          type="warning"
          :closable="false"
          show-icon
          style="margin-bottom: 16px;"
        />
        
        <!-- 左侧数据区域 -->
        <div class="data-left">
          <!-- 设备选择 -->
          <div class="device-selector">
            <h3>鸟笼选择</h3>
            <el-select v-model="selectedBirdcageKey" placeholder="选择鸟笼" @change="handleBirdcageChange" style="width: 100%;">
              <el-option
                v-for="group in birdcageGroups"
                :key="`${group.area}-${group.number}`"
                :label="group.label"
                :value="`${group.area}|${group.number}`"
              />
            </el-select>
          </div>
          
          <!-- 鸟笼设备信息 -->
          <div class="device-data" v-if="selectedDevice">
            <h3>鸟笼设备</h3>
            <div class="birdcage-device-list">
              <!-- CAM 设备行 -->
              <div class="device-info-row">
                <span class="device-type-tag cam-tag">CAM</span>
                <span class="device-info-item">ID: {{ selectedDevice.id }}</span>
                <span class="device-info-sep">|</span>
                <span class="device-info-item name">{{ selectedDevice.name }}</span>
                <span class="device-info-sep">|</span>
                <span class="device-info-item type">ESP32-CAM</span>
                <el-tag :type="selectedDevice.isOnline ? 'success' : 'danger'" size="small">
                  {{ selectedDevice.isOnline ? '在线' : '离线' }}
                </el-tag>
              </div>
              <!-- C3 设备行 -->
              <div class="device-info-row" v-if="selectedC3Device">
                <span class="device-type-tag c3-tag">C3</span>
                <span class="device-info-item">ID: {{ selectedC3Device.id }}</span>
                <span class="device-info-sep">|</span>
                <span class="device-info-item name">{{ selectedC3Device.name }}</span>
                <span class="device-info-sep">|</span>
                <span class="device-info-item type">ESP32-C3</span>
                <el-tag :type="selectedC3Device.isOnline ? 'success' : 'danger'" size="small">
                  {{ selectedC3Device.isOnline ? '在线' : '离线' }}
                </el-tag>
              </div>
            </div>
            
            <!-- 温度湿度数据卡片 -->
            <div class="temp-humid-cards">
              <div class="th-card temp-card">
                <div class="th-card-icon">
                  <TemperatureIcon />
                </div>
                <div class="th-card-body">
                  <span class="th-card-label">温度</span>
                  <span class="th-card-value">{{ selectedDevice.temperature ?? '--' }}<small>°C</small></span>
                </div>
              </div>
              <div class="th-card humid-card">
                <div class="th-card-icon">
                  <HumidityIcon />
                </div>
                <div class="th-card-body">
                  <span class="th-card-label">湿度</span>
                  <span class="th-card-value">{{ selectedDevice.humidity ?? '--' }}<small>%</small></span>
                </div>
              </div>
            </div>
          </div>
          
          <!-- 环境数据环形进度条 -->
          <div class="env-gauges" v-if="selectedC3Device">
            <h3>环境数据 (ESP32-C3)</h3>
            <div class="gauges-row">
              <div class="gauge-item">
                <div ref="aqiGaugeRef" class="gauge-chart"></div>
                <div class="gauge-label">空气质量</div>
              </div>
              <div class="gauge-item">
                <div ref="dbGaugeRef" class="gauge-chart"></div>
                <div class="gauge-label">声音分贝</div>
              </div>
            </div>
            <div class="gauges-row">
              <div class="gauge-item">
                <div ref="luxGaugeRef" class="gauge-chart"></div>
                <div class="gauge-label">光照强度</div>
              </div>
              <div class="gauge-item">
                <div ref="uvGaugeRef" class="gauge-chart"></div>
                <div class="gauge-label">紫外线指数</div>
              </div>
            </div>
          </div>
        </div>
        
        <!-- 右侧视频流区域 -->
        <div class="data-right">
          <div class="video-header">
            <h3>实时监控 (ESP32-CAM)</h3>
            <div class="video-controls">
              <el-button 
                type="warning"
                size="small"
                @click="reconnectWebSocket"
                :disabled="!camDeviceId || isWebSocketConnected"
              >
                <el-icon><Refresh /></el-icon>
                手动重连
              </el-button>
              <el-button 
                :type="flipHorizontal ? 'primary' : 'default'"
                size="small"
                @click="toggleHorizontalFlip"
              >
                <el-icon><Switch /></el-icon>
                左右翻转
              </el-button>
              <el-button 
                :type="flipVertical ? 'primary' : 'default'"
                size="small"
                @click="toggleVerticalFlip"
              >
                <el-icon><Sort /></el-icon>
                上下翻转
              </el-button>
              <div class="quality-control">
                <span class="quality-label">视频质量</span>
                <el-slider v-model="imageQuality" :min="1" :max="63" :step="1" :show-tooltip="false" style="width: 120px" />
              </div>
              <div class="quality-control">
                <span class="quality-label">视频尺寸</span>
                <el-select v-model="frameSize" style="width: 120px" size="small">
                  <el-option label="128x128" value="FRAMESIZE_128X128" />
                  <el-option label="240x240" value="FRAMESIZE_240X240" />
                  <el-option label="320x320" value="FRAMESIZE_320X320" />
                  <el-option label="VGA" value="FRAMESIZE_VGA" />
                  <el-option label="SVGA" value="FRAMESIZE_SVGA" />
                  <el-option label="HD" value="FRAMESIZE_HD" />
                  <el-option label="FHD" value="FRAMESIZE_FHD" />
                </el-select>
              </div>
            </div>
          </div>
          <div class="video-card">
            <div class="video-container" v-if="selectedDevice && currentFrameImage">
              <img 
                :src="currentFrameImage" 
                class="video-stream"
                :class="{ 'flip-horizontal': flipHorizontal, 'flip-vertical': flipVertical, 'disconnected': isStreamDisconnected }"
                alt="实时监控画面"
              />
              <div class="frame-info">
                <span>更新时间: {{ lastFrameTime }}</span>
                <span class="fps-info">帧率: {{ currentFps }} FPS</span>
              </div>
              <div class="stream-disconnected-overlay" v-if="isStreamDisconnected">
                <div class="disconnected-content">
                  <el-icon class="disconnected-icon" :size="48"><Warning /></el-icon>
                  <div class="disconnected-message">视频流已断联，请重连</div>
                </div>
              </div>
            </div>
            <div class="video-error" v-else-if="selectedDevice && connectionError">
              <div class="error-content">
                <el-icon class="error-icon" :size="48"><Warning /></el-icon>
                <div class="error-message">{{ connectionError }}</div>
              </div>
            </div>
            <div class="video-placeholder" v-else-if="selectedDevice">
              <el-empty description="未开始实时监控" />
            </div>
            <div class="video-placeholder" v-else>
              <el-empty description="请选择鸟笼" />
            </div>
          </div>
        </div>
      </div>
      
      <!-- 分析 -->
      <div v-else-if="activeTab === 'analysis'" class="tab-content">
        <div class="card-header">
          <div class="card-title">
            <span>数据分析</span>
          </div>
          <div class="card-actions">
            <el-select v-model="analysisBirdcageKey" @change="handleAnalysisBirdcageChange" class="device-select" placeholder="选择鸟笼">
              <el-option
                v-for="group in birdcageGroups"
                :key="`${group.area}-${group.number}`"
                :label="group.label"
                :value="`${group.area}|${group.number}`"
              />
            </el-select>
          </div>
        </div>

        <!-- 时间范围选择器 -->
        <div class="time-range-selector">
          <div class="time-buttons-container">
            <button
              v-for="label in timeRangeLabels"
              :key="label.value"
              class="time-button"
              :class="{ active: timeRange === label.value }"
              @click="setTimeRange(label.value)"
            >
              {{ label.text }}
            </button>
          </div>
        </div>

        <div class="analysis-content">
          <!-- 温度数据 -->
          <div class="chart-container">
            <h3>温度数据 <span class="time-range">({{ timeRangeText }})</span></h3>
            <div ref="temperatureChartRef" class="chart"></div>
          </div>
          <!-- 湿度数据 -->
          <div class="chart-container">
            <h3>湿度数据 <span class="time-range">({{ timeRangeText }})</span></h3>
            <div ref="humidityChartRef" class="chart"></div>
          </div>
          <!-- 空气质量数据 -->
          <div class="chart-container">
            <h3>空气质量 (AQI) <span class="time-range">({{ timeRangeText }})</span></h3>
            <div ref="analysisAqiChartRef" class="chart"></div>
          </div>
          <!-- 声音分贝数据 -->
          <div class="chart-container">
            <h3>声音分贝 <span class="time-range">({{ timeRangeText }})</span></h3>
            <div ref="analysisSoundChartRef" class="chart"></div>
          </div>
          <!-- 光照强度数据 -->
          <div class="chart-container">
            <h3>光照强度 <span class="time-range">({{ timeRangeText }})</span></h3>
            <div ref="analysisLuxChartRef" class="chart"></div>
          </div>
          <!-- 紫外线指数数据 -->
          <div class="chart-container">
            <h3>紫外线指数 <span class="time-range">({{ timeRangeText }})</span></h3>
            <div ref="analysisUvChartRef" class="chart"></div>
          </div>
        </div>
      </div>
      
      <!-- 历史数据 -->
      <div v-else-if="activeTab === 'history'" class="tab-content">
        <div class="card-header">
          <div class="card-title">
            <span>历史数据</span>
          </div>
        </div>
        <div class="history-content" v-if="historyBirdcageKey">
          <!-- 鸟笼选择器 -->
          <div class="top-section">
            <div class="device-selector">
              <h3>鸟笼选择</h3>
              <el-select v-model="historyBirdcageKey" @change="handleHistoryBirdcageChange" class="device-select" placeholder="请选择鸟笼">
                <el-option
                  v-for="group in birdcageGroups"
                  :key="`${group.area}-${group.number}`"
                  :label="group.label"
                  :value="`${group.area}|${group.number}`"
                />
              </el-select>
            </div>
            <!-- 鸟笼信息 -->
            <div class="device-info">
              <h3>鸟笼信息</h3>
              <div class="info-card">
                <div class="info-item">
                  <span class="info-label">ESP32-CAM:</span>
                  <span class="info-value" :class="historyCamDevice?.isOnline ? 'online' : 'offline'">
                    {{ historyCamDevice?.name || '未配置' }}
                    <el-tag v-if="historyCamDevice" :type="historyCamDevice.isOnline ? 'success' : 'danger'" size="small" class="status-tag-inline">
                      {{ historyCamDevice.isOnline ? '在线' : '离线' }}
                    </el-tag>
                  </span>
                </div>
                <div class="info-item">
                  <span class="info-label">ESP32-C3:</span>
                  <span class="info-value" :class="historyC3Device?.isOnline ? 'online' : 'offline'">
                    {{ historyC3Device?.name || '未配置' }}
                    <el-tag v-if="historyC3Device" :type="historyC3Device.isOnline ? 'success' : 'danger'" size="small" class="status-tag-inline">
                      {{ historyC3Device.isOnline ? '在线' : '离线' }}
                    </el-tag>
                  </span>
                </div>
              </div>
            </div>
          </div>

          <!-- 合并历史数据表格 -->
          <div class="history-table">
            <h3>历史数据记录（温度 / 湿度 / 空气质量 / 声音 / 光照 / 紫外线）</h3>
            <div v-if="historyC3Loading" class="c3-loading">
              <span>加载中...</span>
            </div>
            <el-table v-else :data="paginatedHistoryData" stripe style="width: 100%;" max-height="400" border>
              <el-table-column prop="timestamp" label="时间" min-width="160" align="left" />
              <el-table-column prop="temperature" label="温度 (℃)" min-width="100" align="center">
                <template #default="{ row }">
                  {{ row.temperature !== null ? row.temperature.toFixed(1) : '-' }}
                </template>
              </el-table-column>
              <el-table-column prop="humidity" label="湿度 (%)" min-width="100" align="center">
                <template #default="{ row }">
                  {{ row.humidity !== null ? row.humidity.toFixed(1) : '-' }}
                </template>
              </el-table-column>
              <el-table-column prop="pm25" label="PM2.5 (μg/m³)" min-width="120" align="center">
                <template #default="{ row }">
                  {{ row.pm25 !== null ? row.pm25.toFixed(1) : '-' }}
                </template>
              </el-table-column>
              <el-table-column prop="db" label="分贝 (dB)" min-width="100" align="center">
                <template #default="{ row }">
                  {{ row.db !== null ? row.db.toFixed(1) : '-' }}
                </template>
              </el-table-column>
              <el-table-column prop="lux" label="光照 (Lux)" min-width="100" align="center">
                <template #default="{ row }">
                  {{ row.lux !== null ? row.lux.toFixed(1) : '-' }}
                </template>
              </el-table-column>
              <el-table-column prop="uv" label="UV指数" min-width="90" align="center">
                <template #default="{ row }">
                  {{ row.uv !== null ? row.uv.toFixed(1) : '-' }}
                </template>
              </el-table-column>
            </el-table>
            <div class="pagination-container">
              <el-pagination
                v-model:current-page="currentPage"
                v-model:page-size="pageSize"
                :page-sizes="[10, 20, 50, 100]"
                :total="mergedHistoryData.length"
                layout="total, sizes, prev, pager, next, jumper"
                background
                :prev-text="'上一页'"
                :next-text="'下一页'"
                :total-text="'共 {total} 条'"
                :page-size-text="'条/页'"
                :jumper-text="'前往'"
                :jumper-prepend-text="''"
                :jumper-append-text="'页'"
                @size-change="handleSizeChange"
                @current-change="handleCurrentChange"
              />
            </div>
          </div>
        </div>
        <div class="history-placeholder" v-else>
          <el-empty description="请选择鸟笼查看历史数据" />
        </div>
      </div>
    </div>
  </div>

  <!-- 移动端界面 -->
  <div v-else class="mobile-data">
    <!-- 实时数据 -->
    <div v-if="activeTab === 'realtime'" class="realtime-container">
      <!-- 设备选择 -->
      <div class="device-selector">
        <el-select v-model="selectedBirdcageKey" placeholder="选择鸟笼" @change="handleBirdcageChange" style="width: 100%;">
          <el-option
            v-for="group in birdcageGroups"
            :key="`${group.area}-${group.number}`"
            :label="group.label"
            :value="`${group.area}|${group.number}`"
          />
        </el-select>
      </div>
      
      <!-- 所有设备离线提示 -->
      <el-alert
        v-if="showAllOfflineAlert"
        title="所有设备均处于离线状态"
        type="warning"
        :closable="false"
        show-icon
        style="margin-bottom: 16px;"
      />
      
      <!-- 鸟笼设备信息卡片 -->
      <div v-if="selectedDevice || selectedC3Device" class="device-data-card">
        <div class="data-header">
          <h3>鸟笼设备</h3>
        </div>
        <!-- CAM 设备行 -->
        <div class="mobile-device-row" v-if="selectedDevice">
          <span class="device-type-tag cam-tag">CAM</span>
          <span class="mobile-device-info">ID: {{ selectedDevice.id }} | {{ selectedDevice.name }} | ESP32-CAM</span>
          <el-tag :type="selectedDevice.isOnline ? 'success' : 'danger'" size="small">
            {{ selectedDevice.isOnline ? '在线' : '离线' }}
          </el-tag>
        </div>
        <!-- C3 设备行 -->
        <div class="mobile-device-row" v-if="selectedC3Device">
          <span class="device-type-tag c3-tag">C3</span>
          <span class="mobile-device-info">ID: {{ selectedC3Device.id }} | {{ selectedC3Device.name }} | ESP32-C3</span>
          <el-tag :type="selectedC3Device.isOnline ? 'success' : 'danger'" size="small">
            {{ selectedC3Device.isOnline ? '在线' : '离线' }}
          </el-tag>
        </div>
        <!-- 温度湿度 -->
        <div class="mobile-th-row" v-if="selectedDevice">
          <div class="mobile-th-item temp">
            <span class="mobile-th-icon">
              <TemperatureIcon />
            </span>
            <div class="mobile-th-body">
              <span class="mobile-th-label">温度</span>
              <span class="mobile-th-value">{{ selectedDevice.temperature ?? '--' }}°C</span>
            </div>
          </div>
          <div class="mobile-th-item humid">
            <span class="mobile-th-icon">
              <HumidityIcon />
            </span>
            <div class="mobile-th-body">
              <span class="mobile-th-label">湿度</span>
              <span class="mobile-th-value">{{ selectedDevice.humidity ?? '--' }}%</span>
            </div>
          </div>
        </div>
      </div>
      
      <!-- 环境数据环形进度条（移动端） -->
      <div v-if="selectedC3Device" class="mobile-gauges">
        <div class="mobile-gauges-row">
          <div class="mobile-gauge-item">
             <div ref="aqiGaugeRefMobile" class="mobile-gauge-chart"></div>
             <div class="mobile-gauge-label">空气质量</div>
           </div>
           <div class="mobile-gauge-item">
             <div ref="dbGaugeRefMobile" class="mobile-gauge-chart"></div>
             <div class="mobile-gauge-label">声音分贝</div>
           </div>
        </div>
        <div class="mobile-gauges-row mobile-gauges-row-second">
          <div class="mobile-gauge-item">
             <div ref="luxGaugeRefMobile" class="mobile-gauge-chart"></div>
             <div class="mobile-gauge-label">光照强度</div>
           </div>
           <div class="mobile-gauge-item">
             <div ref="uvGaugeRefMobile" class="mobile-gauge-chart"></div>
             <div class="mobile-gauge-label">紫外线指数</div>
           </div>
        </div>
      </div>
      
      <!-- 实时监控 -->
      <div v-if="selectedDevice" class="video-section">
        <div class="video-header">
          <div class="video-header-top">
            <h3>实时监控 (ESP32-CAM)</h3>
            <div class="video-controls">
              <el-button 
                type="warning"
                size="small"
                @click="reconnectWebSocket"
                :disabled="!camDeviceId || isWebSocketConnected"
              >
                <el-icon><Refresh /></el-icon>
                重连
              </el-button>
              <el-button 
                :type="flipHorizontal ? 'primary' : 'default'"
                size="small"
                @click="toggleHorizontalFlip"
              >
                <el-icon><Switch /></el-icon>
                左右
              </el-button>
              <el-button 
                :type="flipVertical ? 'primary' : 'default'"
                size="small"
                @click="toggleVerticalFlip"
              >
                <el-icon><Sort /></el-icon>
                上下
              </el-button>
            </div>
          </div>
          
          <!-- 视频质量和尺寸控制 -->
          <div class="video-settings">
            <div class="setting-item">
              <span class="setting-label">画质</span>
              <el-slider 
                v-model="imageQuality" 
                :min="1" 
                :max="63" 
                :step="1"
                :show-tooltip="false"
                size="small"
                style="flex: 1; margin-left: 8px;"
              />
            </div>
            <div class="setting-item">
              <span class="setting-label">视频尺寸</span>
              <el-select v-model="frameSize" style="flex: 1; margin-left: 8px;" size="small">
                <el-option label="128x128" value="FRAMESIZE_128X128" />
                <el-option label="240x240" value="FRAMESIZE_240X240" />
                <el-option label="320x320" value="FRAMESIZE_320X320" />
                <el-option label="VGA" value="FRAMESIZE_VGA" />
                <el-option label="SVGA" value="FRAMESIZE_SVGA" />
                <el-option label="HD" value="FRAMESIZE_HD" />
                <el-option label="FHD" value="FRAMESIZE_FHD" />
              </el-select>
            </div>
          </div>
        </div>
        <div class="video-card">
          <div class="video-container" v-if="currentFrameImage">
            <img 
              :src="currentFrameImage" 
              class="video-stream"
              :class="{ 'flip-horizontal': flipHorizontal, 'flip-vertical': flipVertical, 'disconnected': isStreamDisconnected }"
              alt="实时监控画面"
            />
            <div class="frame-info">
                <span>更新时间: {{ lastFrameTime }}</span>
                <span class="fps-info">帧率: {{ currentFps }} FPS</span>
              </div>
            <div class="stream-disconnected-overlay" v-if="isStreamDisconnected">
              <div class="disconnected-content">
                <el-icon class="disconnected-icon" :size="48"><Warning /></el-icon>
                <div class="disconnected-message">视频流已断联</div>
              </div>
            </div>
          </div>
          <div class="video-error" v-else-if="connectionError">
            <div class="error-content">
              <el-icon class="error-icon" :size="48"><Warning /></el-icon>
              <div class="error-message">{{ connectionError }}</div>
            </div>
          </div>
          <div class="video-placeholder" v-else>
            <el-empty description="未开始实时监控" />
          </div>
        </div>
      </div>
    </div>
    
    <!-- 分析 -->
    <div v-else-if="activeTab === 'analysis'" class="analysis-container">
      <div class="device-selector">
        <el-select v-model="analysisBirdcageKey" placeholder="选择鸟笼" @change="handleAnalysisBirdcageChange" style="width: 100%;">
          <el-option
            v-for="group in birdcageGroups"
            :key="`${group.area}-${group.number}`"
            :label="group.label"
            :value="`${group.area}|${group.number}`"
          />
        </el-select>
      </div>

      <!-- 时间轴筛选器 -->
      <div class="time-range-selector mobile">
        <div class="time-slider-container">
          <div class="time-labels">
            <span
              v-for="label in timeRangeLabels"
              :key="label.value"
              class="time-label"
              :class="{ active: timeRange === label.value }"
              @click="setTimeRange(label.value)"
            >
              {{ label.text }}
            </span>
          </div>
          <div class="slider-track">
            <div class="slider-progress" :style="{ width: getTimeRangeProgress() + '%' }"></div>
            <div class="slider-thumb" :style="{ left: getTimeRangeProgress() + '%' }"></div>
          </div>
        </div>
      </div>

      <!-- 图表 -->
      <div class="chart-section">
        <div class="chart-card">
          <h3>温度数据 <span class="time-range">({{ timeRangeText }})</span></h3>
          <div ref="temperatureChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>湿度数据 <span class="time-range">({{ timeRangeText }})</span></h3>
          <div ref="humidityChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>空气质量 (AQI) <span class="time-range">({{ timeRangeText }})</span></h3>
          <div ref="analysisAqiChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>声音分贝 <span class="time-range">({{ timeRangeText }})</span></h3>
          <div ref="analysisSoundChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>光照强度 <span class="time-range">({{ timeRangeText }})</span></h3>
          <div ref="analysisLuxChartRef" class="chart"></div>
        </div>
        <div class="chart-card">
          <h3>紫外线指数 <span class="time-range">({{ timeRangeText }})</span></h3>
          <div ref="analysisUvChartRef" class="chart"></div>
        </div>
      </div>
    </div>
    
    <!-- 历史数据 -->
    <div v-else-if="activeTab === 'history'" class="history-container">
      <div class="device-selector">
        <el-select v-model="historyBirdcageKey" placeholder="选择鸟笼" @change="handleHistoryBirdcageChange" style="width: 100%;">
          <el-option
            v-for="group in birdcageGroups"
            :key="`${group.area}-${group.number}`"
            :label="group.label"
            :value="`${group.area}|${group.number}`"
          />
        </el-select>
      </div>

      <div v-if="historyBirdcageKey" class="history-content">
        <!-- 鸟笼信息卡片 -->
        <div class="birdcage-info-card">
          <div class="birdcage-header">
            <span class="birdcage-icon">🏠</span>
            <span class="birdcage-name">{{ historyBirdcageGroup?.label || '' }}</span>
          </div>
          <div class="birdcage-devices">
            <div class="device-chip" :class="historyCamDevice?.isOnline ? 'online' : 'offline'">
              <span class="chip-dot"></span>
              <span class="chip-label">CAM</span>
              <span class="chip-name">{{ historyCamDevice?.name || '未配置' }}</span>
            </div>
            <div class="device-chip" :class="historyC3Device?.isOnline ? 'online' : 'offline'">
              <span class="chip-dot"></span>
              <span class="chip-label">C3</span>
              <span class="chip-name">{{ historyC3Device?.name || '未配置' }}</span>
            </div>
          </div>
        </div>

        <!-- 合并历史数据列表 -->
        <div class="history-data-section">
          <div v-if="historyC3Loading" class="history-loading">
            <div class="loading-spinner"></div>
            <span>加载传感器数据中...</span>
          </div>
          <div v-else-if="paginatedHistoryData.length === 0" class="history-empty">
            <span class="empty-icon">📊</span>
            <span class="empty-text">暂无历史数据</span>
          </div>
          <div v-else class="history-cards">
            <div v-for="(item, index) in paginatedHistoryData" :key="'merged-' + index" class="history-data-card">
              <div class="card-time">
                <span class="time-icon">🕐</span>
                <span class="time-text">{{ item.timestamp }}</span>
              </div>
              <div class="sensor-grid">
                <div class="sensor-cell temp">
                  <span class="sensor-icon"><TemperatureIcon /></span>
                  <span class="sensor-value">{{ item.temperature !== null ? item.temperature.toFixed(1) : '-' }}</span>
                  <span class="sensor-unit">°C</span>
                </div>
                <div class="sensor-cell humidity">
                  <span class="sensor-icon"><HumidityIcon /></span>
                  <span class="sensor-value">{{ item.humidity !== null ? item.humidity.toFixed(1) : '-' }}</span>
                  <span class="sensor-unit">%</span>
                </div>
                <div class="sensor-cell aqi">
                  <span class="sensor-icon"><Pm25Icon /></span>
                  <span class="sensor-value">{{ item.pm25 !== null ? item.pm25.toFixed(1) : '-' }}</span>
                  <span class="sensor-unit">PM2.5</span>
                </div>
                <div class="sensor-cell sound">
                  <span class="sensor-icon"><SoundIcon /></span>
                  <span class="sensor-value">{{ item.db !== null ? item.db.toFixed(1) : '-' }}</span>
                  <span class="sensor-unit">dB</span>
                </div>
                <div class="sensor-cell lux">
                  <span class="sensor-icon"><LightIcon /></span>
                  <span class="sensor-value">{{ item.lux !== null ? item.lux.toFixed(1) : '-' }}</span>
                  <span class="sensor-unit">Lux</span>
                </div>
                <div class="sensor-cell uv">
                  <span class="sensor-icon"><UvIcon /></span>
                  <span class="sensor-value">{{ item.uv !== null ? item.uv.toFixed(1) : '-' }}</span>
                  <span class="sensor-unit">UV</span>
                </div>
              </div>
            </div>
          </div>
          <div class="pagination-container">
            <el-pagination
              v-model:current-page="currentPage"
              v-model:page-size="pageSize"
              :page-sizes="[5, 10, 20]"
              :total="mergedHistoryData.length"
              layout="prev, pager, next"
              background
              :pager-count="3"
              @size-change="handleSizeChange"
              @current-change="handleCurrentChange"
            />
          </div>
        </div>
      </div>

      <div v-else class="history-placeholder">
        <el-empty description="请选择鸟笼查看历史数据" />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted, nextTick } from 'vue';
import { useRoute, useRouter } from 'vue-router';
import { useDeviceStore } from '../stores/deviceStore';
import { ElMessage, ElMessageBox } from 'element-plus';
import { Warning, Switch, Sort, Refresh } from '@element-plus/icons-vue';
import { api } from '../utils/api';
import * as echarts from 'echarts';
import { shouldUseMobilePage } from '../utils/mobileAdapter';
import TemperatureIcon from './icons/TemperatureIcon.vue';
import HumidityIcon from './icons/HumidityIcon.vue';
import Pm25Icon from './icons/Pm25Icon.vue';
import SoundIcon from './icons/SoundIcon.vue';
import LightIcon from './icons/LightIcon.vue';
import UvIcon from './icons/UvIcon.vue';

// 获取登录token
const getToken = () => {
  return localStorage.getItem('accessToken');
};

const route = useRoute();
const router = useRouter();
const { getDevices, getDeviceHistoryData, getDeviceAverageData, fetchDevices, updateDevice, fetchDeviceHistoryData, getOrUpdateDeviceHistoryData, fetchSensorData, fetchRealtimeSensorData } = useDeviceStore();

// 直接获取设备数据
const devices = getDevices();

const birdcageGroups = computed(() => {
  const allDevices = getDevices() as any[];
  const groupMap = new Map<string, { area: string; number: number; label: string; devices: any[] }>();
  for (const d of allDevices) {
    const key = `${d.area || '未知'}-${d.number ?? '?'}`;
    if (!groupMap.has(key)) {
      groupMap.set(key, { area: d.area || '未知', number: d.number ?? 0, label: `${d.area || '未知'} #${d.number ?? '?'}`, devices: [] });
    }
    groupMap.get(key)!.devices.push(d);
  }
  return Array.from(groupMap.values()).sort((a, b) => a.label.localeCompare(b.label));
});

const birdcagePairedDevice = computed(() => {
  if (!selectedDeviceId.value) return null;
  const allDevices = getDevices() as any[];
  const current = allDevices.find((d: any) => d.id === selectedDeviceId.value);
  if (!current || !current.area) return null;
  return allDevices.find((d: any) =>
    d.id !== current.id &&
    d.area === current.area &&
    d.number === current.number
  ) || null;
});

// 接收父组件传递的activeTab
const props = withDefaults(defineProps<{
  activeTab?: string
}>(), {
  activeTab: 'realtime'
});

// 计算当前激活的标签页
const activeTab = computed(() => props.activeTab || 'realtime');

// 移动端检测
const isMobile = ref(false);
const checkMobileStatus = () => {
  isMobile.value = shouldUseMobilePage();
};

const selectedDeviceId = ref<number | null>(null);
const selectedBirdcageKey = ref<string>('');
const analysisBirdcageKey = ref<string>('');
const showAllOfflineAlert = ref(false);

const analysisCamDeviceId = computed(() => {
  if (!analysisBirdcageKey.value) return null;
  const allDevices = getDevices() as any[];
  const [area, numStr] = analysisBirdcageKey.value.split('|');
  const cam = allDevices.find((d: any) =>
    d.area === area && d.number === Number(numStr) && d.device_type === 'ESP32-CAM'
  );
  return cam?.id ?? null;
});

const analysisC3DeviceId = computed(() => {
  if (!analysisBirdcageKey.value) return null;
  const allDevices = getDevices() as any[];
  const [area, numStr] = analysisBirdcageKey.value.split('|');
  const c3 = allDevices.find((d: any) =>
    d.area === area && d.number === Number(numStr) && d.device_type === 'ESP32-C3'
  );
  return c3?.id ?? null;
});

const camDeviceId = computed(() => {
  if (!selectedBirdcageKey.value) return null;
  const allDevices = getDevices() as any[];
  const [area, numStr] = selectedBirdcageKey.value.split('|');
  const cam = allDevices.find((d: any) =>
    d.area === area && d.number === Number(numStr) && d.device_type === 'ESP32-CAM'
  );
  return cam?.id ?? null;
});

const c3DeviceId = computed(() => {
  if (!selectedBirdcageKey.value) return null;
  const allDevices = getDevices() as any[];
  const [area, numStr] = selectedBirdcageKey.value.split('|');
  const c3 = allDevices.find((d: any) =>
    d.area === area && d.number === Number(numStr) && d.device_type === 'ESP32-C3'
  );
  return c3?.id ?? null;
});

watch(camDeviceId, (newId) => {
  if (activeTab.value === 'realtime') {
    selectedDeviceId.value = newId;
  }
});

const selectedDevice = computed(() => {
  if (!selectedDeviceId.value) return null;
  const latestDevices = getDevices();
  const device = latestDevices.find((device: any) => device.id === selectedDeviceId.value) || null;
  return device;
});

const selectedC3Device = computed(() => {
  if (!c3DeviceId.value) return null;
  const latestDevices = getDevices() as any[];
  return latestDevices.find((d: any) => d.id === c3DeviceId.value) || null;
});

// 计算WebSocket是否已连接
const isWebSocketConnected = computed(() => {
  return ws !== null && ws.readyState === WebSocket.OPEN;
});

// 实时温湿度数据定时器
let realtimeDataTimer: number | null = null;

// 获取实时温湿度数据
const fetchRealtimeData = async () => {
  if (!selectedDeviceId.value) return;
  
  const sensorData = await fetchRealtimeSensorData(selectedDeviceId.value);
  if (sensorData) {
    updateDevice(selectedDeviceId.value, { 
      temperature: sensorData.temperature, 
      humidity: sensorData.humidity 
    });
  }
};

const fetchLuxData = async () => {
  if (!c3DeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: c3DeviceId.value,
      sensor_type: 'veml7700',
      limit: 1,
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const record = response.data[0];
      const parsed = JSON.parse(record.data);
      if (parsed.lux !== undefined) {
        pushHistory(luxHistory, Number(parsed.lux) || 0);
        updateLuxGauge();
      }
    }
  } catch (error) {
    console.error('获取光照数据失败:', error);
  }
};

const fetchUVData = async () => {
  if (!c3DeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: c3DeviceId.value,
      sensor_type: 'uv_meter',
      limit: 1,
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const record = response.data[0];
      const parsed = JSON.parse(record.data);
      if (parsed.uv_index !== undefined) {
        pushHistory(uvHistory, Number(parsed.uv_index) || 0);
        updateUVGauge();
      }
    }
  } catch (error) {
    console.error('获取紫外线数据失败:', error);
  }
};

// 启动实时数据定时器
const startRealtimeDataTimer = () => {
  // 先清除旧的定时器
  stopRealtimeDataTimer();
  
  // 立即获取一次数据
  fetchRealtimeData();
  
  // 每分钟获取一次数据
  realtimeDataTimer = window.setInterval(() => {
    fetchRealtimeData();
  }, 60000); // 60秒
};

// 停止实时数据定时器
const stopRealtimeDataTimer = () => {
  if (realtimeDataTimer) {
    clearInterval(realtimeDataTimer);
    realtimeDataTimer = null;
  }
};

// 获取在线设备列表（按ID升序排序）
const getOnlineDevices = () => {
  const currentDevices = getDevices();

  const onlineDevices = currentDevices
    .filter(device => device?.isOnline)
    .sort((a, b) => (a?.id || 0) - (b?.id || 0));

  return onlineDevices;
};

// 自动选择第一个有CAM设备的鸟笼
const autoSelectFirstOnlineDevice = async () => {
  const onlineCageKey = birdcageGroups.value.find(g =>
    g.devices.some((d: any) => d.isOnline && d.device_type === 'ESP32-CAM')
  );
  if (onlineCageKey) {
    selectedBirdcageKey.value = `${onlineCageKey.area}|${onlineCageKey.number}`;
    showAllOfflineAlert.value = false;
    await fetchRealtimeData();
    startRealtimeDataTimer();
    startEnvDataTimer();
    nextTick(() => {
      initGaugeCharts();
    });
  } else {
    selectedBirdcageKey.value = '';
    showAllOfflineAlert.value = true;
  }
};

const handleBirdcageChange = async () => {
  if (!selectedBirdcageKey.value) return;
  const allDevices = getDevices() as any[];
  const [area, numStr] = selectedBirdcageKey.value.split('|');
  const number = Number(numStr);
  const cageDevices = allDevices.filter((d: any) => d.area === area && d.number === number);
  const cam = cageDevices.find((d: any) => d.device_type === 'ESP32-CAM');
  const c3 = cageDevices.find((d: any) => d.device_type === 'ESP32-C3');
  
  const parts: string[] = [];
  if (cam) parts.push(`${cam.name}`);
  if (c3) parts.push(`${c3.name}`);
  ElMessage.success(`已选择鸟笼: ${area} #${number}${parts.length ? ' (' + parts.join(' + ') + ')' : ''}`);

  if (activeTab.value === 'realtime') {
    await fetchRealtimeData();
    startRealtimeDataTimer();
    startEnvDataTimer();
    nextTick(() => {
      initGaugeCharts();
    });
    startRealtimeMonitoring();
  }
};

const handleDeviceChange = async () => {
  if (selectedDevice.value) {
    ElMessage.success(`已切换到设备: ${selectedDevice.value.name}`);
    
    // 如果在实时界面，立即获取实时温湿度数据并启动定时器
    if (activeTab.value === 'realtime') {
      await fetchRealtimeData();
      startRealtimeDataTimer();
      startEnvDataTimer();
    }
    
    // 如果在分析界面，需要先获取历史数据再初始化图表
    if (activeTab.value === 'analysis' && selectedDeviceId.value) {
      if (isMobile.value) {
        await getOrUpdateDeviceHistoryData(selectedDeviceId.value);
      } else {
        await fetchDeviceHistoryData(); // 不传参数，获取所有设备数据
      }
    }
    nextTick(() => {
      initCharts();
      initGaugeCharts();
    });
    // 重新开始实时监控
    startRealtimeMonitoring();
  }
};

// 历史数据相关 - 以鸟笼为单位
const historyBirdcageKey = ref<string>('');
const historyC3Data = ref<Array<{ timestamp: string; pm25: number | null; db: number | null; lux: number | null; uv: number | null }>>([]);
const historyC3Loading = ref(false);

const historyBirdcageGroup = computed(() => {
  if (!historyBirdcageKey.value) return null;
  const [area, numStr] = historyBirdcageKey.value.split('|');
  const number = Number(numStr);
  return birdcageGroups.value.find(g => g.area === area && g.number === number) || null;
});

const historyCamDeviceId = computed(() => {
  if (!historyBirdcageGroup.value) return null;
  const cam = historyBirdcageGroup.value.devices.find((d: any) => d.device_type === 'ESP32-CAM');
  return cam?.id ?? null;
});

const historyC3DeviceId = computed(() => {
  if (!historyBirdcageGroup.value) return null;
  const c3 = historyBirdcageGroup.value.devices.find((d: any) => d.device_type === 'ESP32-C3');
  return c3?.id ?? null;
});

const historyCamDevice = computed(() => {
  if (!historyBirdcageGroup.value) return null;
  return historyBirdcageGroup.value.devices.find((d: any) => d.device_type === 'ESP32-CAM') || null;
});

const historyC3Device = computed(() => {
  if (!historyBirdcageGroup.value) return null;
  return historyBirdcageGroup.value.devices.find((d: any) => d.device_type === 'ESP32-C3') || null;
});

const historyCamDataList = computed(() => {
  if (!historyCamDeviceId.value) return [];
  const historyData = getDeviceHistoryData(historyCamDeviceId.value);
  const device = historyCamDevice.value;
  const deviceName = device?.name || '';
  return historyData
    .map(data => ({
      ...data,
      deviceName: deviceName,
      deviceId: historyCamDeviceId.value,
      timestamp: data.timestamp
    }))
    .sort((a, b) => new Date(b.timestamp).getTime() - new Date(a.timestamp).getTime());
});

// 合并 CAM 和 C3 数据为一个统一的列表
const mergedHistoryData = computed(() => {
  const c3Map = new Map<string, { pm25: number | null; db: number | null; lux: number | null; uv: number | null }>();
  for (const item of historyC3Data.value) {
    c3Map.set(item.timestamp, { pm25: item.pm25, db: item.db, lux: item.lux, uv: item.uv });
  }
  const allTimes = new Set<string>();
  for (const item of historyCamDataList.value) allTimes.add(item.timestamp);
  for (const item of historyC3Data.value) allTimes.add(item.timestamp);
  return Array.from(allTimes)
    .sort((a, b) => new Date(b).getTime() - new Date(a).getTime())
    .map(ts => {
      const camItem = historyCamDataList.value.find(d => d.timestamp === ts);
      const c3Item = c3Map.get(ts);
      return {
        timestamp: ts,
        temperature: camItem?.temperature ?? null,
        humidity: camItem?.humidity ?? null,
        pm25: c3Item?.pm25 ?? null,
        db: c3Item?.db ?? null,
        lux: c3Item?.lux ?? null,
        uv: c3Item?.uv ?? null,
      };
    });
});

// 分页相关
const currentPage = ref(1);
const pageSize = ref(10);

// 分页后的合并历史数据
const paginatedHistoryData = computed(() => {
  const start = (currentPage.value - 1) * pageSize.value;
  const end = start + pageSize.value;
  return mergedHistoryData.value.slice(start, end);
});

// 处理每页条数变化
const handleSizeChange = (val: number) => {
  pageSize.value = val;
  currentPage.value = 1;
};

// 处理页码变化
const handleCurrentChange = (val: number) => {
  currentPage.value = val;
};

// 获取 C3 历史传感器数据
const fetchHistoryC3Data = async (c3DeviceId: number) => {
  historyC3Loading.value = true;
  try {
    const [aqiData, soundData, luxData, uvData] = await Promise.allSettled([
      fetchSensorTimeSeries(c3DeviceId, 'pms9103m', 'pm2_5_cf1'),
      fetchSensorTimeSeries(c3DeviceId, 'sound_meter', 'db'),
      fetchSensorTimeSeries(c3DeviceId, 'veml7700', 'lux'),
      fetchSensorTimeSeries(c3DeviceId, 'uv_meter', 'uv_index')
    ]);
    const aqiResult = aqiData.status === 'fulfilled' ? aqiData.value : { times: [], values: [] };
    const soundResult = soundData.status === 'fulfilled' ? soundData.value : { times: [], values: [] };
    const luxResult = luxData.status === 'fulfilled' ? luxData.value : { times: [], values: [] };
    const uvResult = uvData.status === 'fulfilled' ? uvData.value : { times: [], values: [] };
    const soundMap = new Map<string, number>();
    soundResult.times.forEach((t: string, i: number) => soundMap.set(t, soundResult.values[i]!));
    const luxMap = new Map<string, number>();
    luxResult.times.forEach((t: string, i: number) => luxMap.set(t, luxResult.values[i]!));
    const uvMap = new Map<string, number>();
    uvResult.times.forEach((t: string, i: number) => uvMap.set(t, uvResult.values[i]!));
    const merged: Array<{ timestamp: string; pm25: number | null; db: number | null; lux: number | null; uv: number | null }> = [];
    const allTimestamps = new Set([...aqiResult.times, ...soundResult.times, ...luxResult.times, ...uvResult.times]);
    const sortedTimestamps = Array.from(allTimestamps).sort().reverse();
    for (const ts of sortedTimestamps) {
      const idx = aqiResult.times.indexOf(ts);
      merged.push({
        timestamp: ts,
        pm25: idx >= 0 ? (aqiResult.values[idx] ?? null) : null,
        db: soundMap.get(ts) ?? null,
        lux: luxMap.get(ts) ?? null,
        uv: uvMap.get(ts) ?? null,
      });
    }
    historyC3Data.value = merged;
  } catch (error) {
    console.error('获取C3历史数据失败:', error);
    historyC3Data.value = [];
  } finally {
    historyC3Loading.value = false;
  }
};

const handleHistoryBirdcageChange = async () => {
  if (!historyBirdcageKey.value) return;
  const group = historyBirdcageGroup.value;
  if (!group) return;
  const parts: string[] = [];
  if (historyCamDevice.value) parts.push(historyCamDevice.value.name);
  if (historyC3Device.value) parts.push(historyC3Device.value.name);
  ElMessage.success(`已选择鸟笼: ${group.area} #${group.number}${parts.length ? ' (' + parts.join(' + ') + ')' : ''}`);
  currentPage.value = 1;
  if (historyCamDeviceId.value) {
    if (isMobile.value) {
      await getOrUpdateDeviceHistoryData(historyCamDeviceId.value);
    } else {
      await fetchDeviceHistoryData(historyCamDeviceId.value);
    }
  }
  if (historyC3DeviceId.value) {
    await fetchHistoryC3Data(historyC3DeviceId.value);
  } else {
    historyC3Data.value = [];
  }
};

// 图表容器引用
const temperatureChartRef = ref<HTMLElement | null>(null);
const humidityChartRef = ref<HTMLElement | null>(null);
const analysisAqiChartRef = ref<HTMLElement | null>(null);
const analysisSoundChartRef = ref<HTMLElement | null>(null);
const analysisLuxChartRef = ref<HTMLElement | null>(null);
const analysisUvChartRef = ref<HTMLElement | null>(null);

// 时间范围状态
const timeRange = ref<'today' | 'two_days'>('today');
const timeRangeLabels = [
  { text: '今日', value: 'today' as const },
  { text: '两天', value: 'two_days' as const }
];

// 获取时间范围显示文本
const timeRangeText = computed(() => {
  return timeRange.value === 'today' ? '今日' : '两天';
});

// 获取时间范围进度百分比
const getTimeRangeProgress = () => {
  const index = timeRangeLabels.findIndex(l => l.value === timeRange.value);
  return (index / (timeRangeLabels.length - 1)) * 100;
};

// 设置时间范围
const setTimeRange = (range: 'today' | 'two_days') => {
  timeRange.value = range;
  refreshChartsForTimeRange();
};

// 将 HH:MM 格式的时间字符串转换为从当天0点开始的分钟数
const timeToMinutes = (timeStr: string): number => {
  const parts = timeStr.split(':');
  if (parts.length !== 2) return 0;
  const hoursStr = parts[0];
  const minutesStr = parts[1];
  if (!hoursStr || !minutesStr) return 0;
  const hours = parseInt(hoursStr, 10);
  const minutes = parseInt(minutesStr, 10);
  if (isNaN(hours) || isNaN(minutes)) return 0;
  return hours * 60 + minutes;
};

// 根据时间范围刷新图表
const refreshChartsForTimeRange = async () => {
  if (!analysisBirdcageKey.value) return;

  if (isMobile.value) {
    initMobileCharts();
  } else {
    initDesktopCharts();
  }
};

// 图表实例
let temperatureChart: echarts.ECharts | null = null;
let humidityChart: echarts.ECharts | null = null;
let analysisAqiChart: echarts.ECharts | null = null;
let analysisSoundChart: echarts.ECharts | null = null;
let analysisLuxChart: echarts.ECharts | null = null;
let analysisUvChart: echarts.ECharts | null = null;

// 实时监控图片帧
const currentFrameImage = ref<string>('');
const lastFrameTime = ref<string>('');
const connectionError = ref<string>('');
const isStreamDisconnected = ref<boolean>(false);
const currentFps = ref<number>(0);
let lastFrameTimestamp = 0;
let previousFrameTimestamp = 0;
let streamCheckInterval: number | null = null;
let ws: WebSocket | null = null;
let currentImageUrl: string = '';

// 帧率计算相关
const fpsHistory: number[] = [];
let fpsUpdateInterval: number | null = null;
const MAX_FPS = 60; // 最大合理帧率
const MIN_FPS = 1; // 最小合理帧率
const SMOOTHING_FACTOR = 0.7; // 平滑因子，值越大平滑效果越好，但响应越慢
let smoothedFps = 0;

// 计算0.5秒内的平均帧率（使用平滑算法）
const calculateAverageFps = () => {
  if (fpsHistory.length === 0) {
    currentFps.value = 0;
    smoothedFps = 0;
    return;
  }
  
  // 计算当前平均值
  const sum = fpsHistory.reduce((acc, fps) => acc + fps, 0);
  const currentAverage = sum / fpsHistory.length;
  
  // 使用指数移动平均（EMA）进行平滑
  if (smoothedFps === 0) {
    // 首次计算
    smoothedFps = currentAverage;
  } else {
    // EMA: 新值 = 旧值 * (1 - 平滑因子) + 当前值 * 平滑因子
    smoothedFps = smoothedFps * (1 - SMOOTHING_FACTOR) + currentAverage * SMOOTHING_FACTOR;
  }
  
  currentFps.value = Math.round(smoothedFps);
};

// 视频翻转控制
const flipHorizontal = ref(false);
const flipVertical = ref(false);
const imageQuality = ref(32); // 默认画质为32
const frameSize = ref('FRAMESIZE_VGA'); // 默认视频尺寸

// 环境数据环形进度条
const aqiValue = ref(75);
const dbValue = ref(45);
const aqiGaugeRef = ref<HTMLElement | null>(null);
const dbGaugeRef = ref<HTMLElement | null>(null);
const aqiGaugeRefMobile = ref<HTMLElement | null>(null);
const dbGaugeRefMobile = ref<HTMLElement | null>(null);
const luxGaugeRef = ref<HTMLElement | null>(null);
const uvGaugeRef = ref<HTMLElement | null>(null);
const luxGaugeRefMobile = ref<HTMLElement | null>(null);
const uvGaugeRefMobile = ref<HTMLElement | null>(null);
let aqiGaugeChart: echarts.ECharts | null = null;
let dbGaugeChart: echarts.ECharts | null = null;
let aqiGaugeChartMobile: echarts.ECharts | null = null;
let dbGaugeChartMobile: echarts.ECharts | null = null;
let luxGaugeChart: echarts.ECharts | null = null;
let uvGaugeChart: echarts.ECharts | null = null;
let luxGaugeChartMobile: echarts.ECharts | null = null;
let uvGaugeChartMobile: echarts.ECharts | null = null;

// 环境数据采集定时器
let envDataTimer: number | null = null;

// AQI 计算：根据 PM2.5、PM10、CO2、TVOC 综合计算空气质量指数
const calculateAQI = (pm25: number, pm10: number, co2: number, tvoc: number): number => {
  // PM2.5 断点 (浓度下限, 浓度上限, AQI下限, AQI上限)
  const pm25Breakpoints: [number, number, number, number][] = [
    [0, 35, 0, 50],
    [36, 75, 51, 100],
    [76, 115, 101, 150],
    [116, 150, 151, 200],
    [151, 250, 201, 300],
    [251, 500, 301, 500],
  ];
  // PM10 断点
  const pm10Breakpoints: [number, number, number, number][] = [
    [0, 50, 0, 50],
    [51, 150, 51, 100],
    [151, 250, 101, 150],
    [251, 350, 151, 200],
    [351, 420, 201, 300],
    [421, 600, 301, 500],
  ];
  // CO2 断点 (ppm) — 室内空气质量参考
  const co2Breakpoints: [number, number, number, number][] = [
    [0, 400, 0, 50],
    [401, 600, 51, 100],
    [601, 1000, 101, 150],
    [1001, 1400, 151, 200],
    [1401, 2000, 201, 300],
    [2001, 5000, 301, 500],
  ];
  // TVOC 断点 (ppb)
  const tvocBreakpoints: [number, number, number, number][] = [
    [0, 250, 0, 50],
    [251, 500, 51, 100],
    [501, 1000, 101, 150],
    [1001, 1600, 151, 200],
    [1601, 3000, 201, 300],
    [3001, 10000, 301, 500],
  ];

  const calc = (c: number, bp: [number, number, number, number][]): number => {
    if (c <= 0) return 0;
    for (const [cLo, cHi, iLo, iHi] of bp) {
      if (c >= cLo && c <= cHi) {
        return ((iHi - iLo) / (cHi - cLo)) * (c - cLo) + iLo;
      }
    }
    return 500;
  };

  // 计算各分指数（缺失传感器时返回 -1 表示忽略）
  const pm25Aqi = pm25 > 0 ? calc(pm25, pm25Breakpoints) : -1;
  const pm10Aqi = pm10 > 0 ? calc(pm10, pm10Breakpoints) : -1;
  const co2Aqi = co2 > 0 ? calc(co2, co2Breakpoints) : -1;
  const tvocAqi = tvoc > 0 ? calc(tvoc, tvocBreakpoints) : -1;

  // 取有效分指数中的最大值（容错：至少需要1个传感器有效）
  const validAqi = [pm25Aqi, pm10Aqi, co2Aqi, tvocAqi].filter(v => v >= 0);
  return validAqi.length > 0 ? Math.round(Math.max(...validAqi)) : 0;
};

// ─── 5分钟滑动平均 ───
// 5分钟 = 300秒，每5秒采样一次 → 需要60个数据点
const MAX_HISTORY = 60;
const pm25History: number[] = [];
const pm10History: number[] = [];
const co2History: number[] = [];
const tvocHistory: number[] = [];

const luxHistory: number[] = [];
const uvHistory: number[] = [];

const pushHistory = (arr: number[], value: number) => {
  arr.push(value);
  if (arr.length > MAX_HISTORY) arr.shift();
};

const slidingAvg = (arr: number[]): number => {
  if (arr.length === 0) return 0;
  const sum = arr.reduce((a, b) => a + b, 0);
  return sum / arr.length;
};

const updateAQIFromHistory = () => {
  aqiValue.value = calculateAQI(
    slidingAvg(pm25History),
    slidingAvg(pm10History),
    slidingAvg(co2History),
    slidingAvg(tvocHistory),
  );
  updateGaugeCharts();
};

const fetchAirQualityData = async () => {
  if (!c3DeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: c3DeviceId.value,
      sensor_type: 'pms9103m',
      limit: 1,
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const record = response.data[0];
      const parsed = JSON.parse(record.data);
      if (parsed.pm2_5_cf1 !== undefined || parsed.pm10_cf1 !== undefined) {
        pushHistory(pm25History, Number(parsed.pm2_5_cf1) || 0);
        pushHistory(pm10History, Number(parsed.pm10_cf1) || 0);
        updateAQIFromHistory();
      }
    }
  } catch (error) {
    console.error('获取颗粒物数据失败:', error);
  }
};

const fetchSGP30Data = async () => {
  if (!c3DeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: c3DeviceId.value,
      sensor_type: 'sgp30',
      limit: 1,
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const record = response.data[0];
      const parsed = JSON.parse(record.data);
      if (parsed.co2_ppm !== undefined || parsed.tvoc_ppb !== undefined) {
        pushHistory(co2History, Number(parsed.co2_ppm) || 0);
        pushHistory(tvocHistory, Number(parsed.tvoc_ppb) || 0);
        updateAQIFromHistory();
      }
    }
  } catch (error) {
    console.error('获取SGP30数据失败:', error);
  }
};

const fetchSoundData = async () => {
  if (!c3DeviceId.value) return;
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: c3DeviceId.value,
      sensor_type: 'sound_meter',
      limit: 1,
    });
    if (response.code === 200 && response.data && response.data.length > 0) {
      const record = response.data[0];
      const parsed = JSON.parse(record.data);
      if (parsed.db !== undefined) {
        dbValue.value = Math.round(parsed.db);
        updateGaugeCharts();
      }
    }
  } catch (error) {
    console.error('获取声音分贝数据失败:', error);
  }
};

const fetchAllEnvData = () => {
  fetchAirQualityData();
  fetchSGP30Data();
  fetchSoundData();
  fetchLuxData();
  fetchUVData();
};

const startEnvDataTimer = () => {
  stopEnvDataTimer();
  fetchAllEnvData();
  envDataTimer = window.setInterval(fetchAllEnvData, 5000);
};

const stopEnvDataTimer = () => {
  if (envDataTimer) {
    clearInterval(envDataTimer);
    envDataTimer = null;
  }
};

// 初始化环形进度条
const initGaugeCharts = () => {
  if (isMobile.value) {
    if (aqiGaugeRefMobile.value) {
      if (aqiGaugeChartMobile) aqiGaugeChartMobile.dispose();
      aqiGaugeChartMobile = echarts.init(aqiGaugeRefMobile.value);
    }
    if (dbGaugeRefMobile.value) {
      if (dbGaugeChartMobile) dbGaugeChartMobile.dispose();
      dbGaugeChartMobile = echarts.init(dbGaugeRefMobile.value);
    }
    if (luxGaugeRefMobile.value) {
      if (luxGaugeChartMobile) luxGaugeChartMobile.dispose();
      luxGaugeChartMobile = echarts.init(luxGaugeRefMobile.value);
    }
    if (uvGaugeRefMobile.value) {
      if (uvGaugeChartMobile) uvGaugeChartMobile.dispose();
      uvGaugeChartMobile = echarts.init(uvGaugeRefMobile.value);
    }
  } else {
    if (aqiGaugeRef.value) {
      if (aqiGaugeChart) aqiGaugeChart.dispose();
      aqiGaugeChart = echarts.init(aqiGaugeRef.value);
    }
    if (dbGaugeRef.value) {
      if (dbGaugeChart) dbGaugeChart.dispose();
      dbGaugeChart = echarts.init(dbGaugeRef.value);
    }
    if (luxGaugeRef.value) {
      if (luxGaugeChart) luxGaugeChart.dispose();
      luxGaugeChart = echarts.init(luxGaugeRef.value);
    }
    if (uvGaugeRef.value) {
      if (uvGaugeChart) uvGaugeChart.dispose();
      uvGaugeChart = echarts.init(uvGaugeRef.value);
    }
  }
  initGaugeChartOption();
  updateGaugeCharts();
};

const getActiveGaugeCharts = () => {
  if (isMobile.value) {
    return {
      aqi: aqiGaugeChartMobile, db: dbGaugeChartMobile,
      lux: luxGaugeChartMobile, uv: uvGaugeChartMobile
    };
  }
  return {
    aqi: aqiGaugeChart, db: dbGaugeChart,
    lux: luxGaugeChart, uv: uvGaugeChart
  };
};

// 获取光照强度颜色
const getLuxColor = (lux: number) => {
  if (lux > 50000) return '#f56c6c';
  if (lux > 25000) return '#e6a23c';
  return '#67c23a';
};
const getLuxBg = (lux: number) => {
  if (lux > 50000) return '#f56c6c1a';
  if (lux > 25000) return '#e6a23c1a';
  return '#67c23a1a';
};

// 获取紫外线颜色
const getUVColor = (uv: number) => {
  if (uv > 11) return '#f56c6c';
  if (uv > 8) return '#f56c6c';
  if (uv > 5) return '#e6a23c';
  if (uv > 2) return '#e6a23c';
  return '#67c23a';
};
const getUVBg = (uv: number) => {
  if (uv > 11) return '#f56c6c1a';
  if (uv > 8) return '#f56c6c1a';
  if (uv > 5) return '#e6a23c1a';
  if (uv > 2) return '#e6a23c1a';
  return '#67c23a1a';
};

// Lux 值格式化
const formatLux = (v: number) => {
  if (v >= 100000) return (v / 100000).toFixed(1) + 'M';
  if (v >= 1000) return (v / 1000).toFixed(1) + 'K';
  return String(Math.round(v));
};

const getGaugeOption = (max: number, color: string, bgColor: string, value: number, name: string, formatter?: (v: number) => string) => ({
  series: [{
    type: 'gauge',
    radius: '95%',
    center: ['50%', '60%'],
    startAngle: 180,
    endAngle: 0,
    min: 0,
    max,
    splitNumber: 1,
    pointer: {
      show: false
    },
    progress: {
      show: true,
      overlap: false,
      roundCap: true,
      clip: false,
      itemStyle: {
        color: {
          type: 'linear',
          x: 0, y: 0, x2: 1, y2: 0,
          colorStops: [
            { offset: 0, color: color },
            { offset: 1, color: color + 'aa' }
          ]
        },
        shadowColor: color + '40',
        shadowBlur: 10,
        shadowOffsetY: 2
      }
    },
    axisLine: {
      lineStyle: {
        width: 14,
        color: [[1, bgColor]]
      }
    },
    splitLine: { show: false },
    axisTick: { show: false },
    axisLabel: { show: false },
    title: { show: false },
    detail: {
      show: true,
      valueAnimation: true,
      fontSize: 20,
      fontWeight: 'bold',
      color: color,
      formatter: formatter ? formatter : '{value}',
      offsetCenter: [0, '0%']
    },
    data: [{ value: Math.min(value, max), name }]
  }]
});

const initGaugeChartOption = () => {
  const aqiColor = aqiValue.value > 150 ? '#f56c6c' : aqiValue.value > 100 ? '#e6a23c' : '#67c23a';
  const dbColor = dbValue.value > 85 ? '#f56c6c' : dbValue.value > 65 ? '#e6a23c' : '#67c23a';
  const aqiBg = aqiValue.value > 150 ? '#f56c6c1a' : aqiValue.value > 100 ? '#e6a23c1a' : '#67c23a1a';
  const dbBg = dbValue.value > 85 ? '#f56c6c1a' : dbValue.value > 65 ? '#e6a23c1a' : '#67c23a1a';
  const luxVal = slidingAvg(luxHistory);
  const luxColor = getLuxColor(luxVal);
  const luxBg = getLuxBg(luxVal);
  const uvVal = slidingAvg(uvHistory);
  const uvColor = getUVColor(uvVal);
  const uvBg = getUVBg(uvVal);
  const { aqi, db, lux, uv } = getActiveGaugeCharts();

  if (aqi) {
    aqi.setOption(getGaugeOption(300, aqiColor, aqiBg, aqiValue.value, 'AQI'));
  }
  if (db) {
    db.setOption(getGaugeOption(120, dbColor, dbBg, dbValue.value, 'dB'));
  }
  if (lux) {
    lux.setOption(getGaugeOption(100000, luxColor, luxBg, luxVal, 'Lux', formatLux));
  }
  if (uv) {
    uv.setOption(getGaugeOption(12, uvColor, uvBg, uvVal, 'UV'));
  }
};

const updateGaugeCharts = () => {
  const aqiColor = aqiValue.value > 150 ? '#f56c6c' : aqiValue.value > 100 ? '#e6a23c' : '#67c23a';
  const dbColor = dbValue.value > 85 ? '#f56c6c' : dbValue.value > 65 ? '#e6a23c' : '#67c23a';
  const aqiBg = aqiValue.value > 150 ? '#f56c6c1a' : aqiValue.value > 100 ? '#e6a23c1a' : '#67c23a1a';
  const dbBg = dbValue.value > 85 ? '#f56c6c1a' : dbValue.value > 65 ? '#e6a23c1a' : '#67c23a1a';
  const luxVal = slidingAvg(luxHistory);
  const luxColor = getLuxColor(luxVal);
  const luxBg = getLuxBg(luxVal);
  const uvVal = slidingAvg(uvHistory);
  const uvColor = getUVColor(uvVal);
  const uvBg = getUVBg(uvVal);
  const { aqi, db, lux, uv } = getActiveGaugeCharts();

  if (aqi) {
    aqi.setOption(getGaugeOption(300, aqiColor, aqiBg, aqiValue.value, 'AQI'));
  }
  if (db) {
    db.setOption(getGaugeOption(120, dbColor, dbBg, dbValue.value, 'dB'));
  }
  if (lux) {
    lux.setOption(getGaugeOption(100000, luxColor, luxBg, luxVal, 'Lux', formatLux));
  }
  if (uv) {
    uv.setOption(getGaugeOption(12, uvColor, uvBg, uvVal, 'UV'));
  }
};

const updateLuxGauge = () => {
  const luxVal = slidingAvg(luxHistory);
  const luxColor = getLuxColor(luxVal);
  const luxBg = getLuxBg(luxVal);
  const { lux } = getActiveGaugeCharts();
  if (lux) {
    lux.setOption(getGaugeOption(100000, luxColor, luxBg, luxVal, 'Lux', formatLux));
  }
};

const updateUVGauge = () => {
  const uvVal = slidingAvg(uvHistory);
  const uvColor = getUVColor(uvVal);
  const uvBg = getUVBg(uvVal);
  const { uv } = getActiveGaugeCharts();
  if (uv) {
    uv.setOption(getGaugeOption(12, uvColor, uvBg, uvVal, 'UV'));
  }
};

// 发送画质设置到WebSocket连接
const sendStreamQuality = async (quality: number) => {
  // 使用当前已建立的WebSocket连接发送画质设置
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    console.warn('WebSocket连接未建立，无法发送画质设置');
    return;
  }
  
  const message = JSON.stringify({
    code: 1,
    item: 'camera',
    key: 'jpeg_quality',
    values: quality.toString()
  });
  
  try {
    ws.send(message);
  } catch (error) {
  }
};

// 发送视频尺寸设置到WebSocket连接
const sendFrameSize = async (size: string) => {
  // 使用当前已建立的WebSocket连接发送视频尺寸设置
  if (!ws || ws.readyState !== WebSocket.OPEN) {
    console.warn('WebSocket连接未建立，无法发送视频尺寸设置');
    return;
  }
  
  // 重置帧率计算
  fpsHistory.length = 0;
  currentFps.value = 0;
  smoothedFps = 0;
  previousFrameTimestamp = 0;
  
  const message = JSON.stringify({
    code: 1,
    item: 'camera',
    key: 'frame_size',
    values: size
  });
  
  try {
    ws.send(message);
  } catch (error) {
  }
};
  
// 监听滑块变化
watch(imageQuality, (newQuality) => {
  // 双端统一：1-63 -> 63-1
  const invertedQuality = 63 - newQuality;
  sendStreamQuality(invertedQuality);
});

// 监听视频尺寸变化
watch(frameSize, (newSize) => {
  sendFrameSize(newSize);
});

// 切换左右翻转
const toggleHorizontalFlip = () => {
  flipHorizontal.value = !flipHorizontal.value;
};

// 切换上下翻转
const toggleVerticalFlip = () => {
  flipVertical.value = !flipVertical.value;
};

// 处理接收到的图片帧数据
const handleFrameData = (data: any) => {
  try {
    let blob: Blob;
    if (typeof data === 'string') {
      // 假设是base64编码的图片数据
      blob = base64ToBlob(data);
    } else if (data instanceof Blob) {
      blob = data;
    } else if (data instanceof ArrayBuffer) {
      blob = new Blob([data], { type: 'image/jpeg' });
    } else if (data instanceof Uint8Array) {
      // 使用类型断言告诉TypeScript这是一个有效的BlobPart
      blob = new Blob([data as any], { type: 'image/jpeg' });
    } else {
      throw new Error('不支持的数据类型');
    }
    
    // 释放旧的URL对象
    if (currentImageUrl) {
      URL.revokeObjectURL(currentImageUrl);
    }
    
    // 创建新的URL
    currentImageUrl = URL.createObjectURL(blob);
    currentFrameImage.value = currentImageUrl;
    lastFrameTime.value = new Date().toLocaleTimeString('zh-CN');
    
    // 计算帧率
    const now = Date.now();
    if (previousFrameTimestamp > 0) {
      const frameInterval = now - previousFrameTimestamp;
      if (frameInterval > 0) {
        let frameFps = 1000 / frameInterval;
        
        // 过滤异常值
        if (frameFps >= MIN_FPS && frameFps <= MAX_FPS) {
          fpsHistory.push(frameFps);
          // 只保留最近30帧的数据，避免数组过大且能快速响应变化
          if (fpsHistory.length > 30) {
            fpsHistory.shift();
          }
        }
      }
    }
    previousFrameTimestamp = now;
    
    lastFrameTimestamp = now;
    isStreamDisconnected.value = false;
  } catch (error) {
  }
};

// 将base64字符串转换为Blob
const base64ToBlob = (base64: string) => {
  const parts = base64.split(';base64,');
  if (parts.length < 2 || !parts[1] || !parts[0]) {
    throw new Error('Invalid base64 string');
  }
  const contentParts = parts[0].split(':');
  const contentType = contentParts.length > 1 ? contentParts[1] : 'image/jpeg';
  const raw = window.atob(parts[1]);
  const rawLength = raw.length;
  const uInt8Array = new Uint8Array(rawLength);
  
  for (let i = 0; i < rawLength; ++i) {
    uInt8Array[i] = raw.charCodeAt(i);
  }
  
  return new Blob([uInt8Array], { type: contentType });
};

// 开始实时监控
const startRealtimeMonitoring = async () => {
  stopRealtimeMonitoring();
  
  if (!selectedDeviceId.value) {
    return;
  }
  
  try {
    let token = await refreshToken();
    if (!token) {
      connectionError.value = '未登录，请先登录';
      return;
    }
    
    const wsUrl = `/api/stream/viewer/ws/${selectedDeviceId.value}?token=${token}`;
    connectionError.value = '正在开启ws连接';
    ws = new WebSocket(wsUrl);
    
    ws.onopen = () => {
      connectionError.value = '';
      lastFrameTimestamp = Date.now();
      isStreamDisconnected.value = false;
      
      if (streamCheckInterval) {
        clearInterval(streamCheckInterval);
      }
      streamCheckInterval = window.setInterval(() => {
        const now = Date.now();
        if (now - lastFrameTimestamp > 2000 && currentFrameImage.value) {
          isStreamDisconnected.value = true;
        }
      }, 2500);
    };
    
    ws.onmessage = (event) => {
      try {
        if (typeof event.data === 'string' && event.data.includes('设备已断开')) {
          connectionError.value = '设备已断开';
          return;
        }
        
        try {
          const data = JSON.parse(event.data);
          if (data.code === 400) {
            connectionError.value = '订阅失败';
          }
        } catch (jsonError) {
          handleFrameData(event.data);
        }
      } catch (error) {
      }
    };
    
    ws.onerror = () => {
      connectionError.value = 'WSS连接错误';
    };
    
    ws.onclose = (event) => {
      if (!event.wasClean) {
        connectionError.value = 'WSS连接已关闭';
      }
    };
    
  } catch (error) {
    connectionError.value = '建立WSS连接失败';
  }
};

// 手动重连WebSocket
const reconnectWebSocket = () => {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ElMessageBox.confirm(
      '当前WebSocket连接正常，确定要重新连接吗？',
      '确认重连',
      {
        confirmButtonText: '确定',
        cancelButtonText: '取消',
        type: 'warning',
      }
    )
      .then(() => {
        startRealtimeMonitoring();
      })
      .catch(() => {
      });
  } else {
    startRealtimeMonitoring();
  }
};

// 刷新token
const refreshToken = async () => {
  try {
    const data = await api.post('/api/refresh');
    if (data.access_token) {
      localStorage.setItem('accessToken', data.access_token);
      return data.access_token;
    }
    return getToken();
  } catch (error) {
    return getToken();
  }
};

// 开启设备推流
const startDeviceStreaming = async () => {
  if (!selectedDeviceId.value) return;
  
  try {
    const data = await api.post(`/api/stream/viewer/following/${selectedDeviceId.value}`);
    
    await new Promise(resolve => setTimeout(resolve, 4000));
    
    const isSuccess = (data.code === 1 || 
      (data.msg && (data.msg.includes('OK') || data.msg.includes('success') || data.msg.includes('成功'))));
    
    if (isSuccess) {
      connectionError.value = '';
    } else if (data.msg && data.msg.toLowerCase().includes('device has subscribed')) {
      connectionError.value = '';
    } else if (data.msg && data.msg.toLowerCase().includes('viewer has not registerd')) {
      connectionError.value = '';
    } else {
      connectionError.value = '设备推流开启失败';
    }
  } catch (error) {
    // 延迟3秒再显示错误提示
    await new Promise(resolve => setTimeout(resolve, 3000));
    connectionError.value = '开启设备推流失败';
  }
};

// 停止实时监控
const stopRealtimeMonitoring = async () => {
  // 清除图片流检查定时器
  if (streamCheckInterval) {
    clearInterval(streamCheckInterval);
    streamCheckInterval = null;
  }
  
  if (ws && (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING)) {
    // 新的连接模式：WebSocket断开时会自动清理订阅关系
    ws.close();
    ws = null;
  }
  
  // 清空帧率历史数据
  fpsHistory.length = 0;
  currentFps.value = 0;
  smoothedFps = 0;
  previousFrameTimestamp = 0;
  
  if (isMobile.value && currentImageUrl) {
    URL.revokeObjectURL(currentImageUrl);
    currentImageUrl = '';
  }
  
  if (isMobile.value) {
    currentFrameImage.value = '';
  }
  
  connectionError.value = '';
  isStreamDisconnected.value = false;
};

// 格式化时间标签，只显示小时和分钟（移动端专用）
const formatTimeLabel = (timestamp: string) => {
  // 如果已经是 HH:00 格式，直接返回
  if (/^\d{2}:\d{2}$/.test(timestamp)) {
    return timestamp;
  }
  const date = new Date(timestamp);
  const hours = date.getHours().toString().padStart(2, '0');
  const minutes = date.getMinutes().toString().padStart(2, '0');
  return `${hours}:${minutes}`;
};

// 采样函数：从数组中均匀采样指定数量的点（移动端专用）
const sampleData = (data: any[], count: number) => {
  if (data.length <= count) return data;
  const result = [];
  const step = (data.length - 1) / (count - 1);
  for (let i = 0; i < count; i++) {
    const index = Math.round(i * step);
    result.push(data[index]);
  }
  return result;
};

// 窗口大小变化时调整图表
const handleResize = () => {
  temperatureChart?.resize();
  humidityChart?.resize();
  analysisAqiChart?.resize();
  analysisSoundChart?.resize();
  analysisLuxChart?.resize();
  analysisUvChart?.resize();
};

const handleAnalysisBirdcageChange = async () => {
  if (!analysisBirdcageKey.value) return;
  await initCharts();
};

const fetchSensorTimeSeries = async (deviceId: number, sensorType: string, valueKey: string): Promise<{ times: string[]; values: number[] }> => {
  const pairs: { time: string; value: number }[] = [];
  try {
    const response = await api.get('/api/sensor-upload', {
      device_id: deviceId,
      sensor_type: sensorType,
      limit: 200,
    });
    if (response.code === 200 && response.data && Array.isArray(response.data)) {
      const rawData: any[] = response.data;
      const cutoff = new Date();
      const secondsAgo = timeRange.value === 'today' ? 86400 : 172800;
      cutoff.setSeconds(cutoff.getSeconds() - secondsAgo);
      
      for (const item of rawData) {
        const ts = new Date(item.timestamp);
        if (ts < cutoff) continue;
        if (isNaN(ts.getTime())) continue;
        let parsed: any;
        try {
          parsed = typeof item.data === 'string' ? JSON.parse(item.data) : item.data;
        } catch {
          continue;
        }
        if (parsed[valueKey] !== undefined && parsed[valueKey] !== null) {
          // "今天"模式只显示时间，"两天"模式显示日期+时间避免重复
          if (timeRange.value === 'two_days') {
            const month = (ts.getMonth() + 1).toString().padStart(2, '0');
            const day = ts.getDate().toString().padStart(2, '0');
            const hh = ts.getHours().toString().padStart(2, '0');
            const mm = ts.getMinutes().toString().padStart(2, '0');
            pairs.push({ time: `${month}/${day} ${hh}:${mm}`, value: Number(parsed[valueKey]) });
          } else {
            const hh = ts.getHours().toString().padStart(2, '0');
            const mm = ts.getMinutes().toString().padStart(2, '0');
            pairs.push({ time: `${hh}:${mm}`, value: Number(parsed[valueKey]) });
          }
        }
      }
    }
  } catch (error) {
    console.error(`获取${sensorType}时间序列数据失败:`, error);
  }
  const times = pairs.map(p => p.time);
  const values = pairs.map(p => p.value);
  return { times, values };
};

// 初始化图表
const initCharts = async () => {
  if (!analysisCamDeviceId.value && !analysisC3DeviceId.value) return;
  
  if (isMobile.value) {
    await initMobileCharts();
  } else {
    await initDesktopCharts();
  }
};

const makeLineChartOption = (times: string[], values: number[], _name: string, color: string, unit: string) => {
  const validValues = values.filter(v => v !== undefined && v !== null);
  const vMin = validValues.length > 0 ? Math.min(...validValues) : 0;
  const vMax = validValues.length > 0 ? Math.max(...validValues) : 100;
  const range = vMax - vMin;
  let yMin = vMin;
  let yMax = vMax;
  if (range < 5) { yMin = Math.floor(vMin - 1); yMax = Math.ceil(vMax + 1); }
  
  return {
    tooltip: { trigger: 'axis' as const },
    xAxis: { type: 'category' as const, data: times },
    yAxis: { type: 'value' as const, name: unit, min: yMin, max: yMax },
    series: [{
      data: values,
      type: 'line' as const,
      smooth: true,
      itemStyle: { color },
      areaStyle: {
        color: {
          type: 'linear' as const,
          x: 0, y: 0, x2: 0, y2: 1,
          colorStops: [
            { offset: 0, color: (areaColorMap[color] || ['rgba(0,0,0,0.1)'])[0] },
            { offset: 1, color: (areaColorMap[color] || ['rgba(0,0,0,0.05)', 'rgba(0,0,0,0.01)'])[1] }
          ]
        }
      }
    }]
  };
};

const areaColorMap: Record<string, string[]> = {
  '#ff7875': ['rgba(255, 120, 117, 0.3)', 'rgba(255, 120, 117, 0.05)'],
  '#69c0ff': ['rgba(105, 192, 255, 0.3)', 'rgba(105, 192, 255, 0.05)'],
  '#e6a23c': ['rgba(230, 162, 60, 0.3)', 'rgba(230, 162, 60, 0.05)'],
  '#909399': ['rgba(144, 147, 153, 0.3)', 'rgba(144, 147, 153, 0.05)'],
  '#67c23a': ['rgba(103, 194, 58, 0.3)', 'rgba(103, 194, 58, 0.05)'],
  '#f56c6c': ['rgba(245, 108, 108, 0.3)', 'rgba(245, 108, 108, 0.05)'],
};

const filterAndSampleSensorData = async (camId: number) => {
  const sensorData = await fetchSensorData(camId, timeRange.value);
  const now = new Date();
  const nowMinutes = now.getHours() * 60 + now.getMinutes();
  const filteredTimes: string[] = [];
  const filteredTemp: number[] = [];
  const filteredHum: number[] = [];
  for (let i = 0; i < sensorData.times.length; i++) {
    const ts = sensorData.times[i];
    const tv = sensorData.temperatureValues[i];
    const hv = sensorData.humidityValues[i];
    if (ts) {
      // "今天"模式下过滤掉未来时间点，"两天"模式不过滤（数据已由后端按时间范围过滤）
      if (timeRange.value === 'today' && timeToMinutes(ts) > nowMinutes) continue;
      filteredTimes.push(ts);
      filteredTemp.push(tv !== undefined ? tv : null as unknown as number);
      filteredHum.push(hv !== undefined ? hv : null as unknown as number);
    }
  }
  return { filteredTimes, filteredTemp, filteredHum };
};

// 桌面端图表初始化
const initDesktopCharts = async () => {
  const camId = analysisCamDeviceId.value;
  const c3Id = analysisC3DeviceId.value;
  
  if (camId) {
    const { filteredTimes, filteredTemp, filteredHum } = await filterAndSampleSensorData(camId);
    
    if (temperatureChartRef.value) {
      temperatureChart?.dispose();
      temperatureChart = echarts.init(temperatureChartRef.value);
      temperatureChart.setOption(makeLineChartOption(filteredTimes, filteredTemp, '温度', '#ff7875', '温度 (℃)'));
    }
    if (humidityChartRef.value) {
      humidityChart?.dispose();
      humidityChart = echarts.init(humidityChartRef.value);
      humidityChart.setOption(makeLineChartOption(filteredTimes, filteredHum, '湿度', '#69c0ff', '湿度 (%)'));
    }
  }
  
  // 环境传感器 (C3)
  if (c3Id) {
    // 并行获取所有C3传感器数据，单个传感器失败不影响其他
    const [aqiData, soundData, luxData, uvData] = await Promise.allSettled([
      fetchSensorTimeSeries(c3Id, 'pms9103m', 'pm2_5_cf1'),
      fetchSensorTimeSeries(c3Id, 'sound_meter', 'db'),
      fetchSensorTimeSeries(c3Id, 'veml7700', 'lux'),
      fetchSensorTimeSeries(c3Id, 'uv_meter', 'uv_index')
    ]);

    // 空气质量图表
    if (analysisAqiChartRef.value) {
      analysisAqiChart?.dispose();
      analysisAqiChart = echarts.init(analysisAqiChartRef.value);
      const aqiResult = aqiData.status === 'fulfilled' ? aqiData.value : { times: [], values: [] };
      analysisAqiChart.setOption(makeLineChartOption(aqiResult.times, aqiResult.values, 'AQI', '#e6a23c', 'PM2.5 (μg/m³)'));
    }
    // 声音分贝图表
    if (analysisSoundChartRef.value) {
      analysisSoundChart?.dispose();
      analysisSoundChart = echarts.init(analysisSoundChartRef.value);
      const soundResult = soundData.status === 'fulfilled' ? soundData.value : { times: [], values: [] };
      analysisSoundChart.setOption(makeLineChartOption(soundResult.times, soundResult.values, 'dB', '#909399', '分贝 (dB)'));
    }
    // 光照强度图表
    if (analysisLuxChartRef.value) {
      analysisLuxChart?.dispose();
      analysisLuxChart = echarts.init(analysisLuxChartRef.value);
      const luxResult = luxData.status === 'fulfilled' ? luxData.value : { times: [], values: [] };
      analysisLuxChart.setOption(makeLineChartOption(luxResult.times, luxResult.values, 'Lux', '#67c23a', '光照 (Lux)'));
    }
    // 紫外线图表
    if (analysisUvChartRef.value) {
      analysisUvChart?.dispose();
      analysisUvChart = echarts.init(analysisUvChartRef.value);
      const uvResult = uvData.status === 'fulfilled' ? uvData.value : { times: [], values: [] };
      analysisUvChart.setOption(makeLineChartOption(uvResult.times, uvResult.values, 'UV', '#f56c6c', 'UV指数'));
    }
  }
};

const makeMobileLineOption = (times: string[], values: number[], name: string, color: string) => {
  const isTwoDays = timeRange.value === 'two_days';
  return {
    tooltip: { trigger: 'axis' as const },
    grid: { left: '15%', right: '8%', bottom: isTwoDays ? '22%' : '15%', top: '12%', containLabel: false },
    xAxis: { type: 'category' as const, data: times, axisLabel: { fontSize: isTwoDays ? 8 : 10, rotate: isTwoDays ? 45 : 0, interval: isTwoDays ? 'auto' : 1 } },
    yAxis: { type: 'value' as const, name, nameTextStyle: { fontSize: 10 }, axisLabel: { fontSize: 10 } },
    series: [{ data: values, type: 'line' as const, smooth: true, itemStyle: { color }, areaStyle: { color: { type: 'linear' as const, x: 0, y: 0, x2: 0, y2: 1, colorStops: [{ offset: 0, color: (areaColorMap[color] || ['rgba(0,0,0,0.1)'])[0] }, { offset: 1, color: (areaColorMap[color] || ['rgba(0,0,0,0.05)', 'rgba(0,0,0,0.01)'])[1] }] } } }]
  };
};

// 移动端图表初始化
const initMobileCharts = async () => {
  const camId = analysisCamDeviceId.value;
  const c3Id = analysisC3DeviceId.value;

  temperatureChart?.dispose();
  humidityChart?.dispose();
  analysisAqiChart?.dispose();
  analysisSoundChart?.dispose();
  analysisLuxChart?.dispose();
  analysisUvChart?.dispose();

  // 温度 & 湿度 (CAM) - 使用 /sensors/grouped 接口
  if (camId) {
    const { filteredTimes, filteredTemp, filteredHum } = await filterAndSampleSensorData(camId);
    const sampledTimes = sampleData(filteredTimes, 12);
    const sampledTemp = sampleData(filteredTemp, 12);
    const sampledHum = sampleData(filteredHum, 12);

    if (temperatureChartRef.value) {
      temperatureChart = echarts.init(temperatureChartRef.value);
      temperatureChart.setOption(makeMobileLineOption(sampledTimes, sampledTemp, '温度 (℃)', '#ff7875'));
    }
    if (humidityChartRef.value) {
      humidityChart = echarts.init(humidityChartRef.value);
      humidityChart.setOption(makeMobileLineOption(sampledTimes, sampledHum, '湿度 (%)', '#69c0ff'));
    }
  }

  // 环境传感器 (C3)
  if (c3Id) {
    // 并行获取所有C3传感器数据，单个传感器失败不影响其他
    const [aqiData, soundData, luxData, uvData] = await Promise.allSettled([
      fetchSensorTimeSeries(c3Id, 'pms9103m', 'pm2_5_cf1'),
      fetchSensorTimeSeries(c3Id, 'sound_meter', 'db'),
      fetchSensorTimeSeries(c3Id, 'veml7700', 'lux'),
      fetchSensorTimeSeries(c3Id, 'uv_meter', 'uv_index')
    ]);

    // 空气质量图表
    if (analysisAqiChartRef.value) {
      analysisAqiChart = echarts.init(analysisAqiChartRef.value);
      const aqiResult = aqiData.status === 'fulfilled' ? aqiData.value : { times: [], values: [] };
      const sAqi = aqiResult.times.length > 0 ? sampleData(aqiResult.times, 12) : [];
      const sAqiV = aqiResult.values.length > 0 ? sampleData(aqiResult.values, 12) : [];
      analysisAqiChart.setOption(makeMobileLineOption(sAqi, sAqiV, 'PM2.5 (μg/m³)', '#e6a23c'));
    }
    // 声音分贝图表
    if (analysisSoundChartRef.value) {
      analysisSoundChart = echarts.init(analysisSoundChartRef.value);
      const soundResult = soundData.status === 'fulfilled' ? soundData.value : { times: [], values: [] };
      const sSound = soundResult.times.length > 0 ? sampleData(soundResult.times, 12) : [];
      const sSoundV = soundResult.values.length > 0 ? sampleData(soundResult.values, 12) : [];
      analysisSoundChart.setOption(makeMobileLineOption(sSound, sSoundV, '分贝 (dB)', '#909399'));
    }
    // 光照强度图表
    if (analysisLuxChartRef.value) {
      analysisLuxChart = echarts.init(analysisLuxChartRef.value);
      const luxResult = luxData.status === 'fulfilled' ? luxData.value : { times: [], values: [] };
      const sLux = luxResult.times.length > 0 ? sampleData(luxResult.times, 12) : [];
      const sLuxV = luxResult.values.length > 0 ? sampleData(luxResult.values, 12) : [];
      analysisLuxChart.setOption(makeMobileLineOption(sLux, sLuxV, '光照 (Lux)', '#67c23a'));
    }
    // 紫外线图表
    if (analysisUvChartRef.value) {
      analysisUvChart = echarts.init(analysisUvChartRef.value);
      const uvResult = uvData.status === 'fulfilled' ? uvData.value : { times: [], values: [] };
      const sUv = uvResult.times.length > 0 ? sampleData(uvResult.times, 12) : [];
      const sUvV = uvResult.values.length > 0 ? sampleData(uvResult.values, 12) : [];
      analysisUvChart.setOption(makeMobileLineOption(sUv, sUvV, 'UV指数', '#f56c6c'));
    }
  }
};

// 组件挂载时初始化
onMounted(async () => {
  // 初始化移动端检测
  checkMobileStatus();
  window.addEventListener('resize', handleResize);

  await fetchDevices();

  const latestDevices = getDevices();

  // 启动帧率计算定时器（每0.5秒计算一次）
  fpsUpdateInterval = window.setInterval(calculateAverageFps, 500);

  setTimeout(async () => {
    const currentDevices = getDevices();

    if (currentDevices.length > 0) {
      if (activeTab.value === 'realtime') {
        if (!selectedBirdcageKey.value) {
          await autoSelectFirstOnlineDevice();
        } else {
          await fetchRealtimeData();
          startRealtimeDataTimer();
          startEnvDataTimer();
          nextTick(() => {
            initGaugeCharts();
          });
        }
      } else if (activeTab.value === 'history' && !historyBirdcageKey.value) {
        if (birdcageGroups.value.length > 0) {
          const g = birdcageGroups.value[0]!;
          historyBirdcageKey.value = `${g.area}|${g.number}`;
          if (historyCamDeviceId.value) {
            if (isMobile.value) {
              await getOrUpdateDeviceHistoryData(historyCamDeviceId.value);
            } else {
              await fetchDeviceHistoryData(historyCamDeviceId.value);
            }
          }
          if (historyC3DeviceId.value) {
            await fetchHistoryC3Data(historyC3DeviceId.value);
          }
        }
      } else if (activeTab.value === 'analysis') {
        // 确保 analysisBirdcageKey 已设置后再初始化图表
        if (!analysisBirdcageKey.value && birdcageGroups.value.length > 0) {
          const g = birdcageGroups.value[0]!;
          analysisBirdcageKey.value = `${g.area}|${g.number}`;
        }
        if (isMobile.value) {
          await getOrUpdateDeviceHistoryData();
        } else {
          await fetchDeviceHistoryData();
        }
        // 历史数据获取完成后初始化图表
        nextTick(async () => {
          await initCharts();
        });
      }
    }
  }, 1000);

  // 如果在分析标签页，初始化图表（等待 watch 和 setTimeout 完成后再初始化）
  if (activeTab.value === 'analysis') {
    // 延迟初始化，确保 analysisBirdcageKey 和设备数据都已就绪
    setTimeout(async () => {
      if (analysisBirdcageKey.value) {
        await initCharts();
      }
    }, 1200);
  }
});

// 组件卸载时清理
onUnmounted(() => {
  window.removeEventListener('resize', handleResize);
  stopRealtimeMonitoring();
  stopRealtimeDataTimer();
  stopEnvDataTimer();
  // 清理帧率计算定时器
  if (fpsUpdateInterval) {
    clearInterval(fpsUpdateInterval);
    fpsUpdateInterval = null;
  }
  temperatureChart?.dispose();
  humidityChart?.dispose();
  analysisAqiChart?.dispose();
  analysisSoundChart?.dispose();
  analysisLuxChart?.dispose();
  analysisUvChart?.dispose();
  aqiGaugeChart?.dispose();
  dbGaugeChart?.dispose();
  aqiGaugeChartMobile?.dispose();
  dbGaugeChartMobile?.dispose();
  luxGaugeChart?.dispose();
  uvGaugeChart?.dispose();
  luxGaugeChartMobile?.dispose();
  uvGaugeChartMobile?.dispose();
});

// 监听activeTab变化，重置设备选择
watch(() => props.activeTab, async (newTab, oldTab) => {
  if (newTab === 'analysis') {
    if (!analysisBirdcageKey.value && birdcageGroups.value.length > 0) {
      const g = birdcageGroups.value[0]!;
      analysisBirdcageKey.value = `${g.area}|${g.number}`;
    }
    nextTick(async () => {
      await initCharts();
    });
    stopRealtimeMonitoring();
    stopRealtimeDataTimer();
  } else if (newTab === 'history') {
    stopRealtimeDataTimer();
    fetchDevices().then(async () => {
      if (birdcageGroups.value.length > 0 && !historyBirdcageKey.value) {
        const g = birdcageGroups.value[0]!;
        historyBirdcageKey.value = `${g.area}|${g.number}`;
        
        if (historyCamDeviceId.value) {
          if (isMobile.value) {
            await getOrUpdateDeviceHistoryData(historyCamDeviceId.value);
          } else {
            await fetchDeviceHistoryData(historyCamDeviceId.value);
          }
        }
        if (historyC3DeviceId.value) {
          await fetchHistoryC3Data(historyC3DeviceId.value);
        }
      }
    });
    stopRealtimeMonitoring();
  } else if (newTab === 'realtime') {
    fetchDevices().then(async () => {
      const latestDevices = getDevices();
      const currentDevice = latestDevices.find((d: any) => d.id === selectedDeviceId.value);
      const isCurrentDeviceOnline = currentDevice?.isOnline ?? false;
      
      if (!selectedDeviceId.value || !isCurrentDeviceOnline) {
          await autoSelectFirstOnlineDevice();
        } else {
          await fetchRealtimeData();
          startRealtimeDataTimer();
          startEnvDataTimer();
          nextTick(() => {
            initGaugeCharts();
          });
        }
      
      if (selectedDeviceId.value) {
        nextTick(() => {
          startRealtimeMonitoring();
        });
      }
    });
  } else {
    selectedDeviceId.value = null;
    historyBirdcageKey.value = '';
    stopRealtimeMonitoring();
  }
}, { immediate: true });

// 监听selectedDeviceId变化，更新设备数据
watch(() => selectedDeviceId.value, async (newDeviceId, oldDeviceId) => {
  if (newDeviceId && activeTab.value === 'realtime') {
    await fetchDevices();
    if (!isMobile.value) {
      const sensorData = await fetchSensorData(newDeviceId);
      if (sensorData.temperatureValues.length > 0 && sensorData.humidityValues.length > 0) {
        const latestTemp = sensorData.temperatureValues[sensorData.temperatureValues.length - 1];
        const latestHumidity = sensorData.humidityValues[sensorData.humidityValues.length - 1];
        updateDevice(newDeviceId, { temperature: latestTemp, humidity: latestHumidity });
      }
    }
  }
});

// 监听路由参数变化，自动选择设备
watch(() => route.query.deviceId, (newDeviceId) => {
  if (newDeviceId) {
    const deviceId = parseInt(newDeviceId as string);
    if (!isNaN(deviceId)) {
      selectedDeviceId.value = deviceId;
    }
  }
}, { immediate: true });

// 监听路由变化，确保在导航到数据页面时自动选择设备
watch(() => route.path, async (newPath) => {
  if (newPath === '/Data' || newPath === '/Stream') {
    setTimeout(async () => {
      const latestDevices = getDevices();

      if (latestDevices.length > 0) {
        if (activeTab.value === 'realtime') {
          const currentDevice = latestDevices.find((d: any) => d.id === selectedDeviceId.value);
          const isCurrentDeviceOnline = currentDevice?.isOnline ?? false;
          
          if (!selectedDeviceId.value || !isCurrentDeviceOnline) {
            await autoSelectFirstOnlineDevice();
          }
        } else if (!selectedDeviceId.value) {
          selectedDeviceId.value = latestDevices[0]?.id || null;
        }
      }
    }, 500);
  }
});

// 监听selectedDeviceId变化，自动开启实时监控
watch(() => selectedDeviceId.value, (newDeviceId, oldDeviceId) => {
  nextTick(() => {
    if (newDeviceId && activeTab.value === 'realtime') {
      startRealtimeMonitoring();
    }
  });
});

// 跳转到总览界面的大图模式全屏状态
const goToFullscreen = () => {
  router.push({
    path: '/Home',
    query: { fullscreen: 'true' }
  });
};
</script>

<style lang="scss" scoped>
/* ==================== 桌面端样式 ==================== */
.data-card {
  z-index: 1;
  width: 75%;
  margin: 1% auto;
  border-radius: 15px;
  background-color: #fff;
  padding: 30px;
  min-height: auto;
}

.title {
  text-align: center;
  margin-bottom: 20px;
}

.title h2 {
  margin: 0;
  color: #303133;
  font-size: 20px;
}

.data-content {
  min-height: 0;
}

.realtime-container {
  display: flex;
  gap: 20px;
  min-height: 0;
}

.data-left {
  flex: 0 0 30%;
  min-height: 0;
}

.data-right {
  flex: 1;
  min-height: 0;
}

.device-selector {
  margin-bottom: 20px;
}

.device-selector h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.device-data h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.data-card-content {
  border-radius: 15px;
  min-height: 0;
}

.data-item {
  margin-bottom: 15px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 5px 0;
}

.data-label {
  color: #909399;
  font-size: 14px;
}

.data-value {
  color: #303133;
  font-size: 14px;
  font-weight: 500;
}

.data-value.online {
  color: #67c23a;
}

.data-value.offline {
  color: #f56c6c;
}

.data-actions {
  margin-top: 20px;
  text-align: center;
  padding-top: 15px;
  border-top: 1px solid #e4e7ed;
}

.data-actions .el-button {
  width: 100%;
}

/* 优化后的设备数据展示样式 */
.data-card-content.compact {
  padding: 15px;
  background: #f8f9fa;
  border-radius: 10px;
}

.data-row {
  display: flex;
  gap: 10px;
  margin-bottom: 12px;
}

.data-row:last-child {
  margin-bottom: 0;
}

.data-row.full {
  flex-direction: column;
}

.data-cell {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 10px;
  background: #fff;
  border-radius: 8px;
  box-shadow: 0 1px 3px rgba(0, 0, 0, 0.05);
}

.data-cell .el-icon {
  color: #909399;
  font-size: 14px;
}

.cell-label {
  color: #909399;
  font-size: 12px;
  min-width: 36px;
}

.cell-value {
  color: #303133;
  font-size: 13px;
  font-weight: 500;
  flex: 1;
  text-align: right;
}

.cell-value.highlight {
  color: #409eff;
  font-size: 14px;
}

.cell-value.time {
  font-size: 11px;
  color: #606266;
}

/* 环境数据环形进度条 */
.env-gauges {
  margin-top: 20px;
}

.env-gauges h3 {
  margin: 0 0 14px 0;
  font-size: 16px;
  font-weight: 600;
  color: #2d5016;
}

.gauges-row {
  display: flex;
  gap: 16px;
  justify-content: center;
}

.gauge-item {
  flex: 1;
  max-width: 180px;
  display: flex;
  flex-direction: column;
  align-items: center;
  background: rgba(255, 255, 255, 0.45);
  backdrop-filter: blur(20px);
  -webkit-backdrop-filter: blur(20px);
  border: 1px solid rgba(139, 173, 66, 0.2);
  border-radius: 20px;
  padding: 20px 16px 16px;
  box-shadow: 0 8px 32px rgba(139, 173, 66, 0.08), inset 0 1px 0 rgba(255, 255, 255, 0.6);
  transition: all 0.3s ease;
}

.gauge-item:hover {
  transform: translateY(-2px);
  box-shadow: 0 12px 40px rgba(139, 173, 66, 0.12), inset 0 1px 0 rgba(255, 255, 255, 0.8);
}

.gauge-chart {
  width: 130px;
  height: 130px;
}

.gauge-label {
  font-size: 13px;
  font-weight: 500;
  color: #4a6741;
  margin-top: 2px;
  letter-spacing: 0.5px;
}

.gauge-value {
  font-size: 16px;
  font-weight: 700;
  color: #2d5016;
  margin-top: 4px;
  transition: color 0.3s ease;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.05);
}

.gauge-value.alert {
  color: #dc2626;
}

/* 视频控制区域样式 */
.video-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
}

.video-header h3 {
  margin: 0;
  font-size: 16px;
  color: #303133;
}

.video-controls {
  display: flex;
  align-items: center;
  gap: 10px;
}

.quality-control {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-left: 10px;
  padding-left: 10px;
  border-left: 1px solid #e4e7ed;
}

.quality-label {
  font-size: 13px;
  color: #606266;
  white-space: nowrap;
}

/* 视频翻转样式 */
.video-stream.flip-horizontal {
  transform: scaleX(-1);
}

.video-stream.flip-vertical {
  transform: scaleY(-1);
}

.video-stream.flip-horizontal.flip-vertical {
  transform: scaleX(-1) scaleY(-1);
}

.analysis-content {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 20px;
}

@media (max-width: 768px) {
  .analysis-content {
    grid-template-columns: 1fr;
  }
}

.chart-container {
  margin-bottom: 0;
}

.chart-container h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.chart-container .time-range {
  font-size: 12px;
  color: #909399;
  font-weight: normal;
  margin-left: 8px;
}

.chart {
  width: 100%;
  height: 300px;
  border-radius: 10px;
  background-color: #f9f9f9;
}

.video-card {
  border-radius: 15px;
  min-height: 400px;
  display: flex;
  flex-direction: column;
}

.video-container {
  flex: 1;
  background: #f5f7fa;
  border-radius: 10px;
  overflow: hidden;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-height: 400px;
  position: relative;
}

.video-stream {
  max-width: 100%;
  max-height: 100%;
  width: auto;
  height: auto;
  object-fit: contain;
  flex: 1;
}

.frame-info {
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
  background: rgba(0, 0, 0, 0.6);
  color: white;
  padding: 8px 12px;
  font-size: 12px;
  text-align: right;
  z-index: 10;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.fps-info {
  font-weight: bold;
  color: #409EFF;
}

.video-stream.disconnected {
  filter: blur(5px);
}

.stream-disconnected-overlay {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  background: rgba(0, 0, 0, 0.5);
  backdrop-filter: blur(5px);
  display: flex;
  align-items: center;
  justify-content: center;
  z-index: 20;
}

.disconnected-content {
  text-align: center;
  color: white;
}

.disconnected-icon {
  color: #f56c6c;
  margin-bottom: 10px;
}

.disconnected-message {
  font-size: 16px;
  font-weight: 500;
}

.video-placeholder {
  flex: 1;
  background: #f5f7fa;
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 0;
}

.video-error {
  flex: 1;
  background: #f5f7fa;
  border-radius: 10px;
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 0;
}

.error-content {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  gap: 16px;
}

.error-icon {
  color: #f56c6c;
}

.error-message {
  font-size: 16px;
  color: #606266;
  text-align: center;
}

.tab-content {
  min-height: 400px;
  background: #f5f7fa;
  border-radius: 15px;
  margin-top: 20px;
  padding: 20px;
}

/* 时间轴筛选器 */
.time-range-selector {
  margin-bottom: 16px;
  margin-top: 12px;
  padding: 0;
  background: transparent;
  border-radius: 0;
  box-shadow: none;
}

.time-buttons-container {
  display: flex;
  gap: 12px;
  align-items: center;
}

.time-button {
  font-size: 14px;
  font-weight: 500;
  color: #606266;
  background: #f5f7fa;
  border: 1px solid #e4e7ed;
  border-radius: 8px;
  padding: 8px 16px;
  cursor: pointer;
  transition: all 0.2s ease;
  white-space: nowrap;
  min-width: 60px;
  text-align: center;
}

.time-button:hover {
  color: #409eff;
  border-color: #c6e2ff;
  background: #ecf5ff;
  transform: translateY(-1px);
  box-shadow: 0 2px 4px rgba(0, 0, 0, 0.08);
}

.time-button.active {
  color: #fff;
  background: linear-gradient(135deg, #409eff 0%, #66b1ff 100%);
  border-color: #409eff;
  box-shadow: 0 3px 6px rgba(64, 158, 255, 0.3);
  transform: translateY(-1px);
}

.time-button.active:hover {
  background: linear-gradient(135deg, #66b1ff 0%, #409eff 100%);
  box-shadow: 0 4px 8px rgba(64, 158, 255, 0.4);
}

/* 移动端保持原有滑块样式 */
.time-range-selector.mobile {
  margin-bottom: 20px;
  padding: 16px 20px;
  background: #fff;
  border-radius: 12px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.06);
}

.time-range-selector.mobile .time-slider-container {
  position: relative;
}

.time-range-selector.mobile .time-labels {
  display: flex;
  justify-content: space-between;
  margin-bottom: 12px;
}

.time-range-selector.mobile .time-label {
  font-size: 14px;
  font-weight: 500;
  color: #909399;
  cursor: pointer;
  padding: 6px 16px;
  border-radius: 20px;
  transition: all 0.3s ease;
}

.time-range-selector.mobile .time-label:hover {
  color: #409eff;
  background: rgba(64, 158, 255, 0.1);
}

.time-range-selector.mobile .time-label.active {
  color: #fff;
  background: linear-gradient(135deg, #409eff 0%, #66b1ff 100%);
  box-shadow: 0 2px 8px rgba(64, 158, 255, 0.3);
}

.time-range-selector.mobile .slider-track {
  position: relative;
  height: 6px;
  background: #e4e7ed;
  border-radius: 3px;
  margin: 0 20px;
}

.time-range-selector.mobile .slider-progress {
  position: absolute;
  left: 0;
  top: 0;
  height: 100%;
  background: linear-gradient(90deg, #409eff 0%, #66b1ff 100%);
  border-radius: 3px;
  transition: width 0.3s ease;
}

.time-range-selector.mobile .slider-thumb {
  position: absolute;
  top: 50%;
  width: 16px;
  height: 16px;
  background: #fff;
  border: 3px solid #409eff;
  border-radius: 50%;
  transform: translate(-50%, -50%);
  box-shadow: 0 2px 6px rgba(64, 158, 255, 0.4);
  transition: left 0.3s ease;
  cursor: pointer;
}

.time-range-selector.mobile .slider-thumb:hover {
  transform: translate(-50%, -50%) scale(1.2);
  box-shadow: 0 4px 10px rgba(64, 158, 255, 0.5);
}

.tab-content .el-card {
  background: #fff;
  border-radius: 15px;
}

.card-header {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 10px;
}

.card-title span {
  white-space: nowrap;
  font-size: 16px;
  font-weight: 500;
}

.card-actions {
  width: 100%;
}

.history-card {
  width: 100%;
}

.history-content {
  display: flex;
  flex-direction: column;
  gap: 20px;
}

.top-section {
  display: flex;
  gap: 20px;
  align-items: flex-start;
}

.device-selector {
  flex: 1;
  min-width: 200px;
}

.device-selector h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.device-select {
  width: 100%;
}

.device-info {
  flex: 1;
  min-width: 300px;
}

.device-info h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.info-card {
  border-radius: 10px;
}

.info-item {
  margin-bottom: 10px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 5px 0;
}

.info-label {
  color: #909399;
  font-size: 14px;
}

.info-value {
  color: #303133;
  font-size: 14px;
  font-weight: 500;
}

.info-value.online {
  color: #67c23a;
}

.info-value.offline {
  color: #f56c6c;
}

.history-table {
  margin-top: 20px;
}

.history-table h3 {
  margin: 0 0 10px 0;
  font-size: 16px;
  color: #303133;
}

.pagination-container {
  margin-top: 20px;
  display: flex;
  justify-content: center;
}

.history-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 300px;
}

/* ==================== 移动端样式 ==================== */
.mobile-data {
  min-height: calc(100vh - 120px);
  background: #f5f7fa;
  display: flex;
  flex-direction: column;
}

/* 设备选择 */
.mobile-data .device-selector {
  margin-bottom: 16px;
}

/* 实时数据 */
.mobile-data .realtime-container {
  padding: 0;
}

/* 设备数据卡片 */
.mobile-data .device-data-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  margin-bottom: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.mobile-data .data-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 16px;
}

.mobile-data .data-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.mobile-data .data-grid {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
}

.mobile-data .data-item {
  padding: 12px;
  background: #f5f7fa;
  border-radius: 8px;
}

.mobile-data .data-item.full {
  grid-column: 1 / -1;
}

.mobile-data .data-label {
  font-size: 12px;
  color: #909399;
  margin-bottom: 4px;
}

.mobile-data .data-value {
  font-size: 18px;
  font-weight: 600;
  color: #303133;
}

.mobile-data .data-value.small {
  font-size: 14px;
}

/* 移动端环境数据环形进度条 */
.mobile-data .mobile-gauges {
  margin-bottom: 16px;
}

.mobile-data .mobile-gauges-row {
  display: flex;
  gap: 12px;
}

.mobile-data .mobile-gauge-item {
  flex: 1;
  display: flex;
  flex-direction: column;
  align-items: center;
  background: rgba(255, 255, 255, 0.5);
  backdrop-filter: blur(16px);
  -webkit-backdrop-filter: blur(16px);
  border: 1px solid rgba(139, 173, 66, 0.18);
  border-radius: 16px;
  padding: 14px 8px 10px;
  box-shadow: 0 4px 20px rgba(139, 173, 66, 0.06), inset 0 1px 0 rgba(255, 255, 255, 0.5);
}

.mobile-data .mobile-gauge-chart {
  width: 100px;
  height: 100px;
}

.mobile-data .mobile-gauge-label {
  font-size: 12px;
  font-weight: 500;
  color: #4a6741;
  margin-top: 2px;
  letter-spacing: 0.3px;
}

.mobile-data .mobile-gauge-value {
  font-size: 14px;
  font-weight: 700;
  color: #2d5016;
  margin-top: 2px;
  transition: color 0.3s ease;
  text-shadow: 0 1px 2px rgba(0, 0, 0, 0.05);
}

.mobile-data .mobile-gauge-value.alert {
  color: #dc2626;
}

.mobile-data .mobile-gauges-row-second {
  margin-top: 12px;
}

/* 视频监控 */
.mobile-data .video-section {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.mobile-data .video-header {
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 12px;
  margin-bottom: 12px;
  width: 100%;
}

.mobile-data .video-header-top {
  display: flex;
  justify-content: space-between;
  align-items: center;
  width: 100%;
}

.mobile-data .video-header h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.mobile-data .video-controls {
  display: flex;
  gap: 8px;
}

.mobile-data .video-settings {
  width: 100%;
  display: flex;
  flex-direction: column;
  gap: 12px;
  padding: 12px;
  background: #f5f7fa;
  border-radius: 8px;
}

.mobile-data .setting-item {
  display: flex;
  align-items: center;
  gap: 8px;
}

.mobile-data .setting-label {
  font-size: 14px;
  color: #606266;
  min-width: 60px;
}

.mobile-data .setting-value {
  font-size: 14px;
  color: #409EFF;
  font-weight: 600;
  min-width: 30px;
  text-align: right;
}

.mobile-data .video-card {
  position: relative;
  width: 100%;
  background: #000;
  border-radius: 8px;
  overflow: hidden;
}

.mobile-data .video-container {
  position: relative;
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: center;
  background: #000;
}

.mobile-data .video-stream {
  max-width: 100%;
  max-height: 70vh;
  width: auto;
  height: auto;
  object-fit: contain;
  display: block;
}

.mobile-data .video-stream.flip-horizontal {
  transform: scaleX(-1);
}

.mobile-data .video-stream.flip-vertical {
  transform: scaleY(-1);
}

.mobile-data .video-stream.disconnected {
  opacity: 0.5;
}

.mobile-data .frame-info {
  position: absolute;
  bottom: 8px;
  left: 8px;
  right: 8px;
  background: rgba(0, 0, 0, 0.6);
  color: white;
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 12px;
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.mobile-data .fps-info {
  font-weight: bold;
  color: #409EFF;
}

.mobile-data .stream-disconnected-overlay {
  position: absolute;
  top: 0;
  left: 0;
  right: 0;
  bottom: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  background: rgba(0, 0, 0, 0.7);
}

.mobile-data .disconnected-content {
  text-align: center;
  color: white;
}

.mobile-data .disconnected-icon {
  color: #f56c6c;
}

.mobile-data .disconnected-message {
  margin-top: 8px;
  font-size: 14px;
}

.mobile-data .video-error {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
  background: #f5f7fa;
}

.mobile-data .error-content {
  text-align: center;
}

.mobile-data .error-icon {
  color: #f56c6c;
}

.mobile-data .error-message {
  margin-top: 8px;
  color: #606266;
}

.mobile-data .video-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
}

/* 分析 */
.mobile-data .analysis-container {
  padding: 0;
}

.mobile-data .time-range-selector {
  margin: 16px 0;
  padding: 12px 16px;
}

.mobile-data .time-range-selector .time-label {
  font-size: 13px;
  padding: 5px 12px;
}

.mobile-data .time-range-selector .slider-track {
  margin: 0 12px;
}

.mobile-data .chart-section {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.mobile-data .chart-card {
  background: white;
  border-radius: 12px;
  padding: 16px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.08);
}

.mobile-data .chart-card h3 {
  margin: 0 0 12px 0;
  font-size: 16px;
  font-weight: 600;
  color: #303133;
}

.mobile-data .chart-card h3 .time-range {
  font-size: 12px;
  font-weight: 400;
  color: #909399;
  margin-left: 8px;
}

.mobile-data .chart {
  width: 100%;
  height: 200px;
}

/* 历史数据 - 移动端 */
.mobile-data .history-container {
  padding: 0 12px;
}

.mobile-data .history-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

/* 鸟笼信息卡片 */
.mobile-data .birdcage-info-card {
  background: linear-gradient(135deg, rgba(139, 173, 66, 0.08) 0%, rgba(106, 154, 53, 0.05) 100%);
  backdrop-filter: blur(16px);
  border-radius: 20px;
  padding: 20px;
  border: 1px solid rgba(139, 173, 66, 0.15);
  box-shadow: 0 4px 16px rgba(139, 173, 66, 0.08);
}

.mobile-data .birdcage-header {
  display: flex;
  align-items: center;
  gap: 10px;
  margin-bottom: 14px;
}

.mobile-data .birdcage-icon {
  font-size: 24px;
}

.mobile-data .birdcage-name {
  font-size: 18px;
  font-weight: 700;
  color: #166534;
}

.mobile-data .birdcage-devices {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

.mobile-data .device-chip {
  display: inline-flex;
  align-items: center;
  gap: 6px;
  padding: 6px 12px;
  border-radius: 50px;
  font-size: 12px;
  background: rgba(255, 255, 255, 0.7);
  border: 1px solid rgba(0, 0, 0, 0.06);
  transition: all 0.3s ease;
}

.mobile-data .device-chip.online {
  border-color: rgba(16, 185, 129, 0.3);
  background: rgba(16, 185, 129, 0.08);
}

.mobile-data .device-chip.offline {
  border-color: rgba(239, 68, 68, 0.2);
  background: rgba(239, 68, 68, 0.05);
}

.mobile-data .device-chip .chip-dot {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background: #9ca3af;
}

.mobile-data .device-chip.online .chip-dot {
  background: #10b981;
  box-shadow: 0 0 6px rgba(16, 185, 129, 0.4);
}

.mobile-data .device-chip.offline .chip-dot {
  background: #ef4444;
  box-shadow: 0 0 6px rgba(239, 68, 68, 0.3);
}

.mobile-data .device-chip .chip-label {
  font-weight: 600;
  color: #4b5563;
}

.mobile-data .device-chip .chip-name {
  color: #6b7280;
  max-width: 100px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

/* 历史数据区域 */
.mobile-data .history-data-section {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.mobile-data .history-loading {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 12px;
  padding: 40px 20px;
  color: #6b7280;
  font-size: 14px;
}

.mobile-data .history-loading .loading-spinner {
  width: 32px;
  height: 32px;
  border: 3px solid rgba(139, 173, 66, 0.2);
  border-top: 3px solid #8BAD42;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}

.mobile-data .history-empty {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 8px;
  padding: 40px 20px;
  color: #9ca3af;
}

.mobile-data .history-empty .empty-icon {
  font-size: 48px;
  opacity: 0.6;
}

.mobile-data .history-empty .empty-text {
  font-size: 14px;
}

/* 历史数据卡片 */
.mobile-data .history-cards {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.mobile-data .history-data-card {
  background: rgba(255, 255, 255, 0.85);
  backdrop-filter: blur(12px);
  border-radius: 16px;
  padding: 16px;
  border: 1px solid rgba(255, 255, 255, 0.6);
  box-shadow: 0 2px 12px rgba(0, 0, 0, 0.06);
  transition: all 0.3s ease;
}

.mobile-data .history-data-card:active {
  transform: scale(0.98);
  box-shadow: 0 1px 6px rgba(0, 0, 0, 0.04);
}

.mobile-data .card-time {
  display: flex;
  align-items: center;
  gap: 6px;
  margin-bottom: 12px;
  padding-bottom: 10px;
  border-bottom: 1px solid rgba(0, 0, 0, 0.04);
}

.mobile-data .card-time .time-icon {
  font-size: 14px;
}

.mobile-data .card-time .time-text {
  font-size: 13px;
  font-weight: 600;
  color: #4b5563;
  letter-spacing: 0.3px;
}

/* 传感器网格 */
.mobile-data .sensor-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 8px;
}

.mobile-data .sensor-cell {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 3px;
  padding: 10px 4px;
  border-radius: 12px;
  background: rgba(255, 255, 255, 0.6);
  border: 1px solid rgba(0, 0, 0, 0.04);
  transition: all 0.2s ease;
}

.mobile-data .sensor-cell.temp {
  background: #fee2e2;
  border-color: transparent;
}

.mobile-data .sensor-cell.humidity {
  background: #dbeafe;
  border-color: transparent;
}

.mobile-data .sensor-cell.aqi {
  background: #fef3c7;
  border-color: transparent;
}

.mobile-data .sensor-cell.sound {
  background: #f3f4f6;
  border-color: transparent;
}

.mobile-data .sensor-cell.lux {
  background: #ecfdf5;
  border-color: transparent;
}

.mobile-data .sensor-cell.uv {
  background: #fee2e2;
  border-color: transparent;
}

.mobile-data .sensor-cell .sensor-icon {
  display: flex;
  align-items: center;
  justify-content: center;
  line-height: 1;
}

.mobile-data .sensor-cell .sensor-icon svg {
  width: 20px;
  height: 20px;
}

.mobile-data .sensor-cell.temp .sensor-icon {
  color: #ef4444;
}

.mobile-data .sensor-cell.humidity .sensor-icon {
  color: #3b82f6;
}

.mobile-data .sensor-cell.aqi .sensor-icon {
  color: #f59e0b;
}

.mobile-data .sensor-cell.sound .sensor-icon {
  color: #6b7280;
}

.mobile-data .sensor-cell.lux .sensor-icon {
  color: #10b981;
}

.mobile-data .sensor-cell.uv .sensor-icon {
  color: #f97316;
}

.mobile-data .sensor-cell .sensor-value {
  font-size: 16px;
  font-weight: 700;
  color: #303133;
  line-height: 1.2;
}

.mobile-data .sensor-cell .sensor-unit {
  font-size: 10px;
  color: #9ca3af;
  font-weight: 500;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.mobile-data .pagination-container {
  margin-top: 8px;
  display: flex;
  justify-content: center;
  padding: 0 4px;
  overflow-x: auto;
}

.mobile-data .pagination-container :deep(.el-pagination) {
  --el-pagination-button-width: 28px;
  --el-pagination-button-height: 28px;
  font-size: 12px;
}

.mobile-data .pagination-container :deep(.el-pagination .el-select .el-input) {
  width: 80px;
}

.mobile-data .history-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  min-height: 200px;
}

/* ==================== 响应式适配 ==================== */
@media (max-width: 768px) {
  .data-card {
    width: 95%;
    padding: 20px;
  }
  
  .realtime-container {
    flex-direction: column;
  }
  
  .data-left,
  .data-right {
    flex: none;
    width: 100%;
  }
  
  .video-container,
  .video-placeholder {
    height: 300px;
  }
}

/* ==================== 鸟笼设备信息样式 ==================== */
.birdcage-device-list {
  background: #f8f9fa;
  border-radius: 10px;
  padding: 12px 16px;
  margin-bottom: 16px;
}

.device-info-row {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 6px 0;
  flex-wrap: wrap;
}

.device-info-row + .device-info-row {
  border-top: 1px solid #e8e8e8;
  margin-top: 6px;
  padding-top: 10px;
}

.device-type-tag {
  display: inline-flex;
  align-items: center;
  justify-content: center;
  width: 36px;
  height: 22px;
  border-radius: 6px;
  font-size: 11px;
  font-weight: 700;
  color: #fff;
  letter-spacing: 0.5px;
  flex-shrink: 0;
}

.device-type-tag.cam-tag {
  background: linear-gradient(135deg, #8BAD42 0%, #6A9A35 100%);
}

.device-type-tag.c3-tag {
  background: linear-gradient(135deg, #60a5fa 0%, #3b82f6 100%);
}

.device-info-item {
  font-size: 13px;
  color: #303133;
}

.device-info-item.name {
  font-weight: 600;
  color: #166534;
}

.device-info-item.type {
  color: #909399;
  font-size: 12px;
}

.device-info-sep {
  color: #dcdfe6;
  font-size: 13px;
}

/* 温度湿度卡片 - 扁平化设计 */
.temp-humid-cards {
  display: flex;
  gap: 12px;
}

.th-card {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 14px;
  padding: 18px 20px;
  border-radius: 8px;
  border: none;
  box-shadow: none;
  transition: background-color 0.2s ease;
}

.th-card:hover {
  filter: brightness(0.95);
}

.th-card.temp-card {
  background: #fee2e2;
  border-color: transparent;
}

.th-card.humid-card {
  background: #dbeafe;
  border-color: transparent;
}

.th-card-icon {
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #ef4444;
}

.th-card.humid-card .th-card-icon {
  color: #3b82f6;
}

.th-card-icon svg {
  width: 32px;
  height: 32px;
}

.th-card-body {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.th-card-label {
  font-size: 13px;
  color: #606266;
  font-weight: 500;
}

.th-card-value {
  font-size: 28px;
  font-weight: 700;
  color: #303133;
  line-height: 1.1;
}

.th-card-value small {
  font-size: 16px;
  font-weight: 500;
  color: #606266;
  margin-left: 2px;
}

.th-card.temp-card .th-card-value {
  color: #ef4444;
}

.th-card.humid-card .th-card-value {
  color: #3b82f6;
}

/* 移动端鸟笼设备行 */
.mobile-device-row {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 0;
  flex-wrap: wrap;
}

.mobile-device-row + .mobile-device-row {
  border-top: 1px solid #f0f0f0;
}

.mobile-device-info {
  font-size: 13px;
  color: #303133;
  flex: 1;
  min-width: 0;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

/* 移动端温度湿度行 - 扁平化设计 */
.mobile-th-row {
  display: flex;
  gap: 12px;
  margin-top: 14px;
}

.mobile-th-item {
  flex: 1;
  display: flex;
  align-items: center;
  gap: 10px;
  padding: 14px 16px;
  border-radius: 8px;
  border: none;
}

.mobile-th-item.temp {
  background: #fee2e2;
  border-color: transparent;
}

.mobile-th-item.humid {
  background: #dbeafe;
  border-color: transparent;
}

.mobile-th-icon {
  flex-shrink: 0;
  display: flex;
  align-items: center;
  justify-content: center;
  color: #ef4444;
}

.mobile-th-item.humid .mobile-th-icon {
  color: #3b82f6;
}

.mobile-th-icon svg {
  width: 28px;
  height: 28px;
}

.mobile-th-body {
  display: flex;
  flex-direction: column;
  gap: 2px;
}

.mobile-th-label {
  font-size: 12px;
  color: #606266;
  font-weight: 500;
}

.mobile-th-value {
  font-size: 22px;
  font-weight: 700;
  line-height: 1.1;
}

.mobile-th-item.temp .mobile-th-value {
  color: #ef4444;
}

.mobile-th-item.humid .mobile-th-value {
  color: #3b82f6;
}
</style>
