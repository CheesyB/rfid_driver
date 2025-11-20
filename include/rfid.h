#ifndef __ZEPHYR_INCLUDE_DRIVERS__RFDI_H__
#define __ZEPHYR_INCLUDE_DRIVERS__RFDI_H__

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*rfid_cmd_t)(const struct device *dev);
typedef int (*rfid_fob_select_t)(const struct device *dev, uint8_t *uid,
                                 const uint8_t UID_valid, uint8_t *sak);
typedef int (*rfid_fob_auth_t)(const struct device *dev,
                               const uint8_t block_addr, const uint8_t *key,
                               const uint8_t *UID);
typedef int (*rfid_fob_read_t)(const struct device *dev,
                               const uint8_t block_addr, uint8_t *length,
                               uint8_t *read_values);
typedef int (*rfid_fob_write_t)(const struct device *dev,
                                const uint8_t block_addr, const uint8_t length,
                                const uint8_t *data);

__subsystem struct rfid_driver_api {
  rfid_cmd_t hw_test;
  rfid_cmd_t fob_reqa;
  rfid_fob_select_t fob_select;
  //rfid_fob_auth_t fob_auth;
  // rfid_cmd_t deauth;
  // rfid_fob_read_t fob_read;
  // rfid_fob_write_t fob_write;
};

/* Runs standard test procedure to confirm working SPI communication with RC522
 * and ability to run commands, run CRC coprocessor, etc. Prints 64 bytes of
 * standard test data which can be cross referenced. Does not use antenna. */
__syscall int hw_test(const struct device *dev);

static inline int z_impl_hw_test(const struct device *dev) {
  const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;
  if (api->hw_test == NULL) {
    return -ENOSYS;
  }
  return api->hw_test(dev);
}

/* Sends a REQA to all nearby fobs. */
__syscall int fob_reqa(const struct device *dev);

static inline int z_impl_fob_reqa(const struct device *dev) {
  const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;
  if (api->fob_reqa == NULL) {
    return -ENOSYS;
  }
  return api->fob_reqa(dev);
}

/* Sends a Select to all nearby tags. Does NOT implement anticollision and just
 * stores the 5 byte response in UID when UID_valid is false. When UID_valid is
 * true, selects the tag with the matching UID and stores the 1 byte response
 * in sak if it is not NULL. */
__syscall int fob_select(const struct device *dev, uint8_t *uid,
                         const uint8_t UID_valid, uint8_t *sak);

static inline int z_impl_fob_select(const struct device *dev, uint8_t *uid,
                                    const uint8_t UID_valid, uint8_t *sak) {
  const struct rfid_driver_api *api = (const struct rfid_driver_api *)dev->api;
  if (api->fob_select == NULL) {
    return -ENOSYS;
  }
  return api->fob_select(dev, uid, UID_valid, sak);
}

// /* Sends a MIFARE authentication command to the selected tag with the
//  * corresponding UID. If the key matches the key for the sector that
//  block_addr
//  * falls under, this function will grant access to that sector. */
// int fob_auth(const struct spi_dt_spec *dev, const uint8_t block_addr,
//              const uint8_t *key, const uint8_t *UID);
//
// /* Does NOT send any commands to any tag. Instead, this function disables
//  * encryption so that non-MIFARE functions can once again be used. */
// int deauth(const struct spi_dt_spec *dev);
//
// /* Assuming authentication has been granted, this function will read the 16
//  * data bytes and 2 CRC bytes of the block located at block_addr into the
//  * read_values array; length >= 18 required. The length variable will be
//  * updated with the length of the actual response if it is shorter for some
//  * reason. */
// int fob_read(const struct spi_dt_spec *dev, const uint8_t block_addr,
//              uint8_t *length, uint8_t *read_values);
//
// /* Assuming authentication has been granted, this function will write the 16
//  * data bytes from the data array into the block located at block_addr. The
//  * length variable MUST be 16. */
// int fob_write(const struct spi_dt_spec *dev, const uint8_t block_addr,
//               const uint8_t length, const uint8_t *data);
//
#include <zephyr/syscalls/rfid.h>

#ifdef __cplusplus
}
#endif

#endif // !__ZEPHYR_INCLUDE_DRIVERS__RFDI_H__
