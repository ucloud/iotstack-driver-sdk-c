#ifndef _EDGE_H_
#define _EDGE_H_

#include "client.h"

typedef enum
{
    TOPO_GET    = 0,
    TOPO_ADD    = 1,
    TOPO_DELETE = 2,
}topo_operation;

typedef enum
{
    SUBDEV_ENABLE,
    SUBDEV_DISABLE,
}subdev_able;

// /topic:$system/{productSN}/{deviceSN}/subdev/topo/notify/add || /$system/{productSN}/{deviceSN}/subdev/topo/notify/delete msg handle
typedef void (*edge_topo_notify_handler)(topo_operation opera, char *payload);

// /topic:$system/{productSN}/{deviceSN}/subdev/enable || /$system/{productSN}/{deviceSN}/subdev/delete msg handle
typedef void (*edge_subdev_status_handler)(subdev_able opera,char *payload);

/**
 * @brief 获取驱动信息
 *
 * @param void:
 *
 * @retval : 成功则返回驱动信息
 */
char *edge_get_driver_info(void);

/**
 * @brief 获取子设备信息
 *
 * @param void:
 *
 * @retval : 成功则返回子设备信息
 */
char *edge_get_device_info(void);

/**
 * @brief 获取网关的产品序列号
 *
 * @param void:
 *
 * @retval : 成功则返回产品序列号
 */
char * edge_get_product_sn(void);

/**
 * @brief 获取网关的设备序列号
 *
 * @param void:
 *
 * @retval : 成功则返回设备序列号
 */
char * edge_get_device_sn(void);

/**
 * @brief 子设备驱动初始化
 *
 * @param void: 
 *
 * @retval : 成功则返回EDGE_OK
 */
edge_status edge_common_init(void);

/**
 * @brief 设置拓扑变化回调接口（控制台增加删减拓扑）
 *
 * @param topo_notify_handle:	拓扑变化回调接口(void (*edge_topo_notify_handler)(topo_operation opera, char *payload)).
 *
 * @retval : void
 */
void edge_set_topo_notify_handle(edge_topo_notify_handler          topo_notify_handle);

/**
 * @brief 设置子设备状态变化回调接口（控制台启动禁用设备）
 *
 * @param subdev_status_handle:	子设备状态变化回调接口(void (*edge_subdev_status_handler)(subdev_able opera,char *payload)).
 *
 * @retval : void
 */
void edge_set_subdev_status_handle(edge_subdev_status_handler subdev_status_handle);

/**
 * @brief 获取子设备拓扑
 *
 * @param pst_subdev_client:        子设备句柄
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
char *edge_get_topo(uint32_t time_out_ms);

/**
 * @brief 增加子设备拓扑
 *
 * @param pst_subdev_client:        子设备句柄
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
edge_status edge_add_topo(subdev_client *pst_subdev_client, uint32_t time_out_ms);

/**
 * @brief 删除子设备拓扑
 *
 * @param pst_subdev_client:         子设备句柄
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
edge_status edge_delete_topo(subdev_client *pst_subdev_client, uint32_t time_out_ms);

#endif
