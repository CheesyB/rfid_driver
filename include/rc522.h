
struct rc522_config {
    struct spi_dt_spec spi;
};



typedef int (*rc522_cmd_t)(const struct device *dev);
//typedef int (*rc522_write_t)(const struct device *dev,  const uint8_t reg, const uint8_t write_value);
//typedef int (*rc522_read_t)(const struct device *dev,  const uint8_t reg)

__subsystem struct rc522_driver_api {
    //rc522_read_t rc522_read_register;
    rc522_cmd_t rc522_read_version;
};


__syscall int rc522_read_version(const struct device *dev);
static inline int z_impl_rc522_read_version(const struct device *dev)
{
    const struct rc522_driver_api *api = (const struct rc522_driver_api *)dev->api;
    if (api->rc522_read_version == NULL) {
        return -ENOSYS;
    }
    return api->rc522_read_version(dev);
}

#include <syscalls/rc522.h>
