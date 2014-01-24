var adding = 0
var add_marker = 0
var map

var basePin = new GIcon();
basePin.shadow = "http://www.placeopedia.com/images/pin_shadow.png";
basePin.iconSize = new GSize(12,20);
basePin.shadowSize = new GSize(22,20);
basePin.iconAnchor = new GPoint(6,20);
basePin.infoWindowAnchor = new GPoint(5,1);

var redPin = new GIcon(basePin);    redPin.image = "http://www.placeopedia.com/images/pin_red.png";
var yellowPin = new GIcon(basePin); yellowPin.image = "http://www.placeopedia.com/images/pin_yellow.png";
function createPin(point, zoomLevel, text) {
    var m = new GMarker(point, redPin);
    m.zoomlevel = zoomLevel
    if (text) {
        GEvent.addListener(m, "click", function() {
            m.openInfoWindowHtml(text)
        });
    }
    return m;
}

function add_place_form(override) {
    if (!document.getElementById('add_place')) return true;
    if (adding && !override) return false
    adding = 1
    document.getElementById('recent_places').style.display='none';
    document.getElementById('add_success').style.display='none';
    document.getElementById('add_place').style.display='block';
    document.getElementById('where').style.display='block';
    if (add_marker) map.addOverlay(add_marker)
    return false
}

function remove_place_form() {
    if (!adding) return
    adding = 0
    document.getElementById('recent_places').style.display='block';
    document.getElementById('add_success').style.display='none';
    document.getElementById('add_place').style.display='none';
    document.getElementById('where').style.display='none';
    if (add_marker) map.removeOverlay(add_marker)
}

function field_error(e,errspan,errstr) {
    e.style.backgroundColor = '#ffeeee';
    e.style.border = 'solid 1px #ff0000';
    if (errspan)
        document.getElementById(errspan).innerHTML = errstr
}
function field_unerror(e,errspan) {
    e.style.backgroundColor = '#ffffff';
    e.style.border = 'solid 1px #000000';
    if (errspan)
        document.getElementById(errspan).innerHTML = ''
}

function add_place(f) {
    if (!adding) return; // Not in adding mode, shouldn't be able to submit
    pass = true
    if (!add_marker) {
        pass = false
        document.getElementById('show_where').innerHTML = '<b style="color: #ff0000">Please select somewhere</b>'
    } else {
    	var am = add_marker.getPoint();
        lng = am.x
        lat = am.y
    }
    title = encodeURIComponent(f.title.value)
    name = encodeURIComponent(f.name.value)
    email = encodeURIComponent(f.email.value)
    emailalert = f.emailalert.checked
    summary = encodeURIComponent(f.summary.value)
    body = encodeURIComponent(f.why.value)
    zoom = map.getZoomLevel()

    if (!name) { pass = false; field_error(f.name, 'nameerror', 'Please give your name') } else field_unerror(f.name, 'nameerror')
    if (!email) { pass = false; field_error(f.email, 'emailerror', 'Please give your email address') } else field_unerror(f.email, 'emailerror')
    if (!title) { pass = false; field_error(f.title, 'titleerror', 'Please give a title') } else field_unerror(f.title, 'titleerror')
    if (!summary) { pass = false; field_error(f.summary, 'summaryerror', 'Please give a summary') } else field_unerror(f.summary, 'summaryerror')
    if (!body) { pass = false; field_error(f.why, 'bodyerror', 'Please give some text') } else field_unerror(f.why, 'bodyerror')
    if (!pass) return;

    document.getElementById('add_submit').value = 'Submitting...'
    var r = GXmlHttp.create();
    url = "/cgi-bin/submit.cgi"
    var post_data = "name="+name+";email="+email+";title="+title+";lng="+lng+";lat="+lat+";zoom="+zoom+";body="+body+";summary="+summary+";emailalert="+emailalert
    r.open("POST", url, true);
    r.setRequestHeader("Content-Type", "application/x-www-form-urlencoded")
    r.onreadystatechange = function(){
        if (r.readyState == 4) {
            x = r.responseXML
            document.getElementById('add_submit').value = 'Add place'
            errors = x.getElementsByTagName('error')
            if (errors.length) {
                form = document.getElementById('f')
                for (e=0; e<errors.length; e++) {
                    field = errors[e].getAttribute('field')
                    error = GXml.value(errors[e])
                    var errspan = field + 'error'
                    field_error(form[field], errspan, error)
                }
                return;
            }
            adding = 0
            document.getElementById('add_place').style.display='none'
            document.getElementById('where').style.display='none'
            document.getElementById('add_success').style.display='block'
            map.removeOverlay(add_marker)
        }
    }
    r.send(post_data);
}

GSize.fromLatLngXml = function(s) {
    return new GSize(s.getAttribute('lng'), s.getAttribute('lat'));
}
GPoint.fromLatLngXml = function(c) {
    return new GPoint(c.getAttribute('lng'), c.getAttribute('lat'));
}

