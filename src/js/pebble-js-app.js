var LOGGING = false;

var TESTINGGPSDATA = false;

var jobs = 0;
var lastStatus;
var sendScore = 0;
var sendLocation;
var sendGrade;
var sendCrimes;

//probably pass these in from c for security
var msKey;
var lat;
var lon;

function send(data) 
{
	//Pebble.sendAppMessage(data);
	setTimeout(function() 
	{
		Pebble.sendAppMessage(data);
		jobs--;
	}, jobs++*420);
}

function successCallback(position) 
{
	if (LOGGING) console.log (JSON.stringify(position));
	
	if (position)
	{
		if (LOGGING) console.log("lat: " + position.coords.latitude + " | long:" + position.coords.longitude);
		if (LOGGING) console.log("accuracy: " + position.coords.accuracy);
		lat =  position.coords.latitude;
		lon = position.coords.longitude;
		//send ({"GPSFix" : 1});
	}
}

function errorCallback(error) {
	if (LOGGING) console.log("geo error code: " + error.code);
	if (LOGGING) console.log("geo error message: " + error.message);
	send ({"GPSFix" : 0});
}

function getScoreData()
{
	if(LOGGING) console.log("Entered: getScoreData()");
	navigator.geolocation.getCurrentPosition(successCallback,
                                             errorCallback,
                                             {maximumAge:600000});
	if (LOGGING) console.log(msKey);
	
	if (!msKey)
	{
		//do simple notification and show config screen again
		if(LOGGING) console.log("no key found");
		//send({"Key" : 0});
		return;
	}
	else
	{
		if (LOGGING) console.log("posting with key: " + msKey);
	}

	if (!lat || !lon)
	{
		if (LOGGING) console.log("no GPS fix");
		send({"Success" : 0 });
		return;
	}
	
	if (TESTINGGPSDATA)
	{
		lat = 38.08809;
		lon = -85.679626;
	}
	
	var getlink = "https://crimescore.p.mashape.com/crimescore?lat=" + lat + "&lon=" + lon + "&f=json";
	
	if (LOGGING) console.log(getlink);

	var req = new XMLHttpRequest();
	req.open("GET", getlink, true);
	req.setRequestHeader("X-Mashape-Authorization", msKey);
	
	if(LOGGING) console.log("Sending score request");
	
	req.onload = function(e) {
		if (req.readyState == 4)
		{
			if(LOGGING) console.log("good readystate");
			if(req.status == 200)
			{
				if(LOGGING) console.log("good status");
				if(LOGGING) console.log(req.responseText);

				lastStatus = JSON.parse(req.responseText);
				if (lastStatus.found == 1 && lastStatus.message == "OK")
				{
					sendScore = lastStatus.score;
					sendLocation = lastStatus.location;//.split(",")[0];
					
					if (LOGGING) console.log("location type: " + typeof(sendLocation));
					
					if(LOGGING) console.log("sending success/score: " + parseInt(sendScore));
					if(LOGGING) console.log("sending location: " + sendLocation);
					
					sendGrade = lastStatus.gradedetail;
					sendCrimes = lastStatus.density;
					
					send({	"Success": 1,
							"Score" : parseInt(sendScore),
							"Location" : sendLocation,
							"Grade" : sendGrade,
							"Crimes" : parseInt(sendCrimes) });
				}
				else
				{
					if(LOGGING) console.log("sending FAIL");
					send({"Success" : 0,
						"Message" : "NO COVERAGE"});
				}
			}
			else
			{
				if(LOGGING) console.log("Bad status: " + req.responseText);
				Pebble.showSimpleNotificationOnPebble("Error", "CrimeWatcher is having trouble logging in.");
				send({	"Success" : 0,
						"Message" : "BAD STATUS"});
			}
		}
		else
		{
			if(LOGGING) console.log("Bad readystate");
			Pebble.showSimpleNotificationOnPebble("Error", "CrimeWatcher can't find the API server.");
			send({	"Success" : 0,
					"Message" : "BAD READYSTATE"});		}
	};
	req.send(null);
}

Pebble.addEventListener("ready",
	function(e) {
		send({"MSKey" : 0}); //request the mashaper key from application - Get yours at https://www.mashape.com/yourmapper/crimescore
		navigator.geolocation.getCurrentPosition(successCallback,
                                             errorCallback,
                                             {maximumAge:600000});
});

Pebble.addEventListener("appmessage",function(e) 
{
	if(LOGGING) console.log("Received message from Pebble: " + JSON.stringify(e.payload));

	for(var prop in e.payload)
	{
		switch(prop)
		{
			case "MSKey":
				if(LOGGING) console.log("got MSKey: " + e.payload.MSKey);
				msKey = e.payload.MSKey;
				getScoreData();
				break;
			case "Refresh":
				if(LOGGING) console.log("got Refresh");
				getScoreData();
				break;
			case "Threshold":
				
				break;
			case "Alerts" : 
				
				break;
			default:
				if(LOGGING) console.log("Unrecognized message" + e.payload);
				break;
		}
	}
});