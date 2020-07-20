// Repository: https://github.com/rickkas7/MAX7360-RK
// License: MIT

#include "MAX7360-RK.h"



MAX7360::MAX7360(uint8_t addr, TwoWire &wire) : addr(addr), wire(wire) {
	if (addr < 0x4) {
		// Just passed in 0 - 3, add in the 0x30 automatically to make addresses 0x30 - 0x33
		addr |= 0x30;
	}
}

MAX7360::~MAX7360() {

}


bool MAX7360::begin() {
	// Initialize the I2C bus in standard master mode.
	wire.begin();

	return true;
}


bool MAX7360::resetRegisterDefaults() {
	// Empty the FIFO. It's small, this won't take long.
	while(readRegister(REG_KEYS_FIFO) != MAX7360Key::FIFO_EMPTY) {
	}

	// Set low registers to factory defaults
	writeRegister(REG_CONFIG, 0b00001010);	
	writeRegister(REG_DEBOUNCE, 0xff);
	writeRegister(REG_KEY_SWITCH_INTERRUPT, 0x00);
	writeRegister(REG_GPO_CONTROL, 0b11111110);
	writeRegister(REG_AUTO_REPEAT, 0x00);
	writeRegister(REG_AUTO_SLEEP, 0b00000111);

	// Resets registers 0x40 - 0x5f
	setConfigResetGpio();

	return true;
}


MAX7360Key MAX7360::readKeyFIFO() {
	MAX7360Key result(keyMapping, readRegister(REG_KEYS_FIFO));

	return result;
}


uint8_t MAX7360::getConfiguration() {
	return readRegister(REG_CONFIG);
}

bool MAX7360::setConfiguration(uint8_t rawValue) {
	return writeRegister(REG_CONFIG, rawValue);
}


bool MAX7360::setConfigurationClearINTKonRead(bool value) {
	return setRegisterBitmask(REG_CONFIG, REG_CONFIG_INTERRUPT_MASK, value);
}


bool MAX7360::setConfigurationEnableKeyRelease(bool value) {
	return setRegisterBitmask(REG_CONFIG, REG_CONFIG_KEY_RELEASE_MASK, value);	
}

bool MAX7360::setConfigurationAutoWakeUp(bool value) {
	return setRegisterBitmask(REG_CONFIG, REG_CONFIG_AUTO_WAKEUP_MASK, value);
}


bool MAX7360::setConfigurationDisableI2CTimeouts(bool value) {
	return setRegisterBitmask(REG_CONFIG, REG_CONFIG_TIMEOUT_DISABLE_MASK, value);
}



uint8_t MAX7360::getDebounceTimeMs() {
	return (readRegister(REG_DEBOUNCE) & REG_DEBOUNCE_MASK) + REG_DEBOUNCE_MS_OFFSET;
}

bool MAX7360::setDebounceTimeMs(uint8_t value) {
	// Convert millisecond value to register value (9 - 40 ms => 0x00 - 0x1f)
	if (value < REG_DEBOUNCE_MS_OFFSET) {
		value = 0;
	}
	else {
		value -= REG_DEBOUNCE_MS_OFFSET;
	}
	if (value > REG_DEBOUNCE_MASK) {
		value = REG_DEBOUNCE_MASK;
	}

	return setRegisterMask(REG_DEBOUNCE, ~REG_DEBOUNCE_MASK, value);
}

uint8_t MAX7360::getGpoEnable() {
	return readRegister(REG_DEBOUNCE) & REG_GPO_ENABLE_MASK;
}

bool MAX7360::setGpoEnable(uint8_t value) {
	value &= REG_GPO_ENABLE_MASK;

	return setRegisterMask(REG_DEBOUNCE, ~REG_GPO_ENABLE_MASK, value);
}


bool MAX7360::setPortInterrupt(uint8_t port, bool enabled, bool risingAndFalling) {

	uint8_t mask = REG_PORT_INTERRUPT_MASK | REG_PORT_EDGE_MASK;

	uint8_t value = 0;
	if (enabled) {
		value |= REG_PORT_INTERRUPT_MASK;
	}
	if (risingAndFalling) {
		value |= REG_PORT_EDGE_MASK;
	}

	return setRegisterMask(REG_PORT_CONFIG + port, ~mask, value);
}


