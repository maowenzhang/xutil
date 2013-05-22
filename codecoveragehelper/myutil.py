import logging
logging.basicConfig(level=logging.INFO)
#===================================================
# myutil:   used to keep utility methods
#

#===================================================
# log methods
#
def log0(msg):
        logging.info(" ")
        logging.info("========================= " + msg)
        logging.info(" ")

def log1(msg):
        logging.info(" ")
        logging.info("*************** #: " + msg)
        logging.info(" ")
def log2(msg):
        logging.info(" ")
        logging.info("====== " + msg)
def log(msg):
        logging.info("------ " + msg)


runtest = True
