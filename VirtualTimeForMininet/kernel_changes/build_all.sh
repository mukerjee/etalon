#!/bin/bash
LOG=build.log
ERR=build.err

check_err() {
    cat $ERR
	if [ -e $ERR ];
	then
		keyword=error
		grep $keyword $ERR > /dev/null
		if [ $? -eq 0 ]; 
		then
			# found error in err file
			echo "build stop during $1"
			grep -A2 -B2 $keyword $ERR
			exit 1
		fi
	fi
}

make_kernel() {
	echo "step 1. make src code for kernel and module"
	sudo make 2>$ERR
	check_err make_kernel
}


make_modules() {
	echo "step 1.2. make modules"
	sudo make modules 2>$ERR
	check_err make_modules
}

install_modules() {
	echo "step 2. install modules"
	sudo make modules_install 2>$ERR
	check_err install_modules
}

install_kernel() {
	echo "step 3. install kernel"
	sudo make install 2>$ERR
	check_err install
	echo "make/install complete for kernel 3.16.3"
}

update_grub() {
	echo "step 4. update GRUB"
	update-grub 2>$ERR
	check_err update_grub
	echo "please reboot OS and press ESC on GRUB interface"
}

build_all() {
    rm -f $LOG
    rm -f $ERR
	make_kernel
	make_modules
	install_modules
	install_kernel
	update_grub

	echo "following software should be rebuilt: iperf-3, basic tests and mininet"
}

usage() {
	echo "akb: script for build kernel 3.16.3 by YJQ"
	echo "		-a: automate build all process, e.g. -k + -m + -i + -g"
    echo "		-k: make kernel"
    echo "		-m: -k + make modules"
    echo "		-i: -m + install modules and kernel"
    echo "		-g: just update GRUB" 
    exit 0
}

echo "log info is in ${LOG}"
echo "err info is in ${ERR}"


if [ $# -eq 0 ]
then
    build_all
else
	while getopts 'ahikmu' OPTION
	do
	    case $OPTION in
			a)	build_all;;
	        h)  usage;;
			i)	make_kernel;
				make_modules;
				install_modules;
				install_kernel;;
			k)	make_kernel;;
			m)	make_kernel;
				make_modules;;

			u)	update_grub;;
			?)	usage;;
	    esac
	done
	shift $(($OPTIND - 1))
fi
