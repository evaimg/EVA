/**************************************************************************
*
* Filename:    PS3000Acon.c
*
* Copyright:   Pico Technology Limited 2010
*
* Author:      RPM
*
* Description:
*   This is a console mode program that demonstrates how to use the
*   PicoScope 3000a series API.
*
* Examples:
*    Collect a block of samples immediately
*    Collect a block of samples when a trigger event occurs
*    Collect a stream of data immediately
*    Collect a stream of data when a trigger event occurs
*    Set Signal Generator, using standard or custom signals
*    Change timebase & voltage scales
*    Display data in mV or ADC counts
*    Handle power source changes (PicoScope34xxA/B devices only)
*
* Digital Examples (MSO veriants only): 
*    Collect a block of digital samples immediately
*    Collect a block of digital samples when a trigger event occurs
*    Collect a block of analogue & digital samples when analogue AND digital trigger events occurs
*    Collect a block of analogue & digital samples when analogue OR digital trigger events occurs
*    Collect a stream of digital data immediately
*   Collect a stream of digital data and show aggregated values
*
*	To build this application:
*			 Set up a project for a 32-bit console mode application
*			 Add this file to the project
*			 Add PS3000A.lib to the project
*			 Add ps3000aApi.h and picoStatus.h to the project
*			 Build the project
*
***************************************************************************/
#include "PS3000Acon.h"
#define PREF4 __stdcall

int cycles = 0;

#define BUFFER_SIZE 	60000

#define QUAD_SCOPE		4
#define DUAL_SCOPE		2

	short * buffers[1];
	short * digiBuffer[PS3000A_MAX_DIGITAL_PORTS];

/****************************************************************************
* Callback
* used by PS3000A data streaimng collection calls, on receipt of data.
* used to set global flags etc checked by user routines
****************************************************************************/
void PREF4 CallBackStreaming(	short handle,
	int32_t noOfSamples,
	uint32_t	startIndex,
	short overflow,
	uint32_t triggerAt,
	short triggered,
	short autoStop,
	void	*pParameter)
{
	int channel;
	BUFFER_INFO * bufferInfo = NULL;

	if (pParameter != NULL)
		bufferInfo = (BUFFER_INFO *) pParameter;

	// used for streaming
	g_sampleCount = noOfSamples;
	g_startIndex	= startIndex;
	g_autoStopped		= autoStop;

	// flag to say done reading data
	g_ready = TRUE;

	// flags to show if & where a trigger has occurred
	g_trig = triggered;
	g_trigAt = triggerAt;

	if (bufferInfo != NULL && noOfSamples)
	{
		if (bufferInfo->mode == ANALOGUE)
		{
			for (channel = 0; channel < bufferInfo->unit->channelCount; channel++)
			{
				if (bufferInfo->unit->channelSettings[channel].enabled)
				{
					if (bufferInfo->appBuffers && bufferInfo->driverBuffers)
					{
						if (bufferInfo->appBuffers[channel * 2]  && bufferInfo->driverBuffers[channel * 2])
						{
							memcpy_s (&bufferInfo->appBuffers[channel * 2][startIndex], noOfSamples * sizeof(short),
								&bufferInfo->driverBuffers[channel * 2][startIndex], noOfSamples * sizeof(short));
						}
						if (bufferInfo->appBuffers[channel * 2 + 1] && bufferInfo->driverBuffers[channel * 2 + 1])
						{
							memcpy_s (&bufferInfo->appBuffers[channel * 2 + 1][startIndex], noOfSamples * sizeof(short),
								&bufferInfo->driverBuffers[channel * 2 + 1][startIndex], noOfSamples * sizeof(short));
						}
					}
				}
			}
		}
		else if (bufferInfo->mode == AGGREGATED)
		{
			for (channel = 0; channel < bufferInfo->unit->digitalPorts; channel++)
			{
				if (bufferInfo->appDigBuffers && bufferInfo->driverDigBuffers)
				{
					if (bufferInfo->appDigBuffers[channel * 2] && bufferInfo->driverDigBuffers[channel * 2])
					{
						memcpy_s (&bufferInfo->appDigBuffers[channel * 2][startIndex], noOfSamples * sizeof(short),
							&bufferInfo->driverDigBuffers[channel * 2][startIndex], noOfSamples * sizeof(short));
					}
					if (bufferInfo->appDigBuffers[channel * 2 + 1] && bufferInfo->driverDigBuffers[channel * 2 + 1])
					{
						memcpy_s (&bufferInfo->appDigBuffers[channel * 2 + 1][startIndex], noOfSamples * sizeof(short),
							&bufferInfo->driverDigBuffers[channel * 2 + 1][startIndex], noOfSamples * sizeof(short));
					}
				}
			}
		}
		else if (bufferInfo->mode == DIGITAL)
		{
			for (channel = 0; channel < bufferInfo->unit->digitalPorts; channel++)
			{
				if (bufferInfo->appDigBuffers && bufferInfo->driverDigBuffers)
				{
					if (bufferInfo->appDigBuffers[channel] && bufferInfo->driverDigBuffers[channel])
					{
						memcpy_s (&bufferInfo->appDigBuffers[channel][startIndex], noOfSamples * sizeof(short),
							&bufferInfo->driverDigBuffers[channel][startIndex], noOfSamples * sizeof(short));
					}
				}
			}
		}
	}
}

/****************************************************************************
* Callback
* used by PS3000A data block collection calls, on receipt of data.
* used to set global flags etc checked by user routines
****************************************************************************/
void PREF4 CallBackBlock( short handle, PICO_STATUS status, void * pParameter)
{
	if (status != PICO_CANCELLED)
		g_ready = TRUE;
}

/****************************************************************************
* SetDefaults - restore default settings
****************************************************************************/
void SetDefaults(UNIT * unit)
{
	PICO_STATUS status;
	int i;

	status = ps3000aSetEts(unit->handle, PS3000A_ETS_OFF, 0, 0, NULL);					// Turn off ETS
	printf(status?"SetDefaults:ps3000aSetEts------ 0x%08lx \n":"", status);

	for (i = 0; i < unit->channelCount; i++) // reset channels to most recent settings
	{
		status = ps3000aSetChannel(unit->handle, (PS3000A_CHANNEL)(PS3000A_CHANNEL_A + i),
			unit->channelSettings[PS3000A_CHANNEL_A + i].enabled,
			(PS3000A_COUPLING)unit->channelSettings[PS3000A_CHANNEL_A + i].DCcoupled,
			(PS3000A_RANGE)unit->channelSettings[PS3000A_CHANNEL_A + i].range, 0);
		printf(status?"SetDefaults:ps3000aSetChannel------ 0x%08lx \n":"", status);
	}
}

/****************************************************************************
* SetDigitals - enable or disable Digital Channels
****************************************************************************/
PICO_STATUS SetDigitals(UNIT *unit, short state)
{
	PICO_STATUS status=0;

	short logicLevel;
	float logicVoltage = 1.5;
	short maxLogicVoltage = 5;

	short timebase = 1;
	short port;


	// Set logic threshold
	logicLevel =  (short) ((logicVoltage / maxLogicVoltage) * PS3000A_MAX_LOGIC_LEVEL);

	// Enable Digital ports
	for (port = PS3000A_DIGITAL_PORT0; port <= PS3000A_DIGITAL_PORT1; port++)
	{
		status = ps3000aSetDigitalPort(unit->handle, (PS3000A_DIGITAL_PORT)port, state, logicLevel);
		printf(status?"SetDigitals:PS3000ASetDigitalPort(Port 0x%X) ------ 0x%08lx \n":"", port, status);
	}
	return status;
}

/****************************************************************************
* DisableAnalogue - Disable Analogue Channels
****************************************************************************/
PICO_STATUS DisableAnalogue(UNIT *unit)
{
	PICO_STATUS status=0;
	short ch;

	// Turn off digital ports
	for (ch = 0; ch < unit->channelCount; ch++)
	{
		if((status = ps3000aSetChannel(unit->handle, (PS3000A_CHANNEL) ch, 0, PS3000A_DC, PS3000A_50MV, 0)) != PICO_OK)
			printf("DisableAnalogue:ps3000aSetChannel(channel %d) ------ 0x%08lx \n", ch, status);
	}
	return status;
}


/****************************************************************************
* adc_to_mv
*
* Convert an 16-bit ADC count into millivolts
****************************************************************************/
int adc_to_mv(int32_t raw, int ch, UNIT * unit)
{
	return (raw * inputRanges[ch]) / unit->maxValue;
}

/****************************************************************************
* mv_to_adc
*
* Convert a millivolt value into a 16-bit ADC count
*
*  (useful for setting trigger thresholds)
****************************************************************************/
short mv_to_adc(short mv, short ch, UNIT * unit)
{
	return (mv * unit->maxValue) / inputRanges[ch];
}

