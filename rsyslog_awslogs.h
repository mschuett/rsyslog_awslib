#ifndef RSYSLOG_AWSLIB_LIBRARY_H
#define RSYSLOG_AWSLIB_LIBRARY_H

#define RSYSLOG_AWSLIB_ERR_MSG_SIZE 256
#define RSYSLOG_AWSLIB_TAG "rsyslog_awslib"

#ifdef __cplusplus
#include <aws/core/Aws.h>
#include <aws/core/Region.h>
#include <aws/core/utils/Outcome.h>
#include <aws/logs/CloudWatchLogsClient.h>

class CloudWatchLogsController {
public:
    int last_status_code;
    char last_error_message[RSYSLOG_AWSLIB_ERR_MSG_SIZE];
    // Aws::String aws_region;
    Aws::String log_group;
    Aws::String log_stream;
    Aws::String seq_token;
    Aws::SDKOptions options;
    Aws::CloudWatchLogs::CloudWatchLogsClient *client;

    CloudWatchLogsController(const char *aws_region, const char *log_group,
                             const char *log_stream);
    int EnsureGroupAndStream();
    int PutLogEvent(const char* msg);
};


extern "C" {
#else /* __cplusplus */

/* opaque C struct to hide the class */
typedef struct CloudWatchLogsController CloudWatchLogsController;

#endif

CloudWatchLogsController* aws_init(const char *region, const char *group_name, const char *stream_name);
int aws_logs_ensure(CloudWatchLogsController *ctl);
void aws_shutdown(CloudWatchLogsController *ctl);

int aws_logs_msg_put(CloudWatchLogsController *ctl, const char *msg);
int aws_logs_get_last_status(CloudWatchLogsController *ctl);
char* aws_logs_get_last_error(CloudWatchLogsController *ctl);

#ifdef __cplusplus
}
#endif

#endif /* RSYSLOG_AWSLIB_LIBRARY_H */
