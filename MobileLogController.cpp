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

#include <stdlib.h>
#include <stdio.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <errno.h>
#include <fcntl.h>
#include <log/event_tag_map.h>
#include <string.h>
#include <sys/klog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "MobileLogController.h"

#define LOG_DEVICE_DIR  ""
#define LOG_FILE_DIR    "/storage/sdcard0/jrdlog/mobilelog"
#define DEFAULT_LOG_ROTATE_SIZE_KBYTES 10240
#define DEFAULT_MAX_ROTATED_LOGS 4

#define FALLBACK_KLOG_BUF_SHIFT    17    /* CONFIG_LOG_BUF_SHIFT from our kernel */
#define FALLBACK_KLOG_BUF_LEN    (1 << FALLBACK_KLOG_BUF_SHIFT)

const char * MobileLogController::deviceArray[] = {"main", "system", "radio", "events"};
const char * MobileLogController::JrdLogcat::logArray[] = {"main_log", "sys_log", "radio_log", "events_log"};

MobileLogController::JrdLogcat* MobileLogController::sJrdLogcatCtrl = NULL;

MobileLogController::MobileLogController() {
    sJrdLogcatCtrl = new JrdLogcat(this);
    mLoggingThread = 0;
    mLogEnable = false;
}

MobileLogController::~MobileLogController() {

}

bool MobileLogController::startMobileLogging() {

    if (mLoggingThread != 0) {
        ALOGE("MobileLogging already started");
        errno = EBUSY;
        return false;
    }

    ALOGD("Starting MobileLogging");
    if (sJrdLogcatCtrl->devices == NULL) {
        if (!setDevices())
            return false;
    }

    if (!openDevices())
        return false;

    if (!setupOutput())
        return false;

    mLogEnable = true;
    if (pthread_create(&mLoggingThread, NULL, threadStart, this)) {
        ALOGE("pthread_create (%s)", strerror(errno));
        return false;
    }

    return true;
}

bool MobileLogController::stopMobileLogging() {

    closeDevices();
    clearOutput();

    if (mLoggingThread == 0) {
        ALOGE("Thread already stopped");
        return true;
    }

    ALOGD("Stopping MobileLogging thread");
    mLogEnable = false;
    void *ret;
    if (pthread_join(mLoggingThread, &ret)) {
        ALOGE("Error joining to listener thread (%s)", strerror(errno));
        mLoggingThread = 0;
        return false;
    }

    mLoggingThread = 0;
    return true;
}

bool MobileLogController::isLoggingStarted() {
    ALOGE("mLoggingThread %d", mLoggingThread);
    return (mLoggingThread != 0);
}

void *MobileLogController::threadStart(void *obj) {
    MobileLogController *me = reinterpret_cast<MobileLogController *>(obj);

    me->sJrdLogcatCtrl->start();
    pthread_exit(NULL);
    return NULL;
}

bool MobileLogController::setDevices() {
    int i = 0;
    int arry_size = sizeof(deviceArray) / sizeof(deviceArray[0]);
    log_device_t* dev;
    bool needBinary = false;

    if (arry_size > MAX_DEV_LOG_TYPE) {
        ALOGE ("device number is not right");
        return false;
    }

    for (i=0; i<arry_size; i++) {
        bool binary = false;
        char* buf = (char*) malloc(strlen(LOG_DEVICE_DIR) + strlen(deviceArray[i]) + 1);
        strcpy(buf, LOG_DEVICE_DIR);
        strcat(buf, deviceArray[i]);

        binary = strcmp(deviceArray[i], "events") == 0;
        if (binary) {
            needBinary = true;
        }

        if (sJrdLogcatCtrl->devices) {
            dev = sJrdLogcatCtrl->devices;
            while (dev->next) {
                dev = dev->next;
            }
            dev->next = new log_device_t(buf, binary, * (deviceArray[i] + 0), (DeviceType)i);
        } else {
            sJrdLogcatCtrl->devices = new log_device_t(buf, binary, * (deviceArray[i] + 0), (DeviceType)i);
        }
        sJrdLogcatCtrl->g_devCount++;
    }

    if (needBinary)
        sJrdLogcatCtrl->g_eventTagMap = android_openEventTagMap(EVENT_TAG_MAP_FILE);

    return true;

}

