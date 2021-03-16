#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "client.h"

subdev_client * edge_subdev_construct(const char *product_sn, const char *device_sn, edge_normal_msg_handler normal_msg_handle, edge_rrpc_msg_handler rrpc_msg_handle)
{
    subdev_client *pst_subdev_client;    

    if((NULL == product_sn) || (NULL == device_sn) || (NULL == normal_msg_handle))
    {
        return NULL;
    }
    
    if((pst_subdev_client = (subdev_client *)EDGE_MALLOC(sizeof(subdev_client))) == NULL)
    {
        log_write(LOG_ERROR, "malloc pst_subdev_client fail!");
        return NULL;
    }
    memset(pst_subdev_client, 0, sizeof(subdev_client));
    pst_subdev_client->nats_online          = false;
    pst_subdev_client->product_sn           = product_sn;
    pst_subdev_client->device_sn            = device_sn;
    pst_subdev_client->normal_msg_handle    = normal_msg_handle;
    pst_subdev_client->rrpc_msg_handle      = rrpc_msg_handle;
    return pst_subdev_client;
}

edge_status _publish_string(const char *subj, const char *str)
{
    edge_status status;

    status = natsConnection_PublishString(conn, subj, str);
    status |= natsConnection_Flush(conn);

    return status;
}

edge_status edge_publish(const char *topic, const char *data, int dataLen)
{
    edge_status status;

    if((NULL == topic) || (NULL == data))
    {
        return EDGE_INVALID_ARG;
    }

    char *normal_payload_base64 = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == normal_payload_base64)
    {
        log_write(LOG_ERROR, "normal_payload_base64 malloc fail!");
        return EDGE_NO_MEMORY;
    }
    memset(normal_payload_base64, 0, NATS_MSG_MAX_LEN);
    base64_encode(data, dataLen, normal_payload_base64);
    log_write(LOG_DEBUG, "send data:%s",data);

    char *normal_msg = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == normal_msg)
    {
        log_write(LOG_ERROR, "normal_msg malloc fail!");
        EDGE_FREE(normal_payload_base64);
        return EDGE_NO_MEMORY;
    }
    memset(normal_msg, 0, NATS_MSG_MAX_LEN);
    snprintf(normal_msg, NATS_MSG_MAX_LEN, NORMAL_MSG_FORMAT, topic, normal_payload_base64);
    log_write(LOG_DEBUG, "normal_msg:%s",normal_msg);

    status = _publish_string(edge_router_subject, normal_msg);
    
    EDGE_FREE(normal_payload_base64);    
    EDGE_FREE(normal_msg);
    return status;
}

edge_status edge_publishString(const char *topic, const char *str)
{
    edge_status status;

    status = edge_publish(topic,str,strlen(str));

    return status;
}

edge_status _msg_parse_str_init(msg_parse **msg_parse_str, uint32_t request_id)
{
    *msg_parse_str = (msg_parse *)EDGE_MALLOC(sizeof(msg_parse));
    if(NULL == *msg_parse_str)
    {
        log_write(LOG_ERROR, "msgParse_str malloc fail!");
        return EDGE_NO_MEMORY;
    }
    memset(*msg_parse_str, 0, sizeof(msg_parse));

    (*msg_parse_str)->request_id = request_id;
    (*msg_parse_str)->payload    = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == (*msg_parse_str)->payload)
    {
        log_write(LOG_ERROR, "msg_parse_str->payload malloc fail!");
        return EDGE_NO_MEMORY;
    }
    memset((*msg_parse_str)->payload, 0, NATS_MSG_MAX_LEN);
    return EDGE_OK;
}

edge_status _wait_payload_is_timeout(msg_parse *msg_parse_str, uint32_t time_out_ms)
{
    int loop = 0;

    while((loop < 10) && (strlen(msg_parse_str->payload) == 0))
    {
        nats_Sleep(time_out_ms/10);
        loop++;
    }

    if((loop >= 10) && (strlen(msg_parse_str->payload) == 0))
    {
        return EDGE_INVALID_TIMEOUT;
    }
    return EDGE_OK;
}

