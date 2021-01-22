#ifndef _CLIENT_H_
#define _CLIENT_H_

#include "common.h"
#include "nats.h"

// normal message
typedef void (*edge_normal_msg_handler)(char *topic, char *payload);

typedef enum
{
    SUBDEV_LOGIN  = 0,
    SUBDEV_LOGOUT = 1,
}subdev_log_in_out;

typedef struct
{
    uint32_t request_id;
    char     *payload;
}msg_parse;

typedef struct
{
    bool                        nats_online;
    const char                  *product_sn;  
    const char                  *device_sn;
    edge_normal_msg_handler     normal_msg_handle;
}subdev_client;

typedef enum
{
    EDGE_OK         = 0,                ///< Success

    EDGE_ERR,                           ///< Generic error
    EDGE_PROTOCOL_ERROR,                ///< Error when parsing a protocol message,
                                        ///  or not getting the expected message.
    EDGE_IO_ERROR,                      ///< IO Error (network communication).
    EDGE_LINE_TOO_LONG,                 ///< The protocol message read from the socket
                                        ///  does not fit in the read buffer.

    EDGE_CONNECTION_CLOSED,             ///< Operation on this connection failed because
                                        ///  the connection is closed.
    EDGE_NO_SERVER,                     ///< Unable to connect, the server could not be
                                        ///  reached or is not running.
    EDGE_STALE_CONNECTION,              ///< The server closed our connection because it
                                        ///  did not receive PINGs at the expected interval.
    EDGE_SECURE_CONNECTION_WANTED,      ///< The client is configured to use TLS, but the
                                        ///  server is not.
    EDGE_SECURE_CONNECTION_REQUIRED,    ///< The server expects a TLS connection.
    EDGE_CONNECTION_DISCONNECTED,       ///< The connection was disconnected. Depending on
                                        ///  the configuration, the connection may reconnect.

    EDGE_CONNECTION_AUTH_FAILED,        ///< The connection failed due to authentication error.
    EDGE_NOT_PERMITTED,                 ///< The action is not permitted.
    EDGE_NOT_FOUND,                     ///< An action could not complete because something
                                        ///  was not found. So far, this is an internal error.

    EDGE_ADDRESS_MISSING,               ///< Incorrect URL. For instance no host specified in
                                        ///  the URL.

    EDGE_INVALID_SUBJECT,               ///< Invalid subject, for instance NULL or empty string.
    EDGE_INVALID_ARG,                   ///< An invalid argument is passed to a function. For
                                        ///  instance passing NULL to an API that does not
                                        ///  accept this value.
    EDGE_INVALID_SUBSCRIPTION,          ///< The call to a subscription function fails because
                                        ///  the subscription has previously been closed.
    EDGE_INVALID_TIMEOUT,               ///< Timeout must be positive numbers.

    EDGE_ILLEGAL_STATE,                 ///< An unexpected state, for instance calling
                                        ///  #natsSubscription_NextMsg() on an asynchronous
                                        ///  subscriber.

    EDGE_SLOW_CONSUMER,                 ///< The maximum number of messages waiting to be
                                        ///  delivered has been reached. Messages are dropped.

    EDGE_MAX_PAYLOAD,                   ///< Attempt to send a payload larger than the maximum
                                        ///  allowed by the NATS Server.
    EDGE_MAX_DELIVERED_MSGS,            ///< Attempt to receive more messages than allowed, for
                                        ///  instance because of #natsSubscription_AutoUnsubscribe().

    EDGE_INSUFFICIENT_BUFFER,           ///< A buffer is not large enough to accommodate the data.

    EDGE_NO_MEMORY,                     ///< An operation could not complete because of insufficient
                                        ///  memory.

    EDGE_SYS_ERROR,                     ///< Some system function returned an error.

    EDGE_TIMEOUT,                       ///< An operation timed-out. For instance
                                        ///  #natsSubscription_NextMsg().

    EDGE_FAILED_TO_INITIALIZE,          ///< The library failed to initialize.
    EDGE_NOT_INITIALIZED,               ///< The library is not yet initialized.

    EDGE_SSL_ERROR,                     ///< An SSL error occurred when trying to establish a
                                        ///  connection.

    EDGE_NO_SERVER_SUPPORT,             ///< The server does not support this action.

    EDGE_NOT_YET_CONNECTED,             ///< A connection could not be immediately established and
                                        ///  #natsOptions_SetRetryOnFailedConnect() specified
                                        ///  a connected callback. The connect is retried asynchronously.

    EDGE_DRAINING,                      ///< A connection and/or subscription entered the draining mode.
                                        ///  Some operations will fail when in that mode.

    EDGE_INVALID_QUEUE_NAME,            ///< An invalid queue name was passed when creating a queue subscription.

} edge_status;

