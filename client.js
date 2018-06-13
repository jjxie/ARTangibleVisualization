var five = require("johnny-five");
var board = new five.Board({port : '/dev/tty.wchusbserial14220'});
var socket = require('socket.io-client')('http://localhost:3000');
var led;
var maxWeight = 1000;
var servoStartDegree = 5;
var servoMilk, servoOrange, serveoMeat, servoBroccoli, servoFish;
var milkSelected = false;
var orangeSelected = false;
var meatSelected = false;
var broccoliSelected = false;
var fishSelected = false;

// The full length of each servo
var servoMilkFullLength = 13.4;
var servoOrangeFullLength = 12.3;
var serveoMeatFullLength = 12.3;
var servoBroccoliFullLength = 12.3;
var servoFishFullLength = 11.9;

// The length of per degree
var servoMilkLengthPerDegree = 0.087;
var servoOrangeLengthPerDegree = 0.072;
var servoMeatLengthPerDegree = 0.072;
var servoBroccoliLengthPerDegree = 0.072;
var servoFishLengthPerDegree = 0.06;

// The max degree of each servo
var servoMilkMaxDegree = 160;
var servoOrangeMaxDegree = 170;
var servoMeatMaxDegree = 170;
var servoBroccoliMaxDegree = 170;
var servoFishMaxDegree = 180;

// Each servo's position
var servoMilkPosition = 0;
var servoOrangePosition = 0;
var serveoMeatPosition = 0;
var servoBroccoliPosition = 0;
var servoFishPosition = 0;

// The number before current number
var milkPreWeight = 0;
var orangePreWeight = 0;
var meatPreWeight = 0;
var broccoliPreWeight = 0;
var fishPreWeight = 0;

// Historical data
var milkHistory = {date:"Wed Jun 06 2018 10:41:53 GMT+0200 (CEST)", time:"1528274513.542", weight:0};
var orangeHistory = {date:"Wed Jun 06 2018 10:41:53 GMT+0200 (CEST)", time:"1528274513.542", weight:0};
var meatHistory = {date:"Wed Jun 06 2018 10:41:53 GMT+0200 (CEST)", time:"1528274513.542", weight:0};
var broccoliHistory = {date:"Wed Jun 06 2018 10:41:53 GMT+0200 (CEST)", time:"1528274513.542", weight:0};
var fishHistory = {date:"Wed Jun 06 2018 10:41:53 GMT+0200 (CEST)", time:"1528274513.542", weight:0};



board.on("ready", function() {
	led = new five.Led(13);
	led.blink(2000);

  // Attach 5 servos to PIN from 8 - 12
  servoMilk = new five.Servo({
    pin: 8,
    startAt: servoStartDegree
  });
  servoOrange = new five.Servo({
    pin: 9,
    startAt: servoStartDegree
  });
  serveoMeat = new five.Servo({
    pin: 10,
    startAt: servoStartDegree
  });
  servoBroccoli = new five.Servo({
    pin: 11,
    startAt: servoStartDegree
  });
  // servoFish = new five.Servo({
  //   pin: 12,
  //   startAt: 5
  // });


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
  	// console.log("Milk selected ", milkSelected);
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
  	// console.log("Orange selected ", orangeSelected); 	
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
    // console.log("Meat selected ", meatSelected);
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
    // console.log("Broccoli selected ", broccoliSelected);
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
  // console.log("Fish selected ", fishSelected);
});

});

// Dealing with the weight changes
socket.on('scaleDataMilk', function (milkWeight) {
	if(board.isReady) {
    servoMilk.to( calDegree(milkWeight, servoMilkFullLength, servoMilkLengthPerDegree));
    
  }
});

socket.on('scaleDataOrange', function (orangeWeight) {
  if(board.isReady) {
    servoOrange.to ( calDegree(orangeWeight, servoOrangeFullLength, servoOrangeLengthPerDegree));
  }

});

socket.on('scaleDataMeat', function (meatWeight) {
	if(board.isReady) {
    serveoMeat.to( calDegree(meatWeight, serveoMeatFullLength, servoMeatLengthPerDegree));
  }
});

socket.on('scaleDataBroccoli', function (broccoliWeight) {
  if(board.isReady) {
    servoBroccoli.to( calDegree(broccoliWeight, servoBroccoliFullLength, servoBroccoliLengthPerDegree));
  }
});

// socket.on('scaleDataFish', function (fishWeight) {
//   if(board.isReady) {
//     servoFish.to( calDegree(fishWeight, servoFishFullLength, servoFishLengthPerDegree));
//   }
// });

function calDegree(weight, fullLength, lenghPerDegree){
  var degree = Math.round((weight/maxWeight * fullLength) / lenghPerDegree + 0.5)
  return degree;
}