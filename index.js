var app = require('express')();
var https = require('https');
var fs = require('fs');

// https secure connection to avoid webcam only secure origins are allowed error
// https://stackoverflow.com/questions/31156884/how-to-use-https-on-node-js-using-express-socket-io/31165649#31165649?newreg=43fe9326a31942a591b02baf50dc1a07
var options = {
	key: fs.readFileSync('./file.pem'),
	cert: fs.readFileSync('./file.crt')
};

var server = https.createServer(options, app);
var io = require('socket.io')(server);


var connectedIDArray = [];

// If selected flag
var milkSelected = false;
var orangeSelected = false;
var meatSelected = false;
var broccoliSelected = false;
var fishSelected = false;

// Historical data
var milkHistory = [];
var orangeHistory = [];
var meatHistory = [];
var broccoliHistory = [];
var fishHistory = [];

// Initial flag
var milkIni = true;
var orangeIni = true;
var meatIni= true;
var broccoliIni = true;
var fishIni = true;



// // Node serialport
// var Serialport = require('serialport');
// var myPort = new Serialport("/dev/tty.wchusbserial1410",{
// 	baudrate: 74880,
// 	parser: Serialport.parsers.readline("\n")
// });


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

	// // If led message received
	// socket.on('ledFrequencySet', function (data) {
	// 	console.log(data);
	// 	io.sockets.emit('ledFrequency',data);
	// });

	// Milk selection status
	socket.on('milk', function (data) {
		console.log("Milk touch sensor is touched " + data);
		// Change selection status and emit corresponding messgae
		if(milkSelected == true){
			milkSelected = false;
			io.sockets.emit('milkIsDeselected', false);	
		}
		else{
			milkSelected = true;
			io.sockets.emit('milkIsSelected', milkHistory);
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
			io.sockets.emit('orangeIsSelected', orangeHistory);
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
			io.sockets.emit('meatIsSelected', meatHistory);	
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
			io.sockets.emit('broccoliIsSelected', broccoliHistory);
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
			io.sockets.emit('fishIsSelected', fishHistory);
		}
		
	});

	// Milk weight
	socket.on('milkWeight', function (data) {
		console.log("milk weight: " + data);
		// First time, check and emit data, and save to history
		if(milkIni === true){
			if(data < 3.00){
				data = 0.00;
				io.sockets.emit('scaleDataMilk', data);		
				console.log("Emit milk data:  " + data);
			}
			else{
				io.sockets.emit('scaleDataMilk', data);	
				console.log("Emit milk data: " + data);
			}
			addToHistory(milkHistory, data);
			milkIni = false;
		}
		// Check data and historical data
		else{
			if( data < 3.00){
				// Prevent to emit and record multiple zeros if the previous weight is zero
				if(!checkZeroData(milkHistory, data)){
					data = 0.00;
					io.sockets.emit('scaleDataMilk', data);
					addToHistory(milkHistory, data);	
					console.log("Emit milk data:  " + data);	
				}
			}	
			else{
				// If the new weight changes more than 1g, then emit new weight and add to history
				if(checkData(milkHistory, data)){
					io.sockets.emit('scaleDataMilk', data);	
					addToHistory(milkHistory, data);
					console.log("Emit milk data: " + data);
				}	
			}
		} 
	});

	// Orange weight
	socket.on('orangeWeight', function (data) {
		console.log("orange weight: " + data);
		// First time, check and emit data, and save to history
		if(orangeIni === true){
			if(data < 3.00){
				data = 0.00;
				io.sockets.emit('scaleDataOrange', data);	
				console.log("Emit orange data:  " + data);
			}
			else{
				io.sockets.emit('scaleDataOrange', data);	
				console.log("Emit orange data: " + data);
			}
			addToHistory(orangeHistory, data);
			orangeIni = false;
		}
		// Check data and historical data
		else{
			if( data < 3.00){
				// Prevent to emit and record multiple zeros if the previous weight is zero
				if(!checkZeroData(orangeHistory, data)){
					data = 0.00;
					io.sockets.emit('scaleDataOrange', data);	
					console.log("Emit orange data:  " + data);
					addToHistory(orangeHistory, data);
				}
			}	
			else{
				if(checkData(orangeHistory, data)){
					io.sockets.emit('scaleDataOrange', data);	
					console.log("Emit orange data: " + data);
					addToHistory(orangeHistory, data);
				}	
			}
		}		
	});

	// Meat weight
	socket.on('meatWeight', function (data) {
		console.log("meat weight: " + data);
		// First time, check and emit data, and save to history
		if(meatIni === true){
			if(data < 3.00){
				data = 0.00;
				io.sockets.emit('scaleDataMeat', data);	
				console.log("Emit meat data:  " + data);
			}
			else{
				io.sockets.emit('scaleDataMeat', data);	
				console.log("Emit meat data: " + data);
			}
			addToHistory(meatHistory, data);
			meatIni = false;
		}
		// Check data and historical data
		else{
			if( data < 3.00){
				// Prevent to emit and record multiple zeros if the previous weight is zero
				if(!checkZeroData(meatHistory, data)){
					data = 0.00;
					io.sockets.emit('scaleDataMeat', data);	
					console.log("Emit meat data:  " + data);
					addToHistory(meatHistory, data);
				}
			}	
			else{
				if(checkData(meatHistory, data)){
					io.sockets.emit('scaleDataMeat', data);	
					console.log("Emit meat data: " + data);
					addToHistory(meatHistory, data);
				}	
			}
		}
	});

	// Broccoli weight
	socket.on('broccoliWeight', function (data) {
		console.log("broccoli weight: " + data);
		// First time, check and emit data, and save to history
		if(broccoliIni === true){
			if(data < 3.00){
				data = 0.00;
				io.sockets.emit('scaleDataBroccoli', data);	
				console.log("Emit broccoli data:  " + data);
			}
			else{
				io.sockets.emit('scaleDataBroccoli', data);	
				console.log("Emit broccoli data: " + data);
			}
			addToHistory(broccoliHistory, data);
			broccoliIni = false;
		}
		// Check data and historical data
		else{
			if( data < 3.00){
				// Prevent to emit and record multiple zeros if the previous weight is zero
				if(!checkZeroData(broccoliHistory, data)){
					data = 0.00;
					io.sockets.emit('scaleDataBroccoli', data);	
					console.log("Emit broccoli data:  " + data);
					addToHistory(broccoliHistory, data);
				}
			}	
			else{
				if(checkData(broccoliHistory, data)){
					io.sockets.emit('scaleDataBroccoli', data);	
					console.log("Emit broccoli data: " + data);
					addToHistory(broccoliHistory, data);
				}	
			}
		}	
	});

	// // Fish weight
	// socket.on('fishWeight', function (data) {
	// 	console.log("fish weight: " + data);	
	// 	io.sockets.emit("scaleDataFish", data);	
	// 	var currentDate = new Date();
	// 	var date = currentDate.toString();
	// 	var time = currentDate/1000;
	// 	fishHistory.push({
	// 		date: date,
	// 		time: time,
	// 		weight: data
	// 	})
	// });

});

