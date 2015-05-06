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
#include <cutils/properties.h>
#include <utils/threads.h>


class ModemLogController {
private:
    pid_t mLoggingPid;
    int   mLoggingFd;
    char * g_outputFileName;
public:

    ModemLogController();
    virtual ~ModemLogController();

    bool setModemLogPath();
    bool isLoggingStarted();
    bool startModemLogging();
    bool stopModemLogging();

private:
    bool setupOutput();
    void clearOutput();
    int create_dir(const char * path);
};
