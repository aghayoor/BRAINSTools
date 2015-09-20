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
 * at Psychiatry Imaging Lab,
 * University of Iowa Health Care 2011
 */

#include "itkIntegrityMetricMembershipFunction.h"

int main( int argc, char * argv[] )
{

  const float threshold = 2.5;

  // create a sample test ListSample object
  const unsigned int MeasurementVectorLength = 2; // number of modality images
  typedef itk::Vector< float, MeasurementVectorLength > MeasurementVectorType;
  typedef itk::Statistics::ListSample< MeasurementVectorType > SampleType;
  SampleType::Pointer sample = SampleType::New();
  sample->SetMeasurementVectorSize( MeasurementVectorLength );
  MeasurementVectorType mv;
  mv[0] = 0;
  mv[1] = 1;
  sample->PushBack( mv );

  mv[0] = 0;
  mv[1] = -1;
  sample->PushBack( mv );

  mv[0] = 3;
  mv[1] = 1;
  sample->PushBack( mv );

  mv[0] = -3;
  mv[1] = 1;
  sample->PushBack( mv );

  mv[0] = 3;
  mv[1] = -1;
  sample->PushBack( mv );

  mv[0] = -3;
  mv[1] = -1;
  sample->PushBack( mv );

  mv[0] = -3;
  mv[1] = 0;
  sample->PushBack( mv );

  mv[0] = 3;
  mv[1] = 0;
  sample->PushBack( mv );

  mv[0] = 0;
  mv[1] = 3;
  sample->PushBack( mv );

  // compute mean and covariance
  typedef itk::Statistics::IntegrityMetricMembershipFunction< SampleType > IntegrityMetricType;
  IntegrityMetricType::Pointer integrityMetric = IntegrityMetricType::New();
  integrityMetric->SetThreshold( threshold );
  bool ispure = integrityMetric->Evaluate( sample );

  IntegrityMetricType::MeanVectorType mean = integrityMetric->GetMean();
  IntegrityMetricType::CovarianceMatrixType cov = integrityMetric->GetCovariance();

  std::cout << "Mean = " << mean << std::endl;
  std::cout << "Covariance = " << cov << std::endl;
  std::cout << "Is pure: " << ispure << std::endl;

  return EXIT_SUCCESS;
}
