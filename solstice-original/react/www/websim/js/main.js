var Main = function() {
    this.canvas = d3.select("svg").append("g")
        .attr("id", "canvas")
        .attr("transform", translate(20, 20));

    d3.select("svg").attr("width", "1200px")
        .attr("height", "1200px");

    this.view = new View(this.canvas);
    this.restart();
}

Main.prototype.restart = function() {
	var c = {};
	function intVal(name) {
		return parseInt($("form [name=" + name + "]").val());
	}
	c.TickPerHour = intVal("TickPerHour");
	c.HourPerFrame = intVal("HourPerFrame");
    var s = JSON.stringify(c);
    sessionStorage.config = s;

    this.view.reset(s);
    var showing = $("form [name=show]").val();
    this.view.show(showing);
}

function setVisual(sel, show) {
    if (show) {
        $(sel).show();
    } else {
        $(sel).hide();
    }
}

function applyVisOptions() {
    setVisual("g.fences", $("form [name=fence]").is(":checked"));
    setVisual("line.grid", $("form [name=grid]").is(":checked"));
}

function canvasResize(w, h) {
    var px = function(i) { return "" + i + "px"; }
    d3.select("svg").attr("width", px(w))
        .attr("height", px(h));
}

Main.prototype.pause = function() { this.view.pause(); }
Main.prototype.show = function(s) { this.view.show(s); }

function formInit(m) {
    $("form [name=run]").click(function() { m.restart(); });
    $("form [name=pause]").click(function() { m.pause(); });
    $("form [name=show]").change(function() { m.show($(this).val()); });

    function visBind(name, sel) {
        $("form [name=" + name + "]").change(function() {
            setVisual(sel, $(this).is(":checked"));
        });
    }

    visBind("fence", "g.fences");
    visBind("grid", "line.grid");
}

function loadConfig() {
    var s = sessionStorage.config;
    if (!undef(s)) {
        var c = JSON.parse(sessionStorage.config);
        $("form [name=TickPerHour]").val(c.TickPerHour)
        $("form [name=HourPerFrame]").val(c.HourPerFrame)
    }
}

function main() {
    loadConfig();
    var m = new Main();
    formInit(m);
}

$(document).ready(main);

