#define USB_CFG_DEVICE_NAME     'D','i','g','i','B','l','i','n','k'
#define USB_CFG_DEVICE_NAME_LEN 9
#include <DigiUSB.h>

#define PIN_LED 0
#define PIN_RST 1


uint32_t now = 0;
uint32_t time_last_tick = 0;
uint32_t tick_count = 0;
uint32_t tick_count_max = 10;
uint32_t tick_rate = 10;
uint8_t led_flash_rate = 0;
uint8_t usb_data_in;

void setup() {
  DigiUSB.begin();
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_RST, OUTPUT);
}

void do_led(uint8_t led_flash_rate)
{
  static uint32_t last_change_time = 0;
  uint32_t now = millis();
  static uint8_t led_state = 0;

  if (now - last_change_time > (led_flash_rate * 100))
  {
    last_change_time = now;
    led_state = 1 - led_state;
    digitalWrite(PIN_LED, led_state);
  }

}

void loop() {
  usb_data_in = 0;
  now = millis();
  do_led(led_flash_rate);
  DigiUSB.refresh();
  if (DigiUSB.available() > 0)
  {
    usb_data_in = DigiUSB.read();
    DigiUSB.print("Got: ");
    DigiUSB.println(usb_data_in, HEX);
  }

  if (now - time_last_tick >= (tick_rate * 100))
  {
    DigiUSB.print("TICK: ");
    DigiUSB.print(tick_count, DEC);
    DigiUSB.print(" Max=");
    DigiUSB.print(tick_count_max, DEC);


    time_last_tick = now;
    tick_count++;
    if (tick_count >= tick_count_max)
    {
      led_flash_rate = 1;
    }
  }

}
