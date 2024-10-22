/**
 * @file thread_queue.h
 * @author Jiemin Cao(jiemin.cao@rafaelmicro.com)
 * @brief
 * @version 0.1
 * @date 2023-08-16
 *
 * @copyright Copyright (c) 2022
 *
 */

#ifndef __THREAD_QUEUE_H__
#define __THREAD_QUEUE_H__

#include "util_queue.h"

#ifdef __cplusplus
extern "C" {
#endif
//=============================================================================
//                Include (Better to prevent)
//=============================================================================

//=============================================================================
//                Public Definitions of const value
//=============================================================================

//=============================================================================
//                Public ENUM
//=============================================================================

//=============================================================================
//                Public Struct
//=============================================================================
// thread queue struct
typedef struct
{
    queue_t thread_queue;
    uint32_t size; //queue sizes
    uint32_t count; // current number of queues
    uint32_t dataSize; // ponter sizes
} thread_queue_t;
//=============================================================================
//                Public Function Declaration
//=============================================================================
void thread_queue_init(thread_queue_t *queue, int size, uint32_t dataSize);
int thread_enqueue_fn(thread_queue_t *queue, void *data, const char *pc_func_ptr, uint32_t u32_line);
#define thread_enqueue(queue, data) thread_enqueue_fn(queue, data, __FUNCTION__, __LINE__)
int thread_dequeue_fn(thread_queue_t *queue, void *data, const char *pc_func_ptr, uint32_t u32_line);
#define thread_dequeue(queue, data) thread_dequeue_fn(queue, data, __FUNCTION__, __LINE__)
#ifdef __cplusplus
};
#endif
#endif /* __OTA_HANDLER_H__ */
