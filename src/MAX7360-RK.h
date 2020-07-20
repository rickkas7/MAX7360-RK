#ifndef __MAX7360_RK_H
#define __MAX7360_RK_H

// Repository: https://github.com/rickkas7/MAX7360-RK
// License: MIT

#include "Particle.h"


class MAX7360KeyMappingBase; // Forward declaration

class MAX7360Key {
public:
	MAX7360Key();
	MAX7360Key(MAX7360KeyMappingBase *keyMapping, uint8_t rawValue);

	void fromRawValue(uint8_t rawValue);
	
	uint8_t getRawValue() const { return rawValue; };
	bool isEmpty() const { return rawValue == FIFO_EMPTY; };
	bool isOverflow() const { return   rawValue == FIFO_OVERFLOW; };
	uint8_t getRawKey() const { return rawKey; };
	char getMappedKey() const;
	bool hasKey() const { return rawKey != FIFO_KEY_NONE; };
	bool hasMore() const { return more; };
	bool isReleased() const { return released; };
	bool isKeyRepeat() const { return rawValue == FIFO_KEY_REPEAT_MORE || rawValue == FIFO_KEY_REPEAT_DONE; };

	static const uint8_t FIFO_EMPTY 			= 0b00111111;
	static const uint8_t FIFO_OVERFLOW 			= 0b01111111;
	static const uint8_t FIFO_KEY63_PRESSED 	= 0b10111111;
	static const uint8_t FIFO_KEY63_RELEASED 	= 0b11111111;
	static const uint8_t FIFO_KEY_REPEAT_MORE 	= 0b00111110;
	static const uint8_t FIFO_KEY_REPEAT_DONE 	= 0b01111110;
	static const uint8_t FIFO_KEY62_PRESSED 	= 0b10111110;
	static const uint8_t FIFO_KEY62_RELEASED 	= 0b11111110;
	
	static const uint8_t FIFO_EMPTY_MASK		= 0b10000000;
	static const uint8_t FIFO_RELEASED_MASK		= 0b01000000;
	static const uint8_t FIFO_KEY_MASK			= 0b00111111;
	
	static const uint8_t FIFO_KEY_NONE			= 0xff;

protected:
	MAX7360KeyMappingBase *keyMapping = 0;
	uint8_t rawValue = 0x00;
	uint8_t rawKey = FIFO_KEY_NONE;
	bool more = false;
	bool released = false;
};

class MAX7360KeyMappingBase {
public:
	MAX7360KeyMappingBase();
	virtual ~MAX7360KeyMappingBase();

	virtual char rawToReadable(uint8_t rawValue) = 0;
	virtual uint8_t readableToRaw(char c) = 0;
};

class MAX7360KeyMappingTable : public MAX7360KeyMappingBase {
public:
	MAX7360KeyMappingTable(const char *table, size_t tableSize);
	virtual ~MAX7360KeyMappingTable();

	virtual char rawToReadable(uint8_t rawValue);
	virtual uint8_t readableToRaw(char c);
	

protected:
	/**
	 * @brief Table of key mappings
	 * 
	 * Row  COL0  COL1  COL2  COL3  COL4  COL5  COL6  COL7
	 * ROW0 KEY0  KEY8  KEY16 KEY24 KEY32 KEY40 KEY48 KEY56
	 * ROW1 KEY1  KEY9  KEY17 KEY25 KEY33 KEY41 KEY49 KEY57
	 * ROW2 KEY2  KEY10 KEY18 KEY26 KEY34 KEY42 KEY50 KEY58
	 * ROW3 KEY3  KEY11 KEY19 KEY27 KEY35 KEY43 KEY51 KEY59
	 * ROW4 KEY4  KEY12 KEY20 KEY28 KEY36 KEY44 KEY52 KEY60
	 * ROW5 KEY5  KEY13 KEY21 KEY29 KEY37 KEY45 KEY53 KEY61
	 * ROW6 KEY6  KEY14 KEY22 KEY30 KEY38 KEY46 KEY53 KEY62
	 * ROW7 KEY7  KEY15 KEY23 KEY21 KEY39 KEY47 KEY55 KEY63
	 * 
	 * KEY0 is index 0 of the table
	 * KEY1 is index 1 of the table
	 * ...
	 * KEY63 is index 63 of the table
	 * 
	 * Unsupported keys should have 0 in the table cell.
	 * The table must be filled in up to tableSize bytes,
	 * but this can be less than 64 bytes if you're not using all 
	 * of the available columns. For example, with a typical 4x3
	 * matrix keypad (phone style), you have 3 columns to the table
	 * can be 24 bytes long.
	 */
	const char *table;
	size_t tableSize;
};

