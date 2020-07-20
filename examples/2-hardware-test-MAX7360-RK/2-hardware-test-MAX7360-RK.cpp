#include "MAX7360-RK.h"

SYSTEM_THREAD(ENABLED);
  
SYSTEM_MODE(MANUAL);

SerialLogHandler logHandler;

MAX7360 keyDriver(0x38);
MAX7360KeyMappingPhone keyMapper;
bool lastPort5 = false;

enum class LedState {
	START,
	WAIT,
	ALL_ON,
	ALL_DIM,
	RED_ON,
	YELLOW_ON,
	GREEN_ON,
	BLINK_RED_SLOW,
	BLINK_GREEN_FAST,
	BLINK_DONE,
	FADE_START,
	FADE_UP,
	FADE_DOWN,
	ALL_OFF,
};
LedState ledState = LedState::START;
LedState ledNextState = LedState::START;
unsigned long ledTime = 0;
unsigned long ledDuration = 0;
int fadeCount = 0;
int rotaryCount = 0;

void ledStateHandler();

void setup() {
    waitFor(Serial.isConnected, 15000);
    delay(1000);
    
	keyDriver.withKeyMapping(&keyMapper);

	keyDriver.begin();

	// Reset default power-on register settings
	keyDriver.resetRegisterDefaults();

	// The power-on default is inexplicably to use COL2 - COL7 and GPO.
	// Disable GPO on the COL pins so the 4x3 key matrix will work on COL2.
	keyDriver.setGpoEnable(MAX7360::REG_GPO_DISABLED);

	// Only generate key down events, not key up
	keyDriver.setConfigurationEnableKeyRelease(false);

	// Enable PWM and constant current drivers
	keyDriver.setConfigEnableGpio();

	// Set PORT0 (red), PORT1 (green), PORT2 (blue) to output
	keyDriver.setGpioInputOutputMode(0b111);
	
	// Enable rotary encoder support of PORT6 and PORT7
	keyDriver.setConfigRotaryEncoder();
}

void loop() {
	MAX7360Key key = keyDriver.readKeyFIFO();
	if (!key.isEmpty()) {
		Log.info("rawKey=0x%02x readable=%c", key.getRawKey(), key.getMappedKey());
	}

	// PORT5 is connected to the switch on the rotary encoded (when the shaft is pressed)
	// There's a pull-up to it should normally be 1 and when pressed 0
	bool port5 = (keyDriver.readGpioInputs() & MAX7360::PORT5_MASK) != 0;
	if (port5 != lastPort5) {
		lastPort5 = port5;
		Log.info("port5=%d", port5);
	}

	int rotaryDelta = (int) keyDriver.readRotarySwitchCount();
	if (rotaryDelta != 0) {
		rotaryCount += rotaryDelta;
		Log.info("rotary count=%d delta=%d", rotaryCount, rotaryDelta);
	}

	ledStateHandler();
}

void ledSetNext(unsigned long durationMs, LedState nextState) {
	ledTime = millis();
	ledDuration = durationMs;
	ledState = LedState::WAIT;
	ledNextState = nextState;
}

