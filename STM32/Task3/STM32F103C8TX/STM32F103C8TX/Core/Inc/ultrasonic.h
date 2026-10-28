/**
 * @file ultrasonic.h
 * @brief Professional Driver for HC-SR04 Ultrasonic Distance Sensor for STM32F103
 * @author Assiut Robotics Team
 * @date 2026-10-28
 *
 * @details This header defines the configuration structures, enumeration types,
 *          and API functions for non-blocking ultrasonic distance measurement.
 *          Measurement is performed using a 1-microsecond hardware timer and
 *          EXTI interrupt capture on the Echo pin.
 */

#ifndef __ULTRASONIC_H__
#define __ULTRASONIC_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/**
 * @brief Measurement units supported by the ultrasonic sensor driver.
 */
typedef enum {
    ULTRASONIC_UNIT_CM = 0,    /**< Distance in Centimeters */
    ULTRASONIC_UNIT_INCH = 1   /**< Distance in Inches */
} Ultrasonic_Unit_t;

/**
 * @brief Structure representing a GPIO pin mapping (Port + Pin).
 */
typedef struct {
    GPIO_TypeDef *port;        /**< Pointer to GPIO Port peripheral (e.g. GPIOA) */
    uint16_t pin;              /**< GPIO Pin bitmask (e.g. GPIO_PIN_3) */
} Ultrasonic_Pin_t;

/**
 * @brief Configuration and runtime state structure for an HC-SR04 Ultrasonic sensor instance.
 */
typedef struct {
    Ultrasonic_Pin_t trig;         /**< Trigger output pin configuration */
    Ultrasonic_Pin_t echo;         /**< Echo input pin configuration (with EXTI) */
    Ultrasonic_Unit_t unit;        /**< Configured measurement unit */
    volatile uint32_t distance;    /**< Most recently calculated distance */
    volatile uint32_t echo_start;  /**< Timestamp of echo rising edge in microseconds */
    volatile uint32_t echo_width;  /**< Measured echo pulse duration in microseconds */
    volatile uint8_t new_data_ready; /**< Status flag: 1 if fresh data ready, 0 otherwise */
} Ultrasonic_t;

/**
 * @brief Initializes the Ultrasonic Sensor GPIO pins and microsecond timer.
 * @param[in,out] us Pointer to an Ultrasonic_t configuration structure.
 * @retval None
 */
void Ultrasonic_Init(Ultrasonic_t *us);

/**
 * @brief Sends a 10-microsecond trigger pulse on the TRIG pin.
 * @param[in] us Pointer to an Ultrasonic_t configuration structure.
 * @retval None
 */
void Ultrasonic_Trigger(Ultrasonic_t *us);

/**
 * @brief Interrupt Service Routine callback for Echo pin edge transitions.
 * @details Must be invoked from the EXTI interrupt handler when an edge is detected on the Echo pin.
 * @param[in,out] us Pointer to an Ultrasonic_t configuration structure.
 * @param[in] GPIO_Pin Pin number that triggered the EXTI interrupt.
 * @retval None
 */
void Ultrasonic_Echo_ISR(Ultrasonic_t *us, uint16_t GPIO_Pin);

/**
 * @brief Retrieves the latest distance measurement in centimeters.
 * @param[in] us Pointer to an Ultrasonic_t configuration structure.
 * @retval uint32_t Distance in centimeters (range: 2 to 400 cm).
 */
uint32_t Ultrasonic_GetDistanceCm(const Ultrasonic_t *us);

/**
 * @brief Retrieves the latest distance measurement in inches.
 * @param[in] us Pointer to an Ultrasonic_t configuration structure.
 * @retval uint32_t Distance in inches.
 */
uint32_t Ultrasonic_GetDistanceInch(const Ultrasonic_t *us);

#ifdef __cplusplus
}
#endif

#endif /* __ULTRASONIC_H__ */
