(function() {
  loadOptions();
  submitHandler();
})();

function submitHandler() {
  var submitButton = document.getElementById('submitButton');

  submitButton.on('click', function() {
    console.log('Submit');

    var return_to = getQueryParam('return_to', 'pebblejs://close#');
    document.location = return_to + encodeURIComponent(JSON.stringify(getAndStoreConfigData()));
  });
}

function loadOptions() {
  var hourlyVibrateCheckbox = document.getElementById('hourlyVibrateCheckbox');
  var timeFormatCheckbox = document.getElementById('timeFormatCheckbox');

  if (localStorage.hourlyVibrate) {
    hourlyVibrateCheckbox[0].checked = JSON.parse(localStorage.hourlyVibrate);
    timeFormatCheckbox[0].checked = JSON.parse(localStorage.twentyFourHourFormat);
  }
}

function getAndStoreConfigData() {
  var hourlyVibrateCheckbox = document.getElementById('hourlyVibrateCheckbox');
  var timeFormatCheckbox = document.getElementById('timeFormatCheckbox');

  var options = {
    hourlyVibrate: hourlyVibrateCheckbox.checked,
    twentyFourHourFormat: timeFormatCheckbox[0].checked
  };

  localStorage.hourlyVibrate = options.hourlyVibrate;
  localStorage.twentyFourHourFormat = options.twentyFourHourFormat;

  console.log('Got options: ' + JSON.stringify(options));
  return options;
}

function getQueryParam(variable, defaultValue) {
  var query = location.search.substring(1);
  var vars = query.split('&');
  for (var i = 0; i < vars.length; i++) {
    var pair = vars[i].split('=');
    if (pair[0] === variable) {
      return decodeURIComponent(pair[1]);
    }
  }
  return defaultValue || false;
}
