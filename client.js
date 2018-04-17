var EtherPortClient = require("etherport-client").EtherPortClient;
var five = require("johnny-five");
var board, led;

// var webduino = require('webduino-js');
// var boardLED,rgb;
// var redValue = 0;
// var greenValue = 0;
// var blueValue = 0;
// var rgbled; 
// var rgbledRed;
// var rgbledGreen;
// var rgbledBlue;

var board = new five.Board({
  port: new EtherPortClient({
    host: "192.168.2.7",
    port: 3030
  }),
  timeout: 1e5,
  repl: false
});

board.on("ready", function() {
  this.pinMode(2, five.Pin.OUTPUT);
  this.digitalWrite(2, 1);

  console.log("READY!");
  led = new five.Led(2);
  // blinks the blue LED on a WeMos ESP8266 board
  // led.blink(500);
});

var socket = require('socket.io-client')('http://134.157.18.187:3000'); // #TODO update server ip

// socket.on('ledFrequency', function (data) {
//   console.log(data);
//   if(board.isReady) {
//     led.blink(data);
//   }
// });

socket.on('sensor data red', function (red) {
  if(board.isReady) {
    if(red > 90){
      led.blink(2000);
    }  
  }
});

socket.on('sensor data green', function (green) {
  if(board.isReady) {
    if(green > 90){
      led.blink(500);
    }  
  }
});

socket.on('sensor data blue', function (blue) {
  if(board.isReady) {
    if(blue > 90){
      led.blink(100);
    }  
  }
});


// boardLED = new webduino.WebArduino('10VE843y');
// boardLED.on(webduino.BoardEvent.READY, function() {
//   rgb = new webduino.module.RGBLed(boardLED,
//     boardLED.getDigitalPin(15),
//     boardLED.getDigitalPin(12),
//     boardLED.getDigitalPin(13)
//     );
//   rgb.setColor(0, 0, 0);
//   console.log(rgb);
// });

// var photocell,rgbledGreen;
// // Connect to Smart and emit sensor data red
// boardLED = new webduino.WebArduino('10VE843y');
// boardLED.on(webduino.BoardEvent.READY, function() {
//   boardLED.systemReset();
//   boardLED.samplingInterval = 50;
//   console.log(boardLED.samplingInterval);
//   rgbledGreen = new webduino.module.RGBLed(boardLED,
//     boardLED.getDigitalPin(15),
//     boardLED.getDigitalPin(12),
//     boardLED.getDigitalPin(13)
//     );
//   rgbledGreen.setColor(0,255,0);

//   photocell = new webduino.module.Photocell(boardLED, boardLED.getDigitalPin(0));
//   // photocell = getPhotocell(boardLED, 0);
//   photocell.on(function(val){
//     console.log(val);
//     photocell.detectedVal = val;
//     greenValue = (Math.round((((photocell.detectedVal - (0)) * (1/((1)-(0)))) * ((100)-(0)) + (0))*100))/100;
//     console.log(greenValue);
//     socket.emit('sensor data green', greenValue);
//   });

// });