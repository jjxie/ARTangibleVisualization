var app = require('express')();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var fs = require('fs');

var connectedIDArray = [];

var milkSelected = false;
var orangeSelected = false;
var meatSelected = false;
var broccoliSelected = false;
var fishSelected = false;


// Node serialport
var Serialport = require('serialport');
var myPort = new Serialport("/dev/tty.wchusbserial1410",{
	baudrate: 74880,
	parser: Serialport.parsers.readline("\n")
});


app.get('/', function(req, res){
	res.sendFile(__dirname + '/index.html');
});


io.on('connection', function(socket){
	console.log('a user connected');
	// console.log(socket.id);
	connectedIDArray.push(socket.id);
	// console.log("Connected array length: " + connectedIDArray.length); 

	// // Send message
	// socket.on('chat message', function(msg){
	// 	// console.log('message: ' + msg);
	// 	// send to all
	// 	io.emit('chat message', socket.id + " : " + msg);
	// 	// send to others except the sender
	// 	// socket.broadcast.emit('chat message', msg);
	// });

	// Online event
	io.emit('online', socket.id);

	// Read initial file data when connected for the first time
	fs.readFile('test.json', function(err, data) {
		if (err) throw err;
		//io.emit("initial content", data);
		// Receive intial data when connected to server
		io.to(socket.id).emit("initial content", data);
		//console.log("First connection data: " + data);
	});

	// Disconnect	
	socket.on('disconnect', function(){
		console.log('user disconnected');
		console.log(socket.id);
		var index = connectedIDArray.indexOf(socket.id); 
		if(index > -1){	
			connectedIDArray.splice(index, 1);
		}
		// console.log("Connected array length: " + connectedIDArray.length); 
		// Offline event
		io.emit('offline', socket.id);
	});

	// If led message received
	socket.on('ledFrequencySet', function (data) {
		console.log(data);
		io.sockets.emit('ledFrequency',data);
	});

	// Milk selection status
	socket.on('milk', function (data) {
		// console.log("Milk touch sensor is touched " + data);
		// Change selection status and emit corresponding messgae
		if(milkSelected == true){
			milkSelected = false;
			io.sockets.emit('milkIsDeselected', false);	
		}
		else{
			milkSelected = true;
			io.sockets.emit('milkIsSelected', true);
		}	
	});

	// Orange selection status
	socket.on('orange', function (data) {
		// console.log("Orange touch sensor is touched " + data);
		if(orangeSelected == true){
			orangeSelected = false;
			io.sockets.emit('orangeIsDeselected', false);	
		}
		else{
			orangeSelected = true;
			io.sockets.emit('orangeIsSelected', true);
		}
		
	});

	// Meat selection status
	socket.on('meat', function (data) {
		// console.log("Meat touch sensor is touched " + data);
		if(meatSelected == true){
			meatSelected = false;
			io.sockets.emit('meatIsDeselected', false);
		}
		else{
			meatSelected = true;
			io.sockets.emit('meatIsSelected', true);	
		}
		
	});

	// Broccoli selection status
	socket.on('broccoli', function (data) {
		// console.log("Broccoli touch sensor is touched " + data);
		if(broccoliSelected == true){
			broccoliSelected = false;
			io.sockets.emit('broccoliIsDeselected', false);
		}
		else{
			broccoliSelected = true;
			io.sockets.emit('broccoliIsSelected', true);
		}
		
	});

	// Fish selection status
	socket.on('fish', function (data) {
		// console.log("Fish touch sensor is touched " + data);
		if(fishSelected == true){
			fishSelected = false;
			io.sockets.emit('fishIsDeselected', false);
		}
		else{
			fishSelected = true;
			io.sockets.emit('fishIsSelected', true);
		}
		
	});
});

// Read data from serial port and emit the weight data
myPort.on('data', function (data){
	console.log("Data received from Wemos: " + data);
	// Get the first character which indicates which food's weight
	var foodType = data.substring(0,1);
  	// Remove the first letter, the remaining is the weight data
  	var weight = data.substring(1);
  	switch(foodType) {
    	// Type A represents milk
    	case "A":
    	io.emit("scaleDataMilk", weight);
    	break;
    	// Type B represents orange
    	case "B":
    	io.emit("scaleDataOrange", weight);
    	break;
    	// Type C represents meat
    	case "C":
    	io.emit("scaleDataMeat", weight);
    	break;
    	// Type D represents broccoli
    	case "D":
    	io.emit("scaleDataBroccoli", weight);
    	break;
    	// Type E represents fish
    	case "E":
    	io.emit("scaleDataFish", weight);
    	break;
    	default:
    	console.log("unknownMessage", data);
    }
});

// Print error message on console when has problem connecting to the port
myPort.on('error', function(err) {
	console.log('Error: ', err.message);
});

// Real time check file content changes
fs.watch('test.json', 
	function(event, filename){
		fs.readFile('test.json',
			function(err, data){
				if (err) throw err;
				io.emit("update content", data);
				console.log('file data: ' + data);
			});
	});


http.listen(3000,'192.168.0.100', function(){
	console.log('listening on *:3000');
});
