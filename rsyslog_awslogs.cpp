#include "rsyslog_awslogs.h"
#include <unistd.h>
#include <climits>
#include <aws/core/Aws.h>
#include <aws/core/utils/Outcome.h>
#include <aws/logs/CloudWatchLogsClient.h>
#include <aws/logs/model/DescribeLogGroupsRequest.h>
#include <aws/logs/model/CreateLogGroupRequest.h>
#include <aws/logs/model/DescribeLogStreamsRequest.h>
#include <aws/logs/model/CreateLogStreamRequest.h>
#include <aws/logs/model/PutLogEventsRequest.h>
#include <aws/logs/model/PutLogEventsResult.h>
#include <aws/core/utils/logging/LogSystemInterface.h>
#include <aws/core/utils/logging/LogMacros.h>

#include <chrono>
#include <iostream>

int CloudWatchLogsController::PutLogEvent(const char *msg) {
    AWS_LOGSTREAM_INFO(RSYSLOG_AWSLIB_TAG,
            "CloudWatchLogsController::PutLogEvent: " << msg);

    Aws::CloudWatchLogs::Model::PutLogEventsRequest put_req;
    Aws::CloudWatchLogs::Model::PutLogEventsOutcome put_resp;

    put_req.SetLogGroupName(log_group);
    put_req.SetLogStreamName(log_stream);
    if (!seq_token.empty()) {
        // only set for existing streams, empty for newly created streams
        put_req.SetSequenceToken(seq_token);
    }

    int_fast64_t ts_now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    Aws::CloudWatchLogs::Model::InputLogEvent event;
    event.SetTimestamp(ts_now);
    event.SetMessage(msg);
    put_req.AddLogEvents(event);

    put_resp = client->PutLogEvents(put_req);
    if (!put_resp.IsSuccess()) {
        AWS_LOGSTREAM_ERROR(RSYSLOG_AWSLIB_TAG,
                           "CloudWatchLogsController::PutLogEvent Error: " << put_resp.GetError());
        std::snprintf(last_error_message, RSYSLOG_AWSLIB_ERR_MSG_SIZE,
                      "Error in PutLogEvents: %s",
                      put_resp.GetError().GetMessage().c_str());
        last_status_code = 1;

        // TODO: I have _no_ idea how this might fail and
        //  how to implement a sensible retry mechanism :-(
        return 1;
    } else {
        Aws::CloudWatchLogs::Model::PutLogEventsResult result;
        result = put_resp.GetResult();
        seq_token = result.GetNextSequenceToken();

        last_error_message[0] = '\0';
        last_status_code = 0;

        // TODO: even on success we should read the RejectedLogEventsInfo
    }
    return 0;
}

CloudWatchLogsController::CloudWatchLogsController(const char *aws_region,
                                                   const char *log_group,
                                                   const char *log_stream) {
    // catch possible NULLs in params
    this->log_group  = log_group  ? Aws::String(log_group)  : Aws::String();
    this->log_stream = log_stream ? Aws::String(log_stream) : Aws::String();

    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    Aws::InitAPI(options);

    Aws::Client::ClientConfiguration clientConfig;
    if (aws_region) {
        clientConfig.region = Aws::String(aws_region);
    } // else: default region is us-east-1
    client = new Aws::CloudWatchLogs::CloudWatchLogsClient(clientConfig);

    /* it might be useful to show some user/client metadata
       like account ID and actual region, possibly the stream ARN.
       but we do not get that from the CloudWatchLogsClient;
       instead we would have to include an IAM client as well. */

    AWS_LOGSTREAM_INFO(RSYSLOG_AWSLIB_TAG, "initialized AWS API and created CloudWatchLogsClient");
}

