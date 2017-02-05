var iotf = require('ibmiotf');

// TODO create your iotf application client here
var appClientConfig // = ... ;
var appClient // = ...;

var Service = require('./service');

// Binding to a port ensures that the service is kept alive.
// We use express for that and implement a /status endpoint.
var express = require('express');
var cfenv = require('cfenv');
var cfappEnv = cfenv.getAppEnv();
var app = express();

app.get('/status', function(req, res) {
  res.status(200).json({ message: 'iot service running' });
});

module.exports = app.listen(cfappEnv.port, function() {
  console.log('server started on ' + cfappEnv.url);
  var service = new Service(appClient);
  // TODO use service to connect here
});
