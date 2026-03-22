/**
 * 移动端适配工具
 * 用于检测设备类型并切换到相应的页面
 */

/**
 * 检测当前设备是否为移动设备
 * @returns {boolean} 是否为移动设备
 */
export function isMobileDevice(): boolean {
  // 简单的设备检测逻辑
  const userAgent = navigator.userAgent.toLowerCase();
  const mobileKeywords = [
    'android',
    'webos',
    'iphone',
    'ipad',
    'ipod',
    'blackberry',
    'windows phone',
    'mobile'
  ];
  
  return mobileKeywords.some(keyword => userAgent.includes(keyword));
}

/**
 * 检测当前设备屏幕宽度
 * @returns {boolean} 是否为小屏幕设备
 */
export function isSmallScreen(): boolean {
  return window.innerWidth < 768;
}

/**
 * 检查是否需要使用移动端页面
 * @returns {boolean} 是否需要使用移动端页面
 */
export function shouldUseMobilePage(): boolean {
  return isMobileDevice() || isSmallScreen();
}

/**
 * 页面切换逻辑（空实现）
 * 后续可以在这里实现具体的页面切换逻辑
 */
export function switchToMobilePage(): void {
  // 空实现，后续添加具体逻辑
  console.log('切换到移动端页面');
}

/**
 * 页面切换逻辑（空实现）
 * 后续可以在这里实现具体的页面切换逻辑
 */
export function switchToDesktopPage(): void {
  // 空实现，后续添加具体逻辑
  console.log('切换到桌面端页面');
}