/**
 * @brief Keypad mapping class for a 4x3 phone keypad
 * 
 * Example:
 * https://www.adafruit.com/product/1824
 */
class MAX7360KeyMappingPhone : public MAX7360KeyMappingTable {
public:
	MAX7360KeyMappingPhone();
	virtual ~MAX7360KeyMappingPhone();
};

/**
 * @brief Class for the MAX7360 LED driver
 *
 * Normally you create one of these as a global variable.
 *
 * Be sure to call the begin() method from setup(). You will probably also want to call
 * withKeyMapping() to specify a raw key to readable key name mapping object.
 */
class MAX7360 {
public:
	/**
	 * @brief Construct the object
	 *
	 * @param addr The address. Can be 0 - 7 based on the address select pin and the normal base of 0x38
	 *
	 * @param wire The I2C interface to use. Normally Wire, the primary I2C interface. Can be a
	 * different one on devices with more than one I2C interface.
	 * 
	 * AD0  I2C Address
	 * GND  0b0111000 = 0x38  (short addr 0, default if parameter omitted)
	 * VCC  0b0111010 = 0x3A  (short addr 2)
	 * SDA  0b0111100 = 0x3C  (short addr 4)
	 * SCL  0b0111110 = 0x3E  (short addr 6)
	 * 
	 * (I2C addresses are 0x00 - 0x7F, not including the R/W bit)
	 */
	MAX7360(uint8_t addr = 0x38, TwoWire &wire = Wire);

	/**
	 * @brief Destructor. Not normally used as this is typically a globally instantiated object.
	 */
	virtual ~MAX7360();

	/**
	 * @brief Sets the key mapping object
	 */
	MAX7360 &withKeyMapping(MAX7360KeyMappingBase *keyMapping) { this->keyMapping = keyMapping; return *this; };

	/**
	 * @brief Get a pointer to the key mapping object 
	 */
	MAX7360KeyMappingBase *getKeyMapping() { return keyMapping; };


	/**
	 * @brief Set up the I2C device and begin running.
	 *
	 * You cannot do this from STARTUP or global object construction time. It should only be done from setup
	 * or loop (once).
	 *
	 * Make sure you set the LED current using withLEDCurrent() before calling begin if your LED has a
	 * current other than the default of 5 mA.
	 */
	bool begin();

	/**
	 * @brief Resets values to power-on defaults
	 */
	bool resetRegisterDefaults();

	/**
	 * @brief Read keypad FIFO
	 * 
	 */
	MAX7360Key readKeyFIFO();

	/**
	 * @brief Get the configuration register value
	 */
	uint8_t getConfiguration();

	/**
	 * @brief Set the configuration register value
	 */
	bool setConfiguration(uint8_t rawValue);

	/**
	 * @brief Sets Clear /INTK on host read mode. The power-on default is Clear INTK on FIFO empty (false).
	 */
	bool setConfigurationClearINTKonRead(bool value = true);

	/**
	 * @brief Enables key release events in the FIFO. The power-on default key release events enabled (true).
	 */
	bool setConfigurationEnableKeyRelease(bool value = false);

	/**
	 * @brief Enables auto wake-up. The power-on default auto-wakeup enabled (true).
	 */
	bool setConfigurationAutoWakeUp(bool value = false);

	/**
	 * @brief Disables I2C timeouts. The power-on default timeouts enabled (false).
	 */
	bool setConfigurationDisableI2CTimeouts(bool value = true);


	/**
	 * @brief Gets the keypad debounce time in milliseconds. Will be 9 to 40. 
	 * 
	 * @return Debounce time setting in milliseconds in the range of 9 to 40 (inclusive).
	 * 
	 * Power-up default is 40 ms.
	 */
	uint8_t getDebounceTimeMs();

	/**
	 * @brief Sets the keypad debounce time in milliseconds. Must be 9 to 40. 
	 * 
	 * @param ms Milliseconds of debounce time. Must be 9 to 40 (inclusive).
	 */
	bool setDebounceTimeMs(uint8_t ms);