edge_status edge_subdev_dynamic_auth(subdev_client *pst_subdev_client, const char *product_secret, uint32_t time_out_ms)
{
    edge_status status;
    
    if((NULL == pst_subdev_client) || (NULL == product_secret))
    {
        return EDGE_INVALID_ARG;
    }
    
    uint32_t request_id;
    request_id = _gen_requestid();
    msg_parse *msg_parse_str = NULL;
    if(EDGE_OK != _msg_parse_str_init(&msg_parse_str, request_id))
    {
        return EDGE_NO_MEMORY;
    }
    _add_to_list(requestid_list, msg_parse_str, requestid_list_mutex);
    
    char *dyn_reg_payload = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == dyn_reg_payload)
    {
        log_write(LOG_ERROR, "dyn_reg_payload malloc fail!");
        return EDGE_NO_MEMORY;
    }
    memset(dyn_reg_payload, 0, NATS_MSG_MAX_LEN);
    snprintf(dyn_reg_payload, NATS_MSG_MAX_LEN, DYN_REG_FORMAT_PAYLOAD, request_id, pst_subdev_client->product_sn, pst_subdev_client->device_sn, product_secret);
    log_write(LOG_DEBUG, "dyn_reg_payload:%s",dyn_reg_payload);

    char *dyn_reg_payload_base64 = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == dyn_reg_payload_base64)
    {
        log_write(LOG_ERROR, "dyn_reg_payload_base64 malloc fail!");
        EDGE_FREE(dyn_reg_payload);
        return EDGE_NO_MEMORY;
    }
    memset(dyn_reg_payload_base64, 0, NATS_MSG_MAX_LEN);
    base64_encode(dyn_reg_payload, strlen(dyn_reg_payload), dyn_reg_payload_base64);
    log_write(LOG_DEBUG, "dyn_reg_payload_base64:%s",dyn_reg_payload_base64);

    char *dyn_reg_msg = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == dyn_reg_msg)
    {
        log_write(LOG_ERROR, "dyn_reg_msg malloc fail!");
        EDGE_FREE(dyn_reg_payload);
        EDGE_FREE(dyn_reg_payload_base64);
        return EDGE_NO_MEMORY;
    }
    memset(dyn_reg_msg, 0, NATS_MSG_MAX_LEN);
    snprintf(dyn_reg_msg, NATS_MSG_MAX_LEN, DYN_REG_FORMAT, pst_subdev_client->product_sn, pst_subdev_client->device_sn, dyn_reg_payload_base64);
    log_write(LOG_DEBUG, "dyn_reg_msg:%s",dyn_reg_msg);
    
    /* pub login msg */
    status = _publish_string(edge_router_subject, dyn_reg_msg);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "publish dynamic register msg fail");
        goto end;
    }

    status = _wait_payload_is_timeout(msg_parse_str, time_out_ms);
    if(EDGE_INVALID_TIMEOUT == status)
    {
        log_write(LOG_ERROR, "dynamic register timeout");
        goto end;
    }

    cJSON *msg_parse_str_payload = cJSON_Parse(msg_parse_str->payload);    
    if (!msg_parse_str_payload) 
    {
        log_write(LOG_ERROR, "json parse msg_parse_str payload error: [%s]",cJSON_GetErrorPtr());
        status = EDGE_ERR;
        goto end;
    }
    cJSON *retCode_json = cJSON_GetObjectItem(msg_parse_str_payload, "RetCode");
    if(0 == retCode_json->valueint)
    {
        pst_subdev_client->nats_online = true;
        log_write(LOG_INFO, "device[%s] dynamic register successful", pst_subdev_client->device_sn);
        status = EDGE_OK;
        cJSON_Delete(msg_parse_str_payload);
        goto end;
    }

    log_write(LOG_ERROR, "device[%s] dynamic register fail", pst_subdev_client->device_sn);
    status = EDGE_ERR;
