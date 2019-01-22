#!/bin/bash -xe
#
# Script to build, install, and package the AWS SDK libraries.
# I use this to test the whole workflow on new EC2 instances.
#

function install_required_packages {
    distname="$1"

    if [[ "$distname" == "Ubuntu" || "$distname" == "Debian GNU/Linux" ]]; then
        sudo apt update

        # for Debian
        sudo apt-get install -y git

        # for autoconf and AWS SDK
        sudo apt-get install -y build-essential autoconf libtool cmake libssl-dev libcurl4-openssl-dev zlib1g-dev

        # for fpm
        sudo apt-get install -y ruby ruby-dev rubygems awscli
    elif [[ "$distname" == "Amazon Linux" || "$distname" == "CentOS Linux" ]]; then
        yum info epel-release && yum install -ty epel-release ||
            sudo yum install -y https://dl.fedoraproject.org/pub/epel/epel-release-latest-7.noarch.rpm
        sudo yum groupinstall -ty "Development Tools"
        sudo yum install -ty cmake3 wget libcurl-devel openssl-devel
        # for fpm
        sudo yum install -ty ruby-devel rpm-build rubygems
        if [[ ! "$distname" == "Amazon Linux" ]]; then
            sudo yum install -ty awscli
        fi
    fi

    sudo gem install --no-ri --no-rdoc fpm
    which fpm
    fpm --version
}


function build_aws_sdk {
	cd "$BASEDIR"

	test -f "${sdkver}.tar.gz" ||
	    wget "https://github.com/aws/aws-sdk-cpp/archive/${sdkver}.tar.gz"
	tar xzvf "${sdkver}.tar.gz"
	cd "aws-sdk-cpp-${sdkver}"
	mkdir -p build
	cd build

	$CMAKE -DCMAKE_BUILD_TYPE="${build_type}" -DBUILD_ONLY="logs" \
	    -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_DEPS=ON -DENABLE_RTTI=OFF ..
	make -j 8
	sudo make install
}

function fpm_package {
    distname="$1"
    cd "$BASEDIR"

    pkg_name="libaws-sdk-cpp-logs"
    pkg_description="AWS SDK for C++ - CloudWatch Logs"

    if [[ "$distname" == "Ubuntu" || "$distname" == "Debian GNU/Linux" ]]; then
        /usr/local/bin/fpm \
          --verbose \
          --input-type dir --output-type deb \
          --name "$pkg_name" \
          --description "$pkg_description" \
          --architecture "$(arch)" \
          --url "https://github.com/aws/aws-sdk-cpp" \
          --vendor "Amazon.com" \
          --version "${sdkver}" \
          --maintainer "<info@mschuette.name>" \
          --depends libssl-dev --depends libcurl4-openssl-dev \
          --force \
          --package . \
          /usr/lib/x86_64-linux-gnu/cmake/AWSSDK \
          /usr/lib/x86_64-linux-gnu/cmake/aws-* \
          /usr/lib/x86_64-linux-gnu/libaws-* \
          /usr/lib/x86_64-linux-gnu/pkgconfig/aws-* \
          /usr/include/aws
    elif [[ "$distname" == "Amazon Linux" || "$distname" == "CentOS Linux" ]]; then
        /usr/local/bin/fpm \
          --verbose \
          --input-type dir --output-type rpm \
          --name "$pkg_name" \
          --description "$pkg_description" \
          --architecture "$(arch)" \
          --url "https://github.com/aws/aws-sdk-cpp" \
          --vendor "Amazon.com" \
          --version "${sdkver}" \
          --maintainer "<info@mschuette.name>" \
          --depends openssl-devel --depends libcurl-devel \
          --force \
          --package . \
          /usr/lib64/cmake/AWSSDK \
          /usr/lib64/cmake/aws-* \
          /usr/lib64/libaws-* \
          /usr/lib64/pkgconfig/aws-* \
          /usr/include/aws
    fi
}

function upload_package {
    # without --region I get a ConnectionResetError on Debian stretch
    aws s3 cp . "s3://${s3bucket}/${pkg_os_release}/$(arch)/" \
      --recursive --exclude "*" --include "*.rpm" --include "*.deb" \
      --region "$s3region"
}

s3bucket="msdev-libaws-sdk-cpp"
s3region="eu-west-1"
sdkver="1.7.38"

. /etc/os-release
pkg_os_release="${ID}-${VERSION_ID}"
install_required_packages "$NAME"

# Ubuntu has cmake, Amazon Linux needs cmake3
CMAKE=$(which cmake3 || which cmake)
# we should use the same build_type across all libraries,
# if a debug-lib depends on a release-lib some symbols cannot be found
build_type=RelWithDebInfo

BASEDIR=${HOME:-/tmp/build}
mkdir -p "$BASEDIR"

build_aws_sdk
fpm_package "$NAME"
upload_package

# finished
df -h
shutdown -h now
