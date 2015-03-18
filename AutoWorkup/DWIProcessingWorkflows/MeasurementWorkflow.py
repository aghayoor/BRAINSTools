## \author Ali Ghayoor
##
## This workflow computes the statistics of RISs over regions of interest from the input label map.
##

import os
import SimpleITK as sitk
import nipype
from nipype.interfaces import ants
from nipype.interfaces.base import CommandLine, CommandLineInputSpec, TraitedSpec, File, Directory
from nipype.interfaces.base import traits, isdefined, BaseInterface
from nipype.interfaces.utility import Merge, Split, Function, Rename, IdentityInterface
import nipype.interfaces.io as nio   # Data i/oS
import nipype.pipeline.engine as pe  # pypeline engine
from SEMTools import *

def CreateMeasurementWorkflow(WFname):

    ###### UTILITY FUNCTIONS #######
    def CreateDWILabelMap(T2LabelMapVolume,DWIBrainMask):
        import SimpleITK as sitk
        # 1- Dilate input DWI mask
        dilateFilter = sitk.BinaryDilateImageFilter()
        dilateFilter.SetKernelRadius(1)
        dilated_mask = dilateFilter.Execute( DWIBrainMask )
        # 2- Resample dilated mask to the space of T2LabelMap (1x1x1)
        # Use Linear interpolation + thresholding
        resFilt = sitk.ResampleImageFilter()
        resFilt.SetReferenceImage(T2LabelMapVolume)
        resFilt.SetOutputPixelType(sitk.sitkFloat32)
        resFilt.SetInterpolator(sitk.sitkLinear)
        dilated_resampled_mask = resFilt.Execute(dilated_mask)
        # Thresholding by 0
        threshFilt = sitk.BinaryThresholdImageFilter()
        thresh_dilated_resampled_mask = threshFilt.Execute(label_mask_res_1,0.0001,1.0,1,0)
        # 3- Multiply this binary mask to the T2 labelmap volume
        DWILabelMapVolume = thresh_dilated_resampled_mask * T2LabelMapVolume
        return DWILabelMapVolume

    #################################

    MeasurementWF = pe.Workflow(name=WFname)

    inputsSpec = pe.Node(interface=IdentityInterface(fields=['T2LabelMapVolume','DWIBrainMask',
                                                             'FAImage','MDImage','RDImage','FrobeniusNormImage',
                                                             'Lambda1Image','Lambda2Image','Lambda3Image']),
                         name='inputsSpec')

    outputsSpec = pe.Node(interface=IdentityInterface(fields=['FA_stats','MD_stats','RD_stats','FrobeniusNorm_stats',
                                                              'Lambda1_stats','Lambda2_stats','Lambda2_stats']),
                          name='outputsSpec')

    # Step1: Create the labelmap volume for DWI scan
    CreateDWILabelMapNode = pe.Node(interface=Function(function = CreateDWILabelMap,
                                                       input_names=['T2LabelMapVolume','DWIBrainMask'],
                                                       output_names=['DWILabelMapVolume']),
                                    name="CreateDWILabelMap")
    MeasurementWF.connect(inputsSpec, 'T2LabelMapVolume', CreateDWILabelMapNode, 'T2LabelMapVolume')
    MeasurementWF.connect(inputsSpec, 'DWIBrainMask', CreateDWILabelMapNode, 'DWIBrainMask')

    # Now we have two labelmap volumes (both have 1x1x1 voxel lattice):
    # (1) T2LabelMap: Used to compute total_volume for each label
    # (2) DWILabelMap: It is probably cropped and missed some part of labels,
    #                  and is used to compute all stats like [mean,std,max,min,median,effective_volume].
