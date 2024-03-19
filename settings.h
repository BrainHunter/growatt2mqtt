//#define DEBUG_SERIAL    1
//#define DEBUG_MQTT      1 
//#define useModulPower   1

// ------------- PIN DEFINES --------------
#if defined(ESP8266)                //
    #define MAX485_DE       5       // D1, DE pin on the TTL to RS485 converter
    #define MAX485_RE_NEG   4       // D2, RE pin on the TTL to RS485 converter
    #define MAX485_RX       14      // D5, RO pin on the TTL to RS485 converter
    #define MAX485_TX       12      // D6, DI pin on the TTL to RS485 converter
    #define STATUS_LED      2       // Status LED on the Wemos D1 mini (D4)

    #define CONFIG_PIN D2           // Pull this pin to GND on startup to reset settings
    #define RGBLED_PIN D3           // Neopixel led D3

#elif defined(ARDUINO_WT32_ETH01)   // WT32_ETH01
    #define MAX485_DE       33      // DE pin on the TTL to RS485 converter
    #define MAX485_RE_NEG   33      // RE pin on the TTL to RS485 converter
    #define MAX485_RX       35      // RO pin on the TTL to RS485 converter
    #define MAX485_TX       17      // DI pin on the TTL to RS485 converter
    #define STATUS_LED      12      // Status LED on the Wemos D1 mini (D4)

    #define CONFIG_PIN      32      // Pull this pin to GND on startup to reset settings
    #define RGBLED_PIN      14      // Neopixel led

#elif defined(ESP32)                // GENERIC ESP32 board
    #define MAX485_DE       33      // DE pin on the TTL to RS485 converter
    #define MAX485_RE_NEG   33      // RE pin on the TTL to RS485 converter
    #define MAX485_RX       35      // RO pin on the TTL to RS485 converter
    #define MAX485_TX       17      // DI pin on the TTL to RS485 converter
    #define STATUS_LED      12      // Status LED on the Wemos D1 mini (D4)

    #define CONFIG_PIN      32      // Pull this pin to GND on startup to reset settings
    #define RGBLED_PIN      14      // Neopixel led
#endif

#define SERIAL_RATE     115200    // Serial speed for status info
#define UPDATE_MODBUS   2         // 1: modbus device is read every second
#define UPDATE_STATUS   30        // 10: status mqtt message is sent every 10 seconds
#define RGBSTATUSDELAY  500       // delay for turning off the status led
#define WIFICHECK       500       // how often check lost wifi connection

#define NUM_LEDS 1
#define LED_TYPE    WS2812
#define COLOR_ORDER GRB
#define BRIGHTNESS  20        // Default LED brightness.



#define CONFIG_VERSION "gro1"
