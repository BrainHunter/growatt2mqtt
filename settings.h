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

// special defines for the ethernet part of the WT32_ETH01 Board
#ifdef ARDUINO_WT32_ETH01
    /* 
    * ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
    * ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
    * ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
    * ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
    */
    //#undefine ETH_CLK_MODE
    //#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT

    // Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
    #define ETH_POWER_PIN   16

    // Type of the Ethernet PHY (LAN8720 or TLK110)
    #define ETH_TYPE        ETH_PHY_LAN8720

    // I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
    #define ETH_ADDR        1

    // Pin# of the I²C clock signal for the Ethernet PHY
    #define ETH_MDC_PIN     23

    // Pin# of the I²C IO signal for the Ethernet PHY
    #define ETH_MDIO_PIN    18

    // MAC address of Device 1
    uint8_t mac[] = {0xDE, 0xAD, 0xAE, 0xEF, 0xFE, 0xEE};
#endif