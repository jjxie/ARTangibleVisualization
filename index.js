var app = require('express')();
var https = require('https');
var fs = require('fs');
var moment = require('moment');

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

// This is used to show five days history in AR detailed view. The data is parsed from Json file
var milkHistoryJson =[];
var orangeHistoryJson = [];
var meatHistoryJson = [];
var broccoliHistoryJson = [];
var fishHistoryJson = [];

// Initial flag
var milkIni = true;
var orangeIni = true;
var meatIni= true;
var broccoliIni = true;
var fishIni = true;

// Json file works as a database for history data
var milkJson = "milkHistory.json";
var orangeJson = "orangeHistory.json";
var meatJson = "meatHistory.json";
var broccoliJson = "broccoliHistory.json";
var fishJson = "fishHistory.json";
var nutritionJson = "nutritionHistory.json";

// Calcium mg, calories, fat g, protein g, vitaminC mg,  per 100g
var milkNutrients = [125, 42, 1, 3.4, 0];
var orangeNutrients = [40, 47.01, 0.11, 0.92, 53.2];
var meatNutrients = [6, 143, 3.5, 26, 0];
var broccoliNutrients = [47, 34, 0.4, 2.8, 89.2];
var fishNutrients = [15, 206, 12, 22, 3.7];

// This data based on woman, better collect user data and set more accurate daily intake
// Calcium, 19-50 years old male and female 1000mg
// Calories, maintain level, female 2000, male 2500
// Fat, 70 g 
// Protein, 0.8 * KG or 0.36 * Pound
// VitaminC, adults female 75mg, male 90mg
var dailyIntake =[1000, 2000, 70, 46, 75];

var nutrientsArray = [950,1800,0,40,50];
var milkNutrientsArray = [200,300,0,0,0];
var orangeNutrientsArray = [300,400,0,0,30];
var meatNutrientsArray = [400,100,60,30,0];
var broccoliNutrientsArray = [100,200,0,0,20];
var fishNutrientsArray = [50,1000,0,10,0];

var virtualFlag = false;
var virtualNutrition =[0,0,0,0,0];

var bodyparser = require('body-parser');
app.use(bodyparser.urlencoded({extended:false}));
app.use(bodyparser.json());


// // Node serialport
// var Serialport = require('serialport');
// var myPort = new Serialport("/dev/tty.wchusbserial1410",{
// 	baudrate: 74880,
// 	parser: Serialport.parsers.readline("\n")
// });
app.get('/index', function(req, res){
	res.sendFile(__dirname + '/index.html');
});

app.get('/client', function(req, res){
	res.sendFile(__dirname+ '/client.html');
});

