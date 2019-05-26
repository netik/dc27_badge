#ifndef COMPONENTS_DRV_IS31FL_H_
#define COMPONENTS_DRV_IS31FL_H_

#define ISSI_ADDR_DEFAULT  (0x50)		/* I2C address of LED Driver */
#define ISSI_ADDR_MAX      (0xBE)		/* Highest memory address in the LED driver */

//Command register
#define ISSI_REG_COMMAND				0xFD
#define ISSI_REG_WRITE_LOCK				0xFE
#define ISSI_REG_WRITE_UNLOCK			0xC5

#define ISSI_PAGE_LED					0x00
#define ISSI_PAGE_PWM					0x01
#define ISSI_PAGE_BREATH				0x02
#define ISSI_PAGE_FUNCTION				0x03

#define ISSI_REG_CONFIG_SSD_ON			0x00	/* Soft shutdown mode enabled */
#define ISSI_REG_CONFIG_SSD_OFF			0x01	/* Soft shutdown mode disabled */
#define ISSI_REG_CONFIG_B_EN			0x02
#define ISSI_REG_CONFIG_OSD				0x04

#define ISSI_REG_CONFIG  				0x00
#define ISSI_REG_CONFIG_PICTUREMODE 	0x00
#define ISSI_REG_CONFIG_AUTOPLAYMODE 	0x08
#define ISSI_REG_CONFIG_AUDIOPLAYMODE 	0x18
#define ISSI_REG_GCC					0x01

#define ISSI_CONF_PICTUREMODE 			0x00
#define ISSI_CONF_AUTOFRAMEMODE 		0x04
#define ISSI_CONF_AUDIOMODE 			0x08

#define ISSI_REG_RESET					0x11

#define ISSI_TIMEOUT				OSAL_MS2I(10)

/**
 * Initializes the LED driver. Initialization is not required and can be done lazily the first
 * time this driver is used.
 *
 * @return true if successful
 */
extern bool drv_is31fl_init(void);

/**
 * Sets global current control. AKA brightness.
 *
 * @param gcc Brightness value 0 to 255
 */
extern void drv_is31fl_gcc_set(uint8_t gcc);

/**
 * Sends an 8 bit value (0 to 255) to a specific address. See the datasheet for LED addresses.
 */
extern void drv_is31fl_send_value(uint8_t address, uint8_t value);

/**
 * Sets the driver page for sending values etc.
 *
 * @param page Page to use. ISSI_PAGE_LED, ISSI_PAGE_PWM, ISSI_PAGE_BREATH, or ISSI_PAGE_FUNCTION
 */
extern void drv_is31fl_set_page(uint8_t page);

/* these are our versions of the write command */
extern bool hal_i2c_write_reg_byte(uint8_t i2c_address, uint8_t reg, uint8_t value);
extern uint8_t hal_i2c_read_reg_byte(uint8_t i2c_address, uint8_t reg);

#endif /* COMPONENTS_DRV_IS31FL_H_ */
