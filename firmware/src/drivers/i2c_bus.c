/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * SPDX-FileCopyrightText: 2026 The DISLED contributors
 * Additional permission under GNU GPL version 3 section 7: see LICENSING.md.
 */
#include "i2c_bus.h"

#include <stdbool.h>

#include "bsp/board.h"
#include "bsp/bsp_time.h"
#include "bsp/ch32.h"
#include "time_utils.h"

/* A byte at 100 kHz takes 90 us; 3 ms covers any legal clock stretching. */
#define WAIT_TIMEOUT_MS 3u

static uint32_t s_speed_hz;

/* Diagnostics of the last failure: step of the transaction and status
 * registers at that moment (see i2c_bus_last_failure). */
static uint8_t s_step;
static i2c_bus_failure_t s_failure;

static uint16_t s_error_count;

static i2c_bus_status_t fail(i2c_bus_status_t status)
{
    if (s_error_count < UINT16_MAX) {
        s_error_count++;
    }
    s_failure.step = s_step;
    s_failure.status = status;
    s_failure.star1 = BOARD_I2C->STAR1;
    s_failure.star2 = BOARD_I2C->STAR2;
    s_failure.ctlr1 = BOARD_I2C->CTLR1;
    return status;
}

/* Clocks SCL by hand: up to nine clocks let a slave finish the byte it is
 * sending and release SDA, then a STOP condition resets its state. */
static void release_bus(void)
{
    GPIO_InitTypeDef pins = {
        .GPIO_Pin = BOARD_I2C_SCL_PIN | BOARD_I2C_SDA_PIN,
        .GPIO_Speed = GPIO_Speed_10MHz,
        .GPIO_Mode = GPIO_Mode_Out_OD,
    };
    GPIO_SetBits(BOARD_I2C_PORT, BOARD_I2C_SCL_PIN | BOARD_I2C_SDA_PIN);
    GPIO_Init(BOARD_I2C_PORT, &pins);
    for (int i = 0; i < 9; i++) {
        if (GPIO_ReadInputDataBit(BOARD_I2C_PORT, BOARD_I2C_SDA_PIN) == Bit_SET) {
            break;
        }
        GPIO_ResetBits(BOARD_I2C_PORT, BOARD_I2C_SCL_PIN);
        bsp_time_delay_us(5);
        GPIO_SetBits(BOARD_I2C_PORT, BOARD_I2C_SCL_PIN);
        bsp_time_delay_us(5);
    }
    /* STOP: SDA rises while SCL is high. SDA must go low while SCL is low;
     * pulling it low with SCL high is a START, which left the sensor out of
     * step and made the next transfer lose arbitration (seen at bring-up). */
    GPIO_ResetBits(BOARD_I2C_PORT, BOARD_I2C_SCL_PIN);
    bsp_time_delay_us(5);
    GPIO_ResetBits(BOARD_I2C_PORT, BOARD_I2C_SDA_PIN);
    bsp_time_delay_us(5);
    GPIO_SetBits(BOARD_I2C_PORT, BOARD_I2C_SCL_PIN);
    bsp_time_delay_us(5);
    GPIO_SetBits(BOARD_I2C_PORT, BOARD_I2C_SDA_PIN);
    bsp_time_delay_us(5);
}

/* Full reset of the peripheral, then hands it the pins. Resetting before
 * the pins switch to the alternate function avoids the spurious BUSY state
 * these controllers are known for (seen at bring-up: the first transfer
 * after power-up left the bus busy for good). */
static void configure_peripheral(void)
{
    I2C_DeInit(BOARD_I2C);
    I2C_SoftwareResetCmd(BOARD_I2C, ENABLE);
    I2C_SoftwareResetCmd(BOARD_I2C, DISABLE);

    GPIO_InitTypeDef pins = {
        .GPIO_Pin = BOARD_I2C_SCL_PIN | BOARD_I2C_SDA_PIN,
        .GPIO_Speed = GPIO_Speed_10MHz,
        .GPIO_Mode = GPIO_Mode_AF_OD,
    };
    GPIO_Init(BOARD_I2C_PORT, &pins);

    I2C_InitTypeDef init = {
        .I2C_ClockSpeed = s_speed_hz,
        .I2C_Mode = I2C_Mode_I2C,
        .I2C_DutyCycle = I2C_DutyCycle_2,
        .I2C_OwnAddress1 = 0,
        .I2C_Ack = I2C_Ack_Enable,
        .I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit,
    };
    I2C_Init(BOARD_I2C, &init);
    I2C_Cmd(BOARD_I2C, ENABLE);
}

void i2c_bus_init(uint32_t speed_hz)
{
    s_speed_hz = speed_hz;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    release_bus();
    configure_peripheral();
}

/* Waits for an event (all flags of `event` set) or a single flag. */
static i2c_bus_status_t wait_event(uint32_t event)
{
    const uint32_t start = bsp_time_ms();
    while (I2C_CheckEvent(BOARD_I2C, event) != READY) {
        if (I2C_GetFlagStatus(BOARD_I2C, I2C_FLAG_AF) != RESET) {
            return fail(I2C_BUS_NACK);
        }
        if (I2C_GetFlagStatus(BOARD_I2C, I2C_FLAG_ARLO) != RESET ||
            I2C_GetFlagStatus(BOARD_I2C, I2C_FLAG_BERR) != RESET) {
            return fail(I2C_BUS_ERROR);
        }
        if (time_elapsed_ms(bsp_time_ms(), start) > WAIT_TIMEOUT_MS) {
            return fail(I2C_BUS_TIMEOUT);
        }
    }
    return I2C_BUS_OK;
}