// Check if it is midnight now.
// If it is, then write nutrientsArray to Json and clear nutrientsArray 
var midnight = "0:00:00";
var now = null;
setInterval(function () {
	now = moment().format("H:mm:ss");
	if (now === midnight) {
		console.log("Write to nutriton json and re-initial nutrition array!");
		var date = new Date();
		date.setDate(date.getDate()-1);
		fs.readFile("nutritionHistory.json", 'utf8', function readFileCallback(err, data){
			if (err){
				console.log("Read nutrition history file err: " + err);
			} else{
				jsonArray = JSON.parse(data);
				jsonArray.push({
					date: date,
					calcium: nutrientsArray[0],
					calories: nutrientsArray[1],
					fat: nutrientsArray[2],
					protein: nutrientsArray[3],
					vitaminC: nutrientsArray[4]
				});
				fs.writeFile("nutritionHistory.json", JSON.stringify(jsonArray), 'utf8', function(err){
					if (err){
						console.log(err);
					}
				});
			}
			// Initial the all nutrition array for the new day
			nutrientsArray = [0,0,0,0,0];
			milkNutrientsArray = [0,0,0,0,0];
			orangeNutrientsArray = [0,0,0,0,0];
			meatNutrientsArray = [0,0,0,0,0];
			broccoliNutrientsArray = [0,0,0,0,0];
			fishNutrientsArray = [0,0,0,0,0];
			virtualNutrition =[0,0,0,0,0];
		});
	}
}, 1000);


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

	// Milk selection status from touch sensor
	socket.on('milk', function (data) {
		console.log("Milk touch sensor is touched " + data);
		// Change selection status and emit corresponding messgae
		if(milkSelected == true){
			milkSelected = false;
			io.sockets.emit('milkIsDeselected', false);	
		}
		else{
			milkSelected = true;
			milkHistoryJson = parseHistory("milkHistory.json");
			// for(var i =0; i< Object.keys(milkHistoryJson).length; i++){
			// 	console.log(milkHistoryJson[i].date);
			// 	console.log(milkHistoryJson[i].sumConsumeWeight);
			// }
			io.sockets.emit('milkIsSelected', milkHistoryJson, milkNutrientsArray, orangeNutrientsArray, meatNutrientsArray, broccoliNutrientsArray, fishNutrientsArray);
		}	
	});

	// Orange selection status from touch sensor
	socket.on('orange', function (data) {
		// console.log("Orange touch sensor is touched " + data);
		if(orangeSelected == true){
			orangeSelected = false;
			io.sockets.emit('orangeIsDeselected', false);	
		}
		else{
			orangeSelected = true;
			orangeHistoryJson = parseHistory("orangeHistory.json");
			io.sockets.emit('orangeIsSelected', orangeHistoryJson, milkNutrientsArray, orangeNutrientsArray, meatNutrientsArray, broccoliNutrientsArray, fishNutrientsArray);
		}
		
	});

	// Meat selection status from touch sensor
	socket.on('meat', function (data) {
		// console.log("Meat touch sensor is touched " + data);
		if(meatSelected == true){
			meatSelected = false;
			io.sockets.emit('meatIsDeselected', false);
		}
		else{
			meatSelected = true;
			meatHistoryJson = parseHistory("meatHistory.json");
			io.sockets.emit('meatIsSelected', meatHistoryJson, milkNutrientsArray, orangeNutrientsArray, meatNutrientsArray, broccoliNutrientsArray, fishNutrientsArray);	
		}
		
	});

	// Broccoli selection status from touch sensor
	socket.on('broccoli', function (data) {
		// console.log("Broccoli touch sensor is touched " + data);
		if(broccoliSelected == true){
			broccoliSelected = false;
			io.sockets.emit('broccoliIsDeselected', false);
		}
		else{
			broccoliSelected = true;
			broccoliHistoryJson = parseHistory("broccoliHistory.json");
			io.sockets.emit('broccoliIsSelected', broccoliHistoryJson, milkNutrientsArray, orangeNutrientsArray, meatNutrientsArray, broccoliNutrientsArray, fishNutrientsArray);
		}
		
	});

	// Fish selection status from touch sensor
	socket.on('fish', function (data) {
		// console.log("Fish touch sensor is touched " + data);
		if(fishSelected == true){
			fishSelected = false;
			io.sockets.emit('fishIsDeselected', false);
		}
		else{
			fishSelected = true;
			fishHistoryJson = parseHistory("fishHistory.json");
			io.sockets.emit('fishIsSelected', fishHistoryJson, milkNutrientsArray, orangeNutrientsArray, meatNutrientsArray, broccoliNutrientsArray, fishNutrientsArray);
		}
	});

	// Milk weight from scale
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
			addToHistoryandToJson(milkHistory, data, "milkHistory.json");
			milkIni = false;
			calculateNutrients(milkHistory, "milk");
			io.sockets.emit('nutritionChanges', nutrientsArray);	
		}
		// Check data and historical data
		else{
			if( data < 3.00){
				// Prevent to emit and record multiple zeros if the previous weight is zero
				if(!checkZeroData(milkHistory, data)){
					data = 0.00;
					io.sockets.emit('scaleDataMilk', data);
					addToHistoryandToJson(milkHistory, data, "milkHistory.json");	
					console.log("Emit milk data:  " + data);
					calculateNutrients(milkHistory, "milk");
					io.sockets.emit('nutritionChanges', nutrientsArray);	
				}
			}	
			else{
				// If the new weight changes more than 1g, then emit new weight and add to history
				if(checkData(milkHistory, data)){
					io.sockets.emit('scaleDataMilk', data);	
					addToHistoryandToJson(milkHistory, data, "milkHistory.json");
					console.log("Emit milk data: " + data);
					calculateNutrients(milkHistory, "milk");
					io.sockets.emit('nutritionChanges', nutrientsArray);
				}	
			}
		} 
	});

	// Orange weight from scale
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
			addToHistoryandToJson(orangeHistory, data, "orangeHistory.json");
			orangeIni = false;
			calculateNutrients(orangeHistory, "orange");
			io.sockets.emit('nutritionChanges', nutrientsArray);
		}
		// Check data and historical data
		else{
			if( data < 3.00){
				// Prevent to emit and record multiple zeros if the previous weight is zero
				if(!checkZeroData(orangeHistory, data)){
					data = 0.00;
					io.sockets.emit('scaleDataOrange', data);	
					console.log("Emit orange data:  " + data);
					addToHistoryandToJson(orangeHistory, data, "orangeHistory.json");
					calculateNutrients(orangeHistory, "orange");
					io.sockets.emit('nutritionChanges', nutrientsArray);
				}
			}	
			else{
				if(checkData(orangeHistory, data)){
					io.sockets.emit('scaleDataOrange', data);	
					console.log("Emit orange data: " + data);
					addToHistoryandToJson(orangeHistory, data, "orangeHistory.json");
					calculateNutrients(orangeHistory, "orange");
					io.sockets.emit('nutritionChanges', nutrientsArray);
				}	
			}
		}		
	});

	// Meat weight from scale
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
			addToHistoryandToJson(meatHistory, data, "meatHistory.json");
			meatIni = false;
			calculateNutrients(meatHistory, "meat");
			io.sockets.emit('nutritionChanges', nutrientsArray);
		}
		// Check data and historical data
		else{
			if( data < 3.00){
				// Prevent to emit and record multiple zeros if the previous weight is zero
				if(!checkZeroData(meatHistory, data)){
					data = 0.00;
					io.sockets.emit('scaleDataMeat', data);	
					console.log("Emit meat data:  " + data);
					addToHistoryandToJson(meatHistory, data, "meatHistory.json");
					calculateNutrients(meatHistory, "meat");
					io.sockets.emit('nutritionChanges', nutrientsArray);
				}
			}	
			else{
				if(checkData(meatHistory, data)){
					io.sockets.emit('scaleDataMeat', data);	
					console.log("Emit meat data: " + data);
					addToHistoryandToJson(meatHistory, data, "meatHistory.json");
					calculateNutrients(meatHistory, "meat");
					io.sockets.emit('nutritionChanges', nutrientsArray);
				}	
			}
		}
	});

	// Broccoli weight from scale
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
			addToHistoryandToJson(broccoliHistory, data, "broccoliHistory.json");
			broccoliIni = false;
			calculateNutrients(broccoliHistory, "broccoli");
			io.sockets.emit('nutritionChanges', nutrientsArray);
		}
		// Check data and historical data
		else{
			if( data < 3.00){
				// Prevent to emit and record multiple zeros if the previous weight is zero
				if(!checkZeroData(broccoliHistory, data)){
					data = 0.00;
					io.sockets.emit('scaleDataBroccoli', data);	
					console.log("Emit broccoli data:  " + data);
					addToHistoryandToJson(broccoliHistory, data, "broccoliHistory.json");
					calculateNutrients(broccoliHistory, "broccoli");
					io.sockets.emit('nutritionChanges', nutrientsArray);
				}
			}	
			else{
				if(checkData(broccoliHistory, data)){
					io.sockets.emit('scaleDataBroccoli', data);	
					console.log("Emit broccoli data: " + data);
					addToHistoryandToJson(broccoliHistory, data, "broccoliHistory.json");
					calculateNutrients(broccoliHistory, "broccoli");
					io.sockets.emit('nutritionChanges', nutrientsArray);
				}	
			}
		}	
	});

	// Fish weight from scale
	socket.on('fishWeight', function (data) {
		console.log("fish weight: " + data);
		// First time, check and emit data, and save to history
		if(fishIni === true){
			if(data < 3.00){
				data = 0.00;
				io.sockets.emit('scaleDataFish', data);	
				console.log("Emit fish data:  " + data);
			}
			else{
				io.sockets.emit('scaleDataFish', data);	
				console.log("Emit fish data: " + data);
			}
			addToHistoryandToJson(fishHistory, data, "fishHistory.json");
			fishIni = false;
			calculateNutrients(fishHistory, "fish");
			io.sockets.emit('nutritionChanges', nutrientsArray);
		}
		// Check data and historical data
		else{
			if( data < 3.00){
				// Prevent to emit and record multiple zeros if the previous weight is zero
				if(!checkZeroData(fishHistory, data)){
					data = 0.00;
					io.sockets.emit('scaleDataFish', data);	
					console.log("Emit fish data:  " + data);
					addToHistoryandToJson(fishHistory, data, "fishHistory.json");
					calculateNutrients(fishHistory, "fish");
					io.sockets.emit('nutritionChanges', nutrientsArray);
				}
			}	
			else{
				if(checkData(fishHistory, data)){
					io.sockets.emit('scaleDataFish', data);	
					console.log("Emit fish data: " + data);
					addToHistoryandToJson(fishHistory, data, "fishHistory.json");
					calculateNutrients(fishHistory, "fish");
					io.sockets.emit('nutritionChanges', nutrientsArray);
				}	
			}
		}	
	});

	// Get new milk weight by moving the rack manually
	// Emit manual data to move the nutrition servos and AR text of the weight
	socket.on('milkWeightByMovingRack', function (data) {
		// Get the virtual food consumption amount
		var virtualConsumption = milkHistory[Object.keys(historicalObject).length-1].weight - data;
		console.log("milk weight from moving the rack: " + data + "Virtual consumption: " + virtualConsumption);
		calculateNewVirtualNutrition(virtualConsumption, milkNutrients);	
		io.sockets.emit('manualDataMilk', data, virtualNutrition);	
	});

	// Get orange weight by moving the rack manually
	socket.on('orangeWeightByMovingRack', function (data) {
		var virtualConsumption = orangeHistory[Object.keys(historicalObject).length-1].weight - data;
		console.log("orange weight from moving the rack: " + data);
		calculateNewVirtualNutrition(virtualConsumption, orangeNutrients);
		io.sockets.emit('manualDataOrange', data, virtualNutrition);	
	});

	// Get meat weight by moving the rack manually
	socket.on('meatWeightByMovingRack', function (data) {
		var virtualConsumption = meatHistory[Object.keys(historicalObject).length-1].weight - data;
		console.log("meat weight from moving the rack: " + data);
		calculateNewVirtualNutrition(virtualConsumption, meatNutrients);
		io.sockets.emit('manualDataMeat', data, virtualNutrition);	
	});

	// Get broccoli weight by moving the rack manually
	socket.on('broccoliWeightByMovingRack', function (data) {
		var virtualConsumption = broccoliHistory[Object.keys(historicalObject).length-1].weight - data;
		console.log("broccoli weight from moving the rack: " + data);
		calculateNewVirtualNutrition(virtualConsumption, broccoliNutrients);
		io.sockets.emit('manualDataBroccoli', data, virtualNutrition);	
	});

	// Get fish weight by moving the rack manually
	socket.on('fishWeightByMovingRack', function (data) {
		var virtualConsumption = fishHistory[Object.keys(historicalObject).length-1].weight - data;
		console.log("fish weight from moving the rack: " + data);
		calculateNewVirtualNutrition(virtualConsumption,fishNutrients);
		io.sockets.emit('manualDataFish', data, virtualNutrition);	
	});

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


