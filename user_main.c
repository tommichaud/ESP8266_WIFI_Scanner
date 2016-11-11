#include "osapi.h"
#include "user_interface.h"
#include "pwm.h"
#include "uart.h"
#include "mem.h"
#include "espmissingincludes.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];
static void loop(os_event_t *events);

uint32 scan_start_time;
uint32 start_connection;

#define MAX_NUM_AP_INFO 6
struct bss_info ap_info[MAX_NUM_AP_INFO];

struct station_config stationConfig;

//
// Stores the AP information to a static array and sorts
// by strongest RSSI.  Only saves the MAX_NUM_AP_INFO
// strongest AP
//
void store_and_sort_ap_info(struct bss_info * bss_link)
{
	int i,j;
	int flag=0;

	//os_printf("sorting %s rssi %d \n", bss_link->ssid,bss_link->rssi);

	for(i=0;i<MAX_NUM_AP_INFO && flag == 0;i++)
	{
		if(ap_info[i].rssi == 0 )
		{
			memcpy( &ap_info[i], bss_link, sizeof( struct bss_info));
			//os_printf("copying %s rssi %d into %d \n", ap_info[i].ssid,ap_info[i].rssi, i);
			flag=1;
		}
		else if (bss_link->rssi > ap_info[i].rssi)
		{
			// move the remaining down and insert here
			for(j=MAX_NUM_AP_INFO-1;j>i;j--)
			{
				//os_printf("moving %s rssi %d into %d down\n", ap_info[j-1].ssid,ap_info[j-1].rssi, j);
				memcpy( &ap_info[j], &ap_info[j-1], sizeof( struct bss_info));
			}
			memcpy( &ap_info[i], bss_link, sizeof( struct bss_info));
			flag=1;
		}
	}

}

void scan_complete(void *arg, STATUS status)
{
	int i;
	uint32 scan_end_time;
	scan_end_time = system_get_time();

	// clear the AP info structure
	memset(&ap_info,0, sizeof(ap_info));

	uint8 ssid[33];
	char temp[128];
	if (status == OK){
		struct bss_info *bss_link = (struct bss_info *)arg;
		while (bss_link != NULL)
		{
			store_and_sort_ap_info(bss_link);
			bss_link = bss_link->next.stqe_next;
		}

		for(i=0;i<MAX_NUM_AP_INFO;i++)
		{
			if(ap_info[i].rssi != 0 )
			{
				ap_info[i].ssid[32]=0;// make sure a null is in the last spot

				os_printf("%d,"MACSTR",%s,%d,%d\n",
						i+1,
						MAC2STR(ap_info[i].bssid),
						ap_info[i].ssid,
						ap_info[i].rssi,
						ap_info[i].channel);
			}
		}
		os_printf("Scan duration... %d us\n\n", (scan_end_time-scan_start_time));

		start_connection= system_get_time();
		wifi_station_connect();
	}
	else{
		os_printf("scan fail !!!\r\n");
	}


	//
	// Go into deep sleep for 10 seconds
	//
	// see http://www.esp8266.com/wiki/doku.php?id=esp8266_power_usage
	//
	// ESP.deepSleep(10000000,0);
	//
	/*
	os_printf("scan_done...sleeping \n");
	system_deep_sleep_set_option(0);
	system_deep_sleep(10000000,0);
	os_printf("waking up... \n");
	*/
}


void wifi_handle_event_cb(System_Event_t *evt)
{
	uint32 duration;

	switch (evt->event)
	{
		case EVENT_STAMODE_GOT_IP:
			duration= system_get_time() - start_connection;
			os_printf(" got ip address, duration %d\n", duration);
			wifi_station_disconnect();
			break;

		case EVENT_STAMODE_CONNECTED:
			duration= system_get_time() - start_connection;
			os_printf(" connected, duration %d\n", duration);
			break;

		case EVENT_STAMODE_DISCONNECTED:
			duration= system_get_time() - start_connection;
			//os_printf(" disconnected, duration %d\n", duration);
			break;

		case EVENT_SOFTAPMODE_STACONNECTED:
			duration= system_get_time() - start_connection;
			os_printf(" sta connected, duration %d\n", duration);
			break;

		case EVENT_SOFTAPMODE_STADISCONNECTED:
			duration= system_get_time() - start_connection;
			os_printf(" sta disconnected, duration %d\n", duration);
			break;

		case EVENT_SOFTAPMODE_PROBEREQRECVED:
			duration= system_get_time() - start_connection;
			//os_printf(" probe, duration %d\n", duration);
			break;

		default:
			os_printf(" event %x \n",
					evt->event);
			break;
	}
}


static os_timer_t some_timer;

void some_timerfunc(void *arg)
{
	os_printf("Timer: entering AP scan..... \n");
	scan_start_time = system_get_time();
	wifi_station_scan(NULL,scan_complete);
}

#define ONE_SEC    1000
#define TEN_SEC   10000
#define SIXTY_SEC 60000

void systemInitDoneCB()
{

	os_printf("SDK version: %s\n", system_get_sdk_version());

	wifi_set_opmode(STATIONAP_MODE);
	os_memset(&stationConfig, 0, sizeof (struct station_config));
	os_sprintf(stationConfig.ssid, "MichaudNetwork");
	os_sprintf(stationConfig.password, "M1chaudNetwork");
	wifi_station_set_config_current(&stationConfig);
	wifi_set_event_handler_cb(wifi_handle_event_cb);
	start_connection= system_get_time();
	wifi_station_connect();

	//
	// setup timer
	//
	os_timer_setfn(&some_timer, (os_timer_func_t *)some_timerfunc, NULL);
	os_timer_arm(&some_timer, TEN_SEC*3, 1);
}

void user_init()
{

	uart_init(115200, 115200);
	os_printf("Hello in user_init \n");

	system_init_done_cb(systemInitDoneCB);
	os_printf("Hello leaving user_init \n");
}
