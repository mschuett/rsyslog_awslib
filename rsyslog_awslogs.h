#ifndef RSYSLOG_AWSLIB_LIBRARY_H
#define RSYSLOG_AWSLIB_LIBRARY_H

#define RSYSLOG_AWSLIB_ERR_MSG_SIZE 256
#define RSYSLOG_AWSLIB_TAG "rsyslog_awslib"

#ifdef __cplusplus
#include <aws/core/Aws.h>
#include <aws/core/Region.h>
#include <aws/core/utils/Outcome.h>
#include <aws/logs/CloudWatchLogsClient.h>
#include <aws/logs/model/InputLogEvent.h>

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
    Aws::Vector<Aws::CloudWatchLogs::Model::InputLogEvent> events;

    CloudWatchLogsController(const char *aws_region, const char *log_group,
                             const char *log_stream);
    int EnsureGroupAndStream();
    int PutLogEvents();
    void SetLogEvent(const char *msg);
    void SetLogEventBatch(const char **msgs, unsigned int msg_count);
};


extern "C" {
#else /* __cplusplus */

/* opaque C struct to hide the class */
typedef struct CloudWatchLogsController CloudWatchLogsController;

#endif

/*
 * Usage:
 * Always start with aws_init(), this initializes the AWS SDK and
 * returns a controller object.
 *
 * With the controller aws_logs_ensure() will check for group and stream
 * and create them if they do not exist. This is also the "first" action
 * against AWS, so this will show credential errors.
 * The usual error message for missing credentials is:
 * 'Error querying for group rsyslog: Missing Authentication Token'.
 *
 * The main actions are aws_logs_msg_put() to send a single event
 * (nb: this is already a legacy API and not used by my own code),
 * and aws_logs_msg_put_batch() to send multiple events.
 *
 * aws_logs_get_last_status() always returns the last return code.
 * aws_logs_get_last_error() returns the last status message, this is
 * only useful for return codes != 0.
 *
 * The last call should be an aws_shutdown() to clean up the controller object.
 */


CloudWatchLogsController* aws_init(const char *region, const char *group_name, const char *stream_name);
int aws_logs_ensure(CloudWatchLogsController *ctl);
void aws_shutdown(CloudWatchLogsController *ctl);

int aws_logs_msg_put(CloudWatchLogsController *ctl, const char *msg) __attribute_deprecated__;
int aws_logs_msg_put_batch(CloudWatchLogsController *ctl, const char *msg[], unsigned int msg_count);
int aws_logs_get_last_status(CloudWatchLogsController *ctl);
char* aws_logs_get_last_error(CloudWatchLogsController *ctl);

#ifdef __cplusplus
}
#endif

#endif /* RSYSLOG_AWSLIB_LIBRARY_H */
