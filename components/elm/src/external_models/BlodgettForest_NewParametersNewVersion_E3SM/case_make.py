#!/usr/bin/env python
#build script
import os,sys,argparse
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--task', dest='task', metavar='task', type=str, nargs=1, default=['none'],help='task for case_make.py: config, install or distclean')
parser.add_argument('--CC', dest='CC', metavar='CC', type=str, nargs=1, default=['gcc8'],help='C compiler')
parser.add_argument('--CXX', dest='CXX', metavar='CXX', type=str, nargs=1, default=['g++8'],help='C++ compiler')
parser.add_argument('--FC', dest='FC', metavar='FC', type=str, nargs=1, default=['gf8'],help='Fortran compiler')
parser.add_argument('--extra', dest='extra', metavar='extra', type=str, nargs=1, default=[''],help='other options')
args = parser.parse_args()
task =args.task[0]
path='/global/u1/j/jingtao/E3SM-PM/E3SM_Master_Uptodate/components/elm/src/external_models//sbetr/'
os.chdir(path)
if task=='clean':
    os.system('make distclean')
elif task=='config':
    command='make config CC='+args.CC[0]+' CXX='+args.CXX[0]+' FC='+args.FC[0]+' '+args.extra[0]
    os.system(command)
elif task=='install':
    command='make install CC='+args.CC[0]+' CXX='+args.CXX[0]+' FC='+args.FC[0]+' '+args.extra[0]
    os.system(command)
    command='cp local/bin/sbetr /global/homes/j/jingtao/E3SM-PM/E3SM_Master_Uptodate/components/elm/src/external_models/BlodgettForest_NewParametersNewVersion_E3SM/'
    os.system(command)
    command='cp local/bin/jarmodel /global/homes/j/jingtao/E3SM-PM/E3SM_Master_Uptodate/components/elm/src/external_models/BlodgettForest_NewParametersNewVersion_E3SM/'
    os.system(command)
