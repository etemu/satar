// global vars
var NodeGlobal;

// knockout
function Node(data) {
    this.id = data.id;
    this.status = data.status.valueOf().toString(2);
    this.delta = data.delta;
    this.link = "/raw/event/" + data.id
}
function NodeViewModel() {
    NodeGlobal = this;
    NodeGlobal.nodes = ko.observableArray([]);
    // first load
    $.getJSON("/json/node", function(raw) {
        var nodes = $.map(raw, function(item) { return new Node(item) });
        NodeGlobal.nodes(nodes);
    });
}
ko.applyBindings(new NodeViewModel(),document.getElementById("nodeContainer"));


// streaming
$( document ).ready(function() {
    console.log( "node ready!" );
    // streaming
    var node_es = new EventSource('/stream/node');
    node_es.onmessage = function (e) {
        var data = JSON.parse(e.data);
        console.log(data);
        var node = ko.utils.arrayFirst(NodeGlobal.nodes(), function (node) {
            return node.id == data.id;
        });
        if (node != null) {
            node.status = data.status.valueOf().toString(2);
            node.delta = data.delta;
        } else {
            NodeGlobal.nodes.push(Node(data));
        }
    };
});