//#indent -kr -bl -bli0 -nut
//#vim: ai ts=4 sts=4 et sw=4 ft=python
#define USB_CFG_DEVICE_NAME     'W','a','t','c','h','_','D','o','g'
#define USB_CFG_DEVICE_NAME_LEN 9
#include <DigiUSB.h>

#define PIN_RST        0        // This pin is used to drive the PI reset line
#define PIN_LED_TICK   1        // This pin has an LED to indicate 'ticks'
#define PIN_LED_ARMED  2        // This pin has an LED to indicate armed state
#define PIN_USB_P      3        // This pin is reserved for USB
#define PIN_USB_N      4        // This pin is reserved for USB
#define PIN_RESET_RSVD 5        // This pin is reserved for RESET

// Longest command "t=255" == 5 chars
#define MAX_CMD_LEN   5

// A 'buffer' we can use to store incoming characters
// as they are read in from the USB interface until
// we are ready to parse them as potential commands
char cmd_buffer[MAX_CMD_LEN + 1];
char cmdbuf_idx = 0;

// Flag to indicate if timeouts should result in
// us actually resetting the PI
uint8_t is_armed = 0;

// How many 'ticks' before we should timeout
uint8_t count_max = 60; // Default to 1 minute timeout

void setup()
{
    // time of last 'tick' - so that we
    // know when to 'tick' again
    static uint32_t last_millis = 0;

    // Remember the state of the 'tick' LED
    // to that we can invert it each time we 'tick'
    static uint8_t blink_state = 0;

    // running count of 'ticks' - when this matches
    // count_max, then we should timeout
    static uint8_t count = 0;

    uint8_t usb_byte_in = 0;

    DigiUSB.begin();
    pinMode(PIN_LED_TICK, OUTPUT);
    pinMode(PIN_LED_ARMED, OUTPUT);
    pinMode(PIN_RST, INPUT); // Tri-state the reset line
                             // Remember: we have a volatage
                             // level difference here - we are
                             // running at 5v, the PI is at 3.3v
                             // So we cant just set this to a
                             // 'high' output. Instead we can let
                             // it float high, and pull it low when
                             // we want to assert RESET by flipping
                             // this pin to an output, and setting it
                             // low.
}



void loop()
{
    // Chuck a NUL terminator at the end of
    // the buffer space for good measure,
    cmd_buffer[MAX_CMD_LEN] = 0;

    // Let the USB code have some CPU time
    DigiUSB.refresh();

    // Handle any input from the host
    if (DigiUSB.available() > 0)
    {
        // We read from the host one byte
        // at a time.
        // We reconstruct the incoming bytes
        // into a C/R or NUL terminated string
        // before attempting to parse it.
        usb_byte_in = DigiUSB.read();

        //DigiUSB.println(usb_byte_in, DEC);

        // Store the new byte away, increment
        // the storage location, null terminate the
        // emerging string and wrap at the end of
        // the buffer space...
        cmd_buffer[cmdbuf_idx++] = usb_byte_in;
        cmd_buffer[cmdbuf_idx] = 0;
        if (cmdbuf_idx >= MAX_CMD_LEN)
            cmdbuf_idx = 0;

        //DigiUSB.println(cmd_buffer);

        // If we have seen the end of line or end of file
        // then go ahead and parse the string
        if (usb_byte_in == 10 || usb_byte_in == 0)
        {
            if (strncmp_P(cmd_buffer, PSTR("arm"), 3)==0)
            {
                is_armed = 1;
                count = 0;
                DigiUSB.println("Armed!");
            }

            if (strncmp_P(cmd_buffer, PSTR("dis"), 3)==0)
            {
                is_armed = 0;
                count = 0;
                DigiUSB.println("Disarmed!");
            }

            if (strncmp_P(cmd_buffer, PSTR("t="), 2)==0)
            {
                // Temporary storage to hold the numeric
                // part of the command 't={number}'
                char decimal_str[4];

                // If there was something after the '='
                if (strlen(cmd_buffer) > 2)
                {
                        // tuck away the rhs of the line
                        // from the '=' up to a max of 3 more
                        // characters (digits)
                        strncpy(decimal_str, (cmd_buffer+2), 3);
                        // Convert the string to it's numeric
                        // equivilance
                        count_max=atoi(decimal_str);
                }
                DigiUSB.print("Timeout:");
                DigiUSB.print(decimal_str);
            }

            // Return to the start of the buffer and
            // start building the next string
            cmdbuf_idx = 0;
        }

    } // End of 'input available'


    // TIME based decisions
    uint32_t tnow = millis();

    // Has a second elapsed since we last 'ticked'
    if (tnow - last_millis > 1000)
    {
        // Mark the time of this new tick
        last_millis = tnow;

        // Increment the total number of ticks
        count = count + 1;

        // Invert the blink LED state
        blink_state = 1 - blink_state;

        // Have there been sufficient 'ticks' to
        // require us to trigger?
        if (count >= count_max)
        {
            count = 0;
            // Only pull the reset if we are 'armed'
            if (is_armed == 1)
            {
                pinMode(PIN_RST, OUTPUT);
                digitalWrite(PIN_RST, 0);
                DigiUSB.println("ALARM!");
            }
            else
                // Otherwise, just emit a notification
                // to anyone who happens to be listening..
                DigiUSB.println("TimerOVF!");
        }
        else
            // Not timed out, leave (or re-set) the
            // RESET pin as an INPUT (let it float)
            pinMode(PIN_RST, INPUT);
    }

    // Update LED's to show current state
    digitalWrite(PIN_LED_ARMED, is_armed);
    digitalWrite(PIN_LED_TICK, blink_state);
}
