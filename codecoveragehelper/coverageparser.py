import myutil
import logging
#logging.basicConfig(level=logging.INFO)

#=========================================================
# coverageparser:   used to parse code coverage data
#

# output { classname, coverageinfo(classname, coverage) }
g_coverages = {}

class CoverageInfo:
    def __init__(self, cname, cov):
        self.classname = cname
        self.coverage = cov
    def print(self):
        print(self.classname, self.coverage)

def run(coveragefile):
    g_coverages.clear()
    myutil.log1("parse coverage file: " + coveragefile)
    f = open(coveragefile, 'r')
    data = f.readlines()
    for ln in data:
        ln = ln.strip()
        if len(ln) == 0:
            continue;
        contents = ln.split()
        if (len(contents) < 2):
            logging.warning("less than 2 column coverage info!")
            continue;
        
        cname = contents[0]
        cov = contents[1]
        cinfo = CoverageInfo(cname, cov)        
        g_coverages[cname] = cinfo
        g_coverages[cname].print()
    return g_coverages

def printresult(res):
    for i in res.values():
        i.print()

#========================================================
def test():
    res = run('inputClassCoverageList.txt')
    for i in res.values():
        i.print()

#test()


    
