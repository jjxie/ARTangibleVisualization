var five = require("johnny-five");
var board = new five.Board();
var socket = require('socket.io-client')('http://localhost:3000');
var led, servoMilk, servoFish;

board.on("ready", function() {
  led = new five.Led(13);
  led.blink(2000);
  //servoMilk = new five.Servo(8);
  //servoFish = new five.Servo(9)
  this.pinMode(2, five.Pin.INPUT);
  this.digitalRead(2, function(value) {
       if(value === 0)
       {
           console.log('0 not in touch');
       }
       else{
       	   console.log('1 in touch');
       }
   });
});


socket.on('sensor data red', function (red) {
  if(board.isReady) {
    if(red > 90){
      led.blink(2000);
      //servoMilk.to( 90 );
    }  
  }
});

socket.on('sensor data green', function (green) {
  if(board.isReady) {
    if(green > 90){
      led.blink(500);
      //servoFish.to( 90 );
    }  
  }
});

socket.on('sensor data blue', function (blue) {
  if(board.isReady) {
    if(blue > 90){
      led.blink(100);
      //servoMilk.to( 0 );
      //servoFish.to( 0 );
    }  
  }
});