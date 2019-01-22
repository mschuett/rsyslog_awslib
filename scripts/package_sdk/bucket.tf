
resource "aws_s3_bucket" "packages" {
  bucket = "msdev-libaws-sdk-cpp"
  acl    = "public-read"
}