static i2c_bus_status_t wait_flag(uint32_t flag)
{
    const uint32_t start = bsp_time_ms();
    while (I2C_GetFlagStatus(BOARD_I2C, flag) == RESET) {
        if (I2C_GetFlagStatus(BOARD_I2C, I2C_FLAG_AF) != RESET) {
            return fail(I2C_BUS_NACK);
        }
        if (time_elapsed_ms(bsp_time_ms(), start) > WAIT_TIMEOUT_MS) {
            return fail(I2C_BUS_TIMEOUT);
        }
    }
    return I2C_BUS_OK;
}

static i2c_bus_status_t wait_bus_free(void)
{
    const uint32_t start = bsp_time_ms();
    while (I2C_GetFlagStatus(BOARD_I2C, I2C_FLAG_BUSY) != RESET) {
        if (time_elapsed_ms(bsp_time_ms(), start) > WAIT_TIMEOUT_MS) {
            return fail(I2C_BUS_TIMEOUT);
        }
    }
    return I2C_BUS_OK;
}

/* Ends a failed transaction and leaves the peripheral ready: if the bus
 * stays busy after the STOP, it is released and the peripheral reset. */
static i2c_bus_status_t abort_transfer(i2c_bus_status_t status)
{
    I2C_GenerateSTOP(BOARD_I2C, ENABLE);
    I2C_ClearFlag(BOARD_I2C, I2C_FLAG_AF | I2C_FLAG_ARLO | I2C_FLAG_BERR);
    I2C_AcknowledgeConfig(BOARD_I2C, ENABLE);
    I2C_NACKPositionConfig(BOARD_I2C, I2C_NACKPosition_Current);
    const i2c_bus_failure_t original = s_failure; /* keep the first cause */
    if (wait_bus_free() != I2C_BUS_OK) {
        i2c_bus_recover();
    }
    s_failure = original;
    return status;
}

static i2c_bus_status_t start_write(uint8_t address)
{
    s_step = 1;
    i2c_bus_status_t status = wait_bus_free();
    if (status != I2C_BUS_OK) {
        return status;
    }
    s_step = 2;
    I2C_GenerateSTART(BOARD_I2C, ENABLE);
    status = wait_event(I2C_EVENT_MASTER_MODE_SELECT);
    if (status != I2C_BUS_OK) {
        return status;
    }
    s_step = 3;
    I2C_Send7bitAddress(BOARD_I2C, (uint8_t)(address << 1), I2C_Direction_Transmitter);
    return wait_event(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED);
}

i2c_bus_status_t i2c_bus_write(uint8_t address, const uint8_t *data, size_t length)
{
    i2c_bus_status_t status = start_write(address);
    for (size_t i = 0; status == I2C_BUS_OK && i < length; i++) {
        s_step = 4;
        I2C_SendData(BOARD_I2C, data[i]);
        status = wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);
    }
    if (status != I2C_BUS_OK) {
        return abort_transfer(status);
    }
    I2C_GenerateSTOP(BOARD_I2C, ENABLE);
    return I2C_BUS_OK;
}

i2c_bus_status_t i2c_bus_read_word(uint8_t address, uint8_t reg, uint8_t out[2])
{
    i2c_bus_status_t status = start_write(address);
    if (status == I2C_BUS_OK) {
        s_step = 4;
        I2C_SendData(BOARD_I2C, reg);
        status = wait_event(I2C_EVENT_MASTER_BYTE_TRANSMITTED);
    }
    if (status == I2C_BUS_OK) {
        s_step = 5;
        I2C_GenerateSTART(BOARD_I2C, ENABLE); /* repeated start */
        status = wait_event(I2C_EVENT_MASTER_MODE_SELECT);
    }
    if (status != I2C_BUS_OK) {
        return abort_transfer(status);
    }

    /* Byte-by-byte reception, as in the WCH I2C examples: the first byte is
     * acknowledged; once it is read, ACK goes off and STOP is queued so the
     * second byte gets the NACK that ends the transfer. (The STM32 two-byte
     * method with POS and BTF never completed on the CH32V203.) */
    I2C_AcknowledgeConfig(BOARD_I2C, ENABLE);
    s_step = 6;
    I2C_Send7bitAddress(BOARD_I2C, (uint8_t)(address << 1), I2C_Direction_Receiver);
    status = wait_event(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED);
    for (size_t i = 0; status == I2C_BUS_OK && i < 2u; i++) {
        if (i == 1u) {
            I2C_AcknowledgeConfig(BOARD_I2C, DISABLE);
            I2C_GenerateSTOP(BOARD_I2C, ENABLE);
        }
        s_step = (uint8_t)(7u + i);
        status = wait_flag(I2C_FLAG_RXNE);
        if (status == I2C_BUS_OK) {
            out[i] = I2C_ReceiveData(BOARD_I2C);
        }
    }
    I2C_AcknowledgeConfig(BOARD_I2C, ENABLE);
    if (status != I2C_BUS_OK) {
        return abort_transfer(status);
    }
    return I2C_BUS_OK;
}

void i2c_bus_recover(void)
{
    I2C_Cmd(BOARD_I2C, DISABLE);
    release_bus();
    configure_peripheral();
}

i2c_bus_failure_t i2c_bus_last_failure(void)
{
    return s_failure;
}

uint16_t i2c_bus_error_count(void)
{
    return s_error_count;
}
