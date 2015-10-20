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
/*
 * Author: Ali Ghayoor
 * at SINAPSE Lab,
 * The University of Iowa 2015
 */
#ifndef __GeneratePurePlugMask_h
#define __GeneratePurePlugMask_h

#include "BRAINSABCUtilities.h"
#include "itkIntegrityMetricMembershipFunction.h"

template <class InputImageType, class ByteImageType>
typename ByteImageType::Pointer
GeneratePurePlugMask(const std::vector<typename InputImageType::Pointer> & inputImages,
                     const float threshold,
                     const typename ByteImageType::SizeType & numberOfContinuousIndexSubSamples,
                     bool areInputsNormalized,
                     bool verbose = false) // verbose was only used for debugging purposes, should always be false.
{
  typedef typename itk::NearestNeighborInterpolateImageFunction<
    InputImageType, double >                                               InputImageNNInterpolationType;
  typedef std::vector<typename InputImageNNInterpolationType::Pointer>     InputImageInterpolatorVector;
  typedef std::vector<typename InputImageType::Pointer>                    InputImageVector;
  typedef itk::Array< double >                                             MeasurementVectorType;
  typedef itk::Statistics::ListSample< MeasurementVectorType >             SampleType;
  typedef itk::Statistics::IntegrityMetricMembershipFunction< SampleType > IntegrityMetricType;

  muLogMacro(<< "\nGenerating pure plug mask..." << std::endl);
  muLogMacro(<< "Threshold value is set to: " << threshold << std::endl);
  muLogMacro(<< "Input number of subsamples per each direction in voxel space: "
             << numberOfContinuousIndexSubSamples << std::endl);

  itk::TimeProbe PurePlugsMaskTimer;
  PurePlugsMaskTimer.Start();

  const unsigned int numberOfImageModalities =
    inputImages.size(); // number of modality images

  InputImageVector                 normalizedInputModalImagesList( numberOfImageModalities );
  InputImageInterpolatorVector     inputImageNNInterpolatorsVector( numberOfImageModalities );

  typename ByteImageType::SpacingType maskSpacing;
  maskSpacing.Fill(0);

  typename ByteImageType::SpacingType minimumSpacing;
  minimumSpacing.Fill(vcl_numeric_limits<double>::max());

  for( size_t i = 0; i < numberOfImageModalities; i++ )
    {
    // Generation of the pure plug mask needs the input images being normalized between 0 and 1.
    if( !areInputsNormalized )
      {
      normalizedInputModalImagesList[i] = NormalizeInputIntensityImage<InputImageType>( inputImages[i] );
      }
    else
      {
      normalizedInputModalImagesList[i] = inputImages[i];
      }
    // create a vector of input image interpolators for evaluation of image values in physical space
    typename InputImageNNInterpolationType::Pointer inputImageInterp =
      InputImageNNInterpolationType::New();
    inputImageInterp->SetInputImage( normalizedInputModalImagesList[i] );
    inputImageNNInterpolatorsVector[i] = inputImageInterp;

    // Pure plug mask should have the highest spacing (lowest resolution) at each direction
    typename InputImageType::SpacingType currImageSpacing = normalizedInputModalImagesList[i]->GetSpacing();
    for( size_t s = 0; s < 3; s++ )
      {
      if( currImageSpacing[s] > maskSpacing[s] )
        {
        maskSpacing[s] = currImageSpacing[s];
        }
      if( currImageSpacing[s] < minimumSpacing[s] )
        {
        minimumSpacing[s] = currImageSpacing[s];
        }
      }
    }

  // if invalid values are passed as numberOfContinuousIndexSubSamples,
  // we recompute that as the ratio between lowest resolution to highest resolution
  bool recomputeNumberOfSubSamples = false;
  for( size_t i = 0; i < 3; i++ )
    {
    if( numberOfContinuousIndexSubSamples[i] <= 0 )
      {
      recomputeNumberOfSubSamples = true;
      }
    }
  typename ByteImageType::SizeType numberOfSubSamples;
  if( recomputeNumberOfSubSamples )
    {
    numberOfSubSamples[0] = itk::Math::Round<itk::SizeValueType>( maskSpacing[0]/minimumSpacing[0] );
    numberOfSubSamples[1] = itk::Math::Round<itk::SizeValueType>( maskSpacing[1]/minimumSpacing[1] );
    numberOfSubSamples[2] = itk::Math::Round<itk::SizeValueType>( maskSpacing[2]/minimumSpacing[2] );
    muLogMacro(<< "\nWARNING: Input number of subsamples in voxel space are invalid." << std::endl
               << "Number of subsamples per each direction in voxel space are recomputed as: "
               << numberOfSubSamples << std::endl);
    }
  else
    {
    numberOfSubSamples[0] = numberOfContinuousIndexSubSamples[0];
    numberOfSubSamples[1] = numberOfContinuousIndexSubSamples[1];
    numberOfSubSamples[2] = numberOfContinuousIndexSubSamples[2];
    }

  /*
   * Create an all zero mask image
   */
  typename ByteImageType::Pointer mask = ByteImageType::New();
  // Spacing is set as the largest spacing at each direction
  mask->SetSpacing( maskSpacing );
  // Origin and direction are set from the first modality image
  mask->SetOrigin( normalizedInputModalImagesList[0]->GetOrigin() );
  mask->SetDirection( normalizedInputModalImagesList[0]->GetDirection() );
  // The FOV of mask is set as the FOV of the first modality image
  typename ByteImageType::SizeType maskSize;
  typename InputImageType::SizeType inputSize = normalizedInputModalImagesList[0]->GetLargestPossibleRegion().GetSize();
  typename InputImageType::SpacingType inputSpacing = normalizedInputModalImagesList[0]->GetSpacing();
  maskSize[0] = itk::Math::Ceil<itk::SizeValueType>( inputSize[0]*inputSpacing[0]/maskSpacing[0] );
  maskSize[1] = itk::Math::Ceil<itk::SizeValueType>( inputSize[1]*inputSpacing[1]/maskSpacing[1] );
  maskSize[2] = itk::Math::Ceil<itk::SizeValueType>( inputSize[2]*inputSpacing[2]/maskSpacing[2] );
  // mask start index
  typename ByteImageType::IndexType maskStart;
  maskStart.Fill(0);
  // Set mask region
  typename ByteImageType::RegionType maskRegion(maskStart, maskSize);
  mask->SetRegions( maskRegion );
  mask->Allocate();
  mask->FillBuffer(0);
  ///////////////////////

  IntegrityMetricType::Pointer integrityMetric = IntegrityMetricType::New();
  integrityMetric->SetThreshold( threshold );

  // define step size based on the number of sub-samples at each direction
  typename ByteImageType::SpacingType stepSize;
  stepSize[0] = maskSpacing[0]/numberOfSubSamples[0];
  stepSize[1] = maskSpacing[1]/numberOfSubSamples[1];
  stepSize[2] = maskSpacing[2]/numberOfSubSamples[2];

  // Now iterate through the mask image
  typedef typename itk::ImageRegionIteratorWithIndex< ByteImageType > MaskItType;
  MaskItType maskIt( mask, mask->GetLargestPossibleRegion() );
  maskIt.GoToBegin();

  while( ! maskIt.IsAtEnd() )
    {
    typename ByteImageType::IndexType idx = maskIt.GetIndex();

    if( verbose )
      {
      muLogMacro(<< "current index: " << idx << std::endl);
      }

    // A sample list is created for every index that is inside all input images buffers
    SampleType::Pointer sample = SampleType::New();
    sample->SetMeasurementVectorSize( numberOfImageModalities );

    // flag that helps to break from loops if the current continous index is
    // not inside the buffer of any input modality image
    bool isInside = true;

    // subsampling is performed in voxel space around each mask index.
    // subsamples are taken as continues indices
    for( double iss = idx[0]-(maskSpacing[0]/2)+(stepSize[0]/2); iss < idx[0]+(maskSpacing[0]/2); iss += stepSize[0] )
      {
      for( double jss = idx[1]-(maskSpacing[1]/2)+(stepSize[1]/2); jss < idx[1]+(maskSpacing[1]/2); jss += stepSize[1] )
        {
        for( double kss = idx[2]-(maskSpacing[2]/2)+(stepSize[2]/2); kss < idx[2]+(maskSpacing[2]/2); kss += stepSize[2] )
          {
          itk::ContinuousIndex<double,3> cidx;
          cidx[0] = iss;
          cidx[1] = jss;
          cidx[2] = kss;

          // All input modality images are aligned in physical space,
          // so we need to transform each continuous index to physical point,
          // so it represent the same location in all input images
          typename ByteImageType::PointType currPoint;
          mask->TransformContinuousIndexToPhysicalPoint(cidx, currPoint);

          if( verbose )
            {
            muLogMacro(<< "current continous index: " << cidx << std::endl);
            muLogMacro(<< "current physical point: " << currPoint << std::endl);
            }

          MeasurementVectorType mv( numberOfImageModalities );

          for( unsigned int i = 0; i < numberOfImageModalities; i++ )
            {
            if( inputImageNNInterpolatorsVector[i]->IsInsideBuffer( currPoint ) )
              {
              mv[i] = inputImageNNInterpolatorsVector[i]->Evaluate( currPoint );
              }
            else
              {
              isInside = false;
              break;
              }
            }

          if( isInside )
            {
            if( verbose )
              {
              muLogMacro(<< "Is inside, measurement vector: " << mv << std::endl);
              }
            sample->PushBack( mv );
            }
          else
            {
            if( verbose )
              {
              muLogMacro(<< "Is not inside! so not pure!" << std::endl);
              }
            break;
            }
          } // end of nested loop 3
        if( !isInside )
          {
          break;
          }
        } // end of nested loop 2
      if( !isInside )
        {
        break;
        }
      } // end of nested loop 1

    if( isInside )
      {
      if( verbose )
        {
        muLogMacro(<< "Integrity filter: " << std::endl);
        integrityMetric->Print(std::cout);
        }
      if( integrityMetric->Evaluate( sample ) )
        {
        maskIt.Set(1);
        if( verbose )
          {
          muLogMacro(<< "Is pure: True" << std::endl);
          }
        }
      else
        {
        if( verbose )
          {
          muLogMacro(<< "Is pure: False" << std::endl);
          }
        }
      }
    if( verbose )
      {
      muLogMacro(<< "-----------" << std::endl);
      }

    ++maskIt;
    } // end of while loop

  PurePlugsMaskTimer.Stop();
  itk::RealTimeClock::TimeStampType elapsedTime = PurePlugsMaskTimer.GetTotal();
  muLogMacro(<< "Generating pure plugs mask took " << elapsedTime
             << " " << PurePlugsMaskTimer.GetUnit() << "." << std::endl);

  return mask;
}

#endif // __GeneratePurePlugMask_h
