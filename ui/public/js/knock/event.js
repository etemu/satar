function Event(data) {
    this.time = ko.observable(data.time);
    this.id = ko.observable(data.id);
}
function EventViewModel() {
    var self = this;
    self.events = ko.observableArray([]);
    
    // first load
    $.getJSON("/json/events", function(raw) {
        var events = $.map(raw, function(item) { return new Event(item) });
        self.events(events);
    });
    
    // streaming
    var es = new EventSource('/stream/events');
    es.onmessage = function(e) {
        var data = JSON.parse(e.data);
        self.events.push(data);
    };
}
ko.applyBindings(new EventViewModel());