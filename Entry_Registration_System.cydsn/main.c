/*
------------Authors-------------
- Nishit Khara
- Parthiv Parikh
- Yash Gupte
*/


/*
 *  LED COLORS
 *  RED    		= 0x3 
 *  YELLOW 		= 0x2
 *  BLUE   		= 0x5     Scanning
 *  GREEN 		= 0x6     Advertising
 *  Purple		= 0x1     Pairing
 *  Light Blue 		= 0x4
 *  White      		= 0x8
*/

#include <project.h>
#include <stdio.h>

/* FOR IAS */
#define NO_ALERT           (0u)
#define MILD_ALERT         (1u)
#define HIGH_ALERT         (2u)


#define SIZE 7
#define FALSE 0
#define TRUE 1

/* BLE States */
#define DISCONNECTED                        (3u)
#define ADVERTISING                         (4u)
#define CONNECTED                           (5u)
#define SCANNING                            (6u)

uint8 state = DISCONNECTED;
uint8 attendanceMarked = FALSE;
uint8 addedDevices = 0;
uint8 maxDevices = SIZE;
uint8 address_store[10][6];
uint8 a[31];

char buff[100];

CYBLE_GAPC_ADV_REPORT_T  present_devices[SIZE];
uint8 permanentList[SIZE][6]=	{
				{0X39,0X00,0X13,0X01,0X00,0X06},	
				{0X04,0X00,0X13,0X01,0X00,0X06},
                                {0Xdc,0Xe8,0X38,0X9d,0X09,0X9a}
				};
															
/* FUNCTION DECLARATIONS */
void HandleScanDevices(CYBLE_GAPC_ADV_REPORT_T* advReport);
void IasEventHandler(uint32 event, void* eventParam);
void MembersPresent();
void resetval();

CY_ISR (press)
{  
	if (state == DISCONNECTED) 
	{
		if (CYBLE_ERROR_OK == CyBle_GappStartAdvertisement(CYBLE_SCANNING_FAST));
		else
		{
			myreg_Write(0x3);
			CyDelay(500);
			SW_ClearInterrupt();
		}
	}
	else if (state == SCANNING) 
	{
		//this will be the condition in which scanning is on and the lecture is over
		//so teacher will press the button and connect and say lecture over
		//it should jump to the evt and then start scanning
		CyBle_GapcStopScan();
		SW_ClearInterrupt();
	}
}

void BleStackHandler(uint32 event, void* eventParam)
{

	CYBLE_GAPC_ADV_REPORT_T advreport = *(CYBLE_GAPC_ADV_REPORT_T *)eventParam;
	switch(event) 
	{
		case CYBLE_EVT_STACK_ON:
		break;

		case CYBLE_EVT_GAPP_ADVERTISEMENT_START_STOP: // CyBle_GappStartAdvertisement || CyBle_GappStopAdvertisement() 
		if (state == DISCONNECTED) 		     // This will be when advertising is started
		{  
			state = ADVERTISING;
			myreg_Write(0x6);     // Green LED is ON for indicating Advertising
		}
		break;

		case CYBLE_EVT_GAPC_SCAN_PROGRESS_RESULT:	// CyBle_GapcStartScan
		if(advreport.eventType == CYBLE_GAPC_CONN_UNDIRECTED_ADV)
		{
			uint8 val;
			for(val = 0u; val <6u; val++)
			{
				a[val] = advreport.data[val+7u];
			}
		//sprintf(buff,"Received data is: %2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x\r\n",a[30u],a[29u],a[28u],a[27u],a[26u],a[25u],a[24u],a[23u],a[22u],a[21u],a[20u],a[19u],a[18u],a[17u],a[16u],a[15u],a[14u],a[13u],a[12u],a[11u],a[10u],a[9u],a[8u],a[7u],a[6u],a[5u],a[4u],a[3u],a[2u],a[1u],a[0u]);
		//UART_PutString(buff);
		/*   sprintf(buff, "DATA: %2.2x%2.2x%2.2x%2.2x%2.2x%2.2x \r\n", a[5u], a[4u], a[3u], a[2u], a[1u], a[0u]); 
		UART_PutString(buff);*/

		HandleScanDevices(&advreport);
		}
		else
		{
			/*  sprintf(buff, "SRP");
			UART_PutString(buff);*/
		}
		break;

		case CYBLE_EVT_GAP_DEVICE_CONNECTED:
		state = CONNECTED;
		myreg_Write(0x1);              // Purple is ON for indicating pairing
		sprintf(buff,"Connected\n\r");
		UART_PutString(buff);
		break;

		case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:

		if (state == CONNECTED && !attendanceMarked) 
		{ 		        		 //this will be when advetising is stopped
			state = DISCONNECTED;
			CyBle_GapcStartScan(CYBLE_SCANNING_FAST);
			myreg_Write(0x5);   		// BLUE is ON for indicating the SCANNING function
			sprintf(buff,"Disconnected1\n\r");
			UART_PutString(buff);
		}
		else if(state == CONNECTED && attendanceMarked)
		{
			state = DISCONNECTED;
			attendanceMarked = FALSE;
			resetval();
			sprintf(buff,"Disconnected2\n\r");
			UART_PutString(buff);
			//deep sleep
		}
		break;

		case CYBLE_EVT_TIMEOUT: 	//timeout for advertising 
		CyBle_GappStopAdvertisement();
		//even start scan should be inititated
		//or it should be sent into sleep mode and wait for another button press
		myreg_Write(0x3);
		break;

		case CYBLE_EVT_GAPC_SCAN_START_STOP:		//CyBle_GapcStopScan()
		if (state == SCANNING) 
		{
			sprintf(buff,"WAITING FOR TEACHER TO END LECTURE\r\n");
			UART_PutString(buff);
			state = DISCONNECTED;
			CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
		}
		else if(state == DISCONNECTED)
		{	//this will be a jump from startADV when teacher pressed button first time
			state = SCANNING;
			myreg_Write(0x5);
		}
		break;
	}
}

