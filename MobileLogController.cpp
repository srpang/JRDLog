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
#include <arpa/inet.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <errno.h>
#include <fcntl.h>
#include <log/event_tag_map.h>
#include <logwrap/logwrap.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "MobileLogController.h"

#define LOG_DEVICE_DIR  "/dev/log/"
#define LOG_FILE_DIR    "/storage/sdcard0/jrdlog/mobilelog"
#define DEFAULT_LOG_ROTATE_SIZE_KBYTES 10240
#define DEFAULT_MAX_ROTATED_LOGS 4

MobileLogController::MobileLogController() {
    sJrdLogcatCtrl = new JrdLogcat();
    memset(logdir, 0, sizeof(logdir));
    strcpy(logdir, LOG_FILE_DIR);
}

MobileLogController::~MobileLogController() {

}

bool MobileLogController::startMobileLogging() {
    pid_t pid;
    int pipefd[2];

	if (mLoggingPid != 0) {
        ALOGE("MobileLogging already started");
        errno = EBUSY;
        return false;
    }

	ALOGD("Starting MobileLogging");
    if (sJrdLogcatCtrl.devices == NULL) {
        setDevices();
    }

    if (!openDevices())
        return false;

    if (!setupOutput())
        return false;

    if (pipe(pipefd) < 0) {
        ALOGE("pipe failed (%s)", strerror(errno));
        return false;
    }

    /*
     * TODO: Create a monitoring thread to handle and restart
     * the daemon if it exits prematurely
     */
    if ((pid = fork()) < 0) {
        ALOGE("fork failed (%s)", strerror(errno));
        close(pipefd[0]);
        close(pipefd[1]);
        return false;
    }

    if (!pid) {
        close(pipefd[1]);
        if (pipefd[0] != STDIN_FILENO) {
            if (dup2(pipefd[0], STDIN_FILENO) != STDIN_FILENO) {
                ALOGE("dup2 failed (%s)", strerror(errno));
                return false;
            }
            close(pipefd[0]);
        }
		
		sJrdLogcatCtrl->start();
    } else {
        close(pipefd[0]);
        mLoggingPid = pid;
        mLoggingFd  = pipefd[1];

        ALOGD("MobileLogging Thread running");

		return true;
    }

	
}

bool MobileLogController::stopMobileLogging() {
	closeDevices();
	clearOutput();
	
    if (mLoggingPid == 0) {
        ALOGE("MobileLogging already stopped");
        return false;
    }

    ALOGD("Stopping MobileLogging thread");

    kill(mLoggingPid, SIGTERM);
    waitpid(mLoggingPid, NULL, 0);
    mLoggingPid = 0;
    close(mLoggingFd);
    mLoggingFd = -1;
    ALOGD("MobileLogging thread stopped");
    return true;
}

bool MobileLogController::isLoggingStarted() {

}

void MobileLogController::setDevices() {
    int i = 0;
    int arry_size = sizeof(deviceArray) / sizeof(deviceArray[0]);
    log_device_t* dev;
    bool needBinary = false;

    if (arry_size > maxDevType) {
        perror ("device number is not right");
        exit(-1);
    }

    for (i=0; i<arry_size; i++) {
        char* buf = (char*) malloc(strlen(LOG_DEVICE_DIR) + strlen(deviceArray[i]) + 1);
        strcpy(buf, LOG_DEVICE_DIR);
        strcat(buf, deviceArray[i]);

        needBinary = strcmp(deviceArray[i], "events") == 0;

        if (sJrdLogcatCtrl.devices) {
            dev = sJrdLogcatCtrl.devices;
            while (dev->next) {
                dev = dev->next;
            }
            dev->next = new log_device_t(buf, needBinary, * (deviceArray[i] + 0), i);
        } else {
            sJrdLogcatCtrl.devices = new log_device_t(buf, needBinary, * (deviceArray[i] + 0), i);
        }
        sJrdLogcatCtrl.g_devCount++;
    }

    if (needBinary)
        sJrdLogcatCtrl.g_eventTagMap = android_openEventTagMap(EVENT_TAG_MAP_FILE);

}

bool MobileLogController::openDevices() {
    log_device_t* dev;
    int mode = O_RDONLY;

    dev = sJrdLogcatCtrl.devices;
    while (dev) {
        dev->fd = open(dev->device, mode);
        if (dev->fd < 0) {
            fprintf(stderr, "Unable to open log device '%s': %s\n",
                dev->device, strerror(errno));
            return false;
        }
    }

    return true;
}