end:
    _remove_from_list(requestid_list, (void *)msg_parse_str, requestid_list_mutex);    
    EDGE_FREE(dyn_reg_payload);
    EDGE_FREE(dyn_reg_payload_base64);
    EDGE_FREE(dyn_reg_msg);
    return status;
}


static edge_status _log_in_out(subdev_client *pst_subdev_client, subdev_log_in_out log_op, uint32_t request_id)
{
    edge_status status;
    
    char *login_str_payload = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == login_str_payload)
    {
        log_write(LOG_ERROR, "login_str_payload malloc fail!");
        return EDGE_NO_MEMORY;
    }
    memset(login_str_payload, 0, NATS_MSG_MAX_LEN);
    snprintf(login_str_payload, NATS_MSG_MAX_LEN, LOG_IN_OUT_FORMAT_PAYLOAD, request_id, pst_subdev_client->product_sn, pst_subdev_client->device_sn);
    log_write(LOG_DEBUG, "login_str_payload:%s\r\n",login_str_payload);

    char *login_str_payload_base64 = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == login_str_payload_base64)
    {
        log_write(LOG_ERROR, "login_str_payload_base64 malloc fail!");
        EDGE_FREE(login_str_payload);
        return EDGE_NO_MEMORY;
    }
    memset(login_str_payload_base64, 0, NATS_MSG_MAX_LEN);
    base64_encode(login_str_payload, strlen(login_str_payload), login_str_payload_base64);
    
    char *login_out_str = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == login_out_str)
    {
        log_write(LOG_ERROR, "login_str malloc fail!");
        EDGE_FREE(login_str_payload);
        EDGE_FREE(login_str_payload_base64);
        return EDGE_NO_MEMORY;
    }
    memset(login_out_str, 0, NATS_MSG_MAX_LEN);
    if(SUBDEV_LOGIN == log_op)
        snprintf(login_out_str, NATS_MSG_MAX_LEN, LOG_IN_OUT_FORMAT, pst_subdev_client->product_sn, pst_subdev_client->device_sn, "login", login_str_payload_base64);
    else
        snprintf(login_out_str, NATS_MSG_MAX_LEN, LOG_IN_OUT_FORMAT, pst_subdev_client->product_sn, pst_subdev_client->device_sn, "logout", login_str_payload_base64);
    log_write(LOG_DEBUG, "login_str:%s",login_out_str);

    /* pub login msg */
    status = _publish_string(edge_router_subject, login_out_str);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "edge_PublishString fail");
    }

    EDGE_FREE(login_str_payload);
    EDGE_FREE(login_str_payload_base64);
    EDGE_FREE(login_out_str);
    return status;
}

edge_status edge_subdev_login_sync(subdev_client *pst_subdev_client, uint32_t time_out_ms)
{
    edge_status status;
    
    if(NULL == pst_subdev_client)
    {
        return EDGE_INVALID_ARG;
    }
    
    uint32_t request_id = _gen_requestid();
    msg_parse *msg_parse_str = NULL;
    if(EDGE_OK != _msg_parse_str_init(&msg_parse_str, request_id))
    {
        return EDGE_NO_MEMORY;
    }
    _add_to_list(requestid_list, msg_parse_str, requestid_list_mutex);
    status = _log_in_out(pst_subdev_client, SUBDEV_LOGIN, request_id);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "_log_in_out fail");
        goto end;
    }
    _add_to_list(conn_device_list, (void *)pst_subdev_client, conn_device_list_mutex);

    status = _wait_payload_is_timeout(msg_parse_str, time_out_ms);
    if(EDGE_INVALID_TIMEOUT == status)
    {
        log_write(LOG_ERROR, "subdev login timeout");
        goto end;
    }

    cJSON *msg_parse_str_payload = cJSON_Parse(msg_parse_str->payload);    
    if (!msg_parse_str_payload) 
    {
        log_write(LOG_ERROR, "json parse msg_parse_str payload error: [%s]",cJSON_GetErrorPtr());
        status = EDGE_ERR;
        goto end;
    }
    cJSON *retCode_json = cJSON_GetObjectItem(msg_parse_str_payload,"RetCode");    
    if(0 == retCode_json->valueint)
    {
        pst_subdev_client->nats_online = true;        
        log_write(LOG_INFO, "device[%s] login successful", pst_subdev_client->device_sn);
        status = EDGE_OK;     
        cJSON_Delete(msg_parse_str_payload);
        goto end;
    }

    log_write(LOG_ERROR, "device[%s] login fail", pst_subdev_client->device_sn);    
    status = EDGE_PROTOCOL_ERROR;
