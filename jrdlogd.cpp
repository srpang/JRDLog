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
//#include "MobileLogController.h"

int main()
{
    CommandListener *cl;
    //MobileLogController *mlc;
    signal(SIGPIPE, exit);

    //int i = 0;

    //while (i <= 120) {
    //    i++;
    //    sleep(1);
    //}

    ALOGI("JrdLogd start CommandListener");
    cl = new CommandListener();
    //mlc = new MobileLogController();
    /*
     * Now that we're up, we can respond to commands
     */
    ALOGI("JrdLogd start Listening");
    if (cl->startListener()) {
        ALOGE("Unable to start CommandListener (%s)", strerror(errno));
        exit(1);
    }
    ALOGI("JrdLogd start logging");
    //mlc->startMobileLogging();
    // Eventually we'll become the monitoring thread
    while(1) {
        sleep(1);
    }

    ALOGI("JrdLogd exiting");
    exit(0);

}