void MobileLogController::closeDevices() {
	log_device_t* dev;
    dev = sJrdLogcatCtrl.devices;
    while (dev) {
        if (dev->fd > 0) {
            close(dev->fd);
			dev->fd = -1;
        }
    }
}
bool MobileLogController::setupOutput() {
    struct stat statbuf;
    time_t now = time(NULL);
	char date[80] = {0};
    int index;
    strftime(date, sizeof(date), "%Y_%m%d_%H_%M_%S", localtime(&now));

	for (index=0; index<maxDevType; index++) {
		char* buf = (char*) malloc(strlen(LOG_FILE_DIR) + strlen("/APLog_") + strlen(date) + strlen("/")
		                                                + strlen(sJrdLogcatCtrl.logArray[i]) + 1);
		strcpy(buf, LOG_FILE_DIR);
		strcat(buf, "/APLog_");
		strcat(buf, date);
		strcat(buf, "/");
		strcat(buf, sJrdLogcatCtrl.logArray[i]);

		sJrdLogcatCtrl.g_outputFileName[index] = buf;

	    sJrdLogcatCtrl.g_outFD[index] = openLogFile (sJrdLogcatCtrl.g_outputFileName[index]);

	    if (sJrdLogcatCtrl.g_outFD[index] < 0) {
	        perror ("couldn't open output file");
	        return false;
	    }

	    fstat(sJrdLogcatCtrl.g_outFD[index], &statbuf);

	    sJrdLogcatCtrl.g_outByteCount[index] = statbuf.st_size;
		
	}

}

void MobileLogController::clearOutput() {
	int index;

	for (index=0; index<maxDevType; index++) {
		free(sJrdLogcatCtrl.g_outputFileName[index]);
		sJrdLogcatCtrl.g_outputFileName[index] = NULL;
		close(sJrdLogcatCtrl.g_outFD[index]);
		sJrdLogcatCtrl.g_outFD[index] = -1;
		sJrdLogcatCtrl.g_outByteCount[index] = 0;
	}
}

