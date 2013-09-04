var EventGlobal;

function Event(data) {
    this.time = ko.observable(data.time);
    this.id = ko.observable(data.id);
    this.rider = 0;
}
function EventViewModel() {
    EventGlobal = this;
    EventGlobal.events = ko.observableArray([]);
    
    // first load
    $.getJSON("/json/event", function(raw) {
        var events = $.map(raw, function(item) { return new Event(item) });
        EventGlobal.events(events);
    });
    
    EventGlobal.saveEvent = function(item) {
		$.ajax({
   	   		type: "POST",
      		url: "/api/event/" + item.id(),
      		data: item.rider,
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
        EventGlobal.events.push(data);
    };
});