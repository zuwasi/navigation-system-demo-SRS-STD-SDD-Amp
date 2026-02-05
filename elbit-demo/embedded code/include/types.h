/**
 * @file types.h
 * @brief Standard type definitions for ARM Cortex-A7 bare-metal project
 * @note MISRA C 2023 and CERT C compliant
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* MISRA C 2023 Rule 4.6: Use fixed-width integer types */
typedef uint8_t   u8_t;
typedef uint16_t  u16_t;
typedef uint32_t  u32_t;
typedef uint64_t  u64_t;
typedef int8_t    s8_t;
typedef int16_t   s16_t;
typedef int32_t   s32_t;
typedef int64_t   s64_t;

/* Volatile types for hardware registers */
typedef volatile uint8_t   vu8_t;
typedef volatile uint16_t  vu16_t;
typedef volatile uint32_t  vu32_t;

/* Status codes - MISRA C 2023 Rule 10.1: Use enum for related constants */
typedef enum {
    STATUS_OK           = 0,
    STATUS_ERROR        = 1,
    STATUS_BUSY         = 2,
    STATUS_TIMEOUT      = 3,
    STATUS_INVALID_PARAM = 4,
    STATUS_NOT_READY    = 5
} status_t;

/* Boolean result type */
typedef enum {
    RESULT_FALSE = 0,
    RESULT_TRUE  = 1
} result_t;

/* NULL pointer check macro - CERT C EXP34-C */
#define IS_NULL_PTR(ptr)    ((ptr) == NULL)
#define IS_VALID_PTR(ptr)   ((ptr) != NULL)

/* Bit manipulation macros - MISRA C 2023 compliant */
#define BIT_SET(reg, bit)       ((reg) |= (1U << (bit)))
#define BIT_CLEAR(reg, bit)     ((reg) &= ~(1U << (bit)))
#define BIT_TOGGLE(reg, bit)    ((reg) ^= (1U << (bit)))
#define BIT_CHECK(reg, bit)     (((reg) & (1U << (bit))) != 0U)

/* Memory barrier for ARM */
#define DMB()   __asm__ volatile ("dmb" ::: "memory")
#define DSB()   __asm__ volatile ("dsb" ::: "memory")
#define ISB()   __asm__ volatile ("isb" ::: "memory")

#endif /* TYPES_H */
