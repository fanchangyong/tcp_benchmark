var cluster = require('cluster');
var net = require('net');

net.createServer(function(c){
	c.on('data',function(data){
	})
	c.on('end',function(){
		c.end();
	});
}).listen(8888);
