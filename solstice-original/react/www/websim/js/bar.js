var Bar = function(g, max, norm, layers) {
    this.g = g.append("g").attr("class", "bar");
    this.max = max;
    this.norm = norm;

    this.bgbars = [];
    var _g = this.g.append("g")
        .attr("class", "bgbars");
    for (var i = 0; i < max; i++) {
        var b = _g.append("rect")
            .attr("class", "bgbar")
            .attr("width", Bar.barWidth)
            .attr("x", i * Bar.barWidth)
            .attr("height", 0)
            .attr("y", Bar.plotHeight);
        this.bgbars.push(b);
    }

    this.showing = "Recv";
    this.layers = {};
    for (var i = 0; i < layers.length; i++) {
        var layer = layers[i];
        this.layers[layer] = zeros(max);
    }

    this.bars = [];
    for (var i = 0; i < max; i++) {
        var b = this.g.append("rect")
            .attr("class", "bar")
            .attr("width", Bar.barFill)
            .attr("x", i * Bar.barWidth)
            .attr("height", 0)
            .attr("y", Bar.plotHeight);
        this.bars.push(b);
    }

    this.title = this.g.append("text")
        .attr("class", "title")
        .attr("x", -5)
        .attr("y", Bar.plotHeight - Bar.normHeight / 2 + 5);

	this.g.append("line")
		.attr("class", "guide")
		.attr("x1", 0)
		.attr("x2", Bar.barWidth * max)
		.attr("y1", Bar.plotHeight - Bar.normHeight)
		.attr("y2", Bar.plotHeight - Bar.normHeight);

	this.g.append("line")
		.attr("class", "axis")
		.attr("x1", 0)
		.attr("x2", Bar.barWidth * max)
		.attr("y1", Bar.plotHeight)
		.attr("y2", Bar.plotHeight);

	this.g.append("line")
		.attr("class", "axis")
		.attr("x1", 0)
		.attr("x2", 0)
		.attr("y1", Bar.plotHeight - Bar.barHeight)
		.attr("y2", Bar.plotHeight);

    for (var i = 0; i < max; i += 10) {
        var w = i * Bar.barWidth;
        this.g.append("line")
            .attr("class", "grid")
            .attr("x1", w)
            .attr("x2", w)
            .attr("y1", Bar.plotHeight - Bar.barHeight)
            .attr("y2", Bar.plotHeight);
    }

    this.fences = [];
    var _g = this.g.append("g")
        .attr("class", "fences");
    for (var i = 0; i < max; i++) {
        var f = _g.append("line")
            .attr("class", "fence")
            .attr("x1", i * Bar.barWidth)
            .attr("x2", i * Bar.barWidth)
            .attr("y1", Bar.plotHeight - Bar.barHeight)
            .attr("y2", Bar.plotHeight)
            .attr("display", "none");
        this.fences.push(f);
    }

    this.curPos = 0;
	this.cursor = this.g.append("line")
		.attr("class", "cursor")
		.attr("x1", Bar.barFill)
		.attr("x2", Bar.barFill)
		.attr("y1", Bar.plotHeight - Bar.barHeight)
		.attr("y2", Bar.plotHeight);

    this.number = this.g.append("text")
        .attr("class", "number")
        .attr("x", 0)
        .attr("y", Bar.plotHeight - Bar.normHeight - 2);
}

Bar.slots = 1000;
Bar.barHeight = 48;
Bar.normHeight = 38;
Bar.plotHeight = 50;
Bar.barWidth = 2;
Bar.barFill = 1.5;
Bar.leftMargin = 20;
Bar.topMargin = 10;

Bar.prototype.labelUp = function() {
    for (var i = 0; i <= this.max; i += 50) {
        if (i == 0) { continue; }
        var t = (i * C.TickPerHour / 1000).toFixed(1);
        var x = i * Bar.barWidth;
        this.g.append("line")
            .attr("class", "tick")
            .attr("y1", Bar.plotHeight)
            .attr("y2", Bar.plotHeight + 5)
            .attr("x1", x)
            .attr("x2", x);
        if (i % 100 == 0) {
            this.g.append("text")
                .attr("class", "tlabel")
                .attr("y", Bar.plotHeight + 15)
                .attr("x", i * Bar.barWidth)
                .text("" + t + "ms");
        }
    }
}

Bar.prototype.style = function(from, to) {
    var c = "bar from" + from + " to" + to;
    this.g.attr("class", c);
}

Bar.prototype.setTitle = function(title) {
    this.title.text(title);
}

Bar.prototype.toNormed = function(d) {
    var ret = d / this.norm;
    if (ret > 1) { ret = 1; }
    return ret;
}

Bar.prototype.append = function(i, d, layer) {
    var normed = this.toNormed(d);
    this.layers[layer][i] = normed;

    if (layer == this.showing) {
        this.redrawBar(i, normed);
    }
}

Bar.prototype.redrawBar = function(i, normed) {
    var h = normed * Bar.normHeight;
    this.bars[i].attr("height", h)
        .attr("y", Bar.plotHeight - h);
}

Bar.prototype.bg = function(i, d) {
    var h = this.toNormed(d) * Bar.normHeight;
    this.bgbars[i].attr("height", h)
        .attr("y", Bar.plotHeight - h);
}

