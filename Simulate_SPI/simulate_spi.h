/*
 * @Descripttion: GPIO模拟SPI
 * @version: 
 * @Author: Newt
 * @Date: 2022-08-23 09:50:18
 * @LastEditors: Newt
 * @LastEditTime: 2022-12-07 17:31:42
 */

#ifndef SIMULATE_SPI_H
#define SIMULATE_SPI_H

#include <stdint.h>

#include <lf_timer.h>
#include <lf_gpio.h>

/* set spi mode and delay function*/
#define SPI_MODE 3
#define DELAY_US 
/* add sumulate pin below here */
#define GPIO_SCK  3
#define GPIO_MOSI 0
#define GPIO_MISO 
#define GPIO_CS   2
/* add ur enabel gpio API functions below here  */
#define ENABLE_SCK  lf_gpio_enable_output(GPIO_SCK, 0 ,0)
#define ENABLE_MOSI lf_gpio_enable_output(GPIO_MOSI, 0 ,0)
#define ENABLE_MISO 
#define ENABLE_CS   lf_gpio_enable_output(GPIO_CS, 0 ,0)
/* add ur set gpio API functions below here */
#define SET_SCK_LOW   lf_gpio_output_set(GPIO_SCK, 0)
#define SET_SCK_HIGH  lf_gpio_output_set(GPIO_SCK, 1)
#define SET_MOSI_LOW  lf_gpio_output_set(GPIO_MOSI, 0)
#define SET_MOSI_HIGH lf_gpio_output_set(GPIO_MOSI, 1)
#define SET_MISO_LOW  
#define SET_MISO_HIGH 
#define SET_CS_LOW    lf_gpio_output_set(GPIO_CS, 0)
#define SET_CS_HIGH   lf_gpio_output_set(GPIO_CS, 1)

int8_t simulate_spi_init(void);
int8_t simulate_spi_transmit(uint8_t *buffer, uint16_t size);

#endif /* SIMULATE_SPI_H */