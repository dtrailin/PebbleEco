var xhrRequest = function (url, type, callback) {
  var xhr = new XMLHttpRequest();
  xhr.onload = function () {
    callback(this.responseText);
  };
  xhr.open(type, url);
  xhr.send();
};

function locationSuccess(pos) {
  // Construct URL
  var url = "http://www.airnowapi.org/aq/observation/latLong/current/?format=application/json&latitude=" + pos.coords.latitude + "&longitude=" + 
      pos.coords.longitude +"&distance=100&API_KEY=89116BB2-7154-4A28-BB40-F3525D0C66BC" ;
  
 console.log(url);

  // Send request to OpenWeatherMap
  xhrRequest(url, 'GET', 
    function(responseText) {
      // responseText contains a JSON object with weather info
      var json = JSON.parse(responseText);

      var AQI = json[0].AQI;
      console.log("AQI is " + AQI);
      var city = json[0].ReportingArea;
       console.log("City is " + city);
      var state = json[0].StateCode;
        console.log("state is " + state);
      var status = json[0].Category.Name;
      
      // Assemble dictionary using our keys
      var dictionary = {
       'KEY_AQI': AQI,
        'KEY_CITY' : city,
        'KEY_STATE' : state,
        'KEY_STATUS' : status
        
      };

      // Send to Pebble
      Pebble.sendAppMessage(dictionary,
        function(e) {
          console.log("AQI info sent to Pebble successfully!");
        },
        function(e) {
          console.log("Error sending AQI info to Pebble!");
        }
      );
    }      
  );
}

function locationError(err) {
  console.log("Error requesting location!");
}

function getAQI() {
  navigator.geolocation.getCurrentPosition(
    locationSuccess,
    locationError,
    {timeout: 15000, maximumAge: 60000}
  );
}

// Listen for when the watchface is opened
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");

    // Get the initial weather
    getAQI();
  }
);

// Listen for when an AppMessage is received
Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
    getAQI();
  }                     
);
