# my-home-sensor-system

An Arduino sketch and a prometheus client for collecting various sensor data in my office.

**This was made for personal purposes and contains all kinds of horrors**

## Usage

This was made for Arduino UNO with a Grove system using various sensors like
temp, humidity, air quality, light and sound.

The node application in `server/index.js` starts up a HTTP server with an endpoint
for prometheus and collects data via serial.

## License

CC BY
