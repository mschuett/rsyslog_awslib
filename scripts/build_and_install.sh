#!/bin/bash -xe

. /etc/os-release
if [[ "$NAME" = "Ubuntu" ]]; then
	# from the Rsyslog readme
	sudo apt-get install -y build-essential pkg-config libestr-dev \
			libfastjson-dev zlib1g-dev uuid-dev libgcrypt20-dev liblogging-stdlog-dev \
			libhiredis-dev uuid-dev libgcrypt11-dev liblogging-stdlog-dev flex bison
	# additional requirements for AWS SDK
	sudo apt-get install -y autoconf libtool cmake libssl-dev libcurl4-openssl-dev

	# TODO: not sure how to determine correct libdir for Debian vs. RedHat on different architectures
	export RSYSLOG_LIBDIR=/usr/lib/x86_64-linux-gnu/rsyslog
elif [[ "$NAME" = "Amazon Linux" ]]; then
	sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
	sudo yum groupinstall -y "Development Tools"
	sudo yum install -y cmake3 libcurl-devel openssl-devel libestr-devel \
			libuuid-devel libgcrypt-devel
	export RSYSLOG_LIBDIR=/usr/lib64/rsyslog
fi

# Ubuntu has cmake, Amazon Linux needs cmake3
export CMAKE=$(which cmake3 || which cmake)
# we should use the same build_type across all libraries,
# if a debug-lib depends on a release-lib some symbols cannot be found
export build_type=Release
# just in case we want to move this to /tmp
export BASEDIR=$HOME


# AWS SDK requirements
if [[ ! -f /usr/local/lib/libaws-c-common.so ]]; then
	cd ${BASEDIR}
	git clone https://github.com/awslabs/aws-c-common
	cd aws-c-common
	git checkout a9380a43ab3762
	mkdir build && cd build
	$CMAKE -DCMAKE_BUILD_TYPE=${build_type} -DBUILD_SHARED_LIBS=ON ..
	make
	sudo make install
fi

if [[ ! -f /usr/local/lib/libaws-checksums.so ]]; then
	cd ${BASEDIR}
	git clone https://github.com/awslabs/aws-checksums
	cd aws-checksums
	git checkout 8db7ac89d232b
	mkdir build && cd build
	$CMAKE -DCMAKE_BUILD_TYPE=${build_type} -DBUILD_SHARED_LIBS=ON ..
	make
	sudo make install
fi

if [[ ! -f /usr/local/lib/libaws-c-event-stream.so ]]; then
	cd ${BASEDIR}
	git clone https://github.com/awslabs/aws-c-event-stream
	cd aws-c-event-stream
	git checkout 40dc42f47f3
	mkdir build && cd build
	$CMAKE -DCMAKE_BUILD_TYPE=${build_type} -DBUILD_SHARED_LIBS=ON ..
	make
	sudo make install
fi

# AWS SDK, build only core and logs components
if [[ ! -f /usr/local/lib/libaws-cpp-sdk-logs.so ]]; then
	cd ${BASEDIR}
	wget https://github.com/aws/aws-sdk-cpp/archive/1.7.14.tar.gz
	tar xzvf 1.7.14.tar.gz
	cd aws-sdk-cpp-1.7.14
	mkdir build
	cd build
	$CMAKE -DCMAKE_BUILD_TYPE=${build_type} -DBUILD_ONLY="logs" -DBUILD_DEPS=OFF -DENABLE_RTTI=OFF ..
	make
	sudo make install
fi

# build glue lib
if [[ ! -f /usr/local/lib/librsyslog_awslogs.so ]]; then
	cd ${BASEDIR}
	git clone https://github.com/mschuett/rsyslog_awslib.git
	cd rsyslog_awslib
	git checkout 15c99d27
	mkdir build
	cd build
	$CMAKE -DCMAKE_BUILD_TYPE=${build_type} ..
	make
	sudo make install
fi


# build plugin
if [[ ! -f "${RSYSLOG_LIBDIR}/omawslogs.so" ]]; then
	cd ${BASEDIR}
	git clone https://github.com/mschuett/rsyslog.git
	cd rsyslog
	git checkout 3483d34
	# note: Amazon Linux 2 has libfastjson-devel 0.99.4,
	# but current rsyslog master requires >= 0.99.8
	# this ignores the requirement, because we _only_ use omawslogs
	if [[ "$NAME" = "Amazon Linux" ]]; then
		sed -i -e 's/libfastjson >= 0.99.8/libfastjson >= 0.99.4/' configure.ac
	fi
	autoreconf -fvi
	./configure --disable-generate-man-pages --enable-shared --enable-awslogs #--libdir=/usr/lib64
	cd contrib/omawslogs
	make
	# TODO: a 'sudo make install' will also install the .la lib; can I prevent that?
	sudo install .libs/omawslogs.so "${RSYSLOG_LIBDIR}/omawslogs.so"
fi


# write sample config
if [[ ! -f /etc/rsyslog.d/omawslogs.conf ]]; then
	# using a bad hack to use the same region as the current EC2 instance
	aws_az=$(ec2metadata --availability-zone)
	aws_az=${aws_az:-us-east-1}
	aws_region=${aws_az::-1}

	echo 'module(load="omawslogs")' | sudo tee -a /etc/rsyslog.d/omawslogs.conf
	echo 'action(name="cloudwatch" type="omawslogs" region="'$aws_region'")' | sudo tee -a /etc/rsyslog.d/omawslogs.conf
	# just to check the status
	sudo service rsyslog restart
	sudo service rsyslog status -l
fi
