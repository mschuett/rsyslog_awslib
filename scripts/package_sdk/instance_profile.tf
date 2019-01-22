resource "aws_iam_role_policy" "buildserver" {
  name = "AllowBuildserverPolicy"
  role = "${aws_iam_role.buildserver.id}"
  policy = <<EOF
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
          "logs:CreateLogGroup",
          "logs:CreateLogStream",
          "logs:DescribeLogGroups",
          "logs:DescribeLogStreams",
          "logs:PutLogEvents"
      ],
      "Resource": [
          "arn:aws:logs:*:*:*"
      ]
    },
    {
      "Effect": "Allow",
      "Action": [
          "s3:*"
      ],
      "Resource": [
          "${aws_s3_bucket.packages.arn}",
          "${aws_s3_bucket.packages.arn}/*"
      ]
    }
  ]
}
EOF
}

resource "aws_iam_role_policy" "logging" {
  name = "AllowLoggingPolicy"
  role = "${aws_iam_role.logging.id}"
  policy = <<EOF
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
          "logs:CreateLogGroup",
          "logs:CreateLogStream",
          "logs:DescribeLogGroups",
          "logs:DescribeLogStreams",
          "logs:PutLogEvents"
      ],
      "Resource": [
          "arn:aws:logs:*:*:*"
      ]
    }
  ]
}
EOF
}

resource "aws_iam_role" "logging" {
  name = "LoggingRole"
  assume_role_policy = <<EOF
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Action": "sts:AssumeRole",
      "Principal": {
        "Service": "ec2.amazonaws.com"
      },
      "Effect": "Allow",
      "Sid": ""
    }
  ]
}
EOF
}

resource "aws_iam_role" "buildserver" {
  name = "BuildserverRole"
  assume_role_policy = <<EOF
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Action": "sts:AssumeRole",
      "Principal": {
        "Service": "ec2.amazonaws.com"
      },
      "Effect": "Allow",
      "Sid": ""
    }
  ]
}
EOF
}


resource "aws_iam_instance_profile" "logging" {
  name = "logging_profile"
  role = "${aws_iam_role.logging.name}"
}

resource "aws_iam_instance_profile" "buildserver" {
  name = "buildserver_profile"
  role = "${aws_iam_role.buildserver.name}"
}
