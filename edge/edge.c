#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "edge.h"
#include <signal.h>
#include <sys/time.h>

/*  global variable start  */
char edge_router_subject[NATS_SUBJECT_MAX_LEN] = {0};
char edge_local_subject[NATS_SUBJECT_MAX_LEN] = {0};
static cJSON *driver_config_json = NULL;
static char *driver_id = NULL;
char *driver_name = NULL;
static char *driver_info = NULL;
static char *device_info = NULL;
static char *productSN = NULL;
static char *deviceSN = NULL;
List *requestid_list;
natsMutex *requestid_list_mutex;
List *conn_device_list;
natsMutex *conn_device_list_mutex;
natsConnection *conn = NULL;
natsSubscription *sub = NULL;
bool edge_state = false;
edge_topo_notify_handler  edge_topo_handle = NULL;
edge_subdev_status_handler edge_subdev_status_handle = NULL;
/*  global variable end  */

static edge_status _edge_subscribe(const char *subject, natsMsgHandler cb, void *cbClosure)
{
    edge_status status;    
    
    status = natsConnection_Subscribe(&sub, conn, subject, cb, cbClosure);
    status |= natsConnection_Flush(conn);
    return status;
}

static cJSON *_get_driver_cfg(void)
{
    if(NULL == driver_config_json)
    {
        unsigned int file_len = 0;
        FILE *fp;

        file_len = calc_file_len(CONFIG_FILE_PATH);
        char *driver_config_str_tmp = (char *)EDGE_MALLOC(file_len + 1);
        if(NULL == driver_config_str_tmp)
        {
            log_write(LOG_ERROR, "file_str malloc fail!");
            return NULL;
        }
        memset(driver_config_str_tmp, 0, file_len + 1);
        
        fp = fopen(CONFIG_FILE_PATH, "r");
        if(NULL == fp)
        {
            log_write(LOG_ERROR, "cannot open file:%s", CONFIG_FILE_PATH);
            return NULL;
        }
        if(file_len != fread(driver_config_str_tmp, 1, file_len, fp))
        {
            log_write(LOG_ERROR, "fread file:%s fail!", CONFIG_FILE_PATH);
        }
        driver_config_str_tmp[file_len] = '\0';
        fclose(fp);
        driver_config_json = cJSON_Parse(driver_config_str_tmp);
        EDGE_FREE(driver_config_str_tmp);
    }
    return driver_config_json;
}

static int _parse_driver_cfg(void)
{
    if(NULL == driver_id || NULL == driver_name)
    {
        if(NULL == _get_driver_cfg())
        {
            log_print("_get_driver_cfg fail!\r\n");
            return EDGE_ERR;
        }
        
        cJSON *driver_id_json = cJSON_GetObjectItem(_get_driver_cfg(), "driverID");
        if(NULL == driver_id_json)
        {
            log_print("parse driverID fail!\r\n");
            return EDGE_ERR;
        }
        driver_id = driver_id_json->valuestring;
        log_print("driverId:%s\r\n", driver_id);   

        cJSON *driver_name_json = cJSON_GetObjectItem(_get_driver_cfg(), "driverName");
        if(NULL == driver_name_json)
        {
            log_print("parse driverName fail!\r\n");
            return EDGE_ERR;
        }

        driver_name = driver_name_json->valuestring;
        log_print("driverName:%s\r\n", driver_name);   
        return EDGE_OK;
    }
    return EDGE_OK;
}

char * edge_get_driver_info(void)
{
    if(NULL == driver_info)
    {
        if(NULL == _get_driver_cfg())
        {
            log_write(LOG_ERROR, "_get_driver_cfg fail!");
            return NULL;
        }
        
        cJSON *driver_info_json = cJSON_GetObjectItem(_get_driver_cfg(), "driverInfo");
        if(NULL == driver_info_json)
        {
            log_write(LOG_ERROR, "parse driverInfo fail!");
            return NULL;
        }

        driver_info = cJSON_Print(driver_info_json);
        log_write(LOG_DEBUG, "driverInfo:%s", driver_info);  
    }
    return driver_info;
}