bool MobileLogController::openDevices() {
    log_device_t* dev;
    dev = sJrdLogcatCtrl->devices;
    int mode = O_RDONLY;
    log_time tail_time(log_time::EPOCH);
    unsigned int tail_lines = 0;

    if (tail_time != log_time::EPOCH) {
        sJrdLogcatCtrl->logger_list = android_logger_list_alloc_time(mode, tail_time, 0);
    } else {
        sJrdLogcatCtrl->logger_list = android_logger_list_alloc(mode, tail_lines, 0);
    }
    while (dev) {
        dev->logger_list = sJrdLogcatCtrl->logger_list;
        dev->logger = android_logger_open(sJrdLogcatCtrl->logger_list,
                                          android_name_to_log_id(dev->device));
        if (!dev->logger) {
            fprintf(stderr, "Unable to open log device '%s'\n", dev->device);
            return false;
        }

        dev = dev->next;
    }

    return true;
}

void MobileLogController::closeDevices() {
    if (sJrdLogcatCtrl->logger_list != NULL) {
        android_logger_list_free(sJrdLogcatCtrl->logger_list);
        sJrdLogcatCtrl->logger_list = NULL;
    }
}

bool MobileLogController::setupOutput() {
    struct stat statbuf;
    time_t now = time(NULL);
    char date[80] = {0};
    char dir_path[255] = {0};
    int index;
    strftime(date, sizeof(date), "%Y_%m%d_%H_%M_%S", localtime(&now));
    char* buf = NULL;

    strcpy(dir_path, LOG_FILE_DIR);
    strcat(dir_path, "/APLog_");
    strcat(dir_path, date);

    if (create_dir(dir_path) != 0) {
        perror ("couldn't create the output directory");
        return false;
    }

    for (index=0; index<MAX_DEV_LOG_TYPE; index++) {
        buf = (char*) malloc(strlen(dir_path) + strlen("/")
                                              + strlen(sJrdLogcatCtrl->logArray[index]) + 1);
        strcpy(buf, dir_path);
        strcat(buf, "/");
        strcat(buf, sJrdLogcatCtrl->logArray[index]);

        sJrdLogcatCtrl->g_outputFileName[index] = buf;

        sJrdLogcatCtrl->g_outFD[index] = openLogFile (sJrdLogcatCtrl->g_outputFileName[index]);

        if (sJrdLogcatCtrl->g_outFD[index] < 0) {
            perror ("couldn't open output file");
            return false;
        }

        fstat(sJrdLogcatCtrl->g_outFD[index], &statbuf);

        sJrdLogcatCtrl->g_outByteCount[index] = statbuf.st_size;
        
    }

    //set Kernel info
    buf = (char*) malloc(strlen(dir_path) + strlen("/")
                                          + strlen("kernel_log") + 1);
    strcpy(buf, dir_path);
    strcat(buf, "/");
    strcat(buf, "kernel_log");
    sJrdLogcatCtrl->g_kernelOutputFileName = buf;
    sJrdLogcatCtrl->g_kernelOutFD = openLogFile(sJrdLogcatCtrl->g_kernelOutputFileName);
    if (sJrdLogcatCtrl->g_kernelOutFD < 0) {
        perror ("couldn't open kernel output file");
        return false;
    }
    fstat(sJrdLogcatCtrl->g_kernelOutFD, &statbuf);
    sJrdLogcatCtrl->g_kernelOutByteCount = statbuf.st_size;

    return true;
}

void MobileLogController::clearOutput() {
    int index;

    for (index=0; index<MAX_DEV_LOG_TYPE; index++) {
        if (sJrdLogcatCtrl->g_outputFileName[index] != NULL) {
            free(sJrdLogcatCtrl->g_outputFileName[index]);
            sJrdLogcatCtrl->g_outputFileName[index] = NULL;
        }

        if (sJrdLogcatCtrl->g_outFD[index] != -1) {
            close(sJrdLogcatCtrl->g_outFD[index]);
            sJrdLogcatCtrl->g_outFD[index] = -1;
        }

        sJrdLogcatCtrl->g_outByteCount[index] = 0;
    }

    //set kernel info
    if (sJrdLogcatCtrl->g_kernelOutputFileName != NULL) {
        free(sJrdLogcatCtrl->g_kernelOutputFileName);
        sJrdLogcatCtrl->g_kernelOutputFileName = NULL;
    }

    if (sJrdLogcatCtrl->g_kernelOutFD != -1) {
        close(sJrdLogcatCtrl->g_kernelOutFD);
        sJrdLogcatCtrl->g_kernelOutFD = -1;
    }

    sJrdLogcatCtrl->g_kernelOutByteCount = 0;
    
}