/****************************************************************************************
* ChangePowerSource - function to handle switches between +5V supply, and USB only power
* Only applies to ps34xxA/B units 
******************************************************************************************/
PICO_STATUS ChangePowerSource(short handle, PICO_STATUS status)
{
	char ch;

	switch (status)
	{
	case PICO_POWER_SUPPLY_NOT_CONNECTED:			// User must acknowledge they want to power via usb
		do
		{
			printf("\n5V power supply not connected");
			printf("\nDo you want to run using USB only Y/N?\n");
			ch = toupper(_getch());
			if(ch == 'Y')
			{
				printf("\nPowering the unit via USB\n");
				status = ps3000aChangePowerSource(handle, PICO_POWER_SUPPLY_NOT_CONNECTED);		// Tell the driver that's ok
				if (status == PICO_POWER_SUPPLY_UNDERVOLTAGE)
					status = ChangePowerSource(handle, status);
			}
		}
		while(ch != 'Y' && ch != 'N');
		printf(ch == 'N'?"Please use the +5V power supply to power this unit\n":"");
		break;

	case PICO_POWER_SUPPLY_CONNECTED:
		printf("\nUsing +5V power supply voltage\n");
		status = ps3000aChangePowerSource(handle, PICO_POWER_SUPPLY_CONNECTED);					// Tell the driver we are powered from +5V supply
		break;

	case PICO_POWER_SUPPLY_UNDERVOLTAGE:
		do
		{
			printf("\nUSB not supplying required voltage");
			printf("\nPlease plug in the +5V power supply\n");
			printf("\nHit any key to continue, or Esc to exit...\n");
			ch = _getch();
			if (ch == 0x1B)	// ESC key
				exit(0);
			else
				status = ps3000aChangePowerSource(handle, PICO_POWER_SUPPLY_CONNECTED);		// Tell the driver that's ok
		}
		while (status == PICO_POWER_SUPPLY_REQUEST_INVALID);
		break;
	}
	return status;
}

/****************************************************************************
* ClearDataBuffers
*
* stops GetData writing values to memory that has been released
****************************************************************************/
PICO_STATUS ClearDataBuffers(UNIT * unit)
{
	int i;
	PICO_STATUS status=1;

	for (i = 0; i < unit->channelCount; i++) 
	{
		if((status = ps3000aSetDataBuffers(unit->handle, (PS3000A_CHANNEL) i, NULL, NULL, 0, 0, PS3000A_RATIO_MODE_NONE)) != PICO_OK)
			printf("ClearDataBuffers:ps3000aSetDataBuffers(channel %d) ------ 0x%08lx \n", i, status);
	}


	for (i= 0; i < unit->digitalPorts; i++) 
	{
		if((status = ps3000aSetDataBuffer(unit->handle, (PS3000A_CHANNEL) (i + PS3000A_DIGITAL_PORT0), NULL, 0, 0, PS3000A_RATIO_MODE_NONE))!= PICO_OK)
			printf("ClearDataBuffers:ps3000aSetDataBuffer(port 0x%X) ------ 0x%08lx \n", i + PS3000A_DIGITAL_PORT0, status);
	}

	return status;
}

/****************************************************************************
* BlockDataHandler
* - Used by all block data routines
* - acquires data (user sets trigger mode before calling), displays 10 items
*   and saves all to block.txt
* Input :
* - unit : the unit to use.
* - text : the text to display before the display of data slice
* - offset : the offset into the data buffer to start the display's slice.
****************************************************************************/
void BlockDataHandler(UNIT * unit, char * text, int offset, MODE mode)
{
	int i, j;
	int32_t timeInterval;
	int32_t sampleCount= BUFFER_SIZE;
	FILE * fp = NULL;
	int32_t maxSamples;

	int32_t timeIndisposed;
	unsigned short digiValue;
	PICO_STATUS status;
	short retry;

	if (mode == ANALOGUE || mode == MIXED)		// Analogue or  (MSO Only) MIXED 
	{
		for (i = 0; i < unit->channelCount; i++) 
		{
			//buffers[i * 2] = (short*)malloc(sampleCount * sizeof(short));
			//buffers[i * 2 + 1] = (short*)malloc(sampleCount * sizeof(short));
			status = ps3000aSetDataBuffers(unit->handle, (PS3000A_CHANNEL)i, buffers[i * 2], buffers[i * 2 + 1], sampleCount, 0, PS3000A_RATIO_MODE_NONE);
			printf(status?"BlockDataHandler:ps3000aSetDataBuffers(channel %d) ------ 0x%08lx \n":"", i, status);
		}
	}

	if (mode == DIGITAL || mode == MIXED)		// (MSO Only) Digital or MIXED
	{
		for (i= 0; i < unit->digitalPorts; i++) 
		{
			digiBuffer[i] = (short*)malloc(sampleCount* sizeof(short));
			status = ps3000aSetDataBuffer(unit->handle, (PS3000A_CHANNEL) (i + PS3000A_DIGITAL_PORT0), digiBuffer[i], sampleCount, 0, PS3000A_RATIO_MODE_NONE);
			printf(status?"BlockDataHandler:ps3000aSetDataBuffer(port 0x%X) ------ 0x%08lx \n":"", i + PS3000A_DIGITAL_PORT0, status);
		}
	}

	/*  find the maximum number of samples, the time interval (in timeUnits),
	*		 the most suitable time units, and the maximum oversample at the current timebase*/
	while (ps3000aGetTimebase(unit->handle, timebase, sampleCount, &timeInterval, oversample, &maxSamples, 0))
	{
		timebase++;
	}

	printf("\nTimebase: %lu  SampleInterval: %ldnS  oversample: %hd\n", timebase, timeInterval, oversample);

	/* Start it collecting, then wait for completion*/
	g_ready = FALSE;


	do
	{
		retry = 0;
		if((status = ps3000aRunBlock(unit->handle, 0, sampleCount, timebase, oversample,	&timeIndisposed, 0, CallBackBlock, NULL)) != PICO_OK)
		{
			if(status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_POWER_SUPPLY_UNDERVOLTAGE)       // 34xxA/B devices...+5V PSU connected or removed
			{
				status = ChangePowerSource(unit->handle, status);
				retry = 1;
			}
			else
			{
				printf("BlockDataHandler:ps3000aRunBlock ------ 0x%08lx \n", status);
				return;
			}
		}
	}
	while(retry);

	printf("Waiting for trigger...Press a key to abort\n");

	while (!g_ready && !_kbhit())
	{
		Sleep(0);
	}
	
	if(g_ready) 
	{
		if((status = ps3000aGetValues(unit->handle, 0, (uint32_t *) &sampleCount, 1, PS3000A_RATIO_MODE_NONE, 0, NULL)) != PICO_OK)
			if(status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_POWER_SUPPLY_UNDERVOLTAGE)
			{
				if (status == PICO_POWER_SUPPLY_UNDERVOLTAGE)
					ChangePowerSource(unit->handle, status);
				else
					printf("\nPower Source Changed. Data collection aborted.\n");
			}
			else
				printf("BlockDataHandler:ps3000aGetValues ------ 0x%08lx \n", status);
		else
		{
			/* Print out the first 10 readings, converting the readings to mV if required */
			printf("%s\n",text);

			if (mode == ANALOGUE || mode == MIXED)		// if we're doing analogue or MIXED
			{
				printf("Channels are in (%s)\n", ( scaleVoltages ) ? ("mV") : ("ADC Counts"));

				for (j = 0; j < unit->channelCount; j++) 
				{
					if (unit->channelSettings[j].enabled) 
						printf("Channel%c:    ", 'A' + j);
				}
			}
			if (mode == DIGITAL || mode == MIXED)	// if we're doing digital or MIXED
				printf("Digital");

			printf("\n");

			for (i = offset; i < offset+10; i++) 
			{
				if (mode == ANALOGUE || mode == MIXED)	// if we're doing analogue or MIXED
				{
					for (j = 0; j < unit->channelCount; j++) 
					{
						if (unit->channelSettings[j].enabled) 
						{
							printf("  %6d     ", scaleVoltages ? 
								adc_to_mv(buffers[j * 2][i], unit->channelSettings[PS3000A_CHANNEL_A + j].range, unit)	// If scaleVoltages, print mV value
								: buffers[j * 2][i]);																	// else print ADC Count
						}
					}
				}
				if (mode == DIGITAL || mode == MIXED)	// if we're doing digital or MIXED
				{
					digiValue = 0x00ff & digiBuffer[1][i];
					digiValue <<= 8;
					digiValue |= digiBuffer[0][i];
					printf("0x%04X", digiValue);
				}
				printf("\n");
			}

			if (mode == ANALOGUE || mode == MIXED)		// if we're doing analogue or MIXED
			{
				sampleCount = min(sampleCount, BUFFER_SIZE);

				fopen_s(&fp, BlockFile, "w");
				if (fp != NULL)
				{
					fprintf(fp, "Block Data log\n\n");
					fprintf(fp,"Results shown for each of the %d Channels are......\n",unit->channelCount);
					fprintf(fp,"Maximum Aggregated value ADC Count & mV, Minimum Aggregated value ADC Count & mV\n\n");

					fprintf(fp, "Time  ");
					for (i = 0; i < unit->channelCount; i++) 
						if (unit->channelSettings[i].enabled) 
							fprintf(fp," Ch   Max ADC   Max mV   Min ADC   Min mV   ");
					fprintf(fp, "\n");

					for (i = 0; i < sampleCount; i++) 
					{
						fprintf(fp, "%5lld ", g_times[0] + (long long)(i * timeInterval));
						for (j = 0; j < unit->channelCount; j++) 
						{
							if (unit->channelSettings[j].enabled) 
							{
								fprintf(	fp,
									"Ch%C  %6d = %+6dmV, %6d = %+6dmV   ",
									'A' + j,
									buffers[j * 2][i],
									adc_to_mv(buffers[j * 2][i], unit->channelSettings[PS3000A_CHANNEL_A + j].range, unit),
									buffers[j * 2 + 1][i],
									adc_to_mv(buffers[j * 2 + 1][i], unit->channelSettings[PS3000A_CHANNEL_A + j].range, unit));
							}
						}
						fprintf(fp, "\n");
					}
				}
				else
				{
					printf(	"Cannot open the file %s for writing.\n"
						"Please ensure that you have permission to access.\n", BlockFile);
				}
			}
		} 
	}
	else 
	{
		printf("data collection aborted\n");
		//_getch();
	}

	if ((status = ps3000aStop(unit->handle)) != PICO_OK)
		printf("BlockDataHandler:ps3000aStop ------ 0x%08lx \n", status);

	if (fp != NULL)
		fclose(fp);

	//if (mode == ANALOGUE || mode == MIXED)		// Only if we allocated these buffers
	//{
	//	for (i = 0; i < unit->channelCount * 2; i++) 
	//	{
	//		free(buffers[i]);
	//	}
	//}

	//if (mode == DIGITAL || mode == MIXED)		// Only if we allocated these buffers
	//{
	//	for (i = 0; i < unit->digitalPorts; i++) 
	//	{
	//		free(digiBuffer[i]);
	//	}
	//}
	ClearDataBuffers(unit);
}


