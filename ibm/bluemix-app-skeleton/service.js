function Service(appClient) {
  this.appClient = appClient;
}

Service.prototype.connect = function() {
  // TODO connect to iotf here with this.appClient

  this.appClient.on('connect', function() {
    // TODO hook up device events here with this.appClient
  }.bind(this));

  this.appClient.on('deviceEvent', function (deviceType, deviceId, eventType, format, payload) {
    // TODO act on device events and call handleTempEvent when the right type of event arrives
  }.bind(this));
};

Service.prototype.handleTempEvent = function(temp) {
  // TODO handle temperature changes here and call this.warningOn/this.warningOff accordingly.
};

Service.prototype.warningOn = function() {
  // TODO send a device commmand here
  // warningOn should only be called when the warning isn't already on
};

Service.prototype.warningOff = function() {
  // TODO send a device commmand here
  // warningOff should only be called when the warning isn't already off
};

module.exports = Service;
