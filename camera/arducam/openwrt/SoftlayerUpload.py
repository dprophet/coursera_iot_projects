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

# This uploads ArduCAM photos to the IBM Softlayer cloud storage. This is designed to run in a Arduino YUN compatible device.
# Because of the size and complexity of python-swiftclient and python-keystoneclient modules they will not be used.
# Runs lean and mean for Arduino YUN OpenWrt OS. I personally use the MediaTek LinKit Smart 7688 Duo.

# HTTPS certificate validation is disabled. Last thing we need is HTTPS certificate issues we cant fix
# on remote embedded devices

import os
import datetime
import urllib2
import requests
import ssl
import piexif
import json
import traceback

# Embed the sensor data into the JPEG images exif metadata section
class EmbedSensorDataInJPEG:
    def __init__(self, sInImage):
        self._filename = sInImage
        self.__addMetaData()

    def DumpExifInfo(self, sInFile):
        ret = {}
        exif_dict = piexif.load(sInFile)
        print "Exif=" + str(exif_dict)
        return exif_dict

    def __addMetaData(self):
        sfile1, sextension = os.path.splitext(self._filename)
        sMetaFile = sfile1 + ".json"
        sNewFile = sfile1 + "_meta.jpg"

        if os.path.isfile(sMetaFile) and os.stat(sMetaFile).st_size > 0:
            exif_dict = piexif.load(self._filename)

            with open(sMetaFile) as data_file:
                # Read the sensor data file and embed into the JPG
                try:
                    data = data_file.read()
                    oDict = json.loads(data)
                except Exception as e:
                    # Its important to crash/shutdown here until all bugs are gone.
                    sTrace = 'ERROR:' + traceback.format_exc()
                    sError = "Error adding metadata file:" + sMetaFile +  "\n\tContents" + data + "\n\ttraceback=" + sTrace + \
                             "\n\t" + e.strerror
                    raise RuntimeError(sError)

                oDate = datetime.datetime.utcnow()
                sTime = oDate.strftime("%Y:%m:%d %H:%M:%S")
                oDict['time_taken'] = oDate.isoformat()
                sNewData = json.dumps(oDict)

                exif_dict['Exif'][piexif.ExifIFD.UserComment] = sNewData
                exif_dict['Exif'][piexif.ExifIFD.DateTimeOriginal] = sTime
                exif_dict['Exif'][piexif.ExifIFD.DateTimeDigitized] = sTime
                exif_dict['Exif'][piexif.ExifIFD.CameraOwnerName] = "Smart Bird House"
                exif_dict['Exif'][piexif.ExifIFD.LensMake] = "ArduCAM"
                exif_dict['Exif'][piexif.ExifIFD.MakerNote] = sNewData
                exif_bytes = piexif.dump(exif_dict)

                piexif.insert(exif_bytes, self._filename)

                self.DumpExifInfo(self._filename)

                data_file.close()

            with open(sMetaFile, "w") as data_file:
                data_file.write(sNewData)
                data_file.close()


class SoftlayerUpload:

    def __init__(self, oInLogger, oInSoftCredentials, nTimeout=30):
        self._logger = oInLogger
        self._SoftCredentials = oInSoftCredentials
        self._timeout = nTimeout

    def uploadImage(self, sInArducamImage):

        EmbedSensorDataInJPEG(sInArducamImage)

        sToken = self._SoftCredentials.SubjectToken()
        filename1 = datetime.datetime.utcnow().strftime("%Y%m%d_%H%M%S")
        filename1 = filename1 + ".jpg"

        sFullURL = self._SoftCredentials.StorageURL() + "/" + self._SoftCredentials.CustomerName() + "/" + filename1

        self._logger.debug("uploadImage: Uploading image=" + sInArducamImage + ", to=" + sFullURL)

        imageFile = open(sInArducamImage, 'rb')
        data = imageFile.read()
        res = requests.put(url=sFullURL,
                            data=data,
                            verify=False,  # Disable HTTPS certificate validation.
                            headers={'Content-Type': 'image/jpeg', 'X-Auth-Token': sToken })

        print "Request headers=" + str(res.request.headers)

        sResponse = res.text
        if res.status_code != 201:
            # Error in HTTP put. Throw error
            sError = "ERROR uploading image. url=" + sFullURL + "\n\tres.status_code=" + str(res.status_code) + \
                "\n\tHTTP headers=" + str(res.request.headers) + "\n\tResponse=" + sResponse
            raise RuntimeError(sError)


        imageFile.close()

        sResults = "SUCCESS: Uploaded " + filename1

        self._logger.debug("uploadImage response=" + sResponse)

        return sResults

    def ListContainers(self):
        sFullURL = self._SoftCredentials.StorageURL()
        oRequest = urllib2.Request(sFullURL)
        sTester = self._SoftCredentials.SubjectToken()
        oRequest.add_header('X-Auth-Token', self._SoftCredentials.SubjectToken())
        # Disable HTTPS certificate validation. Last thing we need is HTTPS certificate issues on remote embedded devices
        context = ssl._create_unverified_context()

        response = urllib2.urlopen(oRequest, timeout=self._timeout, context=context)

        sResponse = response.read()

        if response.code != 200:
            # Error in HTTP put. Throw error
            sError = "ListContainers error.\n\turl=" + sFullURL + "\n\tres.status_code=" + str(response.code) + \
                "\n\tHTTP headers=" + str(oRequest.headers) + "\n\tResponse=" + sResponse
            raise RuntimeError(sError)


        self._logger.debug("ListContainers response=" + sResponse)
        return sResponse

    def ListContainerContent(self, sInContainerName=None):
        sFullURL = self._SoftCredentials.StorageURL() + "/"
        if sInContainerName == None:
            sFullURL = sFullURL + self._SoftCredentials.CustomerName()
        else:
            sFullURL = sFullURL + sInContainerName

        oRequest = urllib2.Request(sFullURL)
        oRequest.add_header('X-Auth-Token', self._SoftCredentials.SubjectToken())
        # Disable HTTPS certificate validation. Last thing we need is HTTPS certificate issues on remote embedded devices
        context = ssl._create_unverified_context()

        response = urllib2.urlopen(oRequest, timeout=self._timeout, context=context)

        sResponse = response.read()

        if response.code != 200:
            # Error in HTTP put. Throw error
            sError = "ListContainerContent error.\n\turl=" + sFullURL + "\n\tres.status_code=" + str(response.code) + \
                "\n\tHTTP headers=" + str(oRequest.headers) + "\n\tResponse=" + sResponse
            raise RuntimeError(sError)

        self._logger.debug("ListContainerContent response=" + sResponse)
        return sResponse