/****************************************************************************
* SetTrigger
*
* - Used to call aall the functions required to set up triggering
*
***************************************************************************/
PICO_STATUS SetTrigger(	UNIT * unit,
struct tPS3000ATriggerChannelProperties * channelProperties,
	short nChannelProperties,
	PS3000A_TRIGGER_CONDITIONS_V2 * triggerConditionsV2,
	short nTriggerConditions,
	TRIGGER_DIRECTIONS * directions,
struct tPwq * pwq,
	uint32_t delay,
	short auxOutputEnabled,
	int32_t autoTriggerMs,
	PS3000A_DIGITAL_CHANNEL_DIRECTIONS * digitalDirections,
	short nDigitalDirections)
{
	PICO_STATUS status;

	if ((status = ps3000aSetTriggerChannelProperties(unit->handle,
		channelProperties,
		nChannelProperties,
		auxOutputEnabled,
		autoTriggerMs)) != PICO_OK) 
	{
		printf("SetTrigger:ps3000aSetTriggerChannelProperties ------ Ox%08lx \n", status);
		return status;
	}

	if ((status = ps3000aSetTriggerChannelConditionsV2(unit->handle, triggerConditionsV2, nTriggerConditions)) != PICO_OK) 
	{
		printf("SetTrigger:ps3000aSetTriggerChannelConditions ------ 0x%08lx \n", status);
		return status;
	}

	if ((status = ps3000aSetTriggerChannelDirections(unit->handle,
		directions->channelA,
		directions->channelB,
		directions->channelC,
		directions->channelD,
		directions->ext,
		directions->aux)) != PICO_OK) 
	{
		printf("SetTrigger:ps3000aSetTriggerChannelDirections ------ 0x%08lx \n", status);
		return status;
	}

	if ((status = ps3000aSetTriggerDelay(unit->handle, delay)) != PICO_OK) 
	{
		printf("SetTrigger:ps3000aSetTriggerDelay ------ 0x%08lx \n", status);
		return status;
	}

	if((status = ps3000aSetPulseWidthQualifierV2(unit->handle, 
		pwq->conditions,
		pwq->nConditions, 
		pwq->direction,
		pwq->lower, 
		pwq->upper, 
		pwq->type)) != PICO_OK)
	{
		printf("SetTrigger:ps3000aSetPulseWidthQualifier ------ 0x%08lx \n", status);
		return status;
	}

	if (unit->digitalPorts)					
	{
		if((status = ps3000aSetTriggerDigitalPortProperties(unit->handle, digitalDirections, nDigitalDirections)) != PICO_OK) 
		{
			printf("SetTrigger:ps3000aSetTriggerDigitalPortProperties ------ 0x%08lx \n", status);
			return status;
		}
	}
	return status;
}

/****************************************************************************
* CollectBlockImmediate
*  this function demonstrates how to collect a single block of data
*  from the unit (start collecting immediately)
****************************************************************************/
void CollectBlockImmediate(UNIT * unit)
{
	struct tPwq pulseWidth;
	struct tTriggerDirections directions;

	memset(&directions, 0, sizeof(struct tTriggerDirections));
	memset(&pulseWidth, 0, sizeof(struct tPwq));

	printf("Collect block immediate...\n");
	printf("Press a key to start\n");
	////_getch();

	SetDefaults(unit);

	/* Trigger disabled	*/
	SetTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	BlockDataHandler(unit, "First 10 readings\n", 0, ANALOGUE);
}


/****************************************************************************
* Select input voltage ranges for channels
****************************************************************************/
void picoSetVoltages(UNIT * unit,short rangeSet)
{
	int ch;

    for (ch = 0; ch < unit->channelCount; ch++) 
	{
		if (rangeSet >= unit->firstRange && rangeSet <= unit->lastRange)
			unit->channelSettings[ch].range = rangeSet;
		else
			unit->channelSettings[ch].range = unit->lastRange;
	}
	SetDefaults(unit);	// Put these changes into effect
}

int32_t timeInterval;

void picoSetTimebase(UNIT *unit,uint32_t timebase_)
{
	
	int32_t maxSamples;
	timebase = timebase_;
	while (ps3000aGetTimebase(unit->handle, timebase, BUFFER_SIZE, &timeInterval, 1, &maxSamples, 0))
	{
		timebase++;  // Increase timebase if the one specified can't be used. 
	}

    //printf("Timebase used %lu = %ldns Sample Interval\n", timebase, timeInterval);
	//oversample = TRUE;
}

