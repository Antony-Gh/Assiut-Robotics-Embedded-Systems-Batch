/**
  ******************************************************************************
  * @file           : usbd_conf.h
  * @brief          : Header for usbd_conf.c - USB Device configuration
  ******************************************************************************
  */

#ifndef __USBD_CONF_H__
#define __USBD_CONF_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

/* Common Config */
#define USBD_MAX_NUM_INTERFACES     1U
#define USBD_MAX_NUM_CONFIGURATION  1U
#define USBD_MAX_STR_DESC_SIZ       512U
#define USBD_SELF_POWERED           1U
#define USBD_DEBUG_LEVEL            0U

/* CDC Class Config */
#define USBD_CDC_INTERVAL           1000U

/* Memory management macros */
#define USBD_malloc                 malloc
#define USBD_free                   free
#define USBD_memset                 memset
#define USBD_memcpy                 memcpy
#define USBD_Delay                  HAL_Delay

/* DEBUG macros */
#if (USBD_DEBUG_LEVEL > 0U)
#define  USBD_UsrLog(...)   do { \
                            printf(__VA_ARGS__); \
                            printf("\n"); \
} while (0)
#else
#define USBD_UsrLog(...) do {} while (0)
#endif

#if (USBD_DEBUG_LEVEL > 1U)
#define  USBD_ErrLog(...) do { \
                            printf("ERROR: ") ; \
                            printf(__VA_ARGS__); \
                            printf("\n"); \
} while (0)
#else
#define USBD_ErrLog(...) do {} while (0)
#endif

#if (USBD_DEBUG_LEVEL > 2U)
#define  USBD_DbgLog(...)   do { \
                            printf("DEBUG : ") ; \
                            printf(__VA_ARGS__); \
                            printf("\n"); \
} while (0)
#else
#define USBD_DbgLog(...) do {} while (0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CONF_H__ */
