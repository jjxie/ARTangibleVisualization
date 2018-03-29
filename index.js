var app = require('express')();
var http = require('http').Server(app);
var io = require('socket.io')(http);
var fs = require('fs');
var connectedIDArray = [];

app.get('/', function(req, res){
	res.sendFile(__dirname + '/index.html');
});


io.on('connection', function(socket){
	console.log('a user connected');
	console.log(socket.id);
	connectedIDArray.push(socket.id);
	console.log("Connected array length: " + connectedIDArray.length); 

	// Send message
	socket.on('chat message', function(msg){
		console.log('message: ' + msg);
		// send to all
		io.emit('chat message', socket.id + " : " + msg);
		// send to others except the sender
		// socket.broadcast.emit('chat message', msg);
	});

	// Send sensor data
	socket.on('sensor data', function(sensorData){
		//console.log('sensor data: ' + sensorData);
		// send to all
		io.emit('sensor data',  sensorData);
		// send to others except the sender
		// socket.broadcast.emit('chat message', msg);
	});

	// Send sensor data 2
	socket.on('sensor data2', function(sensorData){
		//console.log('sensor data2: ' + sensorData);
		// send to all
		io.emit('sensor data2',  sensorData);
		// send to others except the sender
		// socket.broadcast.emit('chat message', msg);
	});

	// Send sensor data 3
	socket.on('sensor data3', function(sensorData){
		//console.log('sensor data2: ' + sensorData);
		// send to all
		io.emit('sensor data3',  sensorData);
		// send to others except the sender
		// socket.broadcast.emit('chat message', msg);
	});

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
		console.log("Connected array length: " + connectedIDArray.length); 
		// Offline event
		io.emit('offline', socket.id);
	});
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

http.listen(3000, function(){
	console.log('listening on *:3000');
});
