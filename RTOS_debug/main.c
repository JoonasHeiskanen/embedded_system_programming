//Tavoittelen kolmea pistettä tämän viikon tehtävistä
//Lisäsin debug taskin joka hoitaa kaikki printk tulostukset +1p
//Lisäsin debug on/off toiminnallisuuden, jolla saa tulostukset päälle ja pois

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/timing/timing.h>

/****************************
 * Remember to add line:
 * CONFIG_HEAP_MEM_POOL_SIZE=1024
 * to prj.conf
 ****************************/

// Thread initializations
#define STACKSIZE 500
#define PRIORITY 0
#define PRIORITY_LOW 10

int led_state = 0; // 0 = idle, 1 = red, 2 = yellow, 3 = green, 4 = red on, 5 = yellow on, 6 = green on
int old_state = 0;
int yellow_blink_state = 0;
uint64_t total_time = 0;
bool debug_enabled = true;

// Configure buttons
#define BUTTON_0 DT_ALIAS(sw0)
// #define BUTTON_1 DT_ALIAS(sw1)
static const struct gpio_dt_spec button_0 = GPIO_DT_SPEC_GET_OR(BUTTON_0, gpios, {0});
static struct gpio_callback button_0_data;

#define BUTTON_1 DT_ALIAS(sw1)
static const struct gpio_dt_spec button_1 = GPIO_DT_SPEC_GET_OR(BUTTON_1, gpios, {0});
static struct gpio_callback button_1_data;

#define BUTTON_2 DT_ALIAS(sw2)
static const struct gpio_dt_spec button_2 = GPIO_DT_SPEC_GET_OR(BUTTON_2, gpios, {0});
static struct gpio_callback button_2_data;

#define BUTTON_3 DT_ALIAS(sw3)
static const struct gpio_dt_spec button_3 = GPIO_DT_SPEC_GET_OR(BUTTON_3, gpios, {0});
static struct gpio_callback button_3_data;

#define BUTTON_4 DT_ALIAS(sw4)
static const struct gpio_dt_spec button_4 = GPIO_DT_SPEC_GET_OR(BUTTON_4, gpios, {0});
static struct gpio_callback button_4_data;

void red_led_task(void *, void *, void*);
void yellow_led_task(void *, void *, void*);
void green_led_task(void *, void *, void*);
void dispatcher_task(void *, void *, void*);
void uart_task(void *, void *, void*);
void debug_task(void *, void *, void*);
void send_to_debug(const char *, const char *, uint64_t);

K_THREAD_STACK_DEFINE(red_stack, STACKSIZE);
K_THREAD_STACK_DEFINE(yellow_stack, STACKSIZE);
K_THREAD_STACK_DEFINE(green_stack, STACKSIZE);

static struct k_thread red_thread_data;
static struct k_thread yellow_thread_data;
static struct k_thread green_thread_data;

K_THREAD_DEFINE(dis_thread,STACKSIZE,dispatcher_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(uart_thread,STACKSIZE,uart_task,NULL,NULL,NULL,PRIORITY,0,0);
K_THREAD_DEFINE(debug_thread,STACKSIZE,debug_task,NULL,NULL,NULL,PRIORITY_LOW,0,0);

// Led pin configurations
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);


// UART initialization
#define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

// Create dispatcher FIFO buffer
K_FIFO_DEFINE(dispatcher_fifo);

// Create FIFO buffer
K_FIFO_DEFINE(data_fifo);

// FIFO dispatcher data type
struct data_t {
	/*************************
	// Add fifo_reserved below
	*************************/
	void *fifo_reserved;
	char msg[32];
	uint64_t time;
	char task_name[20];
};

