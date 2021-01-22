## 编译
编译依赖：cmake 2.8（依赖的nats库仅支持cmake编译）
```
sudo apt-get install cmake
```
编译方式：GNU Make
```
make
```

## 打包方式
将uiot_edge_test改名为main，和依赖的动态链接库（如果有）一起打成zip的压缩包,将压缩包上传到子设备驱动
```
cd samples
cp uiot_edge_test main
zip -r abc.zip main
```

## API参考文档

/**
 * @brief 子设备驱动初始化
 *
 * @param void: 
 *
 * @retval : 成功则返回EDGE_OK
 */
 
edge_status edge_common_init(void)

/**
 * @brief 初始化一个子设备
 *
 * @param product_sn:               指定产品序列号
 * @param device_sn:                指定设备序列号
 * @param normal_msg_handle:        消息回调处理接口(void (*edge_normal_msg_handler)(char *topic, char *payload)). 
 *
 * @retval : 成功则返回句柄，失败返回NULL
 */
 
subdev_client * edge_subdev_construct(const char *product_sn, const char *device_sn, edge_normal_msg_handler normal_msg_handle)

/**
 * @brief 向指定topic发送一条消息
 *
 * @param topic:                    topic名称
 * @param str:                      发送消息内容
 *
 * @retval : 成功则返回EDGE_OK
 */
 
edge_status edge_publish(const char *topic, const char *str)

/**
 * @brief 动态注册一个子设备
 *
 * @param pst_subdev_client:        子设备句柄
 * @param product_secret:           指定的产品密钥.
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
 
edge_status edge_subdev_dynamic_auth(subdev_client *pst_subdev_client, const char *product_secret, uint32_t time_out_ms)

/**
 * @brief 子设备登录（同步接口，等待响应消息）
 *
 * @param pst_subdev_client:        子设备句柄
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
 
edge_status edge_subdev_login_sync(subdev_client *pst_subdev_client, uint32_t time_out_ms)

/**
 * @brief 子设备退出登录（同步接口，等待响应消息）
 *
 * @param pst_subdev_client:        子设备句柄
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
 
edge_status edge_subdev_logout_sync(subdev_client *pst_subdev_client, uint32_t time_out_ms)

/**
 * @brief 子设备登录（异步接口，不等待响应消息）
 *
 * @param pst_subdev_client:        子设备句柄
 *
 * @retval : 成功则返回EDGE_OK
 */
 
edge_status edge_subdev_login_async(subdev_client *pst_subdev_client)

/**
 * @brief 子设备退出登录（异步接口，不等待响应消息）
 *
 * @param pst_subdev_client:        子设备句柄
 *
 * @retval : 成功则返回EDGE_OK
 */
 
edge_status edge_subdev_logout_async(subdev_client *pst_subdev_client)

/**
 * @brief 获取驱动信息
 *
 * @param void:
 *
 * @retval : 成功则返回驱动信息
 */
 
char * edge_get_driver_info(void)

/**
 * @brief 获取设备列表
 *
 * @param void:
 *
 * @retval : 成功则返回设备列表
 */
 
char * edge_get_device_list(void)

/**
 * @brief 设置拓扑变化回调接口（控制台增加删减拓扑）
 *
 * @param topo_notify_handle:	拓扑变化回调接口(void (*edge_topo_notify_handler)(topo_operation opera, char *payload)).
 *
 * @retval : void
 */
 
void edge_set_topo_notify_handle(edge_topo_notify_handler topo_notify_handle)

/**
 * @brief 设置子设备状态变化回调接口（控制台启动禁用设备）
 *
 * @param subdev_status_handle:	子设备状态变化回调接口(void (*edge_subdev_status_handler)(subdev_able opera,char *payload)).
 *
 * @retval : void
 */
 
void edge_set_subdev_status_handle(edge_subdev_status_handler subdev_status_handle)

/**
 * @brief 获取网关在线状态
 *
 * @param void
 *
 * @retval : bool	true:在线 flase离线
 */
 
bool edge_get_online_status(void)

/**
 * @brief 获取子设备拓扑
 *
 * @param pst_subdev_client:        子设备句柄
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
 
char *edge_get_topo(uint32_t time_out_ms)

/**
 * @brief 增加子设备拓扑
 *
 * @param pst_subdev_client:        子设备句柄
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
 
edge_status edge_add_topo(subdev_client *pst_subdev_client, uint32_t time_out_ms)

/**
 * @brief 删除子设备拓扑
 *
 * @param pst_subdev_client:        子设备句柄
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
 
edge_status edge_delete_topo(subdev_client *pst_subdev_client, uint32_t time_out_ms)

/**
 * @brief 设置日志记录等级，日志文件大小和文件个数（一天一个日志文件）
 *
 * @param level:        日志等级分为：DEBUG、INFO、WARNING、ERROR、RITICAL
 * @param file_size_mb: 单个日志文件大小
 * @param file_number:  日志文件个数
 *
 * @retval : 成功则返回EDGE_OK
 */
 
edge_status edge_set_log(log_level level, uint32_t file_size_mb, uint32_t file_number)

/**
 * @brief 记录日志
 *
 * @param level:        日志等级分为：DEBUG、INFO、WARNING、ERROR、CRITICAL
 * @param format:       日志记录格式
 *
 * @retval : 成功则返回EDGE_OK
 */
 
void log_write(log_level level, const char *format,...)

