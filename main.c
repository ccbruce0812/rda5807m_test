#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <esp8266.h>
#include <esp/uart.h>

#include <etstimer.h>
#include <espressif/esp_common.h>

#include <i2c/i2c.h>
#include <rda5807m/rda5807m.h>

#include <string.h>
#include <stdarg.h>

#define MSG_MIN				(0)
#define MSG_KEY_PRESSED		(MSG_MIN+1)

#define KEY_PIN				(5)
#define SCL_PIN				(12)
#define SDA_PIN				(13)

typedef struct {
	unsigned int id;
} Msg;

#define MSEC2TICKS(n) (n/portTICK_PERIOD_MS)

static inline int __dbg__(const char *loc, const char *fmt, ...) {
	va_list args;
	char _fmt[256]="";
	int res;
	
	va_start(args, fmt);
	sprintf(_fmt, "[%s] %s", loc, fmt);
	res=vprintf(_fmt, args);
	va_end(args);
	
	return res;
}

#define DBG(...) __dbg__(__func__, __VA_ARGS__)

QueueHandle_t g_msgQ=NULL;

static void msgTask(void *param) {
	Msg msgRecv={0};
	
	while(1) {
		xQueueReceive(g_msgQ, &msgRecv, portMAX_DELAY);
		
		switch(msgRecv.id) {
			case MSG_KEY_PRESSED: {

			}
				
			default:
				;
		}
	}
}

static void onGPIO(unsigned char num) {
	static unsigned int prev=0;
	unsigned int now=0;
    
	if(num==KEY_PIN) {
		now=xTaskGetTickCount();
		if(now-prev>=MSEC2TICKS(50)) {
			Msg msg={
				.id=MSG_KEY_PRESSED
			};
			
			prev=now;
			xQueueSendFromISR(g_msgQ, &msg, 0);
		}
	}
}

void user_init(void) {
	RDA5807M_SETTING rxSetting={
		.clkSetting={
			.isClkNoCalb=RDA5807M_FALSE,
			.isClkDirInp=RDA5807M_FALSE,
			.freq=RDA5807M_CLK_FREQ_32_768KHZ,
		},
		.useRDS=RDA5807M_TRUE,
		.useNewDMMethod=RDA5807M_FALSE,
		.isDECNST50us=RDA5807M_FALSE,
		.system={
			.band=RDA5807M_BAND_87_108_MHZ,
			.is6575Sys=RDA5807M_FALSE,
			.space=RDA5807M_SPACE_100_KHZ
		}
	};
	
    uart_set_baud(0, 115200);
    DBG("SDK version: %s\n", sdk_system_get_sdk_version());

	gpio_enable(KEY_PIN, GPIO_INPUT);
	gpio_set_pullup(KEY_PIN, true, false);
	GPIO.STATUS_CLEAR=0x0000ffff;
	gpio_set_interrupt(KEY_PIN, GPIO_INTTYPE_EDGE_NEG, onGPIO);
	
	i2c_init(SCL_PIN, SDA_PIN);
	RDA5807M_init(&rxSetting);
	RDA5807M_setFreq(96000);
	RDA5807M_enableOutput(RDA5807M_TRUE);
	RDA5807M_setVolume(14);
	RDA5807M_unmute(RDA5807M_TRUE);
	
	g_msgQ=xQueueCreate(8, sizeof(Msg));
	xTaskCreate(msgTask, "msgTask", 512, NULL, 4, NULL);
}