int main() '
{
	CyGlobalIntEnable;
	CyBle_IasRegisterAttrCallback(IasEventHandler);       //for alert level
	CyBle_Start(BleStackHandler);
	SWInt_StartEx(&press);
	UART_Start();
	for(;;)
	{
		CyBle_ProcessEvents();
	}

}

void IasEventHandler(uint32 event, void *eventParam) 
{

	uint8 alertLevel;
	if (event == CYBLE_EVT_IASS_WRITE_CHAR_CMD) 
	{
		CyBle_IassGetCharacteristicValue(CYBLE_IAS_ALERT_LEVEL, sizeof(alertLevel), &alertLevel);
	}
	//for now only MID_ALERT means lecture start
	//and 1_ALERT means lecture Stop
	switch (alertLevel) 
	{
		case NO_ALERT:  
		break;

		case MILD_ALERT:
		CyBle_GapDisconnect(cyBle_connHandle.bdHandle);
		break;

		case HIGH_ALERT:
		//this case will be when lecture is stopped
		//update the data on UART accordingly
		MembersPresent();
		CyBle_GapDisconnect(cyBle_connHandle.bdHandle);
		break;
	}
}


void HandleScanDevices(CYBLE_GAPC_ADV_REPORT_T* advReport)
{
	uint8 newDevice = TRUE;
	int i = 0;
	uint8 member = FALSE;

	if(addedDevices < maxDevices)
	{
		for(i = 0; i < SIZE; i++)
		{	//checking if class member or not
			if(FALSE == memcmp(permanentList[i], a, 0x06))
			{        
				member = TRUE;
				sprintf(buff,"Member is true \n\r");
				UART_PutString(buff);
				break;
			}
		} 
		if(!member) 
		{
			sprintf(buff, "Not a valid Device from the List \r\n");
			UART_PutString(buff);
		}
		if(member)
		{ 		
			// checks if already marked present         
			for(i = 0; i < addedDevices; i++) 
			{
				present_devices[i].data = &address_store[i][0];
				if(FALSE == memcmp(present_devices[i].data, a, 0x06)) 
			{
				newDevice = FALSE;
				sprintf(buff,"DEVICE NOT NEW \r\n");
				UART_PutString(buff);
				break;
			} 
			//  sprintf(buff,"RUN \r\n");
			//  UART_PutString(buff);
			}
			
			sprintf(buff,"New Device\r\n");
			UART_PutString(buff);
		}
		if ((member == TRUE) && (newDevice == TRUE))
		{
			present_devices[addedDevices].data = &address_store[addedDevices][0];

			present_devices[addedDevices].data[0] = a[0];
			present_devices[addedDevices].data[1] = a[1];
			present_devices[addedDevices].data[2] = a[2];
			present_devices[addedDevices].data[3] = a[3];
			present_devices[addedDevices].data[4] = a[4];
			present_devices[addedDevices].data[5] = a[5];

			sprintf(buff,"DEVICE ADDED \n\r");
			UART_PutString(buff);
			sprintf(buff,"Device is: %2.2x%2.2x%2.2x%2.2x%2.2x%2.2x\r\n\n\n\n",present_devices[addedDevices].data[0],present_devices[addedDevices].data[1], present_devices[addedDevices].data[2], present_devices[addedDevices].data[3], present_devices[addedDevices].data[4], present_devices[addedDevices].data[5]);
			UART_PutString(buff);
			for(i = 0; i<7;i++)
			{
				a[i] = 0;
			}
		sprintf(buff,"a after reset or something: %2.2x%2.2x%2.2x%2.2x%2.2x%2.2x\r\n",a[5u],a[4u],a[3u],a[2u],a[1u],a[0u]);
		UART_PutString(buff);
		addedDevices++;
		}
	}
}
void MembersPresent()
{
	int n = 0;
	sprintf(buff,"Devices present for the Lecture are: ");
	UART_PutString(buff);
	RPI_Write(1u); 
	CyDelay(100);
	for (n = 0; n < addedDevices; n++)
	{
		// All addresses are sent over UART to raspberry PI followed by a comma
		sprintf(buff,"%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x,\r\n",present_devices[n].data[5u],present_devices[n].data[4u],present_devices[n].data[3u],present_devices[n].data[2u],present_devices[n].data[1u],present_devices[n].data[0u]);
		UART_PutString(buff);
	}
	RPI_Write(0u);
	RPI2_Write(1u);
	RPI2_Write(0u);
	attendanceMarked = TRUE;
}

void resetval()
{
	sprintf(buff,"RESETTING VALS\n\r");
	UART_PutString(buff);
	int i, j;
	for(i = 0; i < addedDevices ; i++)
	{  
		for(j = 0; j < 7 ; j++)
		{
			present_devices[i].data[j] = 0;
		}
	}	
	addedDevices = 0;
}



