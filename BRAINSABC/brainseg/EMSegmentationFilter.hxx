/*=========================================================================
 *
 *  Copyright SINAPSE: Scalable Informatics for Neuroscience, Processing and Software Engineering
 *            The University of Iowa
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef __EMSegmentationFilter_hxx
#define __EMSegmentationFilter_hxx

#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>

#include <cmath>
#include <cstdlib>

#include "itkAddImageFilter.h"
#include "itkBSplineDownsampleImageFilter.h"
#include "itkBinaryBallStructuringElement.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkBinaryErodeImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkBlendImageFilter.h"
#include "itkConnectedComponentImageFilter.h"
#include "itkDiscreteGaussianImageFilter.h"
#include "itkHistogramMatchingImageFilter.h"
#include "itkImageDuplicator.h"
#include "itkImageRegionConstIterator.h"
#include "itkImageRegionIterator.h"
#include "itkMinimumMaximumImageCalculator.h"
#include "itkMultiModeHistogramThresholdBinaryImageFilter.h"
#include "itkMultiplyImageFilter.h"
#include "itkNumericTraits.h"
#include "itkRelabelComponentImageFilter.h"
#include "itkResampleImageFilter.h"
// #include "itkMersenneTwisterRandomVariateGenerator.h"

#include "vnl/algo/vnl_determinant.h"
#include "vnl/vnl_math.h"
#include "vnl_index_sort.h"

#include "itkBRAINSROIAutoImageFilter.h"
//#include "BRAINSFitBSpline.h"
#include "BRAINSFitUtils.h"
#ifdef USE_ANTS
#include "BRAINSFitSyN.h"
#endif
#include "BRAINSABCUtilities.h"
#include "LLSBiasCorrector.h"

// #include "QHullMSTClusteringProcess.h"
#include "AtlasDefinition.h"
#include "EMSegmentationFilter.h"
#include "ExtractSingleLargestRegion.h"
#include "PrettyPrintTable.h"
#include "ComputeDistributions.h"

#include "itkImageRandomNonRepeatingConstIteratorWithIndex.h"

template <class TInputImage, class TProbabilityImage>
EMSegmentationFilter<TInputImage, TProbabilityImage>
::EMSegmentationFilter()
{
  m_DirtyLabels = NULL;
  m_CleanedLabels = NULL;
  m_ThresholdedLabels = NULL;
  m_DirtyThresholdedLabels = NULL;

  m_SampleSpacing = 2.0;

  // Bias
  m_MaxBiasDegree = 4;
  m_BiasLikelihoodTolerance = 1e-2;
  // NOTE: warp tol needs to be <= bias tol
  m_WarpLikelihoodTolerance = 1e-3;

  // EM convergence parameters
  m_LikelihoodTolerance = 1e-5;
  m_MaximumIterations = 40;

  m_PriorWeights = VectorType(0);
  m_PriorWeightsSet = false;

  // m_PriorGaussianClusterCountVector = IntVectorType(0);
  // m_PriorGaussianClusterCountVectorSet=false;

  m_PriorLabelCodeVector = IntVectorType(0);
  m_PriorLabelCodeVectorSet = false;

  m_PriorUseForBiasVector = BoolVectorType(0);
  m_PriorUseForBiasVectorSet = false;

  m_PriorIsForegroundPriorVector = BoolVectorType(false);
  m_PriorIsForegroundPriorVectorSet = false;

  m_PriorsBackgroundValues.resize(0);

  m_WarpedPriors.clear();
  m_OriginalSpacePriors.clear();
  m_Posteriors.clear();

  m_InputImages.clear();
  m_RawInputImages.clear();
  m_CorrectedImages.clear();
  m_RawCorrectedImages.clear();

  m_TemplateBrainMask = NULL;
  m_OriginalAtlasImages.clear();
  m_WarpedAtlasImages.clear();

  m_OutputDebugDir = "";
  // m_PriorLookupTable = IntVectorType(0);

  m_NonAirRegion = 0;

  m_AtlasTransformType = "SyN"; // "invalid_TransformationTypeNotSet";

  m_UpdateTransformation = false;

  m_DebugLevel = 0;

  m_TemplateGenericTransform = NULL;

  m_WarpGrid[0] = 5;
  m_WarpGrid[1] = 5;
  m_WarpGrid[2] = 5;

  m_UpdateRequired = true;
  m_AirIndex = 1;
  this->m_PriorNames.clear();
  // m_ClassToPriorMapping.clear();
}

template <class TInputImage, class TProbabilityImage>
EMSegmentationFilter<TInputImage, TProbabilityImage>
::~EMSegmentationFilter()
{
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::CheckInput()
{
  if( m_WarpedPriors.size() < 1 )
    {
    itkExceptionMacro(<< "Must have one or more class probabilities" << std::endl );
    }

  if( m_PriorWeightsSet == false )
    {
    itkExceptionMacro(<< "The PriorWeights were not set." << std::endl );
    }
  if( m_PriorLabelCodeVectorSet == false )
    {
    itkExceptionMacro(<< "The PriorLabelCodeVector was not set." << std::endl );
    }
  if( m_PriorUseForBiasVectorSet == false )
    {
    itkExceptionMacro(<< "The PriorUseForBiasVector was not set." << std::endl );
    }
  if( m_PriorIsForegroundPriorVectorSet == false )
    {
    itkExceptionMacro(<< "The PriorIsForegroundPriorVector was not set." << std::endl );
    }

  if( m_WarpedPriors.size() != m_PriorWeights.size() )
    {
    itkExceptionMacro(<< "The PriorWeights vector size must match the"
                      << " number of priors listed." << std::endl );
    }
  if( m_WarpedPriors.size() != m_PriorLabelCodeVector.size() )
    {
    itkExceptionMacro(<< "The PriorLabelCodeVector vector size must match the"
                      << " number of priors listed." << std::endl );
    }
  if( m_WarpedPriors.size() != m_PriorUseForBiasVector.size() )
    {
    itkExceptionMacro(<< "The PriorUseForBiasVector vector size must match the"
                      << " number of priors listed." << std::endl );
    }
  if( m_WarpedPriors.size() != m_PriorIsForegroundPriorVector.size() )
    {
    itkExceptionMacro(<< "The PriorIsForegroundPriorVector vector size"
                      << " must match the number of priors listed." << std::endl );
    }

  if( m_MaximumIterations == 0 )
    {
    itkWarningMacro(<< "Maximum iterations set to zero" << std::endl );
    }

  if( m_InputImages.empty() )
    {
    itkExceptionMacro(<< "No input images" << std::endl );
    }

  const InputImageSizeType size =
    this->GetFirstInputImage()->GetLargestPossibleRegion().GetSize();

  for(typename MapOfInputImageVectors::iterator mapIt = this->m_InputImages.begin();
      mapIt != this->m_InputImages.end(); ++mapIt)
    {
    for(typename InputImageVector::iterator imIt = mapIt->second.begin();
        imIt != mapIt->second.end(); ++imIt)
      {
      if( (*imIt)->GetImageDimension() != 3 )
        {
        itkExceptionMacro(<< "InputImage ["
                          << mapIt->first << " " << std::distance(mapIt->second.begin(),imIt)
                          << "] has invalid dimension: only supports 3D images" << std::endl );
        }
      const InputImageSizeType isize = (*imIt)->GetLargestPossibleRegion().GetSize();
      if( size != isize )
        {
        itkExceptionMacro(<< "Image data ["
                          << mapIt->first << " " << std::distance(mapIt->second.begin(),imIt)
                          << "] 3D size mismatch "
                          << size << " != " << isize << "." << std::endl );
        }
      }
    }
  for( unsigned i = 0; i < m_WarpedPriors.size(); i++ )
    {
    if( m_WarpedPriors[i]->GetImageDimension() != 3 )
      {
      itkExceptionMacro(<< "Warped Prior [" << i << "] has invalid dimension: only supports 3D images" << std::endl );
      }
    const ProbabilityImageSizeType psize = m_WarpedPriors[i]->GetLargestPossibleRegion().GetSize();
    if( size != psize )
      {
      itkExceptionMacro(<< "Warped prior [" << i << "] and atlas data 3D size mismatch"
                        << size << " != " << psize << "."
                        << std::endl );
      }
    }

  const InputImageSizeType atlasSize =
    this->GetFirstOriginalAtlasImage()->GetLargestPossibleRegion().GetSize();
  for(typename MapOfInputImageVectors::iterator mapIt = this->m_OriginalAtlasImages.begin();
      mapIt != this->m_OriginalAtlasImages.end(); ++mapIt)
    {
    for(typename InputImageVector::iterator imIt = mapIt->second.begin();
        imIt != mapIt->second.end(); ++imIt)
      {
      if( (*imIt)->GetImageDimension() != 3 )
        {
        itkExceptionMacro(<< "Atlas Image ["
                          << mapIt->first << " "
                          << std::distance(mapIt->second.begin(),imIt)
                          << "] has invalid dimension: only supports 3D images" << std::endl );
        }
      const InputImageSizeType asize = (*imIt)->GetLargestPossibleRegion().GetSize();
      if( atlasSize != asize )
        {
        itkExceptionMacro(<< "Image data ["
                          << mapIt->first << " "
                          << std::distance(mapIt->second.begin(),imIt)
                          << "] 3D size mismatch "
                          << atlasSize << " != " << asize << "." << std::endl );
        }
      }
    }

  for(typename ProbabilityImageVectorType::iterator imIt = this->m_OriginalSpacePriors.begin();
      imIt != this->m_OriginalSpacePriors.end(); ++imIt)
    {
    if( (*imIt)->GetImageDimension() != 3 )
      {
      itkExceptionMacro(<< "Prior ["
                        << std::distance(this->m_OriginalSpacePriors.begin(),imIt)
                        << "] has invalid dimension: only supports 3D images" << std::endl );
      }
    const ProbabilityImageSizeType psize = (*imIt)->GetLargestPossibleRegion().GetSize();
    if( atlasSize != psize )
      {
      itkExceptionMacro(<< "Normalized prior ["
                        << std::distance(this->m_OriginalSpacePriors.begin(),imIt)
                        << "] and atlas 3D size mismatch"
                        << atlasSize << " != " << psize << "."
                        << std::endl );
      }
    }
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::SetInputImages(const MapOfInputImageVectors newInputImages)
{
  muLogMacro(<< "SetInputImages" << std::endl);

  if( newInputImages.size() == 0 )
    {
    itkExceptionMacro(<< "No input images" << std::endl );
    }

  m_InputImages = newInputImages;

  this->Modified();
  m_UpdateRequired = true;
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::SetRawInputImages(const MapOfInputImageVectors newInputImages)
{
  muLogMacro(<< "SetRawInputImages" << std::endl);

  if( newInputImages.size() == 0 )
    {
    itkExceptionMacro(<< "No input images" << std::endl );
    }

  m_RawInputImages = newInputImages;

  this->Modified();
  m_UpdateRequired = true;
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::SetOriginalAtlasImages(const MapOfInputImageVectors newAtlasImages)
{
  muLogMacro(<< "SetAtlasImages" << std::endl);

  if( newAtlasImages.size() == 0 )
    {
    itkExceptionMacro(<< "No template images" << std::endl );
    }
  m_OriginalAtlasImages = newAtlasImages;

  this->Modified();
  m_UpdateRequired = true;
}

template <class TInputImage, class TProbabilityImage>
void EMSegmentationFilter<TInputImage, TProbabilityImage>
::WriteDebugPosteriors(const unsigned int ComputeIterationID) const
{
  if( this->m_DebugLevel > 9 )
    {
    // write out posteriors
    const unsigned int numPosteriors = this->m_Posteriors.size();
    const unsigned int write_posteriors_level = ComputeIterationID; // DEBUG:
                                                                    //  This
                                                                    // code is
                                                                    // for
                                                                    // debugging
                                                                    // purposes
                                                                    // only;
    std::stringstream write_posteriors_level_stream;
    write_posteriors_level_stream << write_posteriors_level;
    for( unsigned int iprob = 0; iprob < numPosteriors; iprob++ )
      {
      typedef itk::ImageFileWriter<TProbabilityImage> ProbabilityImageWriterType;
      typename ProbabilityImageWriterType::Pointer writer = ProbabilityImageWriterType::New();

      std::stringstream template_index_stream("");
      template_index_stream << iprob;
      const std::string fn = this->m_OutputDebugDir + "/POSTERIOR_INDEX_" + template_index_stream.str() + "_"
        + this->m_PriorNames[iprob] + "_LEVEL_" + write_posteriors_level_stream.str() + ".nii.gz";

      muLogMacro(<< "Writing posterior images... " << fn <<  std::endl);
      writer->SetInput(m_Posteriors[iprob]);
      writer->SetFileName(fn);
      writer->UseCompressionOn();
      writer->Update();
      }
    }
  return;
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::SetPriors(ProbabilityImageVectorType priors)
{
  muLogMacro(<< "Set and Normalize for segmentation." << std::endl);
  // Need to normalize priors before getting started.
  this->m_OriginalSpacePriors = priors;
  ZeroNegativeValuesInPlace<TProbabilityImage>(this->m_OriginalSpacePriors);
  NormalizeProbListInPlace<TProbabilityImage>(this->m_OriginalSpacePriors);
  this->m_OriginalSpacePriors = priors;
  this->Modified();
  m_UpdateRequired = true;
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::SetPriorWeights(VectorType w)
{
  muLogMacro(<< "SetPriorWeights" << std::endl);

  if( w.size() != m_OriginalSpacePriors.size() )
    {
    itkExceptionMacro(<< "Number of prior weights invalid"
                      << w.size() << " != " << m_OriginalSpacePriors.size() );
    }
  for( unsigned i = 0; i < w.size(); i++ )
    {
    if( w[i] == 0.0 )
      {
      itkExceptionMacro(<< "Prior weight "
                        << i << " is zero" << std::endl );
      }
    }

  m_PriorWeights = w;
  m_PriorWeightsSet = true;
  this->Modified();
  m_UpdateRequired = true;
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::SetPriorLabelCodeVector(IntVectorType ng)
{
  muLogMacro(<< "SetPriorLabelCodeVector" << std::endl );
  if( ng.size() == 0 )
    {
    itkExceptionMacro(<< "Number of clusters info invalid" << std::endl );
    }
  const unsigned int numPriors = m_WarpedPriors.size();
  for( unsigned int i = 0; i < numPriors; i++ )
    {
    if( ng[i] == 0 )
      {
      itkExceptionMacro(<< "PriorLabelCode" << i << " is zero" << std::endl );
      }
    }
  m_PriorLabelCodeVector = ng;
  m_PriorLabelCodeVectorSet = true;
  this->Modified();
  m_UpdateRequired = true;
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::SetPriorUseForBiasVector(const BoolVectorType& ng)
{
  muLogMacro(<< "SetPriorUseForBiasVector" << std::endl );
  if( ng.size() == 0 )
    {
    itkExceptionMacro(<< "Vector size for PriorUseForBiasVector info invalid" << std::endl );
    }
  const unsigned int numPriors = m_WarpedPriors.size();
  for( unsigned int i = 0; i < numPriors; i++ )
    {
    if( ng[i] != 0 && ng[i] != 1 )
      {
      itkExceptionMacro(<< "PriorUseForBiasVector" << i << " can only be 0 or 1" << std::endl );
      }
    }
  m_PriorUseForBiasVector = ng;
  m_PriorUseForBiasVectorSet = true;
  this->Modified();
  m_UpdateRequired = true;
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::SetPriorIsForegroundPriorVector(const BoolVectorType& ng)
{
  muLogMacro(<< "SetPriorIsForegroundPriorVector" << std::endl );
  if( ng.size() == 0 )
    {
    itkExceptionMacro(<< "Vector size for PriorIsForegroundPriorVector info invalid" << std::endl );
    }
  const unsigned int numPriors = m_WarpedPriors.size();
  for( unsigned int i = 0; i < numPriors; i++ )
    {
    if( ng[i] != 0 && ng[i] != 1 )
      {
      itkExceptionMacro(<< "PriorIsForegroundPriorVector" << i << " can only be 0 or 1" << std::endl );
      }
    }
  m_PriorIsForegroundPriorVector = ng;
  m_PriorIsForegroundPriorVectorSet = true;
  this->Modified();
  m_UpdateRequired = true;
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::
ByteImagePointer
EMSegmentationFilter<TInputImage, TProbabilityImage>
::GetThresholdedOutput(void)
{
  // TODO:  This assumes that GetOutput was already called.  This should be made
  // more intelligent
  return m_DirtyThresholdedLabels;
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::
ByteImagePointer
EMSegmentationFilter<TInputImage, TProbabilityImage>
::GetCleanedOutput(void)
{
  // TODO:  This assumes that GetOutput was already called.  This should be made
  // more intelligent
  return m_CleanedLabels;
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::
ByteImagePointer
EMSegmentationFilter<TInputImage, TProbabilityImage>
::GetOutput(void)
{
  this->Update();
  return m_DirtyLabels;
}

template <class TInputImage, class TProbabilityImage>
std::vector<
  typename EMSegmentationFilter<TInputImage, TProbabilityImage>::
  ProbabilityImagePointer>
EMSegmentationFilter<TInputImage, TProbabilityImage>
::GetPosteriors()
{
  return m_Posteriors;
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::MapOfInputImageVectors
EMSegmentationFilter<TInputImage, TProbabilityImage>
::GetCorrected()
{
  return m_CorrectedImages;
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::MapOfInputImageVectors
EMSegmentationFilter<TInputImage, TProbabilityImage>
::GetRawCorrected()
{
  return m_RawCorrectedImages;
}

// HACK -- THIS METHOD IS NEVER CALLED
template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::CheckLoopAgainstFilterOutput(ByteImagePointer &loopImg, ByteImagePointer & filterImg)
{
  typedef typename itk::ImageRegionConstIterator<ByteImageType> IterType;

  IterType     maskIter(loopImg, loopImg->GetLargestPossibleRegion() );
  IterType     dilIter(filterImg, filterImg->GetLargestPossibleRegion() );
  unsigned int count = 0;
  for( maskIter.GoToBegin(), dilIter.GoToBegin();
       !maskIter.IsAtEnd() && !dilIter.IsAtEnd(); ++maskIter, ++dilIter )
    {
    if( maskIter.Value() != dilIter.Value() )
      {
      std::cerr << "mask = " << static_cast<float>(maskIter.Value() )
                << " dilated = " << static_cast<float>(dilIter.Value() )
                << " at vIndex " << maskIter.GetIndex()
                << std::endl;
      count++;
      }
    }
  if( count == 0 )
    {
    muLogMacro( << "DEBUG:  Filter output same as after loop output!" << std::endl);
    }
}

template <class TInputImage, class TProbabilityImage>
std::vector<RegionStats>
EMSegmentationFilter<TInputImage, TProbabilityImage>
::ComputeDistributions(const ByteImageVectorType & SubjectCandidateRegions,
                       const ProbabilityImageVectorType & probAllDistributions)
{
  // IPEK This may be the only place where the images grouped by type are
  // needed for computation.
  muLogMacro(<< "Computing Distributions" << std::endl );
  const ProbabilityImageVectorType & probabilityMaps = probAllDistributions;

  std::vector<RegionStats> outputStats;
  typedef vnl_matrix<FloatingPrecision> MatrixType;
  // IPEK CombinedComputeDistributions is also used by
  // LLSBiasCorrector which unfortunately does NOT keep track of the
  // types of input images.  So there may need to be a custom version
  // of this method that groups by image type
  CombinedComputeDistributions<TInputImage, TProbabilityImage, MatrixType>(SubjectCandidateRegions,
                                                                           this->m_CorrectedImages,
                                                                           probabilityMaps,
                                                                           outputStats,
                                                                           this->m_DebugLevel,
                                                                           false);

  return outputStats;
}

static double ComputeCovarianceDeterminant( const vnl_matrix<FloatingPrecision> & currCovariance)
{
  const FloatingPrecision detcov = vnl_determinant(currCovariance);

  if( detcov <= 0.0 )
    {
    itkGenericExceptionMacro(<< "Determinant of covariance "
                             << " is <= 0.0 (" << detcov << "), covariance matrix:"
                             << std::endl << currCovariance
                             << "\n\n\n This is indicative of providing two images"
                             << " that are related only through a linear depenancy\n"
                             << "at least two images are so close in their ratio of"
                             << " values that a degenerate covariance matrix\n"
                             << "would result, thus making an unstable calculation\n\n\n");
    }
  return detcov;
}

///////////////////////////////////////////////// Posterior computation by kNN //////////////////////////////////////////////
template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::kNNCore( const vnl_matrix<FloatingPrecision> & trainMatrix,
          const vnl_vector<FloatingPrecision> & labelVector,
          const vnl_matrix<FloatingPrecision> & testMatrix,
          vnl_matrix<FloatingPrecision> & liklihoodMatrix,
          unsigned int K )
{
  unsigned int numClasses = labelVector.max_value() + 1; // index starts from zero
  unsigned int numTraining = trainMatrix.rows(); // number of training data
  unsigned int numTest = testMatrix.rows(); // number of test data
  //unsigned int numFeatures = trainMatrix.columns(); // number of features

    // represent each class label as an array
  vnl_matrix<FloatingPrecision> localLabels(numTraining, numClasses, 0);
  for( unsigned int iTrain = 0; iTrain < numTraining; ++iTrain )
    {
    localLabels( iTrain, labelVector(iTrain) ) = 1;
    }

  // Compute Likelihood matrix
  for( unsigned int iTest = 0; iTest < numTest; ++iTest ) ///////
    {
    // compute the distances
    vnl_vector<FloatingPrecision> distances( numTraining, 0 ); // distance vector for ith test data
    for( unsigned int train_i = 0; train_i < numTraining; ++train_i )
      {
      distances(train_i) = ( testMatrix.get_row(iTest) - trainMatrix.get_row(train_i) ).two_norm();
      }
/*
////////////
    vnl_matrix<FloatingPrecision> tempDataMatrix( numTraining, numFeatures, 0 );
    for( unsigned int iRow = 0; iRow < numTraining; ++iRow )
      {
      tempDataMatrix.set_row( iRow, testMatrix.get_row( iTest ) ); // repeat the current testData, #trainingDataNumber times
      }
    vnl_matrix<FloatingPrecision> diffMat = tempDataMatrix - trainMatrix;
    vnl_vector<FloatingPrecision> distances( numTraining, 0 ); // distance vector for ith test data
      //Return square root of sum of squared absolute element values of each row as a distant element
    for( unsigned int iRow = 0; iRow < numTraining; ++iRow )
      {
      distances(iRow) = ( diffMat.get_row(iRow) ).two_norm();
      }
///////////////////
*/
    // sort distances
    vnl_vector<unsigned int> sortedIndexed( numTraining, 0 );

    typedef vnl_index_sort<FloatingPrecision, unsigned int>   IndexSortType;
    IndexSortType                                             indexSort;
    indexSort.vector_sort( distances, sortedIndexed );

      // First K indeces
    vnl_vector<unsigned int> neighborIndeces(K, 0);
    neighborIndeces = sortedIndexed.extract(K, 0); // indeces of first K neighbors

      // Now we should find K labels correspondence to K neighbors
    vnl_matrix<FloatingPrecision> neighborLabels( K, numClasses);
    for( unsigned int n = 0; n < K; ++n )
      {
      neighborLabels.set_row( n, localLabels.get_row( neighborIndeces(n) ) );
      }

    vnl_matrix<FloatingPrecision> weights(1,K,0);
    FloatingPrecision sumOfWeights = 0;
    for( unsigned int n = 0; n < K; ++n )
      {
      FloatingPrecision distSqr = pow( distances( neighborIndeces(n) ), 2);
      if( distSqr == 0 )
        {
        weights(0,n) = 1; // avoids inf weights
        }
      else
        {
        weights(0,n) = 1/distSqr;
        }
      sumOfWeights += weights(0,n);
      }
    weights = weights / sumOfWeights;

    vnl_matrix<FloatingPrecision> likelihoodRow = weights * neighborLabels; // a 1xC vector
    liklihoodMatrix.set_row(iTest, likelihoodRow.get_row(0) );
    } // end of main loop
  muLogMacro(<< "\n--------------------------------" << std::endl);
  muLogMacro(<< "LiklihoodMatrix is calculated: [ " << liklihoodMatrix.rows() << " x " << liklihoodMatrix.cols() << " ]" << std::endl);
  muLogMacro(<< "--------------------------------" << std::endl);
}

