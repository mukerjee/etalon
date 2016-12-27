function translate(x, y) {
    return "translate(" + x + ", " + y + ")";
}

function htmlEsc(s) {
    var ret = s;
    ret.replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
    return ret
}

function zeros(n) {
    var ret = [];
    for (var i = 0; i < n; i++) {
        ret.push(0);
    }
    return ret;
}

function bind(t, f) {
    return function(d) { t[f](d); };
}

function undef(x) { return typeof x == 'undefined'; }

function ajax(url, func) {
    $.ajax({
        url: url,
        data: "",
        success: func,
        cache: false
    });
}

function post(url, d) {
	$.post(url, d);
}

function gpos(e, x, y) {
    e.attr("transform", translate(x, y));
}

function newText(g, x, y) {
    return g.append("text").attr("x", x).attr("y", y);
}