uint32_t _timeout=500; // 5s
void picoInitRapidBlock(UNIT * unit,int32_t sampleOffset_,uint32_t timeout)
{

	int32_t maxSamples;
    int32_t sampleCount_= BUFFER_SIZE;
	int i =0;
	PICO_STATUS status;
	short triggerVoltage = mv_to_adc(inputRanges[unit->channelSettings[PS3000A_CHANNEL_A].range]/2,unit->channelSettings[PS3000A_CHANNEL_A].range, unit);

	struct tPS3000ATriggerChannelProperties sourceDetails = {	triggerVoltage,
		256 * 10,
		triggerVoltage,
		256 * 10,
		PS3000A_EXTERNAL,
		PS3000A_LEVEL};

	struct tPS3000ATriggerConditionsV2 conditions = {	PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_TRUE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE};

	struct tPwq pulseWidth;

	struct tTriggerDirections directions = {	PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_RISING,
		PS3000A_NONE };

	memset(&pulseWidth, 0, sizeof(struct tPwq));


	//unit->channelSettings[0].enabled  = 1;
	//unit->channelSettings[1].enabled  = 1;

	_timeout = timeout;
	//printf("Collect block triggered...\n");
	//printf("Collects when value rises past %d", scaleVoltages?
	//	adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS3000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
	//	: sourceDetails.thresholdUpper);																// else print ADC Count
	//printf(scaleVoltages?"mV\n" : "ADC Counts\n");

	//_getch();

	SetDefaults(unit);

	/* Trigger enabled
	* Rising edge
	* Threshold = 1000mV */

	SetTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth,sampleOffset_, 0, 0, 0, 0);

	//buffers[0] = (short*)malloc(BUFFER_SIZE * sizeof(short));
	//buffers[1] = (short*)malloc(BUFFER_SIZE * sizeof(short));
	//status = ps3000aSetDataBuffers(unit->handle, (PS3000A_CHANNEL)i, buffers[0], buffers[1], BUFFER_SIZE, 0, PS3000A_RATIO_MODE_NONE);
	//printf(status?"BlockDataHandler:ps3000aSetDataBuffers(channel %d) ------ 0x%08lx \n":"", i, status);

	/*  find the maximum number of samples, the time interval (in timeUnits),
	*		 the most suitable time units, and the maximum oversample at the current timebase*/
	while (ps3000aGetTimebase(unit->handle, timebase, sampleCount_, &timeInterval, oversample, &maxSamples, 0))
	{
		timebase++;
	}



}
/****************************************************************************
* picoRunRapidBlock
*  this function demonstrates how to collect a set of captures using 
*  rapid block mode.
****************************************************************************/
PICO_STATUS picoStartRapidBlock(UNIT * unit,unsigned short nCaptures,uint32_t nSamples,uint32_t *CompletedNSample,uint32_t *nCompletedCaptures,int32_t *nMaxSamples,short * pBuf)
{
	int32_t timeIndisposed;
	short  channel;
	uint32_t capture;
	short ***rapidBuffers;
	short *overflow;
	PICO_STATUS status;
	uint32_t count=0;
	short i;
	uint32_t j;
	int32_t lIndex;
	short retry;
	unsigned short nCapturesWanted = nCaptures;
	unsigned short nSampleWanted = nSamples;
	int c=0;
	//Segment the memory
	status = ps3000aMemorySegments(unit->handle, nCaptures, nMaxSamples);
	if(status != PICO_OK)
		return status;
	if(nSamples>*nMaxSamples)
		nSamples = *nMaxSamples;

	//Set the number of captures
	status = ps3000aSetNoOfCaptures(unit->handle, nCaptures);
	if(status != PICO_OK)
		return status;
	//Run

	do
	{
		retry = 0;
		g_ready = 0;
		if((status = ps3000aRunBlock(unit->handle, 0, nSamples, timebase, 1, &timeIndisposed, 0, CallBackBlock, NULL)) != PICO_OK)
		{
			if(status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED)
			{
				status = ChangePowerSource(unit->handle, status);
				retry = 1;
			}
			else
			{
				printf("BlockDataHandler:ps3000aRunBlock ------ 0x%08lx \n", status);
				return status;
			}
		}
	}
	while(retry);


	for (channel = 0; channel < unit->channelCount; channel++) 
	{
		if(unit->channelSettings[channel].enabled)
		{
			for (capture = 0; capture < nCaptures; capture++) 
			{
				status = ps3000aSetDataBuffer(unit->handle, (PS3000A_CHANNEL)channel, pBuf + nSampleWanted*capture+c*(nSampleWanted*nCaptures), nSamples, capture, PS3000A_RATIO_MODE_NONE);
				if(status != PICO_OK)
					return status;
			}
			c++;
		}
	}
	return status;
}
int picoCheckRapidBlockDataReady()
{	
	Sleep(1);
	return g_ready;
}
/****************************************************************************
* picoRunRapidBlock
*  this function demonstrates how to collect a set of captures using 
*  rapid block mode.
****************************************************************************/
PICO_STATUS picoGetRapidBlockData(UNIT * unit,unsigned short nCaptures,uint32_t nSamples,uint32_t *CompletedNSample,uint32_t *nCompletedCaptures,short * pBuf)
{
		int32_t timeIndisposed;
	short  channel;
	uint32_t capture;
	short ***rapidBuffers;
	short *overflow;
	PICO_STATUS status;

	short i;
	uint32_t j;
	int32_t lIndex;
	short retry;
	unsigned short nCapturesWanted = nCaptures;
	unsigned short nSampleWanted = nSamples;
	int32_t nMaxSamples;

	//Wait until data ready

	if(!g_ready)
	{
		//_getch();
		status = ps3000aStop(unit->handle);
		if(status != PICO_OK)
			return status;
		status = ps3000aGetNoOfCaptures(unit->handle, nCompletedCaptures);
		if(status != PICO_OK)
			return status;
		//printf("Rapid capture aborted. %lu complete blocks were captured\n", *nCompletedCaptures);
		//printf("\nPress any key...\n\n");
		//_getch();

		if(nCompletedCaptures == 0)
			return 100;

		//Only display the blocks that were captured
		nCaptures = (unsigned short)(*nCompletedCaptures);

	}

	//Allocate memory
	overflow = (short *) calloc(unit->channelCount * nCaptures, sizeof(short));

	//Get data
	status = ps3000aGetValuesBulk(unit->handle, CompletedNSample, 0, nCaptures - 1, 1, PS3000A_RATIO_MODE_NONE, overflow);
	if (status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED)
	{
		printf("\nPower Source Changed. Data collection aborted.\n");
		return status;
	}
	nSamples = *CompletedNSample;
	if (status == PICO_OK)
	{

		for (capture=0; capture<nCaptures; capture++)
		{ 		
			for (j=0; j<nSamples; j++)
			{
				lIndex = nSampleWanted*capture + j;
				*(pBuf + lIndex) = *(pBuf + lIndex)+32768;
			}

		}

	}

	//Stop
	status = ps3000aStop(unit->handle);
	//Free memory
	free(overflow);

	return status;
}
PICO_STATUS picoStopRapidBlock(UNIT * unit)
{
	PICO_STATUS status;
	//Stop
	status = ps3000aStop(unit->handle);
	return status;
}
/****************************************************************************
* picoRunRapidBlock
*  this function demonstrates how to collect a set of captures using 
*  rapid block mode.
****************************************************************************/
PICO_STATUS picoRunRapidBlock(UNIT * unit,unsigned short nCaptures,uint32_t nSamples,uint32_t *CompletedNSample,uint32_t *nCompletedCaptures,short * pBuf)
{
	int32_t timeIndisposed;
	short  channel=0;
	uint32_t capture=0;
	short *overflow;
	PICO_STATUS status=1;
	uint32_t count=0;
	short i;
	uint32_t j;
	int32_t lIndex=0;
	short retry=2;
	unsigned short nCapturesWanted = nCaptures;
	unsigned short nSampleWanted = nSamples;
	int32_t nMaxSamples=512;
	int c=0;
	//Segment the memory
	status = ps3000aMemorySegments(unit->handle, nCaptures, &nMaxSamples);

	if(nSamples>nMaxSamples)
		nSamples = nMaxSamples;

	//Set the number of captures
	status = ps3000aSetNoOfCaptures(unit->handle, nCaptures);

	//Run

	g_ready = 0;
	do
	{
		retry = 0;
		if((status = ps3000aRunBlock(unit->handle, 0, nSamples, timebase, 1, &timeIndisposed, 0, CallBackBlock, NULL)) != PICO_OK)
		{
			if(status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED)
			{
				status = ChangePowerSource(unit->handle, status);
				retry = 1;
			}
			else
			{
				printf("BlockDataHandler:ps3000aRunBlock ------ 0x%08lx \n", status);
				return status;
			}
		}
	}
	while(retry);

	//_timeout += nCaptures/100;  //adjust timeout according to capture number
	//Wait until data ready

	while (!g_ready && count< _timeout )
	{
		Sleep(1);
		count++;
	}

	if(!g_ready)
	{
		////_getch();
		//status = ps3000aStop(unit->handle);
		//status = ps3000aGetNoOfCaptures(unit->handle, nCompletedCaptures);
		////printf("Rapid capture aborted. %lu complete blocks were captured\n", *nCompletedCaptures);
		////printf("\nPress any key...\n\n");
		////_getch();

		//if(nCompletedCaptures == 0)
		//	return 100;

		////Only display the blocks that were captured
		//nCaptures = (unsigned short)(*nCompletedCaptures);
		status = ps3000aStop(unit->handle);
		return PICO_TRIGGER_ERROR;
	}

	status = ps3000aGetNoOfCaptures(unit->handle, nCompletedCaptures);
	if(status != PICO_OK)
		return PICO_TRIGGER_ERROR;

	//	for (channel = 0; channel < unit->channelCount; channel++) 
	//{
	//	if(unit->channelSettings[channel].enabled)
	//	{
	//		for (capture = 0; capture < nCaptures; capture++) 
	//		{
	//			status = ps3000aSetDataBuffer(unit->handle, (PS3000A_CHANNEL)channel, pBuf + nSampleWanted*capture, nSamples, capture, PS3000A_RATIO_MODE_NONE);
	//		}
	//	}
	//}

	c=0;
	for (channel = 0; channel < unit->channelCount; channel++) 
	{
		if(unit->channelSettings[channel].enabled)
		{
			for (capture = 0; capture < nCaptures; capture++) 
			{
				status = ps3000aSetDataBuffer(unit->handle, (PS3000A_CHANNEL)channel, pBuf + nSampleWanted*capture+c*(nSampleWanted*nCaptures), nSamples, capture, PS3000A_RATIO_MODE_NONE);
				if(status != PICO_OK)
					return status;
			}
			c++;
		}
	}


	//Allocate memory
	overflow = (short *) calloc(unit->channelCount * nCaptures, sizeof(short));

	//for (channel = 0; channel < unit->channelCount; channel++) 
	//{
	//	rapidBuffers[channel] = (short **) calloc(nCaptures, sizeof(short*));
	//}

	//for (channel = 0; channel < unit->channelCount; channel++) 
	//{	
	//	if(unit->channelSettings[channel].enabled)
	//	{
	//		for (capture = 0; capture < nCaptures; capture++) 
	//		{
	//			rapidBuffers[channel][capture] = (short *) calloc(nSamples, sizeof(short));
	//		}
	//	}
	//}



	//Get data
	status = ps3000aGetValuesBulk(unit->handle, CompletedNSample, 0, nCaptures - 1, 1, PS3000A_RATIO_MODE_NONE, overflow);
	if (status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED)
	{
		printf("\nPower Source Changed. Data collection aborted.\n");
		return status;
	}
	nSamples = *CompletedNSample;
	if (status == PICO_OK)
	{
		c=0;
		for (channel = 0; channel < unit->channelCount; channel++) 
		{
			if(unit->channelSettings[channel].enabled)
			{
				for (capture=0; capture<nCaptures; capture++)
				{ 		
					for (j=0; j<nSamples; j++)
					{
						lIndex = nSampleWanted*capture + j+c*(nSampleWanted*nCaptures);
						*(pBuf + lIndex) = *(pBuf + lIndex)+32768;
					}

				}
				c++;
			}
		}
	}
	else
	{
		//Stop
		ps3000aStop(unit->handle);
		//Free memory
		free(overflow);
		return status;
	}
	//Stop
	status = ps3000aStop(unit->handle);
	//Free memory
	free(overflow);
	return status;
}


