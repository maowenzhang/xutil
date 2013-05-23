import urllib.request, os

#=========================================================
# configuration
#
ROOTFOLDER = r'D:\tech\python\codes\html'
ZIPEXE = r'c:\Program Files\7-Zip\7z.exe'

f1 = r'http://s.cn.bing.net/az/hprichv/MoonJellyfish_Corbis_320009_061_ZH-CN.ogv'
f2 = r'http://certools.autodesk.com/cer/NASFileBits.ashx?path=\5-2013\20\13\&report=79953347&ext=zip'

#=========================================================
# settings
#
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
# parse html contents
#
from html.parser import HTMLParser

class MyHTMLParser(HTMLParser):
    def reset(self):
        HTMLParser.reset(self)
        self.zipFileLink = ''
        self.buildNumber = ''
        self.insideBuildNumberTag = False

    def handle_starttag(self, tag, attrs):
        self.getZipFileLink(tag, attrs)
        self.getBuildNumber(tag, attrs)
    
    def handle_endtag(self, tag):
        if tag == 'span':
            self.insideBuildNumberTag = False

    def handle_data(self, data):
        if self.insideBuildNumberTag:
            self.buildNumber = data
            print('build number: ', data)

    def getZipFileLink(self, tag, attrs):
        if tag != "a":
            return;
        id = [v for k, v in attrs if k=='id']
        if id and id[0]=='ZipFileLink':
            href = [v for k, v in attrs if k=='href']
            if href:
                self.zipFileLink = href

    def getBuildNumber(self, tag, attrs):
        if tag != "span":
            return;
        id = [v for k, v in attrs if k=='id']
        if id and id[0]=='BuildNumberLabel':
            self.insideBuildNumberTag = True


def parseZipFileLink(htmlfile):
    parser = MyHTMLParser()
    parser.feed(htmlfile)
    parser.close()    
    if not parser.zipFileLink:
        print('find no zipFileLink in html file')
        return ''
    print(parser.zipFileLink)

    return parser.zipFileLink
    

def testparsezip():
    f = open('report.html', 'r')
    parseZipFileLink(f.read())
    f.close()

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
    dumpFileUrl = parseZipFileLink(contents)
    if not dumpFileUrl:
        return ''

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
def unzipFiles(zipFile, outFolder, inputFilesToUnzip):
    # check if have
    filesToUnzip = []
    for f in inputFilesToUnzip:
        tmp = os.path.join(outFolder, f)
        if os.path.exists(tmp):
            print('file already exit: ', tmp)
            continue
        filesToUnzip.append(f)

    filesToUnzip = ' '.join(filesToUnzip)
    print('unzip files:\t', filesToUnzip)
    cmd = '{0} e {1} -o{2} {3}'.format(ZIPEXE, zipFile, outFolder, filesToUnzip)
    print('unzip command:\t', cmd)
      
    runcmd(cmd)

#=========================================================
# download build files (dll, pdb, etc.)
#
def downloadBuildFiles(buildId, dllpdbNames):
    print('\n---- download dll or pdb names')

    if not dllpdbNames:
        print("no dll or pdb to download")
        return

    buildFolder = os.path.join(BUILDPATH, buildId)
    binZip = os.path.join(buildFolder, 'bin.7z')
    pdbZip = os.path.join(buildFolder, 'pdb.7z')

    filesToUnzip = [x+'.dll' for x in dllpdbNames]
    unzipFiles(binZip, WORKFOLDER, filesToUnzip)

    filesToUnzip = [x+'.pdb' for x in dllpdbNames]
    unzipFiles(pdbZip, WORKFOLDER, filesToUnzip)

    # download again
    names = input("\n---- please input dll or pdb names to download: ")
    dllpdbNames = names.split()
    downloadBuildFiles(buildId, dllpdbNames)


def runReport(reportId, dllpdbNames):
    repInfo = ReportInfo(reportId)

    createFolder(repInfo.workFolder)
    
    if not downloadDumpfile(repInfo):
        return False

    downloadBuildFiles(repInfo.buildId, dllpdbNames)


#=========================================================
# run
#
import argparse

def run():
    print("\n\n===================== autoGetCERDumpFileAndDllPdb\n")

    parser = argparse.ArgumentParser()
    parser.add_argument('reportId', help='report id')
    parser.add_argument('dllpdbNames', help='dll or pdb names', nargs='*')
    args = parser.parse_args()

    reportId = args.reportId
    dllpdbNames = args.dllpdbNames

    if not reportId:        
        tmp = input("please input CER report id and dll or pdb names (79953347 nafusion nasketch):\n")
        tmpitems = temp.split()
        if len(tmpitems) > 0:
            reportId = tmpitems[0]
            dllpdbNames.extend(tmpitems[2:])            
    
    print('report id:\t', reportId)
    print('dll or pdb names:\t', dllpdbNames)

    runReport(reportId, dllpdbNames)

def runtest():
    #reportId = '79953347'
    #runReport(reportId)

    testparsezip()

#run()
runtest()
input('enter to exit...')
