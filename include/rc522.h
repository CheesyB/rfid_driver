#ifndef __RC522_H_
#define __RC522_H_

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

struct rc522_config {
  struct spi_dt_spec spi;
};

typedef int (*rc522_cmd_t)(const struct device *dev);
// typedef int (*rc522_write_t)(const struct device *dev,  const uint8_t reg,
// const uint8_t write_value);
typedef int (*rc522_read_t)(const struct device *dev, uint8_t *read_value);

__subsystem struct rc522_driver_api {
  // rc522_read_t rc522_read_register;
  rc522_read_t rc522_read_version;
};

__syscall int rc522_read_version(const struct device *dev, uint8_t *read_value);

static inline int z_impl_rc522_read_version(const struct device *dev,
                                            uint8_t *read_value) {
  const struct rc522_driver_api *api =
      (const struct rc522_driver_api *)dev->api;
  if (api->rc522_read_version == NULL) {
    return -ENOSYS;
  }
  return api->rc522_read_version(dev, read_value);
}

#include <syscalls/rc522.h>

#endif // !__RC522_H_