void ledStateHandler() {
	switch(ledState) {
	case LedState::START:
		ledState = LedState::ALL_ON;
		break;

	case LedState::WAIT:
		if (millis() - ledTime >= ledDuration) {
			ledState = ledNextState;
		}
		break;

	case LedState::ALL_ON:
		Log.info("ALL_ON");
		keyDriver.setPortPwmRatio(0, 255);
		keyDriver.setPortPwmRatio(1, 255);
		keyDriver.setPortPwmRatio(2, 255);
		ledSetNext(2000, LedState::ALL_DIM);
		break;

	case LedState::ALL_DIM:
		Log.info("ALL_DIM");
		keyDriver.setPortPwmRatio(0, 64);
		keyDriver.setPortPwmRatio(1, 64);
		keyDriver.setPortPwmRatio(2, 64);
		ledSetNext(2000, LedState::RED_ON);
		break;

	case LedState::RED_ON:
		Log.info("RED_ON");
		keyDriver.setPortPwmRatio(0, 255);
		keyDriver.setPortPwmRatio(1, 0);
		keyDriver.setPortPwmRatio(2, 0);
		ledSetNext(2000, LedState::YELLOW_ON);
		break;

	case LedState::YELLOW_ON:
		Log.info("YELLOW_ON");
		keyDriver.setPortPwmRatio(0, 0);
		keyDriver.setPortPwmRatio(1, 255);
		keyDriver.setPortPwmRatio(2, 0);
		ledSetNext(2000, LedState::GREEN_ON);
		break;

	case LedState::GREEN_ON:
		Log.info("GREEN_ON");
		keyDriver.setPortPwmRatio(0, 0);
		keyDriver.setPortPwmRatio(1, 0);
		keyDriver.setPortPwmRatio(2, 255);
		ledSetNext(2000, LedState::BLINK_RED_SLOW);
		break;

	case LedState::BLINK_RED_SLOW:
		Log.info("BLINK_RED_SLOW");
		keyDriver.setPortPwmRatio(0, 255);
		keyDriver.setPortPwmRatio(1, 0);
		keyDriver.setPortPwmRatio(2, 0);
		keyDriver.setBlinkPeriod(0, MAX7360::REG_PORT_BLINK_PERIOD_1024);
		keyDriver.setBlinkOnTimePercent(0, MAX7360::REG_PORT_BLINK_ON_50_PCT);
		ledSetNext(6000, LedState::BLINK_GREEN_FAST);
		break;

	case LedState::BLINK_GREEN_FAST:
		Log.info("BLINK_GREEN_FAST");
		keyDriver.setBlinkPeriod(0, MAX7360::REG_PORT_BLINK_PERIOD_OFF);
		keyDriver.setPortPwmRatio(0, 0);
		keyDriver.setPortPwmRatio(1, 0);
		keyDriver.setPortPwmRatio(2, 255);
		keyDriver.setBlinkPeriod(2, MAX7360::REG_PORT_BLINK_PERIOD_256);
		keyDriver.setBlinkOnTimePercent(2, MAX7360::REG_PORT_BLINK_ON_25_PCT);
		ledSetNext(6000, LedState::BLINK_DONE);
		break;

	case LedState::BLINK_DONE:
		Log.info("BLINK_RED_DONE");
		for(uint8_t port = 0; port < 3; port++) {
			keyDriver.setBlinkPeriod(port, MAX7360::REG_PORT_BLINK_PERIOD_OFF);
			keyDriver.setBlinkOnTimePercent(port, MAX7360::REG_PORT_BLINK_ON_50_PCT);
		}
		ledState = LedState::FADE_START;
		break;

	case LedState::FADE_START:
		Log.info("FADE_START");
		fadeCount = 0;
		keyDriver.setCommmonPwmRatio(0);
		keyDriver.setCommonPwmMode(0, true);
		keyDriver.setCommonPwmMode(1, true);
		keyDriver.setCommonPwmMode(2, true);
		keyDriver.setConfigFadeTime(MAX7360::REG_GPIO_CONFIG_FADE_TIME_2048_MS);

		Log.info("FADE_UP");
		keyDriver.setCommmonPwmRatio(255);
		ledState = LedState::FADE_UP;
		ledTime = millis();
		break;

	case LedState::FADE_UP:
		if (millis() - ledTime >= 2048) {
			// Done fading up, fade down now
			Log.info("FADE_DOWN");
			keyDriver.setCommmonPwmRatio(0);
			ledState = LedState::FADE_DOWN;
			ledTime = millis();
		}
		break;

	case LedState::FADE_DOWN:
		if (millis() - ledTime >= 2048) {
			// Done fading down, fade down now
			if (++fadeCount < 3) {
				Log.info("FADE_UP");
				keyDriver.setCommmonPwmRatio(255);
				ledState = LedState::FADE_UP;
				ledTime = millis();
			}
			else {
				keyDriver.setCommonPwmMode(0, false);
				keyDriver.setCommonPwmMode(1, false);
				keyDriver.setCommonPwmMode(2, false);
				keyDriver.setConfigFadeTime(MAX7360::REG_GPIO_CONFIG_FADE_TIME_DISABLED);
				ledState = LedState::ALL_OFF;
			}
		}
		break;



	case LedState::ALL_OFF:
		Log.info("ALL_OFF");
		keyDriver.setPortPwmRatio(0, 0);
		keyDriver.setPortPwmRatio(1, 0);
		keyDriver.setPortPwmRatio(2, 0);
		ledSetNext(2000, LedState::START);
		break;

	}

}
