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

# This file manages the IBM Cloudant credentials. This is designed to run in a Arduino YUN compatible device.
# Because of the size, complexity, and dependencies of python packages we will use custom singular purpose code.
# Runs lean and mean for Arduino YUN OpenWrt OS. I personally use the MediaTek LinKit Smart 7688 Duo.

# HTTPS certificate validation is disabled. Last thing we need is HTTPS certificate issues we cant fix
# on remote embedded devices

'''
At a minimum to configure the system you must make the file /overlay/etc/cloudant.json
{
    "username": "Cloudant User Name",
    "password": "Cloudant Password here",
    "customer_name": "blue bird customer id",
    "storage_url": "https://blah_blah.cloudant.com"
}
'''

import os
import json

class CloudantCredentials:


    def __init__(self, oInLogger, nTimeout=30):
        self._logger = oInLogger
        self._sUsername = None
        self._sPassword = None
        self._sCustomerName = None
        self._sSoftlayerURL = None
        self._timeout = nTimeout
        self._sStorageURL = None      # The object storage URL
        self._sFilePathJSON = None    # Full path/file for the cloud.json file

        self.__initialize()

    def __initialize(self):
        self.__LoadFromJSONFile()
        self.__validateInputs()

    def __LoadFromJSONFile(self, sInFilePath='/overlay/etc/cloudant.json'):
        if os.path.isfile(sInFilePath) and os.stat(sInFilePath).st_size > 0:
            with open(sInFilePath) as data_file:
                data = json.load(data_file)
                self._sUsername = data['username']
                self._sPassword = data['password']
                self._sCustomerName = data['customer_name']
                self._sStorageURL = data['storage_url']
                data_file.close()
        else:
            sError = "ERROR: You must configure the Cloudant config file" + sInFilePath
            raise RuntimeError(sError)


    def __SaveToJSONFile(self, sInFilePath='/overlay/etc/cloudant.json'):

        output = {}
        output['username'] = self._sUsername
        output['password'] = self._sPassword
        output['customer_name'] = self._sCustomerName
        output['storage_url'] = self._sStorageURL

        data_file = open(sInFilePath, 'w')
        data_file.write(json.dumps(output, sort_keys=False, indent=4))
        data_file.close()

    def Username(self):
        return self._sUsername

    def Password(self):
        return self._sPassword

    def CustomerName(self):
        return self._sCustomerName

    def StorageURL(self):
        return self._sStorageURL

    def __validateInputs(self):
        if self._sCustomerName == None:
            raise RuntimeError("You must set the customer_name")

        if self._sPassword == None:
            raise RuntimeError("You must set a password")

        if self._sUsername == None:
            raise RuntimeError("You must set a username")

        if self._sStorageURL == None:
            raise RuntimeError("You must set the storage URL")

