var five = require("johnny-five");
var board = new five.Board({port : '/dev/tty.wchusbserial14220'});
var socket = require('socket.io-client')('http://localhost:3000');
var led;
var maxWeight = 1000;
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
var servoBroccoliLengthPerDegree = 0.087;
var servoFishLengthPerDegree = 0.087;

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
  servoMilk = new five.Servo(8);
  servoOrange =  new five.Servo(9);
  serveoMeat = new five.Servo(10);
  servoBroccoli = new five.Servo(11);
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
    // servoMilk.attach(8);
    servoMilk.to( Math.round((milkWeight/maxWeight * servoMilkFullLength) / servoMilkLengthPerDegree + 0.5) );
  }
});

socket.on('scaleDataOrange', function (orangeWeight) {
  if(board.isReady) {
    // servoOrange.attach(9);
    console.log("ENTER");
    var degree = Math.round((orangeWeight/maxWeight * servoOrangeFullLength) / servoOrangeLengthPerDegree + 0.5) ;
    console.log(degree);
    servoOrange.to(degree);
    servoOrange.stop();
  }

});

socket.on('scaleDataMeat', function (meatWeight) {
	if(board.isReady) {
    // serveoMeat.attach(10);
    serveoMeat.to( Math.round((meatWeight/maxWeight * serveoMeatFullLength) / servoMeatLengthPerDegree + 0.5) );
    // serveoMeat.detach();
  }
});

socket.on('scaleDataBroccoli', function (broccoliWeight) {
  if(board.isReady) {
    // servoBroccoli.attach(11);
    servoBroccoli.to( Math.round((broccoliWeight/maxWeight * servoBroccoliFullLength) / servoBroccoliLengthPerDegree + 0.5) );
    // servoBroccoli.detach();
  }
});

// socket.on('scaleDataFish', function (fishWeight) {
//   if(board.isReady) {
//     servoFish.attach(12);
//     servoFish.to( Math.round((fishWeight/maxWeight * servoFishFullLength) / servoFishLengthPerDegree + 0.5) );
//     servoFish.detach();
//   }
// });