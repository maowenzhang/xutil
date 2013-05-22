import os
import sys
import myutil

# --------------------------------------------------
# dump files in folder

# ---------------------------------------------------
# Global settings
#
g_files = {}

# ---------------------------------------------------
# Class for dump file info (file feature mapping)
class DumpFileInfo:
        def __init__(self, file):
                self.file = file
                self.filename = ""
                self.filedir = ""
        def print(self):
                print(self.filedir, self.filename)
                
        def printToFile(self, f):
                print(self.file, self.filedir, self.filename, file=f)

# ---------------------------------------------------
# Dump file list from directory
#
class FileDumper:
        def __init__(self, inputfolder, outputfile, dumplevel):
                self.inputfolder = inputfolder
                self.outputfile = outputfile
                self.dumplevel = dumplevel
                self.curlevel = 0        
                
        def dumpFilesRecur(self, folder):
                self.curlevel += 1
                list = os.listdir(folder)
                for item in list:
                        itemFullPath = os.path.join(folder, item)
                        finfo = DumpFileInfo(itemFullPath)
                        finfo.filename = item
                        finfo.filedir = folder.lstrip(self.inputfolder)
                        g_files[itemFullPath] = finfo
                        #g_files[itemFullPath].print()
                        # iterator sub dir
                        if (self.curlevel < self.dumplevel):
                                 if os.path.isdir(itemFullPath):
                                         self.dumpFilesRecur(itemFullPath)
                self.curlevel -= 1

        def dumpFiles(self):
                g_files.clear()
                myutil.log1("Dump files in: " + self.inputfolder)
                self.curlevel = 0
                self.dumpFilesRecur(self.inputfolder)
                
                self.saveToFile(self.outputfile)
                return g_files

        def saveToFile(self, outputfile) :
                myutil.log("save dump files to: " + outputfile)
                
                import io
                f = open(outputfile, 'w')
                for item in g_files.values():
                        item.printToFile(f)
                f.flush()
                f.close()
#===============================================
# RUN

def run(inputfolder, outputfile, dumplevel):
        fd = FileDumper(inputfolder, outputfile, dumplevel)
        res = fd.dumpFiles()        
        return res;

def printresult(res):
        myutil.log("print files")
        for item in res.values():
                item.print()
        
#===============================================
# test
def test():
        inputfolder = r'D:\tech\python\Python33\CodeCoverageHelper\src'
        outputfile = 'outputDumpFileList.txt'
        res = run(inputfolder, outputfile, 7)
        printresult(res)

if __name__ == '__main__':
    test()
    input('enter to exit')
