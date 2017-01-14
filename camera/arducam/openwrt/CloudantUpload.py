#  Copyright (c) 2016, Erik Anderson  https://www.linkedin.com/in/erikanderson
# All rights reserved.
# Standard 3 clause BSD license
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
# following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
#    disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
# USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This uploads sensor to the IBM Cloudant NoSQL database. This is designed to run in a Arduino YUN compatible device.
# Because of the size, complexity, and dependencies of python packages we will use custom singular purpose code.
# Runs lean and mean for Arduino YUN OpenWrt OS. I personally use the MediaTek LinKit Smart 7688 Duo.

# HTTPS certificate validation is disabled. Last thing we need is HTTPS certificate issues we cant fix
# on remote embedded devices

import os
import datetime
import urllib2
import requests
import ssl
import json
from requests.auth import HTTPBasicAuth


class CloudantUpload:

    def __init__(self, oInLogger, oInCloudantCredentials, nTimeout=30):
        self._logger = oInLogger
        self._CloudantCredentials = oInCloudantCredentials
        self._timeout = nTimeout

    def uploadSensorData(self, sInSensorFile):

        sFullURL = self._CloudantCredentials.StorageURL() + "/" + self._CloudantCredentials.CustomerName()
        sUser = self._CloudantCredentials.Username()
        sPass = self._CloudantCredentials.Password()

        self._logger.debug("uploadSensorData: Uploading sensor file=" + sInSensorFile + ", to=" + sFullURL)

        sensorFile = open(sInSensorFile, 'r')
        data = sensorFile.read()
        res = requests.post(url=sFullURL,
                            data=data,
                            auth=HTTPBasicAuth(sUser, sPass),
                            verify=False,  # Disable HTTPS certificate validation.
                            headers={'Content-Type': 'application/json'})

        self._logger.debug("uploadSensorData: Request headers=" + str(res.request.headers))

        sResponse = res.text
        if res.status_code != 201:
            # Error in HTTP put. Throw error
            sError = "ERROR uploading image. url=" + sFullURL + "\n\tres.status_code=" + str(res.status_code) + \
                "\n\tHTTP headers=" + str(res.request.headers) + "\n\tResponse=" + sResponse
            raise RuntimeError(sError)

        sensorFile.close()

        sResults = "SUCCESS: Uploaded " + sInSensorFile

        self._logger.debug("uploadSensorData response=" + sResponse)

        return sResults