function search(s) {
    document.getElementById('Submit1').value = 'Searching...';
    var r = GXmlHttp.create();
    r.open("GET", "/lookup.php?output=xml&q=" + encodeURIComponent(s), true);
    r.onreadystatechange = function(){
        if (r.readyState == 4) {
            document.getElementById('Submit1').value = 'Search';
            x = r.responseXML
            c = x.getElementsByTagName('center')[0]
            s = x.getElementsByTagName('span')[0]
            if (c && s) {
                p = GPoint.fromLatLngXml(c)
                s = GSize.fromLatLngXml(s)
                z = 4; // map.spec.getLowestZoomLevel(p, s, map.viewSize)
                map.centerAndZoom(p, z)
                document.getElementById('search_results').style.display = 'none'
                return 1;
            }
            refinements = x.getElementsByTagName('refinement');
            if (refinements.length) {
                var out = '<p>More than one match:</p><ul>'
                for (ref=0; ref<refinements.length; ref++) {
                    q = GXml.value(refinements[ref].getElementsByTagName('query')[0])
                    d = GXml.value(refinements[ref].getElementsByTagName('description')[0])
                    out += '<li><a href="#needsJS" onclick=\'window.search("' + q + '"); return false;\'>' + d + '</a>'
                }
                out += '</ul>'
                d = document.getElementById('search_results')
                d.innerHTML = out
                d.style.display = 'block'
                return 1;
            }
            error = x.getElementsByTagName('error')[0]
            if (error) {
                d = document.getElementById('search_results')
                d.innerHTML = GXml.value(error)
                d.style.display = 'block'
            }
        }
    }
    r.send(null);
};

function show_post(marker, id) {
    p = marker.getPoint()
    z = marker.zoomlevel
    map.centerAndZoom(p, z)
    GEvent.trigger(marker, "click")
}

function update_place_list() {
    var bounds = map.getBoundsLatLng();
    var span = map.getSpanLatLng();
    var halfwidth = span.width/2;
    var centre = map.getCenterLatLng();
    bounds.minX = centre.x - halfwidth;
    bounds.maxX = parseFloat(centre.x) + halfwidth;
    var r = GXmlHttp.create();
    url = "/cgi-bin/list.cgi?type=xml;topleft_lat=" + bounds.maxY + ";topleft_long="+ bounds.minX + ";bottomright_lat=" + bounds.minY + ";bottomright_long=" + bounds.maxX
    r.open("GET", url, true);
    r.onreadystatechange = function(){
        if (r.readyState ==4) {
            x = r.responseXML
            newhtml = GXml.value(x.getElementsByTagName('newhtml')[0])
            markers = x.getElementsByTagName('result');
            for (m=0; m<marker.length; m++) {
                map.removeOverlay(marker[m])
            }
            marker = []
            for (m=0; m<markers.length; m++) {
                lat = parseFloat(markers[m].getAttribute('lat'))
                lng = parseFloat(markers[m].getAttribute('lng'))
                zoom = parseInt(markers[m].getAttribute('zoom'), 10)
                bubble = GXml.value(markers[m])
                marker[m] = window.createPin(new GPoint(lng, lat), zoom, bubble)
                map.addOverlay(marker[m])
            }
            d = document.getElementById('list')
            if (d) {
                if (newhtml)
                    d.innerHTML = newhtml
                else
                    d.innerHTML = '<p>None</p>'
            }
	}
    }
    r.send(null)
};

function onLoad() {
    if (GBrowserIsCompatible()) {
        // do nothing
    } else {
        document.getElementById('browserwontwork').className= '';
    }

    map = new GMap(document.getElementById("map"));
    map.addControl(new GLargeMapControl());
    map.addControl(new GMapTypeControl());

    GEvent.addListener(map, 'click', function(overlay, point) {
        if (!adding) return false
        if (overlay) return false
        else if (point) {
            var latLngStr =  "lat=" + point.y + "; long=" + point.x;
            document.getElementById('show_where').innerHTML = latLngStr;
            if (add_marker) map.removeOverlay(add_marker)
            add_marker = new GMarker(point, yellowPin)
            map.addOverlay(add_marker)
        }
    });

    if (marker.length==1) {
        map.centerAndZoom(marker[0].getPoint(), marker[0].zoomlevel);
    } else {
        map.centerAndZoom(new GPoint(-4.218750, 54.724620), 12);
    }
    for (p=0; p<marker.length; p++)
        map.addOverlay(marker[p])
    if (marker.length==1)
        GEvent.trigger(marker[0], "click")

    /* Not perfect, but it'll do for now */
    GEvent.addListener(map, 'moveend', update_place_list);

//    d = document.getElementById('f')
//    if (d)
//        InstallAC(d,d.q,d.btnG,"/cgi-bin/suggest.cgi","en", true);

    if (adding)
        add_place_form(1);
}
window.onload = onLoad;
window.onunload = GUnload;

function revert() {
    remove_place_form();
    map.closeInfoWindow()
    map.centerAndZoom(new GPoint(-4.218750, 54.724620), 12);
}
