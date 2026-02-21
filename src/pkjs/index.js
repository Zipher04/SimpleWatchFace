var myAPIKey = 'YOUR_OPENWEATHERMAP_API_KEY';

function descFromId(id) {
    if (id < 300) return 'Thunder';
    else if (id < 600) return 'Rain';
    else if (id < 700) return 'Snow';
    else if (id < 800) return 'Fog';
    else if (id == 800) return 'Clear';
    else if (id < 900) return 'Cloudy';
    else return 'Unknown';
}

function getWeather() {
    var lastUpdate = localStorage.getItem('last_weather_update');
    var now = new Date().getTime();

    // Cache for 60 minutes (3600000 ms)
    if (lastUpdate && (now - lastUpdate < 3600000)) {
        var cachedData = JSON.parse(localStorage.getItem('cached_weather'));
        if (cachedData) {
            console.log('Using cached weather data');
            Pebble.sendAppMessage(cachedData);
            return;
        }
    }

    navigator.geolocation.getCurrentPosition(
        function (pos) {
            var url = 'http://api.openweathermap.org/data/2.5/weather?lat=' +
                pos.coords.latitude + '&lon=' + pos.coords.longitude + '&appid=' + myAPIKey + '&units=metric';

            var xhr = new XMLHttpRequest();
            xhr.onload = function () {
                var json = JSON.parse(this.responseText);
                var dictionary = {
                    'TEMPERATURE': Math.round(json.main.temp),
                    'CONDITION': descFromId(json.weather[0].id)
                };

                localStorage.setItem('cached_weather', JSON.stringify(dictionary));
                localStorage.setItem('last_weather_update', now);

                Pebble.sendAppMessage(dictionary,
                    function (e) { console.log('Weather info sent to Pebble successfully!'); },
                    function (e) { console.log('Error sending weather info to Pebble!'); }
                );
            };
            xhr.open('GET', url);
            xhr.send();
        },
        function (err) { console.log('Error requesting location!'); },
        { timeout: 15000, maximumAge: 60000 }
    );
}

Pebble.addEventListener('ready', function (e) {
    console.log('PebbleKit JS ready!');
    getWeather();
});

Pebble.addEventListener('appmessage', function (e) {
    console.log('AppMessage received!');
    getWeather();
});
