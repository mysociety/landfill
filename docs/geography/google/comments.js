    var map = new GMap(document.getElementById("map"));
    map.addControl(new GLargeMapControl());
    map.addControl(new GMapTypeControl());
    map.centerAndZoom(marker.point, marker.zoomlevel);
    map.addOverlay(marker);