char * edge_get_device_info(void)
{
    if(NULL == device_info)
    {
        if(NULL == _get_driver_cfg())
        {
            log_write(LOG_ERROR, "_get_driver_cfg fail!");
            return NULL;
        }
        
        cJSON *device_list_json = cJSON_GetObjectItem(_get_driver_cfg(), "deviceList");
        if(NULL == device_list_json)
        {
            log_write(LOG_ERROR, "parse driverInfo fail!");
            return NULL;
        }
        
        device_info = cJSON_Print(device_list_json);
        log_write(LOG_DEBUG, "deviceList:%s", device_info);    
    }
    return device_info;
}

char * edge_get_product_sn(void)
{
    if(NULL == productSN)
    {
        if(NULL == _get_driver_cfg())
        {
            log_write(LOG_ERROR, "_get_driver_cfg fail!");
            return NULL;
        }
        
        cJSON *productsn_json = cJSON_GetObjectItem(_get_driver_cfg(), "productSN");
        if(NULL == productsn_json)
        {
            log_write(LOG_ERROR, "parse productSN fail!");
            return NULL;
        }
        
        productSN = productsn_json->valuestring;
        log_write(LOG_DEBUG, "productSN:%s", productSN);    
    }
    return productSN;
}

char * edge_get_device_sn(void)
{
    if(NULL == deviceSN)
    {
        if(NULL == _get_driver_cfg())
        {
            log_write(LOG_ERROR, "_get_driver_cfg fail!");
            return NULL;
        }
        
        cJSON *devicesn_json = cJSON_GetObjectItem(_get_driver_cfg(), "deviceSN");
        if(NULL == devicesn_json)
        {
            log_write(LOG_ERROR, "parse deviceSN fail!");
            return NULL;
        }
        
        deviceSN = devicesn_json->valuestring;
        log_write(LOG_DEBUG, "deviceSN:%s", deviceSN);    
    }
    return deviceSN;
}


void edge_set_topo_notify_handle(edge_topo_notify_handler          topo_notify_handle)
{
    if(NULL != topo_notify_handle)
        edge_topo_handle = topo_notify_handle;
    return;
}

void edge_set_subdev_status_handle(edge_subdev_status_handler subdev_status_handle)
{
    if(NULL != subdev_status_handle)
        edge_subdev_status_handle = subdev_status_handle;
    return;
}

static void _on_local_message(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure)
{
    log_write(LOG_DEBUG, "Received local msg: %s - %.*s", natsMsg_GetSubject(msg), natsMsg_GetDataLength(msg), natsMsg_GetData(msg));

    cJSON *msg_json = cJSON_Parse(natsMsg_GetData(msg));
    if (!msg_json) 
    {
        log_write(LOG_ERROR, "on local message json parse error: [%s]",cJSON_GetErrorPtr());        
        natsMsg_Destroy(msg);
        return;
    }
    cJSON *topic = cJSON_GetObjectItem(msg_json, "topic");
    if(NULL == topic)
    {
        log_write(LOG_ERROR, "cannot find topic, illegal msg");
        cJSON_Delete(msg_json);        
        natsMsg_Destroy(msg);
        return;
    }
    log_write(LOG_DEBUG, "_on_broadcast_message topic:%s", topic->valuestring);
    
    cJSON *device_sn_tmp = cJSON_GetObjectItem(msg_json, "deviceSN");
    if(NULL == device_sn_tmp)
    {
        log_write(LOG_ERROR, "cannot find topic, illegal msg");
        cJSON_Delete(msg_json);        
        natsMsg_Destroy(msg);
        return;
    }
    log_write(LOG_DEBUG, "_on_local_message deviceSN:%s", device_sn_tmp->valuestring);
    
    cJSON *msg_base64code = cJSON_GetObjectItem(msg_json, "payload");
    if(NULL == msg_base64code)
    {
        log_write(LOG_ERROR, "cannot find payload, illegal msg");   
        cJSON_Delete(msg_json);
        natsMsg_Destroy(msg);        
        return;
    }
    log_write(LOG_DEBUG, "_on_local_message msg_base64code:%s", msg_base64code->valuestring);
    char *msg_base64decode = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == msg_base64decode)
    {
        log_write(LOG_ERROR, "msg_base64decode malloc fail!");
        cJSON_Delete(msg_json);        
        natsMsg_Destroy(msg);
        return;
    }
    memset(msg_base64decode, 0, NATS_MSG_MAX_LEN);
    base64_decode(msg_base64code->valuestring, strlen(msg_base64code->valuestring), msg_base64decode);
    log_write(LOG_DEBUG, "_on_local_message msg_base64decode:%s", msg_base64decode);
    
    subdev_client subdev_client_tmp = {0};
    subdev_client *pst_subdev_client = NULL;
    subdev_client_tmp.device_sn = device_sn_tmp->valuestring;
    ListNode *node_tmp = _find_in_list(conn_device_list, (void *)&subdev_client_tmp, conn_device_list_mutex);
    if(NULL == node_tmp)
    {
        log_write(LOG_WARN, "cannot find conn device[%s]", subdev_client_tmp.device_sn);
        goto end;
    }
    pst_subdev_client = (subdev_client *)node_tmp->val;
    if(pst_subdev_client->normal_msg_handle)
        pst_subdev_client->normal_msg_handle(topic->valuestring, msg_base64decode);

