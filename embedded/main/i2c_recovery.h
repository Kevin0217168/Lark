#ifndef __I2C_RECOVERY_H__
#define __I2C_RECOVERY_H__

/**
 * @brief I2C 总线恢复：在 WDT/软重启后，从机可能卡住 SDA 线
 *        通过手动发送 9 个 SCL 时钟脉冲 + STOP 条件来恢复总线
 *
 * @param sda_pin SDA 引脚号
 * @param scl_pin SCL 引脚号
 */
void i2c_bus_recovery(int sda_pin, int scl_pin);

#endif /* __I2C_RECOVERY_H__ */