template <class TInputImage, class TProbabilityImage>
typename TProbabilityImage::Pointer
EMSegmentationFilter<TInputImage, TProbabilityImage>
::assignVectorToImage(const typename TProbabilityImage::Pointer prior,
                      const vnl_vector<FloatingPrecision> & vector)
{
  typename TProbabilityImage::Pointer post = TProbabilityImage::New();
  post->CopyInformation(prior);
  post->SetRegions(prior->GetLargestPossibleRegion() );
  post->Allocate();

  const typename TProbabilityImage::SizeType size = post->GetLargestPossibleRegion().GetSize();
  //muLogMacro(<< "DEBUG: size of posterior inside the assign vector: " << size << std::endl);

  unsigned int vindex = 0;
  for( LOOPITERTYPE kk = 0; kk < (LOOPITERTYPE)size[2]; kk++ )
    {
    for( LOOPITERTYPE jj = 0; jj < (LOOPITERTYPE)size[1]; jj++ )
      {
      for( LOOPITERTYPE ii = 0; ii < (LOOPITERTYPE)size[0]; ii++ )
        {
        const typename TProbabilityImage::IndexType currIndex = {{ii, jj, kk}};
        post->SetPixel(currIndex, vector(vindex) );
        ++vindex;
        }
      }
    }
  return post;
}