end:
    cJSON_Delete(msg_json);
    EDGE_FREE(msg_base64decode);    
    natsMsg_Destroy(msg);    
    return;
}

static void _on_broadcast_message(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure)
{
    log_write(LOG_DEBUG, "Received broadcast msg: %s - %.*s", natsMsg_GetSubject(msg), natsMsg_GetDataLength(msg), natsMsg_GetData(msg));
    
    cJSON *msg_json = cJSON_Parse(natsMsg_GetData(msg));
    if (!msg_json) 
    {
        log_write(LOG_ERROR, "on broadcast message json parse msg error: [%s]",cJSON_GetErrorPtr());        
        natsMsg_Destroy(msg);
        return;
    }
    cJSON *topic = cJSON_GetObjectItem(msg_json, "topic");
    if(NULL == topic)
    {
        log_write(LOG_ERROR, "cannot find topic, illegal msg");
        cJSON_Delete(msg_json);        
        natsMsg_Destroy(msg);
        return;
    }
    log_write(LOG_DEBUG, "_on_broadcast_message topic:%s", topic->valuestring);
    cJSON *msg_base64code = cJSON_GetObjectItem(msg_json, "payload");
    if(NULL == msg_base64code)
    {
        log_write(LOG_ERROR, "cannot find payload, illegal msg");
        cJSON_Delete(msg_json);        
        natsMsg_Destroy(msg);
        return;
    }
    log_write(LOG_DEBUG, "_on_broadcast_message msg_base64code:%s", msg_base64code->valuestring);
    char *msg_base64decode = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == msg_base64decode)
    {
        log_write(LOG_ERROR, "msg_base64decode malloc fail!");
        cJSON_Delete(msg_json);        
        natsMsg_Destroy(msg);
        return;
    }
    memset(msg_base64decode, 0, NATS_MSG_MAX_LEN);
    base64_decode(msg_base64code->valuestring, strlen(msg_base64code->valuestring), msg_base64decode);
    log_write(LOG_DEBUG, "_on_broadcast_message msg_base64decode:%s", msg_base64decode);
    
    if(NULL != strstr(topic->valuestring, "/subdev/topo/notify/add"))
    {
        log_write(LOG_DEBUG, "add topo");
        if(edge_topo_handle)
            edge_topo_handle(TOPO_ADD, msg_base64decode);
    }else if(NULL != strstr(topic->valuestring, "/subdev/topo/notify/delete"))
    {
        log_write(LOG_DEBUG, "delete topo");
        if(edge_topo_handle)
            edge_topo_handle(TOPO_DELETE, msg_base64decode);
    }else if(NULL != strstr(topic->valuestring, "/subdev/enable"))
    {
        log_write(LOG_DEBUG, "enable subdev");
        if(edge_subdev_status_handle)
            edge_subdev_status_handle(SUBDEV_ENABLE, msg_base64decode);
    }else if(NULL != strstr(topic->valuestring, "/subdev/disable"))
    {
        log_write(LOG_DEBUG, "disable subdev");
        if(edge_subdev_status_handle)
            edge_subdev_status_handle(SUBDEV_DISABLE, msg_base64decode);
    }else
    {    
        cJSON *msg_base64decode_json = cJSON_Parse(msg_base64decode);        
        if (!msg_base64decode_json) 
        {
            log_write(LOG_ERROR, "on broadcast message json parse msg_base64 error: [%s]\n",cJSON_GetErrorPtr());
            goto end;
        }
        cJSON *request_id_json = cJSON_GetObjectItem(msg_base64decode_json, "RequestID");
        log_write(LOG_DEBUG, "request_id_str:%s", request_id_json->valuestring);
        msg_parse msg_parse_tmp = {0};
        msg_parse_tmp.request_id = atol(request_id_json->valuestring);        
        cJSON_Delete(msg_base64decode_json);
        ListNode *node_tmp = _find_in_list(requestid_list, (void *)&msg_parse_tmp, requestid_list_mutex);
        if(NULL == node_tmp)
        {
            log_write(LOG_WARN, "cannot find msg request_id[%ld]", msg_parse_tmp.request_id);            
            goto end;
        }
        strncpy(((msg_parse *)node_tmp->val)->payload, msg_base64decode, NATS_MSG_MAX_LEN >= strlen(msg_base64decode)?strlen(msg_base64decode):NATS_MSG_MAX_LEN);
        (((msg_parse *)node_tmp->val)->payload)[strlen(msg_base64decode)] = '\0';
        log_write(LOG_DEBUG, "msg_parse_str->payload:%s",(((msg_parse *)node_tmp->val)->payload));          
    }
