#include "awslib.h"
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


awslib_instance* aws_init(const char *region) {
    struct awslib_instance *inst;
    Aws::SDKOptions *options;
    Aws::CloudWatchLogs::CloudWatchLogsClient *client;
    Aws::Client::ClientConfiguration clientConfig;

    options = new Aws::SDKOptions();
    options->loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    Aws::InitAPI(*options);

    if (region) {
        clientConfig.region = "eu-central-1";
    }
    client = new Aws::CloudWatchLogs::CloudWatchLogsClient(clientConfig);

    // TODO: it might be useful to show some user/client metadata
    // like account ID and region
    // but then we would have to include an IAM client as well ...

    AWS_LOGSTREAM_INFO(RSYSLOG_AWSLIB_TAG, "initialized AWS API and created CloudWatchLogsClient");

    inst = new awslib_instance;
    inst->options = options;
    inst->client  = client;

    return inst;
}

/* check for log group and log stream, create if not present */
int aws_logs_ensure(struct awslib_instance* inst, const char* group_name, const char* stream_name) {
    Aws::CloudWatchLogs::CloudWatchLogsClient *client;
    client = (Aws::CloudWatchLogs::CloudWatchLogsClient*) inst->client;

    Aws::CloudWatchLogs::Model::DescribeLogGroupsRequest desc_group_req;
    Aws::CloudWatchLogs::Model::DescribeLogGroupsOutcome desc_group_resp;
    Aws::CloudWatchLogs::Model::CreateLogGroupRequest create_group_req;
    Aws::CloudWatchLogs::Model::CreateLogGroupOutcome create_group_resp;
    Aws::CloudWatchLogs::Model::DescribeLogStreamsRequest desc_stream_req;
    Aws::CloudWatchLogs::Model::DescribeLogStreamsOutcome desc_stream_resp;
    Aws::CloudWatchLogs::Model::CreateLogStreamRequest create_stream_req;
    Aws::CloudWatchLogs::Model::CreateLogStreamOutcome create_stream_resp;

    desc_group_req.SetLogGroupNamePrefix(group_name);
    desc_group_req.SetLimit(1);
    desc_group_resp = client->DescribeLogGroups(desc_group_req);

    if (!desc_group_resp.IsSuccess()) {
        // AWS API problem, return the error
        std::snprintf(inst->last_error_message, RSYSLOG_AWSLIB_ERR_MSG_SIZE,
                      "Error querying for group %s: %s",
                      group_name, desc_group_resp.GetError().GetMessage().c_str());
        inst->last_status_code = 1;
        return 1;
    }

    if (desc_group_resp.GetResult().GetLogGroups().empty()) {
        // create the group
        create_group_req.SetLogGroupName(group_name);
        create_group_resp = client->CreateLogGroup(create_group_req);

        if (!create_group_resp.IsSuccess()) {
            std::snprintf(inst->last_error_message, RSYSLOG_AWSLIB_ERR_MSG_SIZE,
                          "Error creating group %s: %s",
                          group_name, create_group_resp.GetError().GetMessage().c_str());
            inst->last_status_code = 1;
            return 1;
        }
    }

    desc_stream_req.SetLogGroupName(group_name);
    desc_stream_req.SetLogStreamNamePrefix(stream_name);
    desc_stream_req.SetLimit(1);
    desc_stream_resp = client->DescribeLogStreams(desc_stream_req);


    if (!desc_stream_resp.IsSuccess()) {
        // AWS API problem, return the error
        std::snprintf(inst->last_error_message, RSYSLOG_AWSLIB_ERR_MSG_SIZE,
                      "Error querying for stream %s: %s",
                      stream_name, desc_stream_resp.GetError().GetMessage().c_str());
        inst->last_status_code = 1;
        return 1;
    }

    if (desc_stream_resp.GetResult().GetLogStreams().empty()) {
        // stream does not exist, create it
        create_stream_req.SetLogGroupName(group_name);
        create_stream_req.SetLogStreamName(stream_name);
        create_stream_resp = client->CreateLogStream(create_stream_req);

        if (!create_stream_resp.IsSuccess()) {
            std::snprintf(inst->last_error_message, RSYSLOG_AWSLIB_ERR_MSG_SIZE,
                          "Error creating stream %s: %s",
                          stream_name, create_stream_resp.GetError().GetMessage().c_str());
            inst->last_status_code = 1;
            return 1;
        }
    } else {
        // existing stream, get sequence token
        Aws::String seq_token;
        Aws::CloudWatchLogs::Model::LogStream stream;

        stream = desc_stream_resp.GetResult().GetLogStreams().front();
        seq_token = stream.GetUploadSequenceToken();
        // AWS_LOGSTREAM_INFO(RSYSLOG_AWSLIB_TAG, (
        std::cout << \
                "aws_ensure: found logstream " << stream.GetArn() << \
                ", created " << stream.GetCreationTime() << ", with last sequence token " << \
                stream.GetUploadSequenceToken() << std::endl;
        inst->seq_token = seq_token;
    }

    inst->last_status_code = 0;
    return 0;
}

int aws_logs_msg_put(struct awslib_instance* inst, const char *group_name, const char *stream_name, const char* msg) {
    Aws::CloudWatchLogs::Model::PutLogEventsRequest put_req;
    Aws::CloudWatchLogs::Model::PutLogEventsOutcome put_resp;
    Aws::CloudWatchLogs::CloudWatchLogsClient *client;
    client = (Aws::CloudWatchLogs::CloudWatchLogsClient*) inst->client;

    AWS_LOGSTREAM_INFO(RSYSLOG_AWSLIB_TAG, "aws_logs_msg_put: " << msg);

    put_req.SetLogGroupName(group_name);
    put_req.SetLogStreamName(stream_name);
    if (!inst->seq_token.empty()) {
        // only set for existing streams, empty for newly created streams
        put_req.SetSequenceToken(inst->seq_token);
    }

    int_fast64_t ts_now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    Aws::CloudWatchLogs::Model::InputLogEvent event;
    event.SetTimestamp(ts_now);
    event.SetMessage(msg);
    put_req.AddLogEvents(event);

    put_resp = client->PutLogEvents(put_req);
    if (!put_resp.IsSuccess()) {
        std::cout << "ERROR: " << put_resp.GetError() << std::endl;
        std::snprintf(inst->last_error_message, RSYSLOG_AWSLIB_ERR_MSG_SIZE,
                      "Error in PutLogEvents: %s",
                      put_resp.GetError().GetMessage().c_str());
        inst->last_status_code = 1;
        return 1;
    } else {
        Aws::CloudWatchLogs::Model::PutLogEventsResult result;
        result = put_resp.GetResult();
        std::cout << "Put messages yields token " << result.GetNextSequenceToken() << std::endl;
        inst->seq_token = result.GetNextSequenceToken();
    }

    return 0;
}

void aws_shutdown(struct awslib_instance* inst) {
    Aws::SDKOptions *options;
    options = (Aws::SDKOptions*) inst->options;

    Aws::ShutdownAPI(*options);
}
