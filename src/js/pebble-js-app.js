Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
});

Pebble.addEventListener('showConfiguration', function(e) {
  var url = 'http://192.168.43.158:8000/config/index.html?options=batteryPercentage+showDate';
  console.log('Showing configuration page: ' + url);
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function(e) {
  var config = JSON.parse(decodeURIComponent(e.response));
  console.log('Config window returned: ' + JSON.stringify(config));

  var data = {
    'KEY_BATTERY_PERCENTAGE': config.batteryPercentage ? 1 : 0,
    'KEY_SHOW_DATE': config.showDate ? 1 : 0
  };

  Pebble.sendAppMessage(data, function() {
    console.log('Sent config data to Pebble');
  }, function() {
    console.log('Failed to send config data!');
  });
});
