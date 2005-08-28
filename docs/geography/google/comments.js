    if (GBrowserIsCompatible()) {
            // do nothing
    } else {
        document.getElementById('browserwontwork').className= '';
    }

    var map = new GMap(document.getElementById("map"));
    var google_lat = document.getElementById("google_lat").innerHTML;
    var google_long = document.getElementById("google_long").innerHTML;
    // map.centerAndZoom(new GPoint(-4.218750, 54.724620), 12);
    var target= new GPoint(google_long, google_lat);

    map.centerAndZoom(target, 1);
    
    map.addOverlay(new GMarker(target));
    map.addControl(new GSmallMapControl());
    map.addControl(new GMapTypeControl());
    map.setMapType( _HYBRID_TYPE );


function onLoad() {
    // centre map on location of a click
    GEvent.addListener(map, "moveend", function() {
      var center = map.getCenterLatLng();
      var latLngStr = '(' + center.y + ', ' + center.x + ')';
      document.getElementById('message').innerHTML = latLngStr;
      var status =  "x=" + center.x + ";y=" + center.y + "";
      document.getElementById("where").innerHTML = status;
    });

GEvent.addListener(map, 'click', function(overlay, point) {
  if (overlay) {
    map.removeOverlay(overlay);
  } else if (point) {
    map.addOverlay(new GMarker(point));
  }
});

}
