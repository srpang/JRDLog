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

#ifndef _MOBILELOG_CONTROLLER_H
#define _MOBILELOG_CONTROLLER_H

#include <log/logger.h>
#include <log/logd.h>
#include <log/logprint.h>
#include <log/event_tag_map.h>
#include <cutils/sockets.h>

#define LOG_TRIGGER_WATERLEVEL 10000
enum DeviceType {
    main=0,
    system,
    radio,
    events,
    maxDevType,
};

#define MAX_LOG_TYPE maxDevType;

struct queued_entry_t {
    union {
        unsigned char buf[LOGGER_ENTRY_MAX_LEN + 1] __attribute__((aligned(4)));
        struct logger_entry entry __attribute__((aligned(4)));
    };
    queued_entry_t* next;

    queued_entry_t() {
        next = NULL;
    }
};

static int cmp(queued_entry_t* a, queued_entry_t* b) {
    int n = a->entry.sec - b->entry.sec;
    if (n != 0) {
        return n;
    }
    return a->entry.nsec - b->entry.nsec;
}

struct log_device_t {
    char* device;
    bool binary;
    int fd;
    bool printed;
    char label;
	DeviceType devType;

    queued_entry_t* queue;
    log_device_t* next;

    log_device_t(char* d, bool b, char l, DeviceType index) {
        device = d;
        binary = b;
        label = l;
		devType = index;
        queue = NULL;
        next = NULL;
        printed = false;
    }

    void enqueue(queued_entry_t* entry) {
        if (this->queue == NULL) {
            this->queue = entry;
        } else {
            queued_entry_t** e = &this->queue;
            while (*e && cmp(entry, *e) >= 0) {
                e = &((*e)->next);
            }
            entry->next = *e;
            *e = entry;
        }
    }
};

class MobileLogController {
private:
	static JrdLogcat *sJrdLogcatCtrl;

	char deviceArray[] = {"main", "system", "radio", "events"};
	char logdir[255];
	pid_t mLoggingPid;
	int   mLoggingFd;

public:

    MobileLogController();
    virtual ~MobileLogController();

	bool setMobileLogPath();

	bool startMobileLogging();
	bool stopMobileLogging();

private:
	void setDevices();
	bool openDevices();
	bool closeDevices();
	bool setupOutput();
	bool clearOutput();

    class JrdLogcat {
    public:
		log_device_t* devices = NULL;
		AndroidLogFormat * g_logformat;
		EventTagMap* g_eventTagMap = NULL;
		
		int g_devCount;
		char logArray[] = {"main_log", "sys_log", "radio_log", "events_log"};
		char * g_outputFileName[MAX_LOG_TYPE];
		int g_logRotateSizeKBytes[MAX_LOG_TYPE];
		int g_maxRotatedLogs[MAX_LOG_TYPE];

		int g_outFD[MAX_LOG_TYPE] = {-1};
		off_t g_outByteCount[MAX_LOG_TYPE] = 0;

        JrdLogcat();
        virtual ~JrdLogcat() {}
		
		bool start();
		bool stop();

	private:
		int entry_num = 0;
		bool entry_too_much = false;

        int setLogFormat(const char * formatString);

		void readLogLines(log_device_t* devices);
		
		void chooseFirst(log_device_t* dev, log_device_t** firstdev);
		void constructEntry(queued_entry_t* entry);
		void trigger_log(log_device_t *dev);
		void maybePrintStart(log_device_t *dev);
		void rotateLogs(int dev_log);
		void skipNextEntry(log_device_t* dev);
			
    };
};

#endif