	/**
	 * @brief Gets the current GPO enable state.
	 * 
	 * @return The GPO enable state, which is an enumeration with the following values
	 * 
	 * - REG_GPO_DISABLED			GPO disabled (allows all columns to be use for keyboard)
	 * - REG_GPO_ENABLE_7			GPO enable COL7
	 * - REG_GPO_ENABLE_76			GPO enable COL7, 6
	 * - REG_GPO_ENABLE_765			GPO enable COL7, 6, 5
	 * - REG_GPO_ENABLE_7654		GPO enable COL7, 6, 5, 4
	 * - REG_GPO_ENABLE_76543		GPO enable COL7, 6, 5, 4, 3
	 * - REG_GPO_ENABLE_765432		GPO enable COL7, 6, 5, 4, 3, 2 (power-up default)
	 */
	uint8_t getGpoEnable();

	/**
	 * @brief Set GPO (general purpose output on COL pins) modes
	 * 
	 * The following values are allowed:
	 * - REG_GPO_DISABLED			GPO disabled (allows all columns to be use for keyboard)
	 * - REG_GPO_ENABLE_7			GPO enable COL7
	 * - REG_GPO_ENABLE_76			GPO enable COL7, 6
	 * - REG_GPO_ENABLE_765			GPO enable COL7, 6, 5
	 * - REG_GPO_ENABLE_7654		GPO enable COL7, 6, 5, 4
	 * - REG_GPO_ENABLE_76543		GPO enable COL7, 6, 5, 4, 3
	 * - REG_GPO_ENABLE_765432		GPO enable COL7, 6, 5, 4, 3, 2 (power-up default)
	 * 
	 * Note that this is different than the PORT2 - PORT7 GPIO!
	 */
	bool setGpoEnable(uint8_t value);


	/**
	 * @brief Sets the GPIO Output mode (constant current or non-constant current)
	 * 
	 * @param value The mode to select (see below)
	 * 
	 * - Bit D0 (mask 0b000000001) is PORT0
	 * - Bit D1 (mask 0b000000010) is PORT1
	 *   ...
	 * - Bit D7 (mask 0b100000000) is PORT7
	 * 
	 * If the bit value is:
	 * 
	 * - 0 Port is a constant-current open-drain output (power-up default)
	 * - 1 Port is a non-constant-current open-drain output
	 * 
	 * The power-up default is 5mA constant current. 
	 */
	bool setGpioOutputCurrentMode(uint8_t value) { return writeRegister(REG_GPIO_OUTPUT_MODE, value); };


	/**
	 * @brief Sets the GPIO as input or output
	 * 
	 * @param value The mode to select (see below)
	 * 
	 * - Bit D0 (mask 0b000000001) is PORT0
	 * - Bit D1 (mask 0b000000010) is PORT1
	 *   ...
	 * - Bit D7 (mask 0b100000000) is PORT7
	 * 
	 * If the bit is:
	 * 
	 * - 0 Port is an input (power-up default)
	 * - 1 Port is an output
	 */
	bool setGpioInputOutputMode(uint8_t value) { return writeRegister(REG_GPIO_CONTROL, value); };


	/**
	 * @brief Set common PWM ratio
	 * 
	 * @param ratio Ratio. 0 = fully off, 255 = fully on (default: 0)
	 * 
	 */
	bool setCommmonPwmRatio(uint8_t ratio) { return writeRegister(REG_COMMON_PWM_RATIO, ratio); };

	/**
	 * @brief Gets the common PWM ratio
	 */
	uint8_t getCommonPwmRatio() { return readRegister(REG_COMMON_PWM_RATIO); };

	/**
	 * @brief Set port PWM ratio
	 * 
	 * @param port Port number 0 - 7 (inclusive)
	 * 
	 * @param ratio Ratio. 0 = fully off, 255 = fully on (default: 0)
	 * 
	 */
	bool setPortPwmRatio(uint8_t port, uint8_t ratio) { return writeRegister(REG_PORT_PWM_RATIO + port, ratio); };

	/**
	 * @brief Sets port interrupt settings
	 * 
	 * @param port Port number 0 - 7 (inclusive)
	 * 
	 * @param enabled true to enable port interrupts, false to disable
	 * 
	 * @param risingAndFalling true to generate interrupts on rising and falling edge, false for just rising edge
	 */
	bool setPortInterrupt(uint8_t port, bool enabled, bool risingAndFalling);

