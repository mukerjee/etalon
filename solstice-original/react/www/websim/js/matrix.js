var Cell = function(g, norm) {
    this.g = g.append("g").attr("class", "cell");
    this.norm = norm;
    
    this.g.append("rect").attr("class", "cellbox")
        .attr("width", Cell.width)
        .attr("height", Cell.height)
        .attr("x", 0)
        .attr("y", 0);

    this.bgs = [];
    for (var i = 0; i < Cell.count; i++) {
        this.bgs.push(Cell.newBar(this.g, i, "bg"));
    }
        
    this.bars = [];
    for (var i = 0; i < Cell.count; i++) {
        this.bars.push(Cell.newBar(this.g, i, "fg"));
    }
}

Cell.newBar = function(g, i, c) {
    return g.append("rect").attr("class", c)
        .attr("width", Cell.barWidth)
        .attr("height", 0)
        .attr("x", i * Cell.barWidth)
        .attr("y", Cell.height);
}

Cell.barWidth = 2;
Cell.count = 20;
Cell.width = 40;
Cell.height = 40;
Cell.normHeight = 36;
Cell.margin = 5;

Cell.prototype.pushInto = function(bars, d) {
    var h = d * Cell.normHeight / this.norm;
    if (h > Cell.height) { h = Cell.height; }

    var b = bars.shift();
    b.attr("height", h)
        .attr("y", Cell.height - h);

    bars.push(b);

    for (var i = 0; i < Cell.count; i++) {
        bars[i].attr("x", i * Cell.barWidth);
    }
}

Cell.prototype.push = function(d, bg) {
    this.pushInto(this.bars, d);
    this.pushInto(this.bgs, bg);
}

Cell.prototype.pos = function(x, y) {
    gpos(this.g, x, y);
}

var Matrix = function(g, norm) {
    this.g = g.append("g").attr("class", "matrix");
    this.cells = [];

    for (var i = 0; i < C.Nhost; i++) {
        for (var j = 0; j < C.Nhost; j++) {
            var cell = new Cell(this.g, norm);
            var x = i * (Cell.width + Cell.margin);
            var y = j * (Cell.height + Cell.margin);
            cell.pos(x, y);

            this.cells.push(cell);
        }
    }
}

Matrix.prototype.push = function(d, bg) {
    if (undef(bg)) { bg = zeros(C.Nentry); }

    for (var i = 0; i < C.Nentry; i++) {
        this.cells[i].push(d[i], bg[i]);
    }
}

Matrix.prototype.pos = function(x, y) {
    gpos(this.g, x, y);
}
