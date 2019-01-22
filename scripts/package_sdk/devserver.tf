# reproducibly create my Ubuntu dev machine

provider "aws" {
  region = "${var.aws_region}"
}

data "aws_vpc" "default" {
  tags {
    Name = "default"
  }
}

data "aws_subnet_ids" "default" {
  vpc_id = "${data.aws_vpc.default.id}"
}

data "aws_ami" "ami_ubuntu" {
  most_recent = true
  owners      = ["099720109477"]

  filter {
    name   = "name"
    values = ["ubuntu/images/hvm-ssd/ubuntu-bionic-18.04-amd64-server-*"]
  }
}

data "aws_ami" "ami_debian" {
  most_recent = true
  owners      = ["379101102735"]

  filter {
    name   = "name"
    values = ["debian-stretch-hvm-x86_64-*"]
  }
}

data "aws_ami" "ami_amazon" {
  most_recent = true
  owners      = ["137112412989"]

  filter {
    name   = "name"
    values = ["amzn2-ami-hvm-2.*-x86_64-gp2"]
  }
}

data "aws_ami" "ami_centos" {
  most_recent = true
  owners      = ["679593333241"]

  filter {
    name   = "name"
    values = ["CentOS Linux 7 x86_64 HVM EBS ENA*"]
  }
}

resource "aws_security_group" "ssh" {
  name        = "allow_all"
  description = "Allow all inbound traffic"
  vpc_id      = "${data.aws_vpc.default.id}"

  ingress {
    from_port   = 22
    to_port     = 22
    protocol    = "tcp"
    cidr_blocks = ["0.0.0.0/0"]
  }

  egress {
    from_port   = 0
    to_port     = 0
    protocol    = "-1"
    cidr_blocks = ["0.0.0.0/0"]
  }

  tags {
    Name = "tf allow ssh"
  }
}

resource "aws_spot_instance_request" "build_debian" {
  ami = "${data.aws_ami.ami_debian.id}"
  instance_type = "${var.ec2_type}"

  tags {
    Name = "AWS SDK build server"
  }

  root_block_device {
    volume_size = "${var.ec2_volume_size}"
  }

  key_name = "${var.ec2_sshkey}"
  associate_public_ip_address = true
  subnet_id = "${element(data.aws_subnet_ids.default.ids, 1)}"

  vpc_security_group_ids = ["${aws_security_group.ssh.id}"]
  iam_instance_profile   = "${aws_iam_instance_profile.buildserver.name}"

  user_data = "${file("../package_aws_sdk.sh")}"

  spot_price             = "${var.ec2_spot_price}"
  spot_type              = "one-time"
  wait_for_fulfillment   = true
}

resource "aws_spot_instance_request" "build_amazon" {
  ami = "${data.aws_ami.ami_amazon.id}"
  instance_type = "${var.ec2_type}"

  tags {
    volume_size = "${var.ec2_volume_size}"
  }

  root_block_device {
    volume_size = 64
  }

  key_name = "${var.ec2_sshkey}"
  associate_public_ip_address = true
  subnet_id = "${element(data.aws_subnet_ids.default.ids, 1)}"

  vpc_security_group_ids = ["${aws_security_group.ssh.id}"]
  iam_instance_profile   = "${aws_iam_instance_profile.buildserver.name}"

  user_data = "${file("../package_aws_sdk.sh")}"

  spot_price             = "${var.ec2_spot_price}"
  spot_type              = "one-time"
  wait_for_fulfillment   = true
}

resource "aws_spot_instance_request" "build_centos" {
  ami = "${data.aws_ami.ami_centos.id}"
  instance_type = "${var.ec2_type}"

  tags {
    Name = "AWS SDK build server"
  }

  root_block_device {
    volume_size = "${var.ec2_volume_size}"
  }

  key_name = "${var.ec2_sshkey}"
  associate_public_ip_address = true
  subnet_id = "${element(data.aws_subnet_ids.default.ids, 1)}"

  vpc_security_group_ids = ["${aws_security_group.ssh.id}"]
  iam_instance_profile   = "${aws_iam_instance_profile.buildserver.name}"

  user_data = "${file("../package_aws_sdk.sh")}"

  spot_price             = "${var.ec2_spot_price}"
  spot_type              = "one-time"
  wait_for_fulfillment   = true
}

resource "aws_spot_instance_request" "build_ubuntu" {
  ami = "${data.aws_ami.ami_ubuntu.id}"
  instance_type = "${var.ec2_type}"

  tags {
    Name = "AWS SDK build server"
  }

  root_block_device {
    volume_size = "${var.ec2_volume_size}"
  }

  key_name = "${var.ec2_sshkey}"
  associate_public_ip_address = true
  subnet_id = "${element(data.aws_subnet_ids.default.ids, 1)}"

  vpc_security_group_ids = ["${aws_security_group.ssh.id}"]
  iam_instance_profile   = "${aws_iam_instance_profile.buildserver.name}"

  user_data = "${file("../package_aws_sdk.sh")}"

  spot_price             = "${var.ec2_spot_price}"
  spot_type              = "one-time"
  wait_for_fulfillment   = true
}


output "public_ip_debian" {
  value = "${aws_spot_instance_request.build_debian.public_ip}"
}

output "instance_id_debian" {
  value = "${aws_spot_instance_request.build_debian.spot_instance_id}"
}

output "ssh_debian" {
  value = "ssh admin@${aws_spot_instance_request.build_debian.public_ip}"
}

output "get_output_debian" {
  value = "check console output with: aws ec2 get-console-output --instance-id ${aws_spot_instance_request.build_debian.spot_instance_id}"
}

output "public_ip_ubuntu" {
  value = "${aws_spot_instance_request.build_ubuntu.public_ip}"
}

output "instance_id_ubuntu" {
  value = "${aws_spot_instance_request.build_ubuntu.spot_instance_id}"
}

output "ssh_ubuntu" {
  value = "ssh ubuntu@${aws_spot_instance_request.build_ubuntu.public_ip}"
}

output "get_output_ubuntu" {
  value = "check console output with: aws ec2 get-console-output --instance-id ${aws_spot_instance_request.build_ubuntu.spot_instance_id}"
}

output "public_ip_amazon" {
  value = "${aws_spot_instance_request.build_amazon.public_ip}"
}

output "instance_id_amazon" {
  value = "${aws_spot_instance_request.build_amazon.spot_instance_id}"
}

output "ssh_amazon" {
  value = "ssh ec2-user@${aws_spot_instance_request.build_amazon.public_ip}"
}

output "get_output_amazon" {
  value = "check console output with: aws ec2 get-console-output --instance-id ${aws_spot_instance_request.build_amazon.spot_instance_id}"
}

output "public_ip_centos" {
  value = "${aws_spot_instance_request.build_centos.public_ip}"
}

output "instance_id_centos" {
  value = "${aws_spot_instance_request.build_centos.spot_instance_id}"
}

output "ssh_centos" {
  value = "ssh centos@${aws_spot_instance_request.build_centos.public_ip}"
}

output "get_output_centos" {
  value = "check console output with: aws ec2 get-console-output --instance-id ${aws_spot_instance_request.build_centos.spot_instance_id}"
}
