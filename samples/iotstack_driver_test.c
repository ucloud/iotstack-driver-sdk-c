#include "edge.h"
#include "cJSON.h"

static void edge_normal_msg_handler_user(char *topic, char *payload, int payloadLen)
{
    log_write(LOG_INFO, "topic:%s payload:%s payloadLen:%d", topic, payload, payloadLen);
    return;
}

static void edge_rrpc_msg_handler_user(char *topic, char *payload, int payloadLen)
{
    log_write(LOG_INFO, "rrpc topic:%s payload:%s payloadLen:%d", topic, payload, payloadLen);
    char *response_msg = "rrpc response message!";
    edge_rrpc_response(topic, response_msg, strlen(response_msg));
    return;
}

static void edge_topo_notify_handler_user(topo_operation opera, char *payload)
{
    log_write(LOG_INFO, "topo change:%d payload:%s", opera, payload);
    return;
}

static void edge_subdev_status_handler_user(subdev_able opera,char *payload)
{
    log_write(LOG_INFO, "subdev:%d payload:%s", opera, payload);
    return;
}

int main(int argc, char **argv)
{
    edge_status         status     = EDGE_OK;
    subdev_client *     subdevClient = NULL;
    cJSON *dirver_info_cfg = NULL;
    cJSON *device_list_cfg = NULL;
    int upload_period = 5;
    char *topic_str = NULL;
    struct timeval stamp;
    char *time_stamp = NULL;
    const char *switch_str[2] = {"on", "off"};
    int loop = 0;
    
    status = edge_common_init();
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "edge_Common_Init fail");
        return EDGE_ERR;
    }

    topic_str = (char *)malloc(512);
    if(NULL == topic_str)
    {
        log_write(LOG_ERROR, "topic_str malloc fail");
        return EDGE_ERR;
    }

    time_stamp = (char *)malloc(512);
    if(NULL == time_stamp)
    {
        log_write(LOG_ERROR, "time_stamp malloc fail");
        free(topic_str);
        return EDGE_ERR;
    }

    edge_set_topo_notify_handle(edge_topo_notify_handler_user);
    
    edge_set_subdev_status_handle(edge_subdev_status_handler_user);

    // 解析驱动配置
    /* 
    {
        "period": 10,
        "msg_config": {
            "topic": "/%s/%s/upload",
            "param_name": "relay_status"
        }
    }
    */
    log_write(LOG_INFO, "edge_get_productSN: [%s]",edge_get_product_sn());
    log_write(LOG_INFO, "edge_get_deviceSN: [%s]",edge_get_device_sn());

    dirver_info_cfg = cJSON_Parse(edge_get_driver_info());
    if (!dirver_info_cfg) 
    {
        log_write(LOG_ERROR, "edge_get_driver_info parse error: [%s]",cJSON_GetErrorPtr());
        free(time_stamp);
        free(topic_str);
        return EDGE_ERR;
    }

    cJSON *period = cJSON_GetObjectItem(dirver_info_cfg, "period");
    if(NULL != period)
    {
        upload_period = period->valueint;
    }
    
    // 解析配置取出msg_config下的topic
    cJSON *config_item = cJSON_GetObjectItem(dirver_info_cfg, "msg_config");
    cJSON *topic_format_json = cJSON_GetObjectItem(config_item, "topic");  
    cJSON *topic_param_name_json = cJSON_GetObjectItem(config_item, "param_name");    

    // 解析设备列表
    device_list_cfg = cJSON_Parse(edge_get_device_info());    
    if (!device_list_cfg) 
    {
        log_write(LOG_ERROR, "edge_get_device_list parse error: [%s]",cJSON_GetErrorPtr());
        free(time_stamp);
        free(topic_str);
        cJSON_Delete(dirver_info_cfg);
        return EDGE_ERR;
    }

    // 判断是否绑定子设备，并获取设备列表的productsn和devicesn，将设备上线
    if(cJSON_GetArraySize(device_list_cfg) > 0)
    {
        cJSON *arrary_item = cJSON_GetArrayItem(device_list_cfg, 0);
        cJSON *arrary_productsn = cJSON_GetObjectItem(arrary_item, "productSN");
        cJSON *arrary_devicesn = cJSON_GetObjectItem(arrary_item, "deviceSN");  
        // 组成发送消息topic
        memset(topic_str, 0, 512);
        if(NULL != topic_format_json)
        {
            snprintf(topic_str, 512, topic_format_json->valuestring, arrary_productsn->valuestring, arrary_devicesn->valuestring);
        }
        else
        {
            snprintf(topic_str, 512, "/%s/%s/upload", arrary_productsn->valuestring, arrary_devicesn->valuestring);
        }
        
        // 初始化一个子设备
        subdevClient = edge_subdev_construct(arrary_productsn->valuestring, arrary_devicesn->valuestring, edge_normal_msg_handler_user, edge_rrpc_msg_handler_user);
        if(NULL == subdevClient)
        {
            log_write(LOG_ERROR, "edge construct fail!");
            goto end;
        }

        #if 0
        // 动态注册一个子设备
        status = edge_subdev_dynamic_auth(subdevClient, "product_secret", 5000);
        if(EDGE_OK != status)
        {
            log_write(LOG_ERROR, "edge dynamic auth fail!");
            goto end;
        }
        #endif
        
        // 绑定子设备拓扑
        status = edge_add_topo(subdevClient, 5000);
        if(EDGE_OK != status)
        {
            log_write(LOG_ERROR, "edge_add_topo fail");
            goto end;
        }

        // 查看拓扑
        char *topoa = edge_get_topo(5000);
        if(NULL != topoa)
        {
            log_write(LOG_INFO, "edge_get_topo:%s", topoa);
        }

        // 子设备登录
        //status = edge_subdev_login_sync(subdevClient， 5000); 子设备登录同步接口，等待响应结果
        status = edge_subdev_login_async(subdevClient);     //子设备登录异步接口，不等待响应结果
        if(EDGE_OK != status)
        {
            log_write(LOG_ERROR, "edge_subdev_login fail");
            goto end;
        }
    } 
    
    while(1)
    {            
        // 维持心跳,并周期发送消息
        sleep(upload_period);

        gettimeofday(&stamp, NULL);
        memset(time_stamp, 0, 512);
        if(NULL != topic_param_name_json)
        {
            snprintf(time_stamp, 512, "{\"timestamp\": \"%ld\", \"%s\": \"%s\"}", stamp.tv_sec, topic_param_name_json->valuestring, switch_str[loop++ % 2]);
        }
        else
        {
            snprintf(time_stamp, 512, "{\"timestamp\": \"%ld\", \"%s\": \"%s\"}", stamp.tv_sec, "relay_status", switch_str[loop++ % 2]);
        }
        log_write(LOG_DEBUG, "send message[%s]", time_stamp);
        
        status = edge_publishString(topic_str, time_stamp);
		//spuuprt publish binary message
        status |= edge_publish(topic_str, "0D0A2131", 8);
        if(EDGE_OK != status)
        {
            log_write(LOG_ERROR, "edge_publish fail");
            goto end;
        }
    }

end:
    free(time_stamp);
    free(topic_str);
    cJSON_Delete(dirver_info_cfg);
    cJSON_Delete(device_list_cfg);
    return EDGE_ERR;
}

