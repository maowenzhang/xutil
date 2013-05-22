import urllib.request, os

#=========================================================
# configuration
#
ROOTFOLDER = r'D:\tech\python\codes\html'
ZIPEXE = r'c:\Program Files\7-Zip\7z.exe'

f1 = r'http://s.cn.bing.net/az/hprichv/MoonJellyfish_Corbis_320009_061_ZH-CN.ogv'
f2 = r'http://certools.autodesk.com/cer/NASFileBits.ashx?path=\5-2013\20\13\&report=79953347&ext=zip'

# Settings
class ReportInfo:
    def __init__(self, reportId):
        self.reportId = reportId
        self.workFolder = os.path.join(ROOTFOLDER, reportId)
        self.reportFile = os.path.join(self.workFolder, 'report.html')
        self.dumpFile = os.path.join(self.workFolder, 'dump.zip')
        self.reportUrl = r'http://certools.autodesk.com/cer/ViewReport.aspx?report=' + reportId
        self.dumpFileUrl = ''

    def setDumpFileUrl(self, url):
        self.dumpFileUrl = url

def getWorkFolder(reportId):
    return 

def getReportOutputFile(reportId):
    return 

#=========================================================
# download file util
#
def downloadFile(fileUrl, outputFile):
    if os.path.exists(outputFile):
        print('file already exists:\t', outputFile)
        return True
    
    print('download file:\t', fileUrl)
    print('download to:\t', outputFile);

    #try:
    local_filename, headers = urllib.request.urlretrieve(fileUrl, outputFile)
    #except:

    print('filename:\t', local_filename)
    print('headers:\t', headers)
    
    if not os.path.exists(outputFile):
        print('fail to download file:\t', outputFile)
        return False
    
    return True

def downloadFile2(fileUrl):
    response = urllib.request.urlopen(fileUrl)
    return response.read()

#=========================================================
# download report files
#
# get dump file url by parsing report html file
#
def getDumpfileUrl(repInfo):   
    # get report page
    reportUrl = repInfo.reportUrl
    
    reportHtmlFile = repInfo.reportFile
    if not downloadFile(reportUrl, reportHtmlFile):
        return ''

    # parse report page to get dump file
    contents = open(reportHtmlFile, 'r').read()
    dumpFileUrl = ''

    repInfo.setDumpFileUrl(dumpFileUrl)
    return dumpFileUrl
    
def createFolder(folder):
    print(folder)
    if os.path.exists(folder):
        print('folder is already exists: {0}'.format(folder))
        return
    os.mkdir(folder)
    print('folder is created: ', folder)

def downloadDumpfile(repInfo):
    dumpFile = repInfo.dumpFile
    if os.path.exists(dumpFile):
        print('dump file already exists:\t', dumpFile)
        return True
    
    dumpfileUrl = getDumpfileUrl(repInfo)
    if not dumpfileUrl:
        print('haven not found dump file in report page')
        return False
    
    return downloadFile(dumpfileUrl, dumpFile)

#=========================================================
# run command
#
def runcmd(cmd):
    p = subprocess.Popen(cmd)
    p.wait()

#=========================================================
# unzip files
#
def unzipFiles(zipFile, outFolder, filesToUnzip):
    filesToUnzip = ' '.join(filesToUnzip)
    print('unzip files:\t', filesToUnzip)
    cmd = '{0} e {1} -o{2} {3}'.format(ZIPEXE, zipFile, outFolder, filesToUnzip)
    print('unzip command:\t', cmd)
      
    runcmd(cmd)

#=========================================================
# download build files (dll, pdb, etc.)
#
def downloadBuildFiles(buildId, filesToDownload):
    buildFolder = os.path.join(BUILDPATH, buildId)
    binZip = os.path.join(buildFolder, 'bin.7z')
    pdbZip = os.path.join(buildFolder, 'pdb.7z')

    filesToUnzip = [x+'.dll' for x in filesToDownload]
    unzipFiles(binZip, WORKFOLDER, filesToUnzip)

    filesToUnzip = [x+'.pdb' for x in filesToDownload]
    unzipFiles(pdbZip, WORKFOLDER, filesToUnzip)


def runReport(reportId):
    repInfo = ReportInfo(reportId)

    createFolder(repInfo.workFolder)
    
    if not downloadDumpfile(repInfo):
        return False

    downloadBuildFiles()


#=========================================================
# run
#
import argparse

def runWithArguments():
    parser = argparse.ArgumentParser()
    parser.add_argument('reportId', help='report id')
    parser.add_argument('dllpdbNames', help='dll or pdb names', nargs='+')
    args = parser.parse_args()

    print('report id:\t', args.reportId)
    print('dll or pdb names:\t', str(args.dllpdbNames))

    runReport(args.reportId)

def runtest():
    reportId = '79953347'
    runReport(reportId)

#runWithArguments()
runtest()
input('enter to exit...')
