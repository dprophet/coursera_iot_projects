var express = require('express');
var app = express();
var iotf = require('ibmiotf');
var appConfig;

var serverPort = process.env.PORT || 3000;

if (process.env.VCAP_SERVICES) {
    var env = JSON.parse(process.env.VCAP_SERVICES);
    appConfig = {
                   'org' : env["iotf-service"][0].credentials.org,
                   'id' : 'bi-nodeserver',
                   'auth-key' : env["iotf-service"][0].credentials.apiKey,
                   'auth-token' : env["iotf-service"][0].credentials.apiToken
                  }
} else {
    appConfig = require('./application.json');
}

var responseString = 'Hello Coursera';

var appClient = new iotf.IotfApplication(appConfig);
app.get('/', function(req, res) {
    res.send(responseString);
});

var server = app.listen(serverPort, function() {
    var port = server.address().port;
    console.log('Listening on port : %s', port);
    appClient.connect();
    
    appClient.on('connect', function() {
        appClient.subscribeToDeviceEvents();
    });
    
    appClient.on('deviceEvent', function(deviceType, deviceId, eventType, format, payload) {
        responseString = "Device event at " + new Date().toString() + " from " + deviceType +
                          ":" + deviceId + "; event = "+ eventType +", payload = " + payload;
    });

});

