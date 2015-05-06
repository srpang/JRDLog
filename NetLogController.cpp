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

#include "NetLogController.h"
#include "ResponseCode.h"

#define LOG_DEVICE_DIR  ""
#define LOG_FILE_DIR    "/storage/sdcard0/jrdlog/netlog"
#define DEFAULT_LOG_ROTATE_SIZE_KBYTES 10240
#define DEFAULT_MAX_ROTATED_LOGS 4

NetLogController::NetLogController() {
    g_outputFileName = NULL;
    mLoggingPid = 0;
    mLoggingFd = -1;
}

NetLogController::~NetLogController() {

}

bool NetLogController::startNetLogging() {
    pid_t pid;
    int pipefd[2];

    if (mLoggingPid != 0) {
        ALOGE("NetLogging already started");
        errno = EBUSY;
        return false;
    }

    ALOGD("Starting NetLogging");

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
        //tcpdump -i any -p -s 0 -w /sdcard/capture.pcap
        char *args[] = {
            (char *) "/system/xbin/tcpdump",
            (char *) "-i",
            (char *) "any",
            (char *) "-p",
            (char *) "-s",
            (char *) "0",
            (char *) "-w",
            (char *) g_outputFileName,
            NULL
        };
        
        if (execv(args[0], args)) {
            ALOGE("execl failed (%s)", strerror(errno));
            return false;
        }
        ALOGE("Should never get here!");
        _exit(-1);
    } else {
        close(pipefd[0]);
        mLoggingPid = pid;
        mLoggingFd  = pipefd[1];

        ALOGD("NetLogging Thread running");

        return true;
    }

    return true;
}

bool NetLogController::stopNetLogging() {
    clearOutput();
    
    if (mLoggingPid == 0) {
        ALOGE("NetLog is already stopped");
        return false;
    }

    ALOGD("Stopping NetLogging thread");

    kill(mLoggingPid, SIGTERM);
    waitpid(mLoggingPid, NULL, 0);
    mLoggingPid = 0;
    close(mLoggingFd);
    mLoggingFd = -1;
    ALOGD("NetLogging thread stopped");
    return true;

}

bool NetLogController::isLoggingStarted() {
    ALOGD("mLoggingPid %d", mLoggingPid);
    return (mLoggingPid != 0);
}

bool NetLogController::setupOutput() {
    time_t now = time(NULL);
    char date[80] = {0};
    char dir_path[255] = {0};
    int index;
    strftime(date, sizeof(date), "%Y_%m%d_%H_%M_%S", localtime(&now));
    char* buf = NULL;

    //should not exceed 255
    strcpy(dir_path, LOG_FILE_DIR);
    strcat(dir_path, "/NTLog_");
    strcat(dir_path, date);

    if (create_dir(dir_path) != 0) {
        ALOGE("couldn't create the output directory");
        ResponseCode::resNetLogStatus = ResponseCode::FileAccessFailed;
        return false;
    }
    
    buf = (char*) malloc(strlen(dir_path) + strlen("/") + strlen("net_log.pcap") + 1);
    strcpy(buf, dir_path);
    strcat(buf, "/");
    strcat(buf, "net_log.pcap");

    g_outputFileName = buf;
    return true;
}

void NetLogController::clearOutput() {
    if (g_outputFileName != NULL) {
        free(g_outputFileName);
        g_outputFileName = NULL;
    }
}

int NetLogController::create_dir(const char * path) {
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