end:    
    cJSON_Delete(msg_json);
    EDGE_FREE(msg_base64decode);
    natsMsg_Destroy(msg);
    return;
}

static void _on_edge_state_message(natsConnection *nc, natsSubscription *sub, natsMsg *msg, void *closure)
{
    log_write(LOG_DEBUG, "Received state msg: %s - %.*s", natsMsg_GetSubject(msg), natsMsg_GetDataLength(msg), natsMsg_GetData(msg));

    cJSON *msg_json = cJSON_Parse(natsMsg_GetData(msg));
    if (!msg_json) 
    {
        log_write(LOG_ERROR, "on broadcast message json parse error: [%s]",cJSON_GetErrorPtr());        
        natsMsg_Destroy(msg);
        return;
    }
    cJSON *state = cJSON_GetObjectItem(msg_json, "state");
    if(NULL == state)
    {
        log_write(LOG_ERROR, "cannot find state, illegal msg");        
        natsMsg_Destroy(msg);
        return;
    }
    char *state_str = cJSON_Print(state);
    log_write(LOG_DEBUG, "_on_edge_statue_message state:%s", state_str);
    if(0 == strncmp(state_str, "true", 4))
    {
        edge_state = true;
    }
    else
    {
        edge_state = false;
    }

    EDGE_FREE(state);    
    natsMsg_Destroy(msg);
    return;
}

bool edge_get_online_status(void)
{
    return edge_state;
}

static void _msg_parse_free(void *val)
{
    msg_parse* msg_parse_tmp = (msg_parse*)val;
    EDGE_FREE(msg_parse_tmp->payload);
    EDGE_FREE(msg_parse_tmp);
    return;
}

static int _msg_parse_match(void *a, void *b)
{
    msg_parse* msg_parse_a = (msg_parse*)a;
    msg_parse* msg_parse_b = (msg_parse*)b;

    if(msg_parse_a->request_id == msg_parse_b->request_id)
        return true;
    else
        return false;
}

static void _subdev_client_free(void *val)
{
    subdev_client *client = (subdev_client *)val;
    
    EDGE_FREE(client);
    return;
}

static int _subdev_client_match(void *a, void *b)
{
    subdev_client *subdev_client_a = (subdev_client *)a;
    subdev_client *subdev_client_b = (subdev_client *)b;

    if(0 == strncmp(subdev_client_a->device_sn, subdev_client_b->device_sn, strlen(subdev_client_a->device_sn)))
        return true;
    else
        return false;
}