// // Read data from serial port and emit the weight data
// myPort.on('data', function (data){
// 	console.log("Data received from Wemos: " + data);
// 	// Get the first character which indicates which food's weight
// 	var foodType = data.substring(0,1);
//   	// Remove the first letter, the remaining is the weight data
//   	var weight = data.substring(1);
//   	switch(foodType) {
//     	// Type A represents milk
//     	case "A":
//     	io.emit("scaleDataMilk", weight);
//     	break;
//     	// Type B represents orange
//     	case "B":
//     	io.emit("scaleDataOrange", weight);
//     	break;
//     	// Type C represents meat
//     	case "C":
//     	io.emit("scaleDataMeat", weight);
//     	break;
//     	// Type D represents broccoli
//     	case "D":
//     	io.emit("scaleDataBroccoli", weight);
//     	break;
//     	// Type E represents fish
//     	case "E":
//     	io.emit("scaleDataFish", weight);
//     	break;
//     	default:
//     	console.log("unknownMessage", data);
//     }
// });

// // Print error message on console when has problem connecting to the port
// myPort.on('error', function(err) {
// 	console.log('Error: ', err.message);
// });

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


server.listen(3000,'192.168.0.100', function(){
	console.log('listening on *:3000');
});


//Add to history object
function addToHistory(historicalObject, weight){
	// Add the new data to history
	var currentDate = new Date();
	var date = currentDate.toString();
	var time = currentDate/1000;
	historicalObject.push({
		date: date,
		time: time,
		weight: weight
	})
}

// Check if the previous one is zero, return true is
function checkZeroData(historicalObject, weight){
	// console.log(" Enter zero check ! ");
	if(historicalObject[Object.keys(historicalObject).length - 1].weight === 0){
		return true;
	}
	else{
		return false;
	}
}

// Check it the new received data has difference with the previous data more than 1g
function checkData(historicalObject, weight){
	// console.log(" Enter check ! ");
	var previousWeight = historicalObject[Object.keys(historicalObject).length - 1].weight;
	if(Math.abs(weight - previousWeight) < 1.00){
		return false;
	}
	else{
		return true;
	}
}