int MobileLogController::openLogFile (char *pathname) {
    return open(pathname, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
}

int MobileLogController::create_dir(const char * path) {
    int i = 0;
    int ret = -1;
    char dir_name[256];
    mode_t mode = S_IRWXU | S_IRWXG | S_IROTH | S_IWOTH;
    mode_t oldmod = umask(0);
    int len = strlen(path);

    strcpy(dir_name,path);
    if(dir_name[len-1] != '/')
        strcat(dir_name,"/");

    len = strlen(dir_name);
    for(i = 1; i < len; i++){
        if(dir_name[i] == '/'){
            dir_name[i] = 0;
            if((ret = access(dir_name, F_OK | R_OK)) != 0){
                if(mkdir(dir_name, mode) != 0){
                    ALOGE("make %s failed: %d %s\n", dir_name, errno, strerror(errno));
                    umask(oldmod);
                    return -1;
                }
            }
            dir_name[i] = '/';
        }
    }
    umask(oldmod);
    return 0;
}

MobileLogController::JrdLogcat::JrdLogcat(MobileLogController* obj){
    int index;
    baseControl = obj;
    for (index=0; index<MAX_DEV_LOG_TYPE; index++) {
        g_logRotateSizeKBytes[index] = DEFAULT_LOG_ROTATE_SIZE_KBYTES;
        g_maxRotatedLogs[index] = DEFAULT_MAX_ROTATED_LOGS;
        g_outputFileName[index] = NULL;
    }

    logger_list = NULL;
    g_devCount = 0;
    g_logformat = android_log_format_new();

    //set Kernel info
    g_kernelLogRotateSizeKBytes = DEFAULT_LOG_ROTATE_SIZE_KBYTES;
    g_kernelMaxRotatedLogs = DEFAULT_MAX_ROTATED_LOGS;
    g_kernelOutputFileName = NULL;
    
}

void MobileLogController::JrdLogcat::start() {
    int err = setLogFormat ("time");
    if (err < 0) {
        fprintf(stderr,"Invalid parameter to -v\n");
        exit(-1);
    }
    readLogLines(devices);
}

void MobileLogController::JrdLogcat::stop() {

}

int MobileLogController::JrdLogcat::setLogFormat(const char * formatString) {
    static AndroidLogPrintFormat format;

    format = android_log_formatFromString(formatString);

    if (format == FORMAT_OFF) {
        // FORMAT_OFF means invalid string
        return -1;
    }

    android_log_setPrintFormat(g_logformat, format);

    return 0;
}

void MobileLogController::JrdLogcat::readLogLines(log_device_t* paradevices) {
    log_device_t* dev;
    int max = 0;
    int ret;
    int queued_lines = 0;
    bool sleep = false;

    int result;
    fd_set readset;

    while (1) {
        struct log_msg log_msg;

        if (baseControl->mLogEnable == false) {
            break;
        }
        
        int ret = android_logger_list_read(logger_list, &log_msg);

        if (ret == 0) {
            fprintf(stderr, "read: Unexpected EOF!\n");
            exit(-1);
        }

        if (ret < 0) {
            if (ret == -EAGAIN) {
                break;
            }

            if (ret == -EIO) {
                fprintf(stderr, "read: Unexpected EOF!\n");
                exit(-1);
            }
            if (ret == -EINVAL) {
                fprintf(stderr, "read: unexpected length.\n");
                exit(-1);
            }
            perror("logcat read failure");
            exit(-1);
        }

        for(dev = devices; dev; dev = dev->next) {
            if (android_name_to_log_id(dev->device) == log_msg.id()) {
                break;
            }
        }
        if (!dev) {
            fprintf(stderr, "read: Unexpected log ID!\n");
            exit(-1);
        }

        maybePrintStart(dev);
        processBuffer(dev, &log_msg);
        processKernelBuffer();

    }
}

void MobileLogController::JrdLogcat::rotateLogs(int dev_log)
{
    int err;

    // Can't rotate logs if we're not outputting to a file
    if (g_outputFileName[dev_log] == NULL) {
        return;
    }

    close(g_outFD[dev_log]);

    for (int i = g_maxRotatedLogs[dev_log] ; i > 0 ; i--) {
        char *file0, *file1;

        asprintf(&file1, "%s.%d", g_outputFileName[dev_log], i);

        if (i - 1 == 0) {
            asprintf(&file0, "%s", g_outputFileName[dev_log]);
        } else {
            asprintf(&file0, "%s.%d", g_outputFileName[dev_log], i - 1);
        }

        err = rename (file0, file1);

        if (err < 0 && errno != ENOENT) {
            perror("while rotating log files");
        }

        free(file1);
        free(file0);
    }

    g_outFD[dev_log] = openLogFile(g_outputFileName[dev_log]);

    if (g_outFD[dev_log] < 0) {
        perror ("couldn't open output file");
        exit(-1);
    }

    g_outByteCount[dev_log] = 0;

}

void MobileLogController::JrdLogcat::rotateKernelLogs()
{
    int err;

    // Can't rotate logs if we're not outputting to a file
    if (g_kernelOutputFileName == NULL) {
        return;
    }

    close(g_kernelOutFD);

    for (int i = g_kernelMaxRotatedLogs ; i > 0 ; i--) {
        char *file0, *file1;

        asprintf(&file1, "%s.%d", g_kernelOutputFileName, i);

        if (i - 1 == 0) {
            asprintf(&file0, "%s", g_kernelOutputFileName);
        } else {
            asprintf(&file0, "%s.%d", g_kernelOutputFileName, i - 1);
        }

        err = rename (file0, file1);

        if (err < 0 && errno != ENOENT) {
            perror("while rotating log files");
        }

        free(file1);
        free(file0);
    }

    g_kernelOutFD = openLogFile(g_kernelOutputFileName);

    if (g_kernelOutFD < 0) {
        perror ("couldn't open output file");
        exit(-1);
    }

    g_kernelOutByteCount = 0;

}

void MobileLogController::JrdLogcat::processBuffer(log_device_t* dev, struct log_msg *buf) {
    int bytesWritten = 0;
    int err;
    AndroidLogEntry entry;
    char binaryMsgBuf[1024];
    int index = dev->devType;

    if (dev->binary) {
        err = android_log_processBinaryLogBuffer(&buf->entry_v1, &entry,
                                                 g_eventTagMap,
                                                 binaryMsgBuf,
                                                 sizeof(binaryMsgBuf));
        //printf(">>> pri=%d len=%d msg='%s'\n",
        //    entry.priority, entry.messageLen, entry.message);
    } else {
        err = android_log_processLogBuffer(&buf->entry_v1, &entry);
    }

    if (err < 0) {
        goto error;
    }

    if (android_log_shouldPrintLine(g_logformat, entry.tag, entry.priority)) {
        if (false && g_devCount > 1) {
            binaryMsgBuf[0] = dev->label;
            binaryMsgBuf[1] = ' ';
            bytesWritten = write(g_outFD[index], binaryMsgBuf, 2);
            if (bytesWritten < 0) {
                perror("output error");
                exit(-1);
            }
        }

        bytesWritten = android_log_printLogLine(g_logformat, g_outFD[index], &entry);

        if (bytesWritten < 0) {
            perror("output error");
            exit(-1);
        }
    }

    g_outByteCount[index] += bytesWritten;

    if (g_logRotateSizeKBytes[index] > 0 && (g_outByteCount[index] / 1024) >= g_logRotateSizeKBytes[index]) {
        rotateLogs(index);
    }

error:
    //fprintf (stderr, "Error processing record\n");
    return;
}

void MobileLogController::JrdLogcat::processKernelBuffer() {
    char *buffer;
    int ret;
    int n, op, klog_buf_len;

    klog_buf_len = klogctl(KLOG_SIZE_BUFFER, 0, 0);

    if (klog_buf_len <= 0) {
        klog_buf_len = FALLBACK_KLOG_BUF_LEN;
    }

    buffer = (char *)malloc(klog_buf_len + 1);

    if (!buffer) {
        perror("malloc");
        exit(-1);
    }

    op = KLOG_READ_CLEAR;
    n = klogctl(op, buffer, klog_buf_len);
    if (n < 0) {
        perror("klogctl");
        exit(-1);
    }
    buffer[n] = '\0';

    do {
        ret = write(g_kernelOutFD, buffer, n);
    } while (ret < 0 && errno == EINTR);

    free(buffer);
    if (ret < 0) {
        fprintf(stderr, "+++ LOG: write failed (errno=%d)\n", errno);
        ret = 0;
        exit(-1);
    }

    if (ret < n) {
        fprintf(stderr, "+++ LOG: write partial (%d of %d)\n", ret,
                (int)n);
        exit(-1);
    }

    g_kernelOutByteCount += ret;

    if (g_kernelLogRotateSizeKBytes > 0 && (g_kernelOutByteCount / 1024) >= g_kernelLogRotateSizeKBytes) {
        rotateKernelLogs();
    }   

}

void MobileLogController::JrdLogcat::maybePrintStart(log_device_t* dev) {
    if (!dev->printed) {
        dev->printed = true;
        if (g_devCount > 1) {
            char buf[1024];
            snprintf(buf, sizeof(buf), "--------- beginning of %s\n", dev->device);
            if (write(g_outFD[dev->devType], buf, strlen(buf)) < 0) {
                perror("output error");
                exit(-1);
            }
        }
    }
}

int MobileLogController::JrdLogcat::openLogFile (char *pathname) {
    return open(pathname, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
}

