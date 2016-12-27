var nhost = 8;
var nentry = 8 * 8;

function setNhost(n) {
    nhost = n;
    nentry = n * n;
}

function ajax(url, func) {
    $.ajax({
        url: url,
        data: "",
        success: func,
        cache: false
    });
}

function parseData(dat) {
    var lines = dat.split("\n");
    var headerLine = lines[0];
    console.log(headerLine);
    var header = JSON.parse(headerLine);
    setNhost(header.Nhost);
    var norm = header.Norm;
    
    var ret = [];
    for (var i = 0; i < nentry; i++) {
        ret.push([])
    }

    for (var i = 1; i < lines.length; i++) {
        if (lines[i] == "") { continue; }
        var d = JSON.parse(lines[i]);
        if (d.length != nentry) { 
            console.log("error: incorrect matrix size");
        }
        for (var j = 0; j < nentry; j++) {
            ret[j].push(d[j] / norm);
        }
    }

    return ret
}

var BarPlot = function(svg) {
    this.svg = svg;
    this.g = svg.append("g");

    this.nbar = 100;
    this.norm = 40;
    this.height = 50;
    this.barWidth = 3;
    this.barFill = 2.5;
    this.barClass = "bar";
    this.topMargin = 10;
    this.bottomMargin = 10;
    this.leftMargin = 10;
    this.rightMargin = 10;
}

BarPlot.prototype.Width = function() {
    return this.barWidth * this.nbar + this.leftMargin + this.rightMargin;
}

BarPlot.prototype.Height = function() {
    return this.topMargin + this.height + this.bottomMargin;
}

BarPlot.prototype.Pos = function(x, y) {
    x = x + this.leftMargin;
    y = y + this.topMargin;
    this.g.attr("transform", "translate("+x+","+y+")");
}

BarPlot.prototype.Y = function(y) {
    return this.height - y;
}

BarPlot.prototype.Plot = function(dat) {
    var n = dat.length;
    for (var i = 0; i < n; i++) {
        if (i >= this.nbar) { break; }

        var x = i * this.barWidth;

        var y = dat[i] * this.norm;
        if (y > this.height) { y = this.height; }

        var rect = this.g.append("rect");
        rect.attr("class", this.barClass)
            .attr("x", x)
            .attr("y", this.Y(y))
            .attr("height", y)
            .attr("width", this.barFill);

    }
}

BarPlot.prototype.DrawAxis = function(n) {
    this.g.append("line")
        .attr("class", "axis")
        .attr("x1", 0)
        .attr("x2", n * this.barWidth)
        .attr("y1", this.Y(0))
        .attr("y2", this.Y(0));
}

BarPlot.prototype.Class = function(c) {
    this.g.attr("class", c);
}

BarPlot.prototype.Title = function(t) {
    this.g.append("text")
        .text(t)
        .attr("class", "bar-title")
        .attr("x", 0)
        .attr("y", this.height * .8);
}

var Canvas = function(svg) {
    this.svg = svg;
}

Canvas.prototype.Size = function(w, h) {
    if (this.svg.attr("height") < h) {
        this.svg.attr("height", h);
    }
    if (this.svg.attr("width") < w) {
        this.svg.attr("width", w);
    }
}

Canvas.prototype.BarPlot = function() {
    return new BarPlot(this.svg);
}

function allZeros(d) {
    var n = d.length;
    for (var i = 0; i < n; i++) {
        if (d[i] != 0) { return false }
    }
    return true;
}

function plotData(dat, c) {
    var canv = new Canvas(d3.select("svg"));
    var height = 0;
    var width = 0;
    var margin = 20;

    for (var i = 0; i < nentry; i++) {
        var dest = i % nhost;
        var src = (i - dest) / nhost;
        var title = "" + src + "-" + dest;

        var d = dat[i];
        if (allZeros(d)) {
            continue;
        }
        var bar = canv.BarPlot();
        bar.nbar = d.length;

        bar.Class(c);

        bar.Pos(0, height);
        bar.Plot(d);
        bar.DrawAxis(d.length);
        bar.Title("" + src + "-" + dest);

        var w = bar.Width();
        if (w > width) { width = w; }
        height += bar.Height();
    }

    canv.Size(width, height);
}

function parseAndPlotData(dat, c) {
    var d = parseData(dat);
    plotData(d, c);
}

function readData() {
    ajax("./slice1", function(d) { 
        parseAndPlotData(d, "s1"); 
        ajax("./slice2", function(d) { 
            parseAndPlotData(d, "s2"); 
        })
    })
}

function main() {
    readData();
}

$(document).ready(main);

