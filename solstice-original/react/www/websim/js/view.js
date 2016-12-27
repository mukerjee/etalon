var View = function(canvas) {
    this.active = false;
    this.started = false;
    this.waiting = 0;
    this.canvas = canvas;
    this.g = null;
    this.showing = "Recv";
    this.built = false;

    this.tickCounter = 0;
    var t = this;
    d3.timer(function() { return t.tick(); } )
};
View.hourPerBar = 500;
View.frameDiv = 1;

View.prototype.tick = function() {
	if (!this.active) {
		return;
	}
    
    this.tickCounter++;
    if (this.tickCounter == View.frameDiv) {
        this.tickCounter = 0;
        if (this.waiting < 10) {
            this.waiting++;
            ajax("./frame", bind(this, "frame"));
        }
    }
}

View.prototype.reset = function(c) {
    this.stop();

	post("./restart", c); 
    this.active = true; // will start polling frame
}

View.prototype.pause = function() {
	this.active = !this.active;
}

View.bars = [
    "Send",
    "Recv",
    "CircRecv",
    "PackRecv",
    "Drop",
    "Sched"
];

View.prototype.start = function(c) {
    // configure the setup
    config(c);

    // reset the canvas
    if (this.g != null) {
        this.g.remove();
    }

    this.g = this.canvas.append("g").attr("class", "view");

    var norm = C.TickPerHour * C.LinkBw;
    this.bars = new Bars(this.g, View.hourPerBar, norm, View.bars);

    this.timeLabel = newText(this.g, 0, 5);

    this.sums = [];
    for (var i = 0; i < View.bars.length; i++) {
        this.sums[View.bars[i]] = 0;
    }
    this.sendLabel = newText(this.g, 100, 5);
    this.recvLabel = newText(this.g, 250, 5);
    this.circLabel = newText(this.g, 400, 5);
    this.packLabel = newText(this.g, 550, 5);
    this.dropLabel = newText(this.g, 700, 5);

    // now ready for receiving other messages
    this.built = true;
    this.started = true; 

    this.show(this.showing);

    var w = View.hourPerBar * Bar.barWidth + 100;
    var h = this.bars.height + 100;
    canvasResize(w, h);
    applyVisOptions();
}

View.prototype.stop = function() {
    this.started = false;
    this.active = false; // auto sleep, so we won't be pulling
}

View.prototype.addSum = function(b, d) {
    for (var i = 0; i < C.Nentry; i++) {
        this.sums[b] += d[i];        
    }
}

View.prototype.draw = function(d) {
    var t = d.T + 1;
    var h = Math.floor(d.T / C.TickPerHour) % View.hourPerBar;

    this.timeLabel.text("time=" + t);

    for (var i = 0; i < View.bars.length; i++) {
        var b = View.bars[i];
        this.bars.append(h, d[b], b);
        this.addSum(b, d[b]);
    }
    this.bars.bg(h, d.Sched);
    this.bars.fence(h, d.NewWeek);
    this.bars.cur(h); // move cursor

    this.sendLabel.text("send=" + this.sums["Send"]);
    this.recvLabel.text("recv=" + this.sums["Recv"]);
    this.circLabel.text("circ=" + this.sums["CircRecv"]);
    this.packLabel.text("pack=" + this.sums["PackRecv"]);
    this.dropLabel.text("drop=" + this.sums["Drop"]);
}

View.prototype.frame = function(data) {
    lines = data.split("\n");
    for (var i = 0, n = lines.length; i < n; i++) {
        var line = lines[i];
        if (line.length == 0) { continue; }
        var obj = JSON.parse(line);
        if (obj.C == "start") {
            // obj.D contains the simulation setup
            this.start(obj.D);
        } 
        if (this.started) {
            // only care these messages when properly configured
            if (obj.C == "hour") {
                var d = obj.D;
                this.draw(d);
            } else if (obj.C == "stop") {
                this.stop();
            }
        }
    }

    this.waiting--;
}

View.prototype.show = function(n) {
    this.showing = n;

    if (this.built) {
        this.bars.showLayer(this.showing);
    }
}
