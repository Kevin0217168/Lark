import { ElMessage } from 'element-plus';
import router from '@/router';

const BASE_URL = '';

// 自定义API错误类
class ApiError extends Error {
  status: number;
  data: any;
  
  constructor(message: string, status: number, data: any) {
    super(message);
    this.name = 'ApiError';
    this.status = status;
    this.data = data;
  }
}

// 请求拦截器
const requestInterceptor = (config: RequestInit) => {
  const token = localStorage.getItem('accessToken');
  const headers = new Headers(config.headers);
  
  if (token) {
    headers.set('Authorization', `Bearer ${token}`);
  }
  headers.set('Accept', 'application/json');
  
  return { ...config, headers, credentials: 'include' as RequestCredentials };
};

// 是否正在刷新token的标志，防止重复刷新
let isRefreshing = false;
// 等待token刷新的请求队列
let refreshSubscribers: Array<(token: string) => void> = [];

// 订阅token刷新
const subscribeTokenRefresh = (callback: (token: string) => void) => {
  refreshSubscribers.push(callback);
};

// 通知所有订阅者token已刷新
const onTokenRefreshed = (newToken: string) => {
  refreshSubscribers.forEach(callback => callback(newToken));
  refreshSubscribers = [];
};

// 刷新token的函数
const refreshToken = async (): Promise<string | null> => {
  const refreshTokenValue = localStorage.getItem('refreshToken');
  if (!refreshTokenValue) {
    return null;
  }

  try {
    const response = await fetch('/api/refresh', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json'
      },
      body: JSON.stringify({ refresh_token: refreshTokenValue })
    });

    if (!response.ok) {
      return null;
    }

    const data = await response.json();
    if (data.access_token) {
      localStorage.setItem('accessToken', data.access_token);
      if (data.refresh_token) {
        localStorage.setItem('refreshToken', data.refresh_token);
      }
      return data.access_token;
    }
    return null;
  } catch (error) {
    return null;
  }
};

// 处理登录失效
const handleAuthFailure = () => {
  localStorage.removeItem('accessToken');
  localStorage.removeItem('refreshToken');
  localStorage.removeItem('isAuthenticated');
  localStorage.removeItem('username');
  localStorage.removeItem('role');
  window.dispatchEvent(new Event('loginStatusChanged'));
  ElMessage.error('登录已过期，请重新登录');

  const currentPath = window.location.pathname;
  const isCloudPage = currentPath.startsWith('/cloud');
  router.push(isCloudPage ? '/cloud/login' : '/Login');
};

// 响应拦截器
const responseInterceptor = async (response: Response, originalRequest?: RequestInit & { url?: string }) => {
  // 检查是否在登录注册界面
  const currentPath = window.location.pathname;
  const isAuthPage =
    currentPath === '/Login' ||
    currentPath === '/Register' ||
    currentPath === '/cloud/login' ||
    currentPath === '/cloud/register';

  if (response.status === 401 && !isAuthPage) {
    // 尝试刷新token
    if (!isRefreshing) {
      isRefreshing = true;
      const newToken = await refreshToken();
      isRefreshing = false;

      if (newToken) {
        // token刷新成功，通知所有等待的请求
        onTokenRefreshed(newToken);
        // 重试原始请求
        if (originalRequest && originalRequest.url) {
          const headers = new Headers(originalRequest.headers);
          headers.set('Authorization', `Bearer ${newToken}`);
          const retryResponse = await fetch(originalRequest.url, {
            ...originalRequest,
            headers
          });
          return responseInterceptor(retryResponse);
        }
        return response.json();
      } else {
        // token刷新失败，确定为登录失效
        handleAuthFailure();
        throw new ApiError('Unauthorized', 401, {});
      }
    } else {
      // 正在刷新token，等待刷新完成
      return new Promise((resolve, reject) => {
        subscribeTokenRefresh((newToken: string) => {
          if (originalRequest && originalRequest.url) {
            const headers = new Headers(originalRequest.headers);
            headers.set('Authorization', `Bearer ${newToken}`);
            fetch(originalRequest.url, {
              ...originalRequest,
              headers
            })
              .then(res => responseInterceptor(res))
              .then(resolve)
              .catch(reject);
          } else {
            resolve(response.json());
          }
        });
      });
    }
  }

  if (!response.ok) {
    const error = await response.json().catch(() => ({}));
    const message = error.detail || error.msg || '请求失败';
    throw new ApiError(message, response.status, error);
  }

  return response.json();
};

// 统一请求方法
export const api = {
  async get<T = any>(url: string, params?: Record<string, any>): Promise<T> {
    let fullUrl = BASE_URL + url;
    if (params) {
      const searchParams = new URLSearchParams();
      Object.entries(params).forEach(([key, value]) => {
        if (value !== undefined && value !== null) {
          searchParams.append(key, String(value));
        }
      });
      fullUrl += `?${searchParams.toString()}`;
    }

    const config = requestInterceptor({ method: 'GET' });
    const response = await fetch(fullUrl, config);
    return responseInterceptor(response, { ...config, url: fullUrl });
  },

  async post<T = any>(url: string, data?: any, options?: { headers?: Record<string, string> }): Promise<T> {
    const headers: Record<string, string> = options?.headers || { 'Content-Type': 'application/json' };
    const config = requestInterceptor({
      method: 'POST',
      headers,
      body: data ? (headers['Content-Type'] === 'application/json' ? JSON.stringify(data) : data) : undefined
    });

    const fullUrl = BASE_URL + url;
    const response = await fetch(fullUrl, config);
    return responseInterceptor(response, { ...config, url: fullUrl });
  },

  async put<T = any>(url: string, data?: any): Promise<T> {
    const config = requestInterceptor({
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: data ? JSON.stringify(data) : undefined
    });

    const fullUrl = BASE_URL + url;
    const response = await fetch(fullUrl, config);
    return responseInterceptor(response, { ...config, url: fullUrl });
  },

  async delete<T = any>(url: string): Promise<T> {
    const config = requestInterceptor({ method: 'DELETE' });
    const fullUrl = BASE_URL + url;
    const response = await fetch(fullUrl, config);
    return responseInterceptor(response, { ...config, url: fullUrl });
  }
};
