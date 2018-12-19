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

static Aws::SDKOptions options;

void CloudWatchLogsController::SetLogEvent(const char *msg) {
    this->SetLogEventBatch(&msg, 1);
}

void CloudWatchLogsController::SetLogEventBatch(const char **msgs, unsigned int msg_count) {
    AWS_LOGSTREAM_INFO(RSYSLOG_AWSLIB_TAG,
                       "CloudWatchLogsController::SetLogEventBatch for "
                       << msg_count << " messages");
    int_fast64_t ts_now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    assert(events.empty());
    for (int i = 0; i < msg_count; i++) {
        Aws::CloudWatchLogs::Model::InputLogEvent newEvent;
        newEvent.SetTimestamp(ts_now);
        newEvent.SetMessage(msgs[i]);
        events.push_back(newEvent);
    }
}

int CloudWatchLogsController::PutLogEvents() {
    AWS_LOGSTREAM_INFO(RSYSLOG_AWSLIB_TAG,
                       "CloudWatchLogsController::PutLogEvents for "
                       << events.size() << " messages");

    Aws::CloudWatchLogs::Model::PutLogEventsRequest put_req;
    Aws::CloudWatchLogs::Model::PutLogEventsOutcome put_resp;

    put_req.SetLogGroupName(log_group);
    put_req.SetLogStreamName(log_stream);
    if (!seq_token.empty()) {
        // only set for existing streams, empty for newly created streams
        put_req.SetSequenceToken(seq_token);
    }

    assert(!events.empty());
    put_req.WithLogEvents(events);

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
        //  this line will just throw a way the batch of messages:
        events.clear();

        return 1;
    } else {
        Aws::CloudWatchLogs::Model::PutLogEventsResult result;
        result = put_resp.GetResult();
        seq_token = result.GetNextSequenceToken();

        last_error_message[0] = '\0';
        last_status_code = 0;

        events.clear();

        // TODO: even on success we should read the RejectedLogEventsInfo
    }
    return 0;
}

CloudWatchLogsController::CloudWatchLogsController(const char *aws_region,
                                                   const char *log_group,
                                                   const char *log_stream) {
    // catch possible NULLs in params
    if (log_group) {
        this->log_group = Aws::String(log_group);
    } else {
        this->log_group = "rsyslog";
    }

    if (log_stream) {
        this->log_stream = Aws::String(log_stream);
    } else {
        char hostname[HOST_NAME_MAX];
        gethostname(hostname, HOST_NAME_MAX);
        this->log_stream = Aws::String(hostname);
    }

    Aws::Client::ClientConfiguration clientConfig;
    if (aws_region) {
        clientConfig.region = Aws::String(aws_region);
    } // else: default region is us-east-1
    client = new Aws::CloudWatchLogs::CloudWatchLogsClient(clientConfig);

    /* this const should match the batch size in omawslogs.
       a smaller value will still work, but incur a performance penalty
       (due to std::vector resizing). */
    const int omawslogs_max_batch_size = 1024;
    events.reserve(omawslogs_max_batch_size);

    /* it might be useful to show some user/client metadata
       like account ID and actual region, possibly the stream ARN.
       but we do not get that from the CloudWatchLogsClient;
       instead we would have to include an IAM client as well. */

    AWS_LOGSTREAM_INFO(RSYSLOG_AWSLIB_TAG, "created CloudWatchLogsClient");
}

int CloudWatchLogsController::EnsureGroupAndStream() {
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

CloudWatchLogsController::~CloudWatchLogsController() {
    delete client;
}


void aws_init(int loglevel) {
	options.loggingOptions.logLevel = static_cast<Aws::Utils::Logging::LogLevel>(loglevel);
	Aws::InitAPI(options);
    AWS_LOGSTREAM_INFO(RSYSLOG_AWSLIB_TAG, "initialized AWS API");
}

CloudWatchLogsController *aws_create_controller(const char *region, const char *group_name, const char *stream_name) {
    return new CloudWatchLogsController(region, group_name, stream_name);
}

void aws_free_controller(CloudWatchLogsController *ctl) {
	delete ctl;
}

int aws_logs_ensure(CloudWatchLogsController *ctl) {
    return ctl->EnsureGroupAndStream();
}

int aws_logs_msg_put(CloudWatchLogsController *ctl, const char *msg) {
    ctl->SetLogEvent(msg);
    return ctl->PutLogEvents();
}

int aws_logs_msg_put_batch(CloudWatchLogsController *ctl, const char *msg[], unsigned int msg_count) {
    ctl->SetLogEventBatch(msg, msg_count);
    return ctl->PutLogEvents();
}

void aws_shutdown() {
    Aws::ShutdownAPI(options);
}

int aws_logs_get_last_status(CloudWatchLogsController *ctl) {
    return ctl->last_status_code;
}

char *aws_logs_get_last_error(CloudWatchLogsController *ctl) {
    return ctl->last_error_message;
}
