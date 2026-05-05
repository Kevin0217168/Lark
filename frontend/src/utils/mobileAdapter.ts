/**
 * 移动端适配工具
 * 用于检测设备类型、屏幕尺寸并提供响应式断点
 */

export const BREAKPOINTS = {
  MOBILE: 480,
  TABLET: 768,
  DESKTOP: 1024,
  LARGE_DESKTOP: 1440
} as const;

export function isMobileDevice(): boolean {
  const userAgent = navigator.userAgent.toLowerCase();
  const mobileKeywords = [
    'android', 'webos', 'iphone', 'ipad', 'ipod',
    'blackberry', 'windows phone', 'mobile'
  ];
  return mobileKeywords.some(keyword => userAgent.includes(keyword));
}

export function isSmallScreen(): boolean {
  return window.innerWidth < BREAKPOINTS.TABLET;
}

export function isDesktop(): boolean {
  return window.innerWidth >= BREAKPOINTS.TABLET;
}

export function isLargeDesktop(): boolean {
  return window.innerWidth >= BREAKPOINTS.LARGE_DESKTOP;
}

export function shouldUseMobilePage(): boolean {
  return isMobileDevice() || isSmallScreen();
}

export function switchToMobilePage(): void {
  const mobileRouter = (window as any).__MOBILE_ROUTER__;
  if (mobileRouter) {
    mobileRouter.push('/mobile');
  }
}

export function switchToDesktopPage(): void {
  const desktopRouter = (window as any).__DESKTOP_ROUTER__;
  if (desktopRouter) {
    desktopRouter.push('/');
  }
}

export function subscribeToResize(callback: (isDesktop: boolean) => void): () => void {
  const handler = () => callback(window.innerWidth >= BREAKPOINTS.TABLET);
  window.addEventListener('resize', handler);
  handler();
  return () => window.removeEventListener('resize', handler);
}
