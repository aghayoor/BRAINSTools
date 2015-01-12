#! /usr/bin/env python
"""
DWIWorkFlow.py
==============

The purpose of this pipeline is to complete all the pre-processing steps needed to turn diffusion-weighted images into FA images that will be used to build a template diffusion tensor atlas for fiber tracking.

Usage:
  DWIWorkFlow.py --inputDWIScan DWISCAN [--baseDIR BASEDIR]
  DWIWorkFlow.py -v | --version
  DWIWorkFlow.py -h | --help

Options:
  -h --help                   Show this help and exit
  -v --version                Print the version and exit
  --inputDWIScan DWISCAN      Path to the input DWI scan for further processing
  --baseDIR BASEDIR           Base directory that outputs will be written to

"""

def runWorkflow(DWISCAN, BASEDIR):
    print("Running the workflow ...")

if __name__ == '__main__':
  import sys
  import os

  from docopt import docopt
  argv = docopt(__doc__, version='1.0')
  print argv

  DWISCAN = argv['--inputDWIScan']
  assert os.path.exists(DWISCAN), "Input DWI scan is not found: %s" % DWISCAN

  if argv['--baseDIR'] == None:
      print("*** base directory is set to current working directory.")
      BASEDIR = os.getcwd()
  else:
      BASEDIR = argv['--baseDIR']
  print '=' * 100

  exit = runWorkflow(DWISCAN, BASEDIR)

  sys.exit(exit)