// Button interrupt pausehandler
void button_0_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	//printk("Button pressed\n");
	send_to_debug(NULL, "Button pressed", 0);
	
	if (led_state == 0){
		// jos tila on jo pause = 0, palautetaan tallennettu tila
		led_state = old_state;
	} 	else {
			// Otetaan nykyinen tila talteen ja siirrytään tilaan 0
			old_state = led_state;
			led_state = 0;
			gpio_pin_set_dt(&red,0);
			gpio_pin_set_dt(&green,0);
			printk("paused\n");
	}
}
// Button interrupt red led handler
void button_1_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	//printk("Button 2 pressed\n");
	send_to_debug(NULL, "Button 2 pressed", 0);
	total_time = 0;
	k_tid_t running_thread = NULL;
	running_thread = k_thread_create(&red_thread_data, red_stack,
										K_THREAD_STACK_SIZEOF(red_stack),
										red_led_task, NULL, NULL, NULL,
										PRIORITY, 0, K_NO_WAIT);
	/*if (led_state == 0){
		gpio_pin_set_dt(&red,1);
		printk("Punainen led sytytetty\n");
		led_state = 4; // Red led on
	}	else {
		gpio_pin_set_dt(&red,0);
		printk("Punainen led sammutettu\n");
		led_state = 0;
	}*/
}
// Button interrupt yellow led handler
void button_2_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	//printk("Button 3 pressed\n");
	send_to_debug(NULL, "Button 3 pressed", 0);
	total_time = 0;
	k_tid_t running_thread = NULL;
	running_thread = k_thread_create(&yellow_thread_data, yellow_stack,
												K_THREAD_STACK_SIZEOF(yellow_stack),
												yellow_led_task, NULL, NULL, NULL,
												PRIORITY, 0, K_NO_WAIT);
	
	/*if (led_state == 0){
		gpio_pin_set_dt(&red,1);
		gpio_pin_set_dt(&green,1);
		printk("Keltainen led sytytetty\n");
		led_state = 5; // Yellow led on
	}	else {
		gpio_pin_set_dt(&red,0);
		gpio_pin_set_dt(&green,0);
		printk("Keltainen led sammutettu\n");
		led_state = 0;
	}*/
}
// Button interrupt green led handler
void button_3_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{

	//printk("Button 4 pressed\n");
	send_to_debug(NULL, "Button 4 pressed", 0);
	total_time = 0;
	k_tid_t running_thread = NULL;
	running_thread = k_thread_create(&green_thread_data, green_stack,
												K_THREAD_STACK_SIZEOF(green_stack),
												green_led_task, NULL, NULL, NULL,
												PRIORITY, 0, K_NO_WAIT);
	
	/*if (led_state == 0){
		gpio_pin_set_dt(&green,1);
		printk("Vihrea led sytytetty\n");
		led_state = 6; // Green led on
	}	else {
		gpio_pin_set_dt(&green,0);
		printk("Vihrea led sammutettu\n");
		led_state = 0;
	}*/
}

// Button interrupt yellow led blink handler
void button_4_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	//printk("Button 5 pressed\n");
	send_to_debug(NULL, "Button 5 pressed", 0);
	if (led_state == 0){
		yellow_blink_state = 1;
		led_state = 2;
		printk("Yellow blink tila paalla\n");
	}	else {
		yellow_blink_state = 0;
		gpio_pin_set_dt(&red,0);
		gpio_pin_set_dt(&green,0);
		printk("Yellow blink tila sammutettu\n");
		led_state = 0;
	}
}

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