	/**
	 * @brief Set PWM mode for this port (common or individual)
	 * 
	 * @param port Port number 0 - 7 (inclusive)
	 * 
	 * @param common Enable common PWM mode (true), or individual (false). Default is individual.
	 * 
	 */
	bool setCommonPwmMode(uint8_t port, bool common) { return setRegisterBitmask(REG_PORT_CONFIG + port, REG_PORT_COMMON_PWM_MASK, common); };

	/**
	 * @brief Sets the period
	 * 
	 * @param port Port number 0 - 7 (inclusive)
	 * 
	 * @param period The blink on period, which must be one of:
	 * 
	 * - REG_PORT_BLINK_PERIOD_OFF	= 0;		Blinking disabled (does not blink) (default)
	 * - REG_PORT_BLINK_PERIOD_256	= 1 << 2;	Blink period 256ms
	 * - REG_PORT_BLINK_PERIOD_512	= 2 << 2;	Blink period 512ms
	 * - REG_PORT_BLINK_PERIOD_1024	= 3 << 2;	Blink period 1024ms
	 * - REG_PORT_BLINK_PERIOD_2048	= 4 << 2;	Blink period 2048ms
	 * - REG_PORT_BLINK_PERIOD_4096	= 5 << 2;	Blink period 4096ms
	 * 
	 */
	bool setBlinkPeriod(uint8_t port, uint8_t period) { return setRegisterMask(REG_PORT_CONFIG + port, ~REG_PORT_BLINK_PERIOD_MASK, period); }

	/**
	 * @brief Sets the blink on time percentage of blink period
	 * 
	 * @param port Port number 0 - 7 (inclusive)
	 * 
	 * @param period The blink on period, which must be one of:
	 * 
	 * - REG_PORT_BLINK_ON_50_PCT	 = 0;	LED on for 50% of blink period (default, if blinking is turned on, but by default it's off)
	 * - REG_PORT_BLINK_ON_25_PCT	 = 1;	LED on for 25% of blink period
	 * - REG_PORT_BLINK_ON_12_5_PCT	 = 2;	LED on for 12.5% of blink period
	 * - REG_PORT_BLINK_ON_6_25_PCT	 = 3;	LED on for 6.25% of blink period
	 */
	bool setBlinkOnTimePercent(uint8_t port, uint8_t value) { return setRegisterMask(REG_PORT_CONFIG + port, ~REG_PORT_BLINK_ON_TIME_MASK, value); }

	/**
	 * @brief Enable rotary encoder mode (power-on default is disabled)
	 * 
	 * @param enable true to enable rotary encoder mode or false to disable.
	 * 
	 * Enabling rotary encoder mode takes over PORT6 and PORT7 for the quadrature decoder.
	 */
	bool setConfigRotaryEncoder(bool enable = true) { return setRegisterBitmask(REG_GPIO_CONFIG, REG_GPIO_CONFIG_ROTARY_MASK, enable); };

	/**
	 * @brief Enable using /INTI to indicate I2C bus timeouts (power-on default is disabled)
	 * 
	 * @param enable true to enable /INTI to indicate I2C bus timeouts
	 */
	bool setConfigIntiI2cTimeouts(bool enable = true) { return setRegisterBitmask(REG_GPIO_CONFIG, REG_GPIO_CONFIG_I2C_TIMEOUT_MASK, enable); };

	/**
	 * @brief Enables GPIO mode (constant current and PWM modules) (power-on default is disabled)
	 * 
	 * @param enable true to enable GPIO features or false to disable.
	 */
	bool setConfigEnableGpio(bool enable = true) { return setRegisterBitmask(REG_GPIO_CONFIG, REG_GPIO_CONFIG_ENABLE_MASK, enable); };

	/**
	 * @brief Reset GPIO settings to power-up defaults
	 * 
	 * This includes all registers from 0x40 to 0x5f.
	 */
	bool setConfigResetGpio() { return setRegisterBitmask(REG_GPIO_CONFIG, REG_GPIO_CONFIG_RESET_MASK, true); };