// Calculate food consumption
// Assumption: never consume all the food. 
// Self-report food is finished or new food is added, maybe by pressing finish and new buttons.
// Because when the weight is 0, hard to distinguish if the food is finished or just be taken away and will be returned later
function calculateConsumption(historicalObject, weight){
	var consumeWeight = 0;
	if(weight == 0){
		consumeWeight = 0;
	}
	else{
		// The first weight number
		if(Object.keys(historicalObject).length == 0 ){
			consumeWeight = 0;
		}
		// Has two weight numbers
		if(Object.keys(historicalObject).length == 1){
			var previousWeight = historicalObject[Object.keys(historicalObject).length-1].weight;
			// New add food
			if(previousWeight == 0){
				consumeWeight = 0;
			}
			else{
				consumeWeight = previousWeight - weight;
			}		
		}
		// Has at least three numbers
		if(Object.keys(historicalObject).length >= 2 ){
			var previousWeight = historicalObject[Object.keys(historicalObject).length-1].weight;
			// The previous number is 0
			if(previousWeight == 0){
				// Get the the number of the previous number
				var previousTwoWeight = historicalObject[Object.keys(historicalObject).length-2].weight;
				consumeWeight = previousTwoWeight - weight;
			}
			else{
				consumeWeight = previousWeight - weight;
			}
		}		
	}
	return consumeWeight;
}