end:
    _remove_from_list(requestid_list, (void *)msg_parse_str, requestid_list_mutex);       
    return status;
}

edge_status edge_subdev_logout_sync(subdev_client *pst_subdev_client, uint32_t time_out_ms)
{
    edge_status status;    
    
    if(NULL == pst_subdev_client)
    {
        return EDGE_INVALID_ARG;
    }
    
    if(false == pst_subdev_client->nats_online)
    {
        return EDGE_OK;
    }
    
    uint32_t request_id = _gen_requestid();
    msg_parse *msg_parse_str = NULL;
    if(EDGE_OK != _msg_parse_str_init(&msg_parse_str, request_id))
    {
        return EDGE_NO_MEMORY;
    }
    _add_to_list(requestid_list, (void *)msg_parse_str, requestid_list_mutex);
    status = _log_in_out(pst_subdev_client, SUBDEV_LOGOUT, request_id);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "_log_in_out fail");
        goto end;
    }
    _remove_from_list(conn_device_list, (void *)pst_subdev_client, conn_device_list_mutex);

    status = _wait_payload_is_timeout(msg_parse_str, time_out_ms);
    if(EDGE_INVALID_TIMEOUT == status)
    {
        log_write(LOG_ERROR, "subdev logout timeout");
        goto end;
    }

    cJSON *msg_parse_str_payload = cJSON_Parse(msg_parse_str->payload);    
    if (!msg_parse_str_payload) 
    {
        log_write(LOG_ERROR, "json parse msg_parse_str payload error: [%s]",cJSON_GetErrorPtr());
        status = EDGE_ERR;
        goto end;
    }
    cJSON *retCode_json = cJSON_GetObjectItem(msg_parse_str_payload,"RetCode");    
    if(0 == retCode_json->valueint)
    {
        pst_subdev_client->nats_online = false;        
        status = EDGE_OK;        
        log_write(LOG_INFO, "device[%s] logout successful", pst_subdev_client->device_sn);     
        cJSON_Delete(msg_parse_str_payload);
        goto end;
    }

    log_write(LOG_ERROR, "device[%s] logout fail", pst_subdev_client->device_sn);
    status = EDGE_PROTOCOL_ERROR;
end:    
    _remove_from_list(requestid_list, (void *)msg_parse_str, requestid_list_mutex);
    return status;
}

edge_status edge_subdev_login_async(subdev_client *pst_subdev_client)
{
    edge_status status;
    
    if(NULL == pst_subdev_client)
    {
        return EDGE_INVALID_ARG;
    }
    
    uint32_t request_id = _gen_requestid();
    status = _log_in_out(pst_subdev_client, SUBDEV_LOGIN, request_id);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "_log_in_out fail");
        return status;
    }    
    pst_subdev_client->nats_online = true;      
    log_write(LOG_INFO, "device[%s] login successful", pst_subdev_client->device_sn);     
    status = _add_to_list(conn_device_list, (void *)pst_subdev_client, conn_device_list_mutex);

    return status;
}

edge_status edge_subdev_logout_async(subdev_client *pst_subdev_client)
{
    edge_status status;    
    
    if(NULL == pst_subdev_client)
    {
        return EDGE_INVALID_ARG;
    }
    
    if(false == pst_subdev_client->nats_online)
    {
        return EDGE_OK;
    }
    
    uint32_t request_id = _gen_requestid();
    status = _log_in_out(pst_subdev_client, SUBDEV_LOGOUT, request_id);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "_log_in_out fail");
        return status;
    }
    
    pst_subdev_client->nats_online = false;     
    log_write(LOG_INFO, "device[%s] logout successful", pst_subdev_client->device_sn);     
    status = _remove_from_list(conn_device_list, (void *)pst_subdev_client, conn_device_list_mutex);    
    return status;
}
 