	/**
	 * @brief Set fade time

	 * - REG_GPIO_CONFIG_FADE_TIME_DISABLED	= 0x00;		Fade time disabled (power-on default)
	 * - REG_GPIO_CONFIG_FADE_TIME_256_MS	= 0x01;		Fade time 256 ms
	 * - REG_GPIO_CONFIG_FADE_TIME_512_MS	= 0x02;		Fade time 512 ms
	 * - REG_GPIO_CONFIG_FADE_TIME_1024_MS	= 0x03;		Fade time 1024 ms
	 * - REG_GPIO_CONFIG_FADE_TIME_2048_MS	= 0x04;		Fade time 2048 ms
	 * - REG_GPIO_CONFIG_FADE_TIME_4096_MS	= 0x05;		Fade time 4096 ms
	 */
	bool setConfigFadeTime(uint8_t value) { return setRegisterMask(REG_GPIO_CONFIG, ~REG_GPIO_CONFIG_FADE_TIME_MASK, value); };

	/**
	 * @brief Reads the GPIO inputs when PORTx are configured as inputs
	 * 
	 * - Bit D7 (mask 0b10000000) = PORT7
	 * - Bit D6 (mask 0b01000000) = PORT6
	 * - Bit D5 (mask 0b00100000) = PORT5
	 * - Bit D4 (mask 0b00010000) = PORT4
	 * - Bit D3 (mask 0b00001000) = PORT3
	 * - Bit D2 (mask 0b00000100) = PORT2
	 * - Bit D1 (mask 0b00000010) = PORT1
	 * - Bit D0 (mask 0b00000001) = PORT0
	 * 
	 */
	uint8_t readGpioInputs() { return readRegister(REG_GPIO_INPUT); };

	/**
	 * @brief Reads the rotary switch counter
	 * 
	 * @return A signed number of clicks since the last read
	 */
	int8_t readRotarySwitchCount() { return (int8_t) readRegister(REG_GPIO_ROTARY_SWITCH_COUNT); };



	/**
	 * @brief Low-level call to read a register value
	 *
	 * @param reg The register to read (0x00 to 0x70)
	 */
	uint8_t readRegister(uint8_t reg);

	/**
	 * @brief Low-level call to write a register value
	 *
	 * @param reg The register to read (0x00 to 0x70)
	 *
	 * @param value The value to set
	 *
	 * Note that setProgram bypasses this to write multiple bytes at once, to improve efficiency.
	 */
	bool writeRegister(uint8_t reg, uint8_t value);

	/**
	 * @brief Set the register value using and and or masks
	 */
	bool setRegisterMask(uint8_t reg, uint8_t andValue, uint8_t orValue);

	/**
	 * @brief Set or clear register bits
	 */
	bool setRegisterBitmask(uint8_t reg, uint8_t bitMask, bool set = true);


	static const uint8_t REG_KEYS_FIFO = 0x00;			//!< Read the keys FIFO register

	static const uint8_t REG_CONFIG 						= 0x01;		//!< Configuration register
	static const uint8_t REG_CONFIG_SLEEP_MASK 				= 0x80;		//!< Configuration register sleep bit (D7)
	static const uint8_t REG_CONFIG_INTERRUPT_MASK 			= 0x20;		//!< Configuration register sleep bit (D5)
	static const uint8_t REG_CONFIG_KEY_RELEASE_MASK 		= 0x08;		//!< Configuration register key release bit (D3)
	static const uint8_t REG_CONFIG_AUTO_WAKEUP_MASK 		= 0x02;		//!< Configuration register auto wakeup bit (D1)
	static const uint8_t REG_CONFIG_TIMEOUT_DISABLE_MASK 	= 0x01;		//!< Configuration register I2C timeout disable bit (D0)
	
	static const uint8_t REG_DEBOUNCE 						= 0x02;		//!< Debounce and port configuration register
	static const uint8_t REG_DEBOUNCE_MS_OFFSET 			= 9;		//!< Debounce time starts at 9 ms
	static const uint8_t REG_DEBOUNCE_MASK					= 0x1f;		//!< Debounce time is in the low 5 bits (D4 - D0) (power-up default = 40 ms)
	static const uint8_t REG_GPO_ENABLE_MASK				= 0xe0;		//!< GPO enable is in the high 3 bits (D7 - D5)
	static const uint8_t REG_GPO_DISABLED					= 0;		//!< GPO disabled (allows all columns to be use for keyboard)
	static const uint8_t REG_GPO_ENABLE_7					= 1 << 5; 	//!< GPO enable COL7
	static const uint8_t REG_GPO_ENABLE_76					= 2 << 5;	//!< GPO enable COL7, 6
	static const uint8_t REG_GPO_ENABLE_765					= 3 << 5;	//!< GPO enable COL7, 6, 5
	static const uint8_t REG_GPO_ENABLE_7654				= 4 << 5;	//!< GPO enable COL7, 6, 5, 4
	static const uint8_t REG_GPO_ENABLE_76543				= 5 << 5;	//!< GPO enable COL7, 6, 5, 4, 3
	static const uint8_t REG_GPO_ENABLE_765432				= 6 << 5;	//!< GPO enable COL7, 6, 5, 4, 3, 2 (power-up default)
	
