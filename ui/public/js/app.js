function Event(data) {
    this.time = ko.observable(data.time);
    this.id = ko.observable(data.id);
}

function EventViewModel() {
    var t = this;
    var self = this;
    t.events = ko.observableArray([]);
    $.getJSON("/api/focus/event", function(raw) {
        var events = $.map(raw, function(item) { return new Event(item) });
        self.events(events);
        console.log(events);
    });
    var es = new EventSource('/api/stream/event');
    es.onmessage = function(e) {
        var data = JSON.parse(e.data);
        console.log(data);
        t.events.push(data);
    };
}
ko.applyBindings(new EventViewModel());

