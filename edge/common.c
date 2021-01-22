#include <stdio.h>
#include <stdlib.h>
#include "common.h"

uint32_t _gen_requestid(void)
{
    uint32_t result = 0;
    result = rand();
    result <<= 16;
    result |= rand();
    return result;
}

natsStatus _add_to_list(List * list, void *node, natsMutex *mutex)
{
    ListNode *list_node = _list_node_new(node);
    if (NULL == list_node) 
    {
        printf("_add_to_list error!\r\n");
        return NATS_ERR;
    }
    else
    {
        natsMutex_Lock(mutex);
        _list_rpush(list, list_node);
        natsMutex_Unlock(mutex);
        return NATS_OK;
    }
}

ListNode * _find_in_list(List * list, void *node, natsMutex *mutex)
{
    ListNode *list_node;
    natsMutex_Lock(mutex);
    list_node = _list_find(list, node);
    natsMutex_Unlock(mutex);
    if(NULL != list_node)
    {
        return list_node;
    }
    else
    {
        return NULL;
    }
}

natsStatus _remove_from_list(List * list, void *node, natsMutex *mutex)
{
    ListNode *list_node;
    natsStatus status;
    natsMutex_Lock(mutex);
    list_node = _list_find(list, node);
    if (NULL == list_node) 
    {
        printf("_remove_from_list error!\r\n");
        status = NATS_ERR;
    } 
    else 
    {
        _list_remove(list, list_node);
        status = NATS_OK;
    }
    natsMutex_Unlock(mutex);
    return status;
}

// base64 转换表, 共64个
static const char base64_alphabet[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '+', '/'};

// 解码时使用
static const unsigned char base64_suffix_map[256] = {
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 255,
    255, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 253, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255,  62, 255, 255, 255,  63,
    52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255,
    255, 254, 255, 255, 255,   0,   1,   2,   3,   4,   5,   6,
    7,   8,   9,  10,  11,  12,  13,  14,  15,  16,  17,  18,
    19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255, 255,
    255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,
    37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
    49,  50,  51, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255 };

static char cmove_bits(unsigned char src, unsigned lnum, unsigned rnum) {
    src <<= lnum; // src = src << lnum;
    src >>= rnum; // src = src >> rnum;
    return src;
}

int base64_encode(const char *indata, int inlen, char *outdata) {
    
    int ret = 0; // return value
    if (indata == NULL || inlen == 0) {
        return ret = -1;
    }
    
    int in_len = 0; // 源字符串长度, 如果in_len不是3的倍数, 那么需要补成3的倍数
    int pad_num = 0; // 需要补齐的字符个数, 这样只有2, 1, 0(0的话不需要拼接, )
    if (inlen % 3 != 0) {
        pad_num = 3 - inlen % 3;
    }
    in_len = inlen + pad_num; // 拼接后的长度, 实际编码需要的长度(3的倍数)
        
    char *p = outdata; // 定义指针指向传出data的首地址
    
    //编码, 长度为调整后的长度, 3字节一组
    int i = 0;
    for (i = 0; i < in_len; i+=3) {
        int value = *indata >> 2; // 将indata第一个字符向右移动2bit(丢弃2bit)
        char c = base64_alphabet[value]; // 对应base64转换表的字符
        *p = c; // 将对应字符(编码后字符)赋值给outdata第一字节
        
        //处理最后一组(最后3字节)的数据
        if (i == inlen + pad_num - 3 && pad_num != 0) {
            if(pad_num == 1) {
                *(p + 1) = base64_alphabet[(int)(cmove_bits(*indata, 6, 2) + cmove_bits(*(indata + 1), 0, 4))];
                *(p + 2) = base64_alphabet[(int)cmove_bits(*(indata + 1), 4, 2)];
                *(p + 3) = '=';
            } else if (pad_num == 2) { // 编码后的数据要补两个 '='
                *(p + 1) = base64_alphabet[(int)cmove_bits(*indata, 6, 2)];
                *(p + 2) = '=';
                *(p + 3) = '=';
            }
        } else { // 处理正常的3字节的数据
            *(p + 1) = base64_alphabet[cmove_bits(*indata, 6, 2) + cmove_bits(*(indata + 1), 0, 4)];
            *(p + 2) = base64_alphabet[cmove_bits(*(indata + 1), 4, 2) + cmove_bits(*(indata + 2), 0, 6)];
            *(p + 3) = base64_alphabet[*(indata + 2) & 0x3f];
        }
        
        p += 4;
        indata += 3;
    }
        
    return ret;
}


