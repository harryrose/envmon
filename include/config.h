#ifndef __EM_CONFIG_H_
#define __EM_CONFIG_H_

#define IP_ADDRESS(a,b,c,d) (a | ( b << 8) | (c << 16) | (d << 24))

#define LOCAL_IP      IP_ADDRESS(192,168,0,2)
#define GATEWAY_IP    IP_ADDRESS(192,168,0,1)
#define SUBNET_MASK   IP_ADDRESS(255,255,255,0)
#define DNS_PRIMARY   IP_ADDRESS(8,8,8,8)
#define DNS_SECONDARY IP_ADDRESS(8,8,4,4)

// Define these in the command line to avoid putting them in the git repo
// Using platformIO, this is in platform.ini, and then secrets specified in the environment.
// #define WIFI_SSID 
// #define WIFI_PASS 

// #define THINGSPEAK_CHANNEL 
// #define THINGSPEAK_KEY     

#define BATT_PIN 32
#define SOLAR_PIN 33
#define LED_PIN 2
#define BUFFER_POINTS 10

#define TIME_TO_SLEEP  30       /* Time ESP32 will go to sleep (in seconds) */
#define TIME_TO_SLEEP_AFTER_FAILURE 5

#endif
