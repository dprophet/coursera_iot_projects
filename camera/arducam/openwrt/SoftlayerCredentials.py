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

# This file manages the IBM Softlayer credentials. This is designed to run in a Arduino YUN compatible device.
# Because of the size and complexity of python-swiftclient and python-keystoneclient modules they will not be used.
# Runs lean and mean for Arduino YUN OpenWrt OS. I personally use the MediaTek LinKit Smart 7688 Duo.

# HTTPS certificate validation is disabled. Last thing we need is HTTPS certificate issues we cant fix
# on remote embedded devices

'''
At a minimum to configure the system you must make the file /overlay/etc/softlayer.json
{
    "username": "Softlayer User Name",
    "password": "Softlayer Password here",
    "customer_name": "blue bird customer id",
    "project_id": "Softlayer Project ID"
}
'''

import os
import datetime
import json
import urllib2
import ssl
from dateutil import parser

class SoftlayerCredentials:


    def __init__(self, oInLogger, nTimeout=30):
        self._logger = oInLogger
        self._sUsername = None
        self._sPassword = None
        self._sProjectId = None
        self._sCustomerName = None
        self._sSoftlayerURL = 'https://identity.open.softlayer.com/v3/auth/tokens'
        s = '{"auth":{"identity":{"methods":["password"],"password":{"user":{"id":null,"password":null}}},"scope":{"project":{"id":null}}}}'
        self._sJsonRequestStructure = json.loads(s)
        self._timeout = nTimeout
        self._sSubjectToken = None    # 'X-Subject-Token' from the HTTP header
        self._sExpiresAt = None       # The date/time when the _sSubjectToken expires (usually 24 hours after fetching)
        self._dateExpiresAt = None    # Date object converted from _sExpiresAt
        self._sAdminUser = None       # The admin user name required to upload files to cloud storage Softlayer.com
        self._sStorageURL = None      # The object storage URL
        self._sFilePathJSON = None    # Full path/file for the cloud.json file

        self.__initialize()

    def __initialize(self):
        self.__LoadFromJSONFile()

        if self.__checkExpired():
            self.__FetchCredentials()

    def __LoadFromJSONFile(self, sInFilePath='/overlay/etc/softlayer.json'):
        if os.path.isfile(sInFilePath) and os.stat(sInFilePath).st_size > 0:
            with open(sInFilePath) as data_file:
                data = json.load(data_file)
                self._sUsername = data['username']
                self._sPassword = data['password']
                self._sProjectId = data['project_id']
                self._sCustomerName = data['customer_name']

                if 'subject_token' in data:
                    self._sSubjectToken = data['subject_token']
                if 'expires_at' in data:
                    self._sExpiresAt = data['expires_at']
                if 'admin_user' in data:
                    self._sAdminUser = data['admin_user']
                if 'storage_url' in data:
                    self._sStorageURL = data['storage_url']
                data_file.close()
        else:
            sError = "ERROR: You must configure the Softlayer config file" + sInFilePath
            raise RuntimeError(sError)


    def __SaveToJSONFile(self, sInFilePath='/overlay/etc/softlayer.json'):

        output = {}
        output['username'] = self._sUsername
        output['password'] = self._sPassword
        output['project_id'] = self._sProjectId

        if self._sCustomerName:
            output['customer_name'] = self._sCustomerName

        if self._sSubjectToken:
            output['subject_token'] = self._sSubjectToken

        if self._sExpiresAt:
            output['expires_at'] = self._sExpiresAt

        if self._sAdminUser:
            output['admin_user'] = self._sAdminUser

        if self._sStorageURL:
            output['storage_url'] = self._sStorageURL

        data_file = open(sInFilePath, 'w')
        data_file.write(json.dumps(output, sort_keys=False, indent=4))
        data_file.close()

    def Username(self):
        return self._sUsername

    def Password(self):
        return self._sPassword

    def ProjectId(self):
        return self._sProjectId

    def CustomerName(self):
        return self._sCustomerName

    def SubjectToken(self):
        return self._sSubjectToken

    def ExpiresAt(self):
        return self._sExpiresAt

    def StorageURL(self):
        return self._sStorageURL

    def __validateInputs(self):
        if self._sProjectId == None:
            raise RuntimeError("You must set the project id")

        if self._sPassword == None:
            raise RuntimeError("You must set a password")

        if self._sUsername == None:
            raise RuntimeError("You must set a username")

    def __FetchCredentials(self):
        self.__validateInputs()

        self._sJsonRequestStructure['auth']['identity']['password']['user']['id'] = self._sUsername
        self._sJsonRequestStructure['auth']['identity']['password']['user']['password'] = self._sPassword
        self._sJsonRequestStructure['auth']['scope']['project']['id'] = self._sProjectId

        oRequest = urllib2.Request(self._sSoftlayerURL)
        oRequest.add_header('Content-Type', 'application/json')
        # sJsonData = json.dumps(self._sJsonRequestStructure)
        sJsonData = json.dumps(self._sJsonRequestStructure, sort_keys=True, indent=4)
        self._logger.debug("FetchCredentials: sJsonData=\n" + sJsonData)
        context = ssl._create_unverified_context()
        response = urllib2.urlopen(oRequest, sJsonData, self._timeout, context=context)

        sResponse = response.read()
        decoded = json.loads(sResponse.replace('\x00', ''))
        if len(decoded) > 0:
            self._sSubjectToken = response.info().getheader('X-Subject-Token')
            self._sExpiresAt = decoded['token']['expires_at']
            self._dateExpiresAt = parser.parse(self._sExpiresAt)
            self._dateExpiresAt = self._dateExpiresAt.replace(tzinfo=None)
            iCatalogLen = len(decoded['token']['catalog'])
            if self._sUsername != decoded['token']['user']['id']:
                raise RuntimeError("Something is wrong. Username='" + self._sUsername + "' != token:user:id='" \
                                   + decoded['token']['user']['id'] + "'")
            self._sAdminUser = decoded['token']['user']['name']

            self._logger.debug('Got response from Softlayer.com site.  sResponse=\n' +
                              json.dumps(decoded, sort_keys=True, indent=4)
                              + ', len=' + str(len(sResponse)))
            self._logger.debug('response.info()=' + response.info())

            for iCatalogItem in range(iCatalogLen):
                rescat = decoded['token']['catalog'][iCatalogItem]

                if rescat['type'] == "object-store" and rescat['name'] == "swift":
                    iLenEndpoints = len(rescat['endpoints'])

                    for iEndItem in range(iLenEndpoints):
                        resend = rescat['endpoints'][iEndItem]

                        if resend['interface'] == "public" and resend['region_id'] == "dallas":
                            self._sStorageURL = resend['url']

            self._logger.debug("sSubjectToken=" + self._sSubjectToken)
            self._logger.debug("sExpiresAt=" + self._sExpiresAt)
            self._logger.debug("sAdminUser=" + self._sAdminUser)
            self._logger.debug("sStorageURL=" + self._sStorageURL)
            self._logger.debug("dateExpiresAt=" + self._dateExpiresAt.isoformat())

            self.__validateResults()

            self.__SaveToJSONFile()


    def __validateResults(self):
        if self._sSubjectToken == None:
            raise RuntimeError("Unable to find 'X-Subject-Token' in the HTTP header")

        if self._sExpiresAt == None:
            raise RuntimeError("Unable to find token:expires_at in the results")

        if self._sAdminUser == None:
            raise RuntimeError("Unable to find token:user:name in the results")

        if self._sStorageURL == None:
            raise RuntimeError("Unable to find region_id:dallas in token:catalog: swift object-store")

    def __checkExpired(self):
        if self._sExpiresAt == None:
            return True

        if self._dateExpiresAt == None:
            self._dateExpiresAt = parser.parse(self._sExpiresAt)
            self._dateExpiresAt = self._dateExpiresAt.replace(tzinfo=None)

        oNow = datetime.datetime.utcnow()

        DateTemp = self._dateExpiresAt + datetime.timedelta(hours=-1)

        self._logger.debug('DateTemp=' + DateTemp.isoformat())
        self._logger.debug('oNow=' + oNow.isoformat())
        self._logger.debug('dateExpiresAt=' + self._dateExpiresAt.isoformat())

        if oNow > DateTemp:
            return True
        else:
            return False
