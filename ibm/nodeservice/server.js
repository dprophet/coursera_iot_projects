var express = require("express");
var app = express();  

var serverPort = 3000;
var serverHost = 'localhost';

app.get('/', function(req, res) {
    res.send('Hello Coursera');
});

var server = app.listen(serverPort, serverHost, function() {
    var host = server.address().address;
    var port = server.address().port;
    console.log('Listening at http://%s:%s', host, port);
});

