<template>
  <svg viewBox="0 0 24 24" :width="size" :height="size" fill="currentColor" xmlns="http://www.w3.org/2000/svg">
    <path d="M12 2C11.4477 2 11 2.44772 11 3V13.1707C9.31742 14.2206 8.25 16.0613 8.25 18C8.25 20.9653 10.6347 23.375 13.5 23.375C16.3653 23.375 18.75 20.9653 18.75 18C18.75 16.0613 17.6826 14.2206 16 13.1707V3C16 2.44772 15.5523 2 15 2H12ZM13.5 21.125C11.8782 21.125 10.5 19.6213 10.5 18C10.5 16.3787 11.8782 14.875 13.5 14.875C15.1218 14.875 16.5 16.3787 16.5 18C16.5 19.6213 15.1218 21.125 13.5 21.125Z"/>
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
