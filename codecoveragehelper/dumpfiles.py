import os
import sys

# --------------------------------------------------
# Summary
# output
# file list
#       file feature dev qa coverage (classes)
# input
# 1.  feature list
#       feature dev qa keys
# 2. class coverage list
#        class  coverage
#
#Steps:
# 1. dump files
# 2. match features
# 3. match class to file
# 4. calculate file coverage (classes)

# ---------------------------------------------------
# Global settings
#
g_inputDumpFolder = "D:\\tech\\python\\Python33"
g_inputFeatureList = "inputFeatureList.txt"
g_inputClassCoverageList = "inputClassCoverageList.txt"
g_outputDumpFileList = "outputDumpFileList.txt"

g_files = {}
level_max= 1
level_cur = 1
g_features = {}

# ---------------------------------------------------
# Data storage
#
# Class for feature list
class FeatureInfo:
        def __init__(self, infostr):
                contents = infostr.split("\t", 4)
                self.name = ""
                self.dev = ""
                self.qa = ""
                self.keys = []
                if (len(contents) >= 1):
                        self.name = contents[0]
                if (len(contents) >= 2):
                        self.dev = contents[1]
                if (len(contents) >= 3):
                        self.qa = contents[2]
                if (len(contents) >= 4):
                        tmp = contents[3]
                        self.keys = tmp.split(",")
                        
        def name(self):
                return self.name
        def keywords(self):
                return self.keys
        def print(self):
                print("feature name: " + self.name + "\tdev: " + self.dev + "\tQA: " + self.qa + "\tkeys: " + str(self.keys))

# Class for dump file info (file feature mapping)
class DumpFileInfo:
        def __init__(self, file):
                self.file = file
                self.feature = []
                self.dev = ""
                self.qa = ""
                self.coverage = 0
                self.classes = []
                self.filename = ""
                self.filedir = ""
        def print(self):
                print(self.file, self.filedir, self.filename, self.feature, self.coverage, str(self.classes))
                
        def printToFile(self, f):
                print(self.file, self.feature, self.coverage, str(self.classes), file=f)

# ---------------------------------------------------
# Helper methods
#
def printmsg(msg) :
        print(" ")
        print("========== " + msg)
        
def printmsg1(msg) :
        print(" ")
        print("========== #. " + msg)        
        print(" ")
        
def printmsg2(msg) :
        print("---------- " + msg)
# ---------------------------------------------------
# Dump file list from directory
#
def dumpFilesRecur(folder, level_cur):
        level_cur += 1
        list = os.listdir(folder)
        for item in list:
                itemFullPath = os.path.join(folder, item)
                finfo = DumpFileInfo(itemFullPath)
                finfo.filename = item
                finfo.filedir = folder.lstrip(g_inputDumpFolder)
                g_files[itemFullPath] = finfo
                #g_files[itemFullPath].print()
                # iterator sub dir
                if (level_cur < level_max):
                         if os.path.isdir(itemFullPath):
                                 dumpFilesRecur(itemFullPath, level_cur)
        level_cur -= 1

def printFiles() :
        printmsg2("print current files")
        for item in g_files.values():
                item.print()
                
def dumpFiles(folder):
        printmsg1("Dump files in: " + folder)
        level_cur = 0
        dumpFilesRecur(folder, level_cur)

        printFiles()
        saveToFile(g_outputDumpFileList)

def saveToFile(outputfile) :
        printmsg("save dump files to: " + outputfile)
        
        import io
        f = open(outputfile, 'w')
        for item in g_files.values():
                item.printToFile(f)
        f.flush()
        f.close()

# ---------------------------------------------------
# Mapping Feature info
#
def readFeatures(featurefile) :
        printmsg("read feature list: " + featurefile)
        f = open(featurefile, 'r')
        ln = str(f.readline()).strip().rstrip("\n")
        while (len(ln) > 0):
                #print(ln)
                #print("len of line" + str(len(ln)))
                fe = FeatureInfo(ln)
                g_features[fe.name] = fe
                #fe.print()
                #g_features[fe.name].print()
                ln = str(f.readline()).strip().rstrip("\n")

def mapFeatureToFile():
        for fileInfo in g_files.values():
                #get match                
                for feaInfo in g_features.values():
                        mapFeatureToFileItem(fileInfo, feaInfo)                           
                
def mapFeatureToFileItem(fileInfo, feaInfo):
        for feakey in feaInfo.keywords():
                feakey_low = feakey.lower()
                filename_low = fileInfo.filename.lower()
                if feakey_low in filename_low:
                        printmsg2("feature: " + feaInfo.name + "key: " + feakey + "  match file: " + fileInfo.file)
                        fileInfo.feature.append(feaInfo.name)        

def matchFeatures():
        printmsg1("Macth features to file")
        readFeatures(g_inputFeatureList)
        printFeatures()
        mapFeatureToFile()
        printFiles()        

def printFeatures() :
        printmsg2("print current files")
        for item in g_features.values():
                item.print()

# ---------------------------------------------------
# Mapping Class coverage
#
def readClasses():
        for fileInfo in g_files.values():
                
def matchClassCoverage():
        printmsg1("Match class to file")
        readClasses()
        printClasses()        


# ---------------------------------------------------
# Run program
# 
if __name__ == "__main__":
        # Get input      
        if sys.argv.count == 2:
                folder = sys.argv[1]

        # 1. Dump files        
        dumpFiles(g_inputDumpFolder)

        # 2. Match features
        matchFeatures()