static void _handle_status_sync(int signo)
{
    edge_status status;
    char *device_list_msg = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == device_list_msg)
    {
        log_write(LOG_ERROR, "device_list_msg malloc fail!");
        return;
    }
    memset(device_list_msg, 0, NATS_MSG_MAX_LEN);

    char *tmp_msg = (char *)EDGE_MALLOC(100);
    if(NULL == tmp_msg)
    {
        log_write(LOG_ERROR, "tmp_msg malloc fail!");
        EDGE_FREE(device_list_msg);
        return;
    }
    memset(tmp_msg, 0, 100);

    char *sync_msg = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == sync_msg)
    {
        log_write(LOG_ERROR, "sync_msg malloc fail!");
        EDGE_FREE(device_list_msg);     
        EDGE_FREE(tmp_msg);
        return;
    }
    memset(sync_msg, 0, NATS_MSG_MAX_LEN);
    
    natsMutex_Lock(conn_device_list_mutex);
    if (conn_device_list->len) 
    {
        ListIterator *iter;
        ListNode *node = NULL;
        subdev_client *nats_handle = NULL;

        if (NULL == (iter = _list_iterator_new(conn_device_list, LIST_TAIL))) 
        {
            return;
        }

        for (;;) 
        {
            node = _list_iterator_next(iter);
            if (NULL == node) 
            {
                break;
            }
            
            nats_handle = (subdev_client *)(node->val);
            if (NULL == nats_handle) {
                log_write(LOG_ERROR, "node's value is invalid!");
                continue;
            }

            snprintf(tmp_msg, 100, DEVICE_LIST_INFO, nats_handle->product_sn, nats_handle->device_sn);
            log_write(LOG_DEBUG, "tmp_msg:%s",tmp_msg);
            if(0 == strlen(device_list_msg))
            {
                strncat(device_list_msg, tmp_msg, strlen(tmp_msg));
            }
            else
            {
                strncat(device_list_msg, ",", 1);
                strncat(device_list_msg, tmp_msg, strlen(tmp_msg));
            }            
            log_write(LOG_DEBUG, "device_list_msg:%s",device_list_msg);
        }
        
        _list_iterator_destroy(iter);
    }
    natsMutex_Unlock(conn_device_list_mutex);

    snprintf(sync_msg, NATS_MSG_MAX_LEN, STATUS_SYNC_FORMAT, driver_id, device_list_msg);
    log_write(LOG_DEBUG, "sync_msg:%s",sync_msg);
    status = natsConnection_PublishString(conn, EDGE_STATE_REQ_SUBJECT, sync_msg);
    status |= natsConnection_Flush(conn);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "publish sync msg fail!");
        return;
    }
    
    return;
}

static void _fetch_online_status(void)
{
    nats_timer nats_timer;
    nats_timer.interval_sec = 15;
    nats_timer.interval_usec = 0;
    nats_timer.effect_sec = 15;
    nats_timer.effect_usec = 0;

    natsTimer_Init(&nats_timer, _handle_status_sync);

    return;
}

edge_status edge_common_init(void)
{
    edge_status status;
    char *nats_server_url = NULL;
    
    requestid_list = _list_new();
    if (requestid_list)
    {
        requestid_list->free = _msg_parse_free;
        requestid_list->match = _msg_parse_match;
    }
    else
    {
        log_print("requestid_list malloc fail!\r\n");
        return EDGE_NO_MEMORY;
    }
    
    if ((edge_status)natsMutex_Create(&(requestid_list_mutex)) != EDGE_OK)
    {
        return EDGE_NO_MEMORY;
    }
    
    conn_device_list = _list_new();
    if (conn_device_list)
    {
        conn_device_list->free = _subdev_client_free;
        conn_device_list->match = _subdev_client_match;
    }
    else
    {
        log_print("conn_device_list malloc fail!\r\n");
        return EDGE_NO_MEMORY;
    }
    
    if ((edge_status)natsMutex_Create(&(conn_device_list_mutex)) != EDGE_OK)
    {
        return EDGE_NO_MEMORY;
    }

    // nats server取IOTEDGE_NATS_ADDRESS环境或者本地host
    nats_server_url =  getenv("IOTEDGE_NATS_ADDRESS");
    status = natsConnection_ConnectTo(&conn, nats_server_url != NULL?nats_server_url:NATS_SERVER_DEFAULT_URL);
    if(EDGE_OK != status)
    {
        log_print("connect nats fail!\r\n");
        return EDGE_PROTOCOL_ERROR;
    }

    if(EDGE_ERR == _parse_driver_cfg())
    {
        log_print("pasrse driver cfg fail!\r\n");
        return EDGE_ERR;
    }

    snprintf(edge_router_subject, NATS_SUBJECT_MAX_LEN, EDGE_ROUTER_SUBJECT_FORMAT, driver_id);
    log_write(LOG_DEBUG, "edge_router_subject:%s",edge_router_subject);
    snprintf(edge_local_subject, NATS_SUBJECT_MAX_LEN, EDGE_LOCAL_SUBJECT_FORMAT, driver_id);
    log_write(LOG_DEBUG, "edge_local_subject:%s",edge_local_subject);
    
    //sub edge.local.driverId
    status = _edge_subscribe(edge_local_subject, _on_local_message, NULL);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "edge_subscribe %s fail! status:%d", edge_local_subject, status);
        return EDGE_PROTOCOL_ERROR;
    }

    //edge.local.broadcast
    status = _edge_subscribe(EDGE_LOCAL_BROADCAST_SUBJECT, _on_broadcast_message, NULL);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "edge_subscribe %s fail! status:%d", EDGE_LOCAL_BROADCAST_SUBJECT, status);
        return EDGE_PROTOCOL_ERROR;
    }

    //edge.state.reply
    status = _edge_subscribe(EDGE_STATE_REPLY_SUBJECT, _on_edge_state_message, NULL);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "edge_subscribe %s fail! status:%d", EDGE_STATE_REPLY_SUBJECT, status);
        return EDGE_PROTOCOL_ERROR;
    }
    
    _fetch_online_status();

    return EDGE_OK;
}

