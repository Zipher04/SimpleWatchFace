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
    var myAPIKey = localStorage.getItem('weather_api_key');
    if (!myAPIKey) {
        console.log('No API Key configured!');
        Pebble.sendAppMessage({
            'TEMPERATURE': 0,
            'CONDITION': ''
        });
        return;
    }

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
                if (json && json.weather) {
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
                } else {
                    console.log('Error: Invalid response from weather API');
                }
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

Pebble.addEventListener('showConfiguration', function (e) {
    var apiKey = localStorage.getItem('weather_api_key') || '';
    var quietStart = localStorage.getItem('quiet_start') || '22';
    var quietEnd = localStorage.getItem('quiet_end') || '8';

    var config = {
        apiKey: apiKey,
        quietStart: parseInt(quietStart),
        quietEnd: parseInt(quietEnd)
    };

    var html = '<!DOCTYPE html><html><head><title>Simple Watch Face Settings</title><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1"><link rel="stylesheet" href="https://code.getmdl.io/1.3.0/material.indigo-pink.min.css"><style>body { padding: 20px; background-color: #fafafa; } .card { max-width: 400px; margin: auto; padding: 20px; background: white; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.1); } h4 { margin-top: 0; color: #3f51b5; } .mdl-textfield { width: 100%; } .actions { margin-top: 20px; text-align: right; } label { font-weight: bold; display: block; margin-top: 15px; }</style></head><body><div class="card"><h4>Watchface Settings</h4><p>Weather API Key:</p><div class="mdl-textfield mdl-js-textfield"><input class="mdl-textfield__input" type="text" id="api-key"><label class="mdl-textfield__label" for="api-key">OpenWeatherMap API Key...</label></div><hr><p>Vibration Quiet Hours (0-23):</p><label for="quiet-start">Start Hour:</label><input class="mdl-slider mdl-js-slider" type="range" id="quiet-start" min="0" max="23" step="1"><span id="start-val">22</span><label for="quiet-end">End Hour:</label><input class="mdl-slider mdl-js-slider" type="range" id="quiet-end" min="0" max="23" step="1"><span id="end-val">8</span><div class="actions"><button id="save-button" class="mdl-button mdl-js-button mdl-button--raised mdl-button--colored">Save Settings</button></div></div><script>var config = JSON.parse(decodeURIComponent(window.location.hash.substring(1))); document.getElementById("api-key").value = config.apiKey; document.getElementById("quiet-start").value = config.quietStart; document.getElementById("start-val").innerText = config.quietStart; document.getElementById("quiet-end").value = config.quietEnd; document.getElementById("end-val").innerText = config.quietEnd; document.getElementById("quiet-start").addEventListener("input", function(e) { document.getElementById("start-val").innerText = e.target.value; }); document.getElementById("quiet-end").addEventListener("input", function(e) { document.getElementById("end-val").innerText = e.target.value; }); document.getElementById("save-button").addEventListener("click", function() { var key = document.getElementById("api-key").value; var start = document.getElementById("quiet-start").value; var end = document.getElementById("quiet-end").value; var result = { apiKey: key, quietStart: parseInt(start), quietEnd: parseInt(end) }; window.location.href = "pebblejs://close#" + encodeURIComponent(JSON.stringify(result)); });</script></body></html>';
    Pebble.openURL('data:text/html,' + encodeURIComponent(html) + '#' + encodeURIComponent(JSON.stringify(config)));
});

Pebble.addEventListener('webviewclosed', function (e) {
    if (e.response) {
        var config = JSON.parse(decodeURIComponent(e.response));
        if (config) {
            if (config.apiKey) {
                localStorage.setItem('weather_api_key', config.apiKey);
                localStorage.removeItem('last_weather_update');
            }
            localStorage.setItem('quiet_start', config.quietStart);
            localStorage.setItem('quiet_end', config.quietEnd);

            var dictionary = {
                'QUIET_START': config.quietStart,
                'QUIET_END': config.quietEnd
            };

            Pebble.sendAppMessage(dictionary, function () {
                console.log('Quiet hours sent to Pebble successfully!');
                getWeather(); // Fetch weather after updating settings
            }, function () {
                console.log('Error sending settings to Pebble!');
            });
        }
    }
});
