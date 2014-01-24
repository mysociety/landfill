
function mapload() {
    if (GBrowserIsCompatible()) {
        // do nothing
    } else {
        document.getElementById('browserwontwork').className= '';
    }

    var map = new GMap(document.getElementById("map"));
    map.addControl(new GLargeMapControl());
    /* map.addControl(new GMapTypeControl()); */
    map.centerAndZoom(new GPoint(-4.218750, 54.724620), 12);

    GEvent.addListener(map, 'click', function(overlay, point) {
            var s= 100000
            var str = "long. = " + Math.round(point.x*s)/s + "; lat. = " + Math.round(point.y*s)/s
            document.getElementById('show_where').innerHTML = str
            document.getElementById('geo_lat').value = point.y
            document.getElementById('geo_long').value = point.x
            if (add_marker) map.removeOverlay(add_marker)
            add_marker = new GMarker(point, yellowPin)
            map.addOverlay(add_marker)
    });


}
window.onunload = GUnload;