//Add to history object and to Json file
function addToHistoryandToJson(historicalObject, weight, jsonFileName){
	// Add the new data to history
	var currentDate = new Date();
	var date = currentDate.toString();
	var time = currentDate/1000;
	var consumeWeight = calculateConsumption(historicalObject, weight);
	historicalObject.push({
		date: date,
		time: time,
		weight: weight,
		consumeWeight: consumeWeight
	})

	fs.readFile(jsonFileName, 'utf8', function readFileCallback(err, data){
		if (err){
			console.log("Read " + jsonFileName + " file err:" + err);
		} else{
			jsonArray = JSON.parse(data);
			jsonArray.push({
				date: date,
				time: time,
				weight: weight,
				consumeWeight: consumeWeight
			});
			fs.writeFile(jsonFileName, JSON.stringify(jsonArray), 'utf8', function(err){
				if (err){
					console.log(err);
				}
			});
		}	
	});
}

// Store last five days date and consumption date in an object, and transfer the object to AR histroty view 
function parseHistory(jsonFileName){
	var fiveDaysJsonData = [];
	var historyData = fs.readFileSync(jsonFileName, 'utf8');
	var jsonData = JSON.parse(historyData);
	// console.log(jsonData);
	for(j = 4; j > -1 ; j --){
		var date = new Date();
		date.setDate(date.getDate()-j);
		date = date.toString().substring(4,10);
		var sumConsumeWeight = 0;
		for(i = 0; i < jsonData.length; ++i){
			if(jsonData[i].date.toString().substring(4,10) === date){
				sumConsumeWeight = sumConsumeWeight + jsonData[i].consumeWeight;
			}
		}
		fiveDaysJsonData.push({
			date: date,
			sumConsumeWeight: sumConsumeWeight
		})
	}
	return fiveDaysJsonData;
}

