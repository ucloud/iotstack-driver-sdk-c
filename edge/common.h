#ifndef _COMMON_H
#define _COMMON_H

#include <stdarg.h>
#include "utils_list.h"
#include "cJSON.h"
#include "nats.h"
#include "n-unix.h"

#define EDGE_MALLOC(s)      malloc((s))
#define EDGE_CALLOC(c,s)    calloc((c), (s))
#define EDGE_REALLOC(p, s)  realloc((p), (s))
#define EDGE_FREE(p)        free((p))

#define LOG_FILE_PATH                       "./var/log/iotedge/driver.log.%04d.%02d.%02d"

#define NATS_SERVER_DEFAULT_URL             "tcp://127.0.0.1:4222"

#define TOPO_OP_FORMAT                      "{\"src\": \"local\",\"topic\": \"/$system/%s/%s/subdev/topo/%s\",\"payload\": \"%s\"}"
#define TOPO_OP_FORMAT_PAYLOAD              "{\"RequestID\": \"%u\",\"Params\": [%s]}"

#define STATUS_SYNC_FORMAT                  "{\"driverID\":\"%s\",\"devices\":[%s]}"
#define DEVICE_LIST_INFO                    "{\"productSN\": \"%s\",\"deviceSN\": \"%s\"}"

#define EDGE_ROUTER_SUBJECT_FORMAT          "edge.router.%s"                //edge.router.driveId
#define EDGE_LOCAL_SUBJECT_FORMAT           "edge.local.%s"                 //edge.local.driveId
#define EDGE_LOCAL_BROADCAST_SUBJECT        "edge.local.broadcast"          //edge reply
#define EDGE_STATE_REPLY_SUBJECT            "edge.state.reply"              //state sync reply
#define EDGE_STATE_REQ_SUBJECT              "edge.state.req"                //state sync
#define EDGE_LOG_UPLOAD_SUBJECT             "edge.log.upload"               //log update
#define EDGE_LOG_SET_SUBJECT                "edge.log.set"                  //set log

#define CONFIG_FILE_PATH                    "./etc/iotedge/config.json"    

#define NATS_MSG_MAX_LEN                    2048
#define NATS_SUBJECT_MAX_LEN                100

#define NORMAL_MSG_FORMAT                   "{\"src\": \"local\",\"topic\": \"%s\",\"payload\": \"%s\"}"

#define DYN_REG_FORMAT                      "{\"src\": \"local\",\"topic\": \"/$system/%s/%s/subdev/register\",\"payload\": \"%s\"}"
#define DYN_REG_FORMAT_PAYLOAD              "{\"RequestID\": \"%u\",\"Params\": [{\"ProductSN\": \"%s\",\"DeviceSN\": \"%s\",\"ProductSecret\": \"%s\"}]}"

#define LOG_IN_OUT_FORMAT                   "{\"src\": \"local\",\"topic\": \"/$system/%s/%s/subdev/%s\",\"payload\": \"%s\"}"
#define LOG_IN_OUT_FORMAT_PAYLOAD           "{\"RequestID\": \"%u\",\"Params\": [{\"ProductSN\": \"%s\",\"DeviceSN\": \"%s\"}]}"

#define LOG_UPLOAD_FORMAT                   "{\"module\": \"drv_%s\",\"level\": \"%s\",\"message\": \"%s\",\"timestamp\": %ld}"
#define LOG_SET_FORMAT                      "{\"level\": \"%s\",\"size\": %d,\"number\": %d}"

extern char *driver_name;
extern natsConnection *conn;
extern char edge_router_subject[];
extern List *requestid_list;
extern natsMutex *requestid_list_mutex;
extern List *conn_device_list;
extern natsMutex *conn_device_list_mutex;

typedef struct
{
    unsigned int interval_sec;
    unsigned int interval_usec;
    unsigned int effect_sec;
    unsigned int effect_usec;
}nats_timer;

typedef enum
{
    LOG_DEBUG = 0,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
}log_level;

uint32_t _gen_requestid(void);
natsStatus _add_to_list(List * list, void *node, natsMutex *mutex);
ListNode * _find_in_list(List * list, void *node, natsMutex *mutex);
natsStatus _remove_from_list(List * list, void *node, natsMutex *mutex);
int base64_encode(const char *indata, int inlen, char *outdata);
int base64_decode(const char *indata, int inlen, char *outdata);
int calc_file_len(const char *file_path);
natsStatus natsMutex_Create(natsMutex **newMutex);
void natsMutex_Lock(natsMutex *m);
void natsMutex_Unlock(natsMutex *m);
void natsMutex_Destroy(natsMutex *m);


#endif