void picoInitBlock(UNIT * unit,int32_t sampleOffset_)
{
	int32_t maxSamples;
    int32_t sampleCount_= BUFFER_SIZE;
	int i =0;
	PICO_STATUS status;
	short triggerVoltage = mv_to_adc(1000, unit->channelSettings[PS3000A_CHANNEL_A].range, unit);

	struct tPS3000ATriggerChannelProperties sourceDetails = {	triggerVoltage,
		256 * 10,
		triggerVoltage,
		256 * 10,
		PS3000A_EXTERNAL,
		PS3000A_LEVEL};

	struct tPS3000ATriggerConditionsV2 conditions = {	PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_TRUE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE};

	struct tPwq pulseWidth;

	struct tTriggerDirections directions = {	PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_RISING,
		PS3000A_NONE };

	memset(&pulseWidth, 0, sizeof(struct tPwq));


	unit->channelSettings[0].enabled  = 1;
	unit->channelSettings[1].enabled  = 1;

	//printf("Collect block triggered...\n");
	//printf("Collects when value rises past %d", scaleVoltages?
	//	adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS3000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
	//	: sourceDetails.thresholdUpper);																// else print ADC Count
	//printf(scaleVoltages?"mV\n" : "ADC Counts\n");

	//_getch();

	SetDefaults(unit);

	/* Trigger enabled
	* Rising edge
	* Threshold = 1000mV */

	SetTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth,sampleOffset_, 0, 0, 0, 0);

	buffers[0] = (short*)malloc(BUFFER_SIZE * sizeof(short));
	buffers[1] = (short*)malloc(BUFFER_SIZE * sizeof(short));
	status = ps3000aSetDataBuffers(unit->handle, (PS3000A_CHANNEL)i, buffers[0], buffers[1], BUFFER_SIZE, 0, PS3000A_RATIO_MODE_NONE);
	printf(status?"BlockDataHandler:ps3000aSetDataBuffers(channel %d) ------ 0x%08lx \n":"", i, status);

	/*  find the maximum number of samples, the time interval (in timeUnits),
	*		 the most suitable time units, and the maximum oversample at the current timebase*/
	while (ps3000aGetTimebase(unit->handle, timebase, sampleCount_, &timeInterval, oversample, &maxSamples, 0))
	{
		timebase++;
	}

}
PICO_STATUS picoRunBlock(UNIT *unit,int32_t sampleLength_,uint32_t timeout,uint32_t *sampleCountRet)
{
	int i, j;
	//int32_t timeInterval;

	//FILE * fp = NULL;

	int32_t timeIndisposed;
	uint32_t count=0;
	uint32_t sampleCount = sampleLength_;
	PICO_STATUS status;
	short retry;
		/* Start it collecting, then wait for completion*/
	g_ready = FALSE;
	

	//if(sampleCount_ != sampleCount){
	//	sampleCount_ = sampleCount;
	//	picoInitBlock(unit);
	//}
	do
	{
		retry = 0;
		if((status = ps3000aRunBlock(unit->handle, 0, sampleCount, timebase, oversample,	&timeIndisposed, 0, CallBackBlock, NULL)) != PICO_OK)
		{
			if(status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_POWER_SUPPLY_UNDERVOLTAGE)       // 34xxA/B devices...+5V PSU connected or removed
			{
				status = ChangePowerSource(unit->handle, status);
				retry = 1;
			}
			else
			{
				//printf("BlockDataHandler:ps3000aRunBlock ------ 0x%08lx \n", status);
				return status;
			}
		}
	}
	while(retry);

	//printf("Waiting for trigger...Press a key to abort\n");

	while (!g_ready && count< timeout )
	{
		if(count<0.9*timeout)
		Sleep(0);
		else
		Sleep(1);
		count++;
	}
	
	if(g_ready) 
	{
		if((status = ps3000aGetValues(unit->handle, 0,  &sampleCount, 1, PS3000A_RATIO_MODE_NONE, 0, NULL)) != PICO_OK)
		{
			if(status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_POWER_SUPPLY_UNDERVOLTAGE)
			{
				if (status == PICO_POWER_SUPPLY_UNDERVOLTAGE)
					ChangePowerSource(unit->handle, status);
				else
					printf("\nPower Source Changed. Data collection aborted.\n");
			}
			/*else
				printf("BlockDataHandler:ps3000aGetValues ------ 0x%08lx \n", status);*/
		}
		else
		{
				*sampleCountRet = min(sampleCount, BUFFER_SIZE);

				/*fopen_s(&fp, BlockFile, "w");
				if (fp != NULL)
				{
					fprintf(fp, "Block Data log\n\n");
					fprintf(fp,"Results shown for each of the %d Channels are......\n",unit->channelCount);
					fprintf(fp,"Maximum Aggregated value ADC Count & mV, Minimum Aggregated value ADC Count & mV\n\n");

					fprintf(fp, "Time  ");
					for (i = 0; i < unit->channelCount; i++) 
						if (unit->channelSettings[i].enabled) 
							fprintf(fp," Ch   Max ADC   Max mV   Min ADC   Min mV   ");
					fprintf(fp, "\n");

					for (i = 0; i < sampleCount; i++) 
					{
						fprintf(fp, "%5lld ", g_times[0] + (int32_t int32_t)(i * timeInterval));
						for (j = 0; j < unit->channelCount; j++) 
						{
							if (unit->channelSettings[j].enabled) 
							{
								fprintf(	fp,
									"Ch%C  %6d = %+6dmV, %6d = %+6dmV   ",
									'A' + j,
									buffers[j * 2][i],
									adc_to_mv(buffers[j * 2][i], unit->channelSettings[PS3000A_CHANNEL_A + j].range, unit),
									buffers[j * 2 + 1][i],
									adc_to_mv(buffers[j * 2 + 1][i], unit->channelSettings[PS3000A_CHANNEL_A + j].range, unit));
							}
						}
						fprintf(fp, "\n");
					}
				}
				else
				{
					printf(	"Cannot open the file %s for writing.\n"
						"Please ensure that you have permission to access.\n", BlockFile);
				}*/
		} 
	}
	else
	{
		return 1; // Error
	}
	//else 
	//{
	//	printf("data collection aborted\n");
	//	//_getch();
	//}

	//if ((status = ps3000aStop(unit->handle)) != PICO_OK)
	//	printf("BlockDataHandler:ps3000aStop ------ 0x%08lx \n", status);

	//if (fp != NULL)
	//	fclose(fp);

	//ClearDataBuffers(unit);
	return 0;
}
/****************************************************************************
* CollectBlockTriggered
*  this function demonstrates how to collect a single block of data from the
*  unit, when a trigger event occurs.
****************************************************************************/
void CollectBlockTriggered(UNIT * unit)
{
	//short	triggerVoltage;

	short triggerVoltage = mv_to_adc(1000, unit->channelSettings[PS3000A_CHANNEL_A].range, unit);

	struct tPS3000ATriggerChannelProperties sourceDetails = {	triggerVoltage,
		256 * 10,
		triggerVoltage,
		256 * 10,
		PS3000A_CHANNEL_A,
		PS3000A_LEVEL};

	struct tPS3000ATriggerConditionsV2 conditions = {	PS3000A_CONDITION_TRUE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE};

	struct tPwq pulseWidth;

	struct tTriggerDirections directions = {	PS3000A_RISING,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE };

	memset(&pulseWidth, 0, sizeof(struct tPwq));

	printf("Collect block triggered...\n");
	printf("Collects when value rises past %d", scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS3000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	printf(scaleVoltages?"mV\n" : "ADC Counts\n");

	printf("Press a key to start...\n");
	//_getch();

	SetDefaults(unit);

	/* Trigger enabled
	* Rising edge
	* Threshold = 1000mV */
	SetTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, 0, 0);
	//picoInitBlock(unit);
	//picoRunBlock(unit);
	BlockDataHandler(unit, "Ten readings after trigger\n", 0, ANALOGUE);
}