// Input: input image (InputImagePointer)
// Output: downsample input image (InputImagePointer)
template <class TInputImage, class TProbabilityImage>
const typename TInputImage::Pointer
EMSegmentationFilter<TInputImage, TProbabilityImage>
::DownSampleInputIntensityImages(const TInputImage * inputIntesityImage,
                                 const double resamplingFactor)
{
  // Set new size and spacing
  const typename InputImageType::SizeType inputSize = inputIntesityImage->GetLargestPossibleRegion().GetSize();
  typename InputImageType::SizeType outputSize;
  outputSize[0] = inputSize[0]/ resamplingFactor;
  outputSize[1] = inputSize[1]/ resamplingFactor;
  outputSize[2] = inputSize[2]/ resamplingFactor;

  typename InputImageType::SpacingType outputSpacing;
  outputSpacing[0] = inputIntesityImage->GetSpacing()[0] * resamplingFactor;
  outputSpacing[1] = inputIntesityImage->GetSpacing()[1] * resamplingFactor;
  outputSpacing[2] = inputIntesityImage->GetSpacing()[2] * resamplingFactor;

  // resampling transform is identity
  typedef itk::IdentityTransform<double, 3> IdentityTransformType;

  // Resampling filter
  typedef itk::ResampleImageFilter<TInputImage, TInputImage> ResampleImageFilterType;
  typename ResampleImageFilterType::Pointer resampler = ResampleImageFilterType::New();
  resampler->SetInput( inputIntesityImage );
  resampler->SetOutputOrigin ( inputIntesityImage->GetOrigin() );
  resampler->SetSize( outputSize );
  resampler->SetOutputSpacing( outputSpacing );
  resampler->SetOutputDirection( inputIntesityImage->GetDirection() );
  resampler->SetTransform( IdentityTransformType::New() );
  resampler->UpdateLargestPossibleRegion();

  // Smoothing filter
  typedef DiscreteGaussianImageFilter<TInputImage, TInputImage> SmoothingFilterType;
  typename SmoothingFilterType::Pointer smoothingFilter = SmoothingFilterType::New();
  smoothingFilter->SetUseImageSpacingOn();
  smoothingFilter->SetVariance( vnl_math_sqr( resamplingFactor - 1 ) );
  smoothingFilter->SetMaximumError( 0.01 );
  smoothingFilter->SetInput( resampler->GetOutput() );

  const InputImagePointer downsampledInputImage = smoothingFilter->GetOutput();
  return downsampledInputImage;
}

