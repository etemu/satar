var ResultGlobal;

function Result(data) {
    this.id = data.rider_id;
    this.time = data.time;
}
function ResultViewModel() {
    ResultGlobal = this;
    ResultGlobal.results = ko.observableArray([]);
    
    // first load
    $.getJSON("/json/result", function(raw) {
        var results = $.map(raw, function(item) { return new Result(item) });
        ResultGlobal.results(results);
    });
}
ko.applyBindings(new ResultViewModel(), document.getElementById("resultsContainer"));

// streaming
$( document ).ready(function() {
    var result_es = new EventSource('/stream/result');
    result_es.onmessage = function (e) {
        var data = JSON.parse(e.data);
        console.log(data);
        ResultGlobal.results.push(Result(data));
    };
});