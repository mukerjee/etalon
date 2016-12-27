#!/usr/bin/env Rscript
# library(plotrix);
# library(ggplot2);
library(RColorBrewer);

data_input_file <- c('thput');
draw_output <- c('e2.pdf');

pdf(draw_output, paper = "special", height=6, width = 10)
layout(matrix(c(1,2), 2, 1, byrow = TRUE))

par(mar=c(2, 2.4, 2, 1.2));
par(mgp=c(1, 0, 0));
par(lwd=1.2) 
trace_info <- read.table('trace')
trace_dim <- dim(trace_info)
day_on <- c();
day_off <- c();
now_off <- TRUE
week_end <- c(); 
sched_updates <- c();

ts_max <- trace_info[trace_dim[1], 1];
my_col <- brewer.pal(8, "Accent") ; 
my_col <- c(my_col[1:3], my_col[5:8], 'blue')
xmin = 144
xmax = 4644
plot(c(), xlim=c(0, xmax-xmin), ylim=c(0, 7.2), 
     xaxt='n', type = 'n', bty='n', xlab = 'Time (us)' , yaxt= 'n', ylab = '', cex.lab= 1, cex.axis=1.2);

# now let's start to plot the rectanglar box 
for (i in seq(1, trace_dim[1])) { # trace_dim[1]: rows
    t = trace_info[i, 1]
    if (t < xmin || t > xmax) {
        next
    }
	if (trace_info[i, 3] == 4) { #PFC info
		if (now_off) {
			day_off <-c(day_off, trace_info[i, 1]);
			now_off = FALSE;
		} else { 
			day_on <- c(day_on, trace_info[i, 1]);
			now_off = TRUE; # flip value
		}
	} else if (trace_info[i, 3] == 2) { # weekend signal 
		week_end <- c(week_end, trace_info[i,1]);
    } else if (trace_info[i, 3] == 5) { # schedule loading
        sched_updates <- c(sched_updates, trace_info[i,1]);
	} else if (trace_info[i, 3] == 0) { # data packet 
		left_x <- trace_info[i, 1]
		right_x <- trace_info[i, 1] + 0.2
		left_corner <- trace_info[i, 2] + 0.15
		right_corner <- trace_info[i, 2] + 0.45
		rect(left_x, left_corner, right_x, right_corner, col =
		    my_col[trace_info[i,2]+1], border = my_col[trace_info[i,2]+1]);	
	}
	else if (trace_info[i, 3] == 1) { # ack packet 
		left_x <- trace_info[i, 1]
		right_x <- trace_info[i, 1] + 0.1 
		left_corner <- trace_info[i, 2] + 0.55
		right_corner <- trace_info[i, 2] + 0.85
		rect(left_x, left_corner, right_x, right_corner, col =
		    my_col[trace_info[i,2]+1], border = my_col[trace_info[i,2]+1]);	
	}
}

#abline(v = day_on, lty=1)
#abline(v = day_off, lty=3)
abline(h = seq(0,7), lwd=1, col=gray.colors(20)[15])
# abline(h = seq(1,7)-0.5, lwd=1, col=gray.colors(20)[15])
abline(v=week_end+30, lty=2, col='red')
abline(v=sched_updates, lty=3, col='black')
par(xpd=TRUE);
text(160, seq(0, 6)+0.25, 'Ack', cex=0.7, pos=2)
text(160, seq(0, 6)+0.75, 'Data', cex=0.7, pos=2)
for (i in seq(0, 6)) {
    text(-235, i+0.5, substitute(paste('Host ', x %->% 7), list(x=6-i)), cex=0.8)
    #text(-235, seq(0,6)+0.5, expression(x %->% y), cex=0.8)
}

week_start_times = seq(0,2) * 1500
text(week_end+60, 7.5, 'Apply', cex=0.8, pos=2)
text(week_end+30, -0.1, week_start_times, cex=0.8, pos=1)
# text(week_end[1]-30, -0.5, 'time in trace = 1.7039707s', cex=0.7, pos=1);

# text(800, -0.55, 300, cex=0.8);

#legend(x = 5600, y=9, c('day start', 'day end'), lty=c(1,3), cex=0.7, bty='n') 
#rect(2099.22, 7.5, 2099.22+30, 7.9, col = 'red', border = 'red')
text(sched_updates + 30, 7.5, 'Reconfig', cex=0.8, pos=2);

# **************   second plot ************* 
# start a new one 
par(mar=c(3.5, 2.5, 2, 6));
par(mgp=c(1.5, 0.5, 0));
par(lwd=2);

# now let's read in the text value
data_input_file <- c('thput');
my_data <- read.table(data_input_file);
m_size <- dim(my_data)
my_data[1:m_size[1], 1] = my_data[1:m_size[1], 1]/1000000000; 

t_max <- my_data[m_size[1], 1];
plot(x = my_data[1:m_size[1], 1], y= my_data[1: m_size[1], 2], ylim=c(0, 10), 
col='black', type = 'l', xlab = 'Time (s)' , ylab='Throughput (Gbps)', cex.lab=
1, cex.axis=1, bty='n');

margin_c <- 0
for (i in seq(1, m_size[2]-1)) { 
	for (j in seq(1, m_size[1]-1)) { 
		if (i == 1) {
			x <- c(my_data[j, 1], my_data[j, 1], my_data[j+1, 1], my_data[j+1, 1]); # time points 
			y <- c(0, my_data[j, i+1]-margin_c, my_data[j+1, i+1]-margin_c, 0);
		}
		else { 
			x <- c(my_data[j, 1], my_data[j, 1], my_data[j+1, 1], my_data[j+1, 1]); # time points 
			y <- c(my_data[j, i]+margin_c, my_data[j, i+1]-margin_c, my_data[j+1, i+1]-margin_c,
			my_data[j+1, i]+margin_c);
		}
		polygon(x, y, col=my_col[i], border=my_col[i]);
	}
}

for (i in seq(2, m_size[2])) {
	lines(my_data[1:m_size[1], 1], my_data[1:m_size[1], i], col = 'black',
	type='l', lwd=1);
}

segments(1.7, 11, 1.7, -1, lty=5, lwd=1)
segments(1.73, 11, 1.73, -1, lty=5, lwd=1)
segments(1.73, 11, 4.2, 13.2, lty=5, lwd=1)
segments(1.7, 11, -1, 13, lty=5, lwd=1)

legend_list <- c();
for (i in seq(0,6)) { 
	legend_list <- c(legend_list, substitute(x %->% 7, list(x=6-i)))
}
# print(legend_list)
legend(3.05, 9.8, expression(
                             paste('Host ', 0 %->% 7),
                             paste('Host ', 1 %->% 7),
                             paste('Host ', 2 %->% 7),
                             paste('Host ', 3 %->% 7),
                             paste('Host ', 4 %->% 7),
                             paste('Host ', 5 %->% 7),
                             paste('Host ', 6 %->% 7)
                             ), pch=15, col = rev(my_col[1:7]), bty='n',
cex=1, y.intersp=1.2) 
# dev.off();
