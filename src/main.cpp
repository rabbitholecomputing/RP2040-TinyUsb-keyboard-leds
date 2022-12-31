
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/structs/sio.h"
#include "hardware/gpio.h"
#include "pico/stdio_uart.h"
#include "tusb.h"


// Status LED GPIOs
#define LED_GPIO     12
#define LED_ON()    sio_hw->gpio_set = 1 << LED_GPIO
#define LED_OFF()   sio_hw->gpio_clr = 1 << LED_GPIO
#define LED_SET(x)  (x ? sio_hw->gpio_set = 1 << LED_GPIO : sio_hw->gpio_clr = 1 << LED_GPIO)

// UART out messaging
#define UART_TX_GPIO    16
#define UART_TX_BAUD    115200
#define UART_PORT       uart0

inline void led_on(void) {
    LED_ON();
}

inline void led_off(void) {
    LED_OFF();
}


void led_gpio_init(void) {
    gpio_init(LED_GPIO);
    gpio_set_function(LED_GPIO, GPIO_FUNC_SIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    LED_OFF();
}

void uart_gpio_init(void) {

    uart_init(UART_PORT, UART_TX_BAUD);
    gpio_set_function(UART_TX_GPIO, GPIO_FUNC_UART);
}

// Blink the led n times
void led_blink(uint8_t times) {
    for (uint8_t i = 0; i < times; i++) {
        LED_ON();
        sleep_ms(75);
        LED_OFF();
        sleep_ms(75);
    }
    sleep_ms(75);    
}


uint8_t keyboard_addr = 0;
uint8_t keyboard_instance = 0;
bool keyboard_mounted = false;
bool auto_toggle_start = false;
static uint32_t successful_led_toggles = 0;
static uint8_t leds = 7;
static bool is_on = false;
static bool set_report_failed = false;
   

void toggle_kbd_leds(void)
{
         if (is_on) {
            leds = 0;
            is_on = false;
        }
        else {
            leds = 7;
            is_on = true;
        }
        if ( tuh_hid_set_report(keyboard_addr, keyboard_instance, 0, HID_REPORT_TYPE_OUTPUT, &leds, sizeof(leds)))
        { 
          successful_led_toggles++;
        }
        else if(!set_report_failed)
        {
          set_report_failed = true;
        }
}

int main(void) {
    set_sys_clock_khz(125000, true);
    
    //uart_gpio_init();
    led_gpio_init();
    sleep_ms(10);
    tuh_init(0);
    stdio_init_all();

    printf("Testing tuh_hid_set_report\n");
    
      
  while (true) {
    tuh_task(); // tinyusb host task
    if (keyboard_mounted) 
    {

      if (auto_toggle_start)
      {
        auto_toggle_start = false;
        toggle_kbd_leds();
      }
    }
  }
  return 0;
}



void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* desc_report, uint16_t desc_len)
{
  
  (void)desc_report;
  (void)desc_len;

  const char* protocol_str[] = { "None", "Keyboard", "Mouse" };
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  if (itf_protocol == HID_ITF_PROTOCOL_KEYBOARD )
  {
    if ( !tuh_hid_receive_report(dev_addr, instance) )
    {
      // Error: cannot request report
    }
    else
    {
        led_blink(2);
        keyboard_addr = dev_addr;
        keyboard_instance = instance;
        keyboard_mounted = true;
    }
  }
}

// Invoked when device with hid interface is un-mounted
void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  led_blink(1);
  keyboard_mounted = false;
}


static void process_kbd_report(uint8_t dev_addr, uint8_t instance, hid_keyboard_report_t const *report)
{
    for(int i = 0; i < 6; i++)
    {
      if (report->keycode[i] == HID_KEY_CAPS_LOCK) {
        toggle_kbd_leds();
        auto_toggle_start = false;
      }
    }
}

// Invoked when received report from device via interrupt endpoint
void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const* report, uint16_t len)
{
  (void) len;
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);

  switch(itf_protocol)
  {
    case HID_ITF_PROTOCOL_KEYBOARD:
      process_kbd_report(dev_addr, instance, (hid_keyboard_report_t const*) report );      
    break;
    default: 
    break;
  }

  // continue to request to receive report
  if ( !tuh_hid_receive_report(dev_addr, instance) )
  {
    //    Cannot request report
  }
}

void tuh_hid_set_report_complete_cb(uint8_t dev_addr, uint8_t instance, uint8_t report_id, uint8_t report_type, uint16_t len)
{
  printf("TEST: Set report completed, Toggled leds %lu times so far.\n", successful_led_toggles);

  auto_toggle_start = true;
}
