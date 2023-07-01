#if defined(ARDUINO_ARCH_ESP8266)

#include "RBDmcuESP8266.h"


int pulseWidth = 1;
volatile int current_dim = 0;
int intervalTime = 800; // 1/80MHz	= 12.5ns  * 400 = 10us

static int toggleCounter = 0;
static int toggleReload = 25;

static dimmerLamp* dimmer[ALL_DIMMERS];
volatile uint16_t dimPower[ALL_DIMMERS];
volatile uint16_t dimOutPin[ALL_DIMMERS];
volatile uint16_t dimZCPin[ALL_DIMMERS];
volatile uint16_t zeroCross[ALL_DIMMERS];
volatile DIMMER_MODE_typedef dimMode[ALL_DIMMERS];
volatile ON_OFF_typedef dimState[ALL_DIMMERS];
volatile uint16_t dimCounter[ALL_DIMMERS];
static uint16_t dimPulseBegin[ALL_DIMMERS];
volatile uint16_t togMax[ALL_DIMMERS];
volatile uint16_t togMin[ALL_DIMMERS];
volatile bool togDir[ALL_DIMMERS];

dimmerLamp::dimmerLamp(int user_dimmer_pin, int zc_dimmer_pin):
	dimmer_pin(user_dimmer_pin),
	zc_pin(zc_dimmer_pin)
{
	current_dim++;
	dimmer[current_dim-1] = this;
	current_num = current_dim-1;
	toggle_state = false;
	
	dimPulseBegin[current_dim-1] = 1;
	dimOutPin[current_dim-1] = user_dimmer_pin;
	dimZCPin[current_dim-1] = zc_dimmer_pin;
	dimCounter[current_dim-1] = 0;
	zeroCross[current_dim-1] = 0;
	dimMode[current_dim-1] = NORMAL_MODE;
	togMin[current_dim-1] = 0;
	togMax[current_dim-1] = 1;
	pinMode(user_dimmer_pin, OUTPUT);
}

void dimmerLamp::timer_init(void)
{
	timer1_attachInterrupt(onTimerISR);
	timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
	timer1_write(intervalTime); 
}

void dimmerLamp::ext_int_init(void) 
{
	int inPin = dimZCPin[this->current_num];
	pinMode(inPin, INPUT_PULLUP);
	attachInterrupt(inPin, isr_ext, FALLING);
}


void dimmerLamp::begin(DIMMER_MODE_typedef DIMMER_MODE, ON_OFF_typedef ON_OFF)
{
	dimMode[this->current_num] = DIMMER_MODE;
	dimState[this->current_num] = ON_OFF;
	timer_init();
	ext_int_init();	
}

void dimmerLamp::setPower(int power)
{	
	if (power < 0) 
		power = 0;
	if (power >= 1000) 
		power = 1000;
	
	dimPower[this->current_num] = power;
	dimPulseBegin[this->current_num] = map(power, 0, 1000, 750, 250);
}

int dimmerLamp::getPower(void)
{
	if (dimState[this->current_num] == ON)
		return dimPower[this->current_num];
	else return 0;
}

void dimmerLamp::setState(ON_OFF_typedef ON_OFF)
{
	dimState[this->current_num] = ON_OFF;
}

bool dimmerLamp::getState(void)
{
	bool ret;
	if (dimState[this->current_num] == ON) ret = true;
	else ret = false;
	return ret;
}

void dimmerLamp::changeState(void)
{
	if (dimState[this->current_num] == ON) dimState[this->current_num] = OFF;
	else 
		dimState[this->current_num] = ON;
}

DIMMER_MODE_typedef dimmerLamp::getMode(void)
{
	return dimMode[this->current_num];
}

void dimmerLamp::setMode(DIMMER_MODE_typedef DIMMER_MODE)
{
	dimMode[this->current_num] = DIMMER_MODE;
}

void dimmerLamp::toggleSettings(int minValue, int maxValue)
{
	if (maxValue > 99) 
	{
    	maxValue = 99;
	}
	if (minValue < 1) 
	{
    	minValue = 1;
	}
	dimMode[this->current_num] = TOGGLE_MODE;
	togMin[this->current_num] = powerBuf[maxValue];
	togMax[this->current_num] = powerBuf[minValue];

	toggleReload = 50;
}
 
void IRAM_ATTR isr_ext()
{
	for (int i = 0; i < current_dim; i++ ) 
		if (dimState[i] == ON) 
		{
			zeroCross[i] = 1;
		}
}


static int k;
void IRAM_ATTR onTimerISR()
{	
	
	toggleCounter++;
	for (k = 0; k < current_dim; k++)
	{
		if (zeroCross[k] == 1 )
		{
			dimCounter[k]++;

			if (dimMode[k] == TOGGLE_MODE)
			{
			/*****
			 * TOGGLE DIMMING MODE
			 *****/
			if (dimPulseBegin[k] >= togMax[k]) 	
			{
				// if reach max dimming value 
				togDir[k] = false;	// downcount				
			}
			if (dimPulseBegin[k] <= togMin[k])
			{
				// if reach min dimming value 
				togDir[k] = true;	// upcount
			}
			if (toggleCounter == toggleReload) 
			{
				if (togDir[k] == true) dimPulseBegin[k]++;
				else dimPulseBegin[k]--;
			}
			}
			
			/*****
			 * DEFAULT DIMMING MODE (NOT TOGGLE)
			 *****/
			if (dimCounter[k] >= dimPulseBegin[k] )
			{
				digitalWrite(dimOutPin[k], HIGH);	
			}

			if (dimCounter[k] >=  (dimPulseBegin[k] + pulseWidth) )
			{
				digitalWrite(dimOutPin[k], LOW);
				zeroCross[k] = 0;
				dimCounter[k] = 0;
			}
		}
	}
	if (toggleCounter >= toggleReload)
		toggleCounter = 1;

}

#endif