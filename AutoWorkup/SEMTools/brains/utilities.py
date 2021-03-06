# -*- coding: utf8 -*- 
"""Autogenerated file - DO NOT EDIT
If you spot a bug, please report it on the mailing list and/or change the generator."""

from nipype.interfaces.base import CommandLine, CommandLineInputSpec, SEMLikeCommandLine, TraitedSpec, File, Directory, traits, isdefined, InputMultiPath, OutputMultiPath
import os


class HistogramMatchingFilterInputSpec(CommandLineInputSpec):
    inputVolume = File(desc="The Input image to be computed for statistics", exists=True, argstr="--inputVolume %s")
    referenceVolume = File(desc="The Input image to be computed for statistics", exists=True, argstr="--referenceVolume %s")
    outputVolume = traits.Either(traits.Bool, File(), hash_files=False, desc="Output Image File Name", argstr="--outputVolume %s")
    referenceBinaryVolume = File(desc="referenceBinaryVolume", exists=True, argstr="--referenceBinaryVolume %s")
    inputBinaryVolume = File(desc="inputBinaryVolume", exists=True, argstr="--inputBinaryVolume %s")
    numberOfMatchPoints = traits.Int(desc=" number of histogram matching points", argstr="--numberOfMatchPoints %d")
    numberOfHistogramBins = traits.Int(desc=" number of histogram bin", argstr="--numberOfHistogramBins %d")
    writeHistogram = traits.Str(desc=" decide if histogram data would be written with prefixe of the file name", argstr="--writeHistogram %s")
    histogramAlgorithm = traits.Enum("OtsuHistogramMatching", desc=" histogram algrithm selection", argstr="--histogramAlgorithm %s")
    verbose = traits.Bool(desc=" verbose mode running for debbuging", argstr="--verbose ")


class HistogramMatchingFilterOutputSpec(TraitedSpec):
    outputVolume = File(desc="Output Image File Name", exists=True)


class HistogramMatchingFilter(SEMLikeCommandLine):
    """title: Write Out Image Intensities

category: BRAINS.Utilities

description:  For Analysis

version: 0.1

contributor: University of Iowa Department of Psychiatry, http:://www.psychiatry.uiowa.edu
  

"""

    input_spec = HistogramMatchingFilterInputSpec
    output_spec = HistogramMatchingFilterOutputSpec
    _cmd = " HistogramMatchingFilter "
    _outputs_filenames = {'outputVolume':'outputVolume.nii'}