// Calculate accumulate nutrients value of same day when food weight changes
function calculateNutrients(historicalObject, foodType){
	var newConsumption = historicalObject[Object.keys(historicalObject).length - 1].consumeWeight;
	var consumptionPer = newConsumption/100;
	switch(foodType) {
		case "milk":
		for( i = 0; i < nutrientsArray.length ; i ++ ){
			nutrientsArray[i] += consumptionPer * milkNutrients[i];
			milkNutrientsArray[i] += consumptionPer * milkNutrients[i];
		}
		break;
		case "orange":
		for( i = 0; i < nutrientsArray.length ; i ++ ){
			nutrientsArray[i] += consumptionPer * orangeNutrients[i];
			orangeNutrientsArray[i] += consumptionPer * orangeNutrients[i];
		}
		break;
		case "meat":
		for( i = 0; i < nutrientsArray.length ; i ++ ){
			nutrientsArray[i] += consumptionPer * meatNutrients[i];
			meatNutrientsArray[i] += consumptionPer * meatNutrients[i];
		}
		break;
		case "broccoli":
		for( i = 0; i < nutrientsArray.length ; i ++ ){
			nutrientsArray[i] += consumptionPer * broccoliNutrients[i];
			broccoliNutrientsArray[i] += consumptionPer * broccoliNutrients[i];
		}
		break;
		case "fish":
		for( i = 0; i < nutrientsArray.length ; i ++ ){
			nutrientsArray[i] += consumptionPer * fishNutrients[i];
			fishNutrientsArray[i] += consumptionPer * fishNutrients[i];
		}
		break;
	}
	console.log(nutrientsArray);
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

function calculateNewVirtualNutrition(virtualConsumption, nutrientsRate){
	var virtualConsumptionPer = virtualConsumption/100;
	if(!virtualFlag){
		virtualFlag = true;	
		for(i=0; i< virtualNutrition.length; i++){
			// Get current real nutrients amount, then add or substract virtual nutrients
			virtualNutrition[i] = nutrientsArray[i];
			virtualNutrition[i] += virtualConsumptionPer * nutrientsRate[i];			
		}
	}
	else{
		for(i=0; i< virtualNutrition.length; i++){
			virtualNutrition[i] += virtualConsumptionPer * nutrientsRate[i];	
		}
	}
}
