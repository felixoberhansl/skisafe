var connection = new autobahn.Connection({url: 'ws://127.0.0.1:8080/ws', realm: 'realm1'});

connection.onopen = function (session) {
	console.log("WAMP connected!")
    // subscribe to a topic
    function onevent_locationdata(args) {
      console.log("Location Data:", args[0]);

      if (args[0][1] == "campus"){      

	    if (typeof circle_campus == 'undefined') {
	    	circle_campus = new L.Circle([49.843964, 11.348782], {radius: args[0][0]}).addTo(mymap);
	    } 

	    circle_campus.setRadius(args[0][0])

	    
      }

      if (args[0][1] == "gerhaus"){
      	if (typeof circle_gerhaus == 'undefined') {
      		circle_gerhaus = new L.Circle([49.845115, 11.347917], {radius: args[0][0]}).addTo(mymap);
      	}

      	circle_gerhaus.setRadius(args[0][0])
      	

      }

      var date = new Date(args[0][2]*1000);
      // Hours part from the timestamp
      var hours = date.getHours();
      var minutes = "0" + date.getMinutes();
      var seconds = "0" + date.getSeconds();

      var formattedTime = hours + ':' + minutes.substr(-2) + ':' + seconds.substr(-2);
      document.getElementById("timeout").innerHTML = "Last ping at " + formattedTime;

    }
    session.subscribe('location_info', onevent_locationdata);


   function onevent_fallalert(args) {
    console.log("Fall Alert: ", args[0]);

    if (args[0] == true){
      document.getElementById("happy").style.display = "none";
      document.getElementById("sad").style.display = "block";
      document.getElementById("alarmmessage").innerHTML = "Rescue Forces were notified";
      document.getElementById("alarmmessage").style.color = "red";
      

    }
   }
   session.subscribe('fall_alert', onevent_fallalert);


   function onevent_timeout(args) {
    console.log("Timeout: ", args[0]);

    if (args[0] == true){
      document.getElementById("timeout").style.color = "red";

    }
   }
   session.subscribe('timeout', onevent_timeout);


   function onevent_fallintensity(args) {
    console.log("Fall Intensity: ", args[0]);
    document.getElementById("fallintensity").innerHTML = Math.round(args[0]*10)/10 + "G";

   }
   session.subscribe('fall_intensity', onevent_fallintensity);

   function onevent_temperature(args) {
    console.log("Temperature: ", args[0]);
    document.getElementById("temperature").innerHTML = args[0];

   }
   session.subscribe('temperature', onevent_temperature);


};

connection.open();




var mymap = L.map('mapid').setView([49.843964, 11.348782], 15);

L.tileLayer('https://api.tiles.mapbox.com/v4/{id}/{z}/{x}/{y}.png?access_token={accessToken}', {
    attribution: 'Map data &copy; <a href="https://www.openstreetmap.org/">OpenStreetMap</a> contributors, <a href="https://creativecommons.org/licenses/by-sa/2.0/">CC-BY-SA</a>, Imagery © <a href="https://www.mapbox.com/">Mapbox</a>',
    maxZoom: 18,
    id: 'mapbox.streets',
    accessToken: '' //enter API key here
}).addTo(mymap);


var campus_marker = L.marker([49.843964, 11.348782]).addTo(mymap);
campus_marker.bindPopup("<b>MIOTY basestation @ Fraunhofer IIS</b>").openPopup();

var gerhaus_marker = L.marker([49.845115, 11.347917]).addTo(mymap);
gerhaus_marker.bindPopup("<b>MIOTY basestation @ Gerhaus Waischenfeld</b>").openPopup();