Bar.prototype.cur = function(i) {
    this.curPos = i;
    this.redrawCursor();
}

Bar.prototype.redrawCursor = function() {
    var i = this.curPos;

	var x = i * Bar.barWidth + Bar.barFill;
	this.cursor.attr("x1", x)
		.attr("x2", x);

    var normed = this.layers[this.showing][i];
    var number = (normed * 100).toFixed(1);
    this.number.text("" + number + "%");
    this.number.attr("x", Bar.barWidth * i + 3);
}

Bar.prototype.fence = function(i, b) {
    if (b) {
        this.fences[i].attr("display", "inline");
    } else {
        this.fences[i].attr("display", "none");
    }
}

// switch to a layer
Bar.prototype.showLayer = function(layer) {
    this.showing = layer;

    for (var i = 0; i < this.max; i++) {
        this.redrawBar(i, this.layers[layer][i]);
    }

    this.redrawCursor();
}

// show the entire bar figure
Bar.prototype.show = function() {
    this.g.attr("display", "inline");
}

// hide the entire bar figure
Bar.prototype.hide = function() {
    this.g.attr("display", "none");
}

Bar.prototype.pos = function(x, y) {
    gpos(this.g, x, y);
}

var Bars = function(g, max, norm, layers) {
    this.g = g.append("g")
        .attr("class", "bars");
    gpos(this.g, Bar.leftMargin, Bar.topMargin);

    var y = 0;

    this.sums = [];

    var _g = this.g.append("g")
        .attr("class", "sums");
    for (var i = 0; i < C.Nhost; i++) {
        var p = new Bar(_g, max, norm, layers);
        p.style("", i);
        p.setTitle("" + i + "-*");
        this.sums.push(p);
        p.pos(0,y);
        if ((i + 1) == C.Nhost) {
            p.labelUp();
        }
        y += Bar.plotHeight;
    }
    y += Bars.groupPadding;

    var _g = this.g.append("g")
        .attr("class", "sums2");
    for (var i = 0; i < C.Nhost; i++) {
        var p = new Bar(_g, max, norm, layers);
        p.style("", i);
        p.setTitle("*-" + i);
        this.sums.push(p);
        p.pos(0,y);
        y += Bar.plotHeight;
        if ((i + 1) == C.Nhost) {
            p.labelUp();
        }
    }
    y += Bars.groupPadding;

    var _g = this.g.append("g")
        .attr("class", "elems");
    this.d = [];
    for (var i = 0; i < C.Nhost; i++) {
        for (var j = 0; j < C.Nhost; j++) {

            var p = new Bar(_g, max, norm, layers);
            p.style(i, j);
            p.setTitle("" + i + "-" + j);
            this.d.push(p);
            if (i == j) {
                p.hide();
            } else {
                p.pos(0, y);
                y += Bar.plotHeight;
            }

            if ((j + 1) == C.Nhost) {
                p.labelUp();
            }
        }
        y += Bars.groupPadding;
    }

    this.height = y;
}

Bars.groupPadding = 30;

function getSums(d) {
    var ret = [];
    for (var i = 0; i < C.Nhost; i++) {
        var s = 0;
        for (var j = 0; j < C.Nhost; j++) {
            s += d[i * C.Nhost + j];
        }
        ret.push(s);
    }

    for (var i = 0; i < C.Nhost; i++) {
        var s = 0;
        for (var j = 0; j < C.Nhost; j++) {
            s += d[j * C.Nhost + i];
        }
        ret.push(s);
    }

    return ret;
}

Bars.prototype.append = function(h, data, layer) {
    for (var i = 0; i < C.Nentry; i++) {
        this.d[i].append(h, data[i], layer);
    }

    var sums = getSums(data);
    for (var i = 0; i < C.Nhost * 2; i++) {
        this.sums[i].append(h, sums[i], layer);
    }
}

Bars.prototype.bg = function(h, data) {
    for (var i = 0; i < C.Nentry; i++) {
        this.d[i].bg(h, data[i]);
    }

    var sums = getSums(data);
    for (var i = 0; i < C.Nhost * 2; i++) {
        this.sums[i].bg(h, sums[i]);
    }
}

Bars.prototype.cur = function(h) {
    for (var i = 0; i < C.Nentry; i++) {
        this.d[i].cur(h);
    }
    for (var i = 0; i < C.Nhost * 2; i++) {
        this.sums[i].cur(h);
    }
}

Bars.prototype.fence = function(h, f) {
    for (var i = 0; i < C.Nentry; i++) {
        this.d[i].fence(h, f);
    }
    for (var i = 0; i < C.Nhost * 2; i++) {
        this.sums[i].fence(h, f);
    }
}

Bars.prototype.showLayer = function(layer) {
    for (var i = 0; i < C.Nentry; i++) {
        this.d[i].showLayer(layer);
    }
    for (var i = 0; i < C.Nhost * 2; i++) {
        this.sums[i].showLayer(layer);
    }
}

Bars.prototype.show = function() {
    this.g.attr("display", "inline");
}

Bars.prototype.hide = function() {
    this.g.attr("display", "none");
}

