var five = require("johnny-five");
var board = new five.Board();
var socket = require('socket.io-client')('http://localhost:3000');
var led;
var servoMilk, servoOrange, serveoMeat, servoBroccoli, servoFish;
var milkSelected = false;
var orangeSelected = false;
var meatSelected = false;
var broccoliSelected = false;
var fishSelected = false;

board.on("ready", function() {
	led = new five.Led(13);
	led.blink(2000);

  // Attach 5 servos to PIN from 8 - 12
  // servoMilk = new five.Servo(8);
  // servoOrange =  new five.Servo(9);
  // serveoMeat = new five.Servo(10);
  // servoBroccoli = new five.Servo(11);
  // servoFish = new five.Servo(12);


  // Touch sensor digital PIN 2, milk
  this.pinMode(2, five.Pin.INPUT);
  this.digitalRead(2, function(value) {
  	if(value === 1)
  	{
  		if(milkSelected === false){
  			milkSelected = true;
  		}
  		else{
  			milkSelected = false;
  		}
  	}
  	socket.emit('Milk selection status', milkSelected);
  	console.log("Milk selected ", milkSelected);
  });

  // Touch sensor digital PIN 3, orange
  this.pinMode(3, five.Pin.INPUT);
  this.digitalRead(3, function(value) {
  	if(value === 1)
  	{
  		if(orangeSelected === false){
  			orangeSelected = true;
  		}
  		else{
  			orangeSelected = false;
  		}
  	}
  	socket.emit('Orange selection status', orangeSelected);
  	console.log("Orange selected ", orangeSelected); 	
  });

  // Touch sensor digital PIN 4, meat
  this.pinMode(4, five.Pin.INPUT);
  this.digitalRead(4, function(value) {
  	if(value === 1)
  	{
  		if(meatSelected === false){
  			meatSelected = true;
  		}
  		else{
  			meatSelected = false;
  		}
  	}
    socket.emit('Meat selection status', meatSelected);
    console.log("Meat selected ", meatSelected);
  });

  // Touch sensor digital PIN 5, broccoli
  this.pinMode(5, five.Pin.INPUT);
  this.digitalRead(5, function(value) {
  	if(value === 1)
  	{
  		if(broccoliSelected === false){
  			broccoliSelected = true;
  		}
  		else{
  			broccoliSelected = false;
  		}
  	}
    socket.emit('Broccoli selection status', broccoliSelected);
    console.log("Broccoli selected ", broccoliSelected);
  });

  // Touch sensor digital PIN 6, fish
  this.pinMode(6, five.Pin.INPUT);
  this.digitalRead(6, function(value) {
    if(value === 1)
    {
     if(fishSelected === false){
      fishSelected = true;
    }
    else{
      fishSelected = false;
    }
  }
  socket.emit('Fish selection status', fishSelected);
  console.log("Fish selected ", fishSelected);
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