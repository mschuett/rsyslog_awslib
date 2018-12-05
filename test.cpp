/*
 * Non really a test, rather an executable usage example to try out
 * and debug the glue library without rsyslog.
 */

#include "awslib.h"

int main() {
    struct awslib_instance *awslib_inst;
    char *logGroup = (char*) "test";
    char *logStream = (char*) "test123";
    char *region = (char*) "eu-central-1";
    int rc;
    const char *logMessages[] = {
            "hello world",
            "this is a test",
            "and another one",
            NULL
    };

    awslib_inst = aws_init(region);
    rc = aws_logs_ensure(awslib_inst, logGroup, logStream);

    std::printf("aws_logs_ensure \t--> rc = %d; status = %d; message = '%s'\n",
            rc, awslib_inst->last_status_code, awslib_inst->last_error_message);
    if (rc) {
        aws_shutdown(awslib_inst);
        return rc;
    }

    const char* msg;
    int i = 0;
    while (NULL != (msg = logMessages[i++])) {
        rc = aws_logs_msg_put(awslib_inst, logGroup, logStream, msg);
        std::printf("aws_logs_msg_put\t--> rc = %d; status = %d; message = '%s'\n",
                    rc, awslib_inst->last_status_code, awslib_inst->last_error_message);
    }
    aws_shutdown(awslib_inst);
    return rc;
}