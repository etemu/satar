var EventGlobal;

function Event(data) {
    
    var date = new Date(data.time);
    // hours part from the timestamp
    var hours = date.getHours();
    var minutes = date.getMinutes();
    var seconds = date.getSeconds();
    
    this.time = hours + ':' + minutes + ':' + seconds;
    this.id = data.id;
    this.rider = 0;
    this.node = data.node_id
}
function EventViewModel() {
    EventGlobal = this;
    EventGlobal.events = ko.observableArray([]);
    
    // first load
    $.getJSON("/json/event", function(raw) {
        var events = $.map(raw, function(item) { return new Event(item) });
        console.log(events);
        EventGlobal.events(events);
    });
    
    EventGlobal.saveEvent = function(item) {
        var id = item.id;
				$.ajax({
						type: "POST",
						url: "/api/event/" + id,
						data: item.rider,
						success: function(){EventGlobal.events.remove(item); },
						dataType: "text"
				});
    }
    EventGlobal.deleteEvent = function(item) {
				var id = item.id;
				$.ajax({
						type: "POST",
						url: "/api/delete/" + id,
						data: "YOLO",
						success: function(){EventGlobal.events.remove(item); },
						dataType: "text"
				});
    }
}
ko.applyBindings(new EventViewModel(), document.getElementById("eventContainer"));

// streaming
$( document ).ready(function() {
    var event_es = new EventSource('/stream/event');
    event_es.onmessage = function (e) {
        var data = JSON.parse(e.data);
        data.rider = 0
        EventGlobal.events.push(Event(data));
    };
});
