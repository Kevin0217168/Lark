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

// 响应拦截器
const responseInterceptor = async (response: Response) => {
  if (response.status === 401) {
    // 处理401错误
    localStorage.removeItem('accessToken');
    localStorage.removeItem('refreshToken');
    localStorage.removeItem('isAuthenticated');
    localStorage.removeItem('username');
    localStorage.removeItem('role');
    window.dispatchEvent(new Event('loginStatusChanged'));
    ElMessage.error('登录已过期，请重新登录');
    router.push('/Login');
    throw new ApiError('Unauthorized', 401, {});
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
    return responseInterceptor(response);
  },
  
  async post<T = any>(url: string, data?: any, options?: { headers?: Record<string, string> }): Promise<T> {
    const headers: Record<string, string> = options?.headers || { 'Content-Type': 'application/json' };
    const config = requestInterceptor({
      method: 'POST',
      headers,
      body: data ? (headers['Content-Type'] === 'application/json' ? JSON.stringify(data) : data) : undefined
    });
    
    const response = await fetch(BASE_URL + url, config);
    return responseInterceptor(response);
  },
  
  async put<T = any>(url: string, data?: any): Promise<T> {
    const config = requestInterceptor({
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: data ? JSON.stringify(data) : undefined
    });
    
    const response = await fetch(BASE_URL + url, config);
    return responseInterceptor(response);
  },
  
  async delete<T = any>(url: string): Promise<T> {
    const config = requestInterceptor({ method: 'DELETE' });
    const response = await fetch(BASE_URL + url, config);
    return responseInterceptor(response);
  }
};
