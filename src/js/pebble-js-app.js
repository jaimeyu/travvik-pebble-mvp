/*
* The MIT License (MIT)
* 
* Copyright (c) 2014 Jaime Yu
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
* THE SOFTWARE.
* 
*/

// Fetch last known query

var lastRoute = localStorage.getItem("lastBus");
var lastStation = localStorage.getItem("lastStop");
var lastDirection = localStorage.getItem("lastDirection");
if (!lastRoute) {
    lastRoute = 99;
}

if (!lastStation) {
    lastStation = 3038;
}

if (!lastDirection) {
    lastDirection = 0;
}

if (lastDirection === -1) {
    lastDirection = 0;
}



function fetchWeather(route, station, direction) {
    var response;
    var req = new XMLHttpRequest();
    console.log("Fetching transit data for " + route, station);
    console.log("http://ottawa.travvik.com/v3/json.php?" +
            "routeno=" + route + "&stopno=" + station + "&src=pebble");
    req.open('GET', "http://ottawa.travvik.com/json.php?" +
            "routeno=" + route + "&stopno=" + station + "&src=pebble", true);
    req.onload = function(e) {
        if (req.readyState === 4) {
            if (req.status === 200) {
                console.log(req.responseText);
                response = JSON.parse(req.responseText);
                var temperature, icon, city;
                var number, stopLabel, destination, arrival;
                //console.log("RSPOSNE" + response);
                //if (response > 0) 
                {




                    console.log("***SUCCESS in getting data!");
                    var arrival0 = null, dst0 = null, stoplabel = null;

                    console.log("Executing direction: " + direction);

                    try {
                        arrival0 = response.GetNextTripsForStopResponse.GetNextTripsForStopResult.Route.RouteDirection[direction].Trips.Trip.AdjustedScheduleTime;
                        dst0 = response.GetNextTripsForStopResponse.GetNextTripsForStopResult.Route.RouteDirection[direction].Trips.Trip.TripDestination;
                    }
                    catch (e) {
                        console.log(e);
                    }

                    try {
                        arrival0 = response.GetNextTripsForStopResponse.GetNextTripsForStopResult.Route.RouteDirection.Trips.Trip.AdjustedScheduleTime;
                        dst0 = response.GetNextTripsForStopResponse.GetNextTripsForStopResult.Route.RouteDirection.Trips.Trip.TripDestination;
                    }
                    catch (e) {
                        console.log(e);
                    }

                    try {
                        arrival0 = response.GetNextTripsForStopResponse.GetNextTripsForStopResult.Route.RouteDirection[direction].Trips.Trip[0].AdjustedScheduleTime;
                        dst0 = response.GetNextTripsForStopResponse.GetNextTripsForStopResult.Route.RouteDirection[direction].Trips.Trip[0].TripDestination;
                    }
                    catch (e) {
                        console.log(e);
                    }

                    try {
                        arrival0 = response.GetNextTripsForStopResponse.GetNextTripsForStopResult.Route.RouteDirection.Trips.Trip[0].AdjustedScheduleTime;
                        dst0 = response.GetNextTripsForStopResponse.GetNextTripsForStopResult.Route.RouteDirection.Trips.Trip[0].TripDestination;
                    }
                    catch (e) {
                        console.log(e);
                    }

                    if (arrival0 === null) {
                        Pebble.sendAppMessage({
                            "arrival": 0,
                            "origin": "No data",
                            "destination": "be found",
                            "direction": direction,
                            "busnb": parseInt(route, 10),
                            "stopnb": parseInt(station, 10)
                        });
                    }

                    console.log("Arrival in " + arrival0);
                    console.log("Arrival in " + parseInt(arrival0, 10));

                    stoplabel = response.GetNextTripsForStopResponse.GetNextTripsForStopResult.StopLabel;
                    //arrival0 = arrival0.concat(" mins");


                    Pebble.sendAppMessage({
                        "arrival": parseInt(arrival0, 10),
                        "origin": stoplabel,
                        "destination": dst0,
                        "direction": direction,
                        "busnb": parseInt(route, 10),
                        "stopnb": parseInt(station, 10)
                    });
                    // No error detector, save the values.
                    localStorage.setItem("lastStop", station);
                    localStorage.setItem("lastBus", route);
                    console.log("Sent data to pebble");
                }

            } else {
                console.log("Error");
            }
        }
    };
    req.send(null);
}

function locationSuccess(pos) {
    var coordinates = pos.coords;
    console.log("Running locationSuccess");
    //fetchWeather(coordinates.latitude, coordinates.longitude);
}

function locationError(err) {
    console.warn('location error (' + err.code + '): ' + err.message);
}

function sendErrorMessage() {
    console.log("SendError");
    Pebble.sendAppMessage({
        "arrival": "Error",
        "origin": "Fail",
        "destination": "connect",
        "direction": lastDirection,
        "busnb": parseInt(lastRoute, 10),
        "stopnb": parseInt(lastStation, 10)
    });
}

var locationOptions = {"timeout": 15000, "maximumAge": 60000};
Pebble.addEventListener("ready",
        function(e) {
            console.log("connect!" + e.ready);
            console.log("last known values: " + lastRoute + " " + lastStation);
            fetchWeather(lastRoute, lastStation, 0);
            //locationWatcher = window.navigator.geolocation.watchPosition(locationSuccess, locationError, locationOptions);
            console.log(e.type);
        });
Pebble.addEventListener("appmessage",
        function(e) {
            var bus = null;
            var stop = null;
            var varx = null;
            var direction = null;
            /**enum REQUEST_KEYS {
             REQ_BUS_NB,
             REQ_STOP_NB,
             REQ_NONE,
             REQ_DIRECTION,
             };**/
            console.log("appmessage!");
            if (e.payload.arrival) {
                varx = e.payload.arrival;
                console.log("Payload lastBus: " + varx);
            }
            if (e.payload.origin) {
                varx = e.payload.origin;
                console.log("Payload lastStop: " + varx);
            }

            if (e.payload.direction) {
                varx = e.payload.direction;
                direction = parseInt(varx,10);
                console.log("Payload direction: " + direction);
            }

            if (e.payload.busnb) {
                bus = e.payload.busnb;
                console.log("Payload busnb: " + bus);
            }
            if (e.payload.stopnb) {
                stop = e.payload.stopnb;
                console.log("Payload stopnb: " + stop);
            }

            if (bus === -1 && stop === -1) {
                bus = lastRoute;
                stop = lastStation;
                console.log("Initial conditions");
            }
            
            if (direction === null){
                direction = 0;
            } else if (direction === -1){
                direction = lastDirection;
            }

            if (bus !== null && stop !== null) {

                console.log("Direction is " + direction);

                fetchWeather(bus, stop, direction);
//                switch (direction) {
//                    case 0:
//                        direction = 1;
//                        break;
//                    case 1:
//                        direction = 0;
//                        break;
//                    default:
//                        direction = 0;
//                        break;
//                }
                localStorage.setItem("lastDirection", direction);
            }
            else
            {
                console.log("Could not parse incoming data.");
                sendErrorMessage();
            }

            console.log("message!");
        });
Pebble.addEventListener("webviewclosed",
        function(e) {
            console.log("webview closed");
            console.log(e.type);
            console.log(e.response);
        });


