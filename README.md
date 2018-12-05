# Rsyslog omawslogs

This is an attempt to write an Rsyslog output plugin for AWS CloudWatch Logs.

The conventional method to write syslog data to CloudWatch Logs is to use the
[CloudWatch Agent](https://docs.aws.amazon.com/AmazonCloudWatch/latest/monitoring/Install-CloudWatch-Agent.html)
to forward lines from `/var/log/messages` or `/var/log/syslog`.

This plugin will integrate the CloudWatch Logs output into Rsyslog.

It is currently in a proof-of-concept state because I am neither familiar with
the Rsyslog plugin interface, nor the AWS SDK for C++.
But if this turns out to be usable it could even become a useful boilerplate
for similar outputs to SQS or Kinesis.

## Library and component issues

One big problem with this plugin: it is written in C++ in order to use the
AWS SDK for C++. The shared library provides C functions to be called by
the real "C-only" Rsyslog omawslib plugin.

At runtime we have this dependency chain:

```
+-----------------+                                                          
|                 |                                                          
|    Rsyslogd     |                                                          
|                 |                                                          
|  +--------------+     +----------------------+     +----------------------+
|  |              |     |                      |     |                      |
|  | C plugin     |     | C++ glue code        |     | AWS SDK C++          |
|  | omawslogs.so +-----> librsyslog_awslib.so +-----> libaws-cpp-sdk-*.so  |
|  |              |     |                      |     |                      |
+--+--------------+     +----------------------+     +----------------------+
```

### Toolchain question

I am not happy with this split into two libraries, but I could not get it
to build in another way.

I guess I could put this whole module into the rsyslog repository into a
`contrib/awslib` subfolder, but that alone would not solve the issue;
it also would have to integrate with the Autotools build toolchain.

If someone knows their way around the GNU Autotools I would be very
interested if it is possible to use C++ plugins in Rsyslog.
Simply adding a .cpp file *does* call g++ to compile it, but I could not
include Rsyslog header files as they raise multiple compile errors.

## AWS credentials

This plugin does not do any AWS credential handling but completely relies
on the SDK to do the right thing. Its Readme describes the
[default credential provider chain](https://github.com/aws/aws-sdk-cpp#default-credential-provider-chain).

Advice on credentials:
If your server runs on AWS then use EC2 Instance Profiles
(or ECS Task Roles).
Outside of AWS (for testing and developing) setup your
`$HOME/.aws/credentials` file (e.g. using the [AWS CLI](https://aws.amazon.com/cli/));
if you have more than one set of credentials then use the environment variable
`AWS_PROFILE` to select the right one.

## AWS permissions

For an overview of the CloudWatch Logs permissions see the
[CloudWatch Logs Permissions Reference](https://docs.aws.amazon.com/AmazonCloudWatch/latest/logs/permissions-reference-cwl.html).

This library uses the API calls:
* logs:CreateLogGroup
* logs:CreateLogStream
* logs:DescribeLogGroups
* logs:DescribeLogStreams
* logs:PutLogEvents

## Limitations

### Rsyslog plugin limits

#### Transactions

The current plugin implements the `doAction()` interface and handles single
events, i.e. every log message causes an API call.

It would be nice to switch to the transactional interface,
in order to send multiple messages in a batch.

#### Timestamps

Every AWS CloudWatch Logs event is a pair of timestamp and text.
Right now the library uses the current timestamp when sending the message to AWS.

It would be nice to get some timestamp from Rsyslog, either the one it has
parsed from the message (preferable) or the timestamp of its own message
receive/processing (still better than nothing).

#### Boilerplate

I tried to copy the plugin structure from other plugins in order to make it
understandable and maintainable for the Rsyslog developers. Unfortunately I
do not get most of it myself.
E.g. could we please drop Rsyslog 4.x compatibility at some point?
It would help to have one config interface and not two parallel ones.

### AWS CloudWatch functionality

This library will create the configured log group and stream on startup.
It will not setup any retention policy, any tags, KMS encryption, or stream destinations.

The default region is `us-east-1`. So if no region is configured then your logs will end up there.
