<template>
  <div class="mobile-beian">
    <div class="copyright">© 2026</div>
    <div class="beian">
      <div class="beian-item">
        <img src="https://www.beian.gov.cn/img/ghs.png" class="beian-icon">
        <a class="beian-link" href="https://beian.mps.gov.cn/#/query/webSearch?code=41010502007493" rel="noreferrer" target="_blank">豫公网安备41010502007493号</a>
      </div>
      <div class="beian-item">
        <span class="beian-placeholder"></span>
        <a class="beian-link" href="https://beian.miit.gov.cn/" target="_blank">豫ICP备2026008104号</a>
      </div>
      <div class="beian-item">
        <img src="https://icp.gov.moe/images/ico64.png" class="beian-icon moe-icon">
        <a class="beian-link" target="_blank" href="https://icp.gov.moe/?keyword=20261324">萌ICP备20261324号</a>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
// 备案信息组件
</script>

<style scoped>
.mobile-beian {
  background: white;
  padding: 16px;
  text-align: center;
  border-top: 1px solid #f0f0f0;
  margin-top: auto;
}

.copyright {
  font-size: 12px;
  color: #909399;
  margin-bottom: 8px;
}

.beian {
  font-size: 11px;
  color: #c0c4cc;
}

.beian-item {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 4px;
  line-height: 1.6;
}

.beian-icon {
  width: 15px;
  height: 15px;
  display: inline-block;
  vertical-align: middle;
}

.beian-icon.moe-icon {
  width: 16px;
  height: 16px;
}

.beian-placeholder {
  width: 15px;
  height: 15px;
  display: inline-block;
}

.beian-link {
  color: #c0c4cc;
  text-decoration: none;
  vertical-align: middle;
}

.beian-link:active {
  color: #8bad42;
}
</style>