int CloudWatchLogsController::EnsureGroupAndStream() {
    // set default values
    if (log_group.empty()) {
        log_group = "rsyslog";
    }
    if (log_stream.empty()) {
        char hostname[HOST_NAME_MAX];
        gethostname(hostname, HOST_NAME_MAX);
        log_stream = Aws::String(hostname);
    }

    // check log group
    Aws::CloudWatchLogs::Model::DescribeLogGroupsRequest desc_group_req;
    Aws::CloudWatchLogs::Model::DescribeLogGroupsOutcome desc_group_resp;

    desc_group_req.SetLogGroupNamePrefix(log_group);
    desc_group_req.SetLimit(1);
    desc_group_resp = client->DescribeLogGroups(desc_group_req);

    if (!desc_group_resp.IsSuccess()) {
        // AWS API problem, return the error
        std::snprintf(last_error_message, RSYSLOG_AWSLIB_ERR_MSG_SIZE,
                      "Error querying for group %s: %s",
                      log_group.c_str(), desc_group_resp.GetError().GetMessage().c_str());
        last_status_code = 1;
        return 1;
    }

    if (desc_group_resp.GetResult().GetLogGroups().empty()) {
        // create log_group
        Aws::CloudWatchLogs::Model::CreateLogGroupRequest create_group_req;
        Aws::CloudWatchLogs::Model::CreateLogGroupOutcome create_group_resp;

        create_group_req.SetLogGroupName(log_group);
        create_group_resp = client->CreateLogGroup(create_group_req);

        if (!create_group_resp.IsSuccess()) {
            std::snprintf(last_error_message, RSYSLOG_AWSLIB_ERR_MSG_SIZE,
                          "Error creating group %s: %s",
                          log_group.c_str(), create_group_resp.GetError().GetMessage().c_str());
            last_status_code = 1;
            return 1;
        }
    }

    // group is present, now check stream
    Aws::CloudWatchLogs::Model::DescribeLogStreamsRequest desc_stream_req;
    Aws::CloudWatchLogs::Model::DescribeLogStreamsOutcome desc_stream_resp;

    desc_stream_req.SetLogGroupName(log_group);
    desc_stream_req.SetLogStreamNamePrefix(log_stream);
    desc_stream_req.SetLimit(1);
    desc_stream_resp = client->DescribeLogStreams(desc_stream_req);

    if (!desc_stream_resp.IsSuccess()) {
        // AWS API problem, return the error
        std::snprintf(last_error_message, RSYSLOG_AWSLIB_ERR_MSG_SIZE,
                      "Error querying for stream %s: %s",
                      log_stream.c_str(), desc_stream_resp.GetError().GetMessage().c_str());
        last_status_code = 1;
        return 1;
    }

    if (desc_stream_resp.GetResult().GetLogStreams().empty()) {
        // stream does not exist, create it
        Aws::CloudWatchLogs::Model::CreateLogStreamRequest create_stream_req;
        Aws::CloudWatchLogs::Model::CreateLogStreamOutcome create_stream_resp;

        create_stream_req.SetLogGroupName(log_group);
        create_stream_req.SetLogStreamName(log_stream);
        create_stream_resp = client->CreateLogStream(create_stream_req);

        if (!create_stream_resp.IsSuccess()) {
            std::snprintf(last_error_message, RSYSLOG_AWSLIB_ERR_MSG_SIZE,
                          "Error creating stream %s: %s",
                          log_stream.c_str(), create_stream_resp.GetError().GetMessage().c_str());
            last_status_code = 1;
            return 1;
        }

        // new stream, no sequence token

    } else {
        // existing stream, preserve sequence token
        Aws::CloudWatchLogs::Model::LogStream stream
            = desc_stream_resp.GetResult().GetLogStreams().front();
        seq_token = stream.GetUploadSequenceToken();
        AWS_LOGSTREAM_INFO(RSYSLOG_AWSLIB_TAG,
                "aws_ensure: found logstream " << stream.GetArn() << \
                ", created " << stream.GetCreationTime() << \
                ", with last sequence token " << seq_token);
    }
	last_error_message[0] = '\0';
	last_status_code = 0;
    return 0;
}

CloudWatchLogsController *aws_init(const char *region, const char *group_name, const char *stream_name) {
    auto obj = new CloudWatchLogsController(region, group_name, stream_name);
    return obj;
}

int aws_logs_ensure(CloudWatchLogsController *ctl) {
    return ctl->EnsureGroupAndStream();
}

int aws_logs_msg_put(CloudWatchLogsController *ctl, const char *msg) {
    return ctl->PutLogEvent(msg);
}

void aws_shutdown(CloudWatchLogsController *ctl) {
    Aws::ShutdownAPI(ctl->options);
}

int aws_logs_get_last_status(CloudWatchLogsController *ctl) {
    return ctl->last_status_code;
}

char *aws_logs_get_last_error(CloudWatchLogsController *ctl) {
    return ctl->last_error_message;
}
