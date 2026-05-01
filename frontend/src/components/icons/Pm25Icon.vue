<template>
  <svg viewBox="0 0 24 24" :width="size" :height="size" fill="currentColor" xmlns="http://www.w3.org/2000/svg">
    <path d="M12 2C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8zm-1-13h2v6h-2zm0 8h2v2h-2z"/>
    <circle cx="8" cy="10" r="1.5"/>
    <circle cx="16" cy="10" r="1"/>
    <circle cx="10" cy="15" r="1"/>
    <circle cx="14" cy="14" r="0.8"/>
  </svg>
</template>

<script setup lang="ts">
interface Props {
  size?: string | number
}

withDefaults(defineProps<Props>(), {
  size: '1em'
})
</script>
