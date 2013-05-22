import re
import myutil
import logging

#logging.basicConfig(filename='log.log', level=logging.DEBUG)
logging.basicConfig(level=logging.INFO)

#===================================================
# classparser:  used to parse class from files
#

#===================================================
# class name's regular expression
#
# format:  class [\s\t]+ [EXPORT_API] classname [:] [public baseclass] [{]
# "class classname" or "class exportAPI classname : public ...",
# make sure no ";" to skip class declaration
spaceExp = '[\s\t]+'
classnameExp = '[a-zA-Z_][a-zA-Z_0-9]+'
g_classReExp = 'class' + spaceExp + classnameExp + '[^;]*$'


# output [classname, classname, ...]
g_classes = []

#===================================================
# Class parser
#
class ClassParser:
        def __init__(self):
                self.classReExp = g_classReExp
                self.rep = re.compile(self.classReExp)
                
        def getClasses(self, file):
                g_classes.clear()
                logging.debug("Get classes from file: " + file)
                f = open(file)
                lines = f.readlines()
                for ln in lines:
                        res = self.getClassesFromString(ln)
                        if res[0]:
                                logging.debug("\tclass:\t" + res[1])
                                g_classes.append(res[1])
                return g_classes
                
        def getClassesFromString(self, matchstr):
                #
                matchstr = matchstr.strip().strip('\t')
                logging.debug("To match: " + matchstr)

                res = self.rep.match(matchstr)
                if not res:
                        logging.debug("Not match string: " + matchstr)
                        return (False, res)
                
                logging.debug("match result: " + res.group())
                
                # get class name
                return self.getclassname(res.group())

        def getclassname(self, matchres):
                logging.debug("get class name from: " + matchres)
                classstr = matchres
                if ':' in matchres:
                        #logging.debug("has :")
                        contents = matchres.partition(':')
                        #logging.debug(contents)
                        classstr = contents[0].rstrip()
                        
                elif '{' in matchres:
                        #logging.debug("has {")
                        contents = matchres.partition('{')
                        #logging.debug(contents)
                        classstr = contents[0].rstrip()

                logging.debug("--- class sub string: " + classstr)
                
                # check only two words
                contents = classstr.split() 
                logging.debug("--- class bus splited: " + str(contents))
                if len(contents) > 3:
                        logging.warning('!!!!!!!!!!!!!!!!!!!! error class: ' + matchres)
                        return (False, "")
                className = contents[len(contents)-1]
                logging.debug(className)
                return (True, className)

#===================================================
# Main API
#
def run(inputfile):
        cp = ClassParser()
        res = cp.getClasses(inputfile)
        return res         

def printresult(res):
        logging.debug("results")
        for i in res:
                myutil.log(i)

#===================================================
# Testing
#
class ClassParserTest:
        def __init__(self):
                self.target = ClassParser();                
              
        def test(self, matchstr):
                res = self.target.getclassname(matchstr)
                

        def testfolder(self):
                myutil.log2('test folder')
                import os
                folder = 'D:\\tech\\python\\Python33\\CodeCoverageHelper\\src\\Fusion\\Sketch\\Server\\Sketch\\ConstraintSolver'
                print("folder: " + folder)
                files = os.listdir(folder)
                for f in files:
                        ftmp = os.path.join(folder, f)
                        if os.path.isfile(ftmp):
                                myutil.log(f)
                                self.target.getClasses(ftmp)   
                
        def testfile(self):
                myutil.log2('test file')
                #self.target.getClasses('D:\tech\python\Python33\CodeCoverageHelper\src\Fusion\Sketch\Server\Sketch\ConstraintSolver\SketchConstraintSolver.h')
                self.target.getClasses('C:\SketchConstraintSolver.h')
                
        def testcasewrong(self):
                myutil.log2('test wrong cases')
                self.test('class Sket20chOffsetCmd2 public baseClass, baseclass2 	{	 ')

        def testcasenormal(self):
                myutil.log2('test normal cases')
                self.test('class Sket20chOffsetCmd2')
                self.test('class Sket20chOffsetCmd2  ')
                self.test('class Sket20chOffsetCmd2{        //niaho')
                self.test('class 		Sket20chOffsetCmd2{          //niaho		')
                self.test('class Sket20chOffsetCmd2 	{')
                self.test('class Sket20chOffsetCmd2 : public baseClass')
                
                self.test('class Sket20chOffsetCmd2 : public baseClass')
                self.test('class Sket20chOffsetCmd2 : public baseClass, baseclass2 	{	 ')

                self.test('class  		_EXPORT_API  	_Sket20chOffsetCmd2')
                self.test('class  		_EXPORT_API  	_Sket20chOffsetCmd2')
                self.test('class  		_EXPORT_API  	_Sket20chOffsetCmd2')
                self.test('class  		_EXPORT_API  	_Sket20chOffsetCmd2')
                self.test('class  		_EXPORT_API  	_Sket20chOffsetCmd2 : baseclass')
                self.test('class  		_EXPORT_API  	_Sket20chOffsetCmd2')
                self.test('class  		_EXPORT_API  	_Sket20chOffsetCmd2')
                self.test('class  		_EXPORT_API  	_Sket20chOffsetCmd2')
                self.test('class  		_EXPORT_API  	_Sket20chOffsetCmd2')
                self.test('class  		_EXPORT_API  	_Sket20chOffsetCmd2 : baseclass')

        def testall(self):
                self.testcasenormal()
                self.testcasewrong()
                self.testfile()
                self.testfolder()

#============================================================
# Test

def test():
        myutil.log1("Run Tests")
        t = ClassParserTest()
        t.testall()

        myutil.log1("call run method")
        res = run(r'D:\tech\python\Python33\CodeCoverageHelper\src\Fusion\Sketch\Server\Sketch\ConstraintSolver\SketchConstraintSolver.h')
        printresult(res)

if __name__ == '__main__':
        test()

#============================================================
# old codes
def test1():
	p = re.compile('class[\s\t]+')
	p.match('class ').group()
	# test 2 spaces
	p.match('class  ').group()
	#test tab
	p.match('class	').group()
	#test space and tab
	p.match('class 	 	').group()
	
def test2(matchstr):
        res = p2.match(matchstr).group()
        logging.debug(res)

def oldcodes():
        # format:  class [\s\t]+ [EXPORT_API] classname [:] [public baseclass] [{]
        spaceExp = '[\s\t]+'
        spaceExpOptional = '[\s\t]*'
        exportMacroExp = spaceExp + '[a-zA-Z_][a-zA-Z_0-9]+'
        classnameExp = exportMacroExp
        leftBrace = spaceExpOptional + '\{?'
        # : public baseclass1, baseclass2
        baseClassExp = spaceExpOptional + ':?' + spaceExpOptional + '[a-zA-Z_][a-zA-Z_0-9,\s\t]+'

        #expresion 1: e.g. class SketchOffsetCmd [{]
        exp1 = 'class' + classnameExp + baseClassExp + leftBrace + '[^:\{]*$'
        #expresion 2: e.g. class EXPORT_API OffsetUtil [{]
        exp2 = 'class' + exportMacroExp + classnameExp + baseClassExp + leftBrace

        exp3 = 'class' + classnameExp + '[^;]*'

        logging.debug(exp1)
        p1 = re.compile(exp3)
        p2 = re.compile(exp2)   
