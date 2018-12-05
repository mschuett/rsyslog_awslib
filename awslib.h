#ifndef RSYSLOG_AWSLIB_LIBRARY_H
#define RSYSLOG_AWSLIB_LIBRARY_H

#define RSYSLOG_AWSLIB_ERR_MSG_SIZE 256
#define RSYSLOG_AWSLIB_VERSION "0.1"
#define RSYSLOG_AWSLIB_TAG "rsyslog_awslib"


#ifdef __cplusplus
#include <aws/core/Aws.h>
#include <aws/core/Region.h>
#include <aws/core/utils/Outcome.h>
#include <aws/logs/CloudWatchLogsClient.h>

extern "C" {
/* C++ version with full type info for better compiler checks */
struct awslib_instance {
    Aws::SDKOptions *options;
    Aws::CloudWatchLogs::CloudWatchLogsClient *client;
    int last_status_code;
    char last_error_message[RSYSLOG_AWSLIB_ERR_MSG_SIZE];
    Aws::String seq_token;
};

#else /* __cplusplus */

/* opaque C version of the struct */
struct awslib_instance {
    void *options;
    void *client;
    int last_status_code;
    char last_error_message[RSYSLOG_AWSLIB_ERR_MSG_SIZE];
};
#endif

void hello();
struct awslib_instance* aws_init(const char *region);
void aws_shutdown(struct awslib_instance *);

int aws_logs_ensure(struct awslib_instance *inst, const char *group_name, const char *stream_name);
int aws_logs_msg_put(struct awslib_instance *inst, const char *group_name, const char *stream_name, const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* RSYSLOG_AWSLIB_LIBRARY_H */