edge_status _msg_parse_str_init(msg_parse **msg_parse_str, uint32_t request_id);

edge_status _wait_payload_is_timeout(msg_parse *msg_parse_str, uint32_t time_out_ms);

edge_status _publish_string(const char *subj, const char *str);

void log_print(const char *format,...);

/**
 * @brief 初始化一个子设备
 *
 * @param product_sn:               指定产品序列号
 * @param device_sn:                指定设备序列号
 * @param normal_msg_handle:        接收消息回调处理接口(void (*edge_normal_msg_handler)(char *topic, char *payload)). 
 *
 * @retval : 成功则返回句柄，失败返回NULL
 */
subdev_client * edge_subdev_construct(const char *product_sn, const char *device_sn, edge_normal_msg_handler normal_msg_handle);


/**
 * @brief 向topic发送一条消息
 *
 * @param topic:               		topic名称
 * @param str:                		发送消息内容
 *
 * @retval : 成功则返回EDGE_OK
 */
edge_status edge_publish(const char *topic, const char *str);

/**
 * @brief 动态注册一个子设备
 *
 * @param pst_subdev_client:        子设备句柄
 * @param product_secret:           指定的产品密钥.
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
edge_status edge_subdev_dynamic_auth(subdev_client *pst_subdev_client, const char *product_secret, uint32_t time_out_ms);

/**
 * @brief 子设备登录（同步接口，等待响应消息）
 *
 * @param pst_subdev_client:        子设备句柄
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
edge_status edge_subdev_login_sync(subdev_client *pst_subdev_client, uint32_t time_out_ms);

/**
 * @brief 子设备退出登录（同步接口，等待响应消息）
 *
 * @param pst_subdev_client:        子设备句柄
 * @param time_out_ms:              超时时间，单位ms
 *
 * @retval : 成功则返回EDGE_OK
 */
edge_status edge_subdev_logout_sync(subdev_client *pst_subdev_client, uint32_t time_out_ms);

/**
 * @brief 子设备登录（异步接口，不等待响应消息）
 *
 * @param pst_subdev_client:        子设备句柄
 *
 * @retval : 成功则返回EDGE_OK
 */
edge_status edge_subdev_login_async(subdev_client *pst_subdev_client);

/**
 * @brief 子设备退出登录（异步接口，不等待响应消息）
 *
 * @param pst_subdev_client:        子设备句柄
 *
 * @retval : 成功则返回EDGE_OK
 */
edge_status edge_subdev_logout_async(subdev_client *pst_subdev_client);

/**
 * @brief 记录日志
 *
 * @param level:        日志等级分为：DEBUG、INFO、WARN、ERROR
 * @param format:       日志记录格式
 *
 * @retval : 成功则返回EDGE_OK
 */
void log_write(log_level level, const char *format,...);

/**
 * @brief 查看收到的消息是否为rrpc request
 *
 * @param topic:        消息topic
 *
 * @retval : 是则返回EDGE_OK
 */
edge_status edge_rrpc_check(char *topic);

/**
 * @brief rrpc response
 *
 * @param topic:        rrpc消息topic
 *
 * @param payload:      rrpc回复内容
 *
 * @retval : 成功返回EDGE_OK
 */
edge_status edge_rrpc_response(char *topic,char *payload);

#endif