int base64_decode(const char *indata, int inlen, char *outdata) {
    
    int ret = 0;
    if (indata == NULL || inlen <= 0 || outdata == NULL) {
        return ret = -1;
    }
    if (inlen % 4 != 0) { // 需要解码的数据不是4字节倍数
        return ret = -2;
    }
    
    int t = 0, x = 0, y = 0, i = 0;
    unsigned char c = 0;
    int g = 3;
    
    while (indata[x] != 0) {
        // 需要解码的数据对应的ASCII值对应base64_suffix_map的值
        c = base64_suffix_map[(unsigned int)indata[x++]];
        if (c == 255) return -1;// 对应的值不在转码表中
        if (c == 253) continue;// 对应的值是换行或者回车
        if (c == 254) { c = 0; g--; }// 对应的值是'='
        t = (t<<6) | c; // 将其依次放入一个int型中占3字节
        if (++y == 4) {
            outdata[i++] = (unsigned char)((t>>16)&0xff);
            if (g > 1) outdata[i++] = (unsigned char)((t>>8)&0xff);
            if (g > 2) outdata[i++] = (unsigned char)(t&0xff);
            y = t = 0;
        }
    }
    return ret;
}

int calc_file_len(const char *file_path)
{
    FILE *fp;
    fp = fopen(file_path, "r");
    if(NULL == fp)
    {
        printf("cannot open file:%s\r\n",file_path);
        return 0;
    }

    //计算文件大小，申请内存
    fseek(fp,0L,SEEK_END); 
    int file_len = ftell(fp);
    fclose(fp);
    return file_len;
}

natsStatus natsMutex_Create(natsMutex **newMutex)
{
    natsStatus          s = NATS_OK;
    pthread_mutexattr_t attr;
    natsMutex           *m = EDGE_CALLOC(1, sizeof(natsMutex));
    bool                noAttrDestroy = false;

    if (m == NULL)
        return NATS_NO_MEMORY;

    if ((s == NATS_OK)
        && (pthread_mutexattr_init(&attr) != 0)
        && (noAttrDestroy = true))
    {
        return NATS_SYS_ERROR;
    }

    if ((s == NATS_OK)
        && (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0))
    {
        return NATS_SYS_ERROR;
    }

    if ((s == NATS_OK)
        && (pthread_mutex_init(m, &attr) != 0))
    {
        return NATS_SYS_ERROR;
    }

    if (!noAttrDestroy)
        pthread_mutexattr_destroy(&attr);

    if (s == NATS_OK)
        *newMutex = m;
    else
        EDGE_FREE(m);

    return s;
}

void natsMutex_Lock(natsMutex *m)
{
    if (pthread_mutex_lock(m))
        abort();
}


void natsMutex_Unlock(natsMutex *m)
{
    if (pthread_mutex_unlock(m))
        abort();
}

void natsMutex_Destroy(natsMutex *m)
{
    if (m == NULL)
        return;

    pthread_mutex_destroy(m);
    EDGE_FREE(m);
}

void natsTimer_Init(nats_timer      *pst_nats_timer, timer_handle timer_handle_func)
{        
    struct itimerval timer;

    timer.it_interval.tv_sec = pst_nats_timer->interval_sec;
    timer.it_interval.tv_usec = pst_nats_timer->interval_usec;
    timer.it_value.tv_sec = pst_nats_timer->effect_sec;
    timer.it_value.tv_usec = pst_nats_timer->effect_usec;

    setitimer(ITIMER_REAL, &timer, NULL);//让它产生SIGVTALRM信号
    
    //为SIGALRM注册信号处理函数
    signal(SIGALRM, timer_handle_func);
    return;
}

