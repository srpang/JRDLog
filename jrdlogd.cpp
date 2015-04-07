// Copyright 2006-2014 The Android Open Source Project

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <dirent.h>

#include "cutils/log.h"

#include "CommandListener.h"


int main()
{
    CommandListener *cl;

    signal(SIGPIPE, exit);

    cl = new CommandListener();

    /*
     * Now that we're up, we can respond to commands
     */
    if (cl->startListener()) {
        ALOGE("Unable to start CommandListener (%s)", strerror(errno));
        exit(1);
    }

    // Eventually we'll become the monitoring thread
    while(1) {
        sleep(1000);
    }

    ALOGI("JrdLogd exiting");
    exit(0);

}