int MobileLogController::openLogFile (const char *pathname) {
    return open(pathname, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
}

MobileLogController::JrdLogcat::JrdLogcat(){
	int index;
	
	for (index=0; index<maxDevType; index++) {
	    g_logRotateSizeKBytes[index] = DEFAULT_LOG_ROTATE_SIZE_KBYTES;
	    g_maxRotatedLogs[index] = DEFAULT_MAX_ROTATED_LOGS;
	    g_outputFileName[index] = NULL;
	}

    g_devCount = 0;
    g_logformat = android_log_format_new();
}

bool MobileLogController::JrdLogcat::start() {
    readLogLines(devices);
}

bool MobileLogController::JrdLogcat::stop() {

}

void MobileLogController::JrdLogcat::readLogLines(log_device_t* paradevices) {
    log_device_t* dev;
    int max = 0;
    int ret;
    int queued_lines = 0;
    bool sleep = false;

    int result;
    fd_set readset;

    for (dev=paradevices; dev; dev = dev->next) {
        if (dev->fd > max) {
            max = dev->fd;
        }
    }

    while (1) {
        do {
            timeval timeout = { 0, 5000 /* 5ms */ }; // If we oversleep it's ok, i.e. ignore EINTR.
            FD_ZERO(&readset);
            for (dev=paradevices; dev; dev = dev->next) {
                FD_SET(dev->fd, &readset);
            }
            result = select(max + 1, &readset, NULL, NULL, sleep ? NULL : &timeout);
        } while (result == -1 && errno == EINTR);

        if (result >= 0) {
            for (dev=paradevices; dev; dev = dev->next) {
                if (FD_ISSET(dev->fd, &readset)) {
                    queued_entry_t* entry = new queued_entry_t();
                    /* NOTE: driver guarantees we read exactly one full entry */
                    ret = read(dev->fd, entry->buf, LOGGER_ENTRY_MAX_LEN);
                    if (ret < 0) {
                        if (errno == EINTR) {
                            delete entry;
                            goto next;
                        }
                        if (errno == EAGAIN) {
                            delete entry;
                            break;
                        }
                        perror("logcat read");
                        exit(EXIT_FAILURE);
                    }
                    else if (!ret) {
                        fprintf(stderr, "read: Unexpected EOF!\n");
                        exit(EXIT_FAILURE);
                    }
                    else if (entry->entry.len != ret - sizeof(struct logger_entry)) {
                        fprintf(stderr, "read: unexpected length. Expected %d, got %d\n",
                                entry->entry.len, ret - sizeof(struct logger_entry));
                        exit(EXIT_FAILURE);
                    }

                    entry->entry.msg[entry->entry.len] = '\0';

                    dev->enqueue(entry);
                    entry_num++;
                    if (entry_num >= LOG_TRIGGER_WATERLEVEL) {
                        entry_too_much = true;
                    }
                    ++queued_lines;
                }
            }

            if (result == 0) {
                // we did our short timeout trick and there's nothing new
                // print everything we have and wait for more data
                sleep = true;
                while (true) {
                    chooseFirst(paradevices, &dev);
                    if (dev == NULL) {
                        break;
                    }
                    printNextEntry(dev);
                    --queued_lines;
                }
            } else {
                // print all that aren't the last in their list
                sleep = false;
                while (true) {
                    chooseFirst(paradevices, &dev);
                    if (dev == NULL || dev->queue->next == NULL) {
                        if (entry_too_much) {
                            trigger_log(dev);
                        } else {
                            break;
                        }
                    }

                    printNextEntry(dev);
                    --queued_lines;
                }
            }
        }
next:
        ;
    }
}

void MobileLogController::JrdLogcat::chooseFirst(log_device_t* dev, log_device_t** firstdev) {
    for (*firstdev = NULL; dev != NULL; dev = dev->next) {
        if (dev->queue != NULL && (*firstdev == NULL || cmp(dev->queue, (*firstdev)->queue) < 0)) {
            *firstdev = dev;
        }
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

    g_outFD[dev_log] = openLogFile (g_outputFileName[dev_log]);

    if (g_outFD[dev_log] < 0) {
        perror ("couldn't open output file");
        exit(-1);
    }

    g_outByteCount[dev_log] = 0;

}

void MobileLogController::JrdLogcat::skipNextEntry(log_device_t* dev) {
    maybePrintStart(dev);
    queued_entry_t* entry = dev->queue;
    dev->queue = entry->next;
    delete entry;
    entry_num--;
    if(entry_num < LOG_TRIGGER_WATERLEVEL) {
        entry_too_much = false;
    }
}

void MobileLogController::JrdLogcat::processBuffer(log_device_t* dev, struct logger_entry *buf)
{
    int bytesWritten = 0;
    int err;
    AndroidLogEntry entry;
    char binaryMsgBuf[1024];
	int index = dev->devType;

    if (dev->binary) {
        err = android_log_processBinaryLogBuffer(buf, &entry, g_eventTagMap,
                binaryMsgBuf, sizeof(binaryMsgBuf));
        //printf(">>> pri=%d len=%d msg='%s'\n",
        //    entry.priority, entry.messageLen, entry.message);
    } else {
        err = android_log_processLogBuffer(buf, &entry);
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
        rotateLogs();
    }

error:
    //fprintf (stderr, "Error processing record\n");
    return;
}

void MobileLogController::JrdLogcat::printNextEntry(log_device_t* dev) {
    maybePrintStart(dev);
    processBuffer(dev, &dev->queue->entry);
    skipNextEntry(dev);
}

/*
 * Trigger Log when there are too many entries in memory
 */
void MobileLogController::JrdLogcat::constructEntry(queued_entry_t* entry){
    char trigger_tag[] = {"AEE"};
    char trigger_message[] = {"trigger log"};

    struct timeval now;
    gettimeofday(&now, NULL);
    entry->entry.len = 1 + sizeof(trigger_tag) + sizeof(trigger_message);
    entry->entry.pid = 0;
    entry->entry.tid = 0;
    entry->entry.msg[0] = 0x5;
    memcpy(entry->buf + sizeof(logger_entry) + 1, trigger_tag, sizeof(trigger_tag));
    memcpy(entry->buf + sizeof(logger_entry) + sizeof(trigger_tag) + 1, trigger_message, sizeof(trigger_message));
    entry->entry.sec = now.tv_sec;
    entry->entry.nsec = now.tv_usec * 1000;
}

void MobileLogController::JrdLogcat::trigger_log(log_device_t *dev) {
    queued_entry_t* entry = new queued_entry_t();
    constructEntry(entry);
    dev->enqueue(entry);
    entry_num++;
    if (entry_num >= LOG_TRIGGER_WATERLEVEL) {
        entry_too_much = true;
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

