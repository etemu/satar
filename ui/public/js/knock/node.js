var view;

function Node(data) {
    this.id = ko.observable(data.id);
    this.status = ko.observable(data.status.valueOf().toString(2));
    this.delta = ko.observable(data.delta);
}
function NodeViewModel() {
    var self = this;
    self.nodes = ko.observableArray([]);
    view = self;
    // first load
    $.getJSON("/json/nodes", function(raw) {
        var nodes = $.map(raw, function(item) { return new Node(item) });
        self.nodes(nodes);
        console.log(nodes);
    });
}
ko.applyBindings(new NodeViewModel());

$( document ).ready(function() {
    console.log( "ready!" );
    var self = view;
    // streaming
    var es = new EventSource('/stream/nodes');
    es.onmessage = function (e) {
        var data = JSON.parse(e.data);
        console.log(data);
        var node = ko.utils.arrayFirst(self.nodes(), function (node) {
            return node.id === data.id;
        });
        if (node) {
            node.status = data.status.valueOf().toString(2);
            node.delta = data.delta;
        } else {
            self.nodes.push(data);
        }
    };
});
