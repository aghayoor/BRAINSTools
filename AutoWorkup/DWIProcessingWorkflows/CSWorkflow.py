## \author Ali Ghayoor
##
## This workflow runs compressed Sensing on a DWI scan
## that is already corrected and aligned to t2 image space
## using the CorrectionWF.

import os
import nipype
from nipype.interfaces import ants
from nipype.interfaces.base import CommandLine, CommandLineInputSpec, TraitedSpec, File, Directory
from nipype.interfaces.base import traits, isdefined, BaseInterface
from nipype.interfaces.utility import Merge, Split, Function, Rename, IdentityInterface
import nipype.interfaces.io as nio   # Data i/oS
import nipype.pipeline.engine as pe  # pypeline engine
import nipype.interfaces.matlab as matlab

def CreateCSWorkflow(WFname, PYTHON_AUX_PATHS):

    if PYTHON_AUX_PATHS is not None:
       Path_to_Matlab_Func = os.path.join(PYTHON_AUX_PATHS[0],'DWIProcessingWorkflows')

    #Path_to_Matlab_Func = os.path.join(sys.path,'DWIProcessingWorkflows')
    #assert os.path.exists(Path_to_Matlab_Func), "Path to CS matlab function is not found: %s" % Path_to_Matlab_Func

    #### Utility function
    def createMatlabScript(inputScan,inputMask,CSScanFileName):
        matlabScript="runCS('"+inputScan+"','"+inputMask+"','"+CSScanFileName+"')"
        DWI_CS = CSScanFileName # output CS filename
        return matlabScript, DWI_CS
    #####################

    CSWF = pe.Workflow(name=WFname)

    inputsSpec = pe.Node(interface=IdentityInterface(fields=['DWI_Corrected_Aligned', 'DWIBrainMask']),
                         name='inputsSpec')

    outputsSpec = pe.Node(interface=IdentityInterface(fields=['DWI_Corrected_Aligned_CS']),
                          name='outputsSpec')


    createMatlabScriptNode = pe.Node(interface=Function(function = createMatlabScript,
                                                      input_names=['inputScan','inputMask','CSScanFileName'],
                                                      output_names=['matlabScript','DWI_CS']),
                                     name="createMatlabScriptNode")
    createMatlabScriptNode.inputs.CSScanFileName = 'DWI_Corrected_Aligned_CS.nrrd'
    CSWF.connect([(inputsSpec,createMatlabScriptNode,[('DWI_Corrected_Aligned','inputScan'),('DWIBrainMask','inputMask')])])

    runCS=pe.Node(interface=matlab.MatlabCommand(),name="runCSbyMatlab")
    matlab.MatlabCommand().set_default_matlab_cmd("matlab")
    runCS.inputs.single_comp_thread = False
    runCS.inputs.nodesktop = True
    runCS.inputs.nosplash = True
    runCS.inputs.paths = Path_to_Matlab_Func
    CSWF.connect(createMatlabScriptNode,'matlabScript',runCS,'script')

    CSWF.connect(createMatlabScriptNode,'DWI_CS',outputsSpec,'DWI_Corrected_Aligned_CS')

    return CSWF
