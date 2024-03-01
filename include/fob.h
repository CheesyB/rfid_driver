#pragma once

#include "drivers/rc522.h"

int fob_select(const struct spi_dt_spec *rc522, uint8_t *UID);
int fob_auth(const struct spi_dt_spec *rc522, const uint8_t *UID, const uint8_t sector, const uint8_t *key);

/* Writes the new key to the designated sector. */
int fob_write_key(const struct spi_dt_spec *rc522, const uint8_t *UID, const uint8_t sector, const uint8_t *key);

/* Writes the passcode to the designated sector. */
int fob_write_passcode(const struct spi_dt_spec *rc522, const uint8_t *UID, const uint8_t sector, const uint8_t *passcode);

/* Reads the passcode from the designated sector into the passcode array. */
int fob_read_passcode(const struct spi_dt_spec *rc522, const uint8_t *UID, const uint8_t sector, uint8_t *passcode);
