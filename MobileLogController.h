/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <log/logger.h>
#include <log/logd.h>
#include <log/logprint.h>
#include <log/event_tag_map.h>
#include <cutils/sockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <cutils/properties.h>

#define LOG_TRIGGER_WATERLEVEL 10000
typedef enum {
    DEV_MAIN=0,
    DEV_SYSTEM,
    DEV_RADIO,
    DEV_EVENTS,
    DEV_MAXTYPE,
} DeviceType;

#define MAX_DEV_LOG_TYPE DEV_MAXTYPE

struct log_device_t {
    const char* device;
    bool binary;
    struct logger *logger;
    struct logger_list *logger_list;
    bool printed;
    char label;
    DeviceType devType;

    log_device_t* next;

    log_device_t(const char* d, bool b, char l, DeviceType index) {
        device = d;
        binary = b;
        label = l;
        devType = index;
        next = NULL;
        printed = false;
    }
};

class MobileLogController {
public:
    class JrdLogcat {
    public:
        static const char* logArray[];
        log_device_t* devices = NULL;
        AndroidLogFormat * g_logformat;
        EventTagMap* g_eventTagMap = NULL;

        struct logger_list *logger_list;
        int g_devCount;
        char * g_outputFileName[MAX_DEV_LOG_TYPE];
        int g_logRotateSizeKBytes[MAX_DEV_LOG_TYPE];
        int g_maxRotatedLogs[MAX_DEV_LOG_TYPE];

        int g_outFD[MAX_DEV_LOG_TYPE] = {-1};
        off_t g_outByteCount[MAX_DEV_LOG_TYPE] = {0};

        char * g_kernelOutputFileName;
        int g_kernelLogRotateSizeKBytes;
        int g_kernelMaxRotatedLogs;
        int g_kernelOutFD = -1;
        off_t g_kernelOutByteCount = 0;

        JrdLogcat();
        virtual ~JrdLogcat() {}
        
        void start();
        void stop();

    private:

        int setLogFormat(const char * formatString);

        void readLogLines(log_device_t* devices);
        

        void trigger_log(log_device_t *dev);
        void maybePrintStart(log_device_t *dev);
        void processBuffer(log_device_t* dev, struct log_msg *buf);
        void processKernelBuffer();
        void rotateLogs(int dev_log);
        void rotateKernelLogs();
        int openLogFile (char *pathname);
    };
private:

    static JrdLogcat* sJrdLogcatCtrl;
    static const char* deviceArray[];
    pid_t mLoggingPid;
    int   mLoggingFd;

public:

    MobileLogController();
    virtual ~MobileLogController();

    bool setMobileLogPath();
    bool isLoggingStarted();
    bool startMobileLogging();
    bool stopMobileLogging();

private:
    bool setDevices();
    bool openDevices();
    void closeDevices();
    bool setupOutput();
    void clearOutput();
    int openLogFile (char *pathname);


};
