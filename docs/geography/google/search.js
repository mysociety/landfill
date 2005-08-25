
    var searchmap = new GMap(document.getElementById("searchmap"));

    searchmap.centerAndZoom(new GPoint(-4.218750, 54.724620), 12);

    searchmap.addControl(new GLargeMapControl());
    searchmap.addControl(new GMapTypeControl());
    searchmap.setMapType( _HYBRID_TYPE );


function onLoad() {
    // centre map on location of a click
    GEvent.addListener(searchmap, "moveend", function() {
      var center = searchmap.getCenterLatLng();
      var latLngStr = '(' + center.y + ', ' + center.x + ')';
      document.getElementById('show_where').innerHTML = latLngStr;
      //var status =  "x=" + center.x + ";y=" + center.y + "";
      // document.getElementById("search_where").innerHTML = status;
    });

    GEvent.addListener(searchmap, 'click', function(overlay, point) {
      if (overlay) {
        searchmap.removeOverlay(overlay);
      } else if (point) {
        searchmap.addOverlay(new GMarker(point));
      }
    });



    function update_place_list() {
	var bounds = searchmap.getBoundsLatLng();
        var r = GXmlHttp.create(); 
        r.open("GET", "http://www.yourhistoryhere.com/cgi-bin/list.cgi?topleft_lat=" + bounds.maxY + ";topleft_long="+ bounds.minX + ";bottomright_lat=" + bounds.minY + ";bottomright_long=" + bounds.maxX, true);
        r.onreadystatechange = function(){
	     if (r.readyState ==4) {
		     // document.getElementById('list_of_places').innerHTML = r.responseText; 
	    }
         }
    };

    GEvent.addListener(this.searchmap, "moveend", update_place_list);
    GEvent.addListener(this.searchmap, "click", update_place_list);

}

