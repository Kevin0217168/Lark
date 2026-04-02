# FadeSlider 组件使用文档

## 简介

FadeSlider 是一个 Vue 3 淡入淡出轮播组件，支持自动播放、手动切换、响应式布局等功能。

## 目录结构

```
FadeSlider/
├── assets/
│   ├── left.svg          # 左右切换箭头图标
│   └── pause.svg         # 暂停按钮图标
├── FadeSlider.vue        # 主组件文件
├── types.ts              # TypeScript 类型定义
├── index.ts              # 组件导出入口
└── README.md             # 使用文档
```

## 快速开始

### 基本使用

```vue
<template>
  <FadeSlider :item="sliderItems" />
</template>

<script setup lang="ts">
import FadeSlider from '@/components/FadeSlider';
import type { ItemProps } from '@/components/FadeSlider/types';

const sliderItems: ItemProps[] = [
  {
    title: '第一张轮播图',
    content: '这是第一张轮播图的描述内容',
    src: '/path/to/image1.jpg'
  },
  {
    title: '第二张轮播图',
    content: '这是第二张轮播图的描述内容',
    src: '/path/to/image2.jpg'
  }
];
</script>
```

## API 文档

### Props 属性

| 属性名 | 类型 | 默认值 | 说明 |
|--------|------|--------|------|
| item | ItemProps[] | - | 轮播项数据数组（必填） |
| style | CSSProperties | { height: '60vh' } | 容器样式 |
| itemStyle | CSSProperties | - | 轮播项样式 |
| imgStyle | CSSProperties | - | 图片样式 |
| backgroundImage | boolean | true | 是否显示背景模糊图 |
| autoplay | boolean | true | 是否自动播放 |
| interval | number | 5000 | 自动播放间隔（毫秒） |
| pauseOnHover | boolean | true | 鼠标悬停时是否暂停 |
| scaleRatio | number | 1.15 | 图片缩放比例 |

### Events 事件

| 事件名 | 参数 | 说明 |
|--------|------|------|
| change | index: number | 轮播图切换时触发，返回当前索引 |

### ItemProps 类型

```typescript
interface ItemProps {
  title: VNode | string;      // 标题，支持字符串或虚拟节点
  content: VNode | string;    // 内容，支持字符串或虚拟节点
  src: string;                 // 图片地址
}
```

## 高级用法

### 自定义标题和内容

```vue
<template>
  <FadeSlider :item="sliderItems" />
</template>

<script setup lang="ts">
import FadeSlider from '@/components/FadeSlider';
import { h } from 'vue';
import type { ItemProps } from '@/components/FadeSlider/types';

const sliderItems: ItemProps[] = [
  {
    title: h('span', { style: { color: '#ff0000' } }, '自定义标题'),
    content: h('div', [
      h('p', '这是第一段内容'),
      h('p', '这是第二段内容')
    ]),
    src: '/path/to/image.jpg'
  }
];
</script>
```

### 禁用自动播放

```vue
<template>
  <FadeSlider :item="sliderItems" :autoplay="false" />
</template>
```

### 自定义播放间隔

```vue
<template>
  <FadeSlider :item="sliderItems" :interval="3000" />
</template>
```

### 监听切换事件

```vue
<template>
  <FadeSlider :item="sliderItems" @change="handleChange" />
</template>

<script setup lang="ts">
const handleChange = (index: number) => {
  console.log('当前轮播索引:', index);
};
</script>
```

## 响应式设计

组件内置响应式支持：
- 桌面端显示左右切换箭头和暂停按钮
- 移动端（宽度 < 768px）隐藏左右切换箭头和暂停按钮
- 移动端标题和内容区域自适应调整

## 样式自定义

可以通过以下方式自定义样式：

1. 使用 `style`、`itemStyle`、`imgStyle` 属性
2. 使用 CSS 深度选择器修改内部样式

```vue
<template>
  <FadeSlider 
    :item="sliderItems" 
    :style="{ height: '400px' }"
  />
</template>
```

## 注意事项

1. 确保图片资源可访问
2. 建议至少提供 2 个轮播项
3. 组件会在卸载时自动清理定时器，防止内存泄漏
4. title 和 content 支持传入 VNode 以实现更复杂的渲染

## 示例

完整示例请参考项目中的示例组件。
