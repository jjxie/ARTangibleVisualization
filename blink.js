var five = require("johnny-five");
// by default ESP8266 is a TCP Server so you'll need a TCP client transport for J5
var EtherPortClient = require("etherport-client").EtherPortClient;
// update host to the IP address for your ESP board
var board = new five.Board({
	port: new EtherPortClient({
		host: "192.168.2.7",
		port: 3030
	}),
	timeout: 1e5,
	repl: false
});

board.on("ready", function() {
	// It works the first time and ONLY with trun off led code
	this.pinMode(2, five.Pin.OUTPUT);
	this.digitalWrite(2, 1);
	console.log("READY!");
	var led = new five.Led(2);
	// blinks the blue LED on a WeMos ESP8266 board
	led.blink(500);
});

// var five = require("johnny-five");
// var firmata = require("firmata");
// var EtherPortClient = require('etherport-client').EtherPortClient;
// var io = new firmata.Board(new EtherPortClient({
//     host: '192.168.2.7',
//     port: 3030
// }));

// io.once('ready', function(){
//     console.log('IO Ready');
//     io.isReady = true;

//     var board = new five.Board({io: io, repl: true});

//     board.on('ready', function(){
//         console.log('five ready');

//         var led = new five.Led(2);
//         led.blink(1000);
//     });
// });

