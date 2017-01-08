import os
import sys
import logging
import urllib2
import traceback

from SoftlayerCredentials import SoftlayerCredentials
from SoftlayerUpload import SoftlayerUpload
from CloudantCredentials import CloudantCredentials
from CloudantUpload import CloudantUpload


sLogFilePath = os.getcwd()  # Directory where you want this script to store the Cointerra log files in event of troubles
sCloudLogFile = sLogFilePath + '/cloud_monitor.log'

# Possible logging levels are
#  logging.DEBUG     This is a lot of logs.  You will likely need this level of logging for support issues
#  logging.INFO      Logs confirmations that everything is working as expected.  This should be the default level
#  logging.WARNING   Logs warning.  Issues that did not cause a reboot are logged here.  Like temperature and hash rates.
#  logging.ERROR     Loss errors.  Script exceptions and issues we discovered with the Cointerra hardware
#  logging.CRITICAL  This script doesnt use this level
nLoggingLevel = logging.DEBUG

#
# For checking the internet connection
#

def internet_on():
    try:
        response = urllib2.urlopen('http://www.google.com/', timeout=10)
        if ( response.code == 200 ):
            return True
    except urllib2.URLError as err:
        pass

    return False

if __name__ == "__main__":

    iArgLen = len(sys.argv)
    sHost = sys.argv[1] if iArgLen > 1 else None
    command = sys.argv[2] if iArgLen > 2 else None
    parameter = sys.argv[3] if iArgLen > 3 else None

    # Delete the old log file
    if os.path.isfile(sCloudLogFile) == True:
        os.remove(sCloudLogFile)


    logger = logging.getLogger('BluemixCloud')
    hdlr = logging.FileHandler(sCloudLogFile)
    formatter = logging.Formatter('%(asctime)s %(levelname)s %(message)s')
    hdlr.setFormatter(formatter)
    logger.addHandler(hdlr)
    logger.setLevel(nLoggingLevel)

    try:
        print "hello"

        if ( internet_on() == False ):
            print "ERROR: No internet. Cannot upload to cloud."
            sys.exit(0)

        logger.info('Starting up')

        oSoftlayerCredentials = SoftlayerCredentials(logger)
        oSoftlayerUpload = SoftlayerUpload(logger,oSoftlayerCredentials)
        #oSoftlayerUpload.ListContainers()
        #oSoftlayerUpload.ListContainerContent()

        sResult = oSoftlayerUpload.uploadImage("/overlay/DCIM/1.jpg")

        oCloudantCredentials = CloudantCredentials(logger)
        oCloudantUpload = CloudantUpload(logger, oCloudantCredentials)
        sResult = sResult + "\n" + oCloudantUpload.uploadSensorData("/overlay/DCIM/1.json")

        logger.debug(sResult)
        print sResult

    except KeyboardInterrupt:
        sys.exit(0)

    except Exception as e:
        # Its important to crash/shutdown here until all bugs are gone.
        sTrace = 'ERROR:' + traceback.format_exc()
        logger.error(sTrace)
        print sTrace
        sys.exit(0)
