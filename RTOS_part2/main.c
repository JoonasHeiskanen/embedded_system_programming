// Tavoittelen kolmea pistettä tehtävästä
// Suoritin Sekvenssin vastaanoton sarjaportista +1p
// Lisäksi refaktoroin ohjelmaa, jossa poistettiin superloopit ja kätin THREAD APIA +1p
// Kolmantena lisäsin sekvenssiin toisto ominaisuuden joka toistaa uarttiin antamaani sekvenssikäskyä uudelleen +1p

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>

/****************************
 * Remember to add line:
 * CONFIG_HEAP_MEM_POOL_SIZE=1024
 * to prj.conf
 ****************************/

// Thread initializations
#define STACKSIZE 500
#define PRIORITY 5

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void dispatcher_task(void *, void *, void*);
void uart_task(void *, void *, void*);

K_THREAD_STACK_DEFINE(red_stack, STACKSIZE);
K_THREAD_STACK_DEFINE(yellow_stack, STACKSIZE);
K_THREAD_STACK_DEFINE(green_stack, STACKSIZE);

static struct k_thread red_thread_data;
static struct k_thread yellow_thread_data;
static struct k_thread green_thread_data;

K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);


// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// Create dispatcher FIFO buffer
K_FIFO_DEFINE(dispatcher_fifo);

// FIFO dispatcher data type
struct data_t {
	/*************************
	// Add fifo_reserved below
	*************************/
	void *fifo_reserved;
	char msg[20];
};

/********************
 * init UART
 */
int init_uart(void) {
	// UART initialization
	if (!device_is_ready(uart_dev)) {
		return 1;
	} 
	return 0;
}

// Initialize leds
int  init_led() {

	// Led pin initialization
	int ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}
	// set led off
	gpio_pin_set_dt(&red,0);

	printk("Led red initialized ok\n");

	// Led pin initialization
	ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		printk("Error: Led configure failed\n");		
		return ret;
	}
	// set led off
	gpio_pin_set_dt(&green,0);

	printk("Led green initialized ok\n");

	return 0;
}

/********************
 * Main task
 */
int main(void)
{
	int ret = init_uart();
	if (ret != 0) {
		printk("UART initialization failed!\n");
		return ret;
	}

	init_led();

	return 0;
}

/********************
 * UART task
 */
void uart_task(void *unused1, void *unused2, void *unused3)
{
	// Received character from UART
	char rc=0;
	// Message from UART
	char uart_msg[20];
	memset(uart_msg,0,20);
	int uart_msg_cnt = 0;

	while (true) {
		// Ask UART if data available
		if (uart_poll_in(uart_dev,&rc) == 0) {
			// printk("Received: %c\n",rc);
			// If character is not newline, add to UART message buffer
			if (rc != '\r') {
				uart_msg[uart_msg_cnt] = rc;
				uart_msg_cnt++;
			// Character is newline, copy dispatcher data and put to FIFO buffer
			} else {
				printk("UART msg: %s\n", uart_msg);
                
				struct data_t *buf = k_malloc(sizeof(struct data_t));
				if (buf == NULL) {
					return;
				}
				// Copy UART message to dispatcher data
				// strncpy(buf->msg, 20, uart_msg); // mitä ihmettä, miksi kaatuu!!
				snprintf(buf->msg, 20, "%s", uart_msg);

				// You need to:
				// Put dispatcher data to FIFO buffer
				k_fifo_put(&dispatcher_fifo, buf);

				// Clear UART receive buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);

				// Clear UART message buffer
				uart_msg_cnt = 0;
				memset(uart_msg,0,20);
			}
		}
		k_msleep(10);
	}
	return 0;
}

/********************
 * Dispatcher task
 */
void dispatcher_task(void *unused1, void *unused2, void *unused3)
{
	while (true) {
		// Receive dispatcher data from uart_task fifo
		struct data_t *rec_item = k_fifo_get(&dispatcher_fifo, K_FOREVER);
		char sequence[20];
		memcpy(sequence,rec_item->msg,20);
		k_free(rec_item);

		printk("Dispatcher: %s\n", sequence);
		int cnt = 0;
		// tulostetaan merkki kerrallaan
		while (sequence[cnt] != 0) {
			// Mahdollistetaan sekvenssin pysäytys
			struct data_t *stop_cmd = k_fifo_get(&dispatcher_fifo, K_NO_WAIT);
			if (stop_cmd != NULL) {
    			if (stop_cmd->msg[0] == 'S') {
        			k_free(stop_cmd);
        			break;
    			}
    			k_free(stop_cmd);
}
			k_tid_t running_thread = NULL;
			if (sequence[cnt] == 'R'){
				printk("RED\n");
				running_thread = k_thread_create(&red_thread_data, red_stack,
												K_THREAD_STACK_SIZEOF(red_stack),
												red_led_task, NULL, NULL, NULL,
												PRIORITY, 0, K_NO_WAIT);
			}
			if (sequence[cnt] == 'Y'){
				printk("YELLOW\n");
				running_thread = k_thread_create(&yellow_thread_data, yellow_stack,
												K_THREAD_STACK_SIZEOF(yellow_stack),
												yellow_led_task, NULL, NULL, NULL,
												PRIORITY, 0, K_NO_WAIT);
			}
			if (sequence[cnt] == 'G'){
				printk("GREEN\n");
				running_thread = k_thread_create(&green_thread_data, green_stack,
												K_THREAD_STACK_SIZEOF(green_stack),
												green_led_task, NULL, NULL, NULL,
												PRIORITY, 0, K_NO_WAIT);
			}
			if (sequence[cnt] == 'T'){
					printk("Ollaan toisto sekvenssissa\n");
					cnt = -1;
				}
			
			cnt++;

			// Odotetaan Thread API:lla että käynnistetty säie suorittaa työnsä loppuun
			if(running_thread != NULL){
				k_thread_join(running_thread, K_FOREVER);
			}
		}
	}
}

// Task to handle red led
void red_led_task(void *, void *, void*) {
	
	printk("Red led thread started\n");
	
	// 1. set led on 
	gpio_pin_set_dt(&red,1);
	printk("Red on\n");

	k_sleep(K_SECONDS(1));
		
	gpio_pin_set_dt(&red,0);
	printk("Red off\n");
}

// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {
	
	printk("Yellow led thread started\n");
	
	// 1. set led on 
	gpio_pin_set_dt(&red,1);
	gpio_pin_set_dt(&green,1);
	printk("Yellow on\n");
		
	k_sleep(K_SECONDS(1));
	
	gpio_pin_set_dt(&red,0);
	gpio_pin_set_dt(&green,0);
	printk("Yellow off\n");
	
}

// Task to handle green led
void green_led_task(void *, void *, void*) {
	
	printk("Green led thread started\n");
	
	// 1. set led on 
	gpio_pin_set_dt(&green,1);
	printk("Green on\n");
			
	k_sleep(K_SECONDS(1));
			
	gpio_pin_set_dt(&green,0);
	printk("Green off\n");
}