// Button initialization
int init_button() {

	int ret;
	if (!gpio_is_ready_dt(&button_0)) {
		printk("Error: button 0 is not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&button_0, GPIO_INPUT);
	if (ret != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button_0, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_0_data, button_0_handler, BIT(button_0.pin));
	gpio_add_callback(button_0.port, &button_0_data);
	printk("Set up button 0 ok\n");

	
	if (!gpio_is_ready_dt(&button_1)) {
		printk("Error: button 1 is not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&button_1, GPIO_INPUT);
	if (ret != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button_1, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_1_data, button_1_handler, BIT(button_1.pin));
	gpio_add_callback(button_1.port, &button_1_data);
	printk("Set up button 1 ok\n");

	if (!gpio_is_ready_dt(&button_2)) {
		printk("Error: button 2 is not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&button_2, GPIO_INPUT);
	if (ret != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button_2, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_2_data, button_2_handler, BIT(button_2.pin));
	gpio_add_callback(button_2.port, &button_2_data);
	printk("Set up button 2 ok\n");

	if (!gpio_is_ready_dt(&button_3)) {
		printk("Error: button 3 is not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&button_3, GPIO_INPUT);
	if (ret != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button_3, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_3_data, button_3_handler, BIT(button_3.pin));
	gpio_add_callback(button_3.port, &button_3_data);
	printk("Set up button 3 ok\n");

	if (!gpio_is_ready_dt(&button_4)) {
		printk("Error: button 4 is not ready\n");
		return -1;
	}

	ret = gpio_pin_configure_dt(&button_4, GPIO_INPUT);
	if (ret != 0) {
		printk("Error: failed to configure pin\n");
		return -1;
	}

	ret = gpio_pin_interrupt_configure_dt(&button_4, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		printk("Error: failed to configure interrupt on pin\n");
		return -1;
	}

	gpio_init_callback(&button_4_data, button_4_handler, BIT(button_4.pin));
	gpio_add_callback(button_4.port, &button_4_data);
	printk("Set up button 4 ok\n");
	
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

	ret = init_button();
	if (ret < 0) {
		return 0;
	}

	timing_init();
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
				char uart_buf [32];
				snprintf(uart_buf, sizeof(uart_buf), "Uart: %s", uart_msg);
				send_to_debug(NULL, uart_buf, 0);
                
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
	return;
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
		total_time = 0; // Sets total time cycle to zero

		// Sends dispatcher sequence info to debug_task
		char disp_msg[32];
		snprintf(disp_msg, sizeof(disp_msg), "Dispatcher: %s", sequence);
		send_to_debug(NULL, disp_msg, 0);
		int cnt = 0;

		// tulostetaan merkki kerrallaan
		while (sequence[cnt] != 0) {
			// Mahdollistetaan sekvenssin pysäytys
			struct data_t *stop_cmd = k_fifo_get(&dispatcher_fifo, K_NO_WAIT);
			if (stop_cmd != NULL) {
    			if (stop_cmd->msg[0] == 'S') {
        			k_free(stop_cmd);
					send_to_debug(NULL, "Sekvenssi pysaytetty", 0);
        			break;
    			}
    			k_free(stop_cmd);
			}
			k_tid_t running_thread = NULL;
			if (sequence[cnt] == 'R'){
				send_to_debug(NULL, "RED sequence", 0);
				running_thread = k_thread_create(&red_thread_data, red_stack,
												K_THREAD_STACK_SIZEOF(red_stack),
												red_led_task, NULL, NULL, NULL,
												PRIORITY, 0, K_NO_WAIT);
			}
			if (sequence[cnt] == 'Y'){
				send_to_debug(NULL, "YELLOW sequence", 0);
				running_thread = k_thread_create(&yellow_thread_data, yellow_stack,
												K_THREAD_STACK_SIZEOF(yellow_stack),
												yellow_led_task, NULL, NULL, NULL,
												PRIORITY, 0, K_NO_WAIT);
			}
			if (sequence[cnt] == 'G'){
				send_to_debug(NULL, "GREEN sequence", 0);
				running_thread = k_thread_create(&green_thread_data, green_stack,
												K_THREAD_STACK_SIZEOF(green_stack),
												green_led_task, NULL, NULL, NULL,
												PRIORITY, 0, K_NO_WAIT);
			}
			if (sequence[cnt] == 'T'){
				send_to_debug(NULL, "Ollaan toisto sekvenssissa", 0);
				cnt = -1;
				}
			if (sequence[cnt] == 'D'){
				if (debug_enabled == true){
					send_to_debug("SYS","Debug disabled", 0);
					debug_enabled = false;
				}
				else{
					debug_enabled = true;
					send_to_debug("SYS","Debug enabled", 0);
				}
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
	timing_start();
	timing_t red_start_time = timing_counter_get();
	
	// 1. set led on 
	gpio_pin_set_dt(&red,1);
	send_to_debug(NULL, "Red on", 0);

	k_sleep(K_SECONDS(1));
		
	gpio_pin_set_dt(&red,0);
	send_to_debug(NULL, "Red off", 0);

	timing_t red_end_time = timing_counter_get();
	timing_stop();
    uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&red_start_time, &red_end_time));
	uint64_t timing_us = timing_ns / 1000;
	send_to_debug("Red task", NULL, timing_us);
}

// Task to handle yellow led
void yellow_led_task(void *, void *, void*) {
	
	timing_start();
	timing_t yellow_start_time = timing_counter_get();
	
	// 1. set led on 
	gpio_pin_set_dt(&red,1);
	gpio_pin_set_dt(&green,1);
	
	send_to_debug(NULL, "Yellow on", 0);
		
	k_sleep(K_SECONDS(1));
	
	gpio_pin_set_dt(&red,0);
	gpio_pin_set_dt(&green,0);
	
	send_to_debug(NULL, "Yellow off", 0);

	timing_t yellow_end_time = timing_counter_get();
	timing_stop();
    uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&yellow_start_time, &yellow_end_time));
	uint64_t timing_us = timing_ns / 1000;
	send_to_debug("Yellow task", NULL, timing_us);
}

// Task to handle green led
void green_led_task(void *, void *, void*) {
	
	timing_start();
	timing_t green_start_time = timing_counter_get();

	// 1. set led on 
	gpio_pin_set_dt(&green,1);
	
	send_to_debug(NULL, "Green on", 0);
			
	k_sleep(K_SECONDS(1));
			
	gpio_pin_set_dt(&green,0);
	
	send_to_debug(NULL, "Green off", 0);

	timing_t green_end_time = timing_counter_get();
	timing_stop();
    uint64_t timing_ns = timing_cycles_to_ns(timing_cycles_get(&green_start_time, &green_end_time));
	uint64_t timing_us = timing_ns / 1000;

	send_to_debug("Green task", NULL, timing_us);
}

void debug_task(void *, void *, void*) {

	// Store received data
	struct data_t *received;

	while (true) {
        received = k_fifo_get(&data_fifo, K_FOREVER);

		// Jos viestissä on tunniste "SYS", se tulostetaan AINA
        if (strcmp(received->task_name, "SYS") == 0) {
            printk("%s\n", received->msg);
		}
		else if (debug_enabled){
			if (received->time == 0) {
				// Tilaviesti
				printk("%s\n", received->msg);
			} else {
				// Ajanmittaus
				total_time += received->time;
				printk("%s time: %llu us | Total time: %llu us\n", 
					received->task_name, received->time, total_time);
			}
		}
		k_free(received);
    }
}

void send_to_debug(const char *task, const char *msg, uint64_t time) {
    struct data_t *buf = k_malloc(sizeof(struct data_t));
    if (buf) {
        memset(buf->task_name, 0, sizeof(buf->task_name));
        memset(buf->msg, 0, sizeof(buf->msg));

        if (task) {
            snprintf(buf->task_name, sizeof(buf->task_name), "%s", task);
        }
        if (msg) {
            snprintf(buf->msg, sizeof(buf->msg), "%s", msg);
        }

        buf->time = time;
        k_fifo_put(&data_fifo, buf);
    }
}