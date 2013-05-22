import os
import sys
import myutil

#===================================================
# featureparser:        used to parse feature info
#
# output dictionary <featurename, featureinfo{name, dev, qa, keys} >
g_features = {}

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

def run(featurefile) :
        g_features.clear()
        myutil.log1("parse feature list: " + featurefile)
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
        return g_features

def printresult(features) :
        myutil.log2("print features")
        for item in g_features.values():
                item.print()

def test():
        res = run("inputFeatureList.txt")
        printresult(g_features)

#test()


        
