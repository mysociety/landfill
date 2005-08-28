 
    var map = new GMap(document.getElementById("map"));
    map.centerAndZoom(new GPoint(-4.218750, 54.724620), 12);
    map.setMapType( _HYBRID_TYPE )
    map.addControl(new GLargeMapControl());
    map.addControl(new GMapTypeControl());


function onLoad() {
    // centre map on location of a click
    GEvent.addListener(map, "moveend", function() {
      var center = map.getCenterLatLng();
      var latLngStr =  "lat=" + center.y + ";long=" + center.x + "";
      document.getElementById('where').innerHTML = latLngStr;
      document.getElementById('location').value = latLngStr;
    });

GEvent.addListener(map, 'click', function(overlay, point) {
  if (overlay) {
    map.removeOverlay(overlay);
  } else if (point) {
    map.addOverlay(new GMarker(point));
  }
});

}
GEvent.addListener(map, 'click', function(overlay, point) {
    if (!point) {
        point = map.getCenterLatLng();
    }
      var latLngStr =  "lat=" + point.y + ";long=" + point.x + "";
      document.getElementById('where').innerHTML = latLngStr;
      document.getElementById('location').value = latLngStr;
});