static int _count_string(char *data, char *key)  
{  
    int count = 0;  
    int klen = strlen(key);  
    char *pos_start = data, *pos_end;  
    while (NULL != (pos_end = strstr(pos_start, key))) {  
        pos_start = pos_end + klen;  
        count++;  
    }  
    return count;  
}  
 
static void _replace_str(char *new_buf, char *data, char *rep, char *to)  
{  
    int rep_len = strlen(rep);  
    int to_len  = strlen(to);  
    int counts  = _count_string(data, rep);  
    if(0 == counts)
    {
        strcpy(new_buf, data);
        return;
    }

    char *pos_start = data, *pos_end, *pbuf = new_buf;  
    int copy_len;  
    while (NULL != (pos_end = strstr(pos_start, rep))) {  
        copy_len = pos_end - pos_start;  
        strncpy(pbuf, pos_start, copy_len);  
        pbuf += copy_len;  
        strcpy(pbuf, to);  
        pbuf += to_len;  
        pos_start  = pos_end + rep_len;  
    }  
    strcpy(pbuf, pos_start);  

    return;  
}  

static int print_debug = 0;
void log_print(const char *format,...)
{
    if(print_debug == 0)
        return;
    va_list args;

    va_start(args, format);
    printf(format, args);
    va_end(args);

    fflush(stdout);
}

void log_write(log_level level, const char *format,...)
{    
    if((level < LOG_DEBUG) || (level > LOG_ERROR))
    {
        return;
    }
    const char *log_lev[5] = {"debug", "info", "warn", "error"};
    struct timeval stamp;
    char *msg_str_rep = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == msg_str_rep)
    {
        return;
    }
    memset(msg_str_rep, 0, NATS_MSG_MAX_LEN);
    
    gettimeofday(&stamp, NULL);
    char *msg_str = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == msg_str)
    {
        EDGE_FREE(msg_str_rep);
        return;
    }
    memset(msg_str, 0, NATS_MSG_MAX_LEN);

    char *log_msg = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == log_msg)
    {
        EDGE_FREE(msg_str_rep);
        EDGE_FREE(msg_str);
        return;
    }
    memset(log_msg, 0, NATS_MSG_MAX_LEN);
    va_list args;

    va_start(args, format);
    vsnprintf(msg_str, NATS_MSG_MAX_LEN, format, args);
    va_end(args);

    // 将json字段转换成字符串
    _replace_str(msg_str_rep, msg_str, "\"", "\\\"");
    snprintf(log_msg, NATS_MSG_MAX_LEN, LOG_UPLOAD_FORMAT, driver_name, log_lev[level], msg_str_rep, stamp.tv_sec);
    
    _publish_string(EDGE_LOG_UPLOAD_SUBJECT, log_msg);
    log_print(log_msg);

    EDGE_FREE(msg_str_rep);
    EDGE_FREE(msg_str);
    EDGE_FREE(log_msg);
    return;
}

edge_status edge_rrpc_check(char *topic)
{
    if(strstr(topic,"/rrpc/request/") == NULL){
        return EDGE_ERR; 
    }else{
        return EDGE_OK;
}
}

edge_status edge_rrpc_response(char *topic,char *payload, int payloadLen)
{
    edge_status status; 
    char response_topic[128];
    _replace_str(response_topic, topic, "request", "response");
    status = edge_publish(response_topic, payload, payloadLen);
    if(EDGE_OK != status){
        log_write(LOG_ERROR, "edge_publish rrpc fail");
        return EDGE_ERR;
    }
    log_write(LOG_DEBUG, "edge_publish rrpc :%s",payload);
    return EDGE_OK;
}



