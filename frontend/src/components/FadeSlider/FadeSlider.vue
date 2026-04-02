<template>
  <div
    :class="$style['slider-container']"
    :style="containerStyle"
    data-testid="slider-root"
    @mouseleave="handleMouseLeave"
    @mouseenter="handleMouseEnter"
  >
    <div :class="$style['slider-handler']">
      <img :src="leftIcon" @click="handleLeft" />
      <img :src="leftIcon" :style="{ transform: 'rotate(180deg)' }" @click="handleRight" />
    </div>
    
    <div :class="$style['slider-item-container']">
      <div
        v-for="(m, i) in item"
        :key="`${m.title}${m.content}${m.src}`"
        :class="[
          $style['slider-item'],
          currentIndex === i ? $style['slider-item-active'] : ''
        ]"
        :style="itemStyle"
      >
        <div :class="$style['slider-item-layer']">
          <div :class="$style['slider-item-title']">
            <component :is="() => m.title" v-if="typeof m.title === 'function'" />
            <template v-else>{{ m.title }}</template>
          </div>
          <div :class="$style['slider-item-content']">
            <component :is="() => m.content" v-if="typeof m.content === 'function'" />
            <template v-else>{{ m.content }}</template>
          </div>
        </div>
        
        <div :class="$style['slider-img']">
          <div :class="$style['slider-mask']" />
          <img
            :src="m.src"
            :style="{
              ...imgStyle,
              '--scale-ratio': scaleRatio
            }"
          />
        </div>
        
        <img
          v-if="backgroundImage"
          :class="$style['slider-bg-img']"
          :src="m.src"
        />
      </div>
    </div>
    
    <div :class="$style['slider-dot-container']">
      <div
        v-for="(_, i) in item"
        :key="`slider-dot-${i}`"
        :class="$style['slider-dot']"
        :data-testid="`slider-dot-${i}`"
        @click="setCurrentIndex(i)"
      >
        <div
          :class="currentIndex === i ? $style['slider-dot-active'] : ''"
          :style="{
            '--ani-duration': `${interval / 1000 + 0.2}s`,
            '--running-state': isHover ? 'running' : 'paused'
          }"
        />
        <div :class="$style['slider-dot-bg']" />
      </div>
    </div>
    
  </div>
</template>

<script setup lang="ts">
import { ref, computed, watch, onMounted, onUnmounted } from 'vue';
import type { CSSProperties } from 'vue';
import type { ItemProps } from './types';
import leftIcon from './assets/left.svg';

interface Props {
  item: ItemProps[];
  style?: CSSProperties;
  itemStyle?: CSSProperties;
  imgStyle?: CSSProperties;
  backgroundImage?: boolean;
  autoplay?: boolean;
  interval?: number;
  pauseOnHover?: boolean;
  scaleRatio?: number;
  fullscreen?: boolean;
}

const props = withDefaults(defineProps<Props>(), {
  backgroundImage: true,
  autoplay: true,
  interval: 5000,
  pauseOnHover: true,
  scaleRatio: 1.15,
  fullscreen: false
});

const emit = defineEmits<{
  change: [index: number];
}>();

const currentIndex = ref<number | null>(null);
const isHover = ref(!props.pauseOnHover);
let timer: ReturnType<typeof setInterval> | null = null;

const containerStyle = computed<CSSProperties>(() => {
  if (props.fullscreen) {
    return {
      position: 'fixed',
      top: '0',
      left: '0',
      width: '100vw',
      height: '50vh',
      zIndex: 0,
      ...props.style
    };
  }
  return props.style || { height: '60vh' };
});

function handleStartInterval() {
  if (timer !== null) return;
  isHover.value = true;
  timer = setInterval(() => {
    handleRight();
  }, props.interval);
}

function handleStopInterval() {
  if (timer === null) return;
  isHover.value = false;
  clearInterval(timer);
  timer = null;
}

function handleLeft() {
  if (currentIndex.value === null) return;
  currentIndex.value = currentIndex.value === 0 ? props.item.length - 1 : currentIndex.value - 1;
}

function handleRight() {
  if (currentIndex.value === null) return;
  currentIndex.value = currentIndex.value === props.item.length - 1 ? 0 : currentIndex.value + 1;
}

function setCurrentIndex(index: number) {
  currentIndex.value = index;
}

function handleMouseLeave() {
  if (props.pauseOnHover && props.autoplay) handleStartInterval();
}