uint8_t MAX7360::readRegister(uint8_t reg) {
	wire.beginTransmission(addr);
	wire.write(reg);
	wire.endTransmission(false);

	wire.requestFrom(addr, (uint8_t) 1, (uint8_t) true);
	uint8_t value = (uint8_t) wire.read();

	// Log.trace("readRegister reg=%d value=%d", reg, value);

	return value;
}

bool MAX7360::writeRegister(uint8_t reg, uint8_t value) {

	wire.beginTransmission(addr);
	wire.write(reg);
	wire.write(value);

	int stat = wire.endTransmission(true);

	// Log.trace("writeRegister reg=%d value=%d stat=%d read=%d", reg, value, stat, readRegister(reg));

	return (stat == 0);
}


bool MAX7360::setRegisterMask(uint8_t reg, uint8_t andValue, uint8_t orValue) {
	uint8_t rawValue = readRegister(reg);

	rawValue &= andValue;
	rawValue |= orValue;

	return writeRegister(reg, rawValue);
}

bool MAX7360::setRegisterBitmask(uint8_t reg, uint8_t bitMask, bool set) {
	if (set) {
		return setRegisterMask(reg, 0xff, bitMask);
	}
	else {
		return setRegisterMask(reg, ~bitMask, 0);
	}
}

MAX7360Key::MAX7360Key() {

}

MAX7360Key::MAX7360Key(MAX7360KeyMappingBase *keyMapping, uint8_t rawValue) : keyMapping(keyMapping) {
	fromRawValue(rawValue);
}

void MAX7360Key::fromRawValue(uint8_t rawValue) {
	this->rawValue = rawValue;

	switch(rawValue) {
	case FIFO_EMPTY:
		rawKey = FIFO_KEY_NONE;
		more = false;
		released = false;
		break;

	case FIFO_OVERFLOW:
		rawKey = FIFO_KEY_NONE;
		more = true;
		released = false;
		break;
		
	case FIFO_KEY63_PRESSED:
		rawKey = 63;
		more = true;		
		break;

	case FIFO_KEY63_RELEASED:
		rawKey = 63;
		more = true;		
		released = true;
		break;

	case FIFO_KEY62_PRESSED:
		rawKey = 62;
		more = true;		
		break;

	case FIFO_KEY62_RELEASED:
		rawKey = 63;
		more = true;		
		released = true;
		break;

	case FIFO_KEY_REPEAT_MORE:
		rawKey = FIFO_KEY_NONE;
		more = true;
		released = false;
		break;
		
	case FIFO_KEY_REPEAT_DONE: 
		rawKey = FIFO_KEY_NONE;
		more = false;
		released = false;
		break;

	default:
		more = (rawValue & FIFO_EMPTY_MASK) == 0;
		released = (rawValue & FIFO_RELEASED_MASK) != 0;
		rawKey = rawValue & FIFO_KEY_MASK;
		break;
	}
}

char MAX7360Key::getMappedKey() const {
	if (rawKey != FIFO_KEY_NONE) {
		if (keyMapping) {
			return keyMapping->rawToReadable(rawKey);
		}
		else {
			return '0' + rawKey;
		}
	}
	else {
		return 0;
	}
}


MAX7360KeyMappingBase::MAX7360KeyMappingBase() {
}
MAX7360KeyMappingBase::~MAX7360KeyMappingBase() {
}

MAX7360KeyMappingTable::MAX7360KeyMappingTable(const char *table, size_t tableSize) : table(table), tableSize(tableSize) {

}
MAX7360KeyMappingTable::~MAX7360KeyMappingTable() {

}

char MAX7360KeyMappingTable::rawToReadable(uint8_t rawValue) {
	if (rawValue < tableSize) {
		return table[rawValue];
	}
	else {
		return 0;
	}
}

uint8_t MAX7360KeyMappingTable::readableToRaw(char c) {
	for(size_t ii = 0; ii < tableSize; ii++) {
		if (table[ii] == c) {
			return (uint8_t)ii;
		}
	}
	return MAX7360Key::FIFO_KEY_NONE;
}

static const char _phoneTable[24] = {
	'1', '4', '7', '*',   0,   0,   0,   0,
	'2', '5', '9', '0',   0,   0,   0,   0,
	'3', '6', '9', '#',   0,   0,   0,   0
};

MAX7360KeyMappingPhone::MAX7360KeyMappingPhone() : MAX7360KeyMappingTable(_phoneTable, sizeof(_phoneTable)) {
}

MAX7360KeyMappingPhone::~MAX7360KeyMappingPhone() {
}
	