/****************************************************************************
* CollectRapidBlock
*  this function demonstrates how to collect a set of captures using 
*  rapid block mode.
****************************************************************************/
void CollectRapidBlock(UNIT * unit)
{
	unsigned short nCaptures;
	int32_t nMaxSamples;
	uint32_t nSamples = 1000;
	int32_t timeIndisposed;
	short capture, channel;
	short ***rapidBuffers;
	short *overflow;
	PICO_STATUS status;
	short i;
	uint32_t nCompletedCaptures;
	short retry;

	short	triggerVoltage = mv_to_adc(1000, unit->channelSettings[PS3000A_CHANNEL_A].range, unit);

	struct tPS3000ATriggerChannelProperties sourceDetails = {	triggerVoltage,
		256 * 10,
		triggerVoltage,
		256 * 10,
		PS3000A_CHANNEL_A,
		PS3000A_LEVEL};

	struct tPS3000ATriggerConditionsV2 conditions = {	PS3000A_CONDITION_TRUE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE,
		PS3000A_CONDITION_DONT_CARE};

	struct tPwq pulseWidth;

	struct tTriggerDirections directions = {	PS3000A_RISING,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE,
		PS3000A_NONE };

	memset(&pulseWidth, 0, sizeof(struct tPwq));

	printf("Collect rapid block triggered...\n");
	printf("Collects when value rises past %d",	scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS3000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	printf(scaleVoltages?"mV\n" : "ADC Counts\n");
	printf("Press any key to abort\n");

	SetDefaults(unit);

	// Trigger enabled
	SetTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	//Set the number of captures
	nCaptures = 10;

	//Segment the memory
	status = ps3000aMemorySegments(unit->handle, nCaptures, &nMaxSamples);

	//Set the number of captures
	status = ps3000aSetNoOfCaptures(unit->handle, nCaptures);

	//Run
	timebase = 160;		//1 MS/s

	do
	{
		retry = 0;
		if((status = ps3000aRunBlock(unit->handle, 0, nSamples, timebase, 1, &timeIndisposed, 0, CallBackBlock, NULL)) != PICO_OK)
		{
			if(status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED)
			{
				status = ChangePowerSource(unit->handle, status);
				retry = 1;
			}
			else
				printf("BlockDataHandler:ps3000aRunBlock ------ 0x%08lx \n", status);
		}
	}
	while(retry);

	//Wait until data ready
	g_ready = 0;
	while(!g_ready && !_kbhit())
	{
		Sleep(0);
	}

	if(!g_ready)
	{
		//_getch();
		status = ps3000aStop(unit->handle);
		status = ps3000aGetNoOfCaptures(unit->handle, &nCompletedCaptures);
		printf("Rapid capture aborted. %lu complete blocks were captured\n", nCompletedCaptures);
		printf("\nPress any key...\n\n");
		//_getch();

		if(nCompletedCaptures == 0)
			return;

		//Only display the blocks that were captured
		nCaptures = (unsigned short)nCompletedCaptures;
	}

	//Allocate memory
	rapidBuffers = (short ***) calloc(unit->channelCount, sizeof(short*));
	overflow = (short *) calloc(unit->channelCount * nCaptures, sizeof(short));

	for (channel = 0; channel < unit->channelCount; channel++) 
	{
		rapidBuffers[channel] = (short **) calloc(nCaptures, sizeof(short*));
	}

	for (channel = 0; channel < unit->channelCount; channel++) 
	{	
		if(unit->channelSettings[channel].enabled)
		{
			for (capture = 0; capture < nCaptures; capture++) 
			{
				rapidBuffers[channel][capture] = (short *) calloc(nSamples, sizeof(short));
			}
		}
	}

	for (channel = 0; channel < unit->channelCount; channel++) 
	{
		if(unit->channelSettings[channel].enabled)
		{
			for (capture = 0; capture < nCaptures; capture++) 
			{
				status = ps3000aSetDataBuffer(unit->handle, (PS3000A_CHANNEL)channel, rapidBuffers[channel][capture], nSamples, capture, PS3000A_RATIO_MODE_NONE);
			}
		}
	}

	//Get data
	status = ps3000aGetValuesBulk(unit->handle, &nSamples, 0, nCaptures - 1, 1, PS3000A_RATIO_MODE_NONE, overflow);
	if (status == PICO_POWER_SUPPLY_CONNECTED || status == PICO_POWER_SUPPLY_NOT_CONNECTED)
		printf("\nPower Source Changed. Data collection aborted.\n");

	if (status == PICO_OK)
	{
		//print first 10 samples from each capture
		for (capture = 0; capture < nCaptures; capture++)
		{
			printf("\nCapture %d\n", capture + 1);
			for (channel = 0; channel < unit->channelCount; channel++) 
			{
				printf("Channel %c:\t", 'A' + channel);
			}
			printf("\n");

			for(i = 0; i < 10; i++)
			{
				for (channel = 0; channel < unit->channelCount; channel++) 
				{
					if(unit->channelSettings[channel].enabled)
					{
						printf("   %6d       ", scaleVoltages ? 
							adc_to_mv(rapidBuffers[channel][capture][i], unit->channelSettings[PS3000A_CHANNEL_A +channel].range, unit)	// If scaleVoltages, print mV value
							: rapidBuffers[channel][capture][i]);																	// else print ADC Count
					}
				}
				printf("\n");
			}
		}
	}

	//Stop
	status = ps3000aStop(unit->handle);

	//Free memory
	free(overflow);

	for (channel = 0; channel < unit->channelCount; channel++) 
	{	
		if(unit->channelSettings[channel].enabled)
		{
			for (capture = 0; capture < nCaptures; capture++) 
			{
				free(rapidBuffers[channel][capture]);
			}
		}
	}

	for (channel = 0; channel < unit->channelCount; channel++) 
	{
		free(rapidBuffers[channel]);
	}
	free(rapidBuffers);
}

/****************************************************************************
* Initialise unit' structure with Variant specific defaults
****************************************************************************/
void get_info(UNIT * unit)
{
	char description [6][25]= { "Driver Version",
		"USB Version",
		"Hardware Version",
		"Variant Info",
		"Serial",
		"Error Code" };
	short i, r = 0;
	char line [80];
	int variant;
	PICO_STATUS status = PICO_OK;

	if (unit->handle) 
	{
		for (i = 0; i < 5; i++) 
		{
			status = ps3000aGetUnitInfo(unit->handle, (int8_t *)line, sizeof (line), &r, i);
			if (i == 3) 
			{
				variant = atoi(line);
				//To identify varians.....

				if (strlen(line) == 5)										// A or B variant unit 
				{
					line[4] = toupper(line[4]);

					if (line[1] == '2' && line[4] == 'A')		// i.e 3204A -> 0xA204
						variant += 0x9580;
					else
						if (line[1] == '2' && line[4] == 'B')		//i.e 3204B -> 0xB204
							variant +=0xA580;
						else
							if (line[1] == '4' && line[4] == 'A')		// i.e 3404A -> 0xA404
								variant += 0x96B8;
							else
								if (line[1] == '4' && line[4] == 'B')		//i.e 3404B -> 0xB404
									variant +=0xA6B8;
				}

				if (strlen(line) == 7)
				{
					line[4] = toupper(line[4]);
					line[5] = toupper(line[5]);
					line[6] = toupper(line[6]);

					if(strcmp(line+4, "MSO") == 0)
						variant += 0xc580;						// 3204MSO -> 0xD204
				}
			}
			printf("%s: %s\n", description[i], line);
		}

		switch (variant)
		{
		case MODEL_PS3204A:
			unit->model			= MODEL_PS3204A;
			unit->sigGen		= SIGGEN_FUNCTGEN;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= DUAL_SCOPE;
			unit->ETS			= FALSE;
			unit->AWGFileSize	= MIN_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3204B:
			unit->model			= MODEL_PS3204B;
			unit->sigGen		= SIGGEN_AWG;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= DUAL_SCOPE;
			unit->ETS			= FALSE;
			unit->AWGFileSize	= MAX_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3205A:
			unit->model			= MODEL_PS3205A;
			unit->sigGen		= SIGGEN_FUNCTGEN;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= DUAL_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= MIN_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3205B:
			unit->model			= MODEL_PS3205B;
			unit->sigGen		= SIGGEN_AWG;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= DUAL_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= MAX_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3206A:
			unit->model			= MODEL_PS3206A;
			unit->sigGen		= SIGGEN_FUNCTGEN;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= DUAL_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= MIN_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3206B:
			unit->model			= MODEL_PS3206B;
			unit->sigGen		= SIGGEN_AWG;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= DUAL_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= PS3206B_MAX_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3404A:
			unit->model			= MODEL_PS3404A;
			unit->sigGen		= SIGGEN_FUNCTGEN;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= QUAD_SCOPE;
			unit->ETS			= FALSE;
			unit->AWGFileSize	= MIN_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3404B:
			unit->model			= MODEL_PS3404B;
			unit->sigGen		= SIGGEN_AWG;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= QUAD_SCOPE;
			unit->ETS			= FALSE;
			unit->AWGFileSize	= MAX_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3405A:
			unit->model			= MODEL_PS3405A;
			unit->sigGen		= SIGGEN_FUNCTGEN;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= QUAD_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= MIN_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3405B:
			unit->model			= MODEL_PS3405B;
			unit->sigGen		= SIGGEN_AWG;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= QUAD_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= MAX_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3406A:
			unit->model			= MODEL_PS3406A;
			unit->sigGen		= SIGGEN_FUNCTGEN;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= QUAD_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= MIN_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3406B:
			unit->model			= MODEL_PS3406B;
			unit->sigGen		= SIGGEN_AWG;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= QUAD_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= PS3206B_MAX_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3204MSO:
			unit->model			= MODEL_PS3204MSO;
			unit->sigGen		= SIGGEN_AWG;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= DUAL_SCOPE;
			unit->ETS			= FALSE;
			unit->AWGFileSize	= MAX_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 2;
			break;

		case MODEL_PS3205MSO:
			unit->model			= MODEL_PS3205MSO;
			unit->sigGen		= SIGGEN_AWG;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= DUAL_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= MAX_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 2;
			break;

		case MODEL_PS3206MSO:
			unit->model			= MODEL_PS3206MSO;
			unit->sigGen		= SIGGEN_AWG;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= DUAL_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= PS3206B_MAX_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 2;
			break;

		case MODEL_PS3207A:
			unit->model			= MODEL_PS3207A;
			unit->sigGen		= SIGGEN_FUNCTGEN;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= DUAL_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= MIN_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		case MODEL_PS3207B:
			unit->model			= MODEL_PS3207B;
			unit->sigGen		= SIGGEN_AWG;
			unit->firstRange	= PS3000A_50MV;
			unit->lastRange		= PS3000A_20V;
			unit->channelCount	= DUAL_SCOPE;
			unit->ETS			= TRUE;
			unit->AWGFileSize	= PS3206B_MAX_SIG_GEN_BUFFER_SIZE;
			unit->digitalPorts	= 0;
			break;

		default:
			printf("\nUnsupported device");
			exit(0);
		}
	}
}


/****************************************************************************
* Select input voltage ranges for channels
****************************************************************************/
void SetVoltages(UNIT * unit)
{
	int i, ch;
	int count = 0;

	/* See what ranges are available... */
	for (i = unit->firstRange; i <= unit->lastRange; i++) 
	{
		printf("%d -> %d mV\n", i, inputRanges[i]);
	}

	do
	{
		/* Ask the user to select a range */
		printf("Specify voltage range (%d..%d)\n", unit->firstRange, unit->lastRange);
		printf("99 - switches channel off\n");
		for (ch = 0; ch < unit->channelCount; ch++) 
		{
			printf("\n");
			do 
			{
				printf("Channel %c: ", 'A' + ch);
				fflush(stdin);
				scanf_s("%hd", &unit->channelSettings[ch].range);
			} while (unit->channelSettings[ch].range != 99 && (unit->channelSettings[ch].range < unit->firstRange || unit->channelSettings[ch].range > unit->lastRange));

			if (unit->channelSettings[ch].range != 99) 
			{
				printf(" - %d mV\n", inputRanges[unit->channelSettings[ch].range]);
				unit->channelSettings[ch].enabled = TRUE;
				count++;
			} 
			else 
			{
				printf("Channel Switched off\n");
				unit->channelSettings[ch].enabled = FALSE;
				unit->channelSettings[ch].range = PS3000A_MAX_RANGES-1;
			}
		}
		printf(count == 0? "\n** At least 1 channel must be enabled **\n\n":"");
	}
	while(count == 0);	// must have at least one channel enabled

	SetDefaults(unit);	// Put these changes into effect
}

/****************************************************************************
*
* Select timebase, set oversample to on and time units as nano seconds
*
****************************************************************************/
void SetTimebase(UNIT unit)
{
	int32_t timeInterval;
	int32_t maxSamples;

	printf("Specify desired timebase: ");
	fflush(stdin);
	scanf_s("%lud", &timebase);

	while (ps3000aGetTimebase(unit.handle, timebase, BUFFER_SIZE, &timeInterval, 1, &maxSamples, 0))
	{
		timebase++;  // Increase timebase if the one specified can't be used. 
	}

	printf("Timebase used %lu = %ldns Sample Interval\n", timebase, timeInterval);
	oversample = TRUE;
}

/****************************************************************************
* Sets the signal generator
* - allows user to set frequency and waveform
* - allows for custom waveform (values -32768..32767) 
* - of up to 8192 samples int32_t (PS3x04B & PS3x05B) 16384 samples int32_t (PS3x06B)
******************************************************************************/
void SetSignalGenerator(UNIT unit)
{
	PICO_STATUS status;
	short waveform;
	uint32_t frequency = 1;
	char fileName [128];
	FILE * fp = NULL;
	short *arbitraryWaveform;
	int32_t waveformSize = 0;
	uint32_t pkpk = 4000000;
	int32_t offset = 0;
	char ch;
	short choice;
	double delta;


	while (_kbhit())			// use up keypress
		//_getch();

	do
	{
		printf("\nSignal Generator\n================\n");
		printf("0 - SINE         1 - SQUARE\n");
		printf("2 - TRIANGLE     3 - DC VOLTAGE\n");
		if(unit.sigGen == SIGGEN_AWG)
		{
			printf("4 - RAMP UP      5 - RAMP DOWN\n");
			printf("6 - SINC         7 - GAUSSIAN\n");
			printf("8 - HALF SINE    A - AWG WAVEFORM\n");
		}
		printf("F - SigGen Off\n\n");

		//ch = //_getch();

		//if (ch >= '0' && ch <='9')
		//	choice = ch -'0';
		//else
		//	ch = toupper(ch);
	}
	while(unit.sigGen == SIGGEN_FUNCTGEN && ch != 'F' && (ch < '0' || ch > '3') || unit.sigGen == SIGGEN_AWG && ch != 'A' && ch != 'F' && (ch < '0' || ch > '8')  );



	if(ch == 'F')				// If we're going to turn off siggen
	{
		printf("Signal generator Off\n");
		waveform = 8;		// DC Voltage
		pkpk = 0;				// 0V
		waveformSize = 0;
	}
	else
		if (ch == 'A' && unit.sigGen == SIGGEN_AWG)		// Set the AWG
		{
			arbitraryWaveform = (short*)malloc( unit.AWGFileSize * sizeof(short));
			memset(arbitraryWaveform, 0, unit.AWGFileSize * sizeof(short));

			waveformSize = 0;

			printf("Select a waveform file to load: ");
			scanf_s("%s", fileName, 128);
			if (fopen_s(&fp, fileName, "r") == 0) 
			{ // Having opened file, read in data - one number per line (max 8192 lines for PS3x04B & PS3x05B devices, 16384 for PS3x06B), with values in (0..4095)
				while (EOF != fscanf_s(fp, "%hi", (arbitraryWaveform + waveformSize))&& waveformSize++ < unit.AWGFileSize - 1);
				fclose(fp);
				printf("File successfully loaded\n");
			} 
			else 
			{
				printf("Invalid filename\n");
				return;
			}
		}
		else			// Set one of the built in waveforms
		{
			switch (choice)
			{
			case 0:
				waveform = PS3000A_SINE;
				break;

			case 1:
				waveform = PS3000A_SQUARE;
				break;

			case 2:
				waveform = PS3000A_TRIANGLE;
				break;

			case 3:
				waveform = PS3000A_DC_VOLTAGE;
				do 
				{
					printf("\nEnter offset in uV: (0 to 2000000)\n"); // Ask user to enter DC offset level;
					scanf_s("%lu", &offset);
				} while (offset < 0 || offset > 2000000);
				break;

			case 4:
				waveform = PS3000A_RAMP_UP;
				break;

			case 5:
				waveform = PS3000A_RAMP_DOWN;
				break;

			case 6:
				waveform = PS3000A_SINC;
				break;

			case 7:
				waveform = PS3000A_GAUSSIAN;
				break;

			case 8:
				waveform = PS3000A_HALF_SINE;
				break;

			default:
				waveform = PS3000A_SINE;
				break;
			}
		}

		if(waveform < 8 || (ch == 'A' && unit.sigGen == SIGGEN_AWG))				// Find out frequency if required
		{
			do 
			{
				printf("\nEnter frequency in Hz: (1 to 1000000)\n"); // Ask user to enter signal frequency;
				scanf_s("%lu", &frequency);
			} while (frequency <= 0 || frequency > 1000000);
		}

		if (waveformSize > 0)		
		{
			delta = ((1.0 * frequency * waveformSize) / unit.AWGFileSize) * (4294967296.0 * 5e-8); // delta >= 10

			status = ps3000aSetSigGenArbitrary(	unit.handle, 
				0,												// offset voltage
				pkpk,									// PkToPk in microvolts. Max = 4uV  +2v to -2V
				(uint32_t)delta,			// start delta
				(uint32_t)delta,			// stop delta
				0, 
				0, 
				arbitraryWaveform, 
				waveformSize, 
				(PS3000A_SWEEP_TYPE)0,
				(PS3000A_EXTRA_OPERATIONS)0, 
				PS3000A_SINGLE, 
				0, 
				0, 
				PS3000A_SIGGEN_RISING,
				PS3000A_SIGGEN_NONE, 
				0);

			printf(status?"\nps3000aSetSigGenArbitrary: Status Error 0x%x \n":"", (unsigned int)status);		// If status != 0, show the error
		} 
		else 
		{
			status = ps3000aSetSigGenBuiltIn(unit.handle, 
				offset, 
				pkpk, 
				waveform, 
				(float)frequency, 
				(float)frequency, 
				0, 
				0, 
				(PS3000A_SWEEP_TYPE)0, 
				(PS3000A_EXTRA_OPERATIONS)0, 
				0, 
				0, 
				(PS3000A_SIGGEN_TRIG_TYPE)0, 
				(PS3000A_SIGGEN_TRIG_SOURCE)0, 
				0);
			printf(status?"\nps3000aSetSigGenBuiltIn: Status Error 0x%x \n":"", (unsigned int)status);		// If status != 0, show the error
		}
}




/****************************************************************************
* DisplaySettings 
* Displays information about the user configurable settings in this example
* Parameters 
* - unit        pointer to the UNIT structure
*
* Returns       none
***************************************************************************/
void DisplaySettings(UNIT *unit)
{
	int ch;
	int voltage;

	printf("\n\nReadings will be scaled in (%s)\n", (scaleVoltages)? ("mV") : ("ADC counts"));

	for (ch = 0; ch < unit->channelCount; ch++)
	{
		if (!(unit->channelSettings[ch].enabled))
			printf("Channel %c Voltage Range = Off\n", 'A' + ch);
		else
		{
			voltage = inputRanges[unit->channelSettings[ch].range];
			printf("Channel %c Voltage Range = ", 'A' + ch);
			if (voltage < 1000)
				printf("%dmV\n", voltage);
			else
				printf("%dV\n", voltage / 1000);
		}
	}
	printf("\n");
}

/****************************************************************************
* OpenDevice 
* Parameters 
* - unit        pointer to the UNIT structure, where the handle will be stored
*
* Returns
* - PICO_STATUS to indicate success, or if an error occurred
***************************************************************************/
PICO_STATUS OpenDevice(UNIT *unit)
{
	char ch;
	short value = 0;
	int i;
	struct tPwq pulseWidth;
	struct tTriggerDirections directions;
	PICO_STATUS status = ps3000aOpenUnit(&(unit->handle), NULL);

	if (status == PICO_POWER_SUPPLY_NOT_CONNECTED || status == PICO_USB3_0_DEVICE_NON_USB3_0_PORT )
		status = ChangePowerSource(unit->handle, PICO_POWER_SUPPLY_NOT_CONNECTED);

	printf("Handle: %d\n", unit->handle);
	if (status != PICO_OK) 
	{
		printf("Unable to open device\n");
		printf("Error code : 0x%08lx\n", status);
		//while(!_kbhit());
		//exit(99); // exit program
	}

	printf("Device opened successfully, cycle %d\n\n", ++cycles);

	// setup devices
	get_info(unit);
	timebase = 1;

	ps3000aMaximumValue(unit->handle, &value);
	unit->maxValue = value;

	for ( i = 0; i < unit->channelCount; i++) 
	{
		unit->channelSettings[i].enabled = TRUE;
		unit->channelSettings[i].DCcoupled = TRUE;
		unit->channelSettings[i].range = PS3000A_5V;
	}

	memset(&directions, 0, sizeof(struct tTriggerDirections));
	memset(&pulseWidth, 0, sizeof(struct tPwq));

	SetDefaults(unit);

	/* Trigger disabled	*/
	SetTrigger(unit, NULL, 0, NULL, 0, &directions, &pulseWidth, 0, 0, 0, 0, 0);

	return status;
}



/****************************************************************************
* CloseDevice 
****************************************************************************/
void CloseDevice(UNIT *unit)
{
	ps3000aCloseUnit(unit->handle);
}


/****************************************************************************
* ANDAnalogueDigital
* This function shows how to collect a block of data from the analogue
* ports and the digital ports at the same time, triggering when the 
* digital conditions AND the analogue conditions are met
*
* Returns       none
***************************************************************************/
void ANDAnalogueDigitalTriggered(UNIT * unit)
{
	short	triggerVoltage = mv_to_adc(1000, unit->channelSettings[PS3000A_CHANNEL_A].range, unit);


	PS3000A_TRIGGER_CHANNEL_PROPERTIES sourceDetails = {	triggerVoltage,			// thresholdUpper
		256 * 10,				// thresholdUpper Hysteresis
		triggerVoltage,			// thresholdLower
		256 * 10,				// thresholdLower Hysteresis
		PS3000A_CHANNEL_A,		// channel
		PS3000A_LEVEL};			// mode

	PS3000A_TRIGGER_CONDITIONS_V2 conditions = {	
		PS3000A_CONDITION_TRUE,					// Channel A
		PS3000A_CONDITION_DONT_CARE,			// Channel B
		PS3000A_CONDITION_DONT_CARE,			// Channel C
		PS3000A_CONDITION_DONT_CARE,			// Channel D
		PS3000A_CONDITION_DONT_CARE,			// external
		PS3000A_CONDITION_DONT_CARE,			// aux 
		PS3000A_CONDITION_DONT_CARE,			// pwq
		PS3000A_CONDITION_TRUE					// digital
	};



	TRIGGER_DIRECTIONS directions = {	
		PS3000A_ABOVE,				// Channel A
		PS3000A_NONE,				// Channel B
		PS3000A_NONE,				// Channel C
		PS3000A_NONE,				// Channel D
		PS3000A_NONE,				// external
		PS3000A_NONE };				// aux

	PS3000A_DIGITAL_CHANNEL_DIRECTIONS digDirections[2];		// Array size can be up to 16, an entry for each digital bit

	PWQ pulseWidth;
	memset(&pulseWidth, 0, sizeof(PWQ));

	// Set the Digital trigger so it will trigger when bit 15 is HIGH and bit 13 is HIGH
	// All non-declared bits are taken as PS3000A_DIGITAL_DONT_CARE
	//

	digDirections[0].channel = PS3000A_DIGITAL_CHANNEL_0;
	digDirections[0].direction = PS3000A_DIGITAL_DIRECTION_RISING;

	digDirections[1].channel = PS3000A_DIGITAL_CHANNEL_4;
	digDirections[1].direction = PS3000A_DIGITAL_DIRECTION_HIGH;

	printf("\nCombination Block Triggered\n");
	printf("Collects when value is above %d", scaleVoltages?
		adc_to_mv(sourceDetails.thresholdUpper, unit->channelSettings[PS3000A_CHANNEL_A].range, unit)	// If scaleVoltages, print mV value
		: sourceDetails.thresholdUpper);																// else print ADC Count
	printf(scaleVoltages?"mV\n" : "ADC Counts\n");

	printf("AND \n");
	printf("Digital Channel  0   --- Rising\n");
	printf("Digital Channel  4   --- High\n");
	printf("Other Digital Channels - Don't Care\n");

	printf("Press a key to start...\n");
	//_getch();

	SetDefaults(unit);					// Enable Analogue channels.

	/* Trigger enabled
	* Rising edge
	* Threshold = 100mV */

	if (SetTrigger(unit, &sourceDetails, 1, &conditions, 1, &directions, &pulseWidth, 0, 0, 0, digDirections, 2) == PICO_OK)
	{
		BlockDataHandler(unit, "First 10 readings\n", 0, MIXED);
	}

	DisableAnalogue(unit);			// Disable Analogue ports when finished;
}