function handleMouseEnter() {
  if (props.pauseOnHover && props.autoplay) handleStopInterval();
}

watch(() => props.autoplay, (newAutoplay) => {
  if (newAutoplay) {
    handleStartInterval();
  } else {
    handleStopInterval();
  }
}, { immediate: true });

watch(() => props.interval, () => {
  if (props.autoplay) {
    handleStopInterval();
    handleStartInterval();
  }
});

watch(currentIndex, (newIndex) => {
  if (newIndex !== null) {
    emit('change', newIndex);
  }
});

onMounted(() => {
  currentIndex.value = 0;
});

onUnmounted(() => {
  handleStopInterval();
});
</script>

<style module lang="scss">
.slider-container {
  position: relative;
  width: 100%;
  height: 100%;
}

.slider-handler {
  position: absolute;
  z-index: 2;
  top: 50%;
  transform: translateY(-50%);
  width: 100%;
  display: flex;
  align-items: center;
  justify-content: space-between;

  > img {
    background-color: transparent;
    height: 30px;
    width: 30px;
    padding: 10px;
    border-radius: 50%;
    cursor: pointer;
    margin-inline: 30px;
    transition: all 0.2s;
    opacity: 0.3;

    &:hover {
      opacity: 1;
      background-color: #00000069;
    }
  }
}

.slider-item {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  overflow: hidden;
  visibility: hidden;

  > .slider-img {
    > img {
      width: 100%;
      height: 100%;
      object-fit: cover;
      object-position: top center;
      transform: scale(var(--scale-ratio));
      transition: 6.9s cubic-bezier(0.49, 0.09, 0.18, 0.91);
      transform-origin: 100%;
    }

    z-index: 1;
    opacity: 0;
    height: 100%;
    width: 100%;
    position: absolute;
    transition: all 1s ease-in-out;
  }

  &-layer {
    width: 50%;
    transform: translateX(3%);
    transition: transform 0.8s cubic-bezier(0.07, 0.26, 0.22, 0.77), opacity 0.5s ease-in-out;
    position: absolute;
    z-index: 4;
    margin: 70px 100px;
    color: white;
    opacity: 0;
  }

  &-title {
    display: flex;
    font-family: 'Courier New', Courier, monospace;
    font-size: 2.5rem;
    line-height: 1.5;
  }

  &-content {
    margin-top: 5px;
    line-height: 1.8;
    overflow-y: auto;
  }
}

.slider-item-active {
  visibility: visible;

  > .slider-item-layer {
    opacity: 1;
    transform: translateX(0%);
    filter: blur(0px);
  }

  > .slider-img {
    > img {
      transform: scale(1);
    }

    transform: scale(1);
    transform: translateX(0%);
    filter: blur(0px);
    opacity: 1;
  }

  .slider-bg-img {
    opacity: 1;
  }
}

.slider-bg-img {
  position: absolute;
  z-index: -1;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  object-fit: cover;
  object-position: top center;
  filter: blur(10px);
  opacity: 0;
}

.slider-mask {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  z-index: 2;
  transform: scale(1.1);
  background: linear-gradient(to right,
      rgba(0, 0, 0, 0.78) 0%,
      rgba(0, 0, 0, 0.78) 30%,
      rgba(0, 0, 0, 0.3) 100%);
  pointer-events: none;
}

.slider-dot {
  &-container {
    position: absolute;
    bottom: 0px;
    left: 50%;
    transform: translateX(-50%);
    z-index: 4;
    display: flex;
    align-items: center;
    gap: 5px;
  }

  cursor: pointer;
  width: 35px;
  height: 40px;
  position: relative;

  &-bg {
    top: 50%;
    width: 35px;
    height: 3px;
    background-color: #ffffff39;
    position: absolute;
  }

  &-active {
    top: 50%;
    width: 100%;
    height: 3px;
    animation-name: progress;
    animation-duration: var(--ani-duration);
    animation-timing-function: linear;
    background-color: #ffffff91;
    position: absolute;
    z-index: 5;
  }
}

@keyframes progress {
  from {
    width: 0;
  }

  to {
    width: 100%;
  }
}

@media screen and (max-width: 768px) {
  .slider-handler {
    display: none;
  }

  .slider-item-layer {
    margin: 10% 1.3rem;
    width: 90%;
  }
}
</style>