	static const uint8_t REG_KEY_SWITCH_INTERRUPT			= 0x03; 	//!< /INTK interruot control register
	static const uint8_t REG_GPO_CONTROL					= 0x04; 	//!< Control of COL pins and /INTK used a GPO
	static const uint8_t REG_AUTO_REPEAT					= 0x05; 	//!< Auto-repeat settings
	static const uint8_t REG_AUTO_SLEEP						= 0x06; 	//!< Auto-sleep settings

	// There is no register 0x07 to 0x3f

	static const uint8_t REG_GPIO_CONFIG 					= 0x40;		//!< Global GPIO configuration register
	static const uint8_t REG_GPIO_CONFIG_ROTARY_MASK 		= 0x80;		//!< Rotary switch enabled (default: disabled)
	static const uint8_t REG_GPIO_CONFIG_I2C_TIMEOUT_MASK 	= 0x20;		//!< Enable /INTI as I2C timeout indicator (default: disabled)
	static const uint8_t REG_GPIO_CONFIG_ENABLE_MASK	 	= 0x10;		//!< Enable GPIO and PWM operations (default: disabled)
	static const uint8_t REG_GPIO_CONFIG_RESET_MASK		 	= 0x08;		//!< Set to 1 to reset GPIO settings to factory defaults
	static const uint8_t REG_GPIO_CONFIG_FADE_TIME_MASK		= 0x07;		//!< Fade time mask
	static const uint8_t REG_GPIO_CONFIG_FADE_TIME_DISABLED	= 0x00;		//!< Fade time disabled
	static const uint8_t REG_GPIO_CONFIG_FADE_TIME_256_MS	= 0x01;		//!< Fade time 256 ms
	static const uint8_t REG_GPIO_CONFIG_FADE_TIME_512_MS	= 0x02;		//!< Fade time 512 ms
	static const uint8_t REG_GPIO_CONFIG_FADE_TIME_1024_MS	= 0x03;		//!< Fade time 1024 ms
	static const uint8_t REG_GPIO_CONFIG_FADE_TIME_2048_MS	= 0x04;		//!< Fade time 2048 ms
	static const uint8_t REG_GPIO_CONFIG_FADE_TIME_4096_MS	= 0x05;		//!< Fade time 4096 ms
	

	
	
	

	/**
	 * @brief GPIO Control Register (sets input or output mode)
	 * 
	 * - Bit D0 (mask 0b000000001) is PORT0
	 * - Bit D1 (mask 0b000000010) is PORT1
	 *   ...
	 * - Bit D7 (mask 0b100000000) is PORT7
	 * 
	 * If the bit is:
	 * 
	 * - 0 Port is an input (power-up default)
	 * - 1 Port is an output
	 */
	static const uint8_t REG_GPIO_CONTROL					= 0x41;


	/**
	 * @brief GPIO Output mode (constant current or non-constant current) register
	 * 
	 * - Bit D0 (mask 0b000000001) is PORT0
	 * - Bit D1 (mask 0b000000010) is PORT1
	 *   ...
	 * - Bit D7 (mask 0b100000000) is PORT7
	 * 
	 * If the bit value is:
	 * 
	 * - 0 Port is a constant-current open-drain output (power-up default)
	 * - 1 Port is a non-constant-current open-drain output
	 * 
	 * The power-up default is 5mA constant current.  
	 */
	static const uint8_t REG_GPIO_OUTPUT_MODE				= 0x44;

	static const uint8_t REG_COMMON_PWM_RATIO				= 0x45;		//!< PWM ratio for common PWM

	static const uint8_t REG_ROTARY_SWITCH_CONFIG			= 0x46;		//!< Configuration for rotary switch

	// There is no register 0x47