static edge_status _topo_msg_common(subdev_client *pst_subdev_client, uint32_t request_id, topo_operation op)
{
    edge_status status;
    char *topo_msg = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == topo_msg)
    {
        log_write(LOG_ERROR, "topo_msg malloc fail!");
        return EDGE_NO_MEMORY;
    }
    memset(topo_msg, 0, NATS_MSG_MAX_LEN);
    
    char *subdev_str = (char *)EDGE_MALLOC(100);
    if(NULL == subdev_str)
    {
        log_write(LOG_ERROR, "subdev_str malloc fail!");
        EDGE_FREE(topo_msg);
        return EDGE_NO_MEMORY;
    }
    memset(subdev_str, 0, 100);

    char *payload_str = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == payload_str)
    {
        log_write(LOG_ERROR, "payload_str malloc fail!");
        EDGE_FREE(topo_msg);
        EDGE_FREE(subdev_str);
        return EDGE_NO_MEMORY;
    }
    memset(payload_str, 0, NATS_MSG_MAX_LEN);

    char *payload_str_base64 = (char *)EDGE_MALLOC(NATS_MSG_MAX_LEN);
    if(NULL == payload_str_base64)
    {
        log_write(LOG_ERROR, "payload_str_base64 malloc fail!");
        EDGE_FREE(topo_msg);
        EDGE_FREE(subdev_str);
        EDGE_FREE(payload_str);    
        return EDGE_NO_MEMORY;
    }
    memset(payload_str_base64, 0, NATS_MSG_MAX_LEN);
    
    switch(op)
    {
        case TOPO_GET:
            snprintf(payload_str, NATS_MSG_MAX_LEN, TOPO_OP_FORMAT_PAYLOAD, request_id, "");
            log_write(LOG_DEBUG, "get payload_str:%s",payload_str);
            base64_encode(payload_str, strlen(payload_str), payload_str_base64);
            log_write(LOG_DEBUG, "get payload_str_base64:%s",payload_str_base64);
            snprintf(topo_msg, NATS_MSG_MAX_LEN, TOPO_OP_FORMAT, "i5ztbdmjr8yhlv9a", "123456", "get", payload_str_base64);            
            break;
        case TOPO_ADD:
            snprintf(subdev_str, 100, "{\"ProductSN\": \"%s\",\"DeviceSN\": \"%s\"}", pst_subdev_client->product_sn, pst_subdev_client->device_sn);
            snprintf(payload_str, NATS_MSG_MAX_LEN, TOPO_OP_FORMAT_PAYLOAD, request_id, subdev_str);
            log_write(LOG_DEBUG, "add payload_str:%s",payload_str);
            base64_encode(payload_str, strlen(payload_str), payload_str_base64);
            log_write(LOG_DEBUG, "add payload_str_base64:%s",payload_str_base64);
            snprintf(topo_msg, NATS_MSG_MAX_LEN, TOPO_OP_FORMAT, pst_subdev_client->product_sn, pst_subdev_client->device_sn, "add", payload_str_base64);
            break;
        case TOPO_DELETE:
            snprintf(subdev_str, 100, "{\"ProductSN\": \"%s\",\"DeviceSN\": \"%s\"}", pst_subdev_client->product_sn, pst_subdev_client->device_sn);
            snprintf(payload_str, NATS_MSG_MAX_LEN, TOPO_OP_FORMAT_PAYLOAD, request_id, subdev_str);
            log_write(LOG_DEBUG, "delete payload_str:%s",payload_str);
            base64_encode(payload_str, strlen(payload_str), payload_str_base64);
            log_write(LOG_DEBUG, "delete payload_str_base64:%s",payload_str_base64);
            snprintf(topo_msg, NATS_MSG_MAX_LEN, TOPO_OP_FORMAT, pst_subdev_client->product_sn, pst_subdev_client->device_sn, "delete", payload_str_base64);
            break;
        default:
            log_write(LOG_ERROR, "error operation!");  
            status = EDGE_ERR;
            goto end;
    }

    /* pub get topo msg */
    status = _publish_string(edge_router_subject, topo_msg);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "pub get topo msg fail");
    }
    