// Input: probability image (TProbabilityImage::Pointer)
// Output: Upsampled probability image (TProbabilityImage::Pointer)
template <class TInputImage, class TProbabilityImage>
typename TProbabilityImage::Pointer
EMSegmentationFilter<TInputImage, TProbabilityImage>
::UpSamplePosteriorImages( const TProbabilityImage * downSampledPosteriors,
                           const TProbabilityImage * refPrior)
{
  typedef itk::IdentityTransform<double, 3> IdentityTransformType;
  typedef itk::ResampleImageFilter<TProbabilityImage, TProbabilityImage> ResampleImageFilterType;

  typename ResampleImageFilterType::Pointer resampler = ResampleImageFilterType::New();
  resampler->SetInput( downSampledPosteriors );

  resampler->SetOutputOrigin ( refPrior->GetOrigin() );
  resampler->SetSize( refPrior->GetLargestPossibleRegion().GetSize() );
  resampler->SetOutputSpacing( refPrior->GetSpacing() );
  resampler->SetOutputDirection( refPrior->GetDirection() );
  //resampler->SetReferenceImage( refPrior );

  resampler->SetTransform( IdentityTransformType::New() );
  resampler->UpdateLargestPossibleRegion();

  const typename TProbabilityImage::Pointer UpsampledPosteriors = resampler->GetOutput();
  return UpsampledPosteriors;
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::ProbabilityImageVectorType
EMSegmentationFilter<TInputImage, TProbabilityImage>
::ComputekNNPosteriors(const ProbabilityImageVectorType & Priors,
                       const MapOfInputImageVectors & intensityImages, // input corrected images
                       ByteImagePointer & CleanedLabels,
                       const unsigned int numberOfSamples,
                       const double DownSamplingFactor) // new input

{
    // Phase 1: create train vector, label vector, input test vector
    // Phase 2: pass the above vectors to the "dokNNClassification" function

    // we have 15 classes. We pick 32000 samples from all classes.
  const unsigned int numClasses = Priors.size();

    // change the map of input image vectors to a probability image vector type
  typedef std::vector<InputImagePointer>       InputImageVectorType;
  InputImageVectorType                         inputImagesVector;

  typename MapOfInputImageVectors::const_iterator mapIt = intensityImages.begin();
  const typename InputImageType::SizeType sizeBeforeDwSmpl = mapIt->second[0]->GetLargestPossibleRegion().GetSize();
  muLogMacro(<< "Size of test image before downsampling: ( " << sizeBeforeDwSmpl << " )" << std::endl);

  for(mapIt = intensityImages.begin(); mapIt != intensityImages.end(); ++mapIt)
    {
    const double numCurModality = static_cast<double>(mapIt->second.size());
    for(unsigned m = 0; m < numCurModality; ++m)
      {
      // Test cases are downsampled input image intensities e.g. downsampled T1, T2.
      inputImagesVector.push_back( mapIt->second[m] );
      }
    }
  const unsigned int numOfInputImages = inputImagesVector.size();
  muLogMacro(<< "Number of input images: " << numOfInputImages << std::endl);

    // randomly iterate through the label image
  //const unsigned int numberOfSamples = 32000; // why?

    // set train matrix and label vector by picking samples from label image.
    // set kNN train matrix. it has #numberOfSamples training cases with #numOfInputImages features
  vnl_matrix<FloatingPrecision> trainMatrix(numberOfSamples, numOfInputImages);
  vnl_vector<FloatingPrecision> labelVector(numberOfSamples);

  muLogMacro(<< "\n* Computing the label vector with " << numberOfSamples << " samples..." << std::endl);
  muLogMacro(<< "* Computing train matrix ( " << numberOfSamples << " x " << numOfInputImages << " )" << std::endl);

  itk::ImageRandomNonRepeatingConstIteratorWithIndex<ByteImageType> NRit( CleanedLabels,
                                                                          CleanedLabels->GetBufferedRegion() );
  NRit.SetNumberOfSamples( CleanedLabels->GetBufferedRegion().GetNumberOfPixels() );
  NRit.GoToBegin();

  unsigned int maxAirSamples = numberOfSamples * 0.05;
  unsigned int numAirSamples = 0;
  unsigned int rowIndx = 0;
  while( ( !NRit.IsAtEnd() ) && ( rowIndx < numberOfSamples ) )
    {
    ByteImageType::IndexType currentIndex = NRit.GetIndex();
    unsigned int currLabelCode = NRit.Get();
    if( currLabelCode != 99 ) // 99 is the label code of the voxels that their value is less than threshold
                              // so they are not used in our computations.
      {
      if( (currLabelCode != 0) || ((currLabelCode == 0) && (numAirSamples < maxAirSamples)) )
        {
        // find the index of current label code (can we do that in a more efficient way?)
        unsigned int currLabelIndex = 1000;
        for( unsigned int iclass = 0; iclass < numClasses; iclass++ )
          {
          if( currLabelCode == this->m_PriorLabelCodeVector(iclass) )
            {
            currLabelIndex = iclass;
            break;
            }
          }
        if( currLabelIndex == 1000 ) // if class index of the current label has not changed
          {
          itkGenericExceptionMacro( << "Class index of the current label is not found!" << std::endl );
          }
        labelVector(rowIndx) = currLabelIndex;

        typename InputImageVectorType::const_iterator inIt = inputImagesVector.begin();
        unsigned int colIndx = 0;
        while( ( inIt != inputImagesVector.end() ) && ( colIndx < numOfInputImages ) )
          {
          trainMatrix(rowIndx,colIndx) = inIt->GetPointer()->GetPixel( currentIndex );
          ++colIndx;
          ++inIt;
          }
        ++rowIndx;
        if( currLabelCode == 0 )
          {
          ++numAirSamples;
          }
        }
      }
    ++NRit;
    }

  muLogMacro(<< "\n****** Row index: " << rowIndx << "instead of " << numberOfSamples << std::endl);
  if( rowIndx < numberOfSamples )
    {
    muLogMacro(<< "WARNING: Number of valid samples greater than threshold are\n "
               << rowIndx << ", that is less than total number of chosen samples: "
               << numberOfSamples << ".\nTrain matrix and Lable vector are resized." << std::endl);

    trainMatrix = trainMatrix.extract(rowIndx,numOfInputImages,0,0);
    labelVector = labelVector.extract(rowIndx,0);

    muLogMacro(<< "\n*** New size of Label vector and train matrix: " << std::endl);
    muLogMacro(<< "* Label vector < " << labelVector.size() << " >" << std::endl);
    muLogMacro(<< "* Train matrix ( " << trainMatrix.rows() << " x " << trainMatrix.cols() << " )" << std::endl);
    }

  //  Downsample input images for speed up posterior computations
  //  We do downsampling here because input images are needed for computing train matrix
  muLogMacro(<< "\nDownsampling the input images..." << std::endl);
  InputImageVectorType       DownSampledInputImagesVector;

  typename InputImageVectorType::const_iterator dsIt = inputImagesVector.begin();
  while ( dsIt != inputImagesVector.end() )
    {
      DownSampledInputImagesVector.push_back( this->DownSampleInputIntensityImages(dsIt->GetPointer(), DownSamplingFactor) );
      ++dsIt;
    }

  muLogMacro(<< "Downsampled test image size:" << DownSampledInputImagesVector[0]->GetLargestPossibleRegion().GetSize() << std::endl);

    // set kNN input test matrix of size : #OfVoxels x #OfInputImages
  const typename InputImageType::SizeType size = DownSampledInputImagesVector[0]->GetLargestPossibleRegion().GetSize();
  unsigned int numOfVoxels = DownSampledInputImagesVector[0]->GetLargestPossibleRegion().GetNumberOfPixels();

  muLogMacro(<< "\n* Computing test matrix ( " << numOfVoxels << " x " << numOfInputImages << " )" << std::endl);
  vnl_matrix<FloatingPrecision> testMatrix(numOfVoxels, numOfInputImages);

  unsigned int rowIndex = 0;
  for( LOOPITERTYPE kk = 0; kk < (LOOPITERTYPE)size[2]; kk++ )
    {
    for( LOOPITERTYPE jj = 0; jj < (LOOPITERTYPE)size[1]; jj++ )
      {
      for( LOOPITERTYPE ii = 0; ii < (LOOPITERTYPE)size[0]; ii++ )
        {
        const typename InputImageType::IndexType currIndex = {{ii, jj, kk}};
        unsigned int colIndex = 0;
        typename ProbabilityImageVectorType::const_iterator inIt = DownSampledInputImagesVector.begin();
        while( ( inIt != DownSampledInputImagesVector.end() ) && ( colIndex < numOfInputImages ) )
          {
          testMatrix(rowIndex,colIndex) = inIt->GetPointer()->GetPixel( currIndex );
          ++colIndex;
          ++inIt;
          }
        ++rowIndex;
        }
      }
    }

  const unsigned int K = 45;
    // each column of the memberShip matrix contains the voxel values of a posterior image.
  vnl_matrix<FloatingPrecision> liklihoodMatrix(numOfVoxels, numClasses, 1000);

  muLogMacro(<< "\n* Computing Liklihood Matrix ( " << numOfVoxels << " x " << numClasses << " )" << std::endl);
  muLogMacro(<< "Run k-NN algorithm on test data..." << std::endl);

  this->kNNCore( trainMatrix, labelVector, testMatrix, liklihoodMatrix, K );

    // For validation
  if( liklihoodMatrix.max_value() == 1000 )
    {
    itkGenericExceptionMacro( << "The liklihood matrix is not valid." << std::endl );
    }

    // create posteriors
  muLogMacro(<< "Create posteriors from likelihood matrix..." << std::endl);
  ProbabilityImageVectorType downSampledPosteriors;
  downSampledPosteriors.resize(numClasses);
  for( unsigned int iclass = 0; iclass < numClasses; iclass++ )
    {
    downSampledPosteriors[iclass] = this->assignVectorToImage(
                                                    this->DownSampleInputIntensityImages(Priors[iclass],DownSamplingFactor),
                                                    liklihoodMatrix.get_column(iclass) );
    }
  /*
  typedef itk::ImageFileWriter<TProbabilityImage> PostImageWriterType;
  typename PostImageWriterType::Pointer dwposetriorwriter = PostImageWriterType::New();
  dwposetriorwriter->SetInput(downSampledPosteriors[14]);
  dwposetriorwriter->SetFileName("DEBUG_WHITE_MATTER_DWSmpled.nii.gz");
  dwposetriorwriter->Update();
   */
  muLogMacro(<< "Upsampling posterior to the original size..." << std::endl);
  ProbabilityImageVectorType Posteriors;
  Posteriors.resize(numClasses);
  for( unsigned int iclass = 0; iclass < numClasses; iclass++ )
    {
    Posteriors[iclass] = this->UpSamplePosteriorImages( downSampledPosteriors[iclass], Priors[iclass] );
    }

  const typename InputImageType::SizeType finalPosteriorSize = Posteriors[0]->GetLargestPossibleRegion().GetSize();
  muLogMacro(<< "Size of return posteriors: " << finalPosteriorSize  << std::endl);
  /*
  typename PostImageWriterType::Pointer posetriorwriter = PostImageWriterType::New();
  posetriorwriter->SetInput(Posteriors[14]);
  posetriorwriter->SetFileName("DEBUG_WHITE_MATTER.nii.gz");
  posetriorwriter->Update();
  */
  return Posteriors;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <class TInputImage, class TProbabilityImage>
typename TProbabilityImage::Pointer
EMSegmentationFilter<TInputImage, TProbabilityImage>
::ComputeOnePosterior(const FloatingPrecision priorScale,
  const typename TProbabilityImage::Pointer prior,
  const vnl_matrix<FloatingPrecision> currCovariance,
  typename RegionStats::MeanMapType &currMeans,
  const MapOfInputImageVectors & intensityImages)
{
  typedef vnl_matrix<FloatingPrecision>         MatrixType;
  typedef vnl_matrix_inverse<FloatingPrecision> MatrixInverseType;

  // FOR IPEK & GARY -- this is a stopgap -- even though we use a map
  // of image lists instead of an image list, we're still computing
  // ImageList.size() * ImageList.size() covariance.
  unsigned          numModalities = currMeans.size();

  const FloatingPrecision detcov = ComputeCovarianceDeterminant(currCovariance);

  // Normalizing constant for the Gaussian
  const FloatingPrecision denom =
    vcl_pow(2 * vnl_math::pi, numModalities / 2.0) * vcl_sqrt(detcov) + vnl_math::eps;
  const FloatingPrecision invdenom = 1.0 / denom;
  CHECK_NAN(invdenom, __FILE__, __LINE__, "\n  denom:" << denom );
  const MatrixType invcov = MatrixInverseType(currCovariance);

  typename TProbabilityImage::Pointer post = TProbabilityImage::New();
  post->CopyInformation(prior);
  post->SetRegions(prior->GetLargestPossibleRegion() );
  post->Allocate();

  const typename TProbabilityImage::SizeType size = post->GetLargestPossibleRegion().GetSize();

#if defined(LOCAL_USE_OPEN_MP)
#pragma omp parallel for
#endif
  for( LOOPITERTYPE kk = 0; kk < (LOOPITERTYPE)size[2]; kk++ )
    {
    for( LOOPITERTYPE jj = 0; jj < (LOOPITERTYPE)size[1]; jj++ )
      {
      for( LOOPITERTYPE ii = 0; ii < (LOOPITERTYPE)size[0]; ii++ )
        {
        const typename TProbabilityImage::IndexType currIndex = {{ii, jj, kk}};
        // At a minimum, every class has at least a 0.001% chance of being
        // true no matter what.
        // I realize that this small value makes the priors equal slightly
        // larger than 100%, but everything
        // is renormalized anyway, so it is not really that big of a deal as
        // long as the main priors for
        // the desired class is significantly higher than 1%.
        const typename TProbabilityImage::PixelType minPriorValue = 0.0;
        const typename TProbabilityImage::PixelType priorValue = (prior->GetPixel(currIndex) + minPriorValue);
        // MatrixType X(numModalities, 1);
        // {
        // for(typename RegionStats::MeanMapType::const_iterator mapIt = currMeans.begin();
        //     mapIt != currMeans.end(); ++mapIt)
        //   {
        //   for(typename RegionStats::VectorType::const_iterator vecIt = mapIt->second.begin();
        //       vecIt != mapIt->second.end(); ++vecIt, ++ichan)
        //     {
        //     X(ichan, 0) =
        //       tmpIntensityImages[ichan]->GetPixel(currIndex) - (*vecIt);
        //     }
        //   }
        // }

        MatrixType X(numModalities, 1);
        unsigned long zz = 0;
        for(typename MapOfInputImageVectors::const_iterator mapIt = intensityImages.begin();
            mapIt != intensityImages.end(); ++mapIt, ++zz)
          {
          double curAvg(0.0);
          const double curMean = currMeans[mapIt->first];
          const double numCurModality = static_cast<double>(mapIt->second.size());
          for(unsigned xx = 0; xx < numCurModality; ++xx)
            {
            curAvg += (mapIt->second[xx]->GetPixel(currIndex) - curMean);
            }
          X(zz,0) = curAvg / numCurModality;
          }

        const MatrixType  Y = invcov * X;
        FloatingPrecision mahalo = 0.0;
        for(unsigned int ichan = 0; ichan < numModalities; ichan++ )
          {
          const FloatingPrecision & currVal = X(ichan, 0) * Y(ichan, 0);
          CHECK_NAN(currVal, __FILE__, __LINE__, "\n  currIndex: " << currIndex
                    << "\n  mahalo: " << mahalo
                    << "\n  ichan: " << ichan
                    << "\n  invcov: " << invcov
                    << "\n  X:  " << X
                    << "\n  Y:  " << Y );
          mahalo += currVal;
          }

        // Note:  This is the maximum likelyhood estimate as described in
        // formula at bottom of
        //       http://en.wikipedia.org/wiki/Maximum_likelihood_estimation
        const FloatingPrecision likelihood = vcl_exp(-0.5 * mahalo) * invdenom;

        const typename TProbabilityImage::PixelType currentPosterior =
          static_cast<typename TProbabilityImage::PixelType>( (priorScale * priorValue * likelihood) );
        CHECK_NAN(currentPosterior, __FILE__, __LINE__, "\n  currIndex: " << currIndex
                  << "\n  priorScale: " << priorScale << "\n  priorValue: " << priorValue
                  << "\n  likelihood: " << likelihood
                  << "\n  mahalo: " << mahalo
                  << "\n  invcov: " << invcov
                  << "\n  X:  " << X
                  << "\n  Y:  " << Y );
        post->SetPixel(currIndex, currentPosterior);
        }
      }
    }
  return post;
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::ProbabilityImageVectorType
EMSegmentationFilter<TInputImage, TProbabilityImage>
::ComputePosteriors(const ProbabilityImageVectorType & Priors,
                    const vnl_vector<FloatingPrecision> & PriorWeights,
                    const MapOfInputImageVectors & IntensityImages,
                    std::vector<RegionStats> & ListOfClassStatistics)
{
  // Compute initial distribution parameters
  muLogMacro(<< "ComputePosteriors" << std::endl );
  itk::TimeProbe ComputePosteriorsTimer;
  ComputePosteriorsTimer.Start();

  const unsigned int numClasses = Priors.size();
  muLogMacro(<< "Computing posteriors at full resolution" << std::endl);

  ProbabilityImageVectorType Posteriors;
  Posteriors.resize(numClasses);
  for( unsigned int iclass = 0; iclass < numClasses; iclass++ )
    {
    const FloatingPrecision priorScale = PriorWeights[iclass];
    CHECK_NAN(priorScale, __FILE__, __LINE__, "\n  iclass: " << iclass );

    Posteriors[iclass] = ComputeOnePosterior(priorScale,
                                             Priors[iclass],
                                             ListOfClassStatistics[iclass].m_Covariance,
                                             ListOfClassStatistics[iclass].m_Means,
                                             IntensityImages);
    } // end class loop

  ComputePosteriorsTimer.Stop();
  itk::RealTimeClock::TimeStampType elapsedTime =
    ComputePosteriorsTimer.GetTotal();
  muLogMacro(<< "Computing Posteriors took " << elapsedTime << " " << ComputePosteriorsTimer.GetUnit() << std::endl);
  return Posteriors;
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::WriteDebugLabels(const unsigned int CurrentEMIteration) const
{
  if( this->m_DebugLevel > 6 )
    {
    // write out labels
    std::stringstream CurrentEMIteration_stream("");
    CurrentEMIteration_stream << CurrentEMIteration;
    {
    typedef itk::ImageFileWriter<ByteImageType> LabelImageWriterType;
    typename LabelImageWriterType::Pointer writer = LabelImageWriterType::New();

    const std::string fn = this->m_OutputDebugDir + "/LABELS_LEVEL_" + CurrentEMIteration_stream.str() + ".nii.gz";

    muLogMacro(<< "Writing label images... " << fn <<  std::endl);
    writer->SetInput(m_CleanedLabels);
    writer->SetFileName(fn);
    writer->UseCompressionOn();
    writer->Update();
    }
    }
  if( this->m_DebugLevel > 6 )
    {
    // write out labels
    std::stringstream CurrentEMIteration_stream("");
    CurrentEMIteration_stream << CurrentEMIteration;
    {
    typedef itk::ImageFileWriter<ByteImageType> LabelImageWriterType;
    typename LabelImageWriterType::Pointer writer = LabelImageWriterType::New();

    const std::string fn = this->m_OutputDebugDir + "/LABELSDIRTY_LEVEL_" + CurrentEMIteration_stream.str()
      + ".nii.gz";

    muLogMacro(<< "Writing label images... " << fn <<  std::endl);
    writer->SetInput(m_DirtyLabels);
    writer->SetFileName(fn);
    writer->UseCompressionOn();
    writer->Update();
    }
    }
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::WriteDebugCorrectedImages(const MapOfInputImageVectors &correctImageList,
                            const unsigned int CurrentEMIteration ) const
{
  if(this->m_DebugLevel <= 8)
    {
    return;
    }
  std::stringstream CurrentEMIteration_stream("");
  CurrentEMIteration_stream << CurrentEMIteration;
  for(typename MapOfInputImageVectors::const_iterator mapIt = correctImageList.begin();
      mapIt != correctImageList.end(); ++mapIt)
    {
    for(typename InputImageVector::const_iterator imIt = mapIt->second.begin();
        imIt != mapIt->second.end(); ++imIt)
      {
      typedef itk::ImageFileWriter<InputImageType> WriterType;
      typename WriterType::Pointer writer = WriterType::New();
      writer->UseCompressionOn();
      std::stringstream template_index_stream("");
      template_index_stream << std::distance(mapIt->second.begin(),imIt);
      const std::string fn = this->m_OutputDebugDir
        + "/CORRECTED_INDEX_"
        + mapIt->first
        + template_index_stream.str()
        + "_LEVEL_"
        + CurrentEMIteration_stream.str()
        + ".nii.gz";
      writer->SetInput((*imIt));
      writer->SetFileName(fn.c_str() );
      writer->Update();
      muLogMacro( << "DEBUG:  Wrote image " << fn <<  std::endl);
      }
    }
}

template <class TInputImage, class TProbabilityImage>
FloatingPrecision
EMSegmentationFilter<TInputImage, TProbabilityImage>
::ComputeLogLikelihood() const
{
  const InputImageSizeType size = m_Posteriors[0]->GetLargestPossibleRegion().GetSize();
  const unsigned int       computeInitialNumClasses = m_Posteriors.size();
  FloatingPrecision        logLikelihood = 0.0;

  {
#if defined(LOCAL_USE_OPEN_MP)
#pragma omp parallel for reduction(+:logLikelihood)
#endif
  for( LOOPITERTYPE kk = 0; kk < (LOOPITERTYPE)size[2]; kk++ )
    {
    for( LOOPITERTYPE jj = 0; jj < (LOOPITERTYPE)size[1]; jj++ )
      {
      for( LOOPITERTYPE ii = 0; ii < (LOOPITERTYPE)size[0]; ii++ )
        {
        const ProbabilityImageIndexType currIndex = {{ii, jj, kk}};
        FloatingPrecision               tmp = 1e-20;
        for( unsigned int iclass = 0; iclass < computeInitialNumClasses; iclass++ )
          {
          if( this->m_PriorIsForegroundPriorVector[iclass] ) // We should
            // probably only
            // compute the
            // foreground.
            {
            tmp += m_Posteriors[iclass]->GetPixel(currIndex);
            }
          }
        logLikelihood = logLikelihood + vcl_log(tmp);
        }
      }
    }
  }

  return logLikelihood;
}

/**
 * \param referenceImage is the image to be used for defining the tissueRegion of iterest.
 * \param safetyRegion is the amount to dilate so that there is not such a tight region.
 */
template <class TInputImage, class TByteImage>
typename TByteImage::Pointer
ComputeTissueRegion(const typename TInputImage::Pointer referenceImage, const unsigned int safetyRegion)
{
  typedef itk::BRAINSROIAutoImageFilter<TInputImage, TByteImage> ROIAutoType;
  typename ROIAutoType::Pointer  ROIFilter = ROIAutoType::New();
  ROIFilter->SetInput(referenceImage);
  ROIFilter->SetDilateSize(safetyRegion); // Create a very tight fitting tissue
                                          // region here.
  ROIFilter->Update();
  typename TByteImage::Pointer tissueRegion = ROIFilter->GetOutput();
  return tissueRegion;
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::WriteDebugHeadRegion(const unsigned int CurrentEMIteration) const
{
  if( this->m_DebugLevel > 7 )
    {
    std::stringstream CurrentEMIteration_stream("");
    CurrentEMIteration_stream << CurrentEMIteration;
    { // DEBUG:  This code is for debugging purposes only;
    typedef itk::ImageFileWriter<ByteImageType> WriterType;
    typename WriterType::Pointer writer = WriterType::New();
    writer->UseCompressionOn();

    const std::string fn = this->m_OutputDebugDir + "/HEAD_REGION_LEVEL_" + CurrentEMIteration_stream.str()
      + ".nii.gz";
    writer->SetInput( this->m_NonAirRegion );
    writer->SetFileName(fn.c_str() );
    writer->Update();
    muLogMacro( << "DEBUG:  Wrote image " << fn <<  std::endl);
    }
    }
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::ProbabilityImageVectorType
EMSegmentationFilter<TInputImage, TProbabilityImage>
::WarpImageList(ProbabilityImageVectorType &originalList,
                const InputImagePointer  referenceOutput,
                const BackgroundValueVector & backgroundValues,
                const GenericTransformType::Pointer warpTransform)
{
  if( originalList.size() != backgroundValues.size() )
    {
    itkGenericExceptionMacro(<< "ERROR:  originalList and backgroundValues arrays sizes do not match" << std::endl);
    }
  std::vector<typename TInputImage::Pointer> warpedList(originalList.size() );

  typedef itk::ResampleImageFilter<TInputImage, TInputImage> ResamplerType;
  for( unsigned int vIndex = 0; vIndex < originalList.size(); vIndex++ )
    {
    typename ResamplerType::Pointer warper = ResamplerType::New();
    warper->SetInput(originalList[vIndex]);
    warper->SetTransform(warpTransform);

    // warper->SetInterpolator(linearInt); // Default is linear
    warper->SetOutputParametersFromImage(referenceOutput);
    warper->SetDefaultPixelValue(backgroundValues[vIndex]);
    warper->Update();
    warpedList[vIndex] = warper->GetOutput();
    }
  return warpedList;
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::MapOfInputImageVectors
EMSegmentationFilter<TInputImage, TProbabilityImage>
::WarpImageList(MapOfInputImageVectors &originalList,
                const InputImagePointer referenceOutput,
                const GenericTransformType::Pointer warpTransform)
{
  typedef itk::ResampleImageFilter<TInputImage, TInputImage> ResamplerType;

  MapOfInputImageVectors warpedList;

  for(typename MapOfInputImageVectors::iterator mapIt = originalList.begin();
      mapIt != originalList.end(); ++mapIt)
    {
    for(typename InputImageVector::iterator imIt = mapIt->second.begin();
        imIt != mapIt->second.end(); ++imIt)
      {
      typename ResamplerType::Pointer warper = ResamplerType::New();
      warper->SetInput(*imIt);
      warper->SetTransform(warpTransform);

      // warper->SetInterpolator(linearInt); // Default is linear
      warper->SetOutputParametersFromImage(referenceOutput);
      warper->SetDefaultPixelValue(0);
      warper->Update();
      warpedList[mapIt->first].push_back(warper->GetOutput());
      }
    }
  return warpedList;
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::WriteDebugWarpedAtlasImages(const unsigned int CurrentEMIteration) const
{
  std::stringstream CurrentEMIteration_stream("");

  CurrentEMIteration_stream << CurrentEMIteration;
  if( this->m_DebugLevel > 9 )
    {
    for(typename MapOfInputImageVectors::const_iterator mapIt =
          this->m_WarpedAtlasImages.begin();
        mapIt != this->m_WarpedAtlasImages.end(); ++mapIt)
      {
      for(typename InputImageVector::const_iterator imIt = mapIt->second.begin();
          imIt != mapIt->second.end(); ++imIt)
        {
        typedef itk::ImageFileWriter<InputImageType> WriterType;
        typename WriterType::Pointer writer = WriterType::New();
        writer->UseCompressionOn();

        std::stringstream template_index_stream("");
        template_index_stream << mapIt->first
                              << std::distance(mapIt->second.begin(),imIt);
        const std::string fn = this->m_OutputDebugDir + "/WARPED_ATLAS_INDEX_" + template_index_stream.str()
          + "_LEVEL_" + CurrentEMIteration_stream.str() + ".nii.gz";
        writer->SetInput((*imIt));
        writer->SetFileName(fn.c_str() );
        writer->Update();
        muLogMacro( << "DEBUG:  Wrote image " << fn <<  std::endl);
        }
      }
    }
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::MapOfInputImageVectors
EMSegmentationFilter<TInputImage, TProbabilityImage>
::GenerateWarpedAtlasImages(void)
{
  this->m_WarpedAtlasImages =
    this->WarpImageList(this->m_OriginalAtlasImages,
                        GetMapVectorFirstElement(this->m_InputImages),
                        this->m_TemplateGenericTransform);
  return m_WarpedAtlasImages;
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::ByteImageVectorType
EMSegmentationFilter<TInputImage, TProbabilityImage>
::UpdateIntensityBasedClippingOfPriors(const unsigned int CurrentEMIteration,
                                       const MapOfInputImageVectors &intensityList,
                                       const ProbabilityImageVectorType &WarpedPriorsList,
                                       typename ByteImageType::Pointer &ForegroundBrainRegion)
{
  // #################################################################
  // #################################################################
  // #################################################################
  // #################################################################
  // #################################################################
  // #################################################################
  // For each intensityList, get it's type, and then create an "anded mask of
  // candidate regions"
  // using the table from BRAINSMultiModeHistogramThresholder.
  std::vector<typename ByteImageType::Pointer> subjectCandidateRegions;

  subjectCandidateRegions.resize(WarpedPriorsList.size() );
  { // StartValid Regions Section
#if defined(LOCAL_USE_OPEN_MP)
#pragma omp parallel for default(shared)
#endif
  for( LOOPITERTYPE i = 0; i < (LOOPITERTYPE)WarpedPriorsList.size(); i++ )
    {
    typename ByteImageType::Pointer probThreshImage = NULL;

    typedef itk::BinaryThresholdImageFilter<TProbabilityImage, ByteImageType> ProbThresholdType;
    typename ProbThresholdType::Pointer probThresh = ProbThresholdType::New();
    probThresh->SetInput(  WarpedPriorsList[i] );
    probThresh->SetInsideValue(1);
    probThresh->SetOutsideValue(0);
    probThresh->SetLowerThreshold(0.05);  // Hueristic: Need greater than 1 in 20
    // chance of being this structure
    // from the spatial probabilities
    probThresh->SetUpperThreshold( vcl_numeric_limits<typename TProbabilityImage::PixelType>::max() );
    // No upper limit needed, values
    // should be between 0 and 1
    probThresh->Update();
    probThreshImage = probThresh->GetOutput();
    if( this->m_DebugLevel > 9 )
      {
      std::stringstream CurrentEMIteration_stream("");
      CurrentEMIteration_stream << CurrentEMIteration;
      // Write the subject candidate regions

      std::ostringstream oss;
      oss << this->m_OutputDebugDir << "CANDIDIDATE_PROBTHRESH_" << this->m_PriorNames[i] << "_LEVEL_"
          << CurrentEMIteration_stream.str() << ".nii.gz" << std::ends;
      std::string fn = oss.str();
      muLogMacro( << "Writing Subject Candidate Region." << fn << std::endl );
      muLogMacro( << std::endl );

      typedef itk::ImageFileWriter<ByteImageType> ByteWriterType;
      typename ByteWriterType::Pointer writer = ByteWriterType::New();
      writer->SetInput(probThreshImage);
      writer->SetFileName(fn.c_str() );
      writer->UseCompressionOn();
      writer->Update();
      }


    const unsigned int numberOfModes = TotalMapSize(intensityList);
    typedef typename itk::MultiModeHistogramThresholdBinaryImageFilter<InputImageType,
      ByteImageType> ThresholdRegionFinderType;
    typename ThresholdRegionFinderType::ThresholdArrayType QuantileLowerThreshold(numberOfModes);
    typename ThresholdRegionFinderType::ThresholdArrayType QuantileUpperThreshold(numberOfModes);
    typename ThresholdRegionFinderType::Pointer thresholdRegionFinder = ThresholdRegionFinderType::New();
    // TODO:  Need to define PortionMaskImage from deformed probspace
    thresholdRegionFinder->SetBinaryPortionImage(ForegroundBrainRegion);
    unsigned int modeIndex = 0;
    for(typename MapOfInputImageVectors::const_iterator mapIt = intensityList.begin();
        mapIt != intensityList.end(); ++mapIt)
      {
      for(typename InputImageVector::const_iterator imIt = mapIt->second.begin();
          imIt != mapIt->second.end(); ++imIt, ++modeIndex)
        {
        thresholdRegionFinder->SetInput(modeIndex, (*imIt));
        const std::string imageType = mapIt->first;
        const std::string priorType = this->m_PriorNames[i];
        if( m_TissueTypeThresholdMapsRange[priorType].find(imageType) ==
            m_TissueTypeThresholdMapsRange[priorType].end() )
          {
          muLogMacro(<< "NOT FOUND:" << "[" << priorType << "," << imageType
                     << "]: [" << 0.00 << "," << 1.00 << "]"
                     <<  std::endl);
          QuantileLowerThreshold.SetElement(modeIndex, 0.00);
          QuantileUpperThreshold.SetElement(modeIndex, 1.00);
          }
        else
          {
          const float lower = m_TissueTypeThresholdMapsRange[priorType][imageType].GetLower();
          const float upper = m_TissueTypeThresholdMapsRange[priorType][imageType].GetUpper();
          muLogMacro( <<  "[" << priorType << "," << imageType << "]: [" << lower << "," << upper << "]" <<  std::endl);
          QuantileLowerThreshold.SetElement(modeIndex, lower);
          QuantileUpperThreshold.SetElement(modeIndex, upper);
          m_TissueTypeThresholdMapsRange[priorType][imageType].Print();
          }
        }
      }
    // Assume upto (2*0.025)% of intensities are noise that corrupts the image
    // min/max values
    thresholdRegionFinder->SetLinearQuantileThreshold(0.025);
    thresholdRegionFinder->SetQuantileLowerThreshold(QuantileLowerThreshold);
    thresholdRegionFinder->SetQuantileUpperThreshold(QuantileUpperThreshold);
    // thresholdRegionFinder->SetInsideValue(1);
    // thresholdRegionFinder->SetOutsideValue(0);//Greatly reduce the value to
    // zero.
    thresholdRegionFinder->Update();
    if( this->m_DebugLevel > 8 )
      {
      std::stringstream CurrentEMIteration_stream("");
      CurrentEMIteration_stream << CurrentEMIteration;
      // Write the subject candidate regions

      std::ostringstream oss;
      oss << this->m_OutputDebugDir << "CANDIDIDATE_INTENSITY_REGION_" << this->m_PriorNames[i] << "_LEVEL_"
          << CurrentEMIteration_stream.str() << ".nii.gz" << std::ends;
      std::string fn = oss.str();
      muLogMacro( << "Writing Subject Candidate Region." << fn << std::endl );
      muLogMacro( << std::endl );

      typedef itk::ImageFileWriter<ByteImageType> ByteWriterType;
      typename ByteWriterType::Pointer writer = ByteWriterType::New();
      writer->SetInput(thresholdRegionFinder->GetOutput() );
      writer->SetFileName(fn.c_str() );
      writer->UseCompressionOn();
      writer->Update();
      }

    // Now multiply the warped priors by the subject candidate regions.
    typename itk::MultiplyImageFilter<ByteImageType, ByteImageType, ByteImageType>::Pointer multFilter =
      itk::MultiplyImageFilter<ByteImageType, ByteImageType, ByteImageType>::New();
    multFilter->SetInput1( probThreshImage );
    multFilter->SetInput2( thresholdRegionFinder->GetOutput() );
    multFilter->Update();
    subjectCandidateRegions[i] = multFilter->GetOutput();
    } // End loop over all warped priors

  { // Ensure that every candidate region has some value
  const unsigned int candiateVectorSize = subjectCandidateRegions.size();
  itk::ImageRegionIteratorWithIndex<ByteImageType>
    firstCandidateIter(subjectCandidateRegions[0], subjectCandidateRegions[0]->GetLargestPossibleRegion() );
  size_t AllZeroCounts = 0;
  while( !firstCandidateIter.IsAtEnd() )
    {
    const typename ByteImageType::IndexType myIndex = firstCandidateIter.GetIndex();
    bool AllPixelsAreZero = true;
//        unsigned int maxPriorRegionIndex = 0;
    typename TProbabilityImage::PixelType maxProbValue = WarpedPriorsList[0]->GetPixel(myIndex);
    for( unsigned int k = 0; ( k < candiateVectorSize ) && AllPixelsAreZero; k++ )
      {
      const typename ByteImageType::PixelType value = subjectCandidateRegions[k]->GetPixel(myIndex);
      if( value > 0 )
        {
        AllPixelsAreZero = false;
        }
      typename TProbabilityImage::PixelType currValue = WarpedPriorsList[k]->GetPixel(myIndex);
      if( currValue > maxProbValue )
        {
        maxProbValue = currValue;
//            maxPriorRegionIndex = k;
        }
      }
    if( AllPixelsAreZero ) // If all candidate regions are zero, then force
      // to most likely background value.
      {
      AllZeroCounts++;
      for( unsigned int k = 0; k < candiateVectorSize; k++ )
        {
        if( this->m_PriorIsForegroundPriorVector[k] == false )
          {
          subjectCandidateRegions[k]->SetPixel(myIndex, 1);
          WarpedPriorsList[k]->SetPixel(myIndex, 0.05);
          }
        }
      }
    ++firstCandidateIter;
    }

  if( AllZeroCounts != 0 )
    {
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    std::cout << "Locations with no candidate regions specified!" << AllZeroCounts << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    std::cout << "^^^^^^^^^^^^^^^^" << std::endl;
    }
  }
  if( this->m_DebugLevel > 5 )
    {
    std::stringstream CurrentEMIteration_stream("");
    CurrentEMIteration_stream << CurrentEMIteration;
    for( unsigned int i = 0; i < subjectCandidateRegions.size(); i++ )
      {
      // Write the subject candidate regions

      std::ostringstream oss;
      oss << this->m_OutputDebugDir << "CANDIDIDATE_FINAL" << this->m_PriorNames[i] << "_LEVEL_"
          << CurrentEMIteration_stream.str() << ".nii.gz" << std::ends;
      std::string fn = oss.str();
      muLogMacro( << "Writing Subject Candidate Region." << fn << std::endl );
      muLogMacro( << std::endl );

      typedef itk::ImageFileWriter<ByteImageType> ByteWriterType;
      typename ByteWriterType::Pointer writer = ByteWriterType::New();
      writer->SetInput(subjectCandidateRegions[i]);
      writer->SetFileName(fn.c_str() );
      writer->UseCompressionOn();
      writer->Update();
      }
    }
  } // END Valid regions section

  return subjectCandidateRegions;
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::ByteImageVectorType
EMSegmentationFilter<TInputImage, TProbabilityImage>
::ForceToOne(const unsigned int /* CurrentEMIteration */,
             // const MapOfInputImageVectors &intensityList,
             ProbabilityImageVectorType &WarpedPriorsList,
             typename ByteImageType::Pointer /* NonAirRegion */)
{
  // #################################################################
  // #################################################################
  // #################################################################
  // #################################################################
  // #################################################################
  // #################################################################
  // For each intensityList, get it's type, and then create an "anded mask of
  // candidate regions"
  // using the table from BRAINSMultiModeHistogramThresholder.
  ByteImageVectorType subjectCandidateRegions;

  subjectCandidateRegions.resize(WarpedPriorsList.size() );
  { // StartValid Regions Section
#if defined(LOCAL_USE_OPEN_MP)
#pragma omp parallel for default(shared)
#endif
  for( LOOPITERTYPE i = 0; i < (LOOPITERTYPE)WarpedPriorsList.size(); i++ )
    {
    subjectCandidateRegions[i] = ByteImageType::New();
    subjectCandidateRegions[i]->CopyInformation(WarpedPriorsList[i].GetPointer() );
    subjectCandidateRegions[i]->SetRegions( WarpedPriorsList[i]->GetLargestPossibleRegion() );
    subjectCandidateRegions[i]->Allocate();
    subjectCandidateRegions[i]->FillBuffer(1);
    }
  } // END Valid regions section

  return subjectCandidateRegions;
}

// ReturnBlendedProbList can be the same as one of the inputs!
template <typename TInputImage, typename TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::BlendPosteriorsAndPriors(const double blendPosteriorPercentage,
                           const ProbabilityImageVectorType & ProbList1,
                           const ProbabilityImageVectorType & ProbList2,
                           ProbabilityImageVectorType & ReturnBlendedProbList)
{
  for( unsigned int k = 0; k < ProbList2.size(); k++ )
    {
    std::cout << "Start Blending Prior:" << k << std::endl;
    typename TProbabilityImage::Pointer multInputImage = ProbList2[k];
    // BLEND Posteriors and Priors Here:
    // It is important to keep the warped priors as at least a small component
    // of this part
    // of the algorithm, because otherwise single pixels that exactly match the
    // mean of the
    // NOT* regions will become part of those NOT* regions regardless of spatial
    // locations.
    // std::cout << "\n\nWarpedPriors[" << k << "] \n" << ProbList2[k] <<
    // std::endl;
    if(
      ( ProbList1.size() == ProbList2.size() )
      && ProbList1.size() > k
      && ProbList1[k].IsNotNull()
      && ( blendPosteriorPercentage > 0.01 ) // Need to blend at more than 1%,
                                             // else just skip it
      )
      {
      // Really we need to use a heirarchial approach to solving this problem.
      //  It is not sufficient to have these heuristics
      // break things apart artificially.
      std::cout << "\nBlending Priors with Posteriors with formula: " << (blendPosteriorPercentage)
                << "*Posterior + " << (1.0 - blendPosteriorPercentage) << "*Prior" << std::endl;
      typedef itk::BlendImageFilter<TProbabilityImage, TProbabilityImage> BlenderType;
      typename BlenderType::Pointer myBlender = BlenderType::New();
      myBlender->SetInput1(ProbList1[k]);
      myBlender->SetInput2(ProbList2[k]);
      myBlender->SetBlend1(blendPosteriorPercentage);
      myBlender->SetBlend2(1.0 - blendPosteriorPercentage);
      myBlender->Update();
      multInputImage = myBlender->GetOutput();
      }
    else
      {
      std::cout << "Not Blending Posteriors into Priors" << std::endl;
      multInputImage = ProbList2[k];
      }
#if 1
    ReturnBlendedProbList[k] = multInputImage;
#else
    // Now multiply the warped priors by the subject candidate regions.
    typename itk::MultiplyImageFilter<TProbabilityImage, ByteImageType, TProbabilityImage>::Pointer multFilter =
      itk::MultiplyImageFilter<TProbabilityImage, ByteImageType, TProbabilityImage>::New();
    multFilter->SetInput1(multInputImage);
    multFilter->SetInput2(candidateRegions[k]);
    multFilter->Update();
    ReturnBlendedProbList[k] = multFilter->GetOutput();
    std::cout << "Stop Blending Prior:" << k << std::endl;
#endif
    }
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::WriteDebugWarpedAtlasPriors(const unsigned int CurrentEMIteration) const
{
  std::stringstream CurrentEMIteration_stream("");

  CurrentEMIteration_stream << CurrentEMIteration;
  if( this->m_DebugLevel > 9 )
    {
    for( unsigned int vIndex = 0; vIndex < this->m_WarpedPriors.size(); vIndex++ )
      {
      typedef itk::ImageFileWriter<InputImageType> WriterType;
      typename WriterType::Pointer writer = WriterType::New();
      writer->UseCompressionOn();

      std::stringstream template_index_stream("");
      template_index_stream << this->m_PriorNames[vIndex];
      const std::string fn = this->m_OutputDebugDir + "/WARPED_PRIOR_" + template_index_stream.str() + "_LEVEL_"
        + CurrentEMIteration_stream.str() + ".nii.gz";
      writer->SetInput(m_WarpedPriors[vIndex]);
      writer->SetFileName(fn.c_str() );
      writer->Update();
      muLogMacro( << "DEBUG:  Wrote image " << fn <<  std::endl);
      }
    }
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::WriteDebugBlendClippedPriors( const unsigned int CurrentEMIteration) const
{
  std::stringstream CurrentEMIteration_stream("");

  CurrentEMIteration_stream << CurrentEMIteration;
  if( this->m_DebugLevel > 9 )
    { // DEBUG:  This code is for debugging purposes only;
    for( unsigned int k = 0; k < m_WarpedPriors.size(); k++ )
      {
      typedef itk::ImageFileWriter<InputImageType> WriterType;
      typename WriterType::Pointer writer = WriterType::New();
      writer->UseCompressionOn();

      std::stringstream prior_index_stream("");
      prior_index_stream << k;
      // const std::string fn = this->m_OutputDebugDir +
      //
      // "PRIOR_INDEX_"+prior_index_stream.str()+"_LEVEL_"+CurrentEMIteration_stream.str()+".nii.gz";
      const std::string fn = this->m_OutputDebugDir + "BLENDCLIPPED_PRIOR_INDEX_" + this->m_PriorNames[k] + "_LEVEL_"
        + CurrentEMIteration_stream.str() + ".nii.gz";
      writer->SetInput(m_WarpedPriors[k]);
      writer->SetFileName(fn.c_str() );
      writer->Update();
      muLogMacro( << "DEBUG:  Wrote image " << fn <<  std::endl);
      }
    }
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::UpdateTransformation(const unsigned int /*CurrentEMIteration*/)
{
  if( m_AtlasTransformType == "SyN" )
    {
    muLogMacro(<< "HACK: " << m_AtlasTransformType <<  " not instumented for transformation update."  << std::endl );
    return;
    }
  muLogMacro(<< "Updating Warping with transform type: " << m_AtlasTransformType  << std::endl );
  if( m_UpdateTransformation == false )
    {
    muLogMacro(
      << "WARNING: WARNING: WARNING: WARNING:  Doing warping even though it was turned off from the command line"
      << std::endl);
    }
  for(typename MapOfInputImageVectors::iterator imMapIt =
        this->m_CorrectedImages.begin();
      imMapIt != this->m_CorrectedImages.end(); ++imMapIt)
    {
    typename InputImageVector::iterator imIt = imMapIt->second.begin();
    typename InputImageVector::iterator atIt =
      this->m_OriginalAtlasImages[imMapIt->first].begin();
    for(; imIt != imMapIt->second.end()
          && atIt != this->m_OriginalAtlasImages[imMapIt->first].end();
        ++imIt, ++atIt)
      {
      typedef itk::BRAINSFitHelper HelperType;
      HelperType::Pointer atlasToSubjectRegistrationHelper = HelperType::New();
      atlasToSubjectRegistrationHelper->SetNumberOfSamples(500000);
      atlasToSubjectRegistrationHelper->SetNumberOfHistogramBins(50);
      std::vector<int> numberOfIterations(1);
      numberOfIterations[0] = 1500;
      atlasToSubjectRegistrationHelper->SetNumberOfIterations(numberOfIterations);
      //  atlasToSubjectRegistrationHelper->SetMaximumStepLength(maximumStepSize);
      atlasToSubjectRegistrationHelper->SetTranslationScale(1000);
      atlasToSubjectRegistrationHelper->SetReproportionScale(1.0);
      atlasToSubjectRegistrationHelper->SetSkewScale(1.0);

      // atlasToSubjectRegistrationHelper->SetMaskInferiorCutOffFromCenter(maskInferiorCutOffFromCenter);
      //  atlasToSubjectRegistrationHelper->SetUseWindowedSinc(useWindowedSinc);

      // Register each intrasubject image mode to first image
      atlasToSubjectRegistrationHelper->SetFixedVolume((*imIt));
      // Register all atlas images to first image
      if( false /* m_AtlasTransformType == ID_TRANSFORM */ )
        {
        muLogMacro(<< "Registering (Identity) atlas to first image." << std::endl);
        // TODO: m_AtlasToSubjectTransform = MakeRigidIdentity();
        }
      else // continue;
        {
        std::string preprocessMovingString("");
        // const bool histogramMatch=true;//Setting histogram matching to true

        // Setting histogram matching to false because it appears to have been
        // causing problems for some images.
        const bool histogramMatch = false;
        if( histogramMatch )
          {
          typedef itk::HistogramMatchingImageFilter<InputImageType,
            InputImageType> HistogramMatchingFilterType;
          typename HistogramMatchingFilterType::Pointer histogramfilter
            = HistogramMatchingFilterType::New();

          histogramfilter->SetInput( (*atIt) );
          histogramfilter->SetReferenceImage( (*imIt) );

          histogramfilter->SetNumberOfHistogramLevels( 50 );
          histogramfilter->SetNumberOfMatchPoints( 10 );
          histogramfilter->ThresholdAtMeanIntensityOn();
          histogramfilter->Update();
          typename InputImageType::Pointer equalizedMovingImage = histogramfilter->GetOutput();
          atlasToSubjectRegistrationHelper->SetMovingVolume(equalizedMovingImage);
          preprocessMovingString = "histogram equalized ";
          }
        else
          {
          atlasToSubjectRegistrationHelper->SetMovingVolume((*atIt));
          preprocessMovingString = "";
          }
        typedef itk::BRAINSROIAutoImageFilter<InputImageType, itk::Image<unsigned char, 3> > ROIAutoType;
        typename ROIAutoType::Pointer  ROIFilter = ROIAutoType::New();
        ROIFilter->SetInput((*atIt));
        ROIFilter->SetDilateSize(10);
        ROIFilter->Update();
        atlasToSubjectRegistrationHelper->SetMovingBinaryVolume(ROIFilter->GetSpatialObjectROI() );

        typedef itk::BRAINSROIAutoImageFilter<InputImageType, itk::Image<unsigned char, 3> > ROIAutoType;
        ROIFilter = ROIAutoType::New();
        ROIFilter->SetInput((*imIt));
        ROIFilter->SetDilateSize(10);
        ROIFilter->Update();
        atlasToSubjectRegistrationHelper->SetFixedBinaryVolume(ROIFilter->GetSpatialObjectROI() );

        if( m_AtlasTransformType == "Rigid" )
          {
          muLogMacro(<< "Registering (Rigid) " << preprocessMovingString << "atlas("
                     << imMapIt->first << std::distance(imMapIt->second.begin(),imIt)
                     << ") to template("
                     << imMapIt->first << std::distance(imMapIt->second.begin(),imIt)
                     << ") image." << std::endl);
          std::vector<double> minimumStepSize(1);
          minimumStepSize[0] = 0.005; // NOTE: 0.005 for between subject
          // registration is probably about the
          // limit.
          //atlasToSubjectRegistrationHelper->SetMinimumStepLength(minimumStepSize);
          std::vector<std::string> transformType(1);
          transformType[0] = "Rigid";
          atlasToSubjectRegistrationHelper->SetTransformType(transformType);
          }
        else if( m_AtlasTransformType == "Affine" )
          {
          muLogMacro(
            << "Registering (Affine) " << preprocessMovingString << "atlas("
            << imMapIt->first << std::distance(imMapIt->second.begin(),imIt)
            << ") to template("
            << imMapIt->first << std::distance(imMapIt->second.begin(),imIt)
            << ") image." << std::endl);
          std::vector<double> minimumStepSize(1);
          minimumStepSize[0] = 0.0025;
          //atlasToSubjectRegistrationHelper->SetMinimumStepLength(minimumStepSize);
          std::vector<std::string> transformType(1);
          transformType[0] = "Affine";
          atlasToSubjectRegistrationHelper->SetTransformType(transformType);
          }
        else if( m_AtlasTransformType == "BSpline" )
          {
          muLogMacro(<< "Registering (BSpline) " << preprocessMovingString << "atlas("
                     << imMapIt->first << std::distance(imMapIt->second.begin(),imIt)
                     << ") to template("
                     << imMapIt->first << std::distance(imMapIt->second.begin(),imIt)
                     << ") image." << std::endl);
          std::vector<double> minimumStepSize(1);
          minimumStepSize[0] = 0.0025;
          //atlasToSubjectRegistrationHelper->SetMinimumStepLength(minimumStepSize);
          std::vector<std::string> transformType(1);
          transformType[0] = "BSpline";
          atlasToSubjectRegistrationHelper->SetTransformType(transformType);
          std::vector<int> splineGridSize(3);
          splineGridSize[0] = m_WarpGrid[0];
          splineGridSize[1] = m_WarpGrid[1];
          splineGridSize[2] = m_WarpGrid[2];
          atlasToSubjectRegistrationHelper->SetSplineGridSize(splineGridSize);
          // Setting max displace
          atlasToSubjectRegistrationHelper->SetMaxBSplineDisplacement(6.0);
          }
        else if( m_AtlasTransformType == "SyN" )
          {
          std::cerr << "ERROR:  NOT PROPERLY IMPLEMENTED YET HACK:" << std::endl;
          }

        CompositeTransformType::Pointer templateGenericCompositeTransform = dynamic_cast<CompositeTransformType *>( m_TemplateGenericTransform.GetPointer() );
        if( templateGenericCompositeTransform.IsNull() )
          {
          templateGenericCompositeTransform = CompositeTransformType::New();
          templateGenericCompositeTransform->AddTransform( m_TemplateGenericTransform );
          }
        atlasToSubjectRegistrationHelper->SetCurrentGenericTransform( templateGenericCompositeTransform );

        if( this->m_DebugLevel > 9 )
          {
          static unsigned int atlasToSubjectCounter = 0;
          std::stringstream   ss;
          ss << std::setw(3) << std::setfill('0') << atlasToSubjectCounter;
          atlasToSubjectRegistrationHelper->PrintCommandLine(true, std::string("AtlasToSubjectUpdate") + ss.str() );
          muLogMacro( << __FILE__ << " " << __LINE__ << " "  <<   std::endl );
          atlasToSubjectCounter++;
          }
        atlasToSubjectRegistrationHelper->Update();
        unsigned int actualIterations = atlasToSubjectRegistrationHelper->GetActualNumberOfIterations();
        muLogMacro( << "Registration tool " << actualIterations << " iterations." << std::endl );
        m_TemplateGenericTransform = atlasToSubjectRegistrationHelper->GetCurrentGenericTransform()->GetNthTransform(0);
        }
      }
    }
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::WritePartitionTable(const unsigned int CurrentEMIteration) const
{
  const unsigned int numPriors = this->m_WarpedPriors.size();

  muLogMacro(<< "\n\nEM iteration " << CurrentEMIteration <<  std::endl);
  muLogMacro(<< "---------------------" << std::endl);
  PrettyPrintTable EMIterationTable;
  {
  unsigned int ichan = 0;
  for(typename MapOfInputImageVectors::const_iterator mapIt = this->m_InputImages.begin();
      mapIt != this->m_InputImages.end(); ++mapIt)
      {
      EMIterationTable.add(0, ichan + 2, mapIt->first);
      ++ichan;
      }
  }
  for( unsigned int iclass = 0; iclass < numPriors; iclass++ )
    {
    unsigned int ichan = 0;
    EMIterationTable.add(iclass + 1, 0, std::string("Class ") + this->m_PriorNames[iclass] + " mean ");
    EMIterationTable.add(iclass + 1, 1, std::string(": ") );
    for(typename RegionStats::MeanMapType::const_iterator classIt =
          this->m_ListOfClassStatistics[iclass].m_Means.begin();
        classIt != this->m_ListOfClassStatistics[iclass].m_Means.end();
        ++classIt)
      {
      EMIterationTable.add(iclass + 1, ichan + 2, classIt->second);
      ++ichan;
      }
    }

  std::ostringstream oss;
  EMIterationTable.Print(oss);
  muLogMacro( << oss.str() );
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::WriteDebugForegroundMask(const ByteImageType::Pointer & currForgroundBrainMask,
                           const unsigned int CurrentEMIteration) const
{
  std::stringstream CurrentEMIteration_stream("");

  CurrentEMIteration_stream << CurrentEMIteration;
  typedef itk::ImageFileWriter<ByteImageType> WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->UseCompressionOn();

  const std::string fn = this->m_OutputDebugDir + "/MASK_LEVEL_" + CurrentEMIteration_stream.str() + ".nii.gz";
  writer->SetInput(currForgroundBrainMask);
  writer->SetFileName(fn.c_str() );
  writer->Update();
  muLogMacro( << "DEBUG:  Wrote image " << fn <<  std::endl );
}

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::Update()
{
  if( m_AtlasTransformType == "invalid_TransformationTypeNotSet" )
    {
    itkGenericExceptionMacro( << "The AtlasTransformType has NOT been set!" << std::endl );
    }
  if( m_UpdateRequired )
    {
    // TODO:  This should be filled out from the XML file eventually
    this->m_PriorsBackgroundValues.resize(this->m_OriginalSpacePriors.size() );
    std::fill(this->m_PriorsBackgroundValues.begin(), this->m_PriorsBackgroundValues.end(), 0);
    {
    // HACK:  In the XML file, each prior should also specify the default
    // background value
    //       to be used during the warping process.
    //       For AIR, it should be 1.0,  for all others is should be 0.0
    //
    m_PriorsBackgroundValues[this->GetAirIndex()] = 1;
    }

    this->EMLoop();
    m_UpdateRequired = false;
    }
}

/**
 * The EMLoop is the global algorithmic framework for
 * completing the iterative parts of the processing.
 */

template <class TInputImage, class TProbabilityImage>
void
EMSegmentationFilter<TInputImage, TProbabilityImage>
::EMLoop()
{
  if( this->m_TemplateGenericTransform.IsNull() )
    {
    itkExceptionMacro( << "ERROR:  Must suppply an intial transformation!" );
    }

  this->m_NonAirRegion = ComputeTissueRegion<TInputImage, ByteImageType>(this->GetFirstInputImage(), 3);
  if( this->m_DebugLevel > 9 )
    {
    this->WriteDebugHeadRegion(0);
    }

  this->m_WarpedPriors =
    WarpImageList(this->m_OriginalSpacePriors, this->GetFirstInputImage(),
                  this->m_PriorsBackgroundValues,
                  this->m_TemplateGenericTransform);
  if( this->m_DebugLevel > 9 )
    {
    this->WriteDebugWarpedAtlasPriors(0);
    this->m_WarpedAtlasImages =
      WarpImageList(this->m_OriginalAtlasImages,
                    this->GetFirstInputImage(),
                    this->m_TemplateGenericTransform);
    this->WriteDebugWarpedAtlasImages(0);
    }
  typename ByteImageType::Pointer
    currForgroundBrainMask = ComputeForegroundProbMask<TProbabilityImage>(this->m_WarpedPriors,
                                                                          this->m_PriorIsForegroundPriorVector);
  if( this->m_DebugLevel > 9 )
    {
    WriteDebugForegroundMask(currForgroundBrainMask, 0);
    }

  std::vector<ByteImagePointer> SubjectCandidateRegions =
    this->UpdateIntensityBasedClippingOfPriors(0,
                                               this->m_InputImages,
                                               this->m_WarpedPriors,
                                               currForgroundBrainMask);
  {
  this->BlendPosteriorsAndPriors(0.0, this->m_WarpedPriors, this->m_WarpedPriors, this->m_WarpedPriors);
  NormalizeProbListInPlace<TProbabilityImage>(this->m_WarpedPriors);
  if( this->m_DebugLevel > 9 )
    {
    this->WriteDebugBlendClippedPriors(0);
    }
  }

  // NOTE:  Labels are only needed if debugging them.
  ComputeLabels<TProbabilityImage, ByteImageType, double>(this->m_WarpedPriors, this->m_PriorIsForegroundPriorVector,
                                                          this->m_PriorLabelCodeVector, this->m_NonAirRegion,
                                                          this->m_DirtyLabels,
                                                          this->m_CleanedLabels);
  FloatingPrecision inclusionThreshold = 0.75F;
  ComputeLabels<TProbabilityImage, ByteImageType, double>(this->m_WarpedPriors, this->m_PriorIsForegroundPriorVector,
                                                          this->m_PriorLabelCodeVector, this->m_NonAirRegion,
                                                          this->m_DirtyThresholdedLabels,
                                                          this->m_ThresholdedLabels, inclusionThreshold);

  this->WriteDebugLabels(0);
//  this->m_ListOfClassStatistics.resize(0); // Reset this to empty for debugging
                                           // purposes to induce failures when
                                           // being re-used.

  // Compute BiasCorrected image for the first time.
  this->m_CorrectedImages =
    CorrectBias(1, 0, SubjectCandidateRegions, this->m_InputImages, this->m_CleanedLabels, this->m_NonAirRegion,
                this->m_WarpedPriors, this->m_PriorUseForBiasVector, this->m_SampleSpacing, this->m_DebugLevel,
                this->m_OutputDebugDir);
  WriteDebugCorrectedImages(this->m_CorrectedImages, 0);

//
//  // IPEK -- this is the place where the covariance is generated
//  this->m_ListOfClassStatistics = this->ComputeDistributions(SubjectCandidateRegions, this->m_WarpedPriors);
  //this->WritePartitionTable(0);
//  {
//  // Now check that the intraSubjectOriginalImageList has positive definite
//  // covariance matrix.
//  // The algorithm is not stable if the covariance matrix is not positive
//  // definite, and this
//  // occurs when two or more of the images are linearly dependant (i.e. nearly
//  // the same image).
//  for( unsigned int q = 0; q < this->m_ListOfClassStatistics.size(); q++ )
//    {
//    try
//      {
//      ComputeCovarianceDeterminant( this->m_ListOfClassStatistics[q].m_Covariance );
//      }
//    catch( itk::ExceptionObject &excp)
//      {
//      std::cerr << "Error computing covariance " << std::endl;
//      std::cerr << excp << std::endl;
//      throw;
//      }
//    catch( ... )
//      {
//      itkExceptionMacro( << "ERROR:\nERROR:\nERROR:\nERROR:"
//                         << " Linearly dependant input images detected. "
//                         << "Please remove the images in the above table that show very similar values images."
//                         << "ERROR:\nERROR:\nERROR:\nERROR:" );
//      }
//    }
//  }
//
  muLogMacro(<< "\n\nEM iteration 0" << std::endl);
  muLogMacro(<< "---------------------" << std::endl);
  this->CheckInput();

  // FloatingPrecision logLikelihood = vnl_huge_val(1.0);
  FloatingPrecision logLikelihood = 1.0 / vnl_math::eps;
  FloatingPrecision deltaLogLikelihood = 1.0;

  unsigned int biasdegree = 0;

  // EM loop
  bool   converged = false;
  double priorWeighting = 1.00;       // NOTE:  This turns off blending of
                                      // posteriors and priors when set to 1.0,
                                      // thus short-circuting the system.
    unsigned int CurrentEMIteration = 1;
//=======================================================

  unsigned int NumberOfSamples =  this->m_ThresholdedLabels->GetBufferedRegion().GetNumberOfPixels();
  muLogMacro(<< "\nTotal number of voxels: " << NumberOfSamples << std::endl);
  NumberOfSamples = NumberOfSamples * 0.1;
  muLogMacro(<< "Number of samples used to make train matrix: " << NumberOfSamples << std::endl);
  double DownSamplingFactor = 3;
  muLogMacro(<< "\nDownsampling Factor: " << DownSamplingFactor << std::endl);

  m_MaximumIterations = 2; ////////DEBUG
  while( !converged && ( CurrentEMIteration <= m_MaximumIterations ) )
    {
    // Recompute posteriors, not at full resolution
//    this->m_Posteriors =
//      this->ComputePosteriors(this->m_WarpedPriors, this->m_PriorWeights,
//                              this->m_CorrectedImages,
//                              this->m_ListOfClassStatistics);
    muLogMacro(<< "\nStart computing posteriors at iteration " << CurrentEMIteration
              << " using " << NumberOfSamples << " samples.\n" << std::endl);

    this->m_Posteriors = this->ComputekNNPosteriors(this->m_WarpedPriors,
                                                    this->m_CorrectedImages,
                                                    this->m_ThresholdedLabels,
                                                    NumberOfSamples,
                                                    DownSamplingFactor);

    NormalizeProbListInPlace<TProbabilityImage>(this->m_Posteriors);
    this->WriteDebugPosteriors(CurrentEMIteration);
    ComputeLabels<TProbabilityImage, ByteImageType, double>(this->m_Posteriors, this->m_PriorIsForegroundPriorVector,
                                                            this->m_PriorLabelCodeVector, this->m_NonAirRegion,
                                                            this->m_DirtyLabels,
                                                            this->m_CleanedLabels);
    this->WriteDebugLabels(CurrentEMIteration);
    muLogMacro(<< "m_CorrectedImages " << std::endl);
    this->m_CorrectedImages =
      CorrectBias(this->m_MaxBiasDegree, CurrentEMIteration, SubjectCandidateRegions, this->m_InputImages,
                  this->m_CleanedLabels, this->m_NonAirRegion, this->m_Posteriors, this->m_PriorUseForBiasVector,
                  this->m_SampleSpacing, this->m_DebugLevel,
                  this->m_OutputDebugDir);
    WriteDebugCorrectedImages(this->m_CorrectedImages, CurrentEMIteration);
//    this->m_ListOfClassStatistics.resize(0); // Reset this to empty for
                                             // debugging purposes to induce
                                             // failures when being re-used.
//    this->m_ListOfClassStatistics = this->ComputeDistributions(SubjectCandidateRegions, this->m_Posteriors);
//    this->WritePartitionTable(CurrentEMIteration);
    muLogMacro(<< "\n\nEM iteration " << CurrentEMIteration <<  std::endl);
    muLogMacro(<< "---------------------" << std::endl);

    // Now update transformation and estimates of probability regions based on
    // current knowledge.
    /*
    {
    this->UpdateTransformation(CurrentEMIteration); // This changes the class
    // value of
    //
    // m_TemplateGenericTransform.
    this->m_WarpedPriors =
      WarpImageList(this->m_OriginalSpacePriors,
                    this->GetFirstInputImage(),
                    this->m_PriorsBackgroundValues,
                    this->m_TemplateGenericTransform);
    if( this->m_DebugLevel > 9 )
      {
      this->WriteDebugWarpedAtlasPriors(CurrentEMIteration);
      this->m_WarpedAtlasImages =
        WarpImageList(this->m_OriginalAtlasImages,
                      this->GetFirstInputImage(),
                      this->m_TemplateGenericTransform);
      this->WriteDebugWarpedAtlasImages(CurrentEMIteration);
      }
    SubjectCandidateRegions = this->ForceToOne(CurrentEMIteration,
                                               // this->m_CorrectedImages,
                                               this->m_WarpedPriors,
                                               this->m_CleanedLabels);
    */
    {
    this->BlendPosteriorsAndPriors(1.0 - priorWeighting, this->m_Posteriors, this->m_WarpedPriors,
                                   this->m_WarpedPriors);
    priorWeighting *= priorWeighting;
    NormalizeProbListInPlace<TProbabilityImage>(this->m_WarpedPriors);
    this->WriteDebugBlendClippedPriors(CurrentEMIteration);
    }
//    }

    FloatingPrecision prevLogLikelihood = ( logLikelihood < vnl_math::eps ) ? vnl_math::eps : logLikelihood;
    // Compute log-likelihood and normalize posteriors
    logLikelihood = this->ComputeLogLikelihood();
    muLogMacro(<< "log(likelihood) = " << logLikelihood <<  std::endl);
    // TODO: move to before prevL update
    deltaLogLikelihood = vcl_fabs( (logLikelihood - prevLogLikelihood) / prevLogLikelihood);
    // (logLikelihood - prevLogLikelihood) / vcl_fabs(prevLogLikelihood);
    CHECK_NAN(deltaLogLikelihood, __FILE__, __LINE__,
              "\n logLikelihood: " << logLikelihood << "\n prevLogLikelihood: " << prevLogLikelihood );
    muLogMacro(
      << "delta vcl_log(likelihood) = " << deltaLogLikelihood << "  Convergence Tolerance: "
      << m_WarpLikelihoodTolerance <<  std::endl);

    // Convergence check
    converged = (CurrentEMIteration >= m_MaximumIterations)
      // Ignore jumps in the vcl_log likelihood
      //    ||
      //    (deltaLogLikelihood < 0)
      ||
      ( (deltaLogLikelihood < m_LikelihoodTolerance)
        &&
        (biasdegree == m_MaxBiasDegree) );

    CurrentEMIteration++;
    const float biasIncrementInterval = (m_MaximumIterations / (m_MaxBiasDegree + 1) );
    CHECK_NAN(biasIncrementInterval, __FILE__, __LINE__,
              "\n m_MaximumIterations: " << m_MaximumIterations << "\n  m_MaxBiasDegree: " << m_MaxBiasDegree );
    // Bias correction
    if( m_MaxBiasDegree > 0 )
      {
      if( (
            (deltaLogLikelihood < m_BiasLikelihoodTolerance)
            || ( CurrentEMIteration > (biasdegree + 1) * biasIncrementInterval) )
          &&
          (biasdegree < m_MaxBiasDegree)
        )
        {
        biasdegree++;
        }
      }

    NumberOfSamples = NumberOfSamples * 1.5;
    DownSamplingFactor = DownSamplingFactor / 1.2;

    } // end EM loop

  muLogMacro(<< "Done computing posteriors with " << CurrentEMIteration << " iterations" << std::endl);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////// compute posteriors using kNN ////////////////////////////////////////////////

  ComputeLabels<TProbabilityImage, ByteImageType, double>(this->m_Posteriors, this->m_PriorIsForegroundPriorVector,
                                                          this->m_PriorLabelCodeVector, this->m_NonAirRegion,
                                                          this->m_DirtyLabels,
                                                          this->m_CleanedLabels);
  //FloatingPrecision inclusionThreshold = 0.75F;
  ComputeLabels<TProbabilityImage, ByteImageType, double>(this->m_Posteriors, this->m_PriorIsForegroundPriorVector,
                                                          this->m_PriorLabelCodeVector, this->m_NonAirRegion,
                                                          this->m_DirtyThresholdedLabels,
                                                          this->m_ThresholdedLabels, inclusionThreshold);
/*
  this->m_Posteriors = this->ComputekNNPosteriors(this->m_WarpedPriors,
                                                  this->m_CorrectedImages,
                                                  this->m_ThresholdedLabels,
                                                  NumberOfSamples,
                                                  DownSamplingFactor); //// ??????????????? again needed???
*/
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  NormalizeProbListInPlace<TProbabilityImage>(this->m_Posteriors);
  this->WriteDebugPosteriors(CurrentEMIteration + 100);

  this->WriteDebugLabels(CurrentEMIteration + 100);

  // Bias correction at full resolution, still using downsampled images
  // for computing the bias field coeficients
  if( m_MaxBiasDegree > 0 )
    {
    this->m_CorrectedImages =
      CorrectBias(m_MaxBiasDegree, CurrentEMIteration + 100, SubjectCandidateRegions, this->m_InputImages,
                  this->m_CleanedLabels, this->m_NonAirRegion, this->m_Posteriors, this->m_PriorUseForBiasVector,
                  this->m_SampleSpacing, this->m_DebugLevel,
                  this->m_OutputDebugDir);
    WriteDebugCorrectedImages(this->m_CorrectedImages, CurrentEMIteration + 100);
//    this->m_ListOfClassStatistics.resize(0); // Reset this to empty for
                                             // debugging purposes to induce
                                             // failures when being re-used.
//    this->m_ListOfClassStatistics = this->ComputeDistributions(SubjectCandidateRegions, this->m_Posteriors);
    this->m_RawCorrectedImages =
      CorrectBias(m_MaxBiasDegree, CurrentEMIteration + 100, SubjectCandidateRegions, this->m_RawInputImages,
                  this->m_CleanedLabels, this->m_NonAirRegion, this->m_Posteriors, this->m_PriorUseForBiasVector,
                  this->m_SampleSpacing, this->m_DebugLevel,
                  this->m_OutputDebugDir);
    }
//  this->m_ListOfClassStatistics.resize(0); // Reset this to empty for debugging
                                           // purposes to induce failures when
                                           // being re-used.
//  this->m_ListOfClassStatistics = this->ComputeDistributions(SubjectCandidateRegions, this->m_Posteriors);

//  this->WritePartitionTable(0 + 100);
  muLogMacro(<< "\n\nEM iteration " << CurrentEMIteration + 100 <<  std::endl);
  muLogMacro(<< "---------------------" << std::endl);
}

template <class TInputImage, class TProbabilityImage>
typename EMSegmentationFilter<TInputImage, TProbabilityImage>::MapOfInputImageVectors
EMSegmentationFilter<TInputImage, TProbabilityImage>
::CorrectBias(const unsigned int degree,
              const unsigned int CurrentEMIteration,
              const ByteImageVectorType & CandidateRegions,
              MapOfInputImageVectors & inputImages,
              const ByteImageType::Pointer currentBrainMask,
              const ByteImageType::Pointer currentForegroundMask,
              const ProbabilityImageVectorType & probImages,
              const BoolVectorType & probUseForBias,
              const FloatingPrecision sampleSpacing,
              const int DebugLevel,
              const std::string& OutputDebugDir)
{

  if( degree == 0 )
    {
    muLogMacro(<< "Skipping Bias correction, polynomial degree = " << degree <<  std::endl);
    return inputImages;
    }
  muLogMacro(<< "Bias correction, polynomial degree = " << degree <<  std::endl);

  // Perform bias correction
  const unsigned int                   numClasses = probImages.size();
  std::vector<FloatImageType::Pointer> biasPosteriors;
  std::vector<ByteImageType::Pointer>  biasCandidateRegions;

  for( unsigned int iclass = 0; iclass < numClasses; iclass++ )
    {
    const unsigned iprior = iclass;
    if( probUseForBias[iprior] == 1 )
      {
      // Focus only on FG classes, more accurate if bg classification is bad
      // but sacrifices accuracy in border regions (tend to overcorrect)
      biasPosteriors.push_back(probImages[iclass]);
      biasCandidateRegions.push_back(CandidateRegions[iclass]);
      }
    }

  itk::TimeProbe BiasCorrectorTimer;
  BiasCorrectorTimer.Start();
  typedef LLSBiasCorrector<CorrectIntensityImageType, FloatImageType> BiasCorrectorType;
  typedef BiasCorrectorType::Pointer                                  BiasCorrectorPointer;

  BiasCorrectorPointer biascorr = BiasCorrectorType::New();
  biascorr->SetMaxDegree(degree);
  // biascorr->SetMaximumBiasMagnitude(5.0);
  // biascorr->SetSampleSpacing(2.0*SampleSpacing);
  biascorr->SetSampleSpacing(1);
  biascorr->SetWorkingSpacing(sampleSpacing);
  biascorr->SetForegroundBrainMask(currentBrainMask);
  biascorr->SetAllTissueMask(currentForegroundMask);
  biascorr->SetProbabilities(biasPosteriors, biasCandidateRegions);
  biascorr->SetDebugLevel(DebugLevel);
  biascorr->SetOutputDebugDir(OutputDebugDir);

  if( DebugLevel > 0 )
    {
    biascorr->DebugOn();
    }

  biascorr->SetInputImages(inputImages);
  MapOfInputImageVectors correctedImages = biascorr->CorrectImages(CurrentEMIteration);

  BiasCorrectorTimer.Stop();
  itk::RealTimeClock::TimeStampType elapsedTime = BiasCorrectorTimer.GetTotal();
  muLogMacro(<< "Computing BiasCorrection took " << elapsedTime << " " << BiasCorrectorTimer.GetUnit() << std::endl);

  return correctedImages;
}
#endif
