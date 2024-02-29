#include "fob.h"

static int fob_connect(const struct spi_dt_spec *rc522, uint8_t *UID) {
	int ret;
	while (1) {
		ret = rc522_reqa(rc522);
		if (ret != STATUS_OK) {
			k_msleep(250);
			continue;
		}

		rc522_select(rc522, UID, 0, NULL);

		ret = rc522_select(rc522, UID, 1, NULL);
		if (ret != STATUS_OK) {
			continue;
		}

		break;
	}
	return STATUS_OK;
}

int fob_write_key(const struct spi_dt_spec *rc522, const uint8_t sector, const uint8_t *old_key, const uint8_t *new_key) {
	int ret;

	uint8_t UID[5] = {0};
	fob_connect(rc522, UID);

	uint8_t block_addr = 4 * sector + 3;

	ret = rc522_mifare_auth(rc522, block_addr, old_key, UID);
	if (ret != STATUS_OK) {
		rc522_mifare_deauth(rc522);
		return ret;
	}

	uint8_t length = 18;
	uint8_t read_values[18] = {0};
	ret = rc522_mifare_read(rc522, block_addr, &length, read_values);
	if (ret != STATUS_OK) {
		rc522_mifare_deauth(rc522);
		return ret;
	}

	memcpy(read_values, new_key, 6);
	memcpy(read_values+10, new_key, 6);

	ret = rc522_mifare_write(rc522, block_addr, 16, read_values);
	if (ret != STATUS_OK) {
		rc522_mifare_deauth(rc522);
		return ret;
	}

	rc522_mifare_deauth(rc522);
	return STATUS_OK;
}

int fob_write_passcode(const struct spi_dt_spec *rc522, const uint8_t sector, const uint8_t *key, const uint8_t *passcode) {
	int ret;

	uint8_t UID[5] = {0};
	fob_connect(rc522, UID);

	uint8_t block_addr = 4 * sector + 2;

	ret = rc522_mifare_auth(rc522, block_addr, key, UID);
	if (ret != STATUS_OK) {
		rc522_mifare_deauth(rc522);
		return ret;
	}

	uint8_t write_values[16] = {0};
	memcpy(write_values, passcode, 6);

	ret = rc522_mifare_write(rc522, block_addr, 16, write_values);
	if (ret != STATUS_OK) {
		rc522_mifare_deauth(rc522);
		return ret;
	}
	
	rc522_mifare_deauth(rc522);
	return STATUS_OK;
}

int fob_read_passcode(const struct spi_dt_spec *rc522, const uint8_t sector, const uint8_t *key, uint8_t *passcode) {
	int ret;

	uint8_t UID[5] = {0};
	fob_connect(rc522, UID);

	uint8_t block_addr = 4 * sector + 2;

	ret = rc522_mifare_auth(rc522, block_addr, key, UID);
	if (ret != STATUS_OK) {
		rc522_mifare_deauth(rc522);
		return ret;
	}

	uint8_t length = 18;
	uint8_t read_values[18] = {0};
	ret = rc522_mifare_read(rc522, block_addr, &length, read_values);
	if (ret != STATUS_OK) {
		rc522_mifare_deauth(rc522);
		return ret;
	}

	memcpy(passcode, read_values, 6);
	
	rc522_mifare_deauth(rc522);
	return STATUS_OK;
}
