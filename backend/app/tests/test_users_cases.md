# User API 测试用例矩阵

## tests/test_users.py

| # | 接口 | 场景 | 输入 | Mock 依赖 | 预期状态 | 预期响应 |
|---|------|------|------|------------|----------|----------|
| 1 | GET /users | 列表查询 - 正常 | `?username=user` | `Db.GetUsers` -> 2条用户 | 200 | `data` 包含2条记录 |
| 2 | GET /users | 列表空 | `?username=nonexistent` | `Db.GetUsers` -> [] | 404 | `code=404 msg=User not found` |
| 3 | GET /users/{id} | 根据id查找 - 成功 | `/users/1` | `Db.GetUsers(id=1)` -> 1 | 200 | 返回1条用户|
| 4 | GET /users/{id} | 根据id查找 - 不存在 | `/users/999` | `Db.GetUsers(id=999)` -> [] | 404 | `code=404 msg=User not found` |
| 5 | POST /users | 注册成功 | 完整字段 | `GetUsers`->[]; `GetInvitationCode`有效; `UseInvitationCode`->True; `RegisterUser`返回新用户 | 200 | 用户对象 |
| 6 | POST /users | 用户已存在 | username existing | `GetUsers`->list | 400 | `User already exist.` |
| 7 | POST /users | 邀请码无效 | invalid code | `GetUsers`->[]; `GetInvitationCode`->None | 400 | `邀请码不存在或已失效` |
| 8 | POST /users | 邀请码过期 | expire_at 过去 | `GetUsers`->[]; `GetInvitationCode`过期 | 400 | `邀请码不存在或已失效` |
| 9 | POST /users | 邀请码次数用尽 | remaining_uses=0 | `GetUsers`->[]; `GetInvitationCode`剩余0 | 400 | `邀请码不存在或已失效` |
| 10| POST /users | 请求格式不合法 | 字段长度/枚举错误 | 无 | 422 | 验证失败 |
| 11| PUT /users/{id} | 更新成功 | `username`相应 | `UpdateUser` 返回更新后对象 | 200 | 用户对象 |
| 12| PUT /users/{id} | 更新不存在 | `/users/999` | `UpdateUser` -> None | 404 | `User not found.` |
| 13| DELETE /users/{id} | root 删除 | op root | `DeleteUser`->真 | 200 | `data=None` |
| 14| DELETE /users/{id} | 非 root 拒绝 | op user | 无调用 | 403 | `Permission denied.` |
| 15| DELETE /users/{id} | 删除不存在 | `/users/999` | `DeleteUser`->None | 404 | `User not found.` |
| 16| GET /users/me | 当前登录用户 | 无 | `Security.GetCurrentUser` 覆盖返回假用户 | 200 | 当前用户对象 |

## tests/test_login.py

| # | 接口 | 场景 | 输入 | Mock 依赖 | 预期状态 | 预期响应 |
|---|------|------|------|------------|----------|----------|
| 17| POST /login | 登录成功 | 账号密码正确 | `Db.GetUsers`->用户; `Db.VerifyPassword`->True; `Security.CreateAccessToken`->token/cookie | 200 | `access_token`, `token_type`，响应包含 `long_token` cookie |
| 18| POST /login | 密码错误 | 用户存在但密码错误 | `Db.GetUsers`->用户; `Db.VerifyPassword`->False | 400 | `code=400 msg=Incorrect username or password` |
| 19| POST /login | 用户不存在 | 用户不存在 | `Db.GetUsers`->[] | 400 | `code=400 msg=Incorrect username or password` |
| 20| POST /refresh | Cookie 刷新 token | 有效 long_token | `Security.GetCurrentUserByCookie`->用户 | 200 | `access_token`, `token_type` |
| 21| POST /logout | 退出登录 | 有效 long_token | `Security.GetCurrentUserByCookie`->用户 | 200 | `code=200 msg=Logout` |

## tests/test_db.py

| # | 用例 | 描述 | 预期 |
|---|------|------|------|
| 22 | 数据库连接 | 通过 `Db.OpenDb` 执行 `SELECT 1` | 返回 `1` |