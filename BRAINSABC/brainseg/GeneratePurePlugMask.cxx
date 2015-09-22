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

#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkNearestNeighborInterpolateImageFunction.h"

#include "itkIntegrityMetricMembershipFunction.h"

#include "GeneratePurePlugMaskCLP.h"

int main( int argc, char * argv[] )
{
  PARSE_ARGS;

  typedef itk::Image<float, 3>                                                FloatImageType;
  typedef FloatImageType::Pointer                                             FloatImagePointer;
  typedef std::vector<FloatImagePointer>                                      InputImageList;
  typedef itk::ImageFileReader<FloatImageType>                                LocalReaderType;
  typedef itk::RescaleIntensityImageFilter< FloatImageType, FloatImageType >  RescaleFilterType;
  typedef itk::Image<unsigned char, 3>                                        MaskImageType;

  typedef itk::NearestNeighborInterpolateImageFunction< FloatImageType, double > NNInterpolationType;

  typedef itk::VariableLengthVector< float >                               MeasurementVectorType;
  typedef itk::Statistics::ListSample< MeasurementVectorType >             SampleType;
  typedef itk::Statistics::IntegrityMetricMembershipFunction< SampleType > IntegrityMetricType;

  std::vector<std::string> inputFileNames;
  if( inputImageModalities.size() > 1 )
    {
    inputFileNames = inputImageModalities;
    }
  else
    {
    std::cerr << "ERROR: At least two image modalities are needed to generate pure plug mask." << std::endl;
    return EXIT_FAILURE;
    }
  const unsigned int numberOfImageModalities = inputFileNames.size(); // number of modality images
  //const float threshold( Threshold ); // threshold value

  // Read the input modalities and set them in a vector of images
  typedef LocalReaderType::Pointer             LocalReaderPointer;

  MaskImageType::SpacingType maskSpacing;
  maskSpacing.Fill(0);

  InputImageList inputImageModalitiesList;
  for( unsigned int i = 0; i < numberOfImageModalities; i++ )
     {
     std::cout << "Reading image: " << inputFileNames[i] << std::endl;

     LocalReaderPointer imgreader = LocalReaderType::New();
     imgreader->SetFileName( inputFileNames[i].c_str() );

     try
       {
       imgreader->Update();
       }
     catch( ... )
       {
       std::cout << "ERROR:  Could not read image " << inputFileNames[i] << "." << std::endl;
       return EXIT_FAILURE;
       }

     // Rescale intensity range of input images between 0 and 1
     RescaleFilterType::Pointer rescaleFilter = RescaleFilterType::New();
     rescaleFilter->SetInput( imgreader->GetOutput() );
     rescaleFilter->SetOutputMinimum(0);
     rescaleFilter->SetOutputMaximum(1);
     rescaleFilter->Update();

     FloatImagePointer currImage = rescaleFilter->GetOutput();
     inputImageModalitiesList.push_back( currImage );

     // Pure plug mask should have the highest spacing (lowest resolution) at each direction
     FloatImageType::SpacingType currImageSpacing = currImage->GetSpacing();
     for( unsigned int s = 0; s < 3; s++ )
        {
        if( currImageSpacing[s] > maskSpacing[s] )
          {
          maskSpacing[s] = currImageSpacing[s];
          }
        }
     }

  // Create an empty image for mask
  //
  MaskImageType::Pointer mask = MaskImageType::New();
  // Spacing is set as the largest spacing at each direction
  mask->SetSpacing( maskSpacing );
  // Origin and direction are set from the first modality image
  mask->SetOrigin( inputImageModalitiesList[0]->GetOrigin() );
  mask->SetDirection( inputImageModalitiesList[0]->GetDirection() );
  // The FOV of mask is set as the FOV of the first modality image
  MaskImageType::SizeType maskSize;
  FloatImageType::SizeType inputSize = inputImageModalitiesList[0]->GetLargestPossibleRegion().GetSize();
  FloatImageType::SpacingType inputSpacing = inputImageModalitiesList[0]->GetSpacing();
  maskSize[0] = itk::Math::Ceil<itk::SizeValueType>( inputSize[0]*inputSpacing[0]/maskSpacing[0] );
  maskSize[1] = itk::Math::Ceil<itk::SizeValueType>( inputSize[1]*inputSpacing[1]/maskSpacing[1] );
  maskSize[2] = itk::Math::Ceil<itk::SizeValueType>( inputSize[2]*inputSpacing[2]/maskSpacing[2] );
  // mask start index
  MaskImageType::IndexType maskStart;
  maskStart.Fill(0);
  // Set mask region
  MaskImageType::RegionType maskRegion(maskStart, maskSize);
  mask->SetRegions( maskRegion );
  mask->Allocate();
  mask->FillBuffer(0);
  //------------------------------------------

  IntegrityMetricType::Pointer integrityMetric = IntegrityMetricType::New();
  integrityMetric->SetThreshold( threshold );

  // define step size based on the number of sub-samples at each direction
  MaskImageType::SizeType numberOfContinuousIndexSubSamples;
  numberOfContinuousIndexSubSamples[0] = numberOfSubSamples[0];
  numberOfContinuousIndexSubSamples[1] = numberOfSubSamples[1];
  numberOfContinuousIndexSubSamples[2] = numberOfSubSamples[2];

  MaskImageType::SpacingType stepSize;
  stepSize[0] = maskSpacing[0]/numberOfContinuousIndexSubSamples[0];
  stepSize[1] = maskSpacing[1]/numberOfContinuousIndexSubSamples[1];
  stepSize[2] = maskSpacing[2]/numberOfContinuousIndexSubSamples[2];

  // Now iterate through the mask image
  typedef itk::ImageRegionIteratorWithIndex< MaskImageType > MaskItType;
  MaskItType maskIt( mask, mask->GetLargestPossibleRegion() );
  maskIt.GoToBegin();

  while( ! maskIt.IsAtEnd() )
   {
   MaskImageType::IndexType idx = maskIt.GetIndex();

   // A sample list is created for each index that is inside the images buffers
   SampleType::Pointer sample = SampleType::New();
   sample->SetMeasurementVectorSize( numberOfImageModalities );

   // flag that helps to break from loops if the current continous index is
   // not inside the buffer of any input modality image
   bool isInside = true;

   // around each mask index, subsamples are taken as continues indices
   for( double iss = idx[0]-(maskSpacing[0]/2)+(stepSize[0]/2); iss < idx[0]+(maskSpacing[0]/2); iss += stepSize[0] )
      {
      for( double jss = idx[1]-(maskSpacing[1]/2)+(stepSize[1]/2); jss < idx[1]+(maskSpacing[1]/2); jss += stepSize[1] )
         {
         for( double kss = idx[2]-(maskSpacing[2]/2)+(stepSize[2]/2); kss < idx[2]+(maskSpacing[2]/2); kss += stepSize[2] )
            {
            /////////
            itk::ContinuousIndex<double,3> cidx;
            cidx[0] = iss;
            cidx[1] = jss;
            cidx[2] = kss;

            // All input modality images are aligned in physical space,
            // so we need to transform each continuous index to physical point,
            // so it represent the same location in all input images
            FloatImageType::PointType p;
            inputImageModalitiesList[0]->TransformContinuousIndexToPhysicalPoint(cidx, p);

            MeasurementVectorType mv;
            mv.SetSize( numberOfImageModalities );

            for( unsigned int i = 0; i < numberOfImageModalities; i++ )
              {
              NNInterpolationType::Pointer imInterp = NNInterpolationType::New();
              imInterp->SetInputImage( inputImageModalitiesList[i] );
              if( imInterp->IsInsideBuffer(p) )
                {
                mv[i] = imInterp->Evaluate(p);
                }
              else
                {
                isInside = false;
                break;
                }
              }
            if( isInside )
              {
              sample->PushBack( mv );
              }
            else
              {
              break;
              }
            //////// end of nested loop 3
            }
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
     if( integrityMetric->Evaluate( sample ) )
       {
       maskIt.Set(1);
       }
     }

   ++maskIt;
   } // end of mask iterator

  typedef itk::ImageFileWriter<MaskImageType> MaskWriterType;
  MaskWriterType::Pointer writer = MaskWriterType::New();
  writer->SetInput( mask );
  writer->SetFileName( outputMaskFile );
  writer->UseCompressionOn();
  writer->Update();

  return EXIT_SUCCESS;
}
