var state = 'recent'
var add_marker = 0
var map

var basePin = new GIcon();
basePin.shadow = "/images/pin_shadow.png";
basePin.iconSize = new GSize(12,20);
basePin.shadowSize = new GSize(22,20);
basePin.iconAnchor = new GPoint(6,20);
basePin.infoWindowAnchor = new GPoint(5,1);

var redPin = new GIcon(basePin);    redPin.image = "/images/pin_red.png";
var yellowPin = new GIcon(basePin); yellowPin.image = "/images/pin_yellow.png";
function createPin(id, point, zoomLevel, text) {
    var m = new GMarker(point, redPin);
    m.zoomlevel = zoomLevel
    m.id = id
    m.bubbletext = text
    GEvent.addListener(m, "click", function() {
        m.openInfoWindowHtml(text)
    });
    return m;
}

function add_place_form() {
    if (state=='adding') return
    state = 'adding'
    document.getElementById('recent_places').style.display='none';
    document.getElementById('add_success').style.display='none';
    document.getElementById('add_place').style.display='block';
    document.getElementById('report_success').style.display='none'
    document.getElementById('incorrect_entry').style.display='none'
    if (add_marker) map.addOverlay(add_marker)
}
function show_recent_places() {
    if (state=='recent') return
    state = 'recent'
    document.getElementById('recent_places').style.display='block';
    document.getElementById('add_success').style.display='none';
    document.getElementById('report_success').style.display='none'
    document.getElementById('incorrect_entry').style.display='none'
    document.getElementById('add_place').style.display='none';
    if (add_marker) map.removeOverlay(add_marker)
}

