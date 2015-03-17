## \author Ali Ghayoor
##
## This workflow runs compressed Sensing on a DWI scan
## that is already corrected and aligned to t2 image space
## using the CorrectionWF.

import nipype
from nipype.interfaces import ants
from nipype.interfaces.base import CommandLine, CommandLineInputSpec, TraitedSpec, File, Directory
from nipype.interfaces.base import traits, isdefined, BaseInterface
from nipype.interfaces.utility import Merge, Split, Function, Rename, IdentityInterface
import nipype.interfaces.io as nio   # Data i/oS
import nipype.pipeline.engine as pe  # pypeline engine
from nipype.interfaces.freesurfer import ReconAll
from SEMTools import *

def CreateCSWorkflow(WFname, BASE_DIR):

    CSWF = pe.Workflow(name=WFname)
    CSWF.base_dir = BASE_DIR

    inputsSpec = pe.Node(interface=IdentityInterface(fields=['DWI_Corrected_Aligned', 'DWIBrainMask']),
                         name='inputspec')

    outputsSpec = pe.Node(interface=IdentityInterface(fields=['DWI_Corrected_Aligned_CS']),
                          name='outputsSpec')
