# Scripts

I had some problems with the build process on different systems and gathering all dependencies.
This directory contains the scripts I use to get clean builds on VMs.

## build_and_install.sh

The first script version for a full build and test on one EC2 instance.

## package_sdk

Subdir `package_sdk` contains Terraform code to create an S3 bucket and
start EC2 spot instances for Amazon Linux, CentOS, Debian, and Ubuntu.
The instances receive `package_aws_sdk.sh` as a userdata script to run,
this will build and package the AWS-SDK. The resulting RPM or DEB file is
put in the S3 bucket.

Building the whole SDK is possible and will take about three hours on
a t3.xlarge.

The current script uses the option `-DBUILD_ONLY="logs"` to only build the
CloudWatch Logs (and core) libraries. The whole process takes about 5 minutes
(on a t3.xlarge instance type).

I used this to compile the current SDK version 1.7.38 for the distributions mentioned above:
 * [amzn-2/aarch64/libaws-sdk-cpp-logs-1.7.38-1.aarch64.rpm](https://s3-eu-west-1.amazonaws.com/msdev-libaws-sdk-cpp/amzn-2/aarch64/libaws-sdk-cpp-logs-1.7.38-1.aarch64.rpm)
 * [amzn-2/x86_64/libaws-sdk-cpp-logs-1.7.38-1.x86_64.rpm](https://s3-eu-west-1.amazonaws.com/msdev-libaws-sdk-cpp/amzn-2/x86_64/libaws-sdk-cpp-logs-1.7.38-1.x86_64.rpm)
 * [centos-7/x86_64/libaws-sdk-cpp-logs-1.7.38-1.x86_64.rpm](https://s3-eu-west-1.amazonaws.com/msdev-libaws-sdk-cpp/centos-7/x86_64/libaws-sdk-cpp-logs-1.7.38-1.x86_64.rpm)
 * [debian-9/x86_64/libaws-sdk-cpp-logs_1.7.38_amd64.deb](https://s3-eu-west-1.amazonaws.com/msdev-libaws-sdk-cpp/debian-9/x86_64/libaws-sdk-cpp-logs_1.7.38_amd64.deb)
 * [ubuntu-18.04/x86_64/libaws-sdk-cpp-logs_1.7.38_amd64.deb](https://s3-eu-west-1.amazonaws.com/msdev-libaws-sdk-cpp/ubuntu-18.04/x86_64/libaws-sdk-cpp-logs_1.7.38_amd64.deb)