var marker_reported = 0
function report_post_form(marker) {
    if (state=='reporting') return
    state = 'reporting'
    document.getElementById('report_title').innerHTML = marker.bubbletext
    document.getElementById('report_id').value = marker.id
    marker_reported = marker
    document.getElementById('incorrect_entry').style.display='block'
    document.getElementById('report_success').style.display='none'
    document.getElementById('recent_places').style.display='none'
    document.getElementById('add_success').style.display='none'
    document.getElementById('add_place').style.display='none'
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
    if (state!='adding') return; /* Not in adding mode, shouldn't be able to submit */
    pass = true
    if (!add_marker) {
        pass = false
        var str = '<b style="color: #ff0000">Please select somewhere</b>'
        document.getElementById('show_where').innerHTML = str
        document.getElementById('show_where2').innerHTML = str
    } else {
        var am = add_marker.getPoint()
        lng = am.x
        lat = am.y
    }
    name = encodeURIComponent(f.name.value)
    email = encodeURIComponent(f.email.value)
    title = encodeURIComponent(f.q.value)
    zoom = map.getZoomLevel()

    if (!name) { pass = false; field_error(f.name, 'nameerror', 'Please give your name') } else field_unerror(f.name, 'nameerror')
    if (!email) { pass = false; field_error(f.email, 'emailerror', 'Please give your email address') } else field_unerror(f.email, 'emailerror')
    if (!title) { pass = false; field_error(f.q, 'titleerror', 'Please give an article') } else field_unerror(f.q, 'titleerror')
    if (!pass) return;

    var d = document.getElementById('add_submit')
    d.value = 'Submitting...'; d.disabled = true
    var r = GXmlHttp.create();
    url = "/cgi-bin/submit.cgi"
    var post_data = "name="+name+";email="+email+";title="+title+";lng="+lng+";lat="+lat+";zoom="+zoom
    r.open("POST", url, true);
    r.setRequestHeader("Content-Type", "application/x-www-form-urlencoded")
    r.onreadystatechange = function(){
        if (r.readyState == 4) {
            x = r.responseXML
            var d = document.getElementById('add_submit')
            d.value = 'Submit'; d.disabled = false
            errors = x.getElementsByTagName('error')
            if (errors.length) {
                form = document.getElementById('f')
                for (e=0; e<errors.length; e++) {
                    field = errors[e].getAttribute('field')
                    error = GXml.value(errors[e])
                    var errspan = ''
                    if (field=='q') errspan = 'titleerror'
                    if (field=='email') errspan = 'emailerror'
                    field_error(form[field], errspan, error)
                }
                return;
            }
            state = 'addingsuccess'
            document.getElementById('add_place').style.display='none'
            document.getElementById('add_success').style.display='block'
            map.removeOverlay(add_marker)
            add_marker = 0
            update_place_list()
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

function POPsearch(s) {
    var d = document.getElementById('Submit1')
    d.value = 'Searching...'; d.disabled = true
    document.getElementById('q').disabled = true
    var r = GXmlHttp.create();
    r.open("GET", "/lookup.php?output=xml&q=" + encodeURIComponent(s), true); 
    /* r.open("GET", "/cgi-bin/lookup.cgi?output=xml&q=" + encodeURIComponent(s), true); */
    r.onreadystatechange = function(){
        if (r.readyState == 4) {
            var d = document.getElementById('Submit1')
            d.value = 'Search'; d.disabled = false
            document.getElementById('q').disabled = false
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
                    out += '<li><a href="#needsJS" onclick=\'POPsearch("' + q + '"); return false;\'>' + d + '</a>'
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
    return false;
};

// Auto complete stuff
function sf(){document.f.q.focus();}
function rwt(el,ct,cd,sg){
    el.href="/cgi-bin/suggest.fcg?url="+escape(el.href).replace(/\+/g,"%2B")+"&sg="+sg;
    el.onmousedown="";
    return true;
}
function qs(el) {if (window.RegExp && window.encodeURIComponent) {
    var ue=el.href;
    var qe=encodeURIComponent(document.f.q.value);
    if(ue.indexOf("q=")!=-1){
        el.href=ue.replace(new RegExp("q=[^&$]*"),"q="+qe);
    } else {
        el.href=ue+"&q="+qe;
    }
}
return 1;
}

function show_post(marker) {
    p = marker.getPoint()
    z = marker.zoomlevel
    map.centerAndZoom(p, z)
    GEvent.trigger(marker, "click")
}

function report_post(f) {
    if (state!='reporting') return; /* Not in reporting mode, shouldn't be able to submit */
    pass = true
    if (!add_marker) {
        locstr = ''
    } else {
        zoom = map.getZoomLevel()
        var am = add_marker.getPoint()
        locstr = ";lng=" + am.x + ";lat=" + am.y + ";zoom=" + zoom
    }
    name = encodeURIComponent(f.name.value)
    email = encodeURIComponent(f.email.value)
    explain = encodeURIComponent(f.explain.value)
    report_id = document.getElementById('report_id').value

    if (!name) { pass = false; field_error(f.name, 'nameerror2', 'Please give your name') } else field_unerror(f.name, 'nameerror2')
    if (!email) { pass = false; field_error(f.email, 'emailerror2', 'Please give your email address') } else field_unerror(f.email, 'emailerror2')
    if (!explain) { pass = false; field_error(f.explain, 'explainerror', 'Please given an explanation') } else field_unerror(f.explain, 'explainerror')
    if (!pass) return;

    var d = document.getElementById('report_submit')
    d.value = 'Submitting...'; d.disabled = true
    var r = GXmlHttp.create();
    var url = "/cgi-bin/submit-correction.cgi"
    var post_data = "name="+name+";email="+email+";explain="+explain+";id="+report_id + locstr
    r.open("POST", url, true);
    r.setRequestHeader("Content-Type", "application/x-www-form-urlencoded")
    r.onreadystatechange = function(){
        if (r.readyState == 4) {
            x = r.responseXML
            var d = document.getElementById('report_submit')
            d.value = 'Submit'; d.disabled = false
            errors = x.getElementsByTagName('error')
            if (errors.length) {
                form = document.getElementById('f2')
                for (e=0; e<errors.length; e++) {
                    field = errors[e].getAttribute('field')
                    error = GXml.value(errors[e])
                    var errspan = ''
                    if (field=='email') errspan = 'emailerror2'
                    field_error(form[field], errspan, error)
                }
                return;
            }
            state = 'reportsuccess'
            document.getElementById('incorrect_entry').style.display='none'
            document.getElementById('report_success').style.display='block'
            if (marker_reported) {
                map.removeOverlay(marker_reported)
                marker_reported = 0
            }
            if (add_marker) {
                map.removeOverlay(add_marker)
                add_marker = 0
            }
        }
    }
    r.send(post_data);
}

function update_place_list() {
    var bounds = map.getBoundsLatLng();
    var span = map.getSpanLatLng();
    var halfwidth = span.width/2;
    var centre = map.getCenterLatLng();
    bounds.minX = centre.x - halfwidth;
    bounds.maxX = parseFloat(centre.x) + halfwidth;
    var r = GXmlHttp.create();
    url = "/cgi-bin/list.fcg?type=xml;topleft_lat=" + bounds.maxY + ";topleft_long="+ bounds.minX + ";bottomright_lat=" + bounds.minY + ";bottomright_long=" + bounds.maxX
    r.open("GET", url, true);
    r.onreadystatechange = function(){
        if (r.readyState ==4) {
            x = r.responseXML
            newhtml = GXml.value(x.getElementsByTagName('newhtml')[0])
            var notshown = GXml.value(x.getElementsByTagName('notshown')[0])
            markers = x.getElementsByTagName('result');
            for (m=0; m<marker.length; m++) {
                map.removeOverlay(marker[m])
            }
            marker = []
            for (m=0; m<markers.length; m++) {
                lat = parseFloat(markers[m].getAttribute('lat'))
                lng = parseFloat(markers[m].getAttribute('lng'))
                zoom = parseInt(markers[m].getAttribute('zoom'), 10)
                var id = parseInt(markers[m].getAttribute('id'), 10)
                bubble = GXml.value(markers[m])
                marker[m] = window.createPin(id, new GPoint(lng, lat), zoom, bubble)
                map.addOverlay(marker[m])
            }
            document.getElementById('list').innerHTML = newhtml
            document.getElementById('notshown').innerHTML = ', ' + notshown + ' ' + (notshown==1?'entry':'entries') + ' not shown'
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
        if (state!='adding' && state!='reporting') return false
        if (overlay) return false
        else if (point) {
            var s= 100000
            var str = "long. = " + Math.round(point.x*s)/s + "; lat. = " + Math.round(point.y*s)/s
            document.getElementById('show_where').innerHTML = str
            document.getElementById('show_where2').innerHTML = str
            if (add_marker) map.removeOverlay(add_marker)
            add_marker = new GMarker(point, yellowPin)
            map.addOverlay(add_marker)
        }
    });

    if (marker.length==1) {
        map.centerAndZoom(marker[0].getPoint(), marker[0].zoomlevel);
    } else {
        map.centerAndZoom(new GPoint(10, 34.724620), 16);
    }
    for (p=0; p<marker.length; p++)
        map.addOverlay(marker[p])
    if (marker.length==1)
        GEvent.trigger(marker[0], "click")

    /* Not perfect, but it'll do for now */
    GEvent.addListener(map, 'moveend', update_place_list);

    d = document.getElementById('f')
    if (d)
        InstallAC(d,d.q,d.btnG,"suggest.fcg","en", true);

    var query_string = location.search.substring(1)
    if (query_string.substring(0,3) == 'add') {
        query_string = query_string.substring(8)
        d.q.value = query_string
        add_place_form()
    }
}
window.onload = onLoad;
window.onunload = GUnload;

function revert() {
    if (!map) return true;
    show_recent_places();
    map.closeInfoWindow()
    map.centerAndZoom(new GPoint(10, 34.724620), 16); // -4.218750, 34.724620), 16);
    return false;
}
