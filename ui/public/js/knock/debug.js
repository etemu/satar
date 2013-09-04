// global vars
var DebugGlobal;

// knockout
function Debug(data) {
    this.time = ko.observable(data.time);    
    this.text = ko.observable(data.text);
    this.level = ko.observable(data.level);
}
function DebugViewModel() {
    DebugGlobal = this;
    DebugGlobal.debugs = ko.observableArray([]);
    // first load
    $.getJSON("/json/debug", function(raw) {
        var debugs = $.map(raw, function(item) { return new Debug(item) });
        console.log(debugs);
        DebugGlobal.debugs(debugs);
    });
}
ko.applyBindings(new DebugViewModel(),document.getElementById("debugContainer"));


// streaming
$( document ).ready(function() {
    console.log( "debug ready!" );
    // streaming
    var debug_es = new EventSource('/stream/debug');
    debug_es.onmessage = function (e) {
        var data = JSON.parse(e.data);
        DebugGlobal.debugs.push(data);
    };
});