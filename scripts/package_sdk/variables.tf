variable "aws_region" {
  default = "eu-west-1"
}

variable "ec2_sshkey" {
  # you will have to change this!
  default = "mschuett_ec2"
}

variable "ec2_type" {
  default = "t3.xlarge"
}

variable "ec2_volume_size" {
  # 8Gb is more than enough for the logs-build,
  # but compiling the whole SDK requires more than that
  default = "8"
}

variable "ec2_spot_price" {
  # t3.large  on-demand costs $0.091200
  # t3.xlarge on-demand costs $0.182400
  default = "0.12"
}