	static const uint8_t REG_I2C_TIMEOUT_FLAG				= 0x48;		//!< I2C Timeout Flag register (0x48)
	static const uint8_t REG_I2C_TIMEOUT_FLAG_MASK			= 0x01;		//!< Bit is set if timeout are enabled (default), and a timeout occurred. Reset on read.


	static const uint8_t REG_GPIO_INPUT						= 0x49;		//!< GPIO (PORT0 - PORT7) values when PORTx is used as input (read-only)
	
	static const uint8_t REG_GPIO_ROTARY_SWITCH_COUNT		= 0x4a;		//!< Rotary switch counter value if enabled (read-only)
	
	// There is no register 0x4b - 0x4f

	static const uint8_t REG_PORT_PWM_RATIO 				= 0x50;		//!< Port PWM ration for PORT0 - PORT7 (registers 0x50 - 0x57)


	static const uint8_t REG_PORT_CONFIG 					= 0x58;		//!< Port config for PORT0 - PORT7 (registers 0x58 - 0x5f)
	static const uint8_t REG_PORT_INTERRUPT_MASK 			= 0x80;		//!< 0 = interrupt disabled (default), 1 = interrupt enabled
	static const uint8_t REG_PORT_EDGE_MASK 				= 0x40;		//!< 0 = rising (default), 1 = rising or falling
	static const uint8_t REG_PORT_COMMON_PWM_MASK	 		= 0x20;		//!< 0 = individual PWM (default), 1 = common PWM
	static const uint8_t REG_PORT_BLINK_PERIOD_MASK 		= 0x1c;		//!< Blink period
	static const uint8_t REG_PORT_BLINK_PERIOD_OFF			= 0;		//!< Blinking disabled (does not blink) (default)
	static const uint8_t REG_PORT_BLINK_PERIOD_256		 	= 1 << 2;	//!< Blink period 256ms
	static const uint8_t REG_PORT_BLINK_PERIOD_512		 	= 2 << 2;	//!< Blink period 512ms
	static const uint8_t REG_PORT_BLINK_PERIOD_1024		 	= 3 << 2;	//!< Blink period 1024ms
	static const uint8_t REG_PORT_BLINK_PERIOD_2048		 	= 4 << 2;	//!< Blink period 2048ms
	static const uint8_t REG_PORT_BLINK_PERIOD_4096		 	= 5 << 2;	//!< Blink period 4096ms
	static const uint8_t REG_PORT_BLINK_ON_TIME_MASK 		= 0x03;		//!< Blink on time
	static const uint8_t REG_PORT_BLINK_ON_50_PCT	 		= 0;		//!< LED on for 50% of blink period (default)
	static const uint8_t REG_PORT_BLINK_ON_25_PCT	 		= 1;		//!< LED on for 25% of blink period
	static const uint8_t REG_PORT_BLINK_ON_12_5_PCT	 		= 2;		//!< LED on for 12.5% of blink period
	static const uint8_t REG_PORT_BLINK_ON_6_25_PCT	 		= 3;		//!< LED on for 6.25% of blink period

	static const uint8_t PORT7_MASK							= 0b10000000; //!< PORT7 (bit D7) mask
	static const uint8_t PORT6_MASK							= 0b01000000; //!< PORT6 (bit D6) mask
	static const uint8_t PORT5_MASK							= 0b00100000; //!< PORT5 (bit D5) mask
	static const uint8_t PORT4_MASK							= 0b00010000; //!< PORT4 (bit D4) mask
	static const uint8_t PORT3_MASK							= 0b00001000; //!< PORT3 (bit D3) mask
	static const uint8_t PORT2_MASK							= 0b00000100; //!< PORT2 (bit D2) mask
	static const uint8_t PORT1_MASK							= 0b00000010; //!< PORT1 (bit D1) mask
	static const uint8_t PORT0_MASK							= 0b00000001; //!< PORT0 (bit D0) mask

protected:
	/**
	 * @brief The I2C address (0x00 - 0x7f). Default is 0x37.
	 *
	 * If you passed in an address 0 - 7 into the constructor, 0x38 - 0x3f is stored here.
	 */
	uint8_t addr;

	/**
	 * @brief The I2C interface to use. Default is Wire. Could be Wire1 or Wire3 on some devices.
	 */
	TwoWire &wire;


	MAX7360KeyMappingBase *keyMapping = 0;
};


#endif /* __MAX7360_RK_H */
