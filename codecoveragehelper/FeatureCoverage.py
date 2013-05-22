import os
import sys
import myutil
import featureparser
import classparser
import coverageparser
import filedumper

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
g_inputDumpFolder = r"D:\tech\Python\Python33\CodeCoverageHelper\src"
g_inputFeatureList = "inputFeatureList.txt"
g_inputClassCoverageList = "inputClassCoverageList.txt"
g_outputDumpFileList = "outputDumpFileList.txt"

# Output list of FinalCoverageInfo
g_finalcoverages = []

# ---------------------------------------------------
# Data storage
#
# Class for dump file info (file feature mapping)
class FinalCoverageInfo:
        def __init__(self, fileinfo):
                self.file = fileinfo.file
                self.file2 = os.path.join(fileinfo.filedir, fileinfo.filename)
                self.features = []              
                self.classes = []
                self.coverage = 0
                
        def addfeature(fea):
                self.features.append(fea)
                
        def print(self):
                print(self.file2, str(self.features), str(self.classes), self.coverage)
                
        def printToFile(self, f):
                print(self.file2, str(self.features), str(self.classes), self.coverage, file=f)

# ---------------------------------------------------
# Mapping Feature info
#
def matchFeatures(files, features):
        for fileInfo in files.values():
                finalInfo = FinalCoverageInfo(fileInfo)
                g_finalcoverages.append(finalInfo)
                
                for feaInfo in features.values():
                        matchFeaturesToFile(fileInfo, feaInfo, finalInfo)
                        
def matchFeaturesToFile(fileInfo, feaInfo, finalInfo):
        for feakey in feaInfo.keywords():
                feakey_low = feakey.lower()
                filename_low = fileInfo.filename.lower()
                #print(filename_low, feakey_low)
                if feakey_low in filename_low:
                        #myutil.log("feature: " + feaInfo.name + "key: " + feakey + "  match file: " + fileInfo.file)
                        finalInfo.features.append(feaInfo.name)
                        #finalInfo.print()
                        break

# ---------------------------------------------------
# Mapping Classes
#
def matchClasses():
        for finalInfo in g_finalcoverages:
                if (os.path.isdir(finalInfo.file)):
                        continue;                    
                res = classparser.run(finalInfo.file)
                classparser.printresult(res)
                for ir in res:
                        finalInfo.classes.append(ir)                        

def matchCoverages(resCoverages):
        for finalInfo in g_finalcoverages:
                covtotal = 0
                for c in finalInfo.classes:
                        # get coverage
                        coverInfo = resCoverages.get(c);
                        if coverInfo:
                                cov = coverInfo.coverage
                                cov = cov.strip().rstrip('%')
                                print('\n\t' + cov)
                                covtotal += int(cov)
                finalInfo.coverage = covtotal

def printresult():
        for i in g_finalcoverages:
                i.print()
# ---------------------------------------------------
# Run program
# 
if __name__ == "__main__":
        # Get input      
        if sys.argv.count == 2:
                folder = sys.argv[1]

        myutil.log0("1. Dump files")
        inputfolder = g_inputDumpFolder
        outputfile = g_outputDumpFileList
        dumplevel = 6
        resfiles = filedumper.run(inputfolder, outputfile, dumplevel)
        filedumper.printresult(resfiles)        

        myutil.log0("2. Parse features")
        resfeatures = featureparser.run(g_inputFeatureList)
        featureparser.printresult(resfeatures)       
        

        myutil.log0("3. Match features")
        matchFeatures(resfiles, resfeatures)
        #printresult()
        
        myutil.log0("3. Match classes")
        matchClasses()
        #printresult()

        myutil.log0("4. Get coverage")
        resCoverages = coverageparser.run(g_inputClassCoverageList)
        coverageparser.printresult(resCoverages)

        myutil.log0("5. Match coverages")
        matchCoverages(resCoverages)
        printresult()

        
        