end:    
    EDGE_FREE(topo_msg);
    EDGE_FREE(subdev_str);
    EDGE_FREE(payload_str);    
    EDGE_FREE(payload_str_base64);
    return status;
}

char *edge_get_topo(uint32_t time_out_ms)
{
    edge_status status;
    uint32_t request_id = _gen_requestid();
    msg_parse *msg_parse_str = NULL;
    if(EDGE_OK != _msg_parse_str_init(&msg_parse_str, request_id))
    {
        return NULL;
    }
    _add_to_list(requestid_list, (void *)msg_parse_str, requestid_list_mutex);
    status = _topo_msg_common(NULL, request_id, TOPO_GET);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "publish get topo fail");
        return NULL;
    }

    status = _wait_payload_is_timeout(msg_parse_str, time_out_ms);
    if(EDGE_INVALID_TIMEOUT == status)
    {
        log_write(LOG_ERROR, "get topo timeout");
        return NULL;
    }

    cJSON *msg_parse_str_payload = cJSON_Parse(msg_parse_str->payload);    
    if (!msg_parse_str_payload) 
    {
        log_write(LOG_ERROR, "json parse msg_parse_str payload error: [%s]",cJSON_GetErrorPtr());
        return NULL;
    }
    cJSON *Data_str = cJSON_GetObjectItem(msg_parse_str_payload, "Data");    
    _remove_from_list(requestid_list, (void *)msg_parse_str, requestid_list_mutex);    
    if(NULL != Data_str)
    {
        return Data_str->valuestring;
    }
    else
    {
        return NULL;
    }
}

edge_status edge_add_topo(subdev_client *pst_subdev_client, uint32_t time_out_ms)
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
    _add_to_list(requestid_list, (void *)msg_parse_str, requestid_list_mutex);
    status = _topo_msg_common(pst_subdev_client, request_id, TOPO_ADD);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "publish add topo fail");
        goto end;
    }

    status = _wait_payload_is_timeout(msg_parse_str, time_out_ms);
    if(EDGE_INVALID_TIMEOUT == status)
    {
        log_write(LOG_ERROR, "add topo timeout");
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
        log_write(LOG_INFO, "add topo device[%s] successful", pst_subdev_client->device_sn);
        status = EDGE_OK;        
        cJSON_Delete(msg_parse_str_payload);
        goto end;
    }

    log_write(LOG_ERROR, "add topo device[%s] fail", pst_subdev_client->device_sn);
    status = EDGE_PROTOCOL_ERROR;
end:    
    _remove_from_list(requestid_list, (void *)msg_parse_str, requestid_list_mutex);    
    return status;
}

edge_status edge_delete_topo(subdev_client *pst_subdev_client, uint32_t time_out_ms)
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
    _add_to_list(requestid_list, (void *)msg_parse_str, requestid_list_mutex);
    status = _topo_msg_common(pst_subdev_client, request_id, TOPO_DELETE);
    if(EDGE_OK != status)
    {
        log_write(LOG_ERROR, "publish delete topo fail");
        goto end;
    }

    status = _wait_payload_is_timeout(msg_parse_str, time_out_ms);
    if(EDGE_INVALID_TIMEOUT == status)
    {
        log_write(LOG_ERROR, "del topo timeout");
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
        log_write(LOG_INFO, "delete topo device[%s] successful", pst_subdev_client->device_sn);
        status = EDGE_OK;        
        cJSON_Delete(msg_parse_str_payload);
        goto end;
    }

    log_write(LOG_ERROR, "delete topo device[%s] fail", pst_subdev_client->device_sn);
    status = EDGE_PROTOCOL_ERROR;
end:
    _remove_from_list(requestid_list, (void *)msg_parse_str, requestid_list_mutex);    
    return status;
